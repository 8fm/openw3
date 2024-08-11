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

#include "PxPVDRenderDebug.h"
#include "PxProcessRenderDebug.h"
#include "PsUserAllocated.h"

#pragma warning(disable:4100)

namespace physx
{

class PVDRenderDebug : public PxPVDRenderDebug, public shdfnd::UserAllocated
{
public:
	PVDRenderDebug(bool echoLocally)
	{
		mEchoLocally = NULL;
		if (echoLocally)
		{
			mEchoLocally = createProcessRenderDebug();
		}
	}

	virtual ~PVDRenderDebug(void)
	{
		if (mEchoLocally)
		{
			mEchoLocally->release();
		}
	};

	virtual PxProcessRenderDebug* getEchoLocal(void) const
	{
		return mEchoLocally;
	}

	bool isOk(void) const
	{
		return true;
	}

	virtual void processRenderDebug(const DebugPrimitive** dplist,
	                                PxU32 pcount,
	                                RenderDebugInterface* iface,
	                                PxProcessRenderDebug::DisplayType type)
	{
		// At this location, PVD should stream these primitives over the socket connection.
		// All primitives are raw-data types; no pointers, etc.
		// If you need to munge the data to be endian aware, this is the location you would do it.
		// On the other side, PVD should grab this set of debug primitives and capture them relative to the current frame.
		if (mEchoLocally)
		{
			mEchoLocally->processRenderDebug(dplist, pcount, iface, type);
		}
	}


	virtual void flush(RenderDebugInterface* iface, PxProcessRenderDebug::DisplayType type)
	{
		// Indicates that primitives of this data type are completed.
		if (mEchoLocally)
		{
			mEchoLocally->flush(iface, type);
		}
	}

	// releases the PVD render debug interface.
	virtual void release(void)
	{
		delete this;
	}

	virtual void flushFrame(RenderDebugInterface* /*iface*/)
	{
	}

private:
	PxProcessRenderDebug*	mEchoLocally;
};


PxPVDRenderDebug* createPVDRenderDebug(bool echoLocally)
{
	PVDRenderDebug* f = PX_NEW(PVDRenderDebug)(echoLocally);
	if (!f->isOk())
	{
		delete f;
		f = NULL;
	}
	return static_cast< PxPVDRenderDebug*>(f);
}



};
