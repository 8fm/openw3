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


#ifndef EXT_VISUAL_DEBUGGER_H
#define EXT_VISUAL_DEBUGGER_H

#if PX_SUPPORT_VISUAL_DEBUGGER

#include "CmPhysXCommon.h"
#include "PsUserAllocated.h"
#include "PxVisualDebuggerExt.h"
#include "PxJoint.h"
#include "PvdDataStream.h"
#include "PxExtensionMetaDataObjects.h"
#include "PvdTypeNames.h"
#include "PvdObjectModelBaseTypes.h"

namespace physx
{

class PxJoint;
class PxD6Joint;
class PxDistanceJoint;
class PxFixedJoint;
class PxPrismaticJoint;
class PxRevoluteJoint;
class PxSphericalJoint;

#define JOINT_GROUP 3

namespace debugger {
	#define DEFINE_NATIVE_PVD_PHYSX3_TYPE_MAP( type ) DEFINE_PVD_TYPE_NAME_MAP( type, "physx3", #type )

	DEFINE_NATIVE_PVD_PHYSX3_TYPE_MAP(PxJoint);
	DEFINE_NATIVE_PVD_PHYSX3_TYPE_MAP(PxJointGeneratedValues);
	DEFINE_NATIVE_PVD_PHYSX3_TYPE_MAP(PxFixedJoint);
	DEFINE_NATIVE_PVD_PHYSX3_TYPE_MAP(PxFixedJointGeneratedValues);
	DEFINE_NATIVE_PVD_PHYSX3_TYPE_MAP(PxDistanceJoint);
	DEFINE_NATIVE_PVD_PHYSX3_TYPE_MAP(PxDistanceJointGeneratedValues);
	DEFINE_NATIVE_PVD_PHYSX3_TYPE_MAP(PxPrismaticJoint);
	DEFINE_NATIVE_PVD_PHYSX3_TYPE_MAP(PxPrismaticJointGeneratedValues);
	DEFINE_NATIVE_PVD_PHYSX3_TYPE_MAP(PxRevoluteJoint);
	DEFINE_NATIVE_PVD_PHYSX3_TYPE_MAP(PxRevoluteJointGeneratedValues);
	DEFINE_NATIVE_PVD_PHYSX3_TYPE_MAP(PxSphericalJoint);
	DEFINE_NATIVE_PVD_PHYSX3_TYPE_MAP(PxSphericalJointGeneratedValues);
	DEFINE_NATIVE_PVD_PHYSX3_TYPE_MAP(PxD6Joint);
	DEFINE_NATIVE_PVD_PHYSX3_TYPE_MAP(PxD6JointGeneratedValues);

#undef DEFINE_NATIVE_PVD_PHYSX3_TYPE_MAP

}

namespace Ext
{
	class VisualDebugger: public PxVisualDebuggerExt, public Ps::UserAllocated
	{
		VisualDebugger& operator=(const VisualDebugger&);
	public:
		class PvdNameSpace
		{
		
		public:
			PvdNameSpace(physx::debugger::comm::PvdDataStream& conn, const char* name);
			~PvdNameSpace();
		private:
			PvdNameSpace& operator=(const PvdNameSpace&);
			physx::debugger::comm::PvdDataStream& mConnection;
		};

		static void setActors( physx::debugger::comm::PvdDataStream& PvdDataStream, const PxJoint& inJoint, const PxConstraint& c, const PxActor* newActor0, const PxActor* newActor1 );
		
		template<typename TObjType>
		static void createInstance( physx::debugger::comm::PvdDataStream& inStream, const PxConstraint& c, const TObjType& inSource )
		{
			const PxJoint* theJoint = &inSource;
			PxRigidActor* actor0, *actor1;
			c.getActors( actor0, actor1 );
			inStream.createInstance( &inSource );
			inStream.pushBackObjectRef( c.getScene(), "Joints", (const void*)theJoint );
			if ( actor0 && (actor0->getScene() != NULL ) )
				inStream.pushBackObjectRef( (PxActor*)actor0, "Joints", theJoint );
			if ( actor1 && (actor1->getScene() != NULL ))
				inStream.pushBackObjectRef( (PxActor*)actor1, "Joints", theJoint );
			const void* parent = actor0 ? (const void*)actor0 : (const void*) actor1;
			inStream.setPropertyValue( theJoint, "Parent", parent );
		}

		template<typename jointtype, typename structValue>
		static void updatePvdProperties(physx::debugger::comm::PvdDataStream& pvdConnection, const jointtype& joint)
		{
			structValue theValueStruct( &joint );
			pvdConnection.setPropertyMessage( &joint, theValueStruct );
		}
		
		template<typename jointtype>
		static void simUpdate(physx::debugger::comm::PvdDataStream& /*pvdConnection*/, const jointtype& /*joint*/) {}		
		
		template<typename jointtype>
		static void createPvdInstance(physx::debugger::comm::PvdDataStream& pvdConnection, const PxConstraint& c, const jointtype& joint)
		{
			createInstance<jointtype>( pvdConnection, c, joint );		
		}

		static void releasePvdInstance(physx::debugger::comm::PvdDataStream& pvdConnection, const PxConstraint& c, const PxJoint& joint);
		static void sendClassDescriptions(physx::debugger::comm::PvdDataStream& pvdConnection);
	};
}

}

#endif // PX_SUPPORT_VISUAL_DEBUGGER
#endif // EXT_VISUAL_DEBUGGER_H