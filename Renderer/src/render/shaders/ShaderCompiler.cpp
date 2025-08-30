#include "ShaderCompiler.h"
#include "core/StringView.h"
#include <fstream>
#include "container/Vector.h"

fs::path ShaderCompilerData::ResolveShaderPath(fs::path const& inPath, Span<fs::path const> additionalFolders)
{
	fs::path result = mSrcDir / inPath;
	if (fs::exists(result))
	{
		return result;
	}
	for (const fs::path& otherFolder : additionalFolders)
	{
		result = otherFolder / inPath;
		if (fs::exists(result))
		{
			return result;
		}
	}
	if (fs::exists(inPath))
	{
		return inPath;
	}
	for (const fs::path& otherFolder : mIncludeFolders)
	{
		result = otherFolder / inPath;
		if (fs::exists(result))
		{
			return result;
		}
	}

	return {};
}

ShaderFileId ShaderCompilerData::GetFileId(fs::path const& shaderPath) const
{
	auto relPath = shaderPath.lexically_relative(mSrcDir);
	return Name(relPath.string());
}

class StringParser
{
public:
	StringParser(StringView str)
		: mString(str) {}

	u32 SkipWhitespace()
	{
		u32 start = mPos;
		while (mPos < mString.size() && std::isspace(mString[mPos]))
		{
			++mPos;
		}
		return mPos - start;
	}

	bool ConsumeConstant(StringView constant)
	{
		u32 tempPos = mPos;
		u32 constPos = 0;
		while (tempPos < mString.size() && constPos < constant.size() && constant[constPos] == mString[tempPos])
		{
			++constPos;
			++tempPos;
		}

		if (constPos == constant.size())
		{
			mPos = tempPos;
			return true;
		}
		else
		{
			return false;
		}
	}

	bool ExtractIncludePath(String& outInclude)
	{
		if (mPos >= mString.size() - 2)
		{
			return false;
		}
		u32 pos = mPos;
		char const start = mString[pos++];
		if (start != '"' && start != '<')
		{
			return false;
		}

		bool done = false;
		for (; pos < mString.size() && !done; ++pos)
		{
			char c = mString[pos];
			if (std::isalnum(c))
			{
				outInclude.push_back(c);
			}
			else
			{
				switch (c)
				{
					case '"':
						if (start == '"')
						{
							done = true;
						}
						else
						{
							return false;
						}
						break;
					case '>':
						if (start == '<')
						{
							done = true;
						}
						else
						{
							return false;
						}
						break;
					case '/':
					case '\\':
					case ' ':
					case '.':
						outInclude.push_back(c);
						break;
					default:
						ZE_ENSURE(false);
						break;
				}
			}
		}
		mPos = pos + 1;
		return true;
	}
	
private:
	StringView mString;
	u32 mPos = 0;
};

ShaderFileDesc const* ShaderCompilerData::GetFileData(fs::path const& shaderPath)
{
	fs::path exactPath = ResolveShaderPath(shaderPath);
	ShaderFileId fileId = GetFileId(exactPath);
	if (auto it = mShaderFiles.find(fileId); it != mShaderFiles.end())
	{
		return &it->second;
	}
	if (Find(mCurrentStack, fileId))
	{
		ZE_ASSERT(false);
		return nullptr;
	}
	mCurrentStack.push_back(fileId);

	std::ifstream inStream;

	inStream.open(exactPath.string());
	if (!inStream.is_open())
	{
		return nullptr;
	}
	ShaderFileDesc& desc = mShaderFiles[fileId];
	desc.Id = fileId;
	desc.Path = exactPath.lexically_relative(mSrcDir);
	desc.LastModified = fs::last_write_time(exactPath);

	Vector<String> includes;
	String line;
	while (std::getline(inStream, line))
	{
		StringParser parser(line);
		parser.SkipWhitespace();
		if (parser.ConsumeConstant("#include"))
		{
			parser.SkipWhitespace();
			String include;
			if (parser.ExtractIncludePath(include))
			{
				includes.push_back(include);
			}
		}
	}

	fs::path shaderDir = exactPath.parent_path();

	for (const String& include : includes)
	{
		fs::path includePath(include);

		ShaderFileDesc const* includeDesc = GetFileData(ResolveShaderPath(includePath, Single(shaderDir)));
		if (ZE_Ensuref(includeDesc != nullptr, "Couldn't resolve shader include %s", include.c_str()))
		{
			desc.Dependencies.push_back(includeDesc->Id);
		}
	}
	mCurrentStack.pop_back();
	return &desc;
}

fs::file_time_type ShaderCompilerData::LastUpdatedTime(const ShaderFileDesc& shaderFile)
{
	if (Find(mCurrentStack, shaderFile.Id))
	{
		ZE_Ensuref(false, "Recursive shader inclusion");
		return {};
	}
	mCurrentStack.push_back(shaderFile.Id);
	auto fileTime = shaderFile.LastModified;
	for (auto& dependency : shaderFile.Dependencies)
	{
		fileTime = std::max(fileTime, LastUpdatedTime(mShaderFiles.at(dependency)));
	}
	mCurrentStack.pop_back();
	return fileTime;
}
