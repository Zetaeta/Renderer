#pragma once
#include "BaseObject.h"
#include <functional>
#include "ClassTypeInfo.h"

class BaseSerialized : public BaseObject
{
	DECLARE_RTTI(BaseSerialized, BaseObject);
#if ZE_BUILD_EDITOR
public:
	virtual void Modify(bool allChildren = false, bool modified = true);

	void ClearModified(bool bAllChildren = true);
	bool IsModified()
	{
		return mModified;
	}

protected:
	bool mModified = false;
#endif ZE_BUILD_EDITOR

	BaseSerialized* mOuter = nullptr;
public:
	virtual void ForAllChildren(std::function<void(BaseSerialized*)> callback, bool recursive = false);

};

DECLARE_CLASS_TYPEINFO(BaseSerialized);
