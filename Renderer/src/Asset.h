#pragma once
#include "BaseObject.h"

using AssetPath = String;

class Asset : public BaseObject
{
public:
	Asset(AssetPath const& path)
		: m_Path(path) {}
	using Ref = s32;
	AssetPath m_Path;
	AssetPath GetPath() const { return m_Path; }
};
