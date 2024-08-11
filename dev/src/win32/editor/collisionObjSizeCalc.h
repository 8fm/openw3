#pragma once

//** ************************
namespace CollisionObjMemCalc
{
	Uint32 CollisionSizeOf( const class CCollisionMesh * pCollisionMesh );
	Uint32 CollisionSizeOf( CMesh * pMesh );

	Uint32 CollisionSizeOf( const class CCollisionMesh * pCollisionMesh, const Vector & scale );
	Uint32 CollisionSizeOf( CMesh * pMesh, const Vector & scale );
	Uint32 CollisionSizeOf( CStaticMeshComponent * pStaticMeshComponent, const Vector & scale );

	Uint32 CollisionSizeOf( const class CCompiledCollision * pCompiledCollisionMesh );
}
