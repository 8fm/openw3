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

#ifndef OPC_MESHINTERFACE_H
#define OPC_MESHINTERFACE_H

#include "PsUserAllocated.h"
#include "GuTriangle32.h"
#include "Opcode.h"
#include "CmMemFetch.h"
#include "PxMetaData.h"

namespace physx
{
	using namespace Cm;

namespace Gu
{
	struct VertexPointers
	{
		const PxVec3* vertex[3];
	};

	class PX_PHYSX_COMMON_API MeshInterface : public Ps::UserAllocated
	{
		public:
		static			void				getBinaryMetaData(PxOutputStream& stream);
											MeshInterface();
											~MeshInterface();
		// Common settings
		PX_FORCE_INLINE	PxU32				has16BitIndices()	const	{ return mHas16BitIndices; }	//TODO: use a flag instead later.
		PX_FORCE_INLINE	PxU32				GetNbTriangles()	const	{ return mNbTris;	}
		PX_FORCE_INLINE	PxU32				GetNbVertices()		const	{ return mNbVerts;	}
		PX_FORCE_INLINE	void				SetNbTriangles(PxU32 nb)	{ mNbTris = nb;		}
		PX_FORCE_INLINE	void				SetNbVertices(PxU32 nb)		{ mNbVerts = nb;	}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Pointers control: setups object pointers. Must provide access to faces and vertices for a given object.
		 *	\param		tris	[in] pointer to triangles
		 *	\param		verts	[in] pointer to vertices
		 *	\return		true if success
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
						bool				SetPointers(const void* tris, bool has16BitIndices, const PxVec3* verts);
		PX_FORCE_INLINE	const	void*		GetTris()			const	{ return mTris;			}
		PX_FORCE_INLINE	const	PxVec3*		GetVerts()			const	{ return mVerts;		}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Fetches a triangle given a triangle index.
		 *	\param		vp		[out] required triangle's vertex pointers
		 *	\param		index	[in] triangle index
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		template<class Triangle>	//Triangle is IndexedTriangle16 or IndexedTriangle32
		PX_FORCE_INLINE	void				GetTriangle(VertexPointers& vp, PxU32 index)	const
											{

												const Triangle& T = (reinterpret_cast<const Triangle *>(mTris))[index];
												vp.vertex[0] = &mVerts[T.v[0]];
												vp.vertex[1] = &mVerts[T.v[1]];
												vp.vertex[2] = &mVerts[T.v[2]];
											}

		//version with 16 vs 32 branch!
		PX_FORCE_INLINE	void				GetTriangle(VertexPointers& vp, PxU32 index)	const
											{
												if (has16BitIndices())
												{
	//												const IndexedTriangle16& T = (reinterpret_cast<const IndexedTriangle16 *>(mTris))[index];
													const Gu::TriangleT<PxU16>& T = (reinterpret_cast<const Gu::TriangleT<PxU16> *>(mTris))[index];
													vp.vertex[0] = &mVerts[T.v[0]];
													vp.vertex[1] = &mVerts[T.v[1]];
													vp.vertex[2] = &mVerts[T.v[2]];
												}
												else
												{
													const Gu::TriangleT<PxU32>& T = (reinterpret_cast<const Gu::TriangleT<PxU32> *>(mTris))[index];
													vp.vertex[0] = &mVerts[T.v[0]];
													vp.vertex[1] = &mVerts[T.v[1]];
													vp.vertex[2] = &mVerts[T.v[2]];
												}
											}

		struct IndexTriple16 { PxU16 p0, p1, p2; };
		struct IndexTriple32 { PxU32 p0, p1, p2; };

		static void PX_FORCE_INLINE prefetchTriangleVerts(PxU32 has16BitIndices, const void* indices, const PxVec3* verts, PxU32 triangleIndex)
		{
			PxU32 vref0, vref1, vref2;
			if(has16BitIndices)
			{
				const PxU16* indices16 = ((const PxU16*)indices) + triangleIndex * 3;
				vref0 = indices16[0];
				vref1 = indices16[1];
				vref2 = indices16[2];
			} 
			else 
			{ 
				const PxU32* indices32 = ((const PxU32*)indices) + triangleIndex * 3;
				vref0 = indices32[0];
				vref1 = indices32[1];
				vref2 = indices32[2];
			} 

			Ps::prefetchLine(verts, vref0*sizeof(PxVec3));
			Ps::prefetchLine(verts, vref1*sizeof(PxVec3));
			Ps::prefetchLine(verts, vref2*sizeof(PxVec3));
		}

		static void PX_FORCE_INLINE GetTriangleVerts(
			PxU32 has16BitIndices, MemFetchPtr mTris, MemFetchPtr mVerts, PxU32 TriangleIndex, PxVec3& v0, PxVec3& v1, PxVec3& v2)
		{
			MemFetchSmallBuffer buf0, buf1, buf2;
			PxU32 i0, i1, i2;
			if (has16BitIndices)
			{
				IndexTriple16* inds = memFetchAsync<IndexTriple16>(mTris+(sizeof(PxU16)*TriangleIndex*3), 5, buf0);
				memFetchWait(5);
				i0 = inds->p0; i1 = inds->p1; i2 = inds->p2;
			} 
			else 
			{ 
				IndexTriple32* inds = memFetchAsync<IndexTriple32>(mTris+(sizeof(PxU32)*TriangleIndex*3), 5, buf0);
				memFetchWait(5);
				i0 = inds->p0; i1 = inds->p1; i2 = inds->p2;
			} 

			PxVec3* v[3];
			v[0] = memFetchAsync<PxVec3>(mVerts+i0*12+0, 5, buf0);
			v[1] = memFetchAsync<PxVec3>(mVerts+i1*12+0, 5, buf1);
			v[2] = memFetchAsync<PxVec3>(mVerts+i2*12+0, 5, buf2);
			memFetchWait(5);
			v0 = *v[0]; v1 = *v[1]; v2 = *v[2];
		}

