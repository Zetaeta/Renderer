#pragma once



template<typename T, size_t ChunkCount = 256>
class ChunkedLinearAllocator
{
public:
	static_assert(IsPowerOf2(ChunkCount));
//	constexpr size_t LogChunkCount = RoundUpLog2(ChunkCount);
	// Returns uninitialized T. The caller is responsible for constructing it
	T* Allocate()
	{
		size_t ChunkIdx = mSize / ChunkCount;
		size_t IdxInChunk = mSize % ChunkCount;
		if (IdxInChunk == 0)
		{
			T* newChunk = static_cast<T*>(AlignedAllocate(sizeof(T) * ChunkCount, alignof(T)));
			mChunks.push_back(newChunk);
		}
		mSize++;
		return mChunks[ChunkIdx] + IdxInChunk;
	}

	T& operator[](size_t i)
	{
		ZE_ASSERT(i < mSize);
		size_t ChunkIdx = mSize / ChunkCount;
		size_t IdxInChunk = mSize % ChunkCount;
		return mChunks[ChunkIdx][IdxInChunk];
	}

	void ReleaseAll()
	{
		
		for (u64 i = 0; i < mSize; ++i)
		{
			std::destroy_at(&(*this)[i]);
		}

		for (T*& chunk : mChunks)
		{
			AlignedFree(chunk, sizeof(T) * ChunkCount, alignof(T));
		}
	}
private:
	size_t mSize = 0;
	Vector<T*> mChunks;
};

template<typename T>
class BumpVector
{
	template<typename... Args>
	T& Emplace(Args&&... args)
	{
		T* storage p= mAllocator.Allocate();
	}
	ChunkedLinearAllocator<T> mAllocator;
};
