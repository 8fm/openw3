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
class CCollisionShapeSphere : public ICollisionShape
{
	DECLARE_ENGINE_CLASS( CCollisionShapeSphere, ICollisionShape, 0 );

protected:
	CName m_physicalMaterialName;
	Float m_radius;

public:
	CCollisionShapeSphere() {};

	CCollisionShapeSphere( CCollisionMesh* mesh, void* geometry, const CName& physicalMaterialName, const Matrix& pose, float radius )
		: ICollisionShape( mesh, geometry, pose )
		, m_physicalMaterialName( physicalMaterialName )
		, m_radius( radius )
	{
	}

	virtual Uint32 GetDebugTriangleColor( Uint32 index ) const  { return CollisionFalseColoring().MaterialColor( m_physicalMaterialName ); }

	virtual void GetStats(  Uint32& numVertices, Uint32& numIndices ) const
	{
		CollisionShapeBuilder::GetSphereStats( m_radius, numVertices, numIndices );
	}

	//! Get collision mesh 
	virtual void GetShape( const Matrix& localToWorld, TDynArray< Vector >& vertices, TDynArray< Uint32 >& indices ) const
	{
		Matrix fullMtx = localToWorld * m_pose;

		vertices.ClearFast();
		indices.ClearFast();
		CollisionShapeBuilder::BuildSphere( m_radius, vertices, ( TDynArray< Uint32 >& )indices );

		for ( Uint32 i = 0; i < vertices.Size(); ++i )
		{
			vertices[i] = fullMtx.TransformPoint( vertices[i] );
		}
	}

    virtual Bool CompileShape( struct SCachedGeometry* resultGeometry, const CObject* parentObject ) const
    {
#ifdef USE_PHYSX
		resultGeometry->AllocateCompiledData( sizeof( float ) );

		float* sizes = ( float* ) resultGeometry->GetCompiledData();
		*sizes = m_radius;

		resultGeometry->SetGeometry( ( char ) PxGeometryType::eSPHERE, PxSphereGeometry( *sizes ) );
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

BEGIN_CLASS_RTTI_EX( CCollisionShapeSphere, CF_EditorOnly );
	PARENT_CLASS( ICollisionShape );
	PROPERTY( m_physicalMaterialName );
	PROPERTY( m_radius )
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CCollisionShapeSphere );


ICollisionShape* CCollisionMesh::AddSphere( const Matrix& localToMesh, float radius, const CName& physicalMaterialName )
{
#ifndef USE_PHYSX
	return 0;
#else
	PxGeometry* geom = new PxSphereGeometry( radius );
	return AddShape( new CCollisionShapeSphere( this, geom, physicalMaterialName, localToMesh, radius ) );
#endif
}


