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
#include "../core/feedback.h"
#include "../core/dataError.h"
#include "collisionShape.h"
#include "collisionMesh.h"
#include "meshEnum.h"
#include "collisionCache.h"

#ifdef USE_PHYSX
using namespace physx;
#endif

/// Triangle mesh
class CCollisionShapeTriMesh : public ICollisionShape
{
	DECLARE_ENGINE_CLASS( CCollisionShapeTriMesh, ICollisionShape, 0 );

protected:
	TDynArray< CName, MC_CollisionCache >	m_physicalMaterialNames;
	TDynArray< Vector, MC_CollisionCache >	m_vertices;
	TDynArray< Uint16, MC_CollisionCache >	m_triangles;
	TDynArray< Uint16, MC_CollisionCache >	m_physicalMaterialIndexes;

public:
	CCollisionShapeTriMesh()
	{
		m_physicalMaterialNames.PushBack( CNAME( default ) );
	};

	CCollisionShapeTriMesh( CCollisionMesh* mesh, void* geometry, const TDynArray< Vector >& sourcePoints, const TDynArray< Uint16 >& sourceTriangles, const TDynArray< Uint16 >& physicalMaterialIndexes, const TDynArray< CName >& physicalMaterialNames )
		: ICollisionShape( mesh, geometry )
	{
		m_physicalMaterialNames = physicalMaterialNames;
		m_vertices = sourcePoints;
		m_triangles = sourceTriangles;
		m_physicalMaterialIndexes = physicalMaterialIndexes;
	} 

	virtual Uint32 GetDebugTriangleColor( Uint32 index ) const
	{
		CName materialName;
		Uint32 mtlIdx = 0;
		if ( index < m_physicalMaterialIndexes.Size() )
		{
			mtlIdx = m_physicalMaterialIndexes[index];
		}
		if ( mtlIdx < m_physicalMaterialNames.Size() )
		{
			materialName = m_physicalMaterialNames[mtlIdx];
		}
		return CollisionFalseColoring().MaterialColor( materialName );
	}

	//! Get collision mesh 
	virtual void GetShape( const Matrix& localToWorld, TDynArray< Vector >& vertices, TDynArray< Uint32 >& indices ) const
	{
		Matrix fullMtx = m_pose * localToWorld;

		// Get vertices
		vertices.Resize( m_vertices.Size() );
		for ( Uint32 i=0; i<m_vertices.Size(); i++ )
		{
			vertices[i] = fullMtx.TransformPoint( m_vertices[i] );
		}

		// Get indices
		indices.Resize( m_triangles.Size() );
		for ( Uint32 i=0; i<m_triangles.Size(); i++ )
		{
			indices[i] = (Uint16) m_triangles[i];
		}
	}

	//! Get shape stats - number of vertices, number of indices
	virtual void GetStats( Uint32& numVertices, Uint32& numIndices ) const
	{
		numVertices = m_vertices.Size();
		numIndices = m_triangles.Size();
	}

