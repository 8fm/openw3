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


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains code to build a clean mesh.
 *	\file		IceMeshBuilder2.cpp
 *	\author		Pierre Terdiman
 *	\date		January, 29, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	A mesh converter/builder/cleaner.
 *	- Packs multiple independent topologies in a single hardware-friendly one.
 *	- Tracks and deletes unused or redundant vertices, mapping coordinates or vertex colors.
 *	- Kills zero-area faces.
 *	- Computes vertex normals according to smoothing groups & edge angles.
 *	- Outputs normal information used to recompute them easily at runtime.
 *	- Groups faces in submeshes, according to their rendering properties.
 *	- Duplicates as few vertices as necessary.
 *
 *	\class		MeshBuilder2
 *	\author		Pierre Terdiman
 *	\version	3.7
 *	\warning	support for strips, edge visibility and progressive meshes has been moved elsewhere.
 *	\warning	support for user-defined data has been removed! Use indexed geometry instead.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//	3.5:
//	- second mapping channel added
//	3.6:
//	- vertex normals weighted with angles (thanks to Maurizio De Girolami for the tip)
//	3.61:
//	- remap table NULL if identity
//	3.7:
//	- more robust zero-area detection

#include "GuIceSupport.h"
#include "IceMeshBuilderResults.h"
#include "IceMeshBuilder2.h"
#include "CookingUtils.h"
#include "PsMathUtils.h"
#include "./Ice/IceRevisitedRadix2.h"

#ifndef PX_COOKING
#error Do not include anymore!
#endif

