#pragma once
#include "render/GPUTimer.h"
#include <array>
#include "SharedD3D11.h"

namespace rnd
{
namespace dx11
{

class DX11Timer : public GPUTimer
{
public:

	struct StartEndQuery
	{
		ComPtr<ID3D11Query> Start;
		ComPtr<ID3D11Query> End;
	};

	constexpr static u32 BufferSize = 3;
	std::array<StartEndQuery, BufferSize> Queries;

};

}
}