	//! Compile shape
	virtual Bool CompileShape( struct SCachedGeometry* resultGeometry, const CObject* parentObject ) const
	{
#if defined( USE_PHYSX )
		if( m_vertices.Empty() || m_triangles.Empty() )
		{
#ifndef RED_FINAL_BUILD
			if ( ! m_vertices.Size() || ! m_triangles.Size() )
			{
				DATA_HALT( DES_Major, GetCollisionShapeParentResource( GetParent() ), TXT("Physical collision"), TXT("Collision representation is empty") );
			}
#endif
			return false;
		}

		if( m_physicalMaterialIndexes.Size() < m_triangles.Size() / 3 )
		{
			DATA_HALT( DES_Major, GetCollisionShapeParentResource( GetParent() ), TXT("Physical collision"), TXT("Fail on cooking trimesh collision, number of material indexes differs from number of triangles in trimesh. Asset needs to be reexported from max") );
			return false;
		}

		// Build transformed points
        TDynArray< PxVec3 > pxPoints( m_vertices.Size() );
        const Uint32 num = pxPoints.Size();
        for ( Uint32 i=0; i<num; i++ )
        {
            pxPoints[ i ] = TO_PX_VECTOR(m_vertices[ i ]);
        }

        PxTriangleMeshDesc triangleMeshDesc;
        triangleMeshDesc.points.count = pxPoints.Size();
        triangleMeshDesc.points.stride = sizeof( PxVec3 );
        triangleMeshDesc.points.data = pxPoints.Data();
        triangleMeshDesc.triangles.count = m_triangles.Size() / 3;
        triangleMeshDesc.triangles.stride = 3 * sizeof( Uint16 );
        triangleMeshDesc.triangles.data = m_triangles.Data();
        triangleMeshDesc.flags = PxMeshFlag::e16_BIT_INDICES;
		triangleMeshDesc.materialIndices.data = ( const physx::PxMaterialTableIndex* ) m_physicalMaterialIndexes.Data();
		triangleMeshDesc.materialIndices.stride = sizeof( unsigned short );

		for( Uint32 i = 0; i != m_physicalMaterialIndexes.Size(); ++i )
		{
			if( m_physicalMaterialIndexes[ i ] >= m_physicalMaterialNames.Size() )
			{
				DATA_HALT( DES_Major, GetCollisionShapeParentResource( GetParent() ), TXT("Physical collision"), TXT("Fail on cooking trimesh collision, material names with with material indices doesnt match. Asset needs to be reexported from max ") );
				return false;
			}
		}

		ASSERT( triangleMeshDesc.isValid() );
        PxCooking* cooking = GPhysXEngine->GetCooking();
        MemoryWriteBuffer buf;
        if ( !cooking->cookTriangleMesh( triangleMeshDesc, buf ) )
        {
#ifndef RED_FINAL_BUILD
			DATA_HALT( DES_Major, GetCollisionShapeParentResource( GetParent() ), TXT("Physical collision"), TXT("Fail on cooking trimesh collision ") );
#endif	// RED_FINAL_BUILD
			return false;
        }

		MemoryWriteBuffer savebuf;
		cooking->cookTriangleMesh( triangleMeshDesc, savebuf );
		MemoryReadBuffer saveBuffer(savebuf.data);
		resultGeometry->AllocateCompiledData( savebuf.maxSize );
		saveBuffer.read(resultGeometry->GetCompiledData(), savebuf.maxSize);

		MemoryReadBuffer mrb(buf.data);
        PxTriangleMesh* triangleMesh = GPhysXEngine->GetPxPhysics()->createTriangleMesh( mrb );

		resultGeometry->SetGeometry( ( char ) PxGeometryType::eTRIANGLEMESH, PxTriangleMeshGeometry( triangleMesh, PxMeshScale(), PxMeshGeometryFlags( PxMeshGeometryFlag::eDOUBLE_SIDED ) ) );
		resultGeometry->m_physicalMultiMaterials = m_physicalMaterialNames;
		resultGeometry->m_pose = GetPose();
		resultGeometry->m_densityScaler = m_densityScaler;

		return true;
#else
		return false;
#endif		//defined( USE_PHYSX ) 
    }

	virtual Uint32 GetNumPhysicalMaterials() const
	{
		// Maximum number of materials is one per triangle.
		return m_triangles.Size() / 3;
	}

	virtual CName GetPhysicalMaterial( Uint32 index ) const
	{
		if( m_physicalMaterialIndexes.Size() <= index ) return CNAME( default );

		Uint32 mtlIdx = m_physicalMaterialIndexes[ index ];

		if( m_physicalMaterialNames.Size() <= mtlIdx ) return CNAME( default );
		return m_physicalMaterialNames[ mtlIdx ];
	}

	virtual void SetPhysicalMaterial( const CName& materialName, Uint32 firstIndex, Uint32 count )
	{
		Uint32 maxNumMaterials = GetNumPhysicalMaterials();
		if ( firstIndex >= maxNumMaterials ) return;

		// Check if we know about this material.
		ptrdiff_t mtlIndex = m_physicalMaterialNames.GetIndex( materialName );
		if ( mtlIndex == -1 )
		{
			// Not found, so add it.
			mtlIndex = ( ptrdiff_t )m_physicalMaterialNames.Size();
			m_physicalMaterialNames.PushBack( materialName );
		}

		ASSERT( mtlIndex >= 0 && mtlIndex <= NumericLimits<Uint16>::Max() );
		Uint32 end = Min( firstIndex + count, maxNumMaterials );

		// If we don't have enough array space, grow the index array.
		if ( end > m_physicalMaterialIndexes.Size() )
		{
			m_physicalMaterialIndexes.Resize( end );
		}

		for ( Uint32 i = firstIndex; i < end; ++i )
		{
			m_physicalMaterialIndexes[i] = ( Uint16 )mtlIndex;
		}

		// Any existing instances of this shape are no longer correct, so we need to recreate them...
		Invalidate();
	}

};

