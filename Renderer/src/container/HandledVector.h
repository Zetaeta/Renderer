#pragma once
#include "core/Types.h"
#include "core/CoreTypes.h"

#define HANDLED_VECTOR_TODO 0
// TODO free-list with ids 

namespace HandledVec
{

#if HANDLED_VECTOR_TODO
using IndexType = u32;
using IdType = u32;
struct Handle
{
	IndexType Index;
	IdType Id;
};

template<typename T>
struct Element
{
	IdType Id; 
	UninitializedMemory<T> Value;
};

#else
using IdType = u64;
using Handle = IdType;
#endif


}

template<typename T>
class HandledVector : public std::unordered_map<HandledVec::Handle, T>
{
public:
//	using namespace HandledVec;
	using Handle = HandledVec::Handle;
#if HANDLED_VECTOR_TODO
	template<typename... Args>
	Handle Emplace(Args... args)
	{
		
		if (mFirstFree < mData.size())
		{
			
		}
	}
	Vector<HandledVec::Element<T>> mData;
	IndexType mFirstFree = 0;
#else
	template<typename... Args>
	Handle Emplace(Args... args)
	{
		Handle hdl = id++;
		this->emplace(hdl, std::forward<Args>(args)...);
		return hdl;
	}

	bool Remove(Handle hdl)
	{
		return this->erase(hdl) > 0;
	}
	HandledVec::IdType id = 0;

	template<typename Func>
	void ForEach(Func&& func)
	{
		for (auto const& [key, value] : *this)
		{
			func(value);
		}
	}
#endif
};
