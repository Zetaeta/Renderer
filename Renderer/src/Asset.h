#pragma once
#include "BaseObject.h"

using AssetPath = String;
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
