#pragma once
#include <vector>
#include <memory>
#include "maths.h"
#include "Utils.h"

#define IND(x, y) (x + y * m_Width)

class TextureRef;

class Texture : public BracketAccessed<Texture>
{
public:

	friend class std::shared_ptr<Texture>;
	//using size_type = u32;

	bool IsValid() const {
		return m_Width > 0 && m_Height > 0;
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
		return m_Data[IND(static_cast<size_type>(min(x,1.f) * (m_Width - 1)), static_cast<size_type>(min(y,1.f) * (m_Height - 1)))];
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

	Texture(size_type width, size_type height, u8 const* data = nullptr)
	: m_Width(width), m_Height(height), m_Data(width * height)
	{
		if (data != nullptr)
		{
			memcpy(&m_Data[0], data, width * height * sizeof(u32));
		}
	}

	static TextureRef LoadFrom(char const* fileName);


private:

	Texture()
		: Texture(0, 0) {}

	Texture(Texture const& other) = default;

	std::vector<u32> m_Data;
	size_type m_Width;
	size_type m_Height;
};

class TextureRef
{
private:
	std::shared_ptr<const Texture> m_Tex;
public:
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

	template<typename TAcc>
	auto operator[](TAcc t) const -> decltype((*this->m_Tex)[t])
	{
		return (*m_Tex)[t];
	}
	
};
