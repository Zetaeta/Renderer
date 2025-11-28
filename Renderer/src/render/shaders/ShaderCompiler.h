#pragma once
#include "core/Types.h"
#include "render/Shader.h"

#include <filesystem>
#include "render/DeviceShader.h"

namespace fs = std::filesystem;

DECLARE_LOG_CATEGORY(LogShaderCompiler);

namespace rnd {

class IShaderCompiler
{
public:
	virtual OwningPtr<IDeviceShader> CompileShader(ShaderInstanceId const& id, const ShaderDesc& desc,
		const ShaderCompileEnv& env, EShaderType ShaderType, VertexAttributeMask inputMask = {}, OwningPtr<IShaderReflector>* outReflector = nullptr, bool forceRecompile = false) = 0;
};
}

using ShaderFileId = HashString;

struct ShaderFileDesc
{
	ShaderFileId Id;
	fs::path Path;
	fs::file_time_type LastModified;
	Vector<ShaderFileId> Dependencies;
};

class ShaderCompilerData
{
public:
	fs::path ResolveShaderPath(fs::path const& inPath, Span<fs::path const> additionalFolders = {});
	ShaderFileId GetFileId(fs::path const& shaderPath) const;
	ShaderFileDesc const* GetFileData(fs::path const& shaderPath);

	fs::file_time_type LastUpdatedTime(const ShaderFileDesc& shaderFile);
protected:
	std::unordered_map<ShaderFileId, ShaderFileDesc> mShaderFiles;
	fs::path mSrcDir = "src\\shaders";
	Vector<fs::path> mIncludeFolders;
	Vector<ShaderFileId> mCurrentStack;
};
