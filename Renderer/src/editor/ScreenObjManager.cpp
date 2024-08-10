#include "ScreenObjManager.h"



ScreenObjectId ScreenObjManager::Register(ScreenObject* screenObj)
{
	auto res = mPool.Emplace(screenObj);

	ScreenObjectId id = res.Handle;
	screenObj->Id = id;
	return id;
}

void ScreenObjManager::Unregister(ScreenObject* screenObj)
{
	mPool.Release(screenObj->Id);
}

ScreenObject* ScreenObjManager::GetScreenObj(ScreenObjectId id) const
{
	if (ScreenObject* const* ppSO = mPool.Get(id))
	{
		return *ppSO;
	}

	return nullptr;
}
