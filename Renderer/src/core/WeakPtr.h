#pragma once

#include <memory>

template<typename T>
class WeakPtr : public std::weak_ptr<T>
{
	using Super = std::weak_ptr<T>;
public:
	WeakPtr() = default;

	template<typename TChild>
	WeakPtr(const std::weak_ptr<TChild>& weak)
		: Super(weak) {}

	template<typename TChild>
	WeakPtr(const std::shared_ptr<TChild>& weak)
		: Super(weak) {}

	//template<typename Shared>
	//WeakPtr(const std::enable_shared_from_this<Shared>* es)
	//	requires(std::is_const_v<T>)
	//	: Super(es ? es->shared_from_this() : nullptr) {}

	template<typename EnableShared>
	WeakPtr(EnableShared* es)
		: Super(es ? es->shared_from_this() : nullptr) {}

	T* Get() const
	{
		if (auto shared = this->lock())
		{
			return shared.get();
		}
		return nullptr;
	}

	T* operator->() const
	{
		return Get();
	}

	T& operator*() const
	{
		return *Get();
	}

	bool operator==(T* other) const
	{
		return Get() == other;
	}

	bool operator!=(T* other) const
	{
		return Get() == other;
	}

};
