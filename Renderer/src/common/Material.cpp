#include "common/Material.h"
#include "render/RenderMaterial.h"
#include <render/dx11/DX11Renderer.h>



// Material::Material(Material&& other) noexcept
//	: colour(other.colour), specularExp(other.specularExp), specularity(other.specularity), diffuseness(other.diffuseness), translucent(other.translucent), roughness(other.roughness), albedo(std::move(other.albedo)), normal(std::move(other.normal)), roughnessMap(std::move(other.roughnessMap)),
//{
//	
//}

bool Material::NeedsUpdate() const
{
	return m_Updated.load(std::memory_order_acquire);
}

void Material::MarkUpdated()
{
	rnd::IRenderResourceManager::UpdateMaterials(Id);
	m_Updated.store(true, std::memory_order_release);
}

void Material::MarkUpdateDone()
{
	m_Updated.store(false, std::memory_order_relaxed);
}

void Material::OnPropertyUpdate()
{
	Props.translucent = Props.colour.a < 1;
}

rnd::MaterialPixelShader const* MaterialArchetypeDesc::GetShader(rnd::ShaderManager& shaderMgr, EShadingLayer layer,
	EMatType opacity, OwningPtr<rnd::IShaderReflector>* outReflector) const
{
	using namespace rnd;

	MaterialPixelShader::Permutation perm;

	switch (opacity)
	{
	case E_MT_OPAQUE:
		break;
	case E_MT_TRANSL:
		perm.Set<MaterialPixelShader::Translucent>(true);
		break;
	case E_MT_MASKED:
		perm.Set<MaterialPixelShader::Masked>(true);
		break;
	default:
		break;
	}

	switch (layer)
	{
	case EShadingLayer::BASE:
		perm.Set<MaterialPixelShader::FwdBaseLayer>(true);
		break;
	case EShadingLayer::Depth:
		perm.Set<MaterialPixelShader::FwdBaseLayer>(true);
		break;
	case EShadingLayer::DIRLIGHT:
		perm.Set<MaterialPixelShader::FwdDirLight>(true);
		break;
	case EShadingLayer::POINTLIGHT:
		perm.Set<MaterialPixelShader::FwdPointLight>(true);
		break;
	case EShadingLayer::SPOTLIGHT:
		perm.Set<MaterialPixelShader::FwdSpotLight>(true);
		break;
	case EShadingLayer::GBuffer:
		perm.Set<MaterialPixelShader::GBuffer>(true);
		break;
	default:
		return nullptr;
	}
	return shaderMgr.GetCompiledShader<rnd::MaterialPixelShader>(mPSRegistryId, perm, outReflector);
}

rnd::MaterialVertexShader const* MaterialArchetypeDesc::GetVertexShader(rnd::ShaderManager& shaderMgr, OwningPtr<rnd::IShaderReflector>* outReflector /*= nullptr*/) const
{
	rnd::MaterialVertexShader::Permutation perm;
	return shaderMgr.GetCompiledVertexShader<rnd::MaterialVertexShader>(mVSRegistryId, perm, mVertexMask, outReflector);
}

MaterialArchetypeDesc CreateAndRegisterMatDesc(const char* name, const char* psFilename, const char* psEntryPoint,
												const char* vsFilename, const char* vsEntryPoint, u64 vertexMask)
{
	using namespace rnd;
	ShaderTypeId psType = ShaderRegistry::Get().RegisterShaderType<MaterialPixelShader>(name, psFilename, psEntryPoint);
	ShaderTypeId vsType = ShaderRegistry::Get().RegisterShaderType<MaterialVertexShader>(name, vsFilename, vsEntryPoint);
	return MaterialArchetypeDesc {name, vsType, psType, (VertexAttributeMask) vertexMask};
}