using namespace physx;
using namespace Gu;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Constructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MeshBuilder2::MeshBuilder2()
{
	mNbFaces				= 0;
	mNbVerts				= 0;
	mNbTVerts				= 0;
	mNbCVerts				= 0;
	mNbAvailableFaces		= 0;
	mNbBuildVertices		= 0;
	mNewVertsIndex			= 0;

	mVerts					= NULL;
	mTVerts					= NULL;
	mCVerts					= NULL;

	mFacesArray				= NULL;
	mVertexArray			= NULL;

	mFCounts				= NULL;
	mVOffsets				= NULL;
	mVertexToFaces			= NULL;
	mOutToIn				= NULL;
	mNbNorm					= 0;
	mCrossIndex				= 0;

//	mOptimizeFlags			= VCODE_VERTEX|VCODE_UVW|VCODE_COLOR;
	mKillZeroAreaFaces		= true;
	mUseW					= false;
	mComputeVNorm			= false;
	mComputeFNorm			= false;
	mComputeNormInfo		= false;
	mIndexedGeo				= true;
	mIndexedUVW				= false;
	mIndexedColors			= false;
	mRelativeIndices		= false;
	mIsSkin					= false;
	mWeightNormalWithAngles	= false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Destructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MeshBuilder2::~MeshBuilder2()
{
	FreeUsedRam();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Frees the internal used ram.
 *	\return		Self-Reference.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MeshBuilder2& MeshBuilder2::FreeUsedRam()
{
	mArrayTopology			.reset();
	mArrayTopoSize			.reset();
	mArrayVertsRefs			.reset();
	mArrayTVertsRefs		.reset();
	mArrayColorRefs			.reset();
	mArrayVerts				.reset();
	mArrayTVerts			.reset();
	mArrayCVerts			.reset();
	mArrayVNormals			.reset();
	mArrayFNormals			.reset();
	mArrayNormalInfo		.reset();
	mArraySubmeshProperties	.reset();
	mMaterialInfo			.reset();
	PX_FREE_AND_RESET(mVerts);
	PX_FREE_AND_RESET(mTVerts);
	PX_FREE_AND_RESET(mCVerts);
	PX_FREE_AND_RESET(mFacesArray);
	PX_DELETE_POD(mVertexArray);
	PX_DELETE_POD(mFCounts);
	PX_DELETE_POD(mVOffsets);
	PX_DELETE_POD(mVertexToFaces);
	PX_DELETE_POD(mOutToIn);
	return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Initializes MeshBuilder.
 *	Standard usage:
 *	- Init the builder
 *	- Call AddFace() for each triangle
 *	- Call Build()
 *
 *	\param		create		[in] initialization structure.
 *	\return		true if success.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool MeshBuilder2::Init(const MBCreate& create)
{
	struct Local
	{
		static bool SetupXVerts(PxU32 nb, const PxVec3* source, PxVec3*& dest, PxU32& nb_verts)
		{
			nb_verts	= nb;	if(!nb) return true;
			dest		= (PxVec3*)PX_ALLOC(sizeof(PxVec3) * nb, PX_DEBUG_EXP("PxVec3"));
			//dest		= new PxVec3[nb];
			if(source)	PxMemCopy(dest, source, nb*sizeof(PxVec3));
			else		PxMemZero(dest, nb*sizeof(PxVec3));
			return true;
		}
	};

	// Free already used ram
	FreeUsedRam();

	// Save build parameters
//	mOptimizeFlags			= create.OptimizeFlags;
	mKillZeroAreaFaces		= create.KillZeroAreaFaces;
	mUseW					= create.UseW;
	mComputeVNorm			= create.ComputeVNorm;
	mComputeFNorm			= create.ComputeFNorm;
	mComputeNormInfo		= create.ComputeNormInfo;
	mIndexedGeo				= create.IndexedGeo;
	mIndexedUVW				= create.IndexedUVW;
	mIndexedColors			= create.IndexedColors;
	mRelativeIndices		= create.RelativeIndices;
	mIsSkin					= create.IsSkin;
	mWeightNormalWithAngles	= create.WeightNormalWithAngles;
	mOptimizeVertexList		= create.OptimizeVertexList;

	// Geometries
	if(!Local::SetupXVerts(create.NbVerts, create.Verts, mVerts, mNbVerts))		return false;
	if(!Local::SetupXVerts(create.NbTVerts, create.TVerts, mTVerts, mNbTVerts))	return false;
	if(!Local::SetupXVerts(create.NbCVerts, create.CVerts, mCVerts, mNbCVerts))	return false;

	// Special case for UV mappings
	if(!mUseW && mTVerts)
	{
		// The input data may contain unused W values. Since we're going to sort them anyway,
		// we should clear the array just in case...
		for(PxU32 i=0;i<mNbTVerts;i++)	mTVerts[i].z = 0.0f;
	}

	// Topologies - You're supposed to call AddFace() mNbFaces times afterwards.
	mNbFaces = create.NbFaces;	if(!mNbFaces) return false;
	mFacesArray		= (MBFace*)PX_ALLOC(sizeof(MBFace) * mNbFaces, PX_DEBUG_EXP("MBFace"));
	mVertexArray	= PX_NEW(MBVertex)[mNbFaces * 3];

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Adds a face to the database. You're supposed to call Build() afterwards.
 *	\param		face	[in] data for added face
 *	\return		true if success.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool MeshBuilder2::AddFace(const MBFaceData& face)
{
	// Checkings
	if(!mFacesArray || !mVertexArray)	return false;

	// Check for too many faces
	if(mNbAvailableFaces==mNbFaces)		return false;
	if(face.Index>mNbFaces)				return false;

	// Skip zero-area faces (for the topology only! e.g. a texture-face can be multiply defined)
	if(mKillZeroAreaFaces && face.VRefs)
	{
		if(face.VRefs[0]==face.VRefs[1] || face.VRefs[0]==face.VRefs[2] || face.VRefs[1]==face.VRefs[2]) return true;

		// v 3.7 : better zero-area detection
		const PxVec3& p0 = mVerts[face.VRefs[0]];
		const PxVec3& p1 = mVerts[face.VRefs[1]];
		const PxVec3& p2 = mVerts[face.VRefs[2]];

		float Area2 = ((p0 - p1).cross(p0 - p2)).magnitudeSquared();
		if(Area2==0.0f)	return true;
	}

	// Store face properties. Smoothing groups are discarded when we don't need vertex normals.
	// In other words, the number of replicated vertices and number of submeshes may also be modified.
	mFacesArray[mNbAvailableFaces].MatID	= face.MaterialID;
	mFacesArray[mNbAvailableFaces].SmGroup	= mComputeVNorm ? face.SmoothingGroups : 1;
	mFacesArray[mNbAvailableFaces].Index	= face.Index;
	mFacesArray[mNbAvailableFaces].Normal	= PxVec3(0.f);

	// Create 3 vertices. At first nothing is shared, and 3 brand new vertices are created for each face.
	const PxU8 CCW = face.CCW!=0;
	mVertexArray[mNbBuildVertices+0].VRef	= face.VRefs ? face.VRefs[0]		: PX_INVALID_U32;
	mVertexArray[mNbBuildVertices+1].VRef	= face.VRefs ? face.VRefs[1+CCW]	: PX_INVALID_U32;
	mVertexArray[mNbBuildVertices+2].VRef	= face.VRefs ? face.VRefs[2-CCW]	: PX_INVALID_U32;

	mVertexArray[mNbBuildVertices+0].TRef	= face.TRefs ? face.TRefs[0]		: PX_INVALID_U32;
	mVertexArray[mNbBuildVertices+1].TRef	= face.TRefs ? face.TRefs[1+CCW]	: PX_INVALID_U32;
	mVertexArray[mNbBuildVertices+2].TRef	= face.TRefs ? face.TRefs[2-CCW]	: PX_INVALID_U32;

	mVertexArray[mNbBuildVertices+0].CRef	= face.CRefs ? face.CRefs[0]		: PX_INVALID_U32;
	mVertexArray[mNbBuildVertices+1].CRef	= face.CRefs ? face.CRefs[1+CCW]	: PX_INVALID_U32;
	mVertexArray[mNbBuildVertices+2].CRef	= face.CRefs ? face.CRefs[2-CCW]	: PX_INVALID_U32;

	// Fix whacked meshes right from the start
	if(face.VRefs)
	{
		if(mVertexArray[mNbBuildVertices+0].VRef>=mNbVerts)		mVertexArray[mNbBuildVertices+0].VRef = 0;
		if(mVertexArray[mNbBuildVertices+1].VRef>=mNbVerts)		mVertexArray[mNbBuildVertices+1].VRef = 0;
		if(mVertexArray[mNbBuildVertices+2].VRef>=mNbVerts)		mVertexArray[mNbBuildVertices+2].VRef = 0;
	}
	if(face.TRefs)
	{
		if(mVertexArray[mNbBuildVertices+0].TRef>=mNbTVerts)	mVertexArray[mNbBuildVertices+0].TRef = 0;
		if(mVertexArray[mNbBuildVertices+1].TRef>=mNbTVerts)	mVertexArray[mNbBuildVertices+1].TRef = 0;
		if(mVertexArray[mNbBuildVertices+2].TRef>=mNbTVerts)	mVertexArray[mNbBuildVertices+2].TRef = 0;
	}
	if(face.CRefs)
	{
		if(mVertexArray[mNbBuildVertices+0].CRef>=mNbCVerts)	mVertexArray[mNbBuildVertices+0].CRef = 0;
		if(mVertexArray[mNbBuildVertices+1].CRef>=mNbCVerts)	mVertexArray[mNbBuildVertices+1].CRef = 0;
		if(mVertexArray[mNbBuildVertices+2].CRef>=mNbCVerts)	mVertexArray[mNbBuildVertices+2].CRef = 0;
	}
	// Create a new global face
	mFacesArray[mNbAvailableFaces].Ref[0]	= mNbBuildVertices++;
	mFacesArray[mNbAvailableFaces].Ref[1]	= mNbBuildVertices++;
	mFacesArray[mNbAvailableFaces].Ref[2]	= mNbBuildVertices++;

	// Update member. mNbAvailableFaces may finally be different from mNbFaces when zero-area faces are killed.
	mNbAvailableFaces++;

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Fixes the NULL smoothing groups
 *	\return		true if success.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool MeshBuilder2::FixNULLSmoothingGroups()
{
	// Do nothing if we don't need vertex normals
	if(!mComputeVNorm)	return true;

	PxU32 Index = mNbVerts;
	Ps::Array<PxVec3> Replicated;
	// Loop through available faces
	for(PxU32 i=0;i<mNbAvailableFaces;i++)
	{
		// Look for NULL smoothing groups
		if(!mFacesArray[i].SmGroup)
		{
			// For skins, we don't replicate vertices here, since we don't have access
			// to the skinning info (which must be replicated as well). That is, NULL smoothing
			// groups are not handled correctly for skins.........
			if(!mIsSkin)
			{
				// Replicate 3 vertices
				Replicated.pushBack(mVerts[mVertexArray[mFacesArray[i].Ref[0]].VRef]);
				Replicated.pushBack(mVerts[mVertexArray[mFacesArray[i].Ref[1]].VRef]);
				Replicated.pushBack(mVerts[mVertexArray[mFacesArray[i].Ref[2]].VRef]);
				// Assign new (and unique) vertex references
				mVertexArray[mFacesArray[i].Ref[0]].VRef = Index++;
				mVertexArray[mFacesArray[i].Ref[1]].VRef = Index++;
				mVertexArray[mFacesArray[i].Ref[2]].VRef = Index++;
			}
			// Arbitrary non-NULL group
			mFacesArray[i].SmGroup = 0xffffffff;
		}
	}

	// Avoid useless work...
	PxU32 NbNewVerts = Replicated.size();
	if(!NbNewVerts)	return true;

	// Setup new vertex cloud
	PxVec3* NewVerts = (PxVec3*)PX_ALLOC(sizeof(PxVec3) * (mNbVerts+NbNewVerts), PX_DEBUG_EXP("PxVec3"));
	//PxVec3* NewVerts = new PxVec3[mNbVerts+NbNewVerts];
	PxMemCopy(&NewVerts[0],			mVerts,				mNbVerts*sizeof(PxVec3));
	PxMemCopy(&NewVerts[mNbVerts],	Replicated.begin(),	NbNewVerts*sizeof(PxVec3));

	PX_FREE_AND_RESET(mVerts);
	mVerts = NewVerts;
	mNbVerts+=NbNewVerts;

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Rebuilds a clean mesh.
 *	\param		result	[out] a structure with final computed data
 *	\return		true if success.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool MeshBuilder2::Build(MBResult& result)
{
	// Checkings
	if(!mNbAvailableFaces)			return false;

	// Face cross-ref list
	mOutToIn = PX_NEW(PxU32)[mNbAvailableFaces];
	//mOutToIn = new PxU32[mNbAvailableFaces];
//	StoreDwords(mOutToIn, mNbAvailableFaces, PX_INVALID_U32);
	memset(mOutToIn, 0xff, mNbAvailableFaces*sizeof(PxU32));
	mCrossIndex = 0;

	// Cleaning room
	if(!OptimizeTopology())			return false;
	if(!FixNULLSmoothingGroups())	return false;
	if(!OptimizeGeometry())			return false;
	if(!ComputeNormals())			return false;
	if(!SaveXVertices())			return false;
	if(!ComputeSubmeshes())			return false;

	// Fill result structure
	PxU32 NbSubmeshes		= mArrayTopoSize.size();
	PxU32 NormalInfoSize	= mArrayNormalInfo.size();

	// Topology
						result.Topology.VRefs				=				mArrayTopology			.begin();	// vertex-indices for trilists
	PxU32* NbFaces =	result.Topology.FacesInSubmesh		=				mArrayTopoSize			.begin();	// #faces for trilists
						result.Topology.Normals				= (float*)		mArrayFNormals			.begin();
	// Properties
	MBSubmesh* Props =	result.Topology.SubmeshProperties	= (MBSubmesh*)	mArraySubmeshProperties	.begin();
	// Vertices
		// Points
						result.Geometry.VertsRefs			=				mArrayVertsRefs			.begin();	// Only !=NULL if mIndexedGeo
						result.Geometry.Verts				= (float*)		mArrayVerts				.begin();
		// UVs
						result.Geometry.TVertsRefs			=				mArrayTVertsRefs		.begin();	// Only !=NULL if mIndexedUVW
						result.Geometry.TVerts				=				mArrayTVerts			.begin();
						result.Geometry.UseW				=				mUseW;
		// Vertex-colors
						result.Geometry.ColorRefs			=				mArrayColorRefs			.begin();	// Only !=NULL if mIndexedColors
						result.Geometry.CVerts				= (float*)		mArrayCVerts			.begin();
		// Normals
						result.Geometry.Normals				= (float*)		mArrayVNormals			.begin();
						result.Geometry.NormalInfo			=				mArrayNormalInfo		.begin();

	// Optimize vertex lists for cache-coherence
	PxU32* GlobalPermutation = NULL;

	// Compute material info (compute #materials and #faces for each material, aso)
	// A submesh is a bunch of faces sharing a material ID and smoothing groups. That is, two different submeshes can still have
	// the same material ID. A submesh is really the "cleanest" possible entity, where each vertex (position) maps a single UV,
	// a single normal, a single color... This is not needed for efficient rendering. We can render multiple submeshes in one run
	// with replicated vertices (positions) within a vertex buffer, the important thing is the material ID since it leads to
	// SetMaterial/SetTexture calls - hence multiple DrawIndexedPrimitive in D3D.
	PxU32 Nb = mArraySubmeshProperties.size() / (sizeof(MBSubmesh)/sizeof(PxU32));	// Must be equal to NbSubmeshes
	PX_ASSERT(Nb==NbSubmeshes);
	PxI32 OldMatID = -1;
	PxU32 NbF = 0, CurNbVtxData = 0, CurNbSubm = 0;	// Material params
	PxU32 TotalNbFaces = 0, TotalNbVertices = 0;		// Global params
	// Loop through submeshes
	for(PxU32 i=0;i<Nb;i++)
	{
		// Get current submesh's material ID
		PxI32 CurMatID = Props[i].MatID;
		if(CurMatID!=OldMatID)
		{
			if(OldMatID!=-1)	// ### to check
			{
				// Create a new material entry
				MBMatInfo info;
				info.MatID			= OldMatID;
				info.NbFaces		= NbF;
				info.NbVerts		= CurNbVtxData;
				info.NbSubmeshes	= CurNbSubm;
				mMaterialInfo.pushBack(info);
				// Reset for next one
				CurNbSubm		= 0;
				NbF				= 0;
				CurNbVtxData	= 0;
			}
			OldMatID = CurMatID;
		}
		// Update material params
		CurNbSubm++;
		NbF				+= NbFaces[i];
		CurNbVtxData	+= Props[i].NbVerts;
		// Update global params
		TotalNbFaces	+= NbFaces[i];
		TotalNbVertices	+= Props[i].NbVerts;
	}
	// Create last entry
	MBMatInfo info;
	info.MatID			= OldMatID;
	info.NbFaces		= NbF;
	info.NbVerts		= CurNbVtxData;
	info.NbSubmeshes	= CurNbSubm;
	mMaterialInfo.pushBack(info);

	// Expose material results
	result.Materials.NbMtls			= mMaterialInfo.size();
	result.Materials.MaterialInfo	= mMaterialInfo.begin();

	// Expose data
	result.Topology.NbFaces			= TotalNbFaces;
	result.Topology.NbSourceFaces	= mNbFaces;
	result.Topology.NbSubmeshes		= NbSubmeshes;
	result.Geometry.NbVerts			= TotalNbVertices;
	result.Geometry.NbGeomPts		= mNbVerts;
	result.Geometry.NbTVerts		= mNbTVerts;
	result.Geometry.NbColorVerts	= mNbCVerts;
	result.Geometry.NormalInfoSize	= NormalInfoSize;
	PX_ASSERT(TotalNbFaces==mNbAvailableFaces);

	// Fix NormalInfo.
	// mNbNorm should be equal to TotalNbVertices. mCrossIndex should be equal to mNbAvailableFaces.
	PxU32* NormalData = result.Geometry.NormalInfo;
	// Is normal information needed?
	if(NormalData)
	{
		// We need a correspondence table from old indices to new ones. We currently have the opposite.
		PxU32* mInToOut = PX_NEW(PxU32)[mNbAvailableFaces];
		//PxU32* mInToOut = new PxU32[mNbAvailableFaces];
		for(PxU32 i=0;i<mCrossIndex;i++)	mInToOut[mOutToIn[i]] = i;

		// Now we can fix our data. Expect mNbNorm normals.
		for(PxU32 i=0;i<mNbNorm;i++)
		{
			PxU32 Count = *NormalData++;	// Current vertex-normal depends on Count face-normals.
			while(Count--)
			{
				PxU32 Index = *NormalData;									// Face index before MeshBuilder was applied, i.e. index in mFacesArray
				Index = mInToOut[Index];									// Remap the index to the consolidatated list
				if(GlobalPermutation)	Index = GlobalPermutation[Index];	// Remap the index to the optimized list
				*NormalData++ = Index;
			}
		}
		PX_DELETE_POD(mInToOut);
	}

	// Build out-to-in table
	// mOutToIn currently maps the exposed list of faces to mFacesArray. Almost what we want, we just need to take
	// degenerate faces into account. No new buffer, we just recycle mOutToIn.
	bool IsIdentity = true;
	for(PxU32 i=0;i<mNbAvailableFaces;i++)
	{
		PxU32 Index = mOutToIn[i];				// Index in mFacesArray
		mOutToIn[i] = mFacesArray[Index].Index;	// Index in the original list from the user
		if(mOutToIn[i]!=i)	IsIdentity = false;
	}
// ### missing global permut !
	// Finally expose the remap table
	result.Topology.Map = IsIdentity ? NULL : mOutToIn;

	// Free used ram
	PX_DELETE_POD(GlobalPermutation);

	// TEST
	if(0)
	{
		// Try to optimize this
		if(mNbVerts &&& result.Geometry.VertsRefs && TotalNbVertices==mNbVerts)
		{
			PxVec3* Temp = (PxVec3*)PX_ALLOC_TEMP(sizeof(PxVec3) * mNbVerts, PX_DEBUG_EXP("PxVec3"));
			//PxVec3* Temp = new PxVec3[mNbVerts];

			PxVec3* V = (PxVec3*)result.Geometry.Verts;
			PxMemCopy(Temp, V, mNbVerts*sizeof(PxVec3));

			PxU32* Map = result.Geometry.VertsRefs;
			for(PxU32 i=0;i<mNbVerts;i++)
			{
				V[i] = Temp[Map[i]];
				Map[i] = i;
			}
			PX_FREE_AND_RESET(Temp);
			result.Geometry.VertsRefs = NULL;
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Rebuilds clean mappings.
 *	This is useful for Box mapped objects... Example:
 *	This routine looks for redundant UV mappings in the original MAX list, creates an optimized list, and remaps the object with it.
 *	This may be useful with indexed UV. Ex in a cube, you only need 2 indices (0.0 and 1.0) but MAX uses a lot of them (although using texture faces!).
 *	This may be useless with non-strided data (in a DirectX way of thinking) but it's worth running the code...
 *	\return		true if success.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool MeshBuilder2::OptimizeTopology()
{
	// Skin does not contain valid vertices when the consolidation is performed, since the skin vertices are recomputed each frame.
	// Hence, the optimization pass must be avoided, otherwise it could lead to wrong results. For example if the skin vertices are all
	// NULL at consolidation time, they will be reduced to a single vertex, and this is obviously not what we want.
	if(!mIsSkin)
	{
		// Optimize vertices
		if(!OptimizeXMappings(mNbVerts, mVerts, VCODE_VERTEX))	return false;
	}

	// Optimize UV-mappings
	if(!OptimizeXMappings(mNbTVerts, mTVerts, VCODE_UVW))	return false;

	// Optimize vertex colors
	if(!OptimizeXMappings(mNbCVerts, mCVerts, VCODE_COLOR))	return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Optimizes a list of vertices and rebuilds correct mappings.
 *	- unused vertices are removed
 *	- redundant vertices are removed
 *	- faces are remapped with correct references
 *
 *	X-vertices are vertices, texture-vertices or vertex-colors.
 *
 *	\param		nbxverts	[in/out] The original number of points in the list. On output, updated with the new number of vertices.
 *	\param		xverts		[in/out] The original list of points. On output, updated with the new list.
 *	\param		vcode		[in] Vertex code, since extra work is needed for UV-mappings.
 *	\return		true if success.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool MeshBuilder2::OptimizeXMappings(PxU32& nbxverts, PxVec3*& xverts, VertexCode vcode)
{
	// No x-vertices => nothing to do
	if(!nbxverts) return true;

	// 1) X-vertices are stored in xverts, and used ones are indexed by mVertexArray[i].XRef. Some unreferenced X-vertices
	// may exist in xverts, and the first job is to wipe them out.
	PxVec3* UsedVertices;
	PxU32 NbUsedVertices = 0;
	{
		// 1-1) Mark used vertices
		bool* Markers = PX_NEW_TEMP(bool)[nbxverts];
		PxMemZero(Markers, nbxverts*sizeof(bool));

		// Loop through BuildVertices
		for(PxU32 i=0;i<mNbBuildVertices;i++)
		{
			PxU32 XRef=0xffffffff;

			// Get correct X-ref
					if(vcode==VCODE_VERTEX)	XRef = mVertexArray[i].VRef;
			else	if(vcode==VCODE_UVW)	XRef = mVertexArray[i].TRef;
			else	if(vcode==VCODE_COLOR)	XRef = mVertexArray[i].CRef;

			PX_ASSERT(XRef!=0xffffffff);

#ifdef OLD_WAY
			// Check the reference is valid
			if(XRef>=nbxverts)
			{
				// Input mesh is whacked! Try to fix...
				XRef = 0;
			}
#endif
			// Mark used vertex
			Markers[XRef] = true;
		}

		// 1-2) Now create list of actually used X-vertices
		PxU32* CrossRef	= PX_NEW_TEMP(PxU32)[nbxverts];
		//PxU32* CrossRef	= new PxU32[nbxverts];
		if(!CrossRef)		{ PX_DELETE_POD(Markers);	return false; }

		UsedVertices		= (PxVec3*)PX_ALLOC_TEMP(sizeof(PxVec3) * nbxverts, PX_DEBUG_EXP("PxVec3"));
		//UsedVertices		= new PxVec3[nbxverts];
		if(!UsedVertices)	{ PX_DELETE_POD(CrossRef); PX_DELETE_POD(Markers);	return false; }

		// Loop through input vertices...
		for(PxU32 i=0;i<nbxverts;i++)
		{
			// ...and keep used ones only.
			if(Markers[i])
			{
				// Create the cross-ref table used to remap faces
				CrossRef[i] = NbUsedVertices;
				// Save used vertex
				UsedVertices[NbUsedVertices++] = xverts[i];
			}
		}
		PX_DELETE_POD(Markers);

		// 1-3) Remap faces
		for(PxU32 i=0;i<mNbBuildVertices;i++)
		{
			PxU32 OldXRef;

					if(vcode==VCODE_VERTEX)	{ OldXRef = mVertexArray[i].VRef; mVertexArray[i].VRef = CrossRef[OldXRef]; }
			else	if(vcode==VCODE_UVW)	{ OldXRef = mVertexArray[i].TRef; mVertexArray[i].TRef = CrossRef[OldXRef]; }
			else	if(vcode==VCODE_COLOR)	{ OldXRef = mVertexArray[i].CRef; mVertexArray[i].CRef = CrossRef[OldXRef]; }
		}

		PX_DELETE_POD(CrossRef);
	}

	// Here, the new list of points is UsedVertices, and the new number of points is NbUsedVertices.
	// Topologies have been fixed.

#ifdef OLD_WAY
	// 2) Special case for UV mappings
	if(!mUseW && vcode==VCODE_UVW)
	{
		// The input data may contain unused W values. Since we're going to sort them anyway,
		// we should clear the array just in case...
		for(PxU32 i=0;i<NbUsedVertices;i++)	UsedVertices[i].z = 0.0f;
	}
#endif

	// 3) Now, the list of actually used X-vertices may still contain redundant vertices.
	// Since X-vertices are no more than points in X space, I can use a Vertex Cloud Reducer.
	// We don't *weld* nearby vertices there, we just look for redundant ones.
	PxU32 OldNbXVerts = nbxverts;		// Save it for log text
	PX_UNUSED(OldNbXVerts);
	{
		// Initialize vertex cloud reducer
		ReducedVertexCloud	Reducer(UsedVertices, NbUsedVertices);

		// Reduce cloud
		REDUCEDCLOUD rc;
		Reducer.Reduce(&rc);

		// Avoid useless work....
		if(rc.NbRVerts==NbUsedVertices)
		{
			// Create the final list of vertices
			PxVec3* NewXVerts = (PxVec3*)PX_ALLOC(sizeof(PxVec3) * NbUsedVertices, PX_DEBUG_EXP("PxVec3"));
			//PxVec3* NewXVerts = new PxVec3[NbUsedVertices];
			PxMemCopy(NewXVerts, UsedVertices, NbUsedVertices*sizeof(PxVec3));
			nbxverts = NbUsedVertices;

			// Replace old list
			PX_FREE_AND_RESET(xverts);
			xverts = NewXVerts;

			// We can free this now
			PX_FREE_AND_RESET(UsedVertices);
		}
		else
		{
			// We can free this now
			PX_FREE_AND_RESET(UsedVertices);

			// Final remap
			for(PxU32 i=0;i<mNbBuildVertices;i++)
			{
				PxU32 OldXRef;

						if(vcode==VCODE_VERTEX)	{ OldXRef = mVertexArray[i].VRef; mVertexArray[i].VRef = rc.CrossRef[OldXRef]; }
				else	if(vcode==VCODE_UVW)	{ OldXRef = mVertexArray[i].TRef; mVertexArray[i].TRef = rc.CrossRef[OldXRef]; }
				else	if(vcode==VCODE_COLOR)	{ OldXRef = mVertexArray[i].CRef; mVertexArray[i].CRef = rc.CrossRef[OldXRef]; }
			}

			// Create the final list of vertices
			PxVec3* NewXVerts = (PxVec3*)PX_ALLOC(sizeof(PxVec3) * rc.NbRVerts, PX_DEBUG_EXP("PxVec3"));
			//PxVec3* NewXVerts = new PxVec3[rc.NbRVerts];
			PxMemCopy(NewXVerts, rc.RVerts, rc.NbRVerts*sizeof(PxVec3));
			nbxverts = rc.NbRVerts;

			// Replace old list
			PX_FREE_AND_RESET(xverts);
			xverts = NewXVerts;

			if(vcode==VCODE_VERTEX)
			{
				// Here, we may have to face a final little thing........and yes, I actually encountered the problem:
				// Original face is: 0 1 2, not detected as degenerate at first.
				// But vertices 1 and 2 are exactly the same, so we detect it, and the remap above gives 0 1 1.
				// Hence, we must test for degenerate triangles *again*.
				// Note that we possibly could have to look for unused vertices again as well (e.g. 2 in our example)...... not done here.
				// A quick fix could be to keep track of the valencies in this class as well.
				Ps::Array<PxU32> GoodFaces;
				for(PxU32 i=0;i<mNbAvailableFaces;i++)
				{
					// Get refs
					const PxU32 Ref0 = mFacesArray[i].Ref[0];
					const PxU32 Ref1 = mFacesArray[i].Ref[1];
					const PxU32 Ref2 = mFacesArray[i].Ref[2];

					// Test against the geometry
					if(!(mVertexArray[Ref0].VRef==mVertexArray[Ref1].VRef
						||mVertexArray[Ref0].VRef==mVertexArray[Ref2].VRef
						||mVertexArray[Ref1].VRef==mVertexArray[Ref2].VRef))
					{
						// Face i is not degenerate
						GoodFaces.pushBack(i);
					}
				}

				// Keep good faces
				MBFace* Tmp = (MBFace*)PX_ALLOC(sizeof(MBFace) * mNbAvailableFaces, PX_DEBUG_EXP("MBFace"));
				for(PxU32 i=0;i<GoodFaces.size();i++)
					Tmp[i] = mFacesArray[GoodFaces[i]];

				// Replace faces
				//PxU32 NbDegenerate = mNbAvailableFaces - GoodFaces.size();
				PX_FREE_AND_RESET(mFacesArray);
				mFacesArray = Tmp;
				mNbAvailableFaces = GoodFaces.size();

				// Log file
//				if(NbDegenerate)	Log(_F("MeshBuilder: found %d degenerate faces after reduction.\n", NbDegenerate));
			}
		}
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Rebuilds a clean geometry.
 *
 *	On output:
 *				mNbBuildVertices	is updated with the optimized number of vertices, supposed to be smaller than before the call.
 *									Before the call mNbBuildVertices = mNbAvailableFaces * 3.
 *				mVertexArray		is updated as well, with mNbBuildVertices vertices.
 *				mFacesArray			is updated with new references.
 *	\return		true if success.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool MeshBuilder2::OptimizeGeometry()
{
	// Ok, I'm not dealing with floating-point coordinates, I'm dealing with references.
	// So what? I still can treat them as a vertex cloud.
	// ### This won't be possible anymore with a second set of UVs!
	PX_ASSERT(sizeof(MBVertex)==sizeof(PxVec3));	// ...
	ReducedVertexCloud	Reducer((PxVec3*)mVertexArray, mNbBuildVertices);
	REDUCEDCLOUD rc;
	Reducer.Reduce(&rc);
	const PxU32* XRef = rc.CrossRef;
	PxU32 NbReduced = rc.NbRVerts;
	const void* Reduced = rc.RVerts;

	// Remap topology
	for(PxU32 i=0;i<mNbAvailableFaces;i++)
	{
		mFacesArray[i].Ref[0] = XRef[mFacesArray[i].Ref[0]];
		mFacesArray[i].Ref[1] = XRef[mFacesArray[i].Ref[1]];
		mFacesArray[i].Ref[2] = XRef[mFacesArray[i].Ref[2]];
	}

	// Replace vertex cloud
	PX_DELETE_POD(mVertexArray);			// Previous buffer contained NbFaces*3 vertices, which was the maximum possible
	mVertexArray = PX_NEW(MBVertex)[NbReduced];
	PxMemCopy(mVertexArray, Reduced, NbReduced*sizeof(MBVertex));

	mNbBuildVertices = NbReduced;

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Computes normals. Smoothing groups are taken into account for the final calculation.
 *	\return		true if success.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool MeshBuilder2::ComputeNormals()
{
	// Does nothing if user didn't ask for normals.
	if(mComputeFNorm || mComputeVNorm)
	{
		// Little checkings
		if(!mNbVerts || !mNbBuildVertices || !mVertexArray)	return false;

		// Compute face-normals
		MBFace* f = mFacesArray;	if(!f)	return false;
		const PxVec3* v = mVerts;	if(!v)	return false;
		for(PxU32 i=0;i<mNbAvailableFaces;i++)
		{
			PxU32 Ref0 = mVertexArray[f[i].Ref[0]].VRef;
			PxU32 Ref1 = mVertexArray[f[i].Ref[1]].VRef;
			PxU32 Ref2 = mVertexArray[f[i].Ref[2]].VRef;

			PxVec3 Edge0 = v[Ref2] - v[Ref1];
			PxVec3 Edge1 = v[Ref0] - v[Ref1];
			f[i].Normal = Edge0.cross(Edge1);

			f[i].Normal.normalize();

			if(mComputeFNorm)	mArrayFNormals.pushBack(f[i].Normal);
		}

		// Create tables needed to easily compute vertex-normals according to smoothing groups
		{
			// We need to compute for each vertex the list of faces sharing that vertex
			mFCounts	= PX_NEW(PxU32)[mNbVerts];
			mVOffsets	= PX_NEW(PxU32)[mNbVerts];
			// Init ram
			PxMemZero(mFCounts,	mNbVerts * sizeof(PxU32));
			PxMemZero(mVOffsets,	mNbVerts * sizeof(PxU32));

			// First pass to compute counts
			for(PxU32 i=0;i<mNbAvailableFaces;i++)
			{
				mFCounts[mVertexArray[f[i].Ref[0]].VRef]++;
				mFCounts[mVertexArray[f[i].Ref[1]].VRef]++;
				mFCounts[mVertexArray[f[i].Ref[2]].VRef]++;
			}

			// Create VOffsets
			for(PxU32 i=1;i<mNbVerts;i++) mVOffsets[i] = mVOffsets[i-1] + mFCounts[i-1];

			// Total #referenced faces is mNbAvailableFaces*3.
			mVertexToFaces = PX_NEW(PxU32)[mNbAvailableFaces * 3];

			// Create list
			for(PxU32 i=0;i<mNbAvailableFaces;i++)
			{
				PxU32 Ref0 = mVertexArray[f[i].Ref[0]].VRef;
				PxU32 Ref1 = mVertexArray[f[i].Ref[1]].VRef;
				PxU32 Ref2 = mVertexArray[f[i].Ref[2]].VRef;
				mVertexToFaces[mVOffsets[Ref0]++] = i;
				mVertexToFaces[mVOffsets[Ref1]++] = i;
				mVertexToFaces[mVOffsets[Ref2]++] = i;
			}

			// Recreate VOffsets [needed in StoreVertex()]
			mVOffsets[0] = 0;
			for(PxU32 i=1;i<mNbVerts;i++) mVOffsets[i] = mVOffsets[i-1] + mFCounts[i-1];

			// The table we just computed is valid for current mesh description.
			// Particularly, mVertexToFaces is valid as long as faces are listed in the
			// original order, used to compute the table. This order will further be cut
			// to pieces by MeshBuilder2 (that's what it's been designed for!). Hence,
			// we'll need a cross-references table to fix mVertexToFaces before leaving
			// MeshBuilder2. This table will be filled on exporting triangles.
			//
			// Another solution would've been to compute mVertexToFaces after mesh processing.
			// Unfortunately mVertexToFaces is needed in StoreVertex to compute vertex-normals,
			// then it would've been difficult to compute it later. We could have (and actually
			// used to do so) used another way of computing vertex-normals, but it needed much
			// more ram, and the code was sloppy.
		}
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Stores X-Vertices into export arrays. (X = Geo, Texture, colors)
 *	X-Vertices are indexed vertices.
 *	\return		true if success.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool MeshBuilder2::SaveXVertices()
{
	// We can output two different things, according to what the user wants:
	// - indexed vertices
	// - non-indexed vertices
	//
	// Within the class, we deal with indexed vertices. Hence, when that's what the user wants in output, we just have
	// to dump our list of optimized vertices, and we're done.
	//
	// This is what we do here.
	//
	// Actual indices are saved in another place (where we save the non-indexed vertices)

	// 1) Save indexed geometry
	if(mVerts && mIndexedGeo)
	{
		for(PxU32 i=0;i<mNbVerts;i++)	mArrayVerts.pushBack(mVerts[i]);
	}

	// 2) Save UV(W) mappings
	if(mTVerts && mIndexedUVW)
	{
		for(PxU32 i=0;i<mNbTVerts;i++)
		{
						mArrayTVerts.pushBack(mTVerts[i].x);
						mArrayTVerts.pushBack(mTVerts[i].y);
			if(mUseW)	mArrayTVerts.pushBack(mTVerts[i].z);
		}
	}

	// 3) Save vertex colors
	if(mCVerts && mIndexedColors)
	{
		for(PxU32 i=0;i<mNbCVerts;i++)	mArrayCVerts.pushBack(mCVerts[i]);
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Groups faces by render properties. Each group of faces sharing rendering properties is a submesh.
 *	\return		true if success.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool MeshBuilder2::ComputeSubmeshes()
{
	PxU32* HelpList		= PX_NEW_TEMP(PxU32)[mNbAvailableFaces];
	PxU32* MatID		= PX_NEW_TEMP(PxU32)[mNbAvailableFaces];
	PxU32* SmGrps		= PX_NEW_TEMP(PxU32)[mNbAvailableFaces];

	if(!HelpList || !MatID || !SmGrps)
	{
		PX_DELETE_POD(SmGrps);
		PX_DELETE_POD(MatID);
		PX_DELETE_POD(HelpList);
		return false;
	}

	// Those lists are needed for sorting
	for(PxU32 i=0;i<mNbAvailableFaces;i++)
	{
		MatID[i]	= mFacesArray[i].MatID;
		SmGrps[i]	= mFacesArray[i].SmGroup;
	}

	// Sort faces by properties. The material ID is the most important sort key, since it leads to
	// SetMaterial and SetTexture calls. Smoothing groups only lead to duplicated vertices.
	RadixSortBuffered Radix;
	const PxU32* Sorted = Radix.Sort(SmGrps, mNbAvailableFaces, RADIX_UNSIGNED).Sort(MatID, mNbAvailableFaces, RADIX_UNSIGNED).GetRanks();

	// Cut sorted list into coherent meshes
	PxU32 Index = Sorted[0];
	PxU32 CurrentMatID	= MatID[Index];
	PxU32 CurrentSmGrp	= SmGrps[Index];
	PxU32 NbF = 0, NbSubmesh = 0, Total = 0;

	// Loop through faces
	for(PxU32 i=0;i<mNbAvailableFaces;i++)
	{
		// Read sorted face index
		Index = Sorted[i];
		// Look for all faces sharing the same properties
		if(MatID[Index]==CurrentMatID && SmGrps[Index]==CurrentSmGrp)
		{
			// One more entry found => store face number in HelpList
			HelpList[NbF++] = Index;
		}
		else
		{
			// ...until a new key is found
			Total+=BuildTrilist(HelpList, NbF, CurrentMatID, CurrentSmGrp);
			NbSubmesh++;

			// Update current properties
			CurrentMatID	= MatID[Index];
			CurrentSmGrp	= SmGrps[Index];

			// Reset HelpList
			HelpList[0] = Index;
			NbF = 1;
		}
		// ...and loop again
	}
	// Build last mesh
	Total+=BuildTrilist(HelpList, NbF, CurrentMatID, CurrentSmGrp);
	NbSubmesh++;

	// Here, Total = total #faces

	// Free local ram
	PX_DELETE_POD(SmGrps);
	PX_DELETE_POD(MatID);
	PX_DELETE_POD(HelpList);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Builds a triangle list. The input list contains faces whose rendering properties are the same (Material ID, smoothing groups)
 *	In other words, involved vertices have a single normal, and the bunch of faces is what we call a submesh.
 *	\param		list		[in] Faces to analyze (contains indices into mFacesArray)
 *	\param		nb_faces	[in] Number of faces in the list
 *	\param		mat_id		[in] Common material ID for all those faces
 *	\param		sm_grp		[in] Common smoothing groups for all those faces
 *	\return		the number of faces.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PxU32 MeshBuilder2::BuildTrilist(const PxU32* list, PxU32 nb_faces, PxI32 mat_id, PxU32 sm_grp)
{
	// Checkings
	if(!mFacesArray)	return 0;

	// Save mesh properties (Material ID, Smoothing Groups)
	mArraySubmeshProperties.pushBack(PxU32(mat_id));
	mArraySubmeshProperties.pushBack(sm_grp);

	// Save mesh datas
	Ps::Array<PxU32> ArrayWorkList;	// Will contain the rebuilt vertex data
	PxU32 NbVertsForSubmesh = RebuildVertexData(list, nb_faces, ArrayWorkList);
	const PxU32* tmp = ArrayWorkList.begin();
	// tmp is indexed by mFacesArray[].NewRef[]
	// tmp contains indexed indices, and now we must output real values.

	// Loop through all relevant vertices
	for(PxU32 i=0;i<NbVertsForSubmesh;i++)
	{
		PxU32 v1 = *tmp++;		// Vertex index
		PxU32 tv1 = *tmp++;	// UV index
		PxU32 cv1 = *tmp++;	// Color index
		PxU32 grp = *tmp++;	// Smoothing groups
		// Save UVW mappings
		if(mTVerts)
		{
			// UVW mappings can be indexed or not
			if(mIndexedUVW)	mArrayTVertsRefs.pushBack(tv1);
			else
			{
				mArrayTVerts.pushBack(mTVerts[tv1].x);
				mArrayTVerts.pushBack(mTVerts[tv1].y);
				if(mUseW)	mArrayTVerts.pushBack(mTVerts[tv1].z);
			}
		}

		// Save vertex-color
		if(mCVerts)
		{
			// Vertex-colors can be indexed or not
			if(mIndexedColors)	mArrayColorRefs.pushBack(cv1);
			else	mArrayCVerts.pushBack(mCVerts[cv1]);
		}

		// Save vertex-normal
		if(mComputeVNorm)
		{
			PxVec3 VertexNormal(0.0f, 0.0f, 0.0f);	// Vertex-normal we're about to compute
			PxU32 NbF = 0;							// Number of faces involved in the calculation, for current smoothing group

			// Leave room for the number of faces involved in the computation of current vertex's normal
			PxU32 SavedNb=0;
			if(mComputeNormInfo)
			{
				SavedNb = mArrayNormalInfo.size();
				mArrayNormalInfo.pushBack(0);
			}

			// Loop through faces sharing this vertex
			for(PxU32 j=0;j<mFCounts[v1];j++)
			{
				// Look for faces belonging to the same smoothing groups.
				PxU32 FaceIndex = mVertexToFaces[mVOffsets[v1]+j];	// Index in mFacesArray
				if(mFacesArray[FaceIndex].SmGroup & grp)
				{
					// 3.6: we weight the face normal with the angle formed by the two edges at current vertex
					// The face normal has already been normalized here. We need to know what edges are involved for current vertex.
					if(mWeightNormalWithAngles)
					{
						PxU32 Ref[3];
						Ref[0] = mVertexArray[mFacesArray[FaceIndex].Ref[0]].VRef;
						Ref[1] = mVertexArray[mFacesArray[FaceIndex].Ref[1]].VRef;
						Ref[2] = mVertexArray[mFacesArray[FaceIndex].Ref[2]].VRef;
						PxU32 e0=0,e2=0;
						if(v1==Ref[0])
						{
							e0 = 2;
							e2 = 1;
						}
						else if(v1==Ref[1])
						{
							e0 = 2;
							e2 = 0;
						}
						else if(v1==Ref[2])
						{
							e0 = 0;
							e2 = 1;
						}
						else
						{
							PX_ASSERT(0);
						}
						const PxVec3 Edge0 = mVerts[Ref[e0]] - mVerts[v1];
						const PxVec3 Edge1 = mVerts[Ref[e2]] - mVerts[v1];

						// Take face-normal contribution into account
						VertexNormal += mFacesArray[FaceIndex].Normal * Ps::angle(Edge0, Edge1);
					}
					else
					{
						// Take face-normal contribution into account
						VertexNormal += mFacesArray[FaceIndex].Normal;
					}
					// That's one more face involved in the calculation
					NbF++;
					// If user wants to further recompute vertex-normals for dynamic meshes,
					// he'll need NbF and involved indexed faces (FaceIndex = mVertexToFaces[mVOffsets[v1]+j])
					if(mComputeNormInfo)	mArrayNormalInfo.pushBack(FaceIndex);
				}
				// else the face contains the vertex indeed, but doesn't belong to current smoothing groups
			}

			// Get initial location back and save the number of faces
			if(mComputeNormInfo)
			{
				PxU32* Data = mArrayNormalInfo.begin();
				Data[SavedNb] = NbF;
				mNbNorm++;	// Count one more normal
			}
			// Normalize vertex-normal...
			VertexNormal.normalize();
			// ...and save it
			mArrayVNormals.pushBack(VertexNormal);
		}

		// Save geometry
		if(mVerts)
		{
			// Geometry can be indexed or not
			if(mIndexedGeo)	mArrayVertsRefs.pushBack(v1);
			else			mArrayVerts.pushBack(mVerts[v1]);
		}
	}

	// Save topology
	mArrayTopoSize.pushBack(nb_faces);
	for(PxU32 i=0;i<nb_faces;i++)
	{
		const PxU32 Fnb = list[i];	// Index in mFacesArray
		mArrayTopology.pushBack(mFacesArray[Fnb].NewRef[0]);
		mArrayTopology.pushBack(mFacesArray[Fnb].NewRef[1]);
		mArrayTopology.pushBack(mFacesArray[Fnb].NewRef[2]);
		// Build cross-references faces-list
		if(mOutToIn)	mOutToIn[mCrossIndex++] = Fnb;	// i.e. mOutToIn maps the new list to the original one
	}

	// Store #substrips (0 for trilists, stored for output coherence)
	mArraySubmeshProperties.pushBack(0);

	return nb_faces;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Rebuilds and reorganizes data for a coherent submesh.
 *	\param		faces		[in] The faces to 'rebuild'
 *	\param		nb_faces	[in] Number of faces in *faces
 *	\param		array		[out] A container to store the results
 *	\return		number of vertices involved in this submesh
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PxU32 MeshBuilder2::RebuildVertexData(const PxU32* faces, PxU32 nb_faces, Ps::Array<PxU32>& array)
{
	#define MESHBUILDER_REF_NOT_DONE	-1		//!< Default cross reference

	struct LocalInfo
	{
		MBVertex*			VertexArray;
		MBFace*				FaceArray;

		PxU32*				CrossList;
		PxU32				Grp;
		PxU32				Fnb;
		Ps::Array<PxU32>*	DestArray;
	};

	struct Local
	{
		static void StoreVertex(const LocalInfo& info, PxU32 ref, PxU32& new_verts_index, PxU32 id)
		{
			// Was the vertex already converted?
			if(info.CrossList[ref]==MESHBUILDER_REF_NOT_DONE)
			{
				// Catch original references
				PxU32 v1	= info.VertexArray[ref].VRef;
				PxU32 tv1	= info.VertexArray[ref].TRef;
				PxU32 cv1	= info.VertexArray[ref].CRef;

				// Save relevant values
				info.DestArray->pushBack(v1);
				info.DestArray->pushBack(tv1);
				info.DestArray->pushBack(cv1);
				info.DestArray->pushBack(info.Grp);
				// Update
				info.FaceArray[info.Fnb].NewRef[id]	= new_verts_index;
				// Cross list
				info.CrossList[ref]	= new_verts_index++;
			}
			else info.FaceArray[info.Fnb].NewRef[id] = info.CrossList[ref];
		}
	};

	// Get some bytes for the cross-list
	PxU32* CrossList = PX_NEW_TEMP(PxU32)[mNbBuildVertices];
	PX_ASSERT(CrossList);
	// Initialize cross list
//	StoreDwords(CrossList, mNbBuildVertices, MESHBUILDER_REF_NOT_DONE);
	memset(CrossList, 0xff, mNbBuildVertices*sizeof(PxU32));

//	for(PxU32 i=0;i<mNbBuildVertices;i++)	CrossList[i] = MESHBUILDER_REF_NOT_DONE;

	// Rebuild
	if(mRelativeIndices)	mNewVertsIndex = 0;		// First vertex index is 0 (relative indexing) or the last one (absolute indexing)
	PxU32 NbVerts = mNewVertsIndex;				// 0 or last index. Keep track of that for the final count.

	// Create a local info structure
	LocalInfo LI;
	LI.VertexArray	= mVertexArray;
	LI.FaceArray	= mFacesArray;
	LI.CrossList	= CrossList;
	LI.DestArray	= &array;

	// Loop through faces
	for(PxU32 i=0;i<nb_faces;i++)
	{
		// Get the index of the face to convert
		LI.Fnb = faces[i];
		// Get the smoothing groups, used to compute the normals
		LI.Grp = mFacesArray[LI.Fnb].SmGroup;

		// Get the original references
		PxU32 Ref0 = mFacesArray[LI.Fnb].Ref[0];
		PxU32 Ref1 = mFacesArray[LI.Fnb].Ref[1];
		PxU32 Ref2 = mFacesArray[LI.Fnb].Ref[2];

		// Convert the three vertices
		Local::StoreVertex(LI, Ref0, mNewVertsIndex, 0);
		Local::StoreVertex(LI, Ref1, mNewVertsIndex, 1);
		Local::StoreVertex(LI, Ref2, mNewVertsIndex, 2);
	}

	// Free cross list ram
	PX_DELETE_POD(CrossList);

	// Store #vertex-data saved. mNewVertsIndex is the current index, NbVerts was the index value when entering the method.
	PxU32 NbVertsForThisSubmesh = mNewVertsIndex - NbVerts;

	// Save some useful values
	mArraySubmeshProperties.pushBack(nb_faces);
	mArraySubmeshProperties.pushBack(NbVertsForThisSubmesh);

	// Here, the array has been filled with indexed vertex data.
	// The relevant topology is stored in the NewRef field of all input faces, in mFacesArray.

	return NbVertsForThisSubmesh;
}
