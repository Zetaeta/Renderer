#pragma once
#include <shared_mutex>

class RWLock;
using WriteLock = std::unique_lock<RWLock>;
using ReadLock = std::shared_lock<RWLock>;

class RWLock : public std::shared_mutex
{
public:
	[[nodiscard]] WriteLock ScopedWriteLock()
	{
		return WriteLock(*this);
	}

	[[nodiscard]] ReadLock ScopedReadLock()
	{
		return ReadLock(*this);
	}
};

