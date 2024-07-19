#include "common/Material.h"
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
	m_Updated.store(true, std::memory_order_release);
}

void Material::MarkUpdateDone()
{
	m_Updated.store(false, std::memory_order_relaxed);
}

void Material::OnPropertyUpdate()
{
	translucent = colour.a < 1;
}

IDeviceMaterial* MaterialManager::GetDefaultMaterial(int matType)
{
	return static_cast<rnd::dx11::DX11Renderer*>(mRenderer)->GetDefaultMaterial(matType);
}
