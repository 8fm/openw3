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


#include "PxGeometry.h"
#include "PxcContactMethodImpl.h"

namespace physx
{

// PT: those prototypes shouldn't be public. Keep them here.

// Sphere - other
bool PxcContactSphereSphere				(CONTACT_METHOD_ARGS);
bool PxcContactSpherePlane				(CONTACT_METHOD_ARGS);
bool PxcContactSphereCapsule			(CONTACT_METHOD_ARGS);
bool PxcContactSphereBox				(CONTACT_METHOD_ARGS);
bool PxcContactSphereConvex				(CONTACT_METHOD_ARGS);
bool PxcContactSphereMesh				(CONTACT_METHOD_ARGS);
bool PxcContactSphereHeightField		(CONTACT_METHOD_ARGS);
bool PxcContactSphereHeightFieldUnified	(CONTACT_METHOD_ARGS);

// Plane - other
bool PxcContactPlaneCapsule				(CONTACT_METHOD_ARGS);
bool PxcContactPlaneBox					(CONTACT_METHOD_ARGS);
bool PxcContactPlaneConvex				(CONTACT_METHOD_ARGS);

// Capsule - other
bool PxcContactCapsuleCapsule			(CONTACT_METHOD_ARGS);
bool PxcContactCapsuleBox				(CONTACT_METHOD_ARGS);
bool PxcContactCapsuleConvex			(CONTACT_METHOD_ARGS);
bool PxcContactCapsuleMesh				(CONTACT_METHOD_ARGS);
bool PxcContactCapsuleHeightField		(CONTACT_METHOD_ARGS);
bool PxcContactCapsuleHeightFieldUnified(CONTACT_METHOD_ARGS);

// Box - other
bool PxcContactBoxBox					(CONTACT_METHOD_ARGS);
bool PxcContactBoxConvex				(CONTACT_METHOD_ARGS);
bool PxcContactBoxMesh					(CONTACT_METHOD_ARGS);
bool PxcContactBoxHeightField			(CONTACT_METHOD_ARGS);
bool PxcContactBoxHeightFieldUnified	(CONTACT_METHOD_ARGS);

// Convex - other
bool PxcContactConvexConvex				(CONTACT_METHOD_ARGS);
bool PxcContactConvexMesh2				(CONTACT_METHOD_ARGS);
bool PxcContactConvexHeightField		(CONTACT_METHOD_ARGS);
bool PxcContactConvexHeightFieldUnified	(CONTACT_METHOD_ARGS);


static bool PxcInvalidContactPair		(CONTACT_METHOD_ARGS_UNUSED)
{
	return false;
}


//PCM Sphere - other
bool PxcPCMContactSphereSphere			(CONTACT_METHOD_ARGS);
bool PxcPCMContactSphereBox				(CONTACT_METHOD_ARGS);
bool PxcPCMContactSphereCapsule			(CONTACT_METHOD_ARGS);
bool PxcPCMContactSphereConvex			(CONTACT_METHOD_ARGS);
bool PxcPCMContactSphereMesh			(CONTACT_METHOD_ARGS);
bool PxcPCMContactSphereHeightField		(CONTACT_METHOD_ARGS);

//PCM Box - other
bool PxcPCMContactBoxBox				(CONTACT_METHOD_ARGS);
bool PxcPCMContactBoxConvex				(CONTACT_METHOD_ARGS);
bool PxcPCMContactBoxMesh				(CONTACT_METHOD_ARGS);
bool PxcPCMContactBoxHeightField		(CONTACT_METHOD_ARGS);

//PCM Capsule - other
bool PxcPCMContactCapsuleCapsule		(CONTACT_METHOD_ARGS);
bool PxcPCMContactCapsuleBox			(CONTACT_METHOD_ARGS);
bool PxcPCMContactCapsuleConvex			(CONTACT_METHOD_ARGS);
bool PxcPCMContactCapsuleMesh			(CONTACT_METHOD_ARGS);
bool PxcPCMContactCapsuleHeightField	(CONTACT_METHOD_ARGS);

//PCM Convex
bool PxcPCMContactConvexConvex			(CONTACT_METHOD_ARGS);
bool PxcPCMContactConvexMesh			(CONTACT_METHOD_ARGS);
bool PxcPCMContactConvexHeightField		(CONTACT_METHOD_ARGS);

// First impact sweeps
PxReal PxcSweepCapsuleCapsule			(SWEEP_METHOD_ARGS);
PxReal PxcSweepSphereSphere				(SWEEP_METHOD_ARGS);
PxReal PxcSweepSphereBox				(SWEEP_METHOD_ARGS);
PxReal PxcSweepCapsuleBox				(SWEEP_METHOD_ARGS);
PxReal PxcSweepCapsuleConvex			(SWEEP_METHOD_ARGS);
PxReal PxcSweepBoxBox					(SWEEP_METHOD_ARGS);
PxReal PxcSweepBoxConvex				(SWEEP_METHOD_ARGS);
PxReal PxcSweepConvexConvex				(SWEEP_METHOD_ARGS);
PxReal PxcSweepConvexMesh				(SWEEP_METHOD_ARGS);
PxReal PxcSweepConvexHeightfield		(SWEEP_METHOD_ARGS);

#define DYNAMIC_CONTACT_REGISTRATION(x) PxcInvalidContactPair
//#define DYNAMIC_CONTACT_REGISTRATION(x) x

//Table of contact methods for different shape-type combinations
PxcContactMethod g_ContactMethodTable[][7] = 
{
	//PxGeometryType::eSPHERE
	{
		PxcContactSphereSphere,			//PxGeometryType::eSPHERE
		PxcContactSpherePlane,			//PxGeometryType::ePLANE
		PxcContactSphereCapsule,		//PxGeometryType::eCAPSULE
		PxcContactSphereBox,			//PxGeometryType::eBOX
		PxcContactSphereConvex,			//PxGeometryType::eCONVEXMESH
		PxcContactSphereMesh,			//PxGeometryType::eTRIANGLEMESH
		DYNAMIC_CONTACT_REGISTRATION(PxcContactSphereHeightFieldUnified),	//PxGeometryType::eHEIGHTFIELD	//TODO: make HF midphase that will mask this
		
	},

	//PxGeometryType::ePLANE
	{
		0,								//PxGeometryType::eSPHERE
		PxcInvalidContactPair,			//PxGeometryType::ePLANE
		PxcContactPlaneCapsule,			//PxGeometryType::eCAPSULE
		PxcContactPlaneBox,				//PxGeometryType::eBOX
		PxcContactPlaneConvex,			//PxGeometryType::eCONVEXMESH
		PxcInvalidContactPair,			//PxGeometryType::eTRIANGLEMESH
		PxcInvalidContactPair,			//PxGeometryType::eHEIGHTFIELD
	},

	//PxGeometryType::eCAPSULE
	{
		0,								//PxGeometryType::eSPHERE
		0,								//PxGeometryType::ePLANE
		PxcContactCapsuleCapsule,		//PxGeometryType::eCAPSULE
		PxcContactCapsuleBox,			//PxGeometryType::eBOX
		PxcContactCapsuleConvex,		//PxGeometryType::eCONVEXMESH
		PxcContactCapsuleMesh,			//PxGeometryType::eTRIANGLEMESH
		DYNAMIC_CONTACT_REGISTRATION(PxcContactCapsuleHeightFieldUnified),	//PxGeometryType::eHEIGHTFIELD		//TODO: make HF midphase that will mask this
	},

	//PxGeometryType::eBOX
	{
		0,								//PxGeometryType::eSPHERE
		0,								//PxGeometryType::ePLANE
		0,								//PxGeometryType::eCAPSULE
		PxcContactBoxBox,				//PxGeometryType::eBOX
		PxcContactBoxConvex,			//PxGeometryType::eCONVEXMESH
		PxcContactBoxMesh,				//PxGeometryType::eTRIANGLEMESH
		DYNAMIC_CONTACT_REGISTRATION(PxcContactBoxHeightFieldUnified),		//PxGeometryType::eHEIGHTFIELD		//TODO: make HF midphase that will mask this
	},

	//PxGeometryType::eCONVEXMESH
	{
		0,								//PxGeometryType::eSPHERE
		0,								//PxGeometryType::ePLANE
		0,								//PxGeometryType::eCAPSULE
		0,								//PxGeometryType::eBOX
		PxcContactConvexConvex,			//PxGeometryType::eCONVEXMESH
		PxcContactConvexMesh2,			//PxGeometryType::eTRIANGLEMESH
		DYNAMIC_CONTACT_REGISTRATION(PxcContactConvexHeightFieldUnified),	//PxGeometryType::eHEIGHTFIELD		//TODO: make HF midphase that will mask this
	},

	//PxGeometryType::eTRIANGLEMESH
	{
		0,								//PxGeometryType::eSPHERE
		0,								//PxGeometryType::ePLANE
		0,								//PxGeometryType::eCAPSULE
		0,								//PxGeometryType::eBOX
		0,								//PxGeometryType::eCONVEXMESH
		PxcInvalidContactPair,			//PxGeometryType::eTRIANGLEMESH
		PxcInvalidContactPair,			//PxGeometryType::eHEIGHTFIELD
	},

	//PxGeometryType::eHEIGHTFIELD
	{
		0,								//PxGeometryType::eSPHERE
		0,								//PxGeometryType::ePLANE
		0,								//PxGeometryType::eCAPSULE
		0,								//PxGeometryType::eBOX
		0,								//PxGeometryType::eCONVEXMESH
		0,								//PxGeometryType::eTRIANGLEMESH
		PxcInvalidContactPair,			//PxGeometryType::eHEIGHTFIELD
	},
};


//#if	PERSISTENT_CONTACT_MANIFOLD
//Table of contact methods for different shape-type combinations
PxcContactMethod g_PCMContactMethodTable[][7] = 
{
	//PxGeometryType::eSPHERE
	{
		PxcPCMContactSphereSphere,										//PxGeometryType::eSPHERE
		PxcContactSpherePlane,											//PxGeometryType::ePLANE
		PxcPCMContactSphereCapsule,										//PxGeometryType::eCAPSULE
		PxcPCMContactSphereBox,											//PxGeometryType::eBOX
		PxcPCMContactSphereConvex,										//PxGeometryType::eCONVEXMESH
		PxcPCMContactSphereMesh,										//PxGeometryType::eTRIANGLEMESH
		DYNAMIC_CONTACT_REGISTRATION(PxcPCMContactSphereHeightField),	//PxGeometryType::eHEIGHTFIELD	//TODO: make HF midphase that will mask this	
	},

	//PxGeometryType::ePLANE
	{
		0,															//PxGeometryType::eSPHERE
		PxcInvalidContactPair,										//PxGeometryType::ePLANE
		PxcContactPlaneCapsule,										//PxGeometryType::eCAPSULE
		PxcContactPlaneBox,											//PxGeometryType::eBOX
		PxcContactPlaneConvex,										//PxGeometryType::eCONVEXMESH
		PxcInvalidContactPair,										//PxGeometryType::eTRIANGLEMESH
		PxcInvalidContactPair,										//PxGeometryType::eHEIGHTFIELD
	},

	//PxGeometryType::eCAPSULE
	{
		0,																//PxGeometryType::eSPHERE
		0,																//PxGeometryType::ePLANE
		PxcPCMContactCapsuleCapsule,									//PxGeometryType::eCAPSULE
		PxcPCMContactCapsuleBox,										//PxGeometryType::eBOX
		PxcPCMContactCapsuleConvex,										//PxGeometryType::eCONVEXMESH
		PxcPCMContactCapsuleMesh,										//PxGeometryType::eTRIANGLEMESH	
		DYNAMIC_CONTACT_REGISTRATION(PxcPCMContactCapsuleHeightField),	//PxGeometryType::eHEIGHTFIELD		//TODO: make HF midphase that will mask this
	},

	//PxGeometryType::eBOX
	{
		0,																//PxGeometryType::eSPHERE
		0,																//PxGeometryType::ePLANE
		0,																//PxGeometryType::eCAPSULE
		PxcPCMContactBoxBox,											//PxGeometryType::eBOX
		PxcPCMContactBoxConvex,											//PxGeometryType::eCONVEXMESH
		PxcPCMContactBoxMesh,											//PxGeometryType::eTRIANGLEMESH
		DYNAMIC_CONTACT_REGISTRATION(PxcPCMContactBoxHeightField),		//PxGeometryType::eHEIGHTFIELD		//TODO: make HF midphase that will mask this

	},

	//PxGeometryType::eCONVEXMESH
	{
		0,																	//PxGeometryType::eSPHERE
		0,																	//PxGeometryType::ePLANE
		0,																	//PxGeometryType::eCAPSULE
		0,																	//PxGeometryType::eBOX
		PxcPCMContactConvexConvex,											//PxGeometryType::eCONVEXMESH
		PxcPCMContactConvexMesh,											//PxGeometryType::eTRIANGLEMESH
		DYNAMIC_CONTACT_REGISTRATION(PxcPCMContactConvexHeightField),		//PxGeometryType::eHEIGHTFIELD		//TODO: make HF midphase that will mask this
	},

	//PxGeometryType::eTRIANGLEMESH
	{
		0,																//PxGeometryType::eSPHERE
		0,																//PxGeometryType::ePLANE
		0,																//PxGeometryType::eCAPSULE
		0,																//PxGeometryType::eBOX
		0,																//PxGeometryType::eCONVEXMESH
		PxcInvalidContactPair,											//PxGeometryType::eTRIANGLEMESH
		PxcInvalidContactPair,											//PxGeometryType::eHEIGHTFIELD
	},   

	//PxGeometryType::eHEIGHTFIELD
	{
		0,																//PxGeometryType::eSPHERE
		0,																//PxGeometryType::ePLANE
		0,																//PxGeometryType::eCAPSULE
		0,																//PxGeometryType::eBOX
		0,																//PxGeometryType::eCONVEXMESH
		0,																//PxGeometryType::eTRIANGLEMESH
		PxcInvalidContactPair,											//PxGeometryType::eHEIGHTFIELD
	},

};


bool PxcInvalidSweptContactPair (SWEEP_METHOD_ARGS_UNUSED)
{
	return false;
}


PxReal PxcUnimplementedSweep (SWEEP_METHOD_ARGS_UNUSED)
{
	return PX_MAX_REAL;	//no impact
}


const PxcSweepMethod g_SweepMethodTable[][7] = 
{
	//PxGeometryType::eSPHERE
	{
		PxcSweepSphereSphere,			//PxGeometryType::eSPHERE
		PxcUnimplementedSweep,			//PxGeometryType::ePLANE
		PxcSweepCapsuleCapsule,			//PxGeometryType::eCAPSULE
		PxcSweepSphereBox,				//PxGeometryType::eBOX
		PxcSweepCapsuleConvex,			//PxGeometryType::eCONVEXMESH
		PxcSweepConvexMesh,				//PxGeometryType::eTRIANGLEMESH
		PxcSweepConvexHeightfield,		//PxGeometryType::eHEIGHTFIELD		//TODO		
	},

	//PxGeometryType::ePLANE
	{
		0,								//PxGeometryType::eSPHERE
		PxcUnimplementedSweep,			//PxGeometryType::ePLANE
		PxcUnimplementedSweep,			//PxGeometryType::eCAPSULE
		PxcUnimplementedSweep,			//PxGeometryType::eBOX
		PxcUnimplementedSweep,			//PxGeometryType::eCONVEXMESH
		PxcUnimplementedSweep,			//PxGeometryType::eTRIANGLEMESH
		PxcUnimplementedSweep,			//PxGeometryType::eHEIGHTFIELD
	},

	//PxGeometryType::eCAPSULE
	{
		0,								//PxGeometryType::eSPHERE
		0,								//PxGeometryType::ePLANE
		PxcSweepCapsuleCapsule,			//PxGeometryType::eCAPSULE
		PxcSweepCapsuleBox,				//PxGeometryType::eBOX
		PxcSweepCapsuleConvex,			//PxGeometryType::eCONVEXMESH
		PxcSweepConvexMesh,				//PxGeometryType::eTRIANGLEMESH
		PxcSweepConvexHeightfield,		//PxGeometryType::eHEIGHTFIELD
	},

	//PxGeometryType::eBOX
	{
		0,								//PxGeometryType::eSPHERE
		0,								//PxGeometryType::ePLANE
		0,								//PxGeometryType::eCAPSULE
		PxcSweepBoxBox,					//PxGeometryType::eBOX
		PxcSweepBoxConvex,				//PxGeometryType::eCONVEXMESH
		PxcSweepConvexMesh,				//PxGeometryType::eTRIANGLEMESH
		PxcSweepConvexHeightfield,		//PxGeometryType::eHEIGHTFIELD
	},

	//PxGeometryType::eCONVEXMESH
	{
		0,								//PxGeometryType::eSPHERE
		0,								//PxGeometryType::ePLANE
		0,								//PxGeometryType::eCAPSULE
		0,								//PxGeometryType::eBOX
		PxcSweepConvexConvex,			//PxGeometryType::eCONVEXMESH
		PxcSweepConvexMesh,				//PxGeometryType::eTRIANGLEMESH
		PxcSweepConvexHeightfield,		//PxGeometryType::eHEIGHTFIELD
	},

	//PxGeometryType::eTRIANGLEMESH
	{
		0,								//PxGeometryType::eSPHERE
		0,								//PxGeometryType::ePLANE
		0,								//PxGeometryType::eCAPSULE
		0,								//PxGeometryType::eBOX
		0,								//PxGeometryType::eCONVEXMESH
		PxcUnimplementedSweep,			//PxGeometryType::eTRIANGLEMESH
		PxcUnimplementedSweep,			//PxGeometryType::eHEIGHTFIELD
	},

	//PxGeometryType::eHEIGHTFIELD
	{
		0,								//PxGeometryType::eSPHERE
		0,								//PxGeometryType::ePLANE
		0,								//PxGeometryType::eCAPSULE
		0,								//PxGeometryType::eBOX
		0,								//PxGeometryType::eCONVEXMESH
		0,								//PxGeometryType::eTRIANGLEMESH
		PxcUnimplementedSweep,			//PxGeometryType::eHEIGHTFIELD
	},
};

}
