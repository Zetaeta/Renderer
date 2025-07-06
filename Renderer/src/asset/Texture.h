#pragma once
#include <vector>
#include <memory>
#include <core/Maths.h>
#include <core/Utils.h>
#include <render/DeviceTexture.h>
#include "common/CommonEnums.h"

#define IND(x, y) (x + y * width)

class TextureRef;

using TextureId = u64;

class Texture : public BracketAccessed<Texture>
{
public:
	
	ETextureFormat Format = ETextureFormat::RGBA8_Unorm;

	friend class std::shared_ptr<Texture>;
	//using size_type = u32;

	bool IsValid() const {
		return width > 0 && height > 0;
	}

	u32& at(size_type x, size_type y)
	{
		return m_Data[IND(x,y)];
	}

	u32 at(size_type x, size_type y) const
	{
		return m_Data[IND(x,y)];
	}

	u32 at(float x, float y) const
	{
		return m_Data[IND(static_cast<size_type>(min(x,1.f) * (width - 1)), static_cast<size_type>((min(y,1.f)) * (height - 1)))];
	}

	vec4 operator[](vec2 xy) const
	{
		return rgbaToVec(at(xy.x,xy.y));
	}

	static std::shared_ptr<Texture> const EMPTY;

	template<typename... Args>
	static std::shared_ptr<Texture> Create(Args&&... args)
	{
		return std::make_shared <Texture>(std::forward<Args>(args)...);
	}

	Texture(size_type width, size_type height, std::string const& name = "(unnamed)", u8 const* data = nullptr,
			ETextureFormat format = ETextureFormat::RGBA8_Unorm, bool upload = true);
	~Texture();

	void Upload();
	static TextureRef LoadFrom(char const* fileName, bool isSRGB = true);

	u32 const* GetData() const
	{
		return Addr(m_Data);
	}

	u32* GetMutableData()
	{
		return Addr(m_Data);
	}

	size_type const width;
	size_type const height;
	const TextureId Id;

	rnd::DeviceTextureDesc MakeDesc()
	{
		rnd::DeviceTextureDesc desc;
		desc.ArraySize = 1;
		desc.DebugName = m_Name;
		desc.Width = width;
		desc.Height = height;
		desc.Format = Format;
		desc.Flags = rnd::TF_SRV;
		desc.NumMips = 1; //TODO
		return desc;
	}

	#if !MULTI_RENDER_BACKEND
	rnd::DeviceTextureRef const& GetDeviceTexture() const { return m_DeviceTex; }
	#else
	rnd::DeviceTextureRef GetDeviceTexture() const;
	#endif

	char const* GetName() const { return m_Name.c_str(); }

private:

	Texture()
		: Texture(0, 0) {}

	Texture(Texture const& other) = default;

	std::vector<u32> m_Data;
	std::string m_Name;
	bool mUploaded = false;

	#if !MULTI_RENDER_BACKEND
	mutable rnd::DeviceTextureRef m_DeviceTex = INVALID_DEV_TEX;
	#endif
};

class TextureRef
{
private:
	std::shared_ptr<Texture> m_Tex;
public:

	std::shared_ptr<Texture> Get() const
	{
		return m_Tex;
	}

	TextureRef()
		: TextureRef(Texture::EMPTY) {}

	TextureRef(std::shared_ptr<Texture> tex)
		: m_Tex(tex) {}
	//TextureRef(std::shared_ptr<Texture> tex)
	//	: m_Tex(tex) {}

	Texture& operator*() const {
		return *m_Tex;
	}

	Texture* operator->() const {
		return &*m_Tex;
	}

	operator std::shared_ptr<Texture>() const
	{
		return m_Tex;
	}

	void Clear()
	{
		m_Tex = Texture::EMPTY;
	}

	bool IsValid() const
	{
		return m_Tex != nullptr && m_Tex->IsValid();
	}

	template<typename TAcc>
	auto operator[](TAcc t) const -> decltype((*this->m_Tex)[t])
	{
		return (*m_Tex)[t];
	}
	
};
#undef IND
