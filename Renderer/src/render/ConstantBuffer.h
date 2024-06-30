#pragma once

#include <core/Types.h>
#include <core/TypeInfoUtils.h>
#include <core/Maths.h>


namespace rnd
{

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

class ConstantBufferData;

struct CBAccessor
{
	ConstantBufferData* mCBuffer;
	CBLEntry* mEntry;

	ReflectedValue Access();
	bool		   Exists() const { return mEntry != nullptr; }

	operator bool() const { return Exists(); }

	template<HasTypeInfo T>
	T const& operator=(T const& val);

	template<HasTypeInfo T>
	T const& operator|=(T const& val);
};

struct CBLayout
{
	CBLayout()
		: mAlignment(0), mSize(0) {}
	CBLayout(size_t alignment, Vector<CBLEntry>&& entries);

	size_t mAlignment;
	Vector<CBLEntry> mEntries;
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

	void SetLayout(CBLayout::Ref layout)
	{
		if (layout != nullptr)
		{
			RASSERT(layout->GetSize() < DataSize, "Layout too big");
		}
		Layout = layout;
	}

	u8* GetData(){ return Data.get(); }
	CBLayout::Ref			  Layout;

	CBAccessor operator[](Name const& name)
	{
		RASSERT(Layout != nullptr);
		for (auto& entry: Layout->mEntries)
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

} // namespace rnd
