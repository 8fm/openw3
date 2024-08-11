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


#include "PsIntrinsics.h"
#include "GuTriangleMesh.h"
#include "PsFoundation.h"
#include "GuSerialize.h"
#include "GuMeshFactory.h"
#include "CmRenderOutput.h"
#include "PxVisualizationParameter.h"
#include "GuConvexEdgeFlags.h"
#include "GuDebug.h"
#include "GuBox.h"
#include "GuOverlapTests.h"


// PX_SERIALIZATION
#include "CmUtils.h"

using namespace physx;

bool PxOpcodeError(const char* message, const char* file, unsigned line)
{
	//error hook for opcode
	Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, file, line, message);
	return false;
}

namespace physx
{

// PT: used to be automatic but making it manual saves bytes in the internal mesh

void Gu::TriangleMesh::exportExtraData(PxSerializationContext& stream)
{
	mMesh.exportExtraData(stream);
}

void Gu::TriangleMesh::importExtraData(PxDeserializationContext& context)
{
	mMesh.importExtraData(context);
}

Gu::TriangleMesh* Gu::TriangleMesh::createObject(PxU8*& address, PxDeserializationContext& context)
{
	TriangleMesh* obj = new (address) TriangleMesh(PxBaseFlag::eIS_RELEASABLE);
	address += sizeof(TriangleMesh);	
	obj->importExtraData(context);
	obj->resolveReferences(context);
	return obj;
}
//~PX_SERIALIZATION

Gu::TriangleMesh::TriangleMesh()
: PxTriangleMesh(PxConcreteType::eTRIANGLE_MESH, PxBaseFlag::eOWNS_MEMORY | PxBaseFlag::eIS_RELEASABLE)
{
	mMesh.mData.mAABB = PxBounds3::empty();
}

Gu::TriangleMesh::~TriangleMesh()
{
	mMesh.release();
}

// PX_SERIALIZATION
void Gu::TriangleMesh::onRefCountZero()
{
	if(mMeshFactory->removeTriangleMesh(*this))
	{
		GuMeshFactory* mf = mMeshFactory;
		Cm::deletePxBase(this);
		mf->notifyFactoryListener(this, PxConcreteType::eTRIANGLE_MESH, true);
		return;
	}

	// PT: if we reach this point, we didn't find the mesh in the Physics object => don't delete!
	// This prevents deleting the object twice.
	Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Gu::TriangleMesh::release: double deletion detected!");
}
//~PX_SERIALIZATION

bool Gu::TriangleMesh::load(PxInputStream& stream)
{
	mMesh.release();

	// Import header
	PxU32 version;
	bool mismatch;
	if(!readHeader('M', 'E', 'S', 'H', version, mismatch, stream))
		return false;

	// Check if old (incompatible) mesh format is loaded
	if (version <= 9) // this refers to PX_MESH_VERSION, also see PX_MESH_VERSION_BC
	{
		Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, "Loading triangle mesh failed: "
			"Deprecated mesh cooking format. Please recook your mesh in a new cooking format.");
		return false;
	}

	// Import serialization flags
	PxU32 serialFlags = readDword(mismatch, stream);

	// Import misc values
	mMesh.mConvexEdgeThreshold = readFloat(mismatch, stream);

	// Import mesh

	PxVec3* verts = mMesh.allocateVertices(readDword(mismatch, stream));
	void* tris = mMesh.allocateTriangles(readDword(mismatch, stream));

