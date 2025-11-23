#pragma once

#include <string>
#include <iostream>
#include <comdef.h>
#include <wrl.h>
#include "Logging.h"

using Microsoft::WRL::ComPtr;

#define HR_ERR_CHECK(expr) \
	{\
		HRESULT _hr = expr;\
		if (FAILED(_hr))       \
		{                     \
			_com_error err(_hr);\
			LPCTSTR errMsg = err.ErrorMessage();\
			RLOG(LogGlobal, Error,"Error: %S", errMsg);\
			assert(false);\
		}\
	}

#define CHECK_COM_REF(resource)\
	if (resource)\
	{                           \
		resource->AddRef();\
		ZE_ASSERT (resource->Release() == 1);\
	}

