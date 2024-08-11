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


#ifndef __GRB_ENVENTS_H__
#define __GRB_ENVENTS_H__

#include "PsArray.h"
#include "PsUserAllocated.h"
#include "PxScene.h"
#include "PxRigidStatic.h"
#include "PxRigidDynamic.h"
#include "PxMaterial.h"
#include "PxConvexMesh.h"
#include "PxAggregate.h"
#include "PxBroadPhase.h"
#include "geometry/PxGeometry.h"
#include "geometry/PxSphereGeometry.h"
#include "geometry/PxBoxGeometry.h"
#include "geometry/PxCapsuleGeometry.h"
#include "geometry/PxConvexMeshGeometry.h"
#include "geometry/PxPlaneGeometry.h"
#include "geometry/PxHeightFieldGeometry.h"
#include "geometry/PxTriangleMeshGeometry.h"
#include "PxShape.h"
#include "GuConvexMesh.h"
#include "GuTriangleMesh.h"
#include "GuHeightField.h"

#if defined(PX_WINDOWS) && !defined(PX_WINMODERN)
#	define USE_GRB_INTEROP	1
#else
#	define USE_GRB_INTEROP	0
#endif
#define GRB_INTEROP_PTR	PxU64

namespace physx
{

#if (USE_GRB_INTEROP == 1)
#	define GRB_EVENT(scene, e, ...) \
	{ \
	NpScene * npScene = static_cast<NpScene*>(scene); \
	if (npScene) \
	{ \
		for (PxU32 eventStream = 0; eventStream < npScene->getNumEventStreams(); ++eventStream) \
			npScene->eventStreamSend(e(npScene->getEventStreamStackAlloc(eventStream), __VA_ARGS__), eventStream); \
	} \
}
#else
#	define GRB_EVENT(scene, e, ...)
#endif

#if (USE_GRB_INTEROP == 1)

namespace PxSceneEventDescs
{

class SystemHeapAlloc
{
public:

	void * allocate(size_t size, const char*, int)
	{
		return ::malloc(size);
	}

	void deallocate(void*)
	{
		//::free(ptr);
	}
};

template<class ParentAllocator>
class StackAllocatorT
{
	PX_NOCOPY(StackAllocatorT<ParentAllocator>);

public:
	StackAllocatorT(PxU32 pageSize, ParentAllocator &alloc): mAlloc(alloc), mPageSize(pageSize), mCurrPageByteSize(0)
	{
		mPages.pushBack(StackAllocatorPage(alloc.allocate(pageSize,  __FILE__, __LINE__), pageSize));
		mCurrPage = 0;
	}

	~StackAllocatorT()
	{
		release();
	}

	void release()
	{
		for(shdfnd::Array<StackAllocatorPage>::Iterator it = mPages.begin(), endit = mPages.end(); it != endit; ++it)
		{
			mAlloc.deallocate(it->ptr);
		}

		mPages.reset();
	}

	void grow(size_t size)
	{
		if(size < mPageSize)
		{
			size = mPageSize;
		}

		mPages.pushBack(StackAllocatorPage(mAlloc.allocate(size,  __FILE__, __LINE__), size));

		mCurrPage = mPages.size() - 1;
		mCurrPageByteSize = 0;
	}

	void * allocate(size_t size)
	{
		// alignment is required by some PhysX classes
		//const PxU32 alignment = 16;
		//size = size + alignment - 1;

		if(mCurrPageByteSize + size > mPages[mCurrPage].byteCapacity)
		{
			do
			{
				mCurrPage++; 
			}while(mCurrPage < mPages.size() && size > mPages[mCurrPage].byteCapacity);

			if(mCurrPage < mPages.size())
			{
				mCurrPageByteSize = 0;
			}
			else
			{
				grow(size);
			}
		}

		void *ret = reinterpret_cast<void *>(reinterpret_cast<PxU8 *>(mPages[mCurrPage].ptr) + mCurrPageByteSize); 
		mCurrPageByteSize += size;

//		return reinterpret_cast<void *>( reinterpret_cast<ptrdiff_t>(reinterpret_cast<PxU8 *>(ret) + alignment - 1) & ~(alignment - 1) );

		return ret;
	}

	void deallocate(void * ptr)
	{
	// do nothing
	}

	void reset()
	{
		mCurrPageByteSize = 0;
		mCurrPage = 0;
	}

protected:
	struct StackAllocatorPage
	{
		StackAllocatorPage() : ptr(0), byteCapacity(0)
		{
		}

		StackAllocatorPage(void *p, size_t s): ptr(p), byteCapacity(s)
		{
		}
	
		void *ptr;
		size_t byteCapacity;
	};

	shdfnd::Array<StackAllocatorPage> mPages;
	PxU32						mCurrPage;
	size_t						mCurrPageByteSize;
	const PxU32					mPageSize;
	ParentAllocator				& mAlloc;
};

typedef StackAllocatorT<SystemHeapAlloc> StackAllocator;

#define GRB_INTEROP_RIGID_STATIC_DESC	1
#define GRB_INTEROP_RIGID_DYNAMIC_DESC	2

struct MaterialDesc
{
	GRB_INTEROP_PTR			ptr;
	PxReal					dynamicFriction;				
	PxReal					staticFriction;					
	PxReal					restitution;					
	PxMaterialFlags			flags;							
	PxCombineMode::Enum		frictionCombineMode;
	PxCombineMode::Enum		restitutionCombineMode;
};

struct ShapeDesc
{
	GRB_INTEROP_PTR			ptr;
	PxFilterData			simFilterData;
	PxFilterData			queryFilterData;
	PxShapeFlags			flags;
	PxReal					contactOffset;
	PxReal					restOffset;
	bool					isExclusive;
	PxTransform				localPose;
	PxGeometryHolder		geometry;
	MaterialDesc *			materialDescs;
	PxU32					materialsCount;
};

struct RigidStaticDesc
{
	GRB_INTEROP_PTR			ptr;
	PxTransform				globalPose;
	ShapeDesc *				shapeDescs;
	char *					name;
	PxU32					shapesCount;
	PxActorFlags			actorFlags;
	PxDominanceGroup		dominanceGroup;
	PxClientID				clientID;
	PxU8					type;
};

struct RigidDynamicDesc : public RigidStaticDesc
{
	PxVec3					linearVelocity;
	PxVec3					angularVelocity;
	PxReal					linearDamping;
	PxReal					angularDamping;
	PxReal					contactReportThreshold;
	PxRigidBodyFlags		rigidBodyFlags;
};

struct AggregateDesc
{
	GRB_INTEROP_PTR			ptr;
	GRB_INTEROP_PTR *		actorPtrs;
	PxU32					actorsCount;
};

struct BroadphaseRegionDesc
{
	GRB_INTEROP_PTR			ptr;
	PxBroadPhaseRegion		desc;
	bool					populateRegion;
};

class ClonedTriangleMesh : public Gu::TriangleMesh
{
public:

