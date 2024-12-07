#pragma once
#include <core/Types.h>
#include "Shader.h"
#include "VertexAttributes.h"

namespace rnd
{

class ShaderCompileEnv;

class IDeviceShader
{
public:
	virtual ~IDeviceShader(){}
};

class IShaderCompiler
{
public:
	virtual OwningPtr<IDeviceShader> CompileShader(ShaderInstanceId const& id, const ShaderDesc& desc,
		const ShaderCompileEnv& env, EShaderType ShaderType, VertexAttributeMask inputMask = {}, bool forceRecompile = false) = 0;
};

}
