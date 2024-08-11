/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "collisionColoring.h"
#include "../physics/physicsEngine.h"
#include "../physics/physicsWorld.h"
#include "../physics/PhysXStreams.h"
#include "../physics/compiledCollision.h"
#include "mesh.h"
#include "meshEnum.h"
#include "../core/dataError.h"
#include "collisionShape.h"
#include "collisionMesh.h"
#include "collisionCache.h"

#ifdef USE_PHYSX
using namespace physx;
#endif

/// Simple convex
class CCollisionShapeConvex : public ICollisionShape
{
	DECLARE_ENGINE_CLASS( CCollisionShapeConvex, ICollisionShape, 0 );

protected:
	CName										m_physicalMaterialName;
	TDynArray< Vector, MC_CollisionCache >		m_vertices;		//!< Convex vertices
	TDynArray< Vector, MC_CollisionCache >		m_planes;		//!< Convex planes
	TDynArray< Uint16, MC_CollisionCache >		m_polygons;		//!< Polygons p0(n, index0, index1, ..., indexn), p1(n, ... ), ..., pn


public:
	CCollisionShapeConvex()
	{};

	CCollisionShapeConvex( CCollisionMesh* mesh, void* geometry, const CName& physicalMaterialName, const TDynArray< Vector >& sourcePoints, const TDynArray< Uint16 >& sourcePolygons )
		: ICollisionShape( mesh, geometry )
		, m_physicalMaterialName( physicalMaterialName )
	{
		m_vertices = sourcePoints;
		m_polygons = sourcePolygons;
	}
	
	virtual Uint32 GetDebugTriangleColor( Uint32 index ) const
	{
		return CollisionFalseColoring().MaterialColor( m_physicalMaterialName );
	}


	//! Get shape stats - number of vertices, number of indices
	virtual void GetStats( Uint32& numVertices, Uint32& numIndices ) const
	{
		numVertices = m_vertices.Size();
		Uint32 indices = 0;
		for ( Uint32 i = 0, n = m_polygons.Size(); i < n; )
		{
			Uint32 polyVerts = m_polygons[ i ];
			indices += (polyVerts - 2)*3;
			i += polyVerts + 1;
		}
		numIndices = indices;
	}

	//! Get collision mesh 
	virtual void GetShape( const Matrix& localToWorld, TDynArray< Vector >& vertices, TDynArray< Uint32 >& indices ) const
	{
		Uint32 numVerts, numInds;
		GetStats( numVerts, numInds );

		Matrix fullMtx = m_pose * localToWorld;

		// Get vertices
		vertices.Resize( m_vertices.Size() );
		for ( Uint32 i=0; i<m_vertices.Size(); i++ )
		{
			vertices[i] = fullMtx.TransformPoint( m_vertices[i] );
		}

		indices.ClearFast();
		indices.Reserve( numInds );
		Uint32 i = 0;
		while ( i < m_polygons.Size() )
		{
			Uint16 verticesCount = m_polygons[i++];

			ASSERT( verticesCount > 2 );
			Uint16 v0 = m_polygons[i++];
			Uint16 vi_1 = m_polygons[i++];
			Uint16 vi;

			for ( Uint16 v = 2; v < verticesCount; ++v )
			{
				vi = m_polygons[i++];

				// Generate indices
				indices.PushBack( v0 );
				indices.PushBack( vi_1 );
				indices.PushBack( vi );

				vi_1 = vi;
			}
		}
	}

	virtual Bool CompileShape( struct SCachedGeometry* resultGeometry, const CObject* parentObject ) const
	{
#ifdef USE_PHYSX
		if( m_vertices.Empty() || m_polygons.Empty() )
		{
#ifndef RED_FINAL_BUILD
			DATA_HALT( DES_Major, GetCollisionShapeParentResource( GetParent() ), TXT("Physical collision"), TXT("Collision representation is empty") );
#endif
			return false;
		}

        // Build transformed points
        TDynArray< PxVec3 > pxPoints( m_vertices.Size() );
        const Uint32 num = pxPoints.Size();
        for ( Uint32 i=0; i < num; i++ )
        {
            pxPoints[ num - i - 1 ] = TO_PX_VECTOR( m_vertices[ i ] );
        }

        PxConvexMeshDesc convexDesc;
        convexDesc.points.count = pxPoints.Size();
        convexDesc.points.stride = sizeof( PxVec3 );
        convexDesc.points.data = pxPoints.Data();
        convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

        PxCooking* cooking = GPhysXEngine->GetCooking();
        MemoryWriteBuffer buf;
        if ( !cooking->cookConvexMesh( convexDesc, buf ) )
        {
			// If it failed to create the convex mesh, try again, but inflate it a bit. According to PX docs, this is quite
			// effective for fixing many issues caused by complex geometry.
			convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX | PxConvexFlag::eINFLATE_CONVEX ;
			if ( !cooking->cookConvexMesh( convexDesc, buf ) )
			{
#ifndef RED_FINAL_BUILD
				DATA_HALT( DES_Major, GetCollisionShapeParentResource( GetParent() ), TXT("Physical collision"), TXT("Fail on cooking trimesh collision ") );
#endif
				return false;
			}
        }

		MemoryWriteBuffer savebuf;
		cooking->cookConvexMesh( convexDesc, savebuf );
		MemoryReadBuffer saveBuffer(savebuf.data);
		resultGeometry->AllocateCompiledData( savebuf.maxSize );
		saveBuffer.read(resultGeometry->GetCompiledData(), savebuf.maxSize);

		MemoryReadBuffer mrb(buf.data);
        PxConvexMesh* convexMesh = GPhysXEngine->GetPxPhysics()->createConvexMesh( mrb );

		resultGeometry->SetGeometry( ( char ) PxGeometryType::eCONVEXMESH, PxConvexMeshGeometry( convexMesh ) );
		resultGeometry->m_physicalSingleMaterial = m_physicalMaterialName;
		resultGeometry->m_pose = GetPose();
		resultGeometry->m_densityScaler = m_densityScaler;

		return true;
#else
		return false;
#endif
	}