BEGIN_CLASS_RTTI_EX( CCollisionShapeTriMesh, CF_EditorOnly );
	PARENT_CLASS( ICollisionShape );
	PROPERTY( m_physicalMaterialNames );
	PROPERTY( m_vertices );
	PROPERTY( m_triangles );
	PROPERTY( m_physicalMaterialIndexes );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CCollisionShapeTriMesh );

ICollisionShape* CCollisionMesh::AddTriMesh( const Matrix& localToMesh, const Vector* vertices, Uint32 vertexCount, const Uint32* triangleIndices, Uint32 triangleCount, const TDynArray< unsigned short >& triangleMaterials, const TDynArray< CName >& physicalMaterialNames, Bool flipWinding )
{
#if !defined( USE_PHYSX )
	return 0;
#else
	CTimeCounter timeCounter;

	// Empty mesh
	if ( !vertexCount || !triangleCount )
	{
		return NULL;
	}

	// To many vertices
	if ( vertexCount > 65535 )
	{
		GFeedback->ShowError( TXT("Mesh is too complex to have exact collision") );
		return NULL;
	}

    // Transform vertices
    TDynArray< Vector > transformedVertices;
    transformedVertices.Resize( vertexCount );
    for ( Uint32 i=0; i < vertexCount; i++ )
    {
        transformedVertices[i] = localToMesh.TransformPoint( vertices[i] );
    }

    PxTriangleMeshDesc triangleMeshDesc;
    triangleMeshDesc.points.count = transformedVertices.Size();
    triangleMeshDesc.points.stride = sizeof( Vector );
    triangleMeshDesc.points.data = transformedVertices.Data();
    triangleMeshDesc.triangles.count = triangleCount;
    triangleMeshDesc.triangles.stride = 3 * sizeof( Uint32 );
    triangleMeshDesc.triangles.data = triangleIndices;
	triangleMeshDesc.materialIndices.data = triangleMaterials.TypedData();
	triangleMeshDesc.materialIndices.stride = sizeof( Uint16 );

    PxCooking* cooking = GPhysXEngine->GetCooking();
    MemoryWriteBuffer buf;
    if ( !cooking->cookTriangleMesh( triangleMeshDesc, buf ) )
    {
        return NULL;
    }

	MemoryReadBuffer mrb(buf.data);
    PxTriangleMesh* triangleMesh = GPhysXEngine->GetPxPhysics()->createTriangleMesh( mrb );

	// Create wrapping shape
    PxTriangleMeshGeometry* geom = new PxTriangleMeshGeometry( triangleMesh );

	TDynArray< Uint16 > indices( 3 * triangleCount );

	if ( flipWinding )
	{
		for ( Uint32 i = 0; i < 3 * triangleCount; i += 3 )
		{
			indices[i + 0] = (Uint16)triangleIndices[i + 2];
			indices[i + 1] = (Uint16)triangleIndices[i + 1];
			indices[i + 2] = (Uint16)triangleIndices[i + 0];
		}
	}
	else
	{
		for ( Uint32 i = 0; i < 3 * triangleCount; ++i )
		{
			indices[i] = (Uint16)triangleIndices[i];
		}
	}

	return AddShape( new CCollisionShapeTriMesh( this, geom, transformedVertices, indices, triangleMaterials, physicalMaterialNames ) );
#endif
}

ICollisionShape* CCollisionMesh::AddTriMesh( const Matrix& localToMesh, const CMesh* mesh, const TDynArray< CName >& physicalMaterialNames )
{
#if !defined( USE_PHYSX ) || defined( NO_RESOURCE_IMPORT )
	return 0;
#else
	// No mesh :)
	if ( !mesh )
	{
		return 0;
	}

	// Extract data from mesh
	TDynArray< Vector > meshVertices;
	TDynArray< Uint32 > meshIndices;
	TDynArray< unsigned short > meshMaterialIndices;
	mesh->GetCollisionData( meshVertices, meshIndices, meshMaterialIndices );

	// Flip winding order... imported collision data works fine, but when we generate it from the CMesh, we need to flip the triangles for it to be proper...
	return AddTriMesh( localToMesh, meshVertices.TypedData(), meshVertices.Size(), meshIndices.TypedData(), meshIndices.Size() / 3, meshMaterialIndices, physicalMaterialNames, true );
#endif
}
