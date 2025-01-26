#include "DXShaderCompiler.h"
#include "d3dcompiler.h"
#include "d3d11shader.h"
#include "SharedD3D.h"

DEFINE_LOG_CATEGORY(LogShaderCompiler);

namespace rnd::dx
{

class DX11ShaderReflector final : public IShaderReflector
{
public:
	DX11ShaderReflector(ComPtr<ID3D11ShaderReflection>&& reflection);
	SmallVector<CBDesc, 4> GetConstantBuffers() override;

	void GetBindingInfo(OUT SmallVector<SRVDesc, 4>& srvs, OUT SmallVector<UAVDesc, 4>& uavs) override;

private:
	ComPtr<ID3D11ShaderReflection> mReflection;
};

constexpr bool ShaderCompileDebugging = true;
ComPtr<ID3DBlob> FXCShaderCompiler::GetShaderBytecode(ShaderInstanceId const& id, const ShaderDesc& desc,
	const ShaderCompileEnv& env, EShaderType shaderType, OwningPtr<IShaderReflector>* outReflector, bool forceRecompile)
{
	const String& file = desc.File;
	Vector<D3D_SHADER_MACRO> macros;
	macros.reserve(env.Defines.size() + 1);
	for (auto const& pair : env.Defines)
	{
		macros.push_back({ pair.first.c_str(), pair.second.c_str() });
		if (ShaderCompileDebugging)
		{
			RLOG(LogShaderCompiler, Info, "%s: %s", pair.first.c_str(), pair.second.c_str());
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
		if (outBlob = DXCompileFile(src.wstring().c_str(), desc.EntryPoint.c_str(), mTypeStrings[shaderType], Addr(macros), flags))
		{
			RLOG(LogGlobal, Verbose, "Saving to file %s\n", cso.string().c_str());
			HR_ERR_CHECK(D3DWriteBlobToFile(outBlob.Get(), cso.c_str(), true));
		}
		else
		{
			return nullptr;
		}
	}
	else
	{
		RLOG(LogGlobal, Info, "Reading precompiled shader %s from %s\n", file.c_str(), cso.string().c_str());
		HR_ERR_CHECK(D3DReadFileToBlob(cso.c_str(), &outBlob));
	}

	if (outReflector)
	{
		ComPtr<ID3D11ShaderReflection> reflectionData;
		if (SUCCEEDED(D3DReflect(outBlob->GetBufferPointer(), outBlob->GetBufferSize(), IID_PPV_ARGS(&reflectionData))))
		{
			*outReflector = MakeOwning<DX11ShaderReflector>(std::move(reflectionData));
		}
	}
	return outBlob;
}

ComPtr<ID3DBlob> DXCompileFile(wchar_t const* filePath, char const* entryPoint, char const* shaderTarget, D3D_SHADER_MACRO const* macros, UINT flags)
{

	ComPtr<ID3DBlob> outBlob;
	ComPtr<ID3DBlob> errBlob;
	RLOG(LogGlobal, Info, "Compiling shader in file %S", filePath);
	HRESULT hr = D3DCompileFromFile(filePath, macros, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			entryPoint, shaderTarget, flags, 0, &outBlob, &errBlob);
	if (SUCCEEDED(hr))
	{
		if (errBlob != nullptr && errBlob->GetBufferPointer() != nullptr)
		{
			RLOG(LogGlobal, Info, "Compile warning: %s\n", (const char*) errBlob->GetBufferPointer());
		}
		return outBlob;
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
		ZE_ENSURE(false);
		return nullptr;
	}
}

SmallVector<IShaderReflector::CBDesc, 4> DX11ShaderReflector::GetConstantBuffers()
{
	SmallVector<IShaderReflector::CBDesc, 4> result;
	D3D11_SHADER_DESC desc;
	mReflection->GetDesc(&desc);
	for (u32 i = 0; i < desc.ConstantBuffers; ++i)
	{
		ID3D11ShaderReflectionConstantBuffer* cBuffer = mReflection->GetConstantBufferByIndex(i);
		D3D11_SHADER_BUFFER_DESC			  cbDesc;
		cBuffer->GetDesc(&cbDesc);
		result.push_back({cbDesc.Name, cbDesc.Size});
	}
	return result;
}

DX11ShaderReflector::DX11ShaderReflector(ComPtr<ID3D11ShaderReflection>&& reflection) :mReflection(std::move(reflection))
{

}

void DX11ShaderReflector::GetBindingInfo(_Out_ SmallVector<SRVDesc, 4>& srvs, _Out_ SmallVector<UAVDesc, 4>& uavs)
{
	D3D11_SHADER_DESC desc;
	mReflection->GetDesc(&desc);

	for (u32 i=0; i<desc.BoundResources; ++i)
	{
		D3D11_SHADER_INPUT_BIND_DESC bindDesc;
		DXCALL(mReflection->GetResourceBindingDesc(i, &bindDesc));
		BindingDesc result;
		result.Name = bindDesc.Name;
		switch (bindDesc.Dimension)
		{
		case D3D_SRV_DIMENSION_UNKNOWN:
			result.Type = EResourceType::Unknown;
		case D3D_SRV_DIMENSION_BUFFER:
			result.Type = EResourceType::Buffer;
			break;
		case D3D_SRV_DIMENSION_TEXTURE2D:
		case D3D_SRV_DIMENSION_TEXTURE2DMS:
			result.Type = EResourceType::Texture2D;
			break;
		case D3D_SRV_DIMENSION_TEXTURECUBE:
			result.Type = EResourceType::TextureCube;
			break;
		default:
			ZE_ASSERT(false);
			break;
		}
		switch (bindDesc.Type)
		{
		case D3D_SIT_TEXTURE:
		case D3D_SIT_STRUCTURED:
			srvs.push_back(result);
			break;
		case D3D_SIT_UAV_RWTYPED:
		case D3D_SIT_UAV_RWSTRUCTURED:
			uavs.push_back(result);
			break;
		case D3D_SIT_CBUFFER:
			break;
		case D3D_SIT_SAMPLER:
			break;
		default:
			ZE_ASSERT(false);
			break;
		}
	}
}
}
