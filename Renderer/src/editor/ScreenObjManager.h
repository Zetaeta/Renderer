#pragma once
#include "scene/ScreenObject.h"
#include <unordered_map>
#include <core/Utils.h>

template<typename T, typename HandleType>
struct PooledObject
{
	HandleType Handle = 0;
	T* Object = nullptr;
};

template<typename T, typename HandleType = u64>
class ObjectPool
{
public:
	template<typename... Args>
	PooledObject<T, HandleType> Emplace(Args&&... args)
	{
		HandleType handle = GetNewHandle();
		auto it = mObjects.insert({handle, T{std::forward<Args>(args)...}});
		RASSERT(it.second);
		return {handle, &it.first->second};
	}

	void Release(HandleType handle)
	{
		mObjects.erase(handle);
	}

	void Release(const PooledObject<T, HandleType>& po)
	{
		mObjects.erase(po.Handle);
	}

	HandleType GetNewHandle()
	{
		HandleType nextEmpty = mFirstEmpty + 1;
		while (mObjects.find(nextEmpty) != mObjects.end() || nextEmpty == 0)
		{
			nextEmpty++;
		}

		std::swap(nextEmpty, mFirstEmpty);
		return nextEmpty;
	}

	T* Get(HandleType handle)
	{
		if (auto it = mObjects.find(handle); it != mObjects.end())
		{
			return &it->second;
		}
		return nullptr;
	}

	T const* Get(HandleType handle) const
	{
		if (auto it = mObjects.find(handle); it != mObjects.end())
		{
			return &it->second;
		}
		return nullptr;
	}

	std::unordered_map<HandleType, T> mObjects;
	HandleType mFirstEmpty = 1;
	HandleType mMax = 0;
};

class ScreenObjManager
{
public:
	template<typename T, typename... Args>
	ScreenObjectId Register(Args&&... args)
	{
		return Register(new T(std::forward<Args>(args)...));
	}

	ScreenObjectId Register(ScreenObject* screenObj);
	void Unregister(ScreenObject* screenObj);

	ScreenObject* GetScreenObj(ScreenObjectId id) const;
private:
	ObjectPool<ScreenObject*, ScreenObjectId> mPool;
};
