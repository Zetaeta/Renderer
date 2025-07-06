#pragma once
#include "core/RefCounted.h"
#include "RndFwd.h"

#if PROFILING
namespace rnd
{

class GPUTimer : public RefCountedObject
{
public:
	float GPUTimeMs = 0;
	std::wstring Name;
};

}
#endif
