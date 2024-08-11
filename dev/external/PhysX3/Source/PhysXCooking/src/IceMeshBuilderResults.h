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

#ifndef PX_MESHBUILDERRESULTS_H
#define PX_MESHBUILDERRESULTS_H

#ifndef PX_COOKING
#error Do not include anymore!
#endif

namespace physx
{

	//! Submesh properties.
	struct MBSubmesh
	{
		PxI32			MatID;					//!< MaterialID for this submesh
		PxU32			SmGrp;					//!< Smoothing groups for this submesh
		PxU32			NbFaces;				//!< Number of faces in this submesh
		PxU32			NbVerts;				//!< Number of vertices in this submesh
		PxU32			NbSubstrips;			//!< Number of strips in this submesh
	};

	//! Material properties.
	struct MBMatInfo
	{
		PxI32			MatID;					//!< This material's ID
		PxU32			NbFaces;				//!< Number of faces having this material
		PxU32			NbVerts;				//!< Related number of exported vertices
		PxU32			NbSubmeshes;			//!< Number of related submeshes
	};

	//! The topology structure.
	struct MBTopology
	{
						MBTopology()			{ PxMemZero(this, sizeof(*this));	}

		PxU32			NbFaces;				//!< Total number of faces
		PxU32			NbSourceFaces;			//!< Number of faces in source mesh
		PxU32			NbSubmeshes;			//!< Total number of submeshes (e.g. 6 for the reference cube)
		PxU32*			VRefs;					//!< Vertex references (3 refs for each face) [WARNING: PxU32s since version 3.4]
		PxU32*			FacesInSubmesh;			//!< Number of faces in each submesh
		float*			Normals;				//!< Face normals
		MBSubmesh*		SubmeshProperties;		//!< NbSubmeshes structures
		PxU32*			Map;					//!< Maps new faces to old faces
	};

	//! The geometry structure.
	struct MBGeometry
	{
						MBGeometry()			{ PxMemZero(this, sizeof(*this));	}
		// Basic data
		PxU32			NbGeomPts;				//!< Number of vertices in the original mesh
		PxU32			NbTVerts;				//!< Number of mapping coordinates in the original mesh
		PxU32			NbColorVerts;			//!< Number of vertex-colors in the original mesh
		//
		PxU32			NbVerts;				//!< Number of vertices in the final mesh (some may have been duplicated) = sum of all NbVerts in MBSubmeshes
		// Indices
		PxU32*			VertsRefs;				//!< Vertex indices (only !=NULL if mIndexedGeo, else vertices are duplicated). Index in Verts.
		PxU32*			TVertsRefs;				//!< UVW indices (only !=NULL if mIndexedUVW, else UVWs are duplicated). Index in TVerts.
		PxU32*			ColorRefs;				//!< Vertex-color indices (only !=NULL if mIndexedColors, else colors are duplicated). Index in CVerts.
		// Vertex data
		float*			Verts;					//!< List of vertices, may be duplicated or not
		float*			TVerts;					//!< List of UV(W) mappings, may be duplicated or not.
		float*			CVerts;					//!< List of vertex colors, may be duplicated or not.
		// Normals
		float*			Normals;				//!< Vertex normals. Can't be indexed.
		PxU32			NormalInfoSize;			//!< Size of the NormalInfo field (in number of PxU32s, not in bytes)
		PxU32*			NormalInfo;				//!< Information used to rebuild normals in realtime. See below.
		//
		bool			UseW;					//!< True if TVerts uses W.
	};

	// More about NormalInfo:
	//
	// NormalInfo contains some extra information used to rebuild vertex-normals in realtime, by averaging
	// a number of face-normals. Each vertex-normal depends on a various number of face-normals. The exact
	// number depends on the mesh topology, but also depends on the smoothing groups.
	//
	// NormalInfo contains data to rebuild one normal/vertex, ie to rebuild NbVerts normals.
	// Format is, for each vertex:
	//		PxU32		Count				a number of faces
	//		PxU32		Ref0, Ref1,...		a list of Count face indices
	//
	// To rebuild vertex-normals in realtime you need to:
	// 1) Rebuild all face-normals (which is easy)
	// 2) For each vertex, add Count face-normals according to NormalInfo, then normalize the summed vector.
	//
	// Other techniques exist, of course.

	//! The materials structure.
	struct MBMaterials
	{
						MBMaterials() : NbMtls(0), MaterialInfo(NULL)	{}

		PxU32			NbMtls;					//!< Number of materials found.
		MBMatInfo*		MaterialInfo;			//!< One structure for each material.
	};

	//! Result structure.
	struct MBResult
	{
		MBTopology		Topology;				//!< Result topology.
		MBGeometry		Geometry;				//!< Result geometry
		MBMaterials		Materials;				//!< Result materials
	};

	//
	// Pseudo-code showing how to use the consolidation and the striper result structures:
	// mVB is a DX7 vertex buffer filled thanks to the MBGeometry structure.
	// mResult is the MBResult structure.
	// mStripRuns and mStripLengths are from the STRIPERRESULT structure.
	//
	//	// Get indices
	//	uword* VRefs = mResult->Topology.VRefs;	// NB: since version 3.4 you must actually convert to uwords first!
	//	// Make one API call / material
	//	for(i=0;i<mResult->Materials.NbMtls;i++)
	//	{
	//		// Select correct material
	//		PxU32 MaterialID = mResult->Materials.MaterialInfo[i].MatID;
	//		// Draw mesh
	//		if(mStripRuns)	renderer->DrawIndexedPrimitiveVB(PRIMTYPE_TRISTRIP, mVB, 0, mResult->Geometry.NbVerts, mStripRuns[i], mStripLengths[i]);
	//		else			renderer->DrawIndexedPrimitiveVB(PRIMTYPE_TRILIST, mVB, 0, mResult->Geometry.NbVerts, VRefs, mResult->Materials.MaterialInfo[i].NbFaces*3);
	//		// Update index pointer for trilists
	//		VRefs+=mResult->Materials.MaterialInfo[i].NbFaces*3;
	//	}
	//
}

#endif	// PX_MESHBUILDERRESULTS_H