	virtual Uint32 GetNumPhysicalMaterials() const { return 1; }

	virtual CName GetPhysicalMaterial( Uint32 index ) const
	{
		return m_physicalMaterialName;
	}

	virtual void SetPhysicalMaterial(const CName& materialName, Uint32 firstIndex, Uint32 count )
	{
		m_physicalMaterialName = materialName;
		Invalidate();
	}
};

BEGIN_CLASS_RTTI_EX( CCollisionShapeConvex, CF_EditorOnly );
	PARENT_CLASS( ICollisionShape );
	PROPERTY( m_physicalMaterialName );
	PROPERTY( m_vertices );
	PROPERTY( m_planes );
	PROPERTY( m_polygons );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CCollisionShapeConvex );

ICollisionShape* CCollisionMesh::AddConvex( const Matrix& localToMesh, const Vector* vertices, Uint32 count, const CName& physicalMaterialName )
{
#if !defined( USE_PHYSX )
	return 0;
#else
    CTimeCounter timeCounter;

    // To few vertices
    if ( count < 4 )
    {
        return NULL;
    }

    // Transform vertices
    TDynArray< Vector > transformedVertices;
    transformedVertices.Resize( count );
    for ( Uint32 i=0; i<count; i++ )
    {
        transformedVertices[i] = localToMesh.TransformPoint( vertices[i] );
    }

    PxConvexMeshDesc convexDesc;
    convexDesc.points.count = transformedVertices.Size();
    convexDesc.points.stride = sizeof( Vector );
    convexDesc.points.data = transformedVertices.Data();
    convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

    PxCooking* cooking = GPhysXEngine->GetCooking();
    MemoryWriteBuffer buf;
    if ( !cooking->cookConvexMesh( convexDesc, buf ) )
    {
		// If it failed to create the convex mesh, try again, but inflate it a bit. According to PX docs, this is quite
		// effective for fixing many issues caused by complex geometry.
		convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX | PxConvexFlag::eINFLATE_CONVEX ;
		if ( !cooking->cookConvexMesh( convexDesc, buf ) )
		{
			return NULL;
		}
    }

	MemoryReadBuffer mrb(buf.data);
    PxConvexMesh* convexMesh = GPhysXEngine->GetPxPhysics()->createConvexMesh( mrb );

	// No geometry was built
    if ( !convexMesh || convexMesh->getNbVertices() == 0 || convexMesh->getNbPolygons() == 0 )
    {
        return NULL;
    }

    // Extract source points
    TDynArray< Vector > sourcePoints( convexMesh->getNbVertices() );
    for ( Uint32 i=0; i < sourcePoints.Size(); i++ )
    {
        const PxVec3* vertices = convexMesh->getVertices();
        sourcePoints[i] = TO_VECTOR( vertices[i] );
    }

    const PxU8* indexBuffer = convexMesh->getIndexBuffer();
    // Extract source triangles
    TDynArray< Uint16 > sourcePolygons;//( convexMesh->getNbPolygons() * 3 );
    for ( Uint32 i=0; i < convexMesh->getNbPolygons(); i++ )
    {
        //const hkGeometry::Triangle& tri = geometry.m_triangles[i];
        PxHullPolygon data;
        convexMesh->getPolygonData(i, data);
        ASSERT( data.mNbVerts >= 3 );
		sourcePolygons.PushBack( data.mNbVerts );
		for ( Uint32 vi = 0; vi < data.mNbVerts; ++vi )
		{
			sourcePolygons.PushBack( (Uint16)indexBuffer[data.mIndexBase + vi] );
		}
    }

	PxConvexMeshGeometry* geom = new PxConvexMeshGeometry( convexMesh );
	return AddShape( new CCollisionShapeConvex( this, geom, physicalMaterialName, sourcePoints, sourcePolygons ) );
#endif
}


ICollisionShape* CCollisionMesh::AddConvex( const Matrix& localToMesh, const CMesh* mesh, const CName& physicalMaterialName )
{
#if !defined(USE_PHYSX) || defined(NO_RESOURCE_IMPORT)
	return 0;
#else
	// No mesh, no collision
	if ( !mesh )
	{
		return NULL;
	}

	// Extract vertices
	TDynArray< Vector > collisionVertices;
	mesh->GetCollisionData( collisionVertices );

	// Build convex shape from those vertices
	return AddConvex( localToMesh, collisionVertices.TypedData(), collisionVertices.Size(), physicalMaterialName );
#endif
}
