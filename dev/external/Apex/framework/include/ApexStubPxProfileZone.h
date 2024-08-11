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

#ifndef APEX_STUB_PX_PROFILE_ZONE_H
#define APEX_STUB_PX_PROFILE_ZONE_H

#include "Px.h"
#include "PxProfileZone.h"
#include "PsUserAllocated.h"

namespace physx
{
	class PxUserCustomProfiler;

namespace apex
{

// This class provides a stub implementation of PhysX's PxProfileZone.
// It would be nice to not be forced to do this, but our scoped profile event macros
// cannot have an if(gProfileZone) because it would ruin the scope.  So here we just
// create a stub that will be called so that the user need not create a PxProfileZoneManager
// in debug mode (and suffer an assertion).

class ApexStubPxProfileZone : public physx::PxProfileZone, public physx::UserAllocated
{
public:	

	// physx::PxProfileZone methods
	virtual const char* getName() { return 0; }
	virtual void release() { PX_DELETE(this); }

	virtual void setProfileZoneManager(physx::PxProfileZoneManager* ) {}
	virtual physx::PxProfileZoneManager* getProfileZoneManager() { return 0; }

	virtual PxU16 getEventIdForName( const char*  ) { return 0; }

	virtual PxU16 getEventIdsForNames( const char** , PxU32  ) { return 0; }
	virtual void setUserCustomProfiler(PxUserCustomProfiler* ) {};

	// physx::PxProfileEventBufferClientManager methods
	virtual void addClient( PxProfileZoneClient&  ) {}
	virtual void removeClient( PxProfileZoneClient&  ) {}
	virtual bool hasClients() const { return false; }

	// physx::PxProfileNameProvider methods
	virtual physx::PxProfileNames getProfileNames() const { return PxProfileNames(); }

	// physx::PxProfileEventSender methods
	virtual void startEvent( PxU16 , PxU64 ) {}
	virtual void stopEvent( PxU16 , PxU64 ) {}

	virtual void startEvent( PxU16 , PxU64 , PxU32 ) {}
	virtual void stopEvent( PxU16 , PxU64 , PxU32  ) {}
	virtual void eventValue( PxU16 , PxU64 , PxI64  ) {}

	virtual void CUDAProfileBuffer( PxF32 , const PxU8* , PxU32 , PxU32  ) {}

	// physx::PxProfileEventFlusher methods
	virtual void flushProfileEvents() {}
};

}
} // end namespace physx::apex

#endif // APEX_STUB_PX_PROFILE_ZONE_H
