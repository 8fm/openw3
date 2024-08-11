#include "build.h"
#include "collisionObjSizeCalc.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/collisionCache.h"
#include "../../common/engine/staticMeshComponent.h"
#include "../../common/physics/compiledCollision.h"


//** *****************************
//**
Uint32 CollisionObjMemCalc::CollisionSizeOf( const CCollisionMesh * pCollisionMesh )
{
	return 0;
}

//** *****************************
//**
Uint32 CollisionObjMemCalc::CollisionSizeOf( CMesh * pMesh )
{
	if( pMesh )
	{
		return CollisionSizeOf( pMesh->GetCollisionMesh() );
	}

	return 0;
}

//** *****************************
//**
Uint32 CollisionObjMemCalc::CollisionSizeOf( const CCollisionMesh * pCollisionMesh, const Vector & scale )
{

	return 0;
}

//** *****************************
//**
Uint32 CollisionObjMemCalc::CollisionSizeOf( CMesh * pMesh, const Vector & scale )
{
	if( pMesh )
	{
		return CollisionSizeOf( pMesh->GetCollisionMesh(), scale );
	}

	return 0;
}

//** *****************************
//**
Uint32 CollisionObjMemCalc::CollisionSizeOf( CStaticMeshComponent * pStaticMeshComponent, const Vector & scale )
{
	if( pStaticMeshComponent )
	{
		return CollisionSizeOf( pStaticMeshComponent->GetMeshNow(), scale );
	}

	return 0;
}

//** *****************************
//**
Uint32 CollisionObjMemCalc::CollisionSizeOf( const CCompiledCollision * pCompiledCollisionMesh )
{
#ifdef USE_HAVOK
	if( pCompiledCollisionMesh )
	{
		return sizeof( CCompiledCollisionMesh ) + pCompiledCollisionMesh->GetHavokDataBufferSize();
	}
#endif

	return 0;
}
