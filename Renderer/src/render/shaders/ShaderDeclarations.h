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

class CubemapVS : public VertexShader
{
	VS_INPUTS_START
	VS_INPUT_STATIC(VA_Position)
	VS_INPUTS_END
	DECLARE_SHADER(CubemapVS)
};


class CubemapPS : public PixelShader
{
public:
	struct Greyscale : SHADER_PERM_BOOL("DEPTH_SAMPLE");
	using Permutation = PermutationDomain<Greyscale>;
	DECLARE_SHADER(CubemapPS);
};

class DownsampleCS : public ComputeShader
{
public:
	constexpr static u32 MaxDownsamples = 4;
	struct NumLevels : SHADER_PERM_UINT("NUM_DOWNSAMPLE_LEVELS", MaxDownsamples);
	struct IsSrgb : SHADER_PERM_BOOL("IS_SRGB");
	using Permutation = PermutationDomain<NumLevels, IsSrgb>;
	DECLARE_SHADER(DownsampleCS);

};
}
