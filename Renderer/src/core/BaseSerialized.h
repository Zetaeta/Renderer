#pragma once
#include "BaseObject.h"
#include <functional>
#include "ClassTypeInfo.h"

class BaseSerialized : public BaseObject
{
	DECLARE_RTTI(BaseSerialized, BaseObject);
	void Modify(bool allChildren = false, bool modified = true);
	bool IsModified()
	{
		return mModified;
	}

	virtual void ForAllChildren(std::function<void(BaseSerialized*)> callback, bool recursive = false);

	bool mModified = false;
};

DECLARE_CLASS_TYPEINFO(BaseSerialized);
