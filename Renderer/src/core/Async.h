#pragma once
#include <memory>
#include "thread/RWLock.h"
#include "Utils.h"

class ICancellationToken
{
public:
	virtual ~ICancellationToken() {}
	virtual bool IsCancelled() const = 0; // Is this ever correct?
	virtual ReadLock GetCancelGuard() = 0;
};

using CancellationHandle = std::shared_ptr<ICancellationToken>;

class CancellationToken : public ICancellationToken
{
public:
	bool IsCancelled() const override
	{
		return mIsDestructed.load(std::memory_order_acquire);
	}

	ReadLock GetCancelGuard() override
	{
		// Test before lock for speed, and after lock for certainty
		if (mIsDestructed.load(std::memory_order_acquire))
		{
			return {};
		}

		auto lock = mMutex.ScopedReadLock();
		if (mIsDestructed.load(std::memory_order_acquire))
		{
			return {};
		}

		return lock;
	}

	void Cancel()
	{
		auto lock = mMutex.ScopedWriteLock();
		mIsDestructed.store(true, std::memory_order_release);
	}
	RWLock mMutex;
	std::atomic<bool> mIsDestructed = false;

	// Intended to be called by owner
	WriteLock PauseLock()
	{
		return mMutex.ScopedWriteLock();
	}
};

// RAII cancellation token
class DestructionToken
{
public:
	DestructionToken()
		: mToken(std::make_shared<CancellationToken>()) {}

	ZE_COPY_PROTECT(DestructionToken);

	DestructionToken(DestructionToken&& other)
		: mToken(std::move(other.mToken))
	{
		other.mToken = nullptr;
	}

	DestructionToken& operator=(DestructionToken&& other)
	{
		if (mToken != nullptr)
		{
			mToken->Cancel();
		}
		mToken = std::move(other.mToken);
		other.mToken = nullptr;
	}

	~DestructionToken()
	{
		if (mToken != nullptr)
		{
			mToken->Cancel();
		}
	}

	CancellationToken* operator->() const
	{
		return mToken.get();
	}

	operator std::shared_ptr<CancellationToken>() const
	{
		return mToken;
	}
	operator CancellationHandle() const
	{
		return mToken;
	}
private:
	std::shared_ptr<CancellationToken> mToken;
};
