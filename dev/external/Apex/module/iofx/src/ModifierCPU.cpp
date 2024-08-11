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

#include "Modifier.h"

#pragma warning (disable : 4127)

#include "ModifierData.h"

#define MODIFIER_DECL
#define CURVE_TYPE const physx::apex::NxCurve*
#define EVAL_CURVE(curve, value) curve->evaluate(value)
#define PARAMS_NAME(name) name ## ParamsCPU

#include "ModifierSrc.h"

#undef MODIFIER_DECL
#undef CURVE_TYPE
#undef EVAL_CURVE
#undef PARAMS_NAME

namespace physx
{
namespace apex
{
namespace iofx
{

#define _MODIFIER(name) \
	void name ## Modifier :: mapParamsCPU(ModifierParamsMapperCPU& mapper) const \
	{ \
		mapParams(mapper, (name ## ParamsCPU *)NULL); \
	} \
	 
#define _MODIFIER_SPRITE(name) \
	void updateSprite_##name (const void* params, const SpriteInput& input, SpritePublicState& pubState, SpritePrivateState& privState, const ModifierCommonParams& common, RandState& randState) \
	{ \
		modifier##name <false, ModifierUsage_Sprite> (*static_cast<const name##ParamsCPU *>(params), input, pubState, privState, common, randState); \
	} \
	void updateSpriteOnSpawn_##name (const void* params, const SpriteInput& input, SpritePublicState& pubState, SpritePrivateState& privState, const ModifierCommonParams& common, RandState& randState) \
	{ \
		modifier##name <true, ModifierUsage_Sprite> (*static_cast<const name##ParamsCPU *>(params), input, pubState, privState, common, randState); \
	} \
	Modifier::updateSpriteFunc name##Modifier :: getUpdateSpriteFunc(ModifierStage stage) const \
	{ \
		switch (stage) { \
		case ModifierStage_Spawn: return &updateSpriteOnSpawn_##name; \
		case ModifierStage_Continuous: return &updateSprite_##name; \
		default: \
			PX_ALWAYS_ASSERT(); \
			return 0; \
		} \
	}

#define _MODIFIER_MESH(name) \
	void updateMesh_##name (const void* params, const MeshInput& input, MeshPublicState& pubState, MeshPrivateState& privState, const ModifierCommonParams& common, RandState& randState) \
	{ \
		modifier##name <false, ModifierUsage_Mesh> (*static_cast<const name##ParamsCPU *>(params), input, pubState, privState, common, randState); \
	} \
	void updateMeshOnSpawn_##name (const void* params, const MeshInput& input, MeshPublicState& pubState, MeshPrivateState& privState, const ModifierCommonParams& common, RandState& randState) \
	{ \
		modifier##name <true, ModifierUsage_Mesh> (*static_cast<const name##ParamsCPU *>(params), input, pubState, privState, common, randState); \
	} \
	Modifier::updateMeshFunc name##Modifier :: getUpdateMeshFunc(ModifierStage stage) const \
	{ \
		switch (stage) { \
		case ModifierStage_Spawn: return &updateMeshOnSpawn_##name; \
		case ModifierStage_Continuous: return &updateMesh_##name; \
		default: \
			PX_ALWAYS_ASSERT(); \
			return 0; \
		} \
	}

#include "ModifierList.h"

}
}
} // namespace physx::apex
