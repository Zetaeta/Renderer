
#include "ShaderDeclarations.h"

namespace rnd
{
DEFINE_SHADER(WavyPPPS, "WavyPP_PS", "main");
DEFINE_SHADER(OutlinePPPS, "OutlinePP_PS", "main");
DEFINE_SHADER(PostProcessVS, "PostProcess_VS", "main");
DEFINE_SHADER(FlatPS, "2D_PS", "main");
DEFINE_SHADER(CubemapVS, "BGVertexShader", "main");
DEFINE_SHADER(CubemapPS, "BGPixelShader", "main");
}