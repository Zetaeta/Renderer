#include "DX12ShaderCompiler.h"
#include "core/Types.h"
#include "core/String.h"
#include "d3d12shader.h"
#include "render/dxcommon/DXGIUtils.h"

#pragma comment(lib, "dxcompiler.lib")
//#pragma comment(lib, "dxil.lib")

namespace rnd::dx12
{

OwningPtr<IDeviceShader> DX12LegacyShaderCompiler::CompileShader(ShaderInstanceId const& id,
	const ShaderDesc& desc, const ShaderCompileEnv& env, EShaderType ShaderType, VertexAttributeMask inputMask,
		OwningPtr<IShaderReflector>* outReflector, bool forceRecompile)
{
	auto bc = GetShaderBytecode(id, desc, env, ShaderType, outReflector, forceRecompile);
	if (bc)
	{
		return MakeOwning<DX12Shader>(DX12Shader{bc});
	}
	return nullptr;
}

DX12LegacyShaderCompiler::DX12LegacyShaderCompiler()
{
	mSrcDir = "src\\shaders";
	mOutDir = "generated\\shaders";
	mTypeStrings[EShaderType::Pixel] = "ps_5_0";
	mTypeStrings[EShaderType::Vertex] = "vs_5_0";
	mTypeStrings[EShaderType::Compute] = "cs_5_0";
}

class DX12ShaderReflector : public IShaderReflector
{

public:
	DX12ShaderReflector(ComPtr<ID3D12ShaderReflection>&& reflection)
		: mReflection(std::move(reflection)) {}

	SmallVector<CBDesc, 4> GetConstantBuffers() override;

	void GetBindingInfo(_Out_ SmallVector<SRVBindingDesc, 4>& srvs, _Out_ SmallVector<UAVBindingDesc, 4>& uavs) override;

	ShaderParamersInfo GetParamsInfo() override;

	ShaderSignatureParam TranslateSignature(const D3D12_SIGNATURE_PARAMETER_DESC& desc);
private:
	ComPtr<ID3D12ShaderReflection> mReflection;
};

DX12ShaderCompiler::DX12ShaderCompiler()
{
	mOutDir = "generated\\dx12\\shaders";
	mTypeStrings[EShaderType::Pixel] = "ps_6_0";
	mTypeStrings[EShaderType::Vertex] = "vs_6_0";
	mTypeStrings[EShaderType::Compute] = "cs_6_0";
	mTypeStrings[EShaderType::Geometry] = "gs_6_0";
	mTypeStrings[EShaderType::Mesh] = "ms_6_0";

	HR_ERR_CHECK(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&mCompiler)));
	HR_ERR_CHECK(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&mDxcUtils)));
}

void WriteBlob(ComPtr<IDxcBlob> const& blob, const fs::path& filePath)
{
	FILE* file = nullptr;
	fopen_s(&file, filePath.string().c_str(), "wb");
	if (ZE_ENSURE(file))
	{
		fwrite(blob->GetBufferPointer(), blob->GetBufferSize(), 1, file);
		fclose(file);
	}
}

