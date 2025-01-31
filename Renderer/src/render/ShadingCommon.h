#pragma once

#include "core/Maths.h"

enum class EShadingLayer : u8;

namespace rnd
{

class RenderContext;

using col = vec3; 

__declspec(align(16))
struct PerFramePSData
{
	col ambient;
	int debugMode = 0;
	float debugGrayscaleExp = 1;
	int brdf = 0;
	float roughnessMod = 0;
	float metalnessMod = 0;
};

__declspec(align(16))
struct PFPSDirLight// : PerFramePSData
{
	col directionalCol;
	float _pad2;
	vec3 directionalDir;
	float _pad3;
	mat4 world2Light;
	int debugMode = 0;
	float debugGrayscaleExp = 1;
	int brdf = 0;
	float roughnessMod = 0;
	float metalnessMod = 0;
};

__declspec(align(16))
struct PFPSPointLight// : PerFramePSData
{
	vec3 pointLightPos;
	float pointLightRad;
	vec3 pointLightCol;
	float pointLightFalloff;
	mat4 world2Light;
	int debugMode = 0;
	float debugGrayscaleExp = 1;
	int brdf = 0;
	float roughnessMod = 0;
	float metalnessMod = 0;
};

__declspec(align(16))
struct PFPSSpotLight// : PerFramePSData
{
	float3 spotLightPos;
	float spotLightTan;
	float3 spotLightDir;
	float spotLightFalloff;
	float3 spotLightCol;
	float _pad;
	mat4 world2Light;
	int debugMode = 0;
	float debugGrayscaleExp = 1;
	int brdf = 0;
	float roughnessMod = 0;
	float metalnessMod = 0;
};

void SetupShadingLayer(RenderContext* renderContext, IRenderDeviceCtx& ctx, EShadingLayer lightType, u32 lightIdx);

}
