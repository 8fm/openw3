// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.
#ifndef PX_PROFILER_CALLBACK_H
#define PX_PROFILER_CALLBACK_H

#ifdef PHYSX_PROFILE_SDK

#include "PxProfileZone.h"
#include "PxProfileZoneManager.h"
#include "PxProfileEventSystem.h"
#include "PxProfileEventHandler.h"
#include "PxProfileEventNames.h"
#include "PxProfileScopedEvent.h"
#include "PxProfileCompileTimeEventFilter.h"
#include "PxProfileScopedEvent.h"
#include "PVDBinding.h"
//#include "PvdConnectionType.h"

extern physx::PxProfileZone *gProfileZone;

class PxProfilerCallbackProfileEvent
{
public:
	PxProfilerCallbackProfileEvent(physx::PxU16 eventId,physx::PxU64 context)
	{
		mEventId = eventId;
		mContext = context;
		gProfileZone->startEvent(mEventId,mContext);
	}


	~PxProfilerCallbackProfileEvent(void)
	{
		gProfileZone->stopEvent(mEventId,mContext);
	}

private:
	physx::PxU16			mEventId;
	physx::PxU64			mContext;
};

// PH: 3.2 doesn't have a profiler in release mode (d'oh)
#if defined(PHYSX_PROFILE_SDK)

#define PX_PROFILER_PERF_SCOPE(name) PX_ASSERT(gProfileZone); static physx::PxU16 eventId = gProfileZone->getEventIdForName(name);  PxProfilerCallbackProfileEvent _profile(eventId,0);
#define PX_PROFILER_PERF_DSCOPE(name,data) PX_ASSERT(gProfileZone); static physx::PxU16 eventId = gProfileZone->getEventIdForName(name);  PxProfilerCallbackProfileEvent _profile(eventId,0); gProfileZone->eventValue(eventId,0,data);

#define PX_PROFILER_START_EVENT(x,y) PX_ASSERT(gProfileZone); gProfileZone->startEvent(x,y)
#define PX_PROFILER_STOP_EVENT(x,y) PX_ASSERT(gProfileZone); gProfileZone->stopEvent(x,y)

#define PX_PROFILER_PLOT(x,y) { PX_ASSERT(gProfileZone); static physx::PxU16 eventId = gProfileZone->getEventIdForName(y); gProfileZone->eventValue(eventId,0,x); }

#else // PHYSX_PROFILE_SDK

#define PX_PROFILER_PERF_SCOPE(name)
#define PX_PROFILER_PERF_DSCOPE(name,data)

#define PX_PROFILER_START_EVENT(x,y)
#define PX_PROFILER_STOP_EVENT(x,y)

#define PX_PROFILER_PLOT(x,y)

#endif // PHYSX_PROFILE_SDK

#else

#define PX_PROFILER_PERF_SCOPE(name)
#define PX_PROFILER_PERF_DSCOPE(name,data)

#define PX_PROFILER_START_EVENT(x,y)
#define PX_PROFILER_STOP_EVENT(x,y)

#define PX_PROFILER_PLOT(x,y)

#endif

#endif // PX_FOUNDATION_PX_PROFILER_CALLBACK_H