OwningPtr<rnd::IDeviceShader> DX12ShaderCompiler::CompileShader(ShaderInstanceId const& id, const ShaderDesc& desc, const ShaderCompileEnv& env, EShaderType ShaderType, VertexAttributeMask inputMask, OwningPtr<IShaderReflector>* outReflector, bool forceRecompile)
{
	const String& file = desc.File;
	auto			 srcName = std::format("{}.hlsl", file);
	fs::path		 src = mSrcDir / srcName;
	if (!ZE_ENSURE(fs::exists(src)))
	{
		return nullptr;
	}

	ComPtr<IDxcBlobEncoding> sourceBlob;
	UINT codePage = DXC_CP_UTF8;
	HR_ERR_CHECK(mDxcUtils->LoadFile(src.c_str(), &codePage, &sourceBlob));
	DxcBuffer Source;
	Source.Ptr = sourceBlob->GetBufferPointer();
	Source.Size = sourceBlob->GetBufferSize();
	BOOL knownEncoding = false;
	HR_ERR_CHECK(sourceBlob->GetEncoding(&knownEncoding, &Source.Encoding));
	ZE_ENSURE(knownEncoding);

	// TODO use scratch allocator 
	Vector<HeapWString> wstrings;
	Vector<DxcDefine> defines;
	defines.reserve(env.Defines.size());
	wstrings.reserve(env.Defines.size() * 2);
	for (const auto& define : env.Defines)
	{
		auto key = ToWideHeapString(define.first);
		auto value = ToWideHeapString(define.second);
		defines.push_back({ key.Get(), value.Get() });
		wstrings.push_back(std::move(key));
		wstrings.push_back(std::move(value));
	}
	std::wstring wSourceName = ToWideString(desc.File);
	std::wstring wEntryPoint = ToWideString(desc.EntryPoint);
	std::wstring typeStr = ToWideString(mTypeStrings[ShaderType]);
	//LPCWSTR typePrefix = L"-T";
	//LPCWSTR args[] =
	//{
	//	typePrefix,
	//	typeStr.c_str()
	//};

	ComPtr<IDxcCompilerArgs> args;
	HR_ERR_CHECK(mDxcUtils->BuildArguments(wSourceName.c_str(), wEntryPoint.c_str(), typeStr.c_str(), nullptr, 0, defines.data(), static_cast<u32>(defines.size()), &args));
	ComPtr<IDxcResult> result;
	mCompiler->Compile(&Source, args->GetArguments(), args->GetCount(), this, IID_PPV_ARGS(&result));

	ComPtr<IDxcBlobUtf8> errors;
	result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
	if (errors != nullptr && errors->GetStringLength() > 0)
	{
		RLOG(LogShaderCompiler, Error, "Warnings or errors compiling %s:\n%s", desc.Name.c_str(), errors->GetStringPointer());
	}
	HRESULT status;
	result->GetStatus(&status);
	if (FAILED(status))
	{
		ZE_ENSURE(false);
		return nullptr;
	}

	ComPtr<IDxcBlob> shader;
	ComPtr<IDxcBlobUtf16> shaderName;
	result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), &shaderName);
	if (!ZE_ENSURE(shader != nullptr))
	{
		return nullptr;
	}
	fs::create_directories(mOutDir);
	auto	 csoName = std::format("{}_{:x}.cso", file, id.PermuatationId);
	fs::path cso = mOutDir / csoName;
	WriteBlob(shader, cso);

	ComPtr<IDxcBlob> pdb;
	ComPtr<IDxcBlobUtf16> pdbName;
	result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pdb), &pdbName);
	if (pdb != nullptr)
	{
		WriteBlob(pdb, mOutDir / pdbName->GetStringPointer());
	}

	if (outReflector)
	{
		ComPtr<IDxcBlob> reflectionData;
		result->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflectionData), nullptr);
		if (reflectionData != nullptr)
		{
			// Create reflection interface.
			DxcBuffer ReflectionData;
			ReflectionData.Encoding = DXC_CP_ACP;
			ReflectionData.Ptr = reflectionData->GetBufferPointer();
			ReflectionData.Size = reflectionData->GetBufferSize();

			ComPtr<ID3D12ShaderReflection> reflection;
			mDxcUtils->CreateReflection(&ReflectionData, IID_PPV_ARGS(&reflection));

			*outReflector = MakeOwning<DX12ShaderReflector>(std::move(reflection));
		}
	}

	// These are different classes but the same COM interface...
	ComPtr<ID3DBlob> compatBlob;
	HR_ERR_CHECK(shader.As(&compatBlob));
	return MakeOwning<DX12Shader>(compatBlob);
}

HRESULT DX12ShaderCompiler::LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource)
{
	fs::path resolvedPath = ResolveShaderPath(pFilename, mCurrentCompilePaths);
	fs::path folder = resolvedPath.parent_path();
	if (folder != mSrcDir && !Contains(mIncludeFolders, folder) && !Contains(mCurrentCompilePaths, folder))
	{
		mCurrentCompilePaths.push_back(folder);
	}

	IDxcBlobEncoding* blob = nullptr;
	HRESULT result = mDxcUtils->LoadFile(resolvedPath.c_str(), nullptr, &blob);
	if (blob)
	{
		*ppIncludeSource = blob;
	}

	return result;
}