	virtual	const PxU32*			getTrianglesRemap()										const
	{
		return mRemap;
	}

	virtual	PxMaterialTableIndex	getTriangleMaterialIndex(PxTriangleID triangleIndex)	const
	{
		return mMaterials ? mMaterials[triangleIndex] : 0xffff;	
	}

	PxU16*						mMaterials;		
	PxU32*						mRemap;				
};

class ClonedHeightField : public PxHeightField
{
public:

	void copyDataAndSamples(Gu::HeightField * other, PxSceneEventDescs::StackAllocator &stackAlloc)
	{
		memcpy(&mData, &other->getData(), sizeof(Gu::HeightFieldData));

		PxU32 numBytes = other->getData().rows * other->getData().columns * sizeof(PxHeightFieldSample);
		mData.samples = (PxHeightFieldSample *)stackAlloc.allocate(numBytes);

		memcpy(mData.samples, other->getData().samples, numBytes);
	}

	Gu::HeightFieldData	mData;

	// Interface stuff
	ClonedHeightField() : PxHeightField(PxConcreteType::eHEIGHTFIELD, PxBaseFlag::eOWNS_MEMORY | PxBaseFlag::eIS_RELEASABLE) {}
	virtual		PxU32										getObjectSize() const { return sizeof(*this); }

	virtual	void						release() {}
	virtual	PxU32						saveCells(void* /*destBuffer*/, PxU32 /*destBufferSize*/) const { return 0; }
	virtual	bool						modifySamples(PxI32 /*startCol*/, PxI32 /*startRow*/, const PxHeightFieldDesc& /*subfieldDesc*/, bool /*shrinkBounds*/) { return false; }

	virtual	PxU32						getNbRows()						const { return mData.rows;								}
	virtual	PxU32						getNbColumns()					const { return mData.columns;							}
	virtual	PxHeightFieldFormat::Enum	getFormat()						const { return mData.format;							}
	virtual	PxU32						getSampleStride()				const { return sizeof(PxHeightFieldSample);				}
	virtual	PxReal						getThickness()					const { return mData.thickness;							}
	virtual	PxReal						getConvexEdgeThreshold()		const { return mData.convexEdgeThreshold;				}
	virtual	PxHeightFieldFlags			getFlags()						const { return mData.flags;								}
	virtual	PxU32						getReferenceCount()				const { return 0;										}
	
	PX_FORCE_INLINE	PxU16	getMaterialIndex0(PxU32 vertexIndex) const		{ return mData.samples[vertexIndex].materialIndex0;	}
	PX_FORCE_INLINE	PxU16	getMaterialIndex1(PxU32 vertexIndex) const		{ return mData.samples[vertexIndex].materialIndex1;	}
	PX_FORCE_INLINE PxReal	getHeight(PxU32 vertexIndex) const				{ return PxReal(mData.samples[vertexIndex].height); }
	PX_FORCE_INLINE	bool	isZerothVertexShared(PxU32 vertexIndex) const	{ return mData.samples[vertexIndex].tessFlag() != 0; }
	PX_FORCE_INLINE	bool	isFirstTriangle(PxU32 triangleIndex) const		{ return ((triangleIndex & 0x1) == 0);	}

	PxU32 computeCellCoordinates(PxReal x, PxReal z, PxReal& fracX, PxReal& fracZ) const
	{
		namespace i = physx::intrinsics;

		x = i::selectMax(x, 0.0f);
		z = i::selectMax(z, 0.0f);
		PxF32 epsx = 1.0f - PxAbs(x+1.0f) * 1e-6f; // epsilon needs to scale with values of x,z...
		PxF32 epsz = 1.0f - PxAbs(z+1.0f) * 1e-6f;
		PxF32 x1 = i::selectMin(x, mData.rowLimit+epsx);
		PxF32 z1 = i::selectMin(z, mData.colLimit+epsz);
		x = PxFloor(x1);
		fracX = x1 - x;
		z = PxFloor(z1);
		fracZ = z1 - z;
		PX_ASSERT(x >= 0.0f && x < PxF32(mData.rows));
		PX_ASSERT(z >= 0.0f && z < PxF32(mData.columns));

		const PxU32 vertexIndex = PxU32(x * (mData.nbColumns) + z);
		PX_ASSERT(vertexIndex < (mData.rows)*(mData.columns));

		return vertexIndex;
	}

	virtual	PxMaterialTableIndex		getTriangleMaterialIndex(PxTriangleID triangleIndex) const { return isFirstTriangle(triangleIndex) ? getMaterialIndex0(triangleIndex >> 1) : getMaterialIndex1(triangleIndex >> 1); }
	virtual	PxVec3						getTriangleNormal(PxTriangleID triangleIndex) const
	{
		PxU32 v0, v1, v2;
		getTriangleVertexIndices(triangleIndex, v0, v1, v2); 

		const PxI32 h0 = mData.samples[v0].height;
		const PxI32 h1 = mData.samples[v1].height;
		const PxI32 h2 = mData.samples[v2].height;

		// Fix for NvBug 685420
		//if(mThickness>0.0f)
		//	n = -n;
		const PxReal coeff = physx::intrinsics::fsel(mData.thickness, -1.0f, 1.0f);

		const PxU32 cell = triangleIndex >> 1;
		if (isZerothVertexShared(cell))
		{
			//      <---- COL  
			//      0----2  1 R
			//      | 1 /  /| O
			//      |  /  / | W
			//      | /  /  | |
			//      |/  / 0 | |
			//      1  2----0 V
			//      
			if (isFirstTriangle(triangleIndex))
			{
				return PxVec3(coeff*PxReal(h1-h0), coeff, coeff*PxReal(h0-h2));
			}
			else
			{
				return PxVec3(coeff*PxReal(h0-h1), coeff, coeff*PxReal(h2-h0));
			}
		}
		else
		{
			//      <---- COL  
			//      2  1----0 R
			//      |\  \ 0 | O
			//      | \  \  | W
			//      |  \  \ | |
			//      | 1 \  \| |
			//      0----1  2 V
			//                   
			if (isFirstTriangle(triangleIndex))
			{
				return PxVec3(coeff*PxReal(h0-h2), coeff, coeff*PxReal(h0-h1));
			}
			else
			{
				return PxVec3(coeff*PxReal(h2-h0), coeff, coeff*PxReal(h1-h0));
			}
		}
	}
	PX_INLINE		void getTriangleVertexIndices(PxU32 triangleIndex, PxU32& vertexIndex0, PxU32& vertexIndex1, PxU32& vertexIndex2) const
	{
		const PxU32 cell = triangleIndex >> 1;
		if (isZerothVertexShared(cell))
		{
			//      <---- COL  
			//      0----2  1 R
			//      | 1 /  /| O
			//      |  /  / | W
			//      | /  /  | |
			//      |/  / 0 | |
			//      1  2----0 V
			//      
			if (isFirstTriangle(triangleIndex))
			{
				vertexIndex0 = cell + mData.columns;
				vertexIndex1 = cell;
				vertexIndex2 = cell + mData.columns + 1;
			}
			else
			{
				vertexIndex0 = cell + 1;
				vertexIndex1 = cell + mData.columns + 1;
				vertexIndex2 = cell;
			}
		}
		else
		{
			//      <---- COL  
			//      2  1----0 R
			//      |\  \ 0 | O
			//      | \  \  | W
			//      |  \  \ | |
			//      | 1 \  \| |
			//      0----1  2 V
			//                   
			if (isFirstTriangle(triangleIndex))
			{
				vertexIndex0 = cell;
				vertexIndex1 = cell + 1;
				vertexIndex2 = cell + mData.columns;
			}
			else
			{
				vertexIndex0 = cell + mData.columns + 1;
				vertexIndex1 = cell + mData.columns;
				vertexIndex2 = cell + 1;
			}
		}
	}

