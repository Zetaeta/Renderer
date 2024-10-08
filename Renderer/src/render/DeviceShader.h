#pragma once
#include <core/Types.h>
#include "Shader.h"

namespace rnd
{

class ShaderCompileEnv;

class IDeviceShader
{ };

class IShaderCompiler
{
public:
	virtual OwningPtr<IDeviceShader> CompileShader(ShaderInstanceId const& id, const String& file, const ShaderCompileEnv& env, EShaderType ShaderType, bool forceRecompile = false) = 0;
};

}
