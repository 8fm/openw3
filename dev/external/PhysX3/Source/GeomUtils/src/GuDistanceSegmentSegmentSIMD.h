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

#ifndef GU_DISTANCE_SEGMENT_SEGMENT_SIMD_H
#define GU_DISTANCE_SEGMENT_SEGMENT_SIMD_H

#include "common/PxPhysXCommonConfig.h"
#include "PsVecMath.h"

namespace physx
{
namespace Gu
{

	PX_PHYSX_COMMON_API Ps::aos::FloatV distanceSegmentSegmentSquared(	const Ps::aos::Vec3VArg p1, const Ps::aos::Vec3VArg d1, const Ps::aos::Vec3VArg p2, const Ps::aos::Vec3VArg d2,
										 								Ps::aos::FloatV& param0, 
										 								Ps::aos::FloatV& param1);
	
     
	///*
	//	This function do four segment segment closest point test in one go
	//*/
	//PX_PHYSX_COMMON_API Ps::aos::Vec4V distanceSegmentSegmentSquared4(  const Ps::aos::Vec3VArg p01, const Ps::aos::Vec3VArg q01, 
	//																	const Ps::aos::Vec3VArg p02, const Ps::aos::Vec3VArg q02, 
	//																	const Ps::aos::Vec3VArg p11, const Ps::aos::Vec3VArg q11,
	//																	const Ps::aos::Vec3VArg p12, const Ps::aos::Vec3VArg q12, 
	//																	const Ps::aos::Vec3VArg p21, const Ps::aos::Vec3VArg q21, 
	//																	const Ps::aos::Vec3VArg p22, const Ps::aos::Vec3VArg q22,
	//																	const Ps::aos::Vec3VArg p31, const Ps::aos::Vec3VArg q31,
	//																	const Ps::aos::Vec3VArg p32, const Ps::aos::Vec3VArg q32,
	//																	Ps::aos::Vec4V& s, Ps::aos::Vec4V& t);


	/*
		This function do four segment segment closest point test in one go
	*/
	PX_PHYSX_COMMON_API Ps::aos::Vec4V distanceSegmentSegmentSquared4(  const Ps::aos::Vec3VArg p, const Ps::aos::Vec3VArg d, 
																		const Ps::aos::Vec3VArg p02, const Ps::aos::Vec3VArg d02, 
																		const Ps::aos::Vec3VArg p12, const Ps::aos::Vec3VArg d12, 
																		const Ps::aos::Vec3VArg p22, const Ps::aos::Vec3VArg d22,
																		const Ps::aos::Vec3VArg p32, const Ps::aos::Vec3VArg d32,
																		Ps::aos::Vec4V& s, Ps::aos::Vec4V& t);

	//Returns sqDistance
	PX_PHYSX_COMMON_API Ps::aos::Vec4V distanceSegmentSegmentSquared4(	const Ps::aos::Vec3VArg p, const Ps::aos::Vec3VArg d0, 
																const Ps::aos::Vec3VArg p02,  
																const Ps::aos::Vec3VArg p12,  
																const Ps::aos::Vec3VArg p22, 
																const Ps::aos::Vec3VArg p32, 
																const Ps::aos::Vec3VArg d1,
																Ps::aos::Vec4V& s, Ps::aos::Vec4V& t,
																Ps::aos::Vec3V* closest);


} // namespace Gu

}

#endif