	stream.read(verts, sizeof(PxVec3)*mMesh.getNumVertices());
	if(mismatch)
	{
		for(PxU32 i=0;i<mMesh.getNumVertices();i++)
		{
			verts[i].x = flip(&verts[i].x);
			verts[i].y = flip(&verts[i].y);
			verts[i].z = flip(&verts[i].z);
		}
	}
	//TODO: stop support for format conversion on load!!
	if(serialFlags & IMSF_8BIT_INDICES)
	{
		PxU8 x;
		if(mMesh.has16BitIndices())
		{
			PxU16* tris16 = reinterpret_cast<PxU16*>(tris);
			for(PxU32 i=0;i<3*mMesh.getNumTriangles();i++)
			{
				stream.read(&x, sizeof(PxU8));
				*tris16++ = x;
			}
		}
		else
		{
			PxU32* tris32 = reinterpret_cast<PxU32*>(tris);
			for(PxU32 i=0;i<3*mMesh.getNumTriangles();i++)
			{
				stream.read(&x, sizeof(PxU8));
				*tris32++ = x;
			}
		}
	}
	else if(serialFlags & IMSF_16BIT_INDICES)
	{
		PxU16 x;
		if(mMesh.has16BitIndices())
		{
			PxU16* tris16 = reinterpret_cast<PxU16*>(tris);
			if(mismatch)
				for(PxU32 i=0;i<3*mMesh.getNumTriangles();i++)
					stream.read(&x, sizeof(PxU16)), *tris16++ = flip(&x);
			else
				stream.read(tris16, 3*sizeof(PxU16)*mMesh.getNumTriangles());
				//for(PxU32 i=0;i<3*mMesh.getNumTriangles();i++)
				//	*tris16++ = stream.readWord();
		}
		else
		{
			PxU32* tris32 = reinterpret_cast<PxU32*>(tris);
			if(mismatch)
				for(PxU32 i=0;i<3*mMesh.getNumTriangles();i++)
					stream.read(&x, sizeof(PxU16)), *tris32++ = flip(&x);
			else
				for(PxU32 i=0;i<3*mMesh.getNumTriangles();i++)
				{
					stream.read(&x, sizeof(PxU16));
					*tris32++ = x;
				}
		}

	}
	else
	{
		PxU32 x;
		if(mMesh.has16BitIndices())
		{
			PxU16* tris16 = reinterpret_cast<PxU16*>(tris);
			if(mismatch)
				for(PxU32 i=0;i<3*mMesh.getNumTriangles();i++)
					{ stream.read(&x, sizeof(PxU32)); PX_ASSERT(x <= 0xffff); *tris16++ = (PxU16)flip(&x); }
			else
				for(PxU32 i=0;i<3*mMesh.getNumTriangles();i++)
					{ stream.read(&x, sizeof(PxU32)); PX_ASSERT(x <= 0xffff); *tris16++ = (PxU16)x; }
		}
		else
		{
			PxU32* tris32 = reinterpret_cast<PxU32*>(tris);
			if(mismatch)
				for(PxU32 i=0;i<3*mMesh.getNumTriangles();i++)
					 stream.read(&x, sizeof(PxU32)), *tris32++ = flip(&x);
			else
				stream.read(tris32, 3*sizeof(PxU32)*mMesh.getNumTriangles());
				//for(PxU32 i=0;i<3*mMesh.getNumTriangles();i++)
				//	*tris32++ = stream.readDword();
		}
	}

	if(serialFlags & IMSF_MATERIALS)
	{
		PxU16* materials = mMesh.allocateMaterials();
		stream.read(materials, sizeof(PxU16)*mMesh.getNumTriangles());
		if(mismatch)
		{
			for(PxU32 i=0;i<mMesh.getNumTriangles();i++)
				materials[i] = flip(&materials[i]);
		}
	}
	if(serialFlags & IMSF_FACE_REMAP)
	{
		PxU32* remap = mMesh.allocateFaceRemap();
/*		stream.readBuffer(remap, sizeof(PxU32)*mMesh.getNumTriangles());
		if(mismatch)
			{
			for(PxU32 i=0;i<mMesh.getNumTriangles();i++)
				remap[i] = flip(&remap[i]);
			}*/
		readIndices(readDword(mismatch, stream), mMesh.getNumTriangles(), remap, stream, mismatch);
	}

	if(serialFlags & IMSF_ADJACENCIES)
	{
		PxU32* adj = mMesh.allocateAdjacencies();
		stream.read(adj, sizeof(PxU32)*mMesh.getNumTriangles()*3);
		if(mismatch)
		{
			for(PxU32 i=0;i<mMesh.getNumTriangles()*3;i++)
			{
				adj[i] = flip(&adj[i]);
			}
		}		
	}

