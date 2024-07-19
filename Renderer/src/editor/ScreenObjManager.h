#pragma once
#include "scene/ScreenObject.h"
#include <unordered_map>

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
		auto it = mObjects.insert(handle, std::forward<Args>(args)...);
		return {handle, &it->second};
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
		while (mObjects.find(nextEmpty) != mObjects.end())
		{
			nextEmpty++;
		}

		swap(nextEmpty, mFirstEmpty);
		return nextEmpty;
	}

	std::unordered_map<HandleType, T> mObjects;
	HandleType mFirstEmpty = 0;
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
private:
	ObjectPool<ScreenObject*, ScreenObjectId> mPool;
};
