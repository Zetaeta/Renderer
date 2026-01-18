#include "Asset.h"

#include <filesystem>
#include "core/Utils.h"

namespace fs = std::filesystem;

AssetPath::AssetPath(String primaryPath, String subPath /*= {}*/)
	: PrimaryPath(primaryPath), SubPath(subPath)
{
	if (SubPath.empty() && !PrimaryPath.empty())
	{
		auto lastSlash = primaryPath.find_last_of('/');	
		ZE_ASSERT(lastSlash < primaryPath.size());
		SubPath = primaryPath.substr(lastSlash + 1);
	}
}

EAssetSource AssetPath::GetSource() const
{
	if (PrimaryPath.starts_with("/Content/"))
	{
		return EAssetSource::Content;
	}
	else if (PrimaryPath.starts_with("/File/"))
	{
		return EAssetSource::File;
	}
	ZE_ASSERT(PrimaryPath.starts_with("/Memory/"));
	return EAssetSource::Memory;
}

String const& AssetPath::GetContentDir()
{
	static String contentDir = (std::filesystem::current_path() / "assets").string();
	return contentDir;
}

String AssetPath::ToFilePath() const
{
	if (PrimaryPath.starts_with(ContentPrefix))
	{
		return GetContentDir() + '/' + PrimaryPath.substr(STR_LEN(ContentPrefix));
	}
	if (PrimaryPath.starts_with(FilePrefix))
	{
		return PrimaryPath.substr(STR_LEN(FilePrefix));
	}
	return ""; 
}

String AssetPath::ToString() const
{
	return PrimaryPath + ':' + SubPath;
}

AssetPath AssetPath::GetAssetRootPath() const
{
	return AssetPath(PrimaryPath);
}

AssetPath AssetPath::WithSubpath(String subPath)
{
	return AssetPath(PrimaryPath, subPath);
}

AssetPath AssetPath::Content(const String& pathFromContent)
{
	return AssetPath(ContentPrefix + fs::path(pathFromContent).replace_extension().string());
}

AssetPath AssetPath::ContentFile(const String& filePath)
{
	fs::path path = fs::relative(filePath, GetContentDir());
	ZE_ASSERT(path.string()[0] != '.');
	return AssetPath(ContentPrefix + fs::path(path).replace_extension().string());
}

AssetPath AssetPath::File(const String& filePath)
{
	return AssetPath(FilePrefix + fs::path(filePath).replace_extension().string());
}


DEFINE_CLASS_TYPEINFO(AssetPath)
BEGIN_REFL_PROPS()
REFL_PROP(PrimaryPath)
REFL_PROP(SubPath)
END_REFL_PROPS()
END_CLASS_TYPEINFO()
