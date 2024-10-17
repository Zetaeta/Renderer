#pragma once

#include "render/Shader.h"

namespace rnd
{

class PostProcessVS : public VertexShader
{
	VS_INPUTS(VA_Position);
	DECLARE_SHADER(PostProcessVS);
};

class OutlinePPPS : public PixelShader
{
	DECLARE_SHADER(OutlinePPPS);
};

}
