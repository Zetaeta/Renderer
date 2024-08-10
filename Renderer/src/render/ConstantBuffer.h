#pragma once

#include <core/Types.h>
#include <core/TypeInfoUtils.h>
#include <core/Maths.h>
#include <core/MovableContiguousBumpAllocator.h>
#include <limits>

namespace rnd
{
class CBDataSource;
struct CBLEntry
{
	TypeInfo const* mType;
	Name mName;
	size_t mOffset = 0;
};

template<typename T>
CBLEntry Entry(Name name)
{
	return CBLEntry { &GetTypeInfo<T>(), name };
}

enum class ECBFrequency : u8
{
	PS_PerInstance,
	PS_PerFrame,
	VS_PerInstance,
	VS_PerFrame,
	Count
};

class ConstantBufferData;

using GPUBoolType = u32;

struct CBAccessor
{
	ConstantBufferData* mCBuffer;
	CBLEntry const* mEntry;

	ReflectedValue Access();
	bool		   Exists() const { return mEntry != nullptr; }

	operator bool() const { return Exists(); }

	template<HasTypeInfo T>
	T const& operator=(T const& val);

	template<>
	bool const& operator=(bool const& val)
	{
		*this = (GPUBoolType) val;
		return val;
	}

	template<HasTypeInfo T>
	T const&  operator|=(T const& val);
};

struct CBLayout
{
	CBLayout()
		: mAlignment(0), mSize(0) {}
	CBLayout(size_t alignment, Vector<CBLEntry>&& entries);

	size_t mAlignment;
	Vector<CBLEntry> Entries;
	size_t mSize;

	size_t GetSize()
	{
		return mSize;
	}

	using Ref = CBLayout*;
};

CBLayout MakeCBLayout(ClassTypeInfo const& classType, size_t alignment = 16);

template<typename T>
CBLayout* GetLayout()
	requires HasTypeInfo<T> && std::is_class_v<T>
{
	static CBLayout result = MakeCBLayout(GetClassTypeInfo<T>());

	return &result;
}

class ConstantBufferData
{
public:

	ConstantBufferData() :Layout(nullptr) {}
	ConstantBufferData(CBLayout::Ref layout)
		: Layout(layout), DataSize(layout->GetSize())
	{
		Data = std::make_unique<u8[]>(DataSize);
	}

	ConstantBufferData(size_t size, CBLayout::Ref layout = nullptr)
		: Layout(layout), DataSize(size)
	{
		Data = std::make_unique<u8[]>(size);
	}

	void Resize(size_t size)
	{
		Data = std::make_unique<u8[]>(size);
		DataSize = size;
	}

	void SetLayout(CBLayout::Ref layout);

	u8* GetData(){ return Data.get(); }
	CBLayout::Ref			  Layout;

	CBAccessor operator[](Name const& name)
	{
		RASSERT(Layout != nullptr);
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
		RASSERT(sizeof(T) <= GetSize());
		return *reinterpret_cast<T*>(GetData());
	}

	template<typename T>
	inline void Set(Name const& name, T const& val)
	{
		operator[](name) = val;
	}

	template<typename T>
	inline void SetIfExists(Name const& name, T const& val)
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
	RASSERT(mEntry != nullptr);
	u8* data = mCBuffer->GetData() + mEntry->mOffset;
	return ReflectedValue(data, mEntry->mType);
}

template <HasTypeInfo T>
T const& CBAccessor::operator=(T const& val)
{
	RASSERT(mEntry != nullptr, "Assigning to invalid entry");
	RASSERT(mEntry->mType == &GetTypeInfo(val), "type mismatch");
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
	ConstantBufferData& Data() { return mData; }
	virtual void		Update() = 0;
	virtual void		SetLayout(CBLayout::Ref layout) = 0;
protected:
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

	ConstReflectedValue Get(const Name& key) const
	{
		auto const it = mEntries.find(key);
		if (it == mEntries.end() || !it->second.Enabled)
		{
			return ConstNoValue;
		}

		return {mData[it->second.Pos], it->second.Type};
	}

	template<typename T>
	void Set(const Name& key, const T& value)
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
			RASSERT(GetTypeInfo<T>() == *entry.Type);
		}
		entry.Enabled = true;
		*reinterpret_cast<T*>(mData[entry.Pos]) = value;
	}

	void Unset(const Name& key)
	{
		if (auto const it = mEntries.find(key); it != mEntries.end())
		{
			it->second.Enabled = false;
		}
	}

private:
	std::unordered_map<Name, Entry> mEntries;
	MovableContiguousBumpAllocator mData;
};

} // namespace rnd
