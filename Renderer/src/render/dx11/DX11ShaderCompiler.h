#pragma once
#include <render/DeviceShader.h>
#include <filesystem>
#include "d3d11.h"

#include "core/WinUtils.h"
#include "container/SmallMap.h"

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

	D3D11Shader* GetShader() const
	{
		return Shader.Get();
	}
};

using DX11PixelShader = DX11Shader<ID3D11PixelShader>;
using DX11VertexShader = DX11Shader<ID3D11VertexShader>;
using DX11GeometryShader = DX11Shader<ID3D11GeometryShader>;
using DX11ComputeShader = DX11Shader<ID3D11ComputeShader>;

class DX11ShaderCompiler final : public IShaderCompiler
{
public:
	DX11ShaderCompiler(ID3D11Device* device)
		: mDevice(device) {}
	OwningPtr<IDeviceShader> CompileShader(ShaderInstanceId const& id, const ShaderDesc& desc, const ShaderCompileEnv& env,
		EShaderType ShaderType, VertexAttributeMask vsInputMask, bool forceRecompile) override final;

	char const* GetShaderTypeString(EShaderType shaderType);
	fs::path const mSrcDir{ "src\\shaders" };
	fs::path const mOutDir{ "generated\\shaders" };

	//DX11Device* mMyDevice;
	SmallMap<VertexAttributeMask, ComPtr<ID3DBlob>> mShadersForCIL;
	ID3D11Device* mDevice = nullptr;
};

} // namespace dx11
} // namespace rnd