HRESULT STDMETHODCALLTYPE DX12ShaderCompiler::QueryInterface(REFIID riid, void** ppvObject)
{
	if (riid == __uuidof(IDxcIncludeHandler) || riid == IID_IUnknown)
	{
		*ppvObject = (IDxcIncludeHandler*) this;
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE DX12ShaderCompiler::AddRef(void)
{
	return ++mRefCount;
}

ULONG STDMETHODCALLTYPE DX12ShaderCompiler::Release(void)
{
	return --mRefCount;
}

DX12ShaderCompiler::~DX12ShaderCompiler()
{
	ZE_ASSERT(mRefCount == 0);
}

SmallVector<rnd::IShaderReflector::CBDesc, 4> DX12ShaderReflector::GetConstantBuffers()
{
	// Effectively duplicated in DX11ShaderReflector
	SmallVector<IShaderReflector::CBDesc, 4> result;
	D3D12_SHADER_DESC desc;
	mReflection->GetDesc(&desc);
	for (u32 i = 0; i < desc.ConstantBuffers; ++i)
	{
		ID3D12ShaderReflectionConstantBuffer* cBuffer = mReflection->GetConstantBufferByIndex(i);
		D3D12_SHADER_BUFFER_DESC			  cbDesc;
		cBuffer->GetDesc(&cbDesc);
		result.push_back({cbDesc.Name, cbDesc.Size});
		for (u32 i=0; i<cbDesc.Variables; ++i)
		{
			auto* variable = cBuffer->GetVariableByIndex(i);
			ZE_ASSERT(variable);
			D3D12_SHADER_VARIABLE_DESC varDesc;
			DXPARANOID(variable->GetDesc(&varDesc));
//			ZE_ASSERT(varDesc.uFlags & D3D_SVF_USED);
			result.back().VariableNames.push_back(varDesc.Name);
			result.back().VariableSizes.push_back(varDesc.Size);
			result.back().VariableOffsets.push_back(varDesc.StartOffset);
		}
	}
	return result;
}

void DX12ShaderReflector::GetBindingInfo(_Out_ SmallVector<SRVBindingDesc, 4>& srvs, _Out_ SmallVector<UAVBindingDesc, 4>& uavs)
{
	
	D3D12_SHADER_DESC desc;
	mReflection->GetDesc(&desc);

	for (u32 i=0; i<desc.BoundResources; ++i)
	{
		D3D12_SHADER_INPUT_BIND_DESC bindDesc;
		DXCALL(mReflection->GetResourceBindingDesc(i, &bindDesc));
		BindingDesc result;
		result.Name = bindDesc.Name;
		switch (bindDesc.Dimension)
		{
		case D3D_SRV_DIMENSION_UNKNOWN:
			result.Type = EResourceType::Unknown;
			break;
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

rnd::ShaderParamersInfo DX12ShaderReflector::GetParamsInfo()
{
	ShaderParamersInfo result = IShaderReflector::GetParamsInfo();

	D3D12_SHADER_DESC shaderDesc;
	DXCALL(mReflection->GetDesc(&shaderDesc));
	for (u32 i = 0; i < shaderDesc.InputParameters; ++i)
	{
		D3D12_SIGNATURE_PARAMETER_DESC signature;
		DXCALL(mReflection->GetInputParameterDesc(i, &signature));
		auto const& newInput = result.Inputs.Params.emplace_back(TranslateSignature(signature));
		result.Inputs.SVMask |= newInput.SV;
	}
	for (u32 i = 0; i < shaderDesc.OutputParameters; ++i)
	{
		D3D12_SIGNATURE_PARAMETER_DESC signature;
		DXCALL(mReflection->GetOutputParameterDesc(i, &signature));
		auto const& newOutput = result.Outputs.Params.emplace_back(TranslateSignature(signature));
		result.Outputs.SVMask |= newOutput.SV;
	}

	return result;
}

rnd::ShaderSignatureParam DX12ShaderReflector::TranslateSignature(D3D12_SIGNATURE_PARAMETER_DESC const& desc)
{
	ShaderSignatureParam result;
	result.SemanticName = desc.SemanticName;
	result.SemanticIdx = desc.SemanticIndex;
	result.SV = TranslateSV(desc.SystemValueType);

	return result;
}

}
