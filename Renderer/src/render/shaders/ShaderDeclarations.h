#pragma once

#include "render/Shader.h"
#include "ShaderPermutation.h"

namespace rnd
{

class PostProcessVS : public VertexShader
{
public:
	struct UseUVs : SHADER_PERM_BOOL("USE_UVS");

	using Permutation = PermutationDomain<UseUVs>;

	VS_INPUTS_START
	VS_INPUT_STATIC(VA_Position)
	VS_INPUT_SWITCH(UseUVs, VA_TexCoord)
	VS_INPUTS_END

	DECLARE_SHADER(PostProcessVS);
};

class OutlinePPPS : public PixelShader
{
	DECLARE_SHADER(OutlinePPPS);
};

class WavyPPPS : public PixelShader
{
	DECLARE_SHADER(WavyPPPS);
};

class FlatPS : public PixelShader
{
public:
	struct UseUVs : SHADER_PERM_BOOL("CUSTOMIZATION");

	using Permutation = PermutationDomain<UseUVs>;
	DECLARE_SHADER(FlatPS);
};


}
