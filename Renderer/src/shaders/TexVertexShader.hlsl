#include "VertexShader.hlsli"
#include "MaterialDefinitions.hlsli"

#ifdef MATERIAL_VERTEX_DEFINITION
#include MATERIAL_VERTEX_DEFINITION
#else
VSInputs ModifyVertexProperties(VSInputs vsi)
{
    return vsi;
}
#endif

VSOut main( VSInputs vsi)
{
    vsi = ModifyVertexProperties(vsi);
	VSOut vso;
	vso.pos = mul(fullTrans,float4(vsi.Position, 1));
#if SHADED
	float4 worldPos = mul(model2Shade, float4(vsi.Position,1));
	vso.worldPos = worldPos.xyz;//(n + float3(1,1,1))/2;
	vso.normal = mul(model2ShadeDual, float4(vsi.Normal,0)).xyz;
	vso.tangent = mul(model2Shade, float4(vsi.Tangent,0)).xyz;
	vso.viewDir = mul(model2Shade, float4(vsi.Position,1)).xyz - cameraPos;
#endif
#if TEXTURED
	vso.uv = vsi.UV;
#endif //TEXTURED
	return vso;
}