	if(!mMesh.loadRTree(stream, version))
		return false;

	// Import local bounds
	mMesh.mData.mOpcodeModel.mGeomEpsilon	= readFloat(mismatch, stream);
//	mMesh.mData.mOpcodeModel.mGeomEpsilon	= 1e-5f;
	mMesh.mData.mAABB.minimum.x		= readFloat(mismatch, stream);
	mMesh.mData.mAABB.minimum.y		= readFloat(mismatch, stream);
	mMesh.mData.mAABB.minimum.z		= readFloat(mismatch, stream);
	mMesh.mData.mAABB.maximum.x		= readFloat(mismatch, stream);
	mMesh.mData.mAABB.maximum.y		= readFloat(mismatch, stream);
	mMesh.mData.mAABB.maximum.z		= readFloat(mismatch, stream);

	PxU32 nb = readDword(mismatch, stream);
	if(nb)
	{
		PX_ASSERT(nb==mMesh.getNumTriangles());
		mMesh.mData.mExtraTrigData = PX_NEW(PxU8)[nb];
		// No need to convert those bytes
		stream.read(mMesh.mData.mExtraTrigData, nb*sizeof(PxU8));
	}

	return true;
}

void Gu::TriangleMesh::release()
{
	mMeshFactory->notifyFactoryListener(this, PxConcreteType::eTRIANGLE_MESH, false);

	mBaseFlags &= ~PxBaseFlag::eIS_RELEASABLE;
	decRefCount();
}

PxU32 Gu::TriangleMesh::getReferenceCount() const
{
	return getRefCount();
}


#if PX_ENABLE_DEBUG_VISUALIZATION

// PT: don't use a template when you don't need one. Don't pollute the header with template definition. Don't duplicate the code when one version is enough. Etc.
static void getTriangle(const Gu::TriangleMesh&, PxU32 i, PxVec3* wp, const PxVec3* vertices, const void* indices, bool has16BitIndices)
{
	PxU32 ref0, ref1, ref2;

	if(!has16BitIndices)
	{
		const PxU32* dtriangles = reinterpret_cast<const PxU32*>(indices);
		ref0 = dtriangles[i*3+0];
		ref1 = dtriangles[i*3+1];
		ref2 = dtriangles[i*3+2];
	}
	else
	{
		const PxU16* wtriangles = reinterpret_cast<const PxU16*>(indices);
		ref0 = wtriangles[i*3+0];
		ref1 = wtriangles[i*3+1];
		ref2 = wtriangles[i*3+2];
	}

	wp[0] = vertices[ref0];
	wp[1] = vertices[ref1];
	wp[2] = vertices[ref2];
}

static void getTriangle(const Gu::TriangleMesh& mesh, PxU32 i, PxVec3* wp, const PxVec3* vertices, const void* indices, const Cm::Matrix34& absPose, bool has16BitIndices)
{
	PxVec3 localVerts[3];
	getTriangle(mesh, i, localVerts, vertices, indices, has16BitIndices);

	wp[0] = absPose.transform(localVerts[0]);
	wp[1] = absPose.transform(localVerts[1]);
	wp[2] = absPose.transform(localVerts[2]);
}



