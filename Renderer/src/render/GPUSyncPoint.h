#pragma once
#include "core/RefCounted.h"
namespace rnd
{
class GPUSyncPoint : public SelfReleasingRefCounted
{
public:
	constexpr static u32 Infinite = (u32) -1;
	virtual bool Wait(u32 forMS = Infinite) = 0;
};

}
