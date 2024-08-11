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
#include "../core/dataError.h"

#ifdef USE_PHYSX
using namespace physx;
#endif

/// Simple convex
class CCollisionShapeBox : public ICollisionShape
{
	DECLARE_ENGINE_CLASS( CCollisionShapeBox, ICollisionShape, 0 );

protected:
	CName m_physicalMaterialName;
	Float m_halfExtendsX;
	Float m_halfExtendsY;
	Float m_halfExtendsZ;

public:
	CCollisionShapeBox() {}
	CCollisionShapeBox( CCollisionMesh* mesh, void* geometry, const CName& physicalMaterialName, const Matrix& pose, const Vector& halfExtends )
		: ICollisionShape( mesh, geometry, pose )
		, m_physicalMaterialName( physicalMaterialName )
		, m_halfExtendsX( halfExtends.X )
		, m_halfExtendsY( halfExtends.Y )
		, m_halfExtendsZ( halfExtends.Z )
	{
	}

	virtual Uint32 GetDebugTriangleColor( Uint32 index ) const
	{
		return CollisionFalseColoring().MaterialColor( m_physicalMaterialName );
	}


	//! Get num indices
	virtual void GetStats( Uint32& numVertices, Uint32& numIndices ) const
	{
		CollisionShapeBuilder::GetBoxStats( numVertices, numIndices );
	}

	//! Get collision mesh 
	virtual void GetShape( const Matrix& localToWorld, TDynArray< Vector >& vertices, TDynArray< Uint32 >& indices ) const
	{
		Matrix fullMtx = m_pose * localToWorld;

		vertices.ClearFast();
		indices.ClearFast();
		CollisionShapeBuilder::BuildBox( Vector( m_halfExtendsX, m_halfExtendsY, m_halfExtendsZ ) * 2.0f, vertices, indices );
		for ( Uint32 i = 0; i < vertices.Size(); ++i )
		{
			vertices[i] = fullMtx.TransformPoint( vertices[i] );
		}
	}

	virtual Bool CompileShape( SCachedGeometry* resultGeometry, const CObject* parentObject ) const
	{
#ifdef USE_PHYSX
		if( m_halfExtendsX == 0.0f || m_halfExtendsY == 0.0f || m_halfExtendsZ == 0.0f )
		{
#ifndef RED_FINAL_BUILD
			DATA_HALT( DES_Major, GetCollisionShapeParentResource( GetParent() ), TXT("Physical collision"), TXT("Fail on creating plane from box ") );
#endif
			return nullptr;
		}

		resultGeometry->AllocateCompiledData( sizeof( PxVec3 ) );
		PxVec3* halfExtends = ( PxVec3* ) resultGeometry->GetCompiledData();
		*halfExtends = PxVec3( m_halfExtendsX, m_halfExtendsY, m_halfExtendsZ );
		resultGeometry->SetGeometry( ( char ) PxGeometryType::eBOX, PxBoxGeometry( *halfExtends ) );
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

BEGIN_CLASS_RTTI_EX( CCollisionShapeBox, CF_EditorOnly );
	PARENT_CLASS( ICollisionShape );
	PROPERTY( m_physicalMaterialName );
	PROPERTY( m_halfExtendsX );
	PROPERTY( m_halfExtendsY );
	PROPERTY( m_halfExtendsZ );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CCollisionShapeBox );


ICollisionShape* CCollisionMesh::AddBox( const Matrix& localToMesh, const Vector& halfExtends, const CName& physicalMaterialName )
{
#ifndef USE_PHYSX
	return 0;
#else
	if( halfExtends.X == 0.0f || halfExtends.Y == 0.0f || halfExtends.Z == 0.0f )
	{
		return nullptr;
	}

	PxGeometry* geom = new PxBoxGeometry( TO_PX_VECTOR( halfExtends ) );
	return AddShape( new CCollisionShapeBox( this, geom, physicalMaterialName, localToMesh, halfExtends ) );
#endif
}

