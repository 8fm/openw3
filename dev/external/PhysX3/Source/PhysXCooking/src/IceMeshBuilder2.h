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

#ifndef PX_MESHBUILDER2_H
#define PX_MESHBUILDER2_H

#ifndef PX_COOKING
#error Do not include anymore!
#endif

namespace physx
{
	//! X-vertices
	enum VertexCode
	{
		VCODE_VERTEX		= (1<<0),			//!< Geometry
		VCODE_UVW			= (1<<1),			//!< UV-mappings
		VCODE_COLOR			= (1<<2),			//!< Vertex colors
		VCODE_FORCEDWORD	= 0x7fffffff
	};

	//! Initialization structure.
	struct MBCreate
	{
						MBCreate()				{ PxMemZero(this, sizeof(*this));	}

		PxU32			NbVerts;				//!< Number of vertices in the mesh
		PxU32			NbFaces;				//!< Number of faces in the mesh
		PxU32			NbTVerts;				//!< Number of texture-vertices in the mesh
		PxU32			NbCVerts;				//!< Number of vertex-colors in the mesh
		const PxVec3*	Verts;					//!< Verts in MAX are Point3 == PxVec3
		const PxVec3*	TVerts;					//!< TVerts in MAX are UVVert == Point3 == PxVec3
		const PxVec3*	CVerts;					//!< CVerts in MAX are VertColor == Point3 == PxVec3
//		PxU32			OptimizeFlags;			//!< A combination of VertexCode flags, to enable/disable passes

		bool			KillZeroAreaFaces;		//!< Look for zero-area faces and delete them
		bool			UseW;					//!< Use W coord in UVW mappings, else just deals with standard U & V
		bool			ComputeVNorm;			//!< Compute vertex-normals, else leave it to the user
		bool			ComputeFNorm;			//!< Compute face-normals, else leave it to the user
		bool			ComputeNormInfo;		//!< Compute normal-information table (used to recompute'm dynamically according to smoothing groups)
		bool			IndexedGeo;				//!< Ask for indexed geometry
		bool			IndexedUVW;				//!< Ask for indexed UVW
		bool			IndexedColors;			//!< Ask for indexed vertex-colors
		bool			RelativeIndices;		//!< Reset indices from one submesh to another, else use absolute indices
		bool			IsSkin;					//!< True for skins => MeshBuilder does not touch the list of vertices
		bool			WeightNormalWithAngles;	//!< Take angles between edges into account in normal computation
		bool			OptimizeVertexList;		//!< Optimize vertex list for cache coherence
	};

	// Vertex alpha support:
	// I expect colors as points (RGB floating-point values in CVerts) since that's what I get out of MAX.
	// So at first it seems vertex alpha is not supported. Actually it is, here's how:
	//
	// 1) First method:
	// - convert your colors to RGBA *PxU32s* (since that's what will eventually be in your vertex buffers there's no real loss of precision...)
	// - put this binary data in the Red channel as if it was a float (but don't do the int-to-float conversion of course)
	// - clear the green and blue channels to allow MeshBuilder to reduce the vertex cloud anyway
	// - and that's it, you just have to depack the colors at the other side of the pipeline if needed.
	//
	// It works since I actually only work on the binary representation of the float values without really doing any computation.
	// Here I once again appreciate the power of radix-sort ! With a standard comparison-based sort, as the name suggests, real FPU comparisons
	// would have been made while reducing the cloud, and it probably would have failed since some "floats" would've been read as NaNs or
	// other non-valid float representations..............................................
	//
	// 2) Second method:
	// - Store the alpha in the TVerts list, in the usually unused W.

	struct MBFaceData
	{
						MBFaceData() : Index(0), MaterialID(PX_INVALID_U32), SmoothingGroups(1), VRefs(NULL), TRefs(NULL), CRefs(NULL),
						CCW(false)	{}

		PxU32			Index;				//!< Face index in the original (user-controlled) list
		PxI32			MaterialID;			//!< Material ID for this face
		PxU32			SmoothingGroups;	//!< Smoothing groups for this face
		const PxU32*	VRefs;				//!< Vertex references (3 PxU32s)
		const PxU32*	TRefs;				//!< Texture-vertex references (3 PxU32s)
		const PxU32*	CRefs;				//!< Vertex-colors references (3 PxU32s)
		bool			CCW;				//!< True if vertex references must be swapped
	};

	class MeshBuilder2
	{
		//! A custom vertex holding 4 references.
		struct MBVertex
		{
			PxU32		VRef;				//!< Ref into an array of vertices (mVerts)
			PxU32		TRef;				//!< Ref into an array of UVW mappings (mTVerts)
			PxU32		CRef;				//!< Ref into an array of vertex-colors (mCVerts)
		};

		//! A custom face holding all the face-related properties.
		struct MBFace
		{
			PxU32		NewRef[3];			//!< Final "vertex" references [==refs to one geometry/UVW/Color/Normal component/vertex]
			PxU32		Ref[3];				//!< Ref into an array of MBVertices
			PxI32		MatID;				//!< Material ID
			PxU32		SmGroup;			//!< Smoothing group
			PxVec3		Normal;				//!< Face normal vector
			PxU32		Index;				//!< Original face index from AddFace().
		};

