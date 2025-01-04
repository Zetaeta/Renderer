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

OwningPtr<IDeviceShader> DX11ShaderCompiler::CompileShader(ShaderInstanceId const& id, const ShaderDesc& desc,
	const ShaderCompileEnv& env, EShaderType shaderType, VertexAttributeMask inputMask, bool forceRecompile)
{
	const String& file = desc.File;
	Vector<D3D_SHADER_MACRO> macros;
	macros.reserve(env.Defines.size() + 1);
	for (auto const& pair : env.Defines)
	{
		macros.push_back({ pair.first.c_str(), pair.second.c_str() });
		if (ShaderCompileDebugging)
		{
			RLOG(LogDX11Shader, Info, "%s: %s", pair.first.c_str(), pair.second.c_str());
		}
	}
	switch (shaderType)
	{
	case rnd::EShaderType::Vertex:
		macros.push_back({"VERTEX", "1"});
		break;
	case rnd::EShaderType::Pixel:
		macros.push_back({"PIXEL", "1"});
		break;
	case rnd::EShaderType::Geometry:
		macros.push_back({"GEOMETRY", "1"});
		break;
	case rnd::EShaderType::Compute:
		macros.push_back({"COMPUTE", "1"});
		break;
	default:
		break;
	}
	macros.push_back({nullptr, nullptr});

	fs::create_directories(mOutDir);
	auto			 srcName = std::format("{}.hlsl", file);
	fs::path		 src = mSrcDir / srcName;
	if (!ZE_ENSURE(fs::exists(src)))
	{
		return nullptr;
	}
	auto			 lastWrite = fs::last_write_time(src);
	ComPtr<ID3DBlob> outBlob;
	ComPtr<ID3DBlob> errBlob;
	UINT			 flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
	flags |= D3DCOMPILE_DEBUG;
#endif

	auto	 csoName = std::format("{}_{:x}.cso", file, id.PermuatationId);
	fs::path cso = mOutDir / csoName;
	if (forceRecompile || !fs::exists(cso) || fs::last_write_time(cso) < lastWrite)
	{
		if (outBlob = dx::DXCompileFile(src.wstring().c_str(), desc.EntryPoint.c_str(), GetShaderTypeString(shaderType), Addr(macros), flags))
		{
			RLOG(LogGlobal, Verbose, "Saving to file %s\n", cso.string().c_str());
			HR_ERR_CHECK(D3DWriteBlobToFile(outBlob.Get(), cso.c_str(), true));
		}
		else
		{
			DebugBreak();
			return nullptr;
		}
	}
	else
	{
		RLOG(LogGlobal, Info, "Reading precompiled shader %s from %s\n", file.c_str(), cso.string().c_str());
		HR_ERR_CHECK(D3DReadFileToBlob(cso.c_str(), &outBlob));
	}

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
