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
	u32 AddRef() const
	{
		return ++mRefCount;
	}
	u32 Release() const
	{
		return --mRefCount;
	}

private:
	mutable std::atomic<u32> mRefCount;
};

template<typename T>
concept RefCountedType = std::derived_from<T, RefCountedObject>;

template<typename T>
class RefPtr
{
public:
	RefPtr()
		: Ptr(nullptr) {}

	RefPtr(T* ptr)
		: Ptr(ptr)
	{
		Ptr->AddRef();
	}

	template<typename U>
	RefPtr(const RefPtr<U>& other)
		requires(std::derived_from<T,U>)
		: Ptr(other.Ptr)
	{
		if (Ptr != nullptr)
		{
			Ptr->AddRef();
		}
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

	template<typename U>
	RefPtr& operator=(const RefPtr<U>& other)
		requires(std::derived_from<T,U>)
		: Ptr(other.Ptr)
	{
		if (Ptr != nullptr)
		{
			Ptr->AddRef();
		}
		return *this;
	}

	template<typename U>
	RefPtr& operator=(RefPtr<U>&& other)
		requires(std::derived_from<T,U>)
		: Ptr(other.Ptr)
	{
		other.Ptr = nullptr;
		return *this;
	}

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

private:
	T* Ptr;
};
