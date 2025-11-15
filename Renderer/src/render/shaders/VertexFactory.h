#pragma once
#include "core/Maths.h"
#include "render/Shader.h"
#include "core/ContainerTypeInfo.h"

struct PerInstanceVSData
{
	SHADER_PARAMETER_STRUCT_START(PerInstanceVSData)
	SHADER_PARAMETER(float4x4, fullTrans)
	SHADER_PARAMETER(float4x4, model2Shade)
	SHADER_PARAMETER(float4x4, model2ShadeDual)
	SHADER_PARAMETER_STRUCT_END(PerInstanceVSData)
};

class VertexFactory
{
	
};