void Gu::TriangleMesh::debugVisualize(
	Cm::RenderOutput& out, const Cm::Matrix34& absPose, const PxMeshScale& scaling, const PxBounds3& cullbox,
	const PxU64 mask, const PxReal fscale, const PxU32 numMaterials)const 
{
	PX_UNUSED(numMaterials);

	//bool cscale = !!(mask & ((PxU64)1 << PxVisualizationParameter::eCULL_BOX));
	const PxU64 cullBoxMask = (PxU64)1 << PxVisualizationParameter::eCULL_BOX;
	bool cscale = ((mask & cullBoxMask) == cullBoxMask);


	const PxMat44 midt = PxMat44(PxIdentity);

	PxU32 nbTriangles = mMesh.getNumTriangles();
	const PxU32 nbVertices = mMesh.getNumVertices();
	const PxVec3* vertices = mMesh.getVertices();
	const void* indices = getTrianglesFast();

	const bool has16BitIndices = mMesh.has16BitIndices();

	PxDebugColor::Enum colors[] = 
	{
		PxDebugColor::eARGB_BLACK,		
		PxDebugColor::eARGB_RED,		
		PxDebugColor::eARGB_GREEN,		
		PxDebugColor::eARGB_BLUE,		
		PxDebugColor::eARGB_YELLOW,	
		PxDebugColor::eARGB_MAGENTA,	
		PxDebugColor::eARGB_CYAN,		
		PxDebugColor::eARGB_WHITE,		
		PxDebugColor::eARGB_GREY,		
		PxDebugColor::eARGB_DARKRED,	
		PxDebugColor::eARGB_DARKGREEN,	
		PxDebugColor::eARGB_DARKBLUE,	
	};

	PxU32 colorCount = sizeof(colors)/sizeof(PxDebugColor::Enum);


	if(cscale)
	{
		Gu::Box worldBox;
		worldBox.center = (cullbox.maximum + cullbox.minimum)*0.5f;
		worldBox.extents = (cullbox.maximum - cullbox.minimum)*0.5f;
		worldBox.rot = PxMat33(PxIdentity);
		
		PxU32* results = (PxU32*)PX_ALLOC_TEMP(sizeof(PxU32)*nbTriangles*3, PX_DEBUG_EXP("tmp triangle indices"));
		PxU32 maxResults = nbTriangles*3;
		bool overflow = false;
		const Gu::TriangleMesh* tm = static_cast<const Gu::TriangleMesh*>(this);

		PxTransform meshTransform;
		PxMat33 meshRot = PxMat33(absPose.base0, absPose.base1, absPose.base2);
		meshTransform.p = absPose.base3;
		meshTransform.q = PxQuat(meshRot);
		nbTriangles = Gu::findOverlapOBBMesh(worldBox, tm->getOpcodeModel(), meshTransform, scaling, results, maxResults, 0, overflow);
		if (fscale)
		{
			const PxU32 fcolor = PxU32(PxDebugColor::eARGB_DARKRED);

			for (PxU32 i=0; i<nbTriangles; i++)
			{
				const PxU32 index= results[i];
				PxVec3 wp[3];
				getTriangle(*this, index, wp, vertices, indices, absPose, has16BitIndices);

				const PxVec3 center = (wp[0] + wp[1] + wp[2]) / 3.0f;
				PxVec3 normal = (wp[0] - wp[1]).cross(wp[0] - wp[2]);
				PX_ASSERT(!normal.isZero());
				normal = normal.getNormalized();

				out << midt << fcolor <<
						Cm::DebugArrow(center, normal * fscale);
			}

		}

		if (mask & ((PxU64)1 << PxVisualizationParameter::eCOLLISION_SHAPES))
		{
		
			const PxU32 scolor = PxU32(PxDebugColor::eARGB_MAGENTA);

			out << midt << scolor;	// PT: no need to output this for each segment!

			PxDebugLine* segments = out.reserveSegments(nbTriangles*3);
			for(PxU32 i=0; i<nbTriangles; i++)
			{
				const PxU32 index = results[i];
				PxVec3 wp[3];
				getTriangle(*this, index, wp, vertices, indices, absPose, has16BitIndices);
				segments[0] = PxDebugLine(wp[0], wp[1], scolor);
				segments[1] = PxDebugLine(wp[1], wp[2], scolor);
				segments[2] = PxDebugLine(wp[2], wp[0], scolor);
				segments+=3;
			}
		}

		if ((mask & ((PxU64)1 << PxVisualizationParameter::eCOLLISION_EDGES)) && mMesh.mData.mExtraTrigData)
		{
			const PxU32 ecolor = PxU32(PxDebugColor::eARGB_YELLOW);
			
			for (PxU32 i=0; i<nbTriangles; i++)
			{
				const PxU32 index = results[i];
				PxVec3 wp[3];
				getTriangle(*this, index, wp, vertices, indices, absPose, has16BitIndices);

				const PxU32 flags = mMesh.getTrigSharedEdgeFlagsFromData(index);

				if(flags & Gu::ETD_CONVEX_EDGE_01)
				{
					out << midt << ecolor << Cm::RenderOutput::LINES << wp[0] << wp[1];
				}
				if(flags & Gu::ETD_CONVEX_EDGE_12)
				{
					out << midt << ecolor << Cm::RenderOutput::LINES << wp[1] << wp[2];
				}
				if(flags & Gu::ETD_CONVEX_EDGE_20)
				{
					out << midt << ecolor << Cm::RenderOutput::LINES << wp[0] << wp[2];
				}
			}
			
		}

		PX_FREE(results);
	}
	else
	{
		if (fscale)
		{
			const PxU32 fcolor = PxU32(PxDebugColor::eARGB_DARKRED);

			for (PxU32 i=0; i<nbTriangles; i++)
			{
				PxVec3 wp[3];
				getTriangle(*this, i, wp, vertices, indices, absPose, has16BitIndices);

				const PxVec3 center = (wp[0] + wp[1] + wp[2]) / 3.0f;
				PxVec3 normal = (wp[0] - wp[1]).cross(wp[0] - wp[2]);
				PX_ASSERT(!normal.isZero());
				normal = normal.getNormalized();

				out << midt << fcolor <<
						Cm::DebugArrow(center, normal * fscale);
			}
		}

		if (mask & ((PxU64)1 << PxVisualizationParameter::eCOLLISION_SHAPES))
		{
			PxU32 scolor = PxU32(PxDebugColor::eARGB_MAGENTA);

			out << midt << scolor;	// PT: no need to output this for each segment!

			PxVec3* transformed = (PxVec3*)PX_ALLOC(sizeof(PxVec3)*nbVertices, PX_DEBUG_EXP("PxVec3"));
			for(PxU32 i=0;i<nbVertices;i++)
				transformed[i] = absPose.transform(vertices[i]);

			PxDebugLine* segments = out.reserveSegments(nbTriangles*3);
			for (PxU32 i=0; i<nbTriangles; i++)
			{
				PxVec3 wp[3];
				getTriangle(*this, i, wp, transformed, indices, has16BitIndices);
				const PxU32 localMaterialIndex = getTriangleMaterialIndex(i);
				scolor = colors[localMaterialIndex % colorCount];
				
				segments[0] = PxDebugLine(wp[0], wp[1], scolor);
				segments[1] = PxDebugLine(wp[1], wp[2], scolor);
				segments[2] = PxDebugLine(wp[2], wp[0], scolor);
				segments+=3;
			}

			PX_FREE(transformed);
		}

		if ((mask & ((PxU64)1 << PxVisualizationParameter::eCOLLISION_EDGES)) && mMesh.mData.mExtraTrigData)
		{
			const PxU32 ecolor = PxU32(PxDebugColor::eARGB_YELLOW);

			for (PxU32 i=0; i<nbTriangles; i++)
			{
				PxVec3 wp[3];
				getTriangle(*this, i, wp, vertices, indices, absPose, has16BitIndices);

				const PxU32 flags = mMesh.getTrigSharedEdgeFlagsFromData(i);

				if(flags & Gu::ETD_CONVEX_EDGE_01)
				{
					out << midt << ecolor << Cm::RenderOutput::LINES << wp[0] << wp[1];
				}
				if(flags & Gu::ETD_CONVEX_EDGE_12)
				{
					out << midt << ecolor << Cm::RenderOutput::LINES << wp[1] << wp[2];
				}
				if(flags & Gu::ETD_CONVEX_EDGE_20)
				{
					out << midt << ecolor << Cm::RenderOutput::LINES << wp[0] << wp[2];
				}
			}
		}
	}
}

#endif

}