	virtual	PxReal						getHeight(PxReal x, PxReal z)	const
	{
		PxReal fracX, fracZ;
		const PxU32 vertexIndex = computeCellCoordinates(x, z, fracX, fracZ);

		if (isZerothVertexShared(vertexIndex))
		{
			//    <----Z---+
			//      +----+ | 
			//      |   /| |
			//      |  / | X
			//      | /  | |
			//      |/   | |
			//      +----+ |
			//             V
			const PxReal h0 = getHeight(vertexIndex);
			const PxReal h2 = getHeight(vertexIndex + mData.columns + 1);
			if (fracZ > fracX)
			{
				//    <----Z---+
				//      1----0 | 
				//      |   /  |
				//      |  /   X
				//      | /    |
				//      |/     |
				//      2      |
				//             V
				const PxReal h1 = getHeight(vertexIndex + 1);
				return h0 + fracZ*(h1-h0) + fracX*(h2-h1);
			}
			else
			{
				//    <----Z---+
				//           0 | 
				//          /| |
				//         / | X
				//        /  | |
				//       /   | |
				//      2----1 |
				//             V
				const PxReal h1 = getHeight(vertexIndex + mData.columns);
				return h0 + fracX*(h1-h0) + fracZ*(h2-h1);
			}
		}
		else
		{
			//    <----Z---+
			//      +----+ | 
			//      |\   | |
			//      | \  | X
			//      |  \ | |
			//      |   \| |
			//      +----+ |
			//             V
			const PxReal h2 = getHeight(vertexIndex + mData.columns);
			const PxReal h1 = getHeight(vertexIndex + 1);
			if (fracX + fracZ < 1.0f)
			{
				//    <----Z---+
				//      1----0 | 
				//       \   | |
				//        \  | X
				//         \ | |
				//          \| |
				//           2 |
				//             V
				const PxReal h0 = getHeight(vertexIndex);
				return h0 + fracZ*(h1-h0) + fracX*(h2-h0);
			}
			else
			{
				//    <----Z---+
				//      1      | 
				//      |\     |
				//      | \    X
				//      |  \   |
				//      |   \  |
				//      0----2 |
				//             V
				//
				// Note that we need to flip fracX and fracZ since we are moving the origin
				const PxReal h0 = getHeight(vertexIndex + mData.columns + 1);
				return h0 + (1.0f - fracZ)*(h2-h0) + (1.0f - fracX)*(h1-h0);
			}
		}
	}
};

};

using PxSceneEventDescs::StackAllocator;
using PxSceneEventDescs::ClonedTriangleMesh;
using PxSceneEventDescs::ClonedHeightField;

//-----------------------------------------------------------------------------
static void clonePhysXGeometry(PxGeometryHolder & clone, const PxGeometryHolder & orig, StackAllocator &stackAlloc)
{
	clone = orig;

	//Some geometries have pointers that need to be cloned
	switch (orig.getType())
	{
		//Basic geometries don't have any pointers
	case PxGeometryType::eSPHERE:
	case PxGeometryType::ePLANE:
	case PxGeometryType::eBOX:
	case PxGeometryType::eCAPSULE:
			break;
	case PxGeometryType::eCONVEXMESH:
		{
			const PxConvexMeshGeometry & origGeom = orig.convexMesh();
			PxConvexMeshGeometry & clonedGeom = clone.convexMesh();

			Gu::ConvexMesh * origConvexMesh = static_cast<Gu::ConvexMesh *>(origGeom.convexMesh);
			Gu::ConvexMesh * clonedConvexMesh = new (stackAlloc.allocate(sizeof(Gu::ConvexMesh))) Gu::ConvexMesh();

			size_t numBytes = origConvexMesh->getBufferSize();
			clonedConvexMesh->getHull().mPolygons = reinterpret_cast<Gu::HullPolygonData *>((stackAlloc.allocate(numBytes)));
			memcpy(clonedConvexMesh->getHull().mPolygons, (void *)origConvexMesh->getHull().mPolygons, numBytes);

			clonedConvexMesh->getHull().mNbEdges = origConvexMesh->getHull().mNbEdges;
			clonedConvexMesh->getHull().mNbHullVertices = origConvexMesh->getHull().mNbHullVertices;
			clonedConvexMesh->getHull().mNbPolygons = origConvexMesh->getHull().mNbPolygons;

			clonedConvexMesh->getHull().mCenterOfMass = origConvexMesh->getHull().mCenterOfMass;
			clonedConvexMesh->getHull().mAABB = origConvexMesh->getHull().mAABB;
			clonedConvexMesh->getHull().mInternal = origConvexMesh->getHull().mInternal;

			clonedGeom.convexMesh = static_cast<PxConvexMesh *>(clonedConvexMesh);

			break;
		}
	case PxGeometryType::eTRIANGLEMESH:
		{
			//TODO: clone triangleMesh pointer
			const PxTriangleMeshGeometry & origGeom = orig.triangleMesh();
			PxTriangleMeshGeometry & clonedGeom = clone.triangleMesh();

			Gu::TriangleMesh * origTriMesh = static_cast<Gu::TriangleMesh *>(origGeom.triangleMesh);

			const PxU32 alignment = 16;
			void * allocatedMem = stackAlloc.allocate(sizeof(ClonedTriangleMesh) + (alignment - 1));
			void * allignedMem = reinterpret_cast<void *>( reinterpret_cast<ptrdiff_t>(reinterpret_cast<PxU8 *>(allocatedMem) + alignment - 1) & ~(alignment - 1) );

			ClonedTriangleMesh * clonedTriMesh = new (allignedMem) ClonedTriangleMesh();

			//clone vertices
			clonedTriMesh->mMesh.mData.mNumVertices = origTriMesh->getNbVertices();
			PxU32 numBytes = origTriMesh->getNbVertices() * sizeof(PxVec3);
			clonedTriMesh->mMesh.mData.mVertices = (PxVec3*)stackAlloc.allocate(numBytes);
			memcpy(clonedTriMesh->mMesh.mData.mVertices, (void *)origTriMesh->getVertices(), numBytes);
			
			//clone indices
			clonedTriMesh->mMesh.mData.mNumTriangles = origTriMesh->getNbTriangles();
			clonedTriMesh->mMesh.mData.mFlags = origTriMesh->getTriangleMeshFlags();
			bool bHas16bitIndices = clonedTriMesh->mMesh.mData.mFlags & PxTriangleMeshFlag::eHAS_16BIT_TRIANGLE_INDICES ? true : false;			

			if(bHas16bitIndices)
			{
				numBytes = origTriMesh->getNbTriangles() * 3 * sizeof(PxU16);
				clonedTriMesh->mMesh.mData.mTriangles = (PxU16 *)stackAlloc.allocate(numBytes);
			}
			else
			{
				numBytes = origTriMesh->getNbTriangles() * 3 * sizeof(PxU32);
				clonedTriMesh->mMesh.mData.mTriangles = (PxU32 *)stackAlloc.allocate(numBytes);
			}

			memcpy(clonedTriMesh->mMesh.mData.mTriangles, (void *) origTriMesh->getTriangles(), numBytes);
		
			//clone materials
			if(origTriMesh->mMesh.getMaterials())
			{
				numBytes = origTriMesh->getNbTriangles() * sizeof(PxU16);
				clonedTriMesh->mMaterials = (PxU16 *)stackAlloc.allocate(numBytes);
				memcpy(clonedTriMesh->mMaterials, (void *) origTriMesh->mMesh.getMaterials(), numBytes);		
			}
			else
			{
				clonedTriMesh->mMaterials = 0;
			}

			//clone remap
			if(origTriMesh->mMesh.getFaceRemap())
			{
				numBytes = origTriMesh->getNbTriangles() * sizeof(PxU32);
				clonedTriMesh->mRemap = (PxU32 *)stackAlloc.allocate(numBytes);
				memcpy(clonedTriMesh->mRemap, (void *) origTriMesh->mMesh.getFaceRemap(), numBytes);		
			}
			else
			{
				clonedTriMesh->mRemap = 0;
			}

			//clone extraTrigData
			if(origTriMesh->mMesh.mData.mExtraTrigData)
			{
				numBytes = origTriMesh->getNbTriangles() * sizeof(PxU8);
				clonedTriMesh->mMesh.mData.mExtraTrigData = (PxU8 *)stackAlloc.allocate(numBytes);
				memcpy(clonedTriMesh->mMesh.mData.mExtraTrigData, (void *) origTriMesh->mMesh.mData.mExtraTrigData, numBytes);		
			}
			else
			{
				clonedTriMesh->mMesh.mData.mExtraTrigData = 0;
			}

			clonedTriMesh->mMesh.mData.mAABB = origTriMesh->mMesh.mData.mAABB;

			clonedGeom.triangleMesh = static_cast<PxTriangleMesh *>(clonedTriMesh);

			break;
		}
	case PxGeometryType::eHEIGHTFIELD:
		{
			//TODO: clone heightField pointer
			const PxHeightFieldGeometry & origGeom = orig.heightField();
			PxHeightFieldGeometry & clonedGeom = clone.heightField();

			const PxU32 alignment = 16;
			void * allocatedMem = stackAlloc.allocate(sizeof(ClonedHeightField) + (alignment - 1));
			void * allignedMem = reinterpret_cast<void *>( reinterpret_cast<ptrdiff_t>(reinterpret_cast<PxU8 *>(allocatedMem) + alignment - 1) & ~(alignment - 1) );

			Gu::HeightField * origHeightField = static_cast<Gu::HeightField *>(origGeom.heightField);
			ClonedHeightField * clonedHeightField = new (allignedMem) ClonedHeightField();

			clonedHeightField->copyDataAndSamples(origHeightField, stackAlloc);

			clonedGeom.columnScale = origGeom.columnScale;
			clonedGeom.rowScale = origGeom.rowScale;
			clonedGeom.heightScale = origGeom.heightScale;
			
			break;
		}
	};	
}
//-----------------------------------------------------------------------------
static void clonePhysXName(char * & cloned, const char * orig, StackAllocator & stackAlloc)
{
	if (!orig)
	{
		cloned = NULL;
		return;
	}

	char addition[] = "_GRBinterop";
	PxU32 origLen = (PxU32)strlen(orig);
	PxU32 addLen = (PxU32)strlen(addition);

	cloned = (char *)stackAlloc.allocate((origLen + addLen + 1) * sizeof(char));
	sprintf(cloned, "%s%s", orig, addition);
}
//-----------------------------------------------------------------------------
static void clonePhysXMaterial(PxSceneEventDescs::MaterialDesc & materialDesc, const PxMaterial * orig)
{
	materialDesc.ptr					= (GRB_INTEROP_PTR)orig;
	materialDesc.dynamicFriction		= orig->getDynamicFriction();
	materialDesc.frictionCombineMode	= orig->getFrictionCombineMode();
	materialDesc.restitution			= orig->getRestitution();
	materialDesc.restitutionCombineMode	= orig->getRestitutionCombineMode();
	materialDesc.staticFriction			= orig->getStaticFriction();
	materialDesc.flags					= orig->getFlags();
}
static PxSceneEventDescs::MaterialDesc * clonePhysXMaterials(PxU32 materialsCount, PxMaterial * const * origMaterials, StackAllocator & stackAlloc)
{
	PxSceneEventDescs::MaterialDesc * materialDescs = (PxSceneEventDescs::MaterialDesc *)stackAlloc.allocate(materialsCount * sizeof(PxSceneEventDescs::MaterialDesc));

	for (PxU32 i = 0; i < materialsCount; ++i)
	{
		clonePhysXMaterial(materialDescs[i], origMaterials[i]);
	}

	return materialDescs;
}
static void clonePhysxShape(PxSceneEventDescs::ShapeDesc & shapeDesc, const PxShape * shape, StackAllocator & stackAlloc)
{
	shapeDesc.ptr				= (GRB_INTEROP_PTR)shape;
	shapeDesc.contactOffset		= shape->getContactOffset();
	shapeDesc.restOffset		= shape->getRestOffset();
	shapeDesc.localPose			= shape->getLocalPose();
	shapeDesc.flags				= shape->getFlags();
	shapeDesc.simFilterData		= shape->getSimulationFilterData();
	shapeDesc.queryFilterData	= shape->getQueryFilterData();
	shapeDesc.isExclusive		= shape->isExclusive();

	PxU32 numMaterials = shape->getNbMaterials();
	PxMaterial ** materials = (PxMaterial **)stackAlloc.allocate(numMaterials * sizeof(PxMaterial *));
	shape->getMaterials(materials, numMaterials);

	shapeDesc.materialsCount = numMaterials;
	shapeDesc.materialDescs = clonePhysXMaterials(numMaterials, materials, stackAlloc);
	clonePhysXGeometry(shapeDesc.geometry,shape->getGeometry(), stackAlloc);
}
static PxSceneEventDescs::ShapeDesc * clonePhysXRigidActorShapes(PxU32 shapesCount, const PxRigidActor * rigidActor, StackAllocator & stackAlloc)
{
	PxSceneEventDescs::ShapeDesc * shapeDescs = (PxSceneEventDescs::ShapeDesc *)stackAlloc.allocate(shapesCount * sizeof(PxSceneEventDescs::ShapeDesc));

	PxShape * userBuf[5];
	for (PxU32 shapeCnt = 0; shapeCnt < shapesCount; shapeCnt += 5)
	{
		PxU32 numShapesRemaining = PxMin<PxU32>(5, shapesCount - shapeCnt);
		rigidActor->getShapes(userBuf, 5, shapeCnt);
		
		for (PxU32 procShapeCnt = 0; procShapeCnt < numShapesRemaining; ++procShapeCnt)
		{
			clonePhysxShape(shapeDescs[shapeCnt + procShapeCnt], userBuf[procShapeCnt], stackAlloc);
		}
	}

	return shapeDescs;
}
static void clonePhysXRigidStatic(PxSceneEventDescs::RigidStaticDesc & rigidStaticDesc, const PxRigidStatic * rigidStatic, StackAllocator & stackAlloc)
{
	rigidStaticDesc.type				= GRB_INTEROP_RIGID_STATIC_DESC;

	rigidStaticDesc.ptr					= (GRB_INTEROP_PTR)rigidStatic;
	rigidStaticDesc.globalPose			= rigidStatic->getGlobalPose();
	rigidStaticDesc.dominanceGroup		= rigidStatic->getDominanceGroup();
	rigidStaticDesc.actorFlags			= rigidStatic->getActorFlags();
	rigidStaticDesc.clientID			= rigidStatic->getOwnerClient();

	clonePhysXName(rigidStaticDesc.name, rigidStatic->getName(), stackAlloc);

	rigidStaticDesc.shapesCount = rigidStatic->getNbShapes();
	rigidStaticDesc.shapeDescs = clonePhysXRigidActorShapes(rigidStaticDesc.shapesCount, static_cast<const PxRigidActor *>(rigidStatic), stackAlloc);
}
static void clonePhysXRigidDynamic(PxSceneEventDescs::RigidDynamicDesc & rigidDynamicDesc, const PxRigidDynamic * rigidDynamic, StackAllocator & stackAlloc)
{
	rigidDynamicDesc.type					= GRB_INTEROP_RIGID_DYNAMIC_DESC;

	rigidDynamicDesc.ptr					= (GRB_INTEROP_PTR)rigidDynamic;
	rigidDynamicDesc.globalPose				= rigidDynamic->getGlobalPose();
	rigidDynamicDesc.dominanceGroup			= rigidDynamic->getDominanceGroup();
	rigidDynamicDesc.actorFlags				= rigidDynamic->getActorFlags();
	rigidDynamicDesc.clientID				= rigidDynamic->getOwnerClient();

	rigidDynamicDesc.linearVelocity			= rigidDynamic->getLinearVelocity();
	rigidDynamicDesc.angularVelocity		= rigidDynamic->getAngularVelocity();
	rigidDynamicDesc.linearDamping			= rigidDynamic->getLinearDamping();
	rigidDynamicDesc.angularDamping			= rigidDynamic->getAngularDamping();
	rigidDynamicDesc.rigidBodyFlags			= rigidDynamic->getRigidBodyFlags();
	rigidDynamicDesc.contactReportThreshold	= rigidDynamic->getContactReportThreshold();

	clonePhysXName(rigidDynamicDesc.name, rigidDynamic->getName(), stackAlloc);

	rigidDynamicDesc.shapesCount = rigidDynamic->getNbShapes();
	rigidDynamicDesc.shapeDescs = clonePhysXRigidActorShapes(rigidDynamicDesc.shapesCount, static_cast<const PxRigidActor *>(rigidDynamic), stackAlloc);
}
//-----------------------------------------------------------------------------
static void releaseShapeDesc(PxSceneEventDescs::ShapeDesc & shapeDesc)
{
	switch (shapeDesc.geometry.getType())
	{
	case PxGeometryType::eSPHERE:
		{
			break;
		}
	case PxGeometryType::ePLANE:
		{
			break;
		}
	case PxGeometryType::eBOX:
		{
			break;
		}
	case PxGeometryType::eCAPSULE:
		{
			break;
		}

		/*
			avoroshilov: add special releasers for these three (internal arrays freeing)
		*/

	case PxGeometryType::eCONVEXMESH:
		{
			break;
		}
	case PxGeometryType::eTRIANGLEMESH:
		{
			break;
		}
	case PxGeometryType::eHEIGHTFIELD:
		{
			break;
		}
	}
}

// avoroshilov: deal with allocators
struct SceneAddRigidStaticEvent
{
	SceneAddRigidStaticEvent::SceneAddRigidStaticEvent() {}