		public:
									MeshBuilder2();
									~MeshBuilder2();
		// Creation methods
				bool				Init(const MBCreate& create);
				bool				AddFace(const MBFaceData& face);
		// Build method
				bool				Build(MBResult& result);
		private:
		// Custom arrays / containers
				Ps::Array<PxU32>	mArrayTopology;					//!< Array to store triangle strip runs / face data => topology
				Ps::Array<PxU32>	mArrayTopoSize;					//!< Array to store triangle strip lengths / nb of faces => topology size
				Ps::Array<PxU32>	mArrayVertsRefs;				//!< Array to store vertices references.
				Ps::Array<PxU32>	mArrayTVertsRefs;				//!< Array to store texture-vertices references.
				Ps::Array<PxU32>	mArrayColorRefs;				//!< Array to store vertex-colors references.
				Ps::Array<PxVec3>	mArrayVerts;					//!< Array to store vertices.
				Ps::Array<PxF32>	mArrayTVerts;					//!< Array to store mapping coordinates. [Container only for UVs without W]
				Ps::Array<PxVec3>	mArrayCVerts;					//!< Array to store vertex-colors.
				Ps::Array<PxVec3>	mArrayVNormals;					//!< Array to store vertex normals.
				Ps::Array<PxVec3>	mArrayFNormals;					//!< Array to store face normals.
				Ps::Array<PxU32>	mArrayNormalInfo;				//!< Array to store information about normals.
				Ps::Array<PxU32>	mArraySubmeshProperties;		//!< Array to store submesh properties: material ID, smoothing groups, #substrips, #vertex-data.
			Ps::Array<MBMatInfo>	mMaterialInfo;					//!< Array to derive material informations
				PxU32				mNbFaces;						//!< Maximum (expected) number of faces (provided by user)
				PxU32				mNbVerts;						//!< Number of vertices in the original mesh (provided by user)
				PxU32				mNbTVerts;						//!< Number of mapping coordinates in the original mesh (provided by user)
				PxU32				mNbCVerts;						//!< Number of vertex colors in the original mesh (provided by user)
				PxU32				mNbAvailableFaces;				//!< Number of faces in database (could be <NbFaces if zero-area faces are skipped)
				PxU32				mNbBuildVertices;				//!< TO BE DOCUMENTED
				PxU32				mNewVertsIndex;					//!< TO BE DOCUMENTED

				PxVec3*				mVerts;							//!< Original/reduced list of vertices
				PxVec3*				mTVerts;						//!< Original/reduced list of texture-vertices
				PxVec3*				mCVerts;						//!< Original/reduced list of vertex-colors
				MBFace*				mFacesArray;					//!< The original list of faces.
				MBVertex*			mVertexArray;					//!< The list of rebuilt vertices.
		// Faces permutation
				PxU32*				mOutToIn;						//!< New (consolidated) list to old list mapping
				PxU32				mCrossIndex;					//!< Running face index used to build mOutToIn
		// Normals computation / smoothing groups
				PxU32*				mFCounts;						//!< For each vertex, the number of faces sharing that vertex.
				PxU32*				mVOffsets;						//!< Radix-style offsets of mFCounts.
				PxU32*				mVertexToFaces;					//!< List of faces sharing each vertex.
				PxU32				mNbNorm;						//!< Number of normals currently computed in mArrayNormalInfo
		// Build settings
//				PxU32				mOptimizeFlags;					//!< TO BE DOCUMENTED
				bool				mKillZeroAreaFaces;				//!< true if degenerate triangles must be skipped (default)
				bool				mUseW;							//!< true if W coord must be used, else false (default)
				bool				mComputeVNorm;					//!< true if vertex-normals must be computed, else false (default)
				bool				mComputeFNorm;					//!< true if face-normals must be computed, else false (default)
				bool				mComputeNormInfo;				//!< true if normal information must be computed, else false (default)
				bool				mIndexedGeo;					//!< true if geometry must be exported as indexed (default), else false
				bool				mIndexedUVW;					//!< true if UVW must be exported as indexed, else false (default)
				bool				mIndexedColors;					//!< true if vertex-colors must be exported as indexed, else false (default)
				bool				mRelativeIndices;				//!< true if indices must be reset for each submesh, else false (default)
				bool				mIsSkin;						//!< true if the input mesh is a skin, else false (default)
				bool				mWeightNormalWithAngles;		//!< true if angles between edges must be used as weights in normal computation
				bool				mOptimizeVertexList;			//!< true if consolidated vertex list must further be optimized for cache coherence
		// Internal methods
				MeshBuilder2&		FreeUsedRam();
				bool				FixNULLSmoothingGroups();
				bool				OptimizeTopology();
				bool				OptimizeXMappings(PxU32& nbxverts, PxVec3*& xverts, VertexCode vcode);
				bool				OptimizeGeometry();
				bool				ComputeNormals();
				bool				SaveXVertices();
				bool				ComputeSubmeshes();
				PxU32				BuildTrilist(const PxU32* list, PxU32 nb_faces, PxI32 mat_id, PxU32 sm_grp);
				PxU32				RebuildVertexData(const PxU32* faces, PxU32 nb_faces, Ps::Array<PxU32>& array);
	};
}

#endif // PX_MESHBUILDER2_H
