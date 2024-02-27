#pragma once
#include "ContainerTypeInfo.h"
#include "PointerTypeInfo.h"
#include "JsonSerializer.h"
#include "JsonDeserializer.h"

class Test
{
	DECLARE_STI_NOBASE(Test);
	int			m_Test;
	Vector<int> m_VecTest;
	vec3 m_V3;
	OwningPtr<int> m_PtrTest;

	String m_Str = "hi";
};
DECLARE_CLASS_TYPEINFO(Test);


DEFINE_CLASS_TYPEINFO(Test)
BEGIN_REFL_PROPS()
REFL_PROP(m_Test)
REFL_PROP(m_VecTest)
REFL_PROP(m_PtrTest)
REFL_PROP(m_V3)
REFL_PROP(m_Str)
END_REFL_PROPS()
END_CLASS_TYPEINFO()

void Testing()
{
	JsonSerializer serial;
	Test test{};
	test.m_PtrTest = std::make_unique<int>();
	auto  v = ValuePtr::From(test);
	json		   j = serial.Serialize(v);
	auto s = j.dump();
	printf("%s\n", s.c_str());
	json j2 = json::parse(s);
	Test out;
	auto  v2 = ValuePtr::From(out);
	JsonDeserializer deser;
	deser.Deserialize(v2, j2);
//	printf(test.m_Str.c_str());
	printf("%d\n", *out.m_PtrTest);
}