	SceneAddRigidStaticEvent::SceneAddRigidStaticEvent(const PxRigidStatic * rigidStatic, StackAllocator &stackAlloc)
	{
		clonePhysXRigidStatic(rigidStaticDesc, rigidStatic, stackAlloc);
	}

	~SceneAddRigidStaticEvent()
	{
		for (PxU32 i = 0; i < rigidStaticDesc.shapesCount; ++i)
		{
			releaseShapeDesc(rigidStaticDesc.shapeDescs[i]);
		}
	}

	PxSceneEventDescs::RigidStaticDesc rigidStaticDesc;
};

struct SceneAddRigidDynamicEvent
{
	SceneAddRigidDynamicEvent::SceneAddRigidDynamicEvent() {}

	SceneAddRigidDynamicEvent::SceneAddRigidDynamicEvent(const PxRigidDynamic * rigidDynamic, StackAllocator &stackAlloc)
	{
		clonePhysXRigidDynamic(rigidDynamicDesc, rigidDynamic, stackAlloc);
	}

	~SceneAddRigidDynamicEvent()
	{
		for (PxU32 i = 0; i < rigidDynamicDesc.shapesCount; ++i)
		{
			releaseShapeDesc(rigidDynamicDesc.shapeDescs[i]);
		}
	}

