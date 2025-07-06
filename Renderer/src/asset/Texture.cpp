#include "asset/Texture.h"
#include "stb_image.h"
#include "render/RenderResourceManager.h"
std::shared_ptr<Texture> const Texture::EMPTY = Create(0, 0, "empty", nullptr);

TextureRef Texture::LoadFrom(char const* fileName, bool isSRGB)
{
	int x, y, comp;
	unsigned char* data = stbi_load(fileName, &x, &y, &comp, 4);
//	fprintf(stdout, "%s: width: %d, height: %d, comp: %d\n", fileName, x, y, comp);
	if (data == nullptr)
	{
		return TextureRef {};
	}
	TextureRef result = (x > 0 && y > 0) ? Texture::Create(x,y, fileName,data , isSRGB ? ETextureFormat::RGBA8_Unorm_SRGB : ETextureFormat::RGBA8_Unorm
		) : Texture::EMPTY;
	return result;
}

static std::atomic<u64> NextTextureId = 0;

Texture::Texture(size_type width, size_type height, std::string const& name /*= "(unnamed)"*/, u8 const* data /*= nullptr*/, ETextureFormat format, bool upload)
: width(width), height(height), Id(NextTextureId++), m_Data(width* height), m_Name(name), Format(format)
{
	if (data != nullptr)
	{
		memcpy(&m_Data[0], data, width * height * sizeof(u32));
	}

	if (data && upload)
	{
		Upload();
	}
}

Texture::~Texture()
{
	rnd::IRenderResourceManager::DestroyRenderTextures(this);
}

void Texture::Upload()
{
	if (!mUploaded)
	{
		rnd::IRenderResourceManager::CreateRenderTextures(this);
		mUploaded = true;
	}
}

rnd::DeviceTextureRef Texture::GetDeviceTexture() const
{
	return rnd::IRenderResourceManager::GetMainDeviceTexture(this);
}
