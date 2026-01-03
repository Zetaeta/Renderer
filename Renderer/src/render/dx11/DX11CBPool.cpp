#include "DX11CBPool.h"

namespace rnd::dx11
{

void DX11CBPool::OnEndFrame()
{
	for (auto& pool : mPools)
	{
		auto size = pool.Size();
		for (u32 i=0; i<size; ++i)
		{
			if (pool.IsInUse(i))
			{
				pool.Release(i);
			}
		}
	}
}

}
