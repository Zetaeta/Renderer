#pragma once
#include <vector>
#include <memory>
#include <core/Maths.h>
#include <core/Utils.h>
#include <render/DeviceTexture.h>

#define IND(x, y) (x + y * width)

class TextureRef;

class Texture : public BracketAccessed<Texture>
{
public:

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

	Texture(size_type width, size_type height, std::string const& name = "(unnamed)", u8 const* data = nullptr)
	: width(width), height(height), m_Data(width * height)
	{
		if (data != nullptr)
		{
			memcpy(&m_Data[0], data, width * height * sizeof(u32));
		}
	}

	static TextureRef LoadFrom(char const* fileName);

	u32 const* GetData() const
	{
		return Addr(m_Data);
	}

	size_type const width;
	size_type const height;

	DeviceTextureRef const& GetDeviceTexture() const { return m_DeviceTex; }
	void SetDeviceTexture(DeviceTextureRef const& dt) const { m_DeviceTex = dt; }

	char const* GetName() const { return m_Name.c_str(); }

private:

	Texture()
		: Texture(0, 0) {}

	Texture(Texture const& other) = default;

	std::vector<u32> m_Data;
	std::string m_Name;
	mutable DeviceTextureRef m_DeviceTex = INVALID_DEV_TEX;
};

class TextureRef
{
private:
	std::shared_ptr<const Texture> m_Tex;
public:

	std::shared_ptr<const Texture> Get() const
	{
		return m_Tex;
	}

	TextureRef()
		: TextureRef(Texture::EMPTY) {}

	TextureRef(std::shared_ptr<Texture> tex)
		: m_Tex(tex) {}
	TextureRef(std::shared_ptr<const Texture> tex)
		: m_Tex(tex) {}

	Texture const& operator*() const {
		return *m_Tex;
	}

	Texture const* operator->() const {
		return &*m_Tex;
	}

	void Clear()
	{
		m_Tex = Texture::EMPTY;
	}

	bool IsValid()
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
