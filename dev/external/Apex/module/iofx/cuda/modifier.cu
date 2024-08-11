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


#include "common.cuh"

#include "../public/NxModifierDefs.h"

#include "include/common.h"
using namespace physx::apex;
using namespace physx::apex::iofx;
#include "include/modifier.h"


// Modifiers

#define MODIFIER_DECL __device__
#define CURVE_TYPE Curve
#define EVAL_CURVE(curve, value) curve.evaluate(KERNEL_CONST_MEM(modifierConstMem), value)
#define PARAMS_NAME(name) name ## ParamsGPU

#include "../include/ModifierSrc.h"

#undef MODIFIER_DECL
#undef CURVE_TYPE
#undef EVAL_CURVE
#undef PARAMS_NAME


#define MODIFIER_LIST_BEG(usage) \
template <bool spawn, typename Input, typename PublicState, typename PrivateState> \
__device__ void runModifiers(Usage2Type< usage > , const ModifierList& list, const ModifierCommonParams& commonParams, const Input& input, PublicState& pubState, PrivateState& privState, physx::RandState& randState) \
{ \
	typedef Usage2Type< usage > UsageType; \
	const ModifierListElem* listElems = list.getElems( KERNEL_CONST_MEM(modifierConstMem) ); \
	const physx::PxU32 listCount = list.getSize(); \
	for (unsigned int i = 0; i < listCount; ++i) \
	{ \
		unsigned int type = listElems[i].type; \

#define MODIFIER_LIST_ELEM(name) \
		if (type == physx::apex::ModifierType_##name) \
		{ \
			const name##ParamsGPU & params = *listElems[i].paramsHandle.resolveAndCastTo< name##ParamsGPU >( KERNEL_CONST_MEM(modifierConstMem) ); \
			modifier##name <spawn, UsageType::usage> (params, input, pubState, privState, commonParams, randState); \
		} \
		else \

#define MODIFIER_LIST_END() \
		{ \
		} \
	} \
} \

template <int U>
struct Usage2Type
{
	static const int usage = U;
};

//Sprite modifiers
MODIFIER_LIST_BEG(physx::apex::ModifierUsage_Sprite)
#define _MODIFIER_SPRITE(name) MODIFIER_LIST_ELEM(name)
#include "../include/ModifierList.h"
MODIFIER_LIST_END()

//Mesh modifiers
MODIFIER_LIST_BEG(physx::apex::ModifierUsage_Mesh)
#define _MODIFIER_MESH(name) MODIFIER_LIST_ELEM(name)
#include "../include/ModifierList.h"
MODIFIER_LIST_END()


template <
	int Usage, typename Input, typename PublicState, typename PrivateState,
	typename InputArgs, typename PrivateStateArgs, typename OutputLayout
>
__device__ void modifiersKernel(
	unsigned int outputCount, unsigned int OutputDWords,
	unsigned int inStateOffset, unsigned int outStateOffset,
	InplaceHandle<AssetParamsHandleArray> assetParamsHandleArrayHandle,
	ModifierCommonParams commonParams, unsigned int numActorIDs,
	unsigned int* g_sortedActorIDs, unsigned int* g_sortedStateIDs, unsigned int* g_outStateToInput,
	InputArgs inputArgs, PrivateStateArgs privStateArgs,
	PRNGInfo rand,
	unsigned int* g_outputBuffer, OutputLayout outputLayout
)
{
	unsigned int idx = threadIdx.x;

	const unsigned int BlockSize = blockDim.x;
	const unsigned int Pitch = BlockSize + (NUM_BANKS + OutputDWords-1) / OutputDWords;
	extern __shared__ volatile unsigned int sdata[]; //size = (BlockSize + NUM_BANKS) * outputDWords;

	__shared__ physx::apex::LCG_PRNG randBlock;
	if (idx == 0) {
		randBlock = rand.g_randBlock[blockIdx.x];
	}

	for (unsigned int outputBeg = BlockSize * blockIdx.x; outputBeg < outputCount; outputBeg += BlockSize*gridDim.x)
	{
		physx::apex::LCG_PRNG randVal = (idx == 0 ? randBlock : rand.randThread);
		randVal = randScanBlock(randVal, sdata, sdata + BlockSize*2);

		unsigned int currSeed = randVal(rand.seed);
		if (idx == 0) {
			randBlock *= rand.randGrid;
		}
		__syncthreads();

		const unsigned int outputEnd = min(outputBeg + BlockSize, outputCount);
		const unsigned int outputID = outputBeg + idx;
		if (outputID < outputEnd)
		{
			unsigned int stateID = (g_sortedStateIDs[ outputID ] & STATE_ID_MASK);
			// stateID should be < maxStateID
			unsigned int inputID = tex1Dfetch( KERNEL_TEX_REF(InStateToInput), stateID );
			// inputID should be < maxInputID
			bool isNewParticle = ((inputID & NiIosBufferDesc::NEW_PARTICLE_FLAG) != 0);
			inputID &= ~NiIosBufferDesc::NEW_PARTICLE_FLAG;

			unsigned int actorID = g_sortedActorIDs[ outputID ];
			if (actorID < numActorIDs)
			{
				const AssetParamsHandleArray& assetParamsHandleArray = *assetParamsHandleArrayHandle.resolve( KERNEL_CONST_MEM(modifierConstMem) );
				const physx::PxU32 numberActorClasses = assetParamsHandleArray.getSize();
				const InplaceHandle<AssetParams> assetParamsHandle = assetParamsHandleArray.getElems( KERNEL_CONST_MEM(modifierConstMem) )[ actorID % numberActorClasses ];
				const AssetParams& assetParams = *assetParamsHandle.resolve( KERNEL_CONST_MEM(modifierConstMem) );

				//const ModifierCommonParams& commonParams = *commonParamsHandle.resolve( modifierConstMem );

				//prepare input
				Input		input;
				InputArgs::read(inputArgs, input, inputID, commonParams);

				//prepare state
				PublicState  pubState;
				PrivateState privState;

				//always run spawn modifiers
				PublicState::initDefault(pubState);
				PrivateState::initDefault(privState);

				unsigned int spawnSeed = isNewParticle ? currSeed : tex1Dfetch( KERNEL_TEX_REF(StateSpawnSeed), inStateOffset + stateID );
				RandState spawnRandState( spawnSeed );
				runModifiers<true>(Usage2Type<Usage>(), assetParams.spawnModifierList, commonParams, input, pubState, privState, spawnRandState);

				if (isNewParticle == false)
				{
					//read private state
					PrivateStateArgs::read(privStateArgs, privState, inStateOffset + stateID);
				}

				//run continuous modifiers
				RandState currRandState( currSeed );
				runModifiers<false>(Usage2Type<Usage>(), assetParams.continuousModifierList, commonParams, input, pubState, privState, currRandState);

				//write state
				rand.g_stateSpawnSeed[ outStateOffset + outputID ] = spawnSeed;
				PrivateStateArgs::write(privStateArgs, privState, outStateOffset + outputID);

				//write output to Output
				outputLayout.write(sdata, idx, Pitch, input, pubState, outputID);
			}
			g_outStateToInput[ outputID ] = inputID;
		}
		__syncthreads();

		if (g_outputBuffer != 0)
		{
			const unsigned int OutputBufferDwords = OutputDWords * (outputEnd - outputBeg);
			for (unsigned int pos = threadIdx.x; pos < OutputBufferDwords; pos += BlockSize)
			{
				g_outputBuffer[(outputBeg * OutputDWords) + pos] = sdata[(pos / OutputDWords) + (pos % OutputDWords)*Pitch];
			}
		}
		__syncthreads();
	}
}

// Sprite
namespace physx {
namespace apex {

struct SpriteInputArgs
{
	static __device__ void read(const SpriteInputArgs& args, SpriteInput& input, unsigned int pos, const ModifierCommonParams& commonParams)
	{
		float4 positionMass = tex1Dfetch(KERNEL_TEX_REF(PositionMass), pos);
		float4 velocityLife = tex1Dfetch(KERNEL_TEX_REF(VelocityLife), pos);
		float  density      = commonParams.inputHasDensity ? tex1Dfetch(KERNEL_TEX_REF(Density), pos) : 0;

		input.position.x = positionMass.x;
		input.position.y = positionMass.y;
		input.position.z = positionMass.z;
		input.mass       = positionMass.w;

		input.velocity.x = velocityLife.x;
		input.velocity.y = velocityLife.y;
		input.velocity.z = velocityLife.z;
		input.liferemain = velocityLife.w;

		input.density    = density;

		input.userData   = tex1Dfetch(KERNEL_TEX_REF(UserData), pos);
	}
};

__device__ unsigned int floatFlip(float f)
{
    unsigned int i = __float_as_int(f);
	unsigned int mask = -int(i >> 31) | 0x80000000;
	return i ^ mask;
}


__device__ void SpritePrivateStateArgs::read(const SpritePrivateStateArgs& args, SpritePrivateState& state, unsigned int pos)
{
	IofxSlice slice0 = uint4_to_IofxSlice(tex1Dfetch(KERNEL_TEX_REF(SpritePrivState0), pos));

	// Slice 0 (underused)
	state.rotation = __int_as_float(slice0.x);
}
__device__ void SpritePrivateStateArgs::write(SpritePrivateStateArgs& args, const SpritePrivateState& state, unsigned int pos)
{
	IofxSlice slice0;

	// Slice 0 (underused)
	slice0.x = __float_as_int(state.rotation);

	args.g_state[0][pos] = slice0;
}

// Mesh

struct MeshInputArgs
{
	static __device__ void read(const MeshInputArgs& args, MeshInput& input, unsigned int pos, const ModifierCommonParams& commonParams)
	{
		float4 positionMass         = tex1Dfetch(KERNEL_TEX_REF(PositionMass), pos);
		float4 velocityLife         = tex1Dfetch(KERNEL_TEX_REF(VelocityLife), pos);
		float4 collisionNormalFlags = commonParams.inputHasCollision ? tex1Dfetch(KERNEL_TEX_REF(CollisionNormalFlags), pos) : make_float4(0, 0, 0, 0);
		float  density              = commonParams.inputHasDensity ? tex1Dfetch(KERNEL_TEX_REF(Density), pos) : 0;

		input.position.x = positionMass.x;
		input.position.y = positionMass.y;
		input.position.z = positionMass.z;
		input.mass       = positionMass.w;

		input.velocity.x = velocityLife.x;
		input.velocity.y = velocityLife.y;
		input.velocity.z = velocityLife.z;
		input.liferemain = velocityLife.w;

		input.density    = density;

		input.collisionNormal.x = collisionNormalFlags.x;
		input.collisionNormal.y = collisionNormalFlags.y;
		input.collisionNormal.z = collisionNormalFlags.z;
		input.collisionFlags    = __float_as_int(collisionNormalFlags.w);

		input.userData   = tex1Dfetch(KERNEL_TEX_REF(UserData), pos);
	}
};


__device__ void MeshPrivateStateArgs::read(const MeshPrivateStateArgs& args, MeshPrivateState& state, unsigned int pos)
{
	IofxSlice slice0 = uint4_to_IofxSlice(tex1Dfetch(KERNEL_TEX_REF(MeshPrivState0), pos)),
		slice1 = uint4_to_IofxSlice(tex1Dfetch(KERNEL_TEX_REF(MeshPrivState1), pos)),
		slice2 = uint4_to_IofxSlice(tex1Dfetch(KERNEL_TEX_REF(MeshPrivState2), pos));

	// Slice 0
	state.rotation(0,0) = __int_as_float(slice0.x);
	state.rotation(0,1) = __int_as_float(slice0.y);
	state.rotation(0,2) = __int_as_float(slice0.z);
	state.rotation(1,0) = __int_as_float(slice0.w);

	// Slice 1
	state.rotation(1,1) = __int_as_float(slice1.x);
	state.rotation(1,2) = __int_as_float(slice1.y);
	state.rotation(2,0) = __int_as_float(slice1.z);
	state.rotation(2,1) = __int_as_float(slice1.w);

	// Slice 2 (underused)
	state.rotation(2,2) = __int_as_float(slice2.x);
}
__device__ void MeshPrivateStateArgs::write(MeshPrivateStateArgs& args, const MeshPrivateState& state, unsigned int pos)
{
	IofxSlice slice0, slice1, slice2;
	
	// Slice 0
	slice0.x = __float_as_int(state.rotation(0,0));
	slice0.y = __float_as_int(state.rotation(0,1));
	slice0.z = __float_as_int(state.rotation(0,2));
	slice0.w = __float_as_int(state.rotation(1,0));

	// Slice 1
	slice1.x = __float_as_int(state.rotation(1,1));
	slice1.y = __float_as_int(state.rotation(1,2));
	slice1.z = __float_as_int(state.rotation(2,0));
	slice1.w = __float_as_int(state.rotation(2,1));

	// Slice 2 (underused)
	slice2.x = __float_as_int(state.rotation(2,2));

	args.g_state[0][pos] = slice0;
	args.g_state[1][pos] = slice1;
	args.g_state[2][pos] = slice2;
}


}} // namespace apex

//__launch_bounds__( GET_WARPS_PER_BLOCK(SPRITE_MODIFIER_WARPS_PER_BLOCK) * WARP_SIZE )
BOUND_KERNEL_BEG(SPRITE_MODIFIER_WARPS_PER_BLOCK, spriteModifiersKernel,
	unsigned int inStateOffset, unsigned int outStateOffset,
	InplaceHandle<AssetParamsHandleArray> assetParamsHandleArrayHandle,
	ModifierCommonParams commonParams, unsigned int numActorIDs,
	unsigned int* g_sortedActorIDs, unsigned int* g_sortedStateIDs, unsigned int* g_outStateToInput,
	SpritePrivateStateArgs privStateArgs,
	PRNGInfo rand, unsigned int* g_outputBuffer,
	InplaceHandle<SpriteOutputLayout> outputLayoutHandle
)
	SpriteInputArgs inputArgs;

	const SpriteOutputLayout& outputLayout = *outputLayoutHandle.resolve( KERNEL_CONST_MEM(modifierConstMem) );
	unsigned int OutputDWords = (outputLayout.stride >> 2);

	modifiersKernel<ModifierUsage_Sprite, 
		SpriteInput, SpritePublicState, SpritePrivateState, 
		SpriteInputArgs, SpritePrivateStateArgs, SpriteOutputLayout>
	(
		_threadCount, OutputDWords,
		inStateOffset, outStateOffset,
		assetParamsHandleArrayHandle,
		commonParams, numActorIDs,
		g_sortedActorIDs, g_sortedStateIDs, g_outStateToInput,
		inputArgs, privStateArgs,
		rand,
		g_outputBuffer, outputLayout
	);
BOUND_KERNEL_END()


//__launch_bounds__( GET_WARPS_PER_BLOCK(SPRITE_MODIFIER_WARPS_PER_BLOCK) * WARP_SIZE )
BOUND_KERNEL_BEG(SPRITE_MODIFIER_WARPS_PER_BLOCK, spriteTextureModifiersKernel,
	unsigned int inStateOffset, unsigned int outStateOffset,
	InplaceHandle<AssetParamsHandleArray> assetParamsHandleArrayHandle,
	ModifierCommonParams commonParams, unsigned int numActorIDs,
	unsigned int* g_sortedActorIDs, unsigned int* g_sortedStateIDs, unsigned int* g_outStateToInput,
	SpritePrivateStateArgs privStateArgs,
	PRNGInfo rand, SpriteTextureOutputLayout outputLayout
)
	SpriteInputArgs inputArgs;

	modifiersKernel<ModifierUsage_Sprite, 
		SpriteInput, SpritePublicState, SpritePrivateState, 
		SpriteInputArgs, SpritePrivateStateArgs, SpriteTextureOutputLayout>
	(
		_threadCount, 1,
		inStateOffset, outStateOffset,
		assetParamsHandleArrayHandle,
		commonParams, numActorIDs,
		g_sortedActorIDs, g_sortedStateIDs, g_outStateToInput,
		inputArgs, privStateArgs,
		rand,
		0, outputLayout
	);
BOUND_KERNEL_END()

//__launch_bounds__( GET_WARPS_PER_BLOCK(MESH_MODIFIER_WARPS_PER_BLOCK) * WARP_SIZE )
BOUND_KERNEL_BEG(MESH_MODIFIER_WARPS_PER_BLOCK, meshModifiersKernel,
	unsigned int inStateOffset, unsigned int outStateOffset,
	InplaceHandle<AssetParamsHandleArray> assetParamsHandleArrayHandle,
	ModifierCommonParams commonParams, unsigned int numActorIDs,
	unsigned int* g_sortedActorIDs, unsigned int* g_sortedStateIDs, unsigned int* g_outStateToInput,
	MeshPrivateStateArgs privStateArgs,
	PRNGInfo rand,
	unsigned int* g_outputBuffer,
	InplaceHandle<MeshOutputLayout> outputLayoutHandle
)
	MeshInputArgs inputArgs;

	const MeshOutputLayout& outputLayout = *outputLayoutHandle.resolve( KERNEL_CONST_MEM(modifierConstMem) );
	unsigned int OutputDWords = (outputLayout.stride >> 2);

	modifiersKernel<ModifierUsage_Mesh,
		MeshInput, MeshPublicState, MeshPrivateState,
		MeshInputArgs, MeshPrivateStateArgs, MeshOutputLayout>
	(
		_threadCount, OutputDWords,
		inStateOffset, outStateOffset,
		assetParamsHandleArrayHandle,
		commonParams, numActorIDs,
		g_sortedActorIDs, g_sortedStateIDs, g_outStateToInput,
		inputArgs, privStateArgs,
		rand,
		g_outputBuffer, outputLayout
	);
BOUND_KERNEL_END()
