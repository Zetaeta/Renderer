#pragma once

#include "InputImgui.h"
#include "ImageRenderer.h"
#include "ImageDX11.h"

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
		m_Img = std::make_unique<ImageDX11>(m_Device);
		m_Img->Init(m_ViewWidth, m_ViewHeight, &m_ImgData[0], false);
		ImGui::Image(m_Img->GetHandle(), { static_cast<float>(m_ViewWidth), static_cast<float>(m_ViewHeight) }, { 0, 1 }, { 1, 0 });
	}
	std::unique_ptr<ImageDX11> m_Img;
	ID3D11Device* m_Device;


};
