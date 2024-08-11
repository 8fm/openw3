/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "collisionColoring.h"
#include "../physics/physicsEngine.h"
#include "../physics/physicsWorld.h"
#include "../physics/PhysXStreams.h"
#include "../physics/compiledCollision.h"
#include "collisionShapeBuilder.h"
#include "collisionShape.h"
#include "collisionMesh.h"

#ifdef USE_PHYSX
using namespace physx;
#endif

/// Simple convex
class CCollisionShapeCapsule : public ICollisionShape
{
	DECLARE_ENGINE_CLASS( CCollisionShapeCapsule, ICollisionShape, 0 );

protected:
	CName m_physicalMaterialName;
	Float m_radius;
	Float m_height;

public:
	CCollisionShapeCapsule() {};

	CCollisionShapeCapsule( CCollisionMesh* mesh, void* geometry, const CName& physicalMaterialName, const Matrix& pose, float radius, float height )
		: ICollisionShape( mesh, geometry, pose )
		, m_physicalMaterialName( physicalMaterialName )
		, m_radius( radius )
		, m_height( height )
	{
	}

	virtual Uint32 GetDebugTriangleColor( Uint32 index ) const
	{
		return CollisionFalseColoring().MaterialColor( m_physicalMaterialName );
	}


	virtual void GetStats( Uint32& numVertices, Uint32& numIndices ) const
	{
		CollisionShapeBuilder::GetCapsuleStats( m_radius, numVertices, numIndices );
	}

	//! Get collision mesh 
	virtual void GetShape( const Matrix& localToWorld, TDynArray< Vector >& vertices, TDynArray< Uint32 >& indices ) const override
	{
		Matrix fullMtx = m_pose * localToWorld;

		vertices.ClearFast();
		indices.ClearFast();
		CollisionShapeBuilder::BuildCapsule( m_radius, m_height / 2.0f, vertices, indices );

		for ( Uint32 i = 0; i < vertices.Size(); ++i )
		{
			vertices[i] = fullMtx.TransformPoint( vertices[i] );
		}
	}

    virtual Bool CompileShape( struct SCachedGeometry* resultGeometry, const CObject* parentObject ) const
    {
#ifdef USE_PHYSX
		resultGeometry->AllocateCompiledData( sizeof( float) * 2 );
		float* sizes = ( float* ) resultGeometry->GetCompiledData();
		sizes[ 0 ] = m_radius;
		sizes[ 1 ] = ( m_height / 2 );

		resultGeometry->SetGeometry( ( char ) PxGeometryType::eCAPSULE, PxCapsuleGeometry( sizes[ 0 ], sizes[ 1 ] ) );
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

	virtual void SetPhysicalMaterial( const CName& materialName, Uint32 firstIndex, Uint32 count )
	{
		m_physicalMaterialName = materialName;
		Invalidate();
	}

};

BEGIN_CLASS_RTTI_EX( CCollisionShapeCapsule, CF_EditorOnly );
	PARENT_CLASS( ICollisionShape );
	PROPERTY( m_physicalMaterialName );
	PROPERTY( m_radius )
	PROPERTY( m_height )
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CCollisionShapeCapsule );

ICollisionShape* CCollisionMesh::AddCapsule( const Matrix& localToMesh, float radius, float height, const CName& physicalMaterialName )
{
#ifndef USE_PHYSX
	return 0;
#else
	PxGeometry* geom = new PxCapsuleGeometry( radius, height / 2 );
	return AddShape( new CCollisionShapeCapsule( this, geom, physicalMaterialName, localToMesh, radius, height ) );
#endif
}