	PxSceneEventDescs::RigidDynamicDesc rigidDynamicDesc;
};

struct SceneAddRigidActorsEvent
{
	SceneAddRigidActorsEvent::SceneAddRigidActorsEvent() {}

	SceneAddRigidActorsEvent::SceneAddRigidActorsEvent(PxActor *const* actors, PxU32 numActors, StackAllocator &stackAlloc)
	{
		numRigidActors = 0;
		rigidActorDescs = (PxSceneEventDescs::RigidStaticDesc **)stackAlloc.allocate(numActors * sizeof(PxSceneEventDescs::RigidStaticDesc *));

		for (PxU32 i = 0; i < numActors; ++i)
		{
			PxActorType::Enum actorType = actors[i]->getType();
			if (actorType == PxActorType::eRIGID_STATIC)
			{
				PxSceneEventDescs::RigidStaticDesc * rigidStaticDesc = (PxSceneEventDescs::RigidStaticDesc *)stackAlloc.allocate(sizeof(PxSceneEventDescs::RigidStaticDesc));

				clonePhysXRigidStatic(*rigidStaticDesc, static_cast<PxRigidStatic *>(actors[i]), stackAlloc);

				rigidActorDescs[numRigidActors] = rigidStaticDesc;
				++numRigidActors;
			}
			else
			if (actorType == PxActorType::eRIGID_DYNAMIC)
			{
				PxSceneEventDescs::RigidDynamicDesc * rigidDynamicDesc = (PxSceneEventDescs::RigidDynamicDesc *)stackAlloc.allocate(sizeof(PxSceneEventDescs::RigidDynamicDesc));

				clonePhysXRigidDynamic(*rigidDynamicDesc, static_cast<PxRigidDynamic *>(actors[i]), stackAlloc);

				rigidActorDescs[numRigidActors] = static_cast<PxSceneEventDescs::RigidStaticDesc *>(rigidDynamicDesc);
				++numRigidActors;
			}
		}
	}

