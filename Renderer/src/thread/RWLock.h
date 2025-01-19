#pragma once
#include <shared_mutex>

class RWLock;
using WriteLock = std::unique_lock<RWLock>;
using ReadLock = std::shared_lock<RWLock>;

class RWLock : public std::shared_mutex
{
public:
	WriteLock ScopedWriteLock()
	{
		return WriteLock(*this);
	}

	ReadLock ScopedReadLock()
	{
		return ReadLock(*this);
	}
};

