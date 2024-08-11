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

#ifndef NI_APEX_RENDER_MESH_ASSET_H
#define NI_APEX_RENDER_MESH_ASSET_H

#include "PsShare.h"
#include "NxRenderMeshActor.h"
#include "NxRenderMeshAsset.h"
#include "PsArray.h"
#include "PxMat34Legacy.h"

namespace physx
{
namespace apex
{

// Forward declarations
class DebugRenderParams;
class NiApexRenderDebug;

class NiApexVertexBuffer : public NxVertexBuffer
{
public:
	virtual ~NiApexVertexBuffer() {}
	virtual NxVertexFormat& getFormatWritable() = 0;
	virtual void build(const NxVertexFormat& format, physx::PxU32 vertexCount) = 0;
	virtual void applyTransformation(const physx::PxMat34Legacy& transformation) = 0;
	virtual void applyScale(physx::PxF32 scale) = 0;
	virtual bool mergeBinormalsIntoTangents() = 0;
};

class NiApexRenderSubmesh : public NxRenderSubmesh
{
public:
	virtual ~NiApexRenderSubmesh() {}

	virtual NiApexVertexBuffer& getVertexBufferWritable() = 0;
	virtual PxU32* getIndexBufferWritable(PxU32 partIndex) = 0;
	virtual void applyPermutation(const Array<PxU32>& old2new, const Array<PxU32>& new2old) = 0;
};


/**
* Framework interface to ApexRenderMesh for use by modules
*/
class NiApexRenderMeshAsset : public NxRenderMeshAsset
{
public:
	virtual NiApexRenderSubmesh&	getNiSubmesh(physx::PxU32 submeshIndex) = 0;
	virtual void					permuteBoneIndices(const physx::Array<physx::PxI32>& old2new) = 0;
	virtual void					applyTransformation(const physx::PxMat34Legacy& transformation, physx::PxF32 scale) = 0;
	virtual void					applyScale(physx::PxF32 scale) = 0;
	virtual bool					mergeBinormalsIntoTangents() = 0;
	virtual void					setOwnerModuleId(NxAuthObjTypeID id) = 0;
	virtual NxTextureUVOrigin::Enum getTextureUVOrigin() const = 0;

};

class NiApexRenderMeshAssetAuthoring : public NxRenderMeshAssetAuthoring
{
public:
	virtual NiApexRenderSubmesh&	getNiSubmesh(physx::PxU32 submeshIndex) = 0;
	virtual void					permuteBoneIndices(const physx::Array<physx::PxI32>& old2new) = 0;
	virtual void					applyTransformation(const physx::PxMat34Legacy& transformation, physx::PxF32 scale) = 0;
	virtual void					applyScale(physx::PxF32 scale) = 0;
};


class NiApexRenderMeshActor : public NxRenderMeshActor
{
public:
	virtual void updateRenderResources(bool useBones, bool rewriteBuffers, void* userRenderData) = 0;

	// add a buffer that will replace the dynamic buffer for the submesh
	virtual void addVertexBuffer(PxU32 submeshIndex, bool alwaysDirty, PxVec3* position, PxVec3* normal, PxVec4* tangents) = 0;
	virtual void removeVertexBuffer(PxU32 submeshIndex) = 0;

	virtual void setStaticPositionReplacement(PxU32 submeshIndex, const PxVec3* staticPositions) = 0;
	virtual void setStaticColorReplacement(PxU32 submeshIndex, const PxColorRGBA* staticColors) = 0;

	virtual void visualize(NiApexRenderDebug& batcher, physx::apex::DebugRenderParams* debugParams, PxMat33* scaledRotations = NULL, PxVec3* translations = NULL, PxU32 stride = 0, PxU32 numberOfTransforms = 0) const = 0;

	virtual void dispatchRenderResources(NxUserRenderer& renderer, const physx::PxMat44& globalPose) = 0;

	// Access to previous frame's transforms (if the buffer exists)
	virtual void setLastFrameTM(const physx::PxMat44& tm, physx::PxU32 boneIndex = 0) = 0;
	virtual void setLastFrameTM(const physx::PxMat44& tm, const physx::PxVec3& scale, physx::PxU32 boneIndex = 0) = 0;
};

} // end namespace apex
} // end namespace physx

#endif // NI_APEX_RENDER_MESH_ASSET_H
