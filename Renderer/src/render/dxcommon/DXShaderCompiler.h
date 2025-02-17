#pragma once
#include "container/EnumArray.h"
#include "render/Shader.h"
#include "render/DeviceShader.h"
#include "core/WinUtils.h"
#include "d3dcommon.h"
#include <filesystem>
#include "render/shaders/ShaderReflection.h"

namespace rnd { class IShaderReflector; }

namespace fs = std::filesystem;

struct ID3D11ShaderReflection;

namespace rnd::dx
{

ComPtr<ID3DBlob> DXCompileFile(wchar_t const* filePath, char const* entryPoint, char const* shaderTarget, D3D_SHADER_MACRO const* macros, UINT flags );

class DXShaderCompiler
{
protected:
	EnumArray<EShaderType, char const*> mTypeStrings; 
	fs::path mOutDir;
	fs::path mSrcDir;
//	fs::path mOverrideSrcDir;
};

class FXCShaderCompiler : public DXShaderCompiler
{
public:
	ComPtr<ID3DBlob> GetShaderBytecode(ShaderInstanceId const& id, const ShaderDesc& desc, const ShaderCompileEnv& env,
		EShaderType shaderType, OwningPtr<IShaderReflector>* outReflector, bool forceRecompile);
};

}