	~SceneAddRigidActorsEvent()
	{
		for (PxU32 j = 0; j < numRigidActors; ++j)
		{
			PxSceneEventDescs::RigidStaticDesc * curDesc = rigidActorDescs[j];
			for (PxU32 i = 0; i < curDesc->shapesCount; ++i)
			{
				releaseShapeDesc(curDesc->shapeDescs[i]);
			}
		}
	}

	PxSceneEventDescs::RigidStaticDesc **	rigidActorDescs;
	PxU32									numRigidActors;
};

struct SceneAddAggregateEvent
{
	SceneAddAggregateEvent::SceneAddAggregateEvent() {}

	SceneAddAggregateEvent::SceneAddAggregateEvent(const PxAggregate * aggregate, StackAllocator &stackAlloc)
	{
		aggregateDesc.actorsCount = aggregate->getNbActors();
		aggregateDesc.actorPtrs = (GRB_INTEROP_PTR *)stackAlloc.allocate(aggregateDesc.actorsCount * sizeof(GRB_INTEROP_PTR));

		PxActor * userBuf[5];
		for (PxU32 actorCnt = 0; actorCnt < aggregateDesc.actorsCount; actorCnt += 5)
		{
			PxU32 numActorsRemaining = PxMin<PxU32>(5, aggregateDesc.actorsCount - actorCnt);
			aggregate->getActors(userBuf, 5, actorCnt);

			for (PxU32 i = 0; i < numActorsRemaining; ++i)
				aggregateDesc.actorPtrs[actorCnt + i] = (GRB_INTEROP_PTR)userBuf[i];
		}
	}

	PxSceneEventDescs::AggregateDesc aggregateDesc;
};

struct SceneAddBroadphaseRegionEvent
{
	SceneAddBroadphaseRegionEvent::SceneAddBroadphaseRegionEvent() {}

	SceneAddBroadphaseRegionEvent::SceneAddBroadphaseRegionEvent(const PxBroadPhaseRegion * bpRegion, bool populateRegion, StackAllocator& /*stackAlloc*/ )
	{
		bpRegionDesc.ptr = (GRB_INTEROP_PTR)bpRegion;
		bpRegionDesc.desc = *bpRegion;
		bpRegionDesc.populateRegion = populateRegion;
	}

	PxSceneEventDescs::BroadphaseRegionDesc bpRegionDesc;
};

struct SceneUpdateMaterialEvent
{
	SceneUpdateMaterialEvent::SceneUpdateMaterialEvent() {}

	SceneUpdateMaterialEvent::SceneUpdateMaterialEvent(const PxMaterial * material, StackAllocator& /*stackAlloc*/)
	{
		clonePhysXMaterial(materialDesc, material);
	}

	PxSceneEventDescs::MaterialDesc materialDesc;
};

struct SceneFetchResultsEvent
{
	struct ActorUpdateData
	{
		PxTransform		pose;
		PxVec3			linVel;
		PxVec3			angVel;
		GRB_INTEROP_PTR	actor;
	};

	void set(PxScene & scene)
	{
		// avoroshilov:
		// there is a bug in physx (reported and in the process of fixing already) which duplicates entries
		// when getActors(eDYNAMIC) performed, hence for now workaround should be applied
		// TODO avoroshilov: fix this after merging PhysX bugfix

		const PxU32 numRigidActors = scene.getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC);	// Since this is called at the end of fetchResults(), this should not change
		PxActor * userBuf[5];
		for (PxU32 actorCnt = 0; actorCnt < numRigidActors; actorCnt += 5)
		{
			PxU32 numActorsRemaining = PxMin<PxU32>(5, numRigidActors - actorCnt);
			scene.getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC, userBuf, 5, actorCnt);
			
			for (PxU32 procActorCnt = 0; procActorCnt < numActorsRemaining; ++procActorCnt)
			{
				PxRigidDynamic * curActor = userBuf[procActorCnt]->isRigidDynamic();
				if (curActor)
				{
					ActorUpdateData & updateData = updates.insert();
					updateData.pose =	curActor->getGlobalPose();
					updateData.linVel =	curActor->getLinearVelocity();
					updateData.angVel =	curActor->getAngularVelocity();
					updateData.actor =	(GRB_INTEROP_PTR)curActor;
				}
			}
		}
	}

	void clear()
	{
		updates.clear();			// note this doesn't actually free any memory
	}

	bool isEmpty()
	{
		return updates.begin() == updates.end();
	}

	shdfnd::Array<ActorUpdateData> updates;
};
//-----------------------------------------------------------------------------
struct ActorCreateShapeEvent
{
	ActorCreateShapeEvent(
		const PxActor * actor,
		const PxShape * shape,
		StackAllocator &stackAlloc
		)
	{
		pxActorPtr = (GRB_INTEROP_PTR)actor;
		clonePhysxShape(shapeDesc, shape, stackAlloc);
	}

