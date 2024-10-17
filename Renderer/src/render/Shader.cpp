#include "Shader.h"
#include <core/Hash.h>

namespace rnd
{

ShaderRegistry& ShaderRegistry::Get()
{
	static ShaderRegistry sSingleton;
	return sSingleton;
}

void DynamicShaderPermutation::ModifyCompileEnv(ShaderCompileEnv& env) const
{
	for (auto const& pair : mEnvironment)
	{
		env.SetEnv(pair.first, pair.second);
	}
}

u64 DynamicShaderPermutation::GetUniqueId() const
{
	u64 hash = 0;
	for (auto const pair : mEnvironment)
	{
		hash = CombineHash(hash, pair.first);
		hash = CombineHash(hash, pair.second);
	}
	return hash;
}

}
