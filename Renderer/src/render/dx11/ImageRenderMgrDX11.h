#pragma once

#include "common/InputImgui.h"
#include "render/ImageRenderer.h"
#include "render/dx11/DX11Texture.h"

namespace rnd
{
namespace dx11
{

class ImageRenderMgrDX11 : public ImageRenderMgr
{
public:
	ImageRenderMgrDX11(ID3D11Device* device)
		: m_Device(device), ImageRenderMgr(new InputImgui()) {
	}

	void OnRenderStart()
	{
		m_Camera.Tick(m_FrameTime );
	}

	void OnRenderFinish() override
	{
//		m_Img = std::make_unique<DX11Texture>(m_Device);
		m_Img->Init(m_ViewWidth, m_ViewHeight, &m_ImgData[0], TF_NONE);
		ImGui::Image(reinterpret_cast<u64>(m_Img->GetHandle()), { static_cast<float>(m_ViewWidth), static_cast<float>(m_ViewHeight) }, { 0, 1 }, { 1, 0 });
	}
	std::unique_ptr<DX11Texture> m_Img;
	ID3D11Device* m_Device;
};

}
}
