#pragma once

#include <atomic>
#include "Maths.h"

class RefCountedObject
{
public:
	RefCountedObject()
		: mRefCount(0) {}
	template<typename TOther>
	RefCountedObject(TOther&& other) noexcept
		: mRefCount(0)
	{
	}

	RefCountedObject(RefCountedObject const& other)
	:mRefCount(0)
	{
	}

	RefCountedObject(RefCountedObject&& other)
	:mRefCount(0)
	{
	}

	RefCountedObject& operator=(RefCountedObject&& other)
	{
		return *this;
	}

	RefCountedObject& operator=(RefCountedObject const& other)
	{
		return *this;
	}

	u32 AddRef() const
	{
		return ++mRefCount;
	}
	u32 Release() const
	{
		return --mRefCount;
	}

	u32 GetRefCount() const
	{
		return mRefCount.load(std::memory_order_acquire);
	}

private:
	mutable std::atomic<u32> mRefCount;
};

class SelfDestructingRefCounted
{
public:
	SelfDestructingRefCounted()
		: mRefCount(0) {}
	template<typename TOther>
	SelfDestructingRefCounted(TOther&& other) noexcept
		: mRefCount(0)
	{
	}
	u32 AddRef() const
	{
		return ++mRefCount;
	}
	u32 Release()
	{
		u32 newRefCount = --mRefCount;
		if (newRefCount == 0)
		{
			OnFullyReleased();
		}
		return newRefCount;
	}

	u32 GetRefCount() const
	{
		return mRefCount.load(std::memory_order_acquire);
	}

	virtual void OnFullyReleased() {}

private:
	mutable std::atomic<u32> mRefCount;
};

template<typename T>
concept RefCountedType = std::derived_from<T, RefCountedObject>;

#ifndef FORCEINLINE
#define FORCEINLINE __forceinline
#endif

template<typename T>
class RefPtr
{
public:
	RefPtr()
		: Ptr(nullptr) {}

	RefPtr(T* ptr)
		: Ptr(ptr)
	{
		if (Ptr != nullptr)
		{
			Ptr->AddRef();
		}
	}

	FORCEINLINE void InternalAddRef()
	{
		if (Ptr != nullptr)
		{
			Ptr->AddRef();
		}
	}

	FORCEINLINE void InternalDecRef()
	{
		if (Ptr != nullptr)
		{
			Ptr->Release();
		}
	}

	RefPtr(const RefPtr& other)
	:Ptr(other.Ptr)
	{
		InternalAddRef();
	}

	RefPtr(RefPtr&& other)
	:Ptr(other.Ptr)
	{
		other.Ptr = nullptr;
	}

	template<typename U>
	RefPtr(const RefPtr<U>& other)
		requires(std::derived_from<T,U>)
		: Ptr(other.Ptr)
	{
		InternalAddRef();
	}
	template<typename U>
	RefPtr(RefPtr<U>&& other)
		requires(std::derived_from<T,U>)
		: Ptr(other.Ptr)
	{
		other.Ptr = nullptr;
	}

	template<typename Base>
	operator RefPtr<Base>()
//		requires(std::derived_from<Base, T>)
	{
		return RefPtr<Base>(this->Ptr);
	}

	template<typename Derived>
	RefPtr& operator=(Derived* derivedPtr)
//		requires(std::derived_from<T, Derived>)
	{
		InternalDecRef();
		Ptr = derivedPtr;
		InternalAddRef();
		return *this;
	}

	template<typename U>
	RefPtr& operator=(RefPtr<U> other)
//		requires(std::derived_from<T,U>)
	{
		std::swap(Ptr, other.Ptr);
		return *this;
	}

	RefPtr& operator=(const RefPtr& other)
	{
		InternalDecRef();
		Ptr = other.Ptr;
		InternalAddRef();
		return *this;
	}
	RefPtr& operator=(RefPtr&& other)
	{
		std::swap(Ptr, other.Ptr);
		return *this;
	}

	//template<typename U>
	//RefPtr& operator=(RefPtr<U>&& other)
	//	requires(std::derived_from<T,U>)
	//	: Ptr(other.Ptr)
	//{
	//	other.Ptr = nullptr;
	//	return *this;
	//}

	~RefPtr()
	{
		if (Ptr)
		{
			Ptr->Release();
		}
	}

	T* Get() const { return Ptr; }
	T& operator*() const
	{
		return *Ptr;
	}

	T* operator->() const &
	{
		return Ptr;
	}

	operator T*() const &
	{
		return Ptr;
	}

	bool IsValid() const
	{
		return Ptr != nullptr;
	}

private:
	T* Ptr;
};

// Deletes pointed-to object and clears the pointer
template<typename T>
inline void Delete(RefPtr<T>& ptr)
{
	T* raw = ptr.Get();
	ptr = nullptr;
	delete raw;
}
