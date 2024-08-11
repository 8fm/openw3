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
// Copyright (c) 2008-2011 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#ifndef GRBFORCEFIELDLINEARKERNELDESC_H
#define GRBFORCEFIELDLINEARKERNELDESC_H

#include "GrbVector.h"
#include "GrbMatrix33.h"

class GrbForceFieldLinearKernelDesc
{
public:
	
	/**
	\brief Constant part of force field function

	<b>Default</b> zero

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes [SW fallback]
	\li PS3  : Yes
	\li XB360: Yes
	*/
	GrbVector				constant;
	
	/**
	\brief Coefficient of force field function position term

	<b>Default</b> zero

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes [SW fallback]
	\li PS3  : Yes
	\li XB360: Yes
	*/
	GrbMatrix33				positionMultiplier;
	
	/**
	\brief Force field position target.

	<b>Default</b> zero

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes [SW fallback]
	\li PS3  : Yes
	\li XB360: Yes
	*/
	GrbVector				positionTarget;
	
	/**
	\brief Coefficient of force field function velocity term

	<b>Default</b> zero

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes [SW fallback]
	\li PS3  : Yes
	\li XB360: Yes
	*/
	GrbMatrix33				velocityMultiplier;
	
	/**
	\brief  Force field velocity target

	<b>Default</b> zero

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes [SW fallback]
	\li PS3  : Yes
	\li XB360: Yes
	*/
	GrbVector				velocityTarget;
	
	/**
	\brief Radius for NX_FFC_TOROIDAL type coordinates.

	<b>Default</b> zero

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes [SW fallback]
	\li PS3  : Yes
	\li XB360: Yes
	*/
	PxFloat					torusRadius;

	/**
	\brief Linear term in magnitude falloff factor. Range (each component): [0, inf)

	<b>Default</b> zero

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes [SW fallback]
	\li PS3  : Yes
	\li XB360: Yes
	*/
	GrbVector				falloffLinear;
	
	/**
	\brief Quadratic term in magnitude falloff factor. Range (each component): [0, inf)

	<b>Default</b> zero

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes [SW fallback]
	\li PS3  : Yes
	\li XB360: Yes
	*/
	GrbVector				falloffQuadratic;

	/**
	\brief Noise scaling

	<b>Default</b> zero

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes [SW fallback]
	\li PS3  : Yes
	\li XB360: Yes
	*/
	GrbVector				noise;

	/**
	\brief Possible debug name. The string is not copied by the SDK, only the pointer is stored.

	<b>Default</b> NULL

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes [SW fallback]
	\li PS3  : Yes
	\li XB360: Yes
	*/
	const char* name;

	/**
	\brief Will be copied to NxForceField::userData

	<b>Default:</b> NULL

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes [SW fallback]
	\li PS3  : Yes
	\li XB360: Yes

	@see NxForceField.userData
	*/
	void*					userData; 

	/**
	\brief Constructor sets to default.
	*/
	inline GrbForceFieldLinearKernelDesc();

	/**
	\brief (re)sets the structure to the default.	
	*/
	inline void setToDefault();

	/**
	\brief Returns true if the descriptor is valid.

	\return true if the current settings are valid
	*/
	inline bool isValid() const;

};

inline GrbForceFieldLinearKernelDesc::GrbForceFieldLinearKernelDesc()
{
	setToDefault();
}

inline void GrbForceFieldLinearKernelDesc::setToDefault()
{
	constant			.set(0.0f); 
	positionMultiplier	.setZero();
	positionTarget		.set(0.0f);
	velocityMultiplier	.setZero();
	velocityTarget		.set(0.0f);
	falloffLinear		.set(0.0f);
	falloffQuadratic	.set(0.0f);
	noise				.set(0.0f);
	torusRadius			= 1.0f;
	name				= NULL;
}

inline bool GrbForceFieldLinearKernelDesc::isValid() const
{
	if(torusRadius<0.0f)
		return false;
	return true;
}

#endif