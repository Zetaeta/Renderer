#pragma once
#include <render/DeviceShader.h>
#include <filesystem>
#include "d3d11.h"

#include "core/WinUtils.h"

namespace fs = std::filesystem;

namespace rnd
{
namespace dx11
{



template<typename D3D11Shader>
class DX11Shader : public IDeviceShader
{
public:
	ComPtr<D3D11Shader> Shader;
};

using DX11PixelShader = DX11Shader<ID3D11PixelShader>;
using DX11VertexShader = DX11Shader<ID3D11VertexShader>;
using DX11GeometryShader = DX11Shader<ID3D11GeometryShader>;
using DX11ComputeShader = DX11Shader<ID3D11ComputeShader>;

class DX11ShaderCompiler : public IShaderCompiler
{
public:
	DX11ShaderCompiler(ID3D11Device* device)
		: mDevice(device) {}
	OwningPtr<IDeviceShader> CompileShader(ShaderInstanceId const& id, const String& file, const ShaderCompileEnv& env, EShaderType ShaderType, bool forceRecompile) override final;

	char const* GetShaderTypeString(EShaderType shaderType);
	fs::path const mSrcDir{ "src\\shaders" };
	fs::path const mOutDir{ "generated\\shaders" };

	ID3D11Device* mDevice;
};

} // namespace dx11
} // namespace rnd
