#include "LightObject.h"

#define LIGHTS \
	X(DirLight)\
	X(PointLight)\
	X(SpotLight)



#define X(l)\
	DEFINE_CLASS_TYPEINFO_TEMPLATE(template<>, LightComponent<l>)\
	BEGIN_REFL_PROPS()\
	REFL_PROP(m_LightData, META(AutoExpand))\
	END_REFL_PROPS()\
	END_CLASS_TYPEINFO()

LIGHTS

#undef X

#define X(l)\
	DEFINE_CLASS_TYPEINFO_TEMPLATE(template<>, LightObject<l>)\
	BEGIN_REFL_PROPS()\
	END_REFL_PROPS()\
	END_CLASS_TYPEINFO()

LIGHTS

#undef X
