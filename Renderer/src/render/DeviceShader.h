#pragma once
#include <core/Types.h>
#include "Shader.h"
#include "VertexAttributes.h"
#include "core/Logging.h"

namespace rnd
{

class ShaderCompileEnv;
class IShaderReflector;

class IDeviceShader
{
public:
	virtual ~IDeviceShader(){}
};

}
