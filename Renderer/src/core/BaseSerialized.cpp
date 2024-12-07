#include "BaseSerialized.h"



void BaseSerialized::Modify(bool allChildren /*= false*/, bool modified /*= true*/)
{
	mModified = modified;
	if (mOuter)
	{
		mOuter->Modify(false, modified);
	}

	if (allChildren)
	{
		ForAllChildren([modified](BaseSerialized* child)
		{
			child->Modify(true, modified);
		});
	}
}

void BaseSerialized::ClearModified(bool bAllChildren /*= true*/)
{
	mModified = false;
	ForAllChildren([](BaseSerialized* child)
	{
		child->mModified = false;
	}, true);
}

void BaseSerialized::ForAllChildren(std::function<void(BaseSerialized*)> callback, bool recursive /*= false*/)
{
	ClassTypeInfo const& thisClass = GetTypeInfo();
	auto handleChild = [&callback, recursive](BaseSerialized* child)
	{
		callback(child);
		if (recursive)
		{
			child->ForAllChildren(callback, true);
		}
	};

	thisClass.ForEachProperty([&callback, recursive, &handleChild, &thisClass, this](const PropertyInfo& prop)
	{
		if (prop.GetType().Contains(BaseSerialized::StaticClass()))
		{
			handleChild(&prop.Access(ClassValuePtr(this, thisClass)).GetAs<BaseSerialized>());
		}
	});
}

DEFINE_CLASS_TYPEINFO(BaseSerialized)
BEGIN_REFL_PROPS()
END_REFL_PROPS()
END_CLASS_TYPEINFO()

