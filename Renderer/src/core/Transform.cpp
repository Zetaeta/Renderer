
#include "core/Transform.h"

DEFINE_CLASS_TYPEINFO_TEMPLATE(template<>, TTransform<quat>)
BEGIN_REFL_PROPS()
REFL_PROP(translation)
REFL_PROP(scale)
REFL_PROP(rotation)
END_REFL_PROPS()
END_CLASS_TYPEINFO()

DEFINE_CLASS_TYPEINFO_TEMPLATE(template<>, TTransform<Rotator>)
BEGIN_REFL_PROPS()
REFL_PROP(translation)
REFL_PROP(scale)
REFL_PROP(rotation)
END_REFL_PROPS()
END_CLASS_TYPEINFO()
