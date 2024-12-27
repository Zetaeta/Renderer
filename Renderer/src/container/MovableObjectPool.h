#pragma once

#include "core/Types.h"

template<typename T, bool ListUsed = false, typename SizeType = u32>
class MovableObjectPool
{
public:
	MovableObjectPool(size_t initialSize = 0)
	{
		mObjects.reserve(initialSize);
		if constexpr (ListUsed)
		{
			mInUse.resize(initialSize);
		}
	}

	// Returns true if object is new and needs initialization
	bool Claim(SizeType& id, T*& object)
	{
		if (!mFreeObjects.empty())
		{
			SizeType freeIdx = mFreeObjects.back();
			mFreeObjects.pop_back();
			id = freeIdx;
			object = &this->operator[](freeIdx);
			if constexpr (ListUsed)
			{
				mInUse[freeIdx] = true;
			}
			return false;
		}
		object = &mObjects.emplace_back();
		id = NumCast<SizeType>(mObjects.size() - 1);

		if constexpr (ListUsed)
		{
			mObjects.emplace_back(true);
		}
		return true;
	}

	T& operator[](SizeType index)
	{
		//if constexpr (ListUsed)
		//{
		//	ZE_ASSERT_DEBUG(mInUse[index]);
		//}
		return mObjects[index];
	}

	T const& operator[](SizeType index) const
	{
		return const_cast<MovableObjectPool*>(this)->operator[](index);
	}

	bool IsInUse(SizeType index)
	{
		if constexpr (ListUsed)
		{
			return mInUse[index];
		}
		else
		{
			return mFreeObjects.find(index) == mFreeObjects.end();
		}
	}

	T& Release(SizeType index)
	{
		mFreeObjects.push_back(index);
		if constexpr (ListUsed)
		{
			mInUse[index] = false;
		}
		return operator[](index);
	}
	
	SizeType Size() const
	{
		return mObjects.size();
	}

	std::vector<T> mObjects;
	Vector<SizeType> mFreeObjects;
	Vector<bool> mInUse;
};

