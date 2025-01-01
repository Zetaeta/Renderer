#pragma once

#include "core/CoreTypes.h"
#include "BaseUtils.h"

// Like GrowingImmobileObjectPool but destructs objects when released
template<typename T, size_t PageSize = 256, bool ListUsed = false, typename SizeType = u32, typename Destructor = DefaultDestruct, typename Constructor = DefaultConstruct<T>>
class GrowingImmobilePool : public PoolUsageTracker<ListUsed, SizeType>
{
	static_assert(sizeof(T) >= sizeof(void*), "Free-list requires at least pointer size");
	using PoolUsageTracker<ListUsed, SizeType>::TrackUsed;
	using PoolUsageTracker<ListUsed, SizeType>::CheckUsed;
	using PoolUsageTracker<ListUsed, SizeType>::GrowUsedTracker;

	using EntryType = UninitializedMemory<T>;
public:
	GrowingImmobilePool(Destructor&& destructor = {}, Constructor&& construct = {})
		: mDestruct(std::move(destructor)), mConstruct(std::move(construct))
	{
	}

	constexpr static bool CacheIndexAfterPointer = sizeof(T) >= sizeof(void*) + sizeof(SizeType);

	void AddPage()
	{
		mPages.emplace_back() = std::make_unique<EntryType[]>(PageSize);
		GrowUsedTracker(PageSize);
		mLastPageSize = 0;
	}

	// Returns true if new
	T* Acquire()
	{
		if (mFirstFree)
		{
			if constexpr (CacheIndexAfterPointer)
			{
				SizeType index = *reinterpret_cast<SizeType*>(reinterpret_cast<u8*>(mFirstFree) + sizeof(T));
				TrackUsed(index, true);
			}
			else
			{
				TrackUsed(mFirstFree, true);
			}
			u8* nextFree = *reinterpret_cast<u8* const*>(mFirstFree);
			T& result = mConstruct(nextFree);
			mFirstFree = nextFree;
			return &result;
		}

		if (mPages.empty() || mLastPageSize == PageSize)
		{
			AddPage();
		}

		auto& lastPage = mPages.back();
		SizeType idx = NumCast<SizeType>((mPages.size() - 1) * PageSize + mLastPageSize);
		T& newObj = mConstruct(lastPage[mLastPageSize++].Data);
		TrackUsed(idx, true);
		return &newObj;
	}

	T& operator[](SizeType index)
	{
		CheckUsed(index);
		return *std::launder<T>(reinterpret_cast<T*>(Access(index)));
	}

	T const& operator[](SizeType index) const
	{
		return const_cast<GrowingImmobilePool*>(this)->operator[](index);
	}

	void Release(T&& val)
	{
		//T& val = operator[](index);
		mDestruct(val);
		*reinterpret_cast<u8**>(&val) = mFirstFree;
		SizeType index = GetIndex(&val);
		if constexpr (sizeof(T) >= sizeof(void) + sizeof(SizeType))
		{
			// Store index after the next-free pointer so we don't have to calculate it later.
			*reinterpret_cast<SizeType*>(reinterpret_cast<u8*>(&val) + sizeof(T)) = index;
		}
		TrackUsed(index, false);
		mFirstFree = reinterpret_cast<u8*>(&val);
	}

	void TrackUsed(T* address, bool isUsed)
	{
		if constexpr (ListUsed)
		{
			TrackUsed(GetIndex(address), isUsed);
		}
	}

	bool IsInPage(T* address, void* pageStart)
	{
		T* typedPageStart =	static_cast<T*>(pageStart);
		return address - typedPageStart < PageSize;
	}

	SizeType GetIndex(T* address)
	{
		for (int i = 0; i < mPages.size(); ++i)
		{
			if (IsInPage(address, mPages[i].get()))
			{
				return i;
			}
		}
		return (SizeType) -1;
	}

	u8* Access(SizeType index)
	{
		SizeType pageIdx = index / PageSize;
		SizeType objIdx = index % PageSize;
		ZE_ASSERT(pageIdx < mPages.size() && objIdx < mPages[pageIdx].size());
		return mPages[pageIdx][objIdx].Data;
	}
	
	SizeType Size() const
	{
		size_t size = (mPages.size() - 1) * PageSize + mPages.back().size();
		return NumCast<SizeType>(size);
	}

	u8* mFirstFree = nullptr;
	Vector<std::unique_ptr<EntryType[]>> mPages;
	u32 mLastPageSize = 0;
	Constructor mConstruct;
	Destructor mDestruct;
};
