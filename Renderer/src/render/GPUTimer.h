#pragma once

#ifndef PROFILING
#define PROFILING 1
#endif

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
