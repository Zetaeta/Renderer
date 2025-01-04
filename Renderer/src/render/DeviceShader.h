#pragma once
#include <core/Types.h>
#include "Shader.h"
#include "VertexAttributes.h"
#include "core/Logging.h"

DECLARE_LOG_CATEGORY(LogShaderCompiler);

namespace rnd
{

class ShaderCompileEnv;
class IShaderReflector;

class IDeviceShader
{
public:
	virtual ~IDeviceShader(){}
};

class IShaderCompiler
{
public:
	virtual OwningPtr<IDeviceShader> CompileShader(ShaderInstanceId const& id, const ShaderDesc& desc,
		const ShaderCompileEnv& env, EShaderType ShaderType, VertexAttributeMask inputMask = {}, OwningPtr<IShaderReflector>* outReflector = nullptr, bool forceRecompile = false) = 0;
};


}
