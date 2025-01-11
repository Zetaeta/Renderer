#pragma once
#include "render/Shader.h"
#include "common/Material.h"
#include "ShaderPermutation.h"

namespace rnd
{


class MaterialPixelShader : public PixelShader
{
public:
	struct Opaque : SHADER_PERM_BOOL("OPAQUE");
	struct Masked : SHADER_PERM_BOOL("MASKED");
	struct Translucent : SHADER_PERM_BOOL("TRANSLUCENT");
	struct FwdPointLight : SHADER_PERM_BOOL("POINT_LIGHT");
	struct FwdDirLight : SHADER_PERM_BOOL("DIR_LIGHT");
	struct FwdSpotLight : SHADER_PERM_BOOL("SPOTLIGHT");
	struct FwdBaseLayer : SHADER_PERM_BOOL("BASE_LAYER");
	struct GBuffer : SHADER_PERM_BOOL("GBUFFER");

	using Permutation = PermutationDomain<Opaque, Masked, Translucent, FwdPointLight, FwdDirLight, FwdSpotLight, FwdBaseLayer, GBuffer>;
};

class MaterialVertexShader : public VertexShader
{

};

}
