#include "DX11ShaderCompiler.h"
#include "D3Dcompiler.h"
#include "core/WinUtils.h"
#include "core/Logging.h"
#include "SharedD3D11.h"
namespace rnd
{
namespace dx11
{

OwningPtr<IDeviceShader> DX11ShaderCompiler::CompileShader(ShaderInstanceId const& id, const ShaderDesc& desc,
	const ShaderCompileEnv& env, EShaderType shaderType, VertexAttributeMask inputMask, bool forceRecompile)
{
	const String& file = desc.File;
	Vector<D3D_SHADER_MACRO> macros;
	macros.reserve(env.Defines.size());
	for (auto const& pair : env.Defines)
	{
		macros.push_back({ pair.first.c_str(), pair.second.c_str() });
	}

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
//	flags |= D3DCOMPILE_DEBUG;
#endif

	auto	 csoName = std::format("{}_{:x}.cso", file, id.PermuatationId);
	fs::path cso = mOutDir / csoName;
	if (forceRecompile || !fs::exists(cso) || fs::last_write_time(cso) < lastWrite)
	{
		RLOG(LogGlobal, Info, "Compiling shader in file %s\n", src.string().c_str());
		HRESULT hr = D3DCompileFromFile(src.wstring().c_str(), Addr(macros), D3D_COMPILE_STANDARD_FILE_INCLUDE, desc.EntryPoint.c_str(), GetShaderTypeString(shaderType), flags, 0, &outBlob, &errBlob);
		if (SUCCEEDED(hr))
		{
			RLOG(LogGlobal, Info, "Saving to file %s\n", cso.string().c_str());
			HR_ERR_CHECK(D3DWriteBlobToFile(outBlob.Get(), cso.c_str(), true));
			if (errBlob != nullptr && errBlob->GetBufferPointer() != nullptr)
			{
				RLOG(LogGlobal, Info, "Compile warning: %s\n", (const char*) errBlob->GetBufferPointer());
			}
		}
		else
		{
			LPTSTR errorText = NULL;
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM
						| FORMAT_MESSAGE_ALLOCATE_BUFFER
						| FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL, hr, 0, (LPTSTR) &errorText, 0, NULL);
			RLOG(LogGlobal, Info, "Compile error: %S", errorText);
			if (errBlob != nullptr && errBlob->GetBufferPointer() != nullptr)
			{
				RLOG(LogGlobal, Info, "Output: %s\n", (const char*) errBlob->GetBufferPointer());
			}
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
