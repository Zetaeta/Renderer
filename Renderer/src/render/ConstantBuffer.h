#pragma once

#include <core/Types.h>
#include <core/TypeInfoUtils.h>
#include <core/Maths.h>
#include <core/MovableContiguousBumpAllocator.h>
#include <limits>
#include "DataLayout.h"

namespace rnd
{
class CBDataSource;

enum class ECBFrequency : u8
{
	PS_PerInstance,
	PS_PerFrame,
	VS_PerInstance,
	VS_PerFrame,
	Count
};

class ConstantBufferData;

using GPUBool = u32;

struct CBAccessor
{
	ConstantBufferData* mCBuffer;
	DataLayoutEntry const* mEntry;

	ReflectedValue Access();
	bool		   Exists() const { return mEntry != nullptr; }

	operator bool() const { return Exists(); }

	template<HasTypeInfo T>
	T const& operator=(T const& val);

	template<>
	bool const& operator=(bool const& val)
	{
		*this = (GPUBool) val;
		return val;
	}

	template<HasTypeInfo T>
	T const&  operator|=(T const& val);
};

DataLayout MakeCBLayout(ClassTypeInfo const& classType, size_t alignment = 16);

template<typename T>
DataLayout* GetLayout()
	requires HasTypeInfo<T> && std::is_class_v<T>
{
	static DataLayout result = MakeCBLayout(GetClassTypeInfo<T>());

	return &result;
}

class ConstantBufferData
{
public:

	ConstantBufferData() :Layout(nullptr) {}
	ConstantBufferData(DataLayout::Ref layout)
		: Layout(layout), DataSize(layout->GetSize())
	{
		Data = std::make_unique<u8[]>(DataSize);
	}

	ConstantBufferData(size_t size, DataLayout::Ref layout = nullptr)
		: Layout(layout), DataSize(size)
	{
		Data = std::make_unique<u8[]>(size);
	}

	void Resize(size_t size)
	{
		Data = std::make_unique<u8[]>(size);
		DataSize = size;
	}

	void SetLayout(DataLayout::Ref layout);

	Span<u8 const> GetView() const
	{
		return {Data.get(), DataSize};
	}

	u8* GetData(){ return Data.get(); }
	DataLayout::Ref			  Layout;

	CBAccessor operator[](HashString const& name)
	{
		ZE_ASSERT(Layout != nullptr);
		for (auto& entry: Layout->Entries)
		{
			if (entry.mName == name)
			{
				return CBAccessor {this, &entry};
			}
		}

		return CBAccessor {this, nullptr};
	}

	template<typename T>
	void SetData(T const& val)
	{
		GetAs<T>() = val;
	}

	template<typename T>
	T& GetAs()
	{
		ZE_ASSERT(sizeof(T) <= GetSize());
		return *reinterpret_cast<T*>(GetData());
	}

	template<typename T>
	inline void Set(HashString const& name, T const& val)
	{
		operator[](name) = val;
	}

	template<typename T>
	inline void SetIfExists(HashString const& name, T const& val)
	{
		operator[](name) |= val;
	}

	void FillFromSource(const CBDataSource& source);

	std::unique_ptr<u8[]> Data;
	size_t DataSize;

	size_t GetSize() const { return DataSize; }
};

inline ReflectedValue CBAccessor::Access()
{
	ZE_ASSERT(mEntry != nullptr);
	u8* data = mCBuffer->GetData() + mEntry->mOffset;
	return ReflectedValue(data, mEntry->mType);
}

template <HasTypeInfo T>
T const& CBAccessor::operator=(T const& val)
{
	ZE_ASSERT(mEntry != nullptr, "Assigning to invalid entry");
	ZE_ASSERT(mEntry->mType == &GetTypeInfo(val), "type mismatch");
	mEntry->mType->Copy(ConstReflectedValue::From(val), Access());
	return val;
}

template <HasTypeInfo T>
T const& CBAccessor::operator|=(T const& val)
{
	if (mEntry != nullptr)
	{
		*this = val;
	}
	return val;
}

class IConstantBuffer
{
public:
	template<typename... Args>
	IConstantBuffer(Args... args)
		: mData(std::forward<Args>(args)...) {}
	virtual ~IConstantBuffer() {}
	ConstantBufferData& Data() { return mData; }
	virtual void		Update() = 0;
	virtual void		SetLayout(DataLayout::Ref layout) = 0;

	ConstantBufferData& GetCBData() { return mData; }


	template<typename T>
	void WriteData(const T& val)
	{
		mData.SetData(val);
		Update();
	}

protected:
	RMOVE_DEFAULT(IConstantBuffer);
	RCOPY_DEFAULT(IConstantBuffer);
	ConstantBufferData mData;
};

class CBDataSource
{
	constexpr static size_t InvalidPos = std::numeric_limits<size_t>::max();
public:
	CBDataSource()
		: mData(64) {}

	struct Entry
	{
		TypeInfo const* Type = nullptr;
		size_t Pos = InvalidPos;
		bool Enabled = true;
	};

	ConstReflectedValue Get(const HashString& key) const
	{
		auto const it = mEntries.find(key);
		if (it == mEntries.end() || !it->second.Enabled)
		{
			return ConstNoValue;
		}

		return {mData[it->second.Pos], it->second.Type};
	}

	template<typename T>
	void Set(const HashString& key, const T& value)
		requires(std::is_trivially_copyable_v<T>)
	{
		Entry& entry = mEntries[key];
		if (entry.Type == nullptr)
		{
			entry.Type = &GetTypeInfo<T>();
			entry.Pos = mData.Allocate(sizeof(T), alignof(T));
		}
		else
		{
			ZE_ASSERT(GetTypeInfo<T>() == *entry.Type);
		}
		entry.Enabled = true;
		*reinterpret_cast<T*>(mData[entry.Pos]) = value;
	}

	void Unset(const HashString& key)
	{
		if (auto const it = mEntries.find(key); it != mEntries.end())
		{
			it->second.Enabled = false;
		}
	}

private:
	std::unordered_map<HashString, Entry> mEntries;
	MovableContiguousBumpAllocator mData;
};

} // namespace rnd
