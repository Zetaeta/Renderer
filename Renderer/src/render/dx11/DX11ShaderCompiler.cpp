#include "DX11ShaderCompiler.h"
#include "D3Dcompiler.h"
#include "core/WinUtils.h"
#include "core/Logging.h"
#include "SharedD3D11.h"
#include "render/dxcommon/DXShaderCompiler.h"
namespace rnd
{
namespace dx11
{

DEFINE_LOG_CATEGORY_STATIC(LogDX11Shader);

constexpr bool ShaderCompileDebugging = true;

DX11ShaderCompiler::DX11ShaderCompiler(ID3D11Device* device) : mDevice(device)
{
	mSrcDir = "src\\shaders";
	mOutDir = "generated\\shaders";
	for (EShaderType type = EShaderType::GraphicsStart; type < EShaderType::Count; ++type)
	{
		mTypeStrings[type] = GetShaderTypeString(type);
	}
}

OwningPtr<IDeviceShader> DX11ShaderCompiler::CompileShader(ShaderInstanceId const& id, const ShaderDesc& desc,
	const ShaderCompileEnv& env, EShaderType shaderType, VertexAttributeMask inputMask, OwningPtr<IShaderReflector>* outReflector, bool forceRecompile)
{
	ComPtr<ID3DBlob> outBlob = GetShaderBytecode(id, desc, env, shaderType, outReflector, forceRecompile);
	switch (shaderType)
	{
		case EShaderType::Vertex:
		{
			auto result = MakeOwning<DX11VertexShader>();
			HR_ERR_CHECK(mDevice->CreateVertexShader(outBlob->GetBufferPointer(), outBlob->GetBufferSize(), nullptr, &result->Shader));
			mShadersForCIL[inputMask] = outBlob;
			SetResourceName(result->Shader, desc.Name);
			return result;
		}
		case EShaderType::Pixel:
		{
			auto result = MakeOwning<DX11PixelShader>();
			HR_ERR_CHECK(mDevice->CreatePixelShader(outBlob->GetBufferPointer(), outBlob->GetBufferSize(), nullptr, &result->Shader));
			SetResourceName(result->Shader, desc.Name);
			return result;
		}
		case EShaderType::Geometry:
		{
			auto result = MakeOwning<DX11GeometryShader>();
			HR_ERR_CHECK(mDevice->CreateGeometryShader(outBlob->GetBufferPointer(), outBlob->GetBufferSize(), nullptr, &result->Shader));
			SetResourceName(result->Shader, desc.Name);
			return result;
		}
		case EShaderType::Compute:
		{
			auto result = MakeOwning<DX11ComputeShader>();
			HR_ERR_CHECK(mDevice->CreateComputeShader(outBlob->GetBufferPointer(), outBlob->GetBufferSize(), nullptr, &result->Shader));
			SetResourceName(result->Shader, desc.Name);
			return result;
		}
		default:
			ZE_ASSERT(false);
			break;
	}
	return nullptr;
}

char const* DX11ShaderCompiler::GetShaderTypeString(EShaderType shaderType)
{
	switch (shaderType)
	{
		case rnd::EShaderType::Vertex:
			return "vs_5_0";
		case rnd::EShaderType::Pixel:
			return "ps_5_0";
		case rnd::EShaderType::Geometry:
			return "gs_5_0";
		case rnd::EShaderType::Compute:
			return "cs_5_0";
		default:
			return nullptr;
	}
}

}
}