		static void PX_FORCE_INLINE GetTriangleVerts(
			PxU32 has16BitIndices, MemFetchPtr mTris, MemFetchPtr mVerts, PxU32 TriangleIndex, PxVec3& v0, PxVec3& v1, PxVec3& v2, PxU32& ind0, PxU32& ind1, PxU32& ind2)
		{
			MemFetchSmallBuffer buf0, buf1, buf2;
			PxU32 i0, i1, i2;
			if (has16BitIndices)
			{
				IndexTriple16* inds = memFetchAsync<IndexTriple16>(mTris+(sizeof(PxU16)*TriangleIndex*3), 5, buf0);
				memFetchWait(5);
				i0 = inds->p0; i1 = inds->p1; i2 = inds->p2;
			} 
			else 
			{ 
				IndexTriple32* inds = memFetchAsync<IndexTriple32>(mTris+(sizeof(PxU32)*TriangleIndex*3), 5, buf0);
				memFetchWait(5);
				i0 = inds->p0; i1 = inds->p1; i2 = inds->p2;
			} 

			PxVec3* v[3];
			v[0] = memFetchAsync<PxVec3>(mVerts+i0*12+0, 5, buf0);
			v[1] = memFetchAsync<PxVec3>(mVerts+i1*12+0, 5, buf1);
			v[2] = memFetchAsync<PxVec3>(mVerts+i2*12+0, 5, buf2);
			memFetchWait(5);
			v0 = *v[0]; v1 = *v[1]; v2 = *v[2];
			ind0 = i0;
			ind1 = i1;
			ind2 = i2;
		}


		template<int N> static void PX_FORCE_INLINE getTriangleVertsN(
			PxU32 has16BitIndices, MemFetchPtr mTris, MemFetchPtr mVerts,
			const PxU32 triIndices, PxU32 indexCount, PxVec3 output[N][3], PxU32 outputInds[N][3]
		)
		{
			MemFetchSmallBuffer buf0[N], buf1[N], buf2[N], buf3[N];

			PxVec3* v[N][3];
			PX_ASSERT(indexCount <= N);
			if (has16BitIndices)
			{
				IndexTriple16* inds[N];
				for (PxU32 i = 0; i < indexCount; i++)
					inds[i] = memFetchAsync<IndexTriple16>(mTris+(sizeof(PxU16)*(triIndices + i)*3), i/*dma tag*/, buf3[i]);

				for (PxU32 i = 0; i < indexCount; i++)
				{
					memFetchWait(i);
					v[i][0] = memFetchAsync<PxVec3>(mVerts+inds[i]->p0*12+0, i, buf0[i]);
					v[i][1] = memFetchAsync<PxVec3>(mVerts+inds[i]->p1*12+0, i, buf1[i]);
					v[i][2] = memFetchAsync<PxVec3>(mVerts+inds[i]->p2*12+0, i, buf2[i]);
					outputInds[i][0] = inds[i]->p0;
					outputInds[i][0] = inds[i]->p1;
					outputInds[i][0] = inds[i]->p2;
				}
			} 
			else 
			{ 
				IndexTriple32* inds[N];
				for (PxU32 i = 0; i < indexCount; i++)
					inds[i] = memFetchAsync<IndexTriple32>(mTris+(sizeof(PxU32)*(triIndices+i)*3), i/*dma tag*/, buf3[i]);

				for (PxU32 i = 0; i < indexCount; i++)
				{
					memFetchWait(i);
					v[i][0] = memFetchAsync<PxVec3>(mVerts+inds[i]->p0*12+0, i, buf0[i]);
					v[i][1] = memFetchAsync<PxVec3>(mVerts+inds[i]->p1*12+0, i, buf1[i]);
					v[i][2] = memFetchAsync<PxVec3>(mVerts+inds[i]->p2*12+0, i, buf2[i]);
					outputInds[i][0] = inds[i]->p0;
					outputInds[i][0] = inds[i]->p1;
					outputInds[i][0] = inds[i]->p2;
				}
			} 

			for (PxU32 i = 0; i < indexCount; i++)
			{
				memFetchWait(i);
				output[i][0] = *v[i][0];
				output[i][1] = *v[i][1];
				output[i][2] = *v[i][2];
			}
		}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Checks the mesh interface is valid, i.e. things have been setup correctly.
		 *	\return		true if valid
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					bool				IsValid()		const;

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Checks the mesh itself is valid.
		 *	Currently we only look for degenerate faces.
		 *	\return		number of degenerate faces
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					PxU32				CheckTopology()	const;
		public:
					PxU32				mNbTris;			//!< Number of triangles in the input model
					PxU32				mNbVerts;			//!< Number of vertices in the input model
		// User pointers
					const void*			mTris;				//!< Array of indexed triangles, 16 or 32 bit.
					const PxVec3*		mVerts;				//!< Array of vertices
					PxU32				mHas16BitIndices;	//!< Whether indices are 16 or 32 bits wide.  In cooking we are always using 32 bits, otherwise we could tell from the number of vertices.
	};
} // namespace Gu

}

#endif // OPC_MESHINTERFACE_H
