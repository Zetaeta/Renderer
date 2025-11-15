#include "ContainerTypeInfo.h"



void ContainerTypeInfo::Serialize(class Serializer& serializer, void* val) const
{
	auto acc = CreateAccessor(ReflectedValue{val, this});
	int l = NumCast<int>(acc->GetSize());
	serializer.EnterArray(l);
	if (serializer.IsLoading())
	{
		acc->Resize(l);
	}
	for (int i=0;i<l;++i)
	{
		GetContainedType().Serialize(serializer, acc->GetAt(i));
	}
	serializer.LeaveArray();
}
