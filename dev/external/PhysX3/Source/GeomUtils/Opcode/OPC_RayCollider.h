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

#include "PxSimpleTypes.h"
#include "PxQueryReport.h"
#include "PxVec3.h"
#include "OPC_MeshInterface.h"
#include "OPC_ModelData.h"

namespace physx
{
	
namespace Cm
{
class Matrix34;
}

namespace Gu {

class MeshInterface;
struct RTreeMidphaseData;

struct CallbackMode { enum Enum { eANY, eCLOSEST, eMULTIPLE }; };

template<typename HitType>
struct MeshHitCallback
{
	CallbackMode::Enum mode;

	MeshHitCallback(CallbackMode::Enum aMode) : mode(aMode) {}

	bool inAnyMode() const { return mode == CallbackMode::eANY; }
	bool inClosestMode() const { return mode == CallbackMode::eCLOSEST; }
	bool inMultipleMode() const { return mode == CallbackMode::eMULTIPLE; }

	virtual PxAgain processHit( // all reported coords are in mesh local space including hit.position
		const HitType& hit, const PxVec3& v0, const PxVec3& v1, const PxVec3& v2, bool indicesAre16bit, void* indices,
		PxReal& shrunkMaxT) = 0;

	virtual ~MeshHitCallback() {}
};

template <int tInflate>
struct MeshRayCollider
{
	static void Collide(
		const PxVec3& orig, const PxVec3& dir, PxReal maxT, bool bothTriangleSidesCollide,
		const RTreeMidphaseData& model, MeshHitCallback<PxRaycastHit>& callback, PxReal geomEpsilon, // AP todo: eliminate geomEpsilon
		const Cm::Matrix34* world=NULL, const PxVec3* inflate = NULL);
};

} } // namespace physx::Ice