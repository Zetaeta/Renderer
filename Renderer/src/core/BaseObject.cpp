#include "core/BaseObject.h"



ClassTypeInfo const& BaseObject::GetTypeInfo() const
{
	return GetClassTypeInfo<BaseObject>();
}

ClassTypeInfo const& BaseObject::StaticClass()
{
	return GetClassTypeInfo<BaseObject>();
}

ClassTypeInfoImpl<BaseObject> const BaseObject::TypeInfoHelper::s_TypeInfo = BaseObject::TypeInfoHelper::MakeTypeInfo();
ClassTypeInfoImpl<BaseObject> BaseObject::TypeInfoHelper::MakeTypeInfo() {
	HashString							  name = "BaseObject";
	ClassTypeInfo::Properties attrs;
	ClassTypeInfo const* parent = nullptr;
	return ClassTypeInfoImpl<BaseObject>(name, parent, {}, std::move(attrs));
}

