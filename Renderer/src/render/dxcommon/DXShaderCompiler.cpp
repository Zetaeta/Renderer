#include "DXShaderCompiler.h"
#include "d3dcompiler.h"

DEFINE_LOG_CATEGORY(LogShaderCompiler);

namespace rnd::dx
{

constexpr bool ShaderCompileDebugging = true;
ComPtr<ID3DBlob> FXCShaderCompiler::GetShaderBytecode(ShaderInstanceId const& id, const ShaderDesc& desc, const ShaderCompileEnv& env, EShaderType shaderType, bool forceRecompile)
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
		DebugBreak();
		return nullptr;
	}
}


}