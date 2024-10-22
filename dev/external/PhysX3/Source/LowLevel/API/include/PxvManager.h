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


#ifndef PXV_MANAGER_H
#define PXV_MANAGER_H

#include "PxvConfig.h"
#include "PxvGeometry.h"
#include "PxVec3.h"
#include "PxQuat.h"
#include "PxTransform.h"
#include "PxMemory.h"

namespace physx
{

class PxvContact;

/*!
\file
Manager interface
*/

/************************************************************************/
/* Managers                                                             */
/************************************************************************/

class PxsContactManager;
class PxsContext;

struct PxsRigidCore;
struct PxsShapeCore;



/*!
Type of PXD_MANAGER_CCD_MODE property
*/
enum PxvContactManagerCCDMode
{
	PXD_MANAGER_CCD_NONE,
	PXD_MANAGER_CCD_LINEAR,
};


/*!
Manager descriptor
*/
struct PxvManagerDescRigidRigid
{
	/*!
	Manager user data

	\sa PXD_MANAGER_USER_DATA
	*/
	void* userData;

	/*!
	Dominance setting for one way interactions.
	A dominance of 0 means the corresp. body will 
	not be pushable by the other body in the constraint.
	\sa PXD_MANAGER_DOMINANCE0
	*/
	PxReal		dominance0;
	
	/*!
	Dominance setting for one way interactions.
	A dominance of 0 means the corresp. body will 
	not be pushable by the other body in the constraint.
	\sa PXD_MANAGER_DOMINANCE1
	*/
	PxReal 	dominance1;

	/*!
	PxsRigidBodies
	*/
	PxsRigidBody*	rigidBody0;
	PxsRigidBody*	rigidBody1;

	/*!
	Shape Core structures
	*/

	const PxsShapeCore*	shapeCore0;
	const PxsShapeCore*	shapeCore1;

	/*!
	Body Core structures
	*/

	PxsRigidCore*	rigidCore0;
	PxsRigidCore*	rigidCore1;

	/*!
	Enable contact information reporting.

	*/
	int		reportContactInfo;

	/*!
	Enable contact impulse threshold reporting.

	*/
	int		hasForceThreshold;

	/*!
	Enable generated contacts to be changeable

	*/
	int		contactChangeable;

	/*!
	Disable strong friction

	*/
	//int		disableStrongFriction;

	/*!
	Contact resolution rest distance.

	*/
	PxReal		restDistance;

	/*!
	Disable contact response

	*/
	int		disableResponse;

	/*!
	Disable CCD contact response

	*/
	int		disableCCDResponse;

	/*!
	Is connected to an articulation (1 - first body, 2 - second body)

	*/
	int		hasArticulations;

	/*!
	is connected to a dynamic (1 - first body, 2 - second body)
	*/
	int		hasDynamics;

	/*
	Index entries into the transform cache for shape 0
	*/

	PxU32 transformCache0;

	/*
	Index entries into the transform cache for shape 1
	*/

	PxU32 transformCache1;


	PxvManagerDescRigidRigid()
	{
		PxMemSet(this, 0, sizeof(PxvManagerDescRigidRigid));

		dominance0 = 1.0f;
		dominance1 = 1.0f;
	}
};


/*!
Report struct for change in contact manager status
*/
struct PxvContactManagerStatusChange
{
	/*!
	Manager handle
	*/
	PxsContactManager* manager;

	/*!
	Manager userdata
	*/
	void* userData;
};

}

#endif

