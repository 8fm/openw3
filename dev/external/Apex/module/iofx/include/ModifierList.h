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

#ifndef _MODIFIER
#define _MODIFIER(name)
#endif

#ifndef _MODIFIER_SPRITE
#define _MODIFIER_SPRITE(name)
#endif

#ifndef _MODIFIER_MESH
#define _MODIFIER_MESH(name)
#endif

//All
_MODIFIER(Rotation)
_MODIFIER(SimpleScale)
_MODIFIER(RandomScale)
_MODIFIER(ScaleByMass)
_MODIFIER(ColorVsLife)
_MODIFIER(ColorVsDensity)
_MODIFIER(SubtextureVsLife)
_MODIFIER(OrientAlongVelocity)
_MODIFIER(ScaleAlongVelocity)
_MODIFIER(RandomSubtexture)
_MODIFIER(RandomRotation)
_MODIFIER(ScaleVsLife)
_MODIFIER(ScaleVsDensity)
_MODIFIER(ScaleVsCameraDistance)
_MODIFIER(ViewDirectionSorting)
_MODIFIER(RotationRate)
_MODIFIER(RotationRateVsLife)
_MODIFIER(OrientScaleAlongScreenVelocity)
_MODIFIER(ColorVsVelocity)

//Sprite
_MODIFIER_SPRITE(SimpleScale)
_MODIFIER_SPRITE(RandomScale)
_MODIFIER_SPRITE(ScaleByMass)
_MODIFIER_SPRITE(ColorVsLife)
_MODIFIER_SPRITE(ColorVsDensity)
_MODIFIER_SPRITE(SubtextureVsLife)
_MODIFIER_SPRITE(RandomSubtexture)
_MODIFIER_SPRITE(RandomRotation)
_MODIFIER_SPRITE(ScaleVsLife)
_MODIFIER_SPRITE(ScaleVsDensity)
_MODIFIER_SPRITE(ScaleVsCameraDistance)
_MODIFIER_SPRITE(ViewDirectionSorting)
_MODIFIER_SPRITE(RotationRate)
_MODIFIER_SPRITE(RotationRateVsLife)
_MODIFIER_SPRITE(OrientScaleAlongScreenVelocity)
_MODIFIER_SPRITE(ColorVsVelocity)

//Mesh
_MODIFIER_MESH(Rotation)
_MODIFIER_MESH(SimpleScale)
_MODIFIER_MESH(RandomScale)
_MODIFIER_MESH(ScaleByMass)
_MODIFIER_MESH(ColorVsLife)
_MODIFIER_MESH(ColorVsDensity)
_MODIFIER_MESH(OrientAlongVelocity)
_MODIFIER_MESH(ScaleAlongVelocity)
_MODIFIER_MESH(ScaleVsLife)
_MODIFIER_MESH(ScaleVsDensity)
_MODIFIER_MESH(ScaleVsCameraDistance)
_MODIFIER_MESH(ColorVsVelocity)

#undef _MODIFIER
#undef _MODIFIER_SPRITE
#undef _MODIFIER_MESH
