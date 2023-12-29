#pragma once
#include <vector>
#include "maths.h"
#include "Utils.h"

#define IND(x, y) (x + y * m_Width)


class Texture : public BracketAccessed<Texture>
{
public:

	//using size_type = u32;

	Texture()
		: Texture(0, 0) {}

	Texture(size_type width, size_type height, u8 const* data = nullptr)
	: m_Width(width), m_Height(height), m_Data(width * height)
	{
		if (data != nullptr)
		{
			memcpy(&m_Data[0], data, width * height * sizeof(u32));
		}
	}

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


private:
	std::vector<u32> m_Data;
	size_type m_Width;
	size_type m_Height;
};
