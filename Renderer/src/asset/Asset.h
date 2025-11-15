#pragma once
#include "core/BaseObject.h"
#include "core/Hash.h"

enum class EAssetSource : u8
{
	Content,
	File,
	Memory
};

struct AssetPath
{
	DECLARE_STI_NOBASE(AssetPath);
	AssetPath() = default;
	explicit AssetPath(String primaryPath, String subPath = {});

	EAssetSource GetSource() const;
	
	static String const& GetContentDir();

	constexpr static char const ContentPrefix[] = "/Content/";
	constexpr static char const FilePrefix[] = "/File/";
	constexpr static char const MemoryPrefix[] = "/Memory/";

	String ToFilePath() const;

	String ToString() const;

	AssetPath GetAssetRootPath() const;

	AssetPath WithSubpath(String subPath);

	static AssetPath Content(const String& pathFromContent);
	static AssetPath ContentFile(const String& filePath);

	static AssetPath File(const String& filePath);

	bool operator==(const AssetPath& other) const
	{
		return PrimaryPath == other.PrimaryPath && SubPath == other.SubPath;
	}

public:
	String PrimaryPath;
	String SubPath;
};

template<>
struct std::hash<AssetPath>
{
	size_t operator()(const AssetPath& assetPath) const
	{
		std::hash<Name> hasher;
		return CombineHash(hasher(assetPath.PrimaryPath), assetPath.SubPath);
	}
};

class AssetManager;

class Asset : public BaseObject
{
	friend class AssetManager;
public:
	Asset(AssetPath const& path)
		: m_Path(path) {}
	using Ref = s32;
	AssetPath m_Path;
	AssetPath GetPath() const { return m_Path; }

protected:
	void SetPath(AssetPath const& path) { m_Path = path; }
};
