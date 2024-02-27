#include "BaseObject.h"
#include "TypeInfo.h"



ClassTypeInfo const& BaseObject::GetTypeInfo() const
{
	return GetClassTypeInfo<BaseObject>();
}

ClassTypeInfo const& BaseObject::GetStaticTypeInfo()
{
	return GetClassTypeInfo<BaseObject>();
}

	ClassTypeInfoImpl<BaseObject> const TypeInfoHelper<BaseObject>::s_TypeInfo = MakeTypeInfo();
	ClassTypeInfoImpl<BaseObject> TypeInfoHelper<BaseObject>::MakeTypeInfo() {
		Name							  name = "BaseObject";
		Vector<OwningPtr<PropertyInfo>> attrs;
		ClassTypeInfo const* parent = nullptr;
		return ClassTypeInfoImpl<BaseObject>(name, parent, std::move(attrs));
	}