	PxSceneEventDescs::ShapeDesc	shapeDesc;
	GRB_INTEROP_PTR					pxActorPtr;
};
//-----------------------------------------------------------------------------
struct ActorAttachShapeEvent
{
	ActorAttachShapeEvent(
		const PxActor * actor,
		const PxShape * shape,
		StackAllocator &stackAlloc
		)
	{
		pxActorPtr = (GRB_INTEROP_PTR)actor;
		clonePhysxShape(shapeDesc, shape, stackAlloc);
	}

	PxSceneEventDescs::ShapeDesc	shapeDesc;
	GRB_INTEROP_PTR					pxActorPtr;
};
//-----------------------------------------------------------------------------
struct ShapeSetGeometryEvent
{
	ShapeSetGeometryEvent(
		const PxShape * shape,
		StackAllocator &stackAlloc
		)
	{
		pxShapePtr = (GRB_INTEROP_PTR)shape;
		clonePhysXGeometry(geometry, shape->getGeometry(), stackAlloc);
	}

	PxGeometryHolder				geometry;
	GRB_INTEROP_PTR					pxShapePtr;
};
//-----------------------------------------------------------------------------
template<typename T>
struct PxSceneEventPool
{
	~PxSceneEventPool<T>()
	{
		while( eventCache.size() != 0 )
		{
			delete eventCache.back();
			eventCache.popBack();
		}
	}

	T*	getEvent()
	{
		for( T** i = eventCache.begin(); i < eventCache.end(); ++i )
		{
			T* e = *i;
			if( e->isEmpty() )
			{	// we may use this one
				return e;
			}
		}

		// avoroshilov: (NX_MEMORY_PERSISTENT) parameter
		T* e = new T();
		eventCache.pushBack( e );
		return e;
	}

private:

	shdfnd::Array<T*>	eventCache;
};

//-----------------------------------------------------------------------------
//Class for communicating the details of a PhysX API call to GRB

class PxSceneEvent
{

private:
	PxSceneEvent& operator=(const PxSceneEvent&);

public:
	enum Type
	{
		//PxScene events
		PxSceneSetGravity,
		PxSceneAddRigidStatic,
		PxSceneAddRigidDynamic,
		PxSceneAddRigidActors,
		PxSceneRemoveActor,
		PxSceneAddAggregate,
		PxSceneRemoveAggregate,
		PxSceneAddBroadphaseRegion,
		PxSceneRemoveBroadphaseRegion,
		PxSceneCreateActor,
		PxSceneFetchResults,
		PxSceneUpdateMaterial,
		PxSceneRemoveMaterial,
		PxSceneRelease,


		//PxActor events
		PxActorCreateShape,
		PxActorAttachShape,
		PxActorDetachShape,
		PxActorSetFlags,
		PxActorSetDominanceGroup,
		PxActorSetName,

		PxActorSetRigidBodyFlags,
		PxActorSetContactReportThreshold,


		//PxShapeEvents
		PxShapeSetGeometry,
		PxShapeSetLocalPose,
		PxShapeSetSimulationFilterData,
		PxShapeSetQueryFilterData,
		PxShapeResetFiltering,
		PxShapeSetMaterials,
		PxShapeSetContactOffset,
		PxShapeSetRestOffset,
		PxShapeSetFlags,
		PxShapeSetName,
		PxShapeRelease,


		//PxMaterial events
		/*
		PxMaterialLoadFromDesc,
		PxMaterialSetRestitution,
		PxMaterialSetDynamicFriction,
		PxMaterialSetStaticFriction,
		PxMaterialSetRestitutionCombineMode,
		PxMaterialSetFrictionCombineMode,
		*/


		//PxAggregate events
		PxAggregateAddActor,
		PxAggregateRelease,


		ForceSizeTo32Bits = 0xFFFFFFFF
	};

	struct PoolSet
	{
		PxSceneEventPool<SceneFetchResultsEvent>	sceneFetchResultsEventPool;
	};

	//PxScene events

	PxSceneEvent(StackAllocator &stackAlloc): mStackAlloc(stackAlloc) {}

#define SCENE_EVENT_DEF(...) \
	PxSceneEvent(StackAllocator &stackAlloc, Type t, __VA_ARGS__) : type(t), mStackAlloc(stackAlloc)

	// PxScene
	SCENE_EVENT_DEF(const PxVec3 & vec)												{ new (&vec3()) PxVec3(vec); }
	SCENE_EVENT_DEF(const PxRigidStatic * rigidStatic)								{ arg.sceneAddRigidStaticEventPtr = new(stackAlloc.allocate(sizeof(SceneAddRigidStaticEvent))) SceneAddRigidStaticEvent(rigidStatic, stackAlloc); }
	SCENE_EVENT_DEF(const PxRigidDynamic * rigidDynamic)							{ arg.sceneAddRigidDynamicEventPtr = new(stackAlloc.allocate(sizeof(SceneAddRigidDynamicEvent))) SceneAddRigidDynamicEvent(rigidDynamic, stackAlloc); }
	SCENE_EVENT_DEF(PxActor *const* actors, PxU32 actorsCount)						{ arg.sceneAddRigidActorsEventPtr = new(stackAlloc.allocate(sizeof(SceneAddRigidActorsEvent))) SceneAddRigidActorsEvent(actors, actorsCount, stackAlloc); }
	SCENE_EVENT_DEF(const PxAggregate * aggregate)									{ arg.sceneAddAggregateEventPtr = new(stackAlloc.allocate(sizeof(SceneAddAggregateEvent))) SceneAddAggregateEvent(aggregate, stackAlloc); }
	SCENE_EVENT_DEF(const PxAggregate * aggregate, PxU32)							{ arg.aggregatePtr = (GRB_INTEROP_PTR)aggregate; }
	SCENE_EVENT_DEF(const PxBroadPhaseRegion * bpRegion, bool populateRegion)		{ arg.sceneAddBroadphaseRegionEventPtr = new(stackAlloc.allocate(sizeof(SceneAddBroadphaseRegionEvent))) SceneAddBroadphaseRegionEvent(bpRegion, populateRegion, stackAlloc); }
	SCENE_EVENT_DEF(const PxMaterial * material)									{ arg.sceneUpdateMaterialEventPtr = new(stackAlloc.allocate(sizeof(SceneUpdateMaterialEvent))) SceneUpdateMaterialEvent(material, stackAlloc); }
	SCENE_EVENT_DEF(const PxMaterial * material, PxU32)								{ arg.materialPtr = (GRB_INTEROP_PTR)material; }
	SCENE_EVENT_DEF(const PxActor * actor)											{ arg.actorPtr = (GRB_INTEROP_PTR)actor; }
	SCENE_EVENT_DEF(PoolSet & pools, PxScene & scene)								{ (arg.sceneFetchResultsEventPtr = pools.sceneFetchResultsEventPool.getEvent())->set( scene ); }
	SCENE_EVENT_DEF(PxScene * scene), object(scene)									{ }
	SCENE_EVENT_DEF(PxU32 pxu32)													{ arg.pxu32 = pxu32; }

