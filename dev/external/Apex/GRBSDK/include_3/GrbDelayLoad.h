#ifndef GRBDELAYLOAD_H
#define GRBDELAYLOAD_H

#include "PxPhysics.h"

#if defined(PX_WINDOWS)

	#define NOMINMAX
	#include <windows.h>
	#include <delayimp.h>

	#if defined GRB_CORE_EXPORTS
		#define GRB_CORE_API __declspec(dllexport)
	#else
		#define GRB_CORE_API __declspec(dllimport)
	#endif

	PX_C_EXPORT GRB_CORE_API void PX_CALL_CONV GrbSetDelayHook(PfnDliHook);
#endif

#endif
