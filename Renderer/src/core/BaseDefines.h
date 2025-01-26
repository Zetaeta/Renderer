#pragma once
#ifndef NOMINMAX
#define NOMINMAX 1
#endif

#ifndef _HAS_STD_BYTE
#define _HAS_STD_BYTE 0
#endif

#ifndef MULTI_RENDER_BACKEND
#define MULTI_RENDER_BACKEND 1
#endif


#define ZE_BUILD_EDITOR 1

#ifndef HIT_TESTING
#define HIT_TESTING ZE_BUILD_EDITOR
#endif
#ifndef OBJECT_SELECTION
#define OBJECT_SELECTION ZE_BUILD_EDITOR
#endif


#ifdef _DEBUG
#define ZE_DEBUG_BUILD 1
#else
#define ZE_DEBUG_BUILD 0
#endif

#define GARBAGE_VALUE 0xcc