	// PxActor
	SCENE_EVENT_DEF(const PxActor * actor, const PxShape * shape, PxU32 creationFlag), object(actor)
	{
		switch (creationFlag)
		{
		case 1:
			{
				arg.actorCreateShapeEventPtr = new(stackAlloc.allocate(sizeof(ActorCreateShapeEvent))) ActorCreateShapeEvent(actor, shape, stackAlloc);
				break;
			}
		case 2:
			{
				arg.actorAttachShapeEventPtr = new(stackAlloc.allocate(sizeof(ActorAttachShapeEvent))) ActorAttachShapeEvent(actor, shape, stackAlloc);
				break;
			}
		}
	}
	SCENE_EVENT_DEF(const PxActor * actor, const PxShape * shape), object(actor)		{ arg.shapePtr = (GRB_INTEROP_PTR)shape; }
	SCENE_EVENT_DEF(const PxActor * actor, PxU8 pxu8), object(actor)					{ arg.pxu8 = pxu8; }
	SCENE_EVENT_DEF(const PxActor * actor, PxU16 pxu16), object(actor)					{ arg.pxu16 = pxu16; }
	SCENE_EVENT_DEF(const PxActor * actor, PxU32 pxu32), object(actor)					{ arg.pxu32 = pxu32; }
	SCENE_EVENT_DEF(const PxActor * actor, PxReal real), object(actor)					{ arg.real = real; }
	SCENE_EVENT_DEF(const PxActor * actor, const char * name), object(actor)			{ clonePhysXName(arg.name, name, stackAlloc); }

	// PxShape
	SCENE_EVENT_DEF(const PxShape * shape, const PxGeometry& /*geom*/), object(shape)							{ arg.shapeSetGeometryEventPtr = new(stackAlloc.allocate(sizeof(ShapeSetGeometryEvent))) ShapeSetGeometryEvent(shape, stackAlloc); }
	SCENE_EVENT_DEF(const PxShape * shape, PxMaterial*const* materials, PxU32 materialCount), object(shape)	{ arg.setMaterialsPair.materialsCount = materialCount; arg.setMaterialsPair.materials = clonePhysXMaterials(materialCount, materials, stackAlloc); }
	SCENE_EVENT_DEF(const PxShape * shape, PxU8 pxu8), object(shape)										{ arg.pxu8 = pxu8; }
	SCENE_EVENT_DEF(const PxShape * shape, PxU16 pxu16), object(shape)										{ arg.pxu16 = pxu16; }
	SCENE_EVENT_DEF(const PxShape * shape, PxU32 pxu32), object(shape)										{ arg.pxu32 = pxu32; }
	SCENE_EVENT_DEF(const PxShape * shape, PxReal real), object(shape)										{ arg.real = real; }
	SCENE_EVENT_DEF(const PxShape * shape, const PxTransform & trans), object(shape)						{ transform() = trans; }
	SCENE_EVENT_DEF(const PxShape * shape, const PxFilterData & simFilterData), object(shape)				{ filterData() = simFilterData; }
	SCENE_EVENT_DEF(const PxShape * shape, const char * name), object(shape)								{ clonePhysXName(arg.name, name, stackAlloc); }

	// PxAggregate
	SCENE_EVENT_DEF(const PxAggregate * aggregate, PxActor * actor), object(aggregate)						{ arg.actorPtr = (GRB_INTEROP_PTR)actor; }

#undef SCENE_EVENT_DEF

	//inheriting all events from PxSceneEvent in order to use a virtual destructor was more lines of code
	void release()
	{
		//switch(type)
		//{
		//default:
		//	break;
		//}
	}

	StackAllocator &mStackAlloc;
	const void *	object;
	Type			type;

	PxVec3 &					vec3()							{ return *(PxVec3*)arg.buf; }
	PxMat33 &					mat33()							{ return *(PxMat33*)arg.buf; }
	PxQuat &					quat()							{ return *(PxQuat*)arg.buf; }
	PxTransform &				transform()						{ return *(PxTransform*)arg.buf; }
	PxFilterData &				filterData()					{ return *(PxFilterData*)arg.buf; }

	union
	{
		PxU8	buf[sizeof(PxMat33)];															// buf should have sizeof of the largest element

		char * name;																			// Clone, do read in GRB

		const char *	charPtr;																// Not a clone, do not read in GRB
		GRB_INTEROP_PTR shapePtr;
		GRB_INTEROP_PTR actorPtr;
		GRB_INTEROP_PTR materialPtr;
		GRB_INTEROP_PTR aggregatePtr;

		PxU8	pxu8;																			// u8, u16, u32 - internal GRB types
		PxU16	pxu16;
		PxU32	pxu32;
		PxU64	pxu64;
		PxReal	real;

		struct
		{
			PxSceneEventDescs::MaterialDesc * materials;
			PxU32 materialsCount;
		} setMaterialsPair;

		SceneAddRigidStaticEvent *		sceneAddRigidStaticEventPtr;								// OK to dereference
		SceneAddRigidDynamicEvent *		sceneAddRigidDynamicEventPtr;								// OK to dereference
		SceneAddRigidActorsEvent *		sceneAddRigidActorsEventPtr;								// OK to dereference
		SceneAddAggregateEvent *		sceneAddAggregateEventPtr;									// OK to dereference
		SceneAddBroadphaseRegionEvent *	sceneAddBroadphaseRegionEventPtr;							// OK to dereference
		SceneUpdateMaterialEvent *		sceneUpdateMaterialEventPtr;								// OK to dereference
		SceneFetchResultsEvent *		sceneFetchResultsEventPtr;									// OK to dereference
		ActorCreateShapeEvent *			actorCreateShapeEventPtr;									// OK to dereference
		ActorAttachShapeEvent *			actorAttachShapeEventPtr;									// OK to dereference
		ShapeSetGeometryEvent *			shapeSetGeometryEventPtr;									// OK to dereference
	} arg;
};

#endif	// #if (USE_GRB_INTEROP == 1)

};

#endif
