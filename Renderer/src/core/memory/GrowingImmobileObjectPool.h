#pragma once

#include "core/Maths.h"

struct Noop
{
	template<typename... Args>
	void operator()(Args&&...)
	{
	}
};

//template <typename T, typename U>
//using Pair = std::pair;

template<typename T, size_t PageSize = 256, bool ListUsed = false, typename Initializer = Noop, typename SizeType = u32>
class GrowingImmobileObjectPool
{
public:
	GrowingImmobileObjectPool(Initializer&& init = {})
		: mInit(init)
	{
		mPages.emplace_back().reserve(PageSize);
		mInUse.resize(PageSize);
	}

	// Returns true if new
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

		if (mPages.empty() || mPages.back().size() == PageSize)
		{
			mPages.emplace_back().reserve(PageSize);
			if constexpr (ListUsed)
			{
				mInUse.resize(mInUse.size() + PageSize);
			}
		}

		Vector<T>& lastPage = mPages.back();
		T& newObj = lastPage.emplace_back();
		mInit(newObj);
		SizeType idx = NumCast<SizeType>((mPages.size() - 1) * PageSize + (lastPage.size() - 1));
		id = idx;
		if constexpr (ListUsed)
		{
			mInUse[idx] = true;
		}
		object = &newObj;
		return true;
	}

	T& operator[](SizeType index)
	{
		SizeType pageIdx = index / PageSize;
		SizeType objIdx = index % PageSize;
		ZE_ENSURE(pageIdx < mPages.size() && objIdx < mPages[pageIdx].size());
		return mPages[pageIdx][objIdx];
	}

	T const& operator[](SizeType index) const
	{
		return const_cast<GrowingImmobileObjectPool*>(this)->operator[](index);
	}

	// slow if not using ListUsed
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
		size_t size = (mPages.size() - 1) * PageSize + mPages.back().size();
		return NumCast<SizeType>(size);
	}

	Vector<Vector<T>> mPages;
	Vector<SizeType> mFreeObjects;
	Vector<bool> mInUse;
	Initializer mInit;
};

