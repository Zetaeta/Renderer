#pragma once
#include "container/Vector.h"

#define DECLARE_BROADCAST_INTERFACE_COMMON(ClassName)\
		static Vector<ClassName*> sInstances;\
	public:\
		ClassName();\
		~ClassName();

#define DECLARE_BROADCAST_INTERFACE_ONEPARAM(ClassName, FuncName, ParamType, ParamName)\
	class ClassName                                                                     \
	{                                                                                   \
		DECLARE_BROADCAST_INTERFACE_COMMON(ClassName)\
		virtual void FuncName(ParamType ParamName) = 0;\
		static void	 Broadcast##FuncName(ParamType ParamName);\
	};

#define DECLARE_BROADCAST_INTERFACE_TWOPARAMS(ClassName, FuncName, Param1Type, Param1Name, Param2Type, Param2Name)\
	class ClassName                                                                     \
	{                                                                                   \
		DECLARE_BROADCAST_INTERFACE_COMMON(ClassName)\
		virtual void FuncName(Param1Type Param1Name, Param2Type Param2Name) = 0;\
		static void	 Broadcast##FuncName(Param1Type Param1Nam, Param2Type Param2Namee);\
	};

#define DEFINE_BROADCAST_INTERFACE_COMMON(ClassName)\
	Vector<ClassName*> ClassName::sInstances;\
	ClassName::ClassName()                                                             \
	{                                                                                  \
		sInstances.push_back(this);                                                         \
	}\
	ClassName::~ClassName()                                                             \
	{                                                                                  \
		Remove(sInstances, this);                                                         \
	}\

#define DEFINE_BROADCAST_INTERFACE_ONEPARAM(ClassName, FuncName, ParamType, ParamName)\
	DEFINE_BROADCAST_INTERFACE_COMMON(ClassName)\
	void			   ClassName::Broadcast##FuncName(ParamType ParamName)\
	{\
		for (auto* instance : sInstances)\
		{\
			instance->FuncName(ParamName);\
		}\
	}

#define DEFINE_BROADCAST_INTERFACE_TWOPARAMS(ClassName, FuncName, Param1Type, Param1Name, Param2Type, Param2Name)\
	DEFINE_BROADCAST_INTERFACE_COMMON(ClassName)\
	void			   ClassName::Broadcast##FuncName(Param1Type Param1Name, Param2Type Param2Name)\
	{\
		for (auto* instance : sInstances)\
		{\
			instance->FuncName(Param1Name, Param2Name);\
		}\
	}

