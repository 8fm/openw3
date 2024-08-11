#include "build.h"
#include "physicsWorldPhysxImplBatchTrace.h"

#ifdef USE_PHYSX
using namespace physx;
#endif

#ifdef USE_APEX
#include "NxApexSDK.h"
#include "NxApexScene.h"
#endif

IMPLEMENT_RTTI_ENUM( EBatchQueryQueryFlag );

#ifdef USE_PHYSX
PxQueryHitType::Enum BatchPreFilterShader(
	PxFilterData filterData0,
	PxFilterData filterData1,
	const void* constantBlock, PxU32 constantBlockSize,
	const PxQueryHit& hit)
{
	SPhysicalFilterData& data0 = ( SPhysicalFilterData& ) filterData0;
	SPhysicalFilterData& data1 = ( SPhysicalFilterData& ) filterData1;

	if( data0.GetCollisionType() )
	{
		if( data0.GetCollisionType() & data1.GetCollisionType() )
		{
			return PxQueryHitType::eTOUCH;
		}
	}
	if( data0.GetFlags() != 0 )
	{
		if( data0.GetFlags() & data1.GetFlags() )
		{
			return PxQueryHitType::eTOUCH;
		}
	}

	return PxQueryHitType::eNONE;
};

#endif

CPhysicsWorldBatchQuery::CPhysicsWorldBatchQuery( )
	: m_raycastSlotsUsed( 0 )
	, m_sweepSlotsUsed( 0 )
#ifdef USE_PHYSX
	, m_scene( 0 )
	, m_query( 0 )
#endif
{
}

CPhysicsWorldBatchQuery::~CPhysicsWorldBatchQuery()
{
#ifdef USE_PHYSX
	if( m_query ) m_query->release();
#endif

}

void CPhysicsWorldBatchQuery::Init( Uint32 raycastPrealocate, Uint32 maxRaycastHits, Uint32 sweepPrealocate, Uint32 maxSweepHits )
{
#ifdef USE_PHYSX
	if( m_query ) m_query->release();

	if( raycastPrealocate )
	{
		if( m_raycastQueryResult.Size() < raycastPrealocate )
		{
			m_raycastQueryResult.Resize( raycastPrealocate );
		}

		if( m_raycastHit.Size() < maxRaycastHits )
		{
			m_raycastHit.Resize( maxRaycastHits );
		}
	}

	if( sweepPrealocate )
	{
		if( m_sweepQueryResult.Size() < sweepPrealocate )
		{
			m_sweepQueryResult.Resize( sweepPrealocate );
		}

		if( m_sweepHit.Size() < maxSweepHits )
		{
			m_sweepHit.Resize( maxSweepHits );
		}
	}

	PxBatchQueryDesc desc( raycastPrealocate, sweepPrealocate, 0 );

	desc.queryMemory.userRaycastResultBuffer = m_raycastQueryResult.TypedData();
	desc.queryMemory.userRaycastTouchBuffer = m_raycastHit.TypedData();
	desc.queryMemory.raycastTouchBufferSize = m_raycastHit.Size();

	desc.queryMemory.userSweepResultBuffer = m_sweepQueryResult.TypedData();
	desc.queryMemory.userSweepTouchBuffer = m_sweepHit.TypedData();
	desc.queryMemory.sweepTouchBufferSize = m_sweepHit.Size();

	desc.preFilterShader = ( PxBatchQueryPreFilterShader) BatchPreFilterShader;
	m_query = m_scene->createBatchQuery( desc );
#endif
	m_raycastSlotsUsed = 0;
	m_sweepSlotsUsed = 0;
}

Int8 CPhysicsWorldBatchQuery::Process()
{
	PC_SCOPE_PHYSICS( Physics world trace batch );
#ifdef USE_PHYSX

#ifdef USE_APEX
	NxApexScene* apexScene = ( NxApexScene* ) m_scene->userData;
	CPhysicsWorldPhysXImpl* world = ( CPhysicsWorldPhysXImpl* ) apexScene->userData;
#else
	CPhysicsWorldPhysXImpl* world = ( CPhysicsWorldPhysXImpl* ) m_scene->userData;
#endif
	Int8 retVal = 0;
	world->SceneUsageAddRef();
	if( !world->IsSceneWhileProcessing() )
	{
		m_query->execute();
	}
	else
	{
		RED_LOG( PhysicalTrace, TXT( "failed CPhysicsWorldBatchQuery processed while scene fetch" ) );
		retVal = -1;
	}
	world->SceneUsageRemoveRef();
	return retVal;
#else
	return -1;
#endif
}

Uint32 CPhysicsWorldBatchQuery::IsSlotLeftForNewRaycast()
{
#ifndef USE_PHYSX
	return 0;
#else
	return m_raycastSlotsUsed < m_raycastQueryResult.Size();
#endif
}

Uint32 CPhysicsWorldBatchQuery::GetRaycastBufferSize()
{
#ifndef USE_PHYSX
	return 0;
#else
	return m_raycastQueryResult.Size();
#endif
}

Uint32 CPhysicsWorldBatchQuery::IsSlotLeftForNewSweep()
{
#ifndef USE_PHYSX
	return 0;
#else
	return m_sweepSlotsUsed < m_sweepQueryResult.Size();
#endif
}

Uint32 CPhysicsWorldBatchQuery::GetSweepBufferSize()
{
#ifndef USE_PHYSX
	return 0;
#else
	return m_sweepQueryResult.Size();
#endif
}

void CPhysicsWorldBatchQuery::Raycast( const Vector& origin, const Vector& unitDir, Float distance, const SPhysicalFilterData& filterData, Uint16 outputFlags, void* userData )
{
#ifdef USE_PHYSX
	if( !unitDir.IsNormalized3() )
	{
		return;
	}
	if( !IsFinite( distance ) )
	{
		return;
	}
	PxQueryFilterData data( filterData.m_data, PxSceneQueryFilterFlag::eSTATIC | PxSceneQueryFilterFlag::eDYNAMIC | PxSceneQueryFilterFlag::ePREFILTER );
	m_query->raycast( TO_PX_VECTOR( origin ), TO_PX_VECTOR( unitDir ), distance, ( Uint16 ) m_raycastHit.Size(), PxHitFlags( outputFlags ), data, userData );
	m_raycastSlotsUsed++;
#endif
}

void CPhysicsWorldBatchQuery::Sweep( const Vector& origin, Float radius, const Vector& unitDir, Float distance, const SPhysicalFilterData& filterData, Uint16 outputFlags, void* userData )
{
#ifdef USE_PHYSX
	if( !unitDir.IsNormalized3() )
	{
		return;
	}
	if( !IsFinite( distance ) )
	{
		return;
	}

	PxSphereGeometry geometry( radius );
	PxTransform transform( TO_PX_VECTOR( origin ), PxQuat::createIdentity() );
	PxQueryFilterData data( filterData.m_data, PxSceneQueryFilterFlag::eSTATIC | PxSceneQueryFilterFlag::eDYNAMIC | PxSceneQueryFilterFlag::ePREFILTER );

	m_query->sweep( geometry, transform, TO_PX_VECTOR( unitDir ), distance, ( Uint16 ) m_sweepHit.Size(), PxHitFlags( outputFlags ), data, userData );
	m_sweepSlotsUsed++;
#endif
}


Bool CPhysicsWorldBatchQuery::IsRaycastQueryCorrect( Uint32 queryIndex )
{
	if( queryIndex >= m_raycastSlotsUsed ) return false;

#ifndef USE_PHYSX
	return false;
#else
	PxRaycastQueryResult& result = m_raycastQueryResult[ queryIndex ];
	return result.queryStatus != PxBatchQueryStatus::eOVERFLOW;
#endif
}

Int32 CPhysicsWorldBatchQuery::GetRaycastResultHitCount( Uint32 queryIndex )
{
	if( queryIndex >= m_raycastSlotsUsed ) return -1;

#ifndef USE_PHYSX
	return false;
#else
	PxRaycastQueryResult& result = m_raycastQueryResult[ queryIndex ];
	return result.nbTouches;
#endif
}

void* CPhysicsWorldBatchQuery::GetRaycastResultHitUserData( Uint32 queryIndex )
{
	if( queryIndex >= m_raycastSlotsUsed ) return 0;
#ifndef USE_PHYSX
	return nullptr;
#else
	return m_raycastQueryResult[ queryIndex ].userData;
#endif
}

Float CPhysicsWorldBatchQuery::GetRaycastResultHitDistance( Uint32 queryIndex, Uint32 queryHitIndex )
{
	if( queryIndex >= m_raycastSlotsUsed ) return 0.0f;

#ifndef USE_PHYSX
	return 0.0f;
#else
	PxRaycastQueryResult& result = m_raycastQueryResult[ queryIndex ];
	if( queryHitIndex >= result.nbTouches ) return 0.0f;

	PxRaycastHit& hit = result.touches[ queryHitIndex ];

	return hit.distance;
#endif
}

Bool CPhysicsWorldBatchQuery::GetRaycastResultHitLocation( Uint32 queryIndex, Uint32 queryHitIndex, Float& outDistance, Vector& outPosition, Vector& outNormal )
{
	if( queryIndex >= m_raycastSlotsUsed ) return false;

#ifndef USE_PHYSX
	return false;
#else
	PxRaycastQueryResult& result = m_raycastQueryResult[ queryIndex ];
	if( queryHitIndex >= result.nbTouches ) return false;

	PxRaycastHit& hit = result.touches[ queryHitIndex ];

	outDistance = hit.distance;
	outPosition = TO_VECTOR( hit.position );
	outNormal = TO_VECTOR( hit.normal );

	return true;
#endif
}

CPhysicsWrapperInterface* CPhysicsWorldBatchQuery::GetRaycastResultHitWrapper( Uint32 queryIndex, Uint32 queryHitIndex )
{
	if( queryIndex >= m_raycastSlotsUsed ) return 0;

#ifndef USE_PHYSX
	return 0;
#else
	PxRaycastQueryResult& result = m_raycastQueryResult[ queryIndex ];
	if( queryHitIndex >= result.nbTouches ) return 0;

	PxRaycastHit& hit = result.touches[ queryHitIndex ];
	CPhysicsWrapperInterface* wrapper = ( CPhysicsWrapperInterface* ) hit.shape->getActor()->userData;

	return wrapper;
#endif
}

SActorShapeIndex CPhysicsWorldBatchQuery::GetRaycastResultHitActorShapeIndex( Uint32 queryIndex, Uint32 queryHitIndex )
{
	if( queryIndex >= m_raycastSlotsUsed ) return SActorShapeIndex();

#ifndef USE_PHYSX
	return SActorShapeIndex();
#else
	PxRaycastQueryResult& result = m_raycastQueryResult[ queryIndex ];
	if( queryHitIndex >= result.nbTouches ) return SActorShapeIndex();

	PxRaycastHit& hit = result.touches[ queryHitIndex ];
	SActorShapeIndex& index = ( SActorShapeIndex& ) hit.shape->userData;
	return index;
#endif
}

SPhysicalMaterial* CPhysicsWorldBatchQuery::GetRaycastResultHitPhysicalMaterial( Uint32 queryIndex, Uint32 queryHitIndex )
{
	if( queryIndex >= m_raycastSlotsUsed ) return 0;

#ifndef USE_PHYSX
	return 0;
#else
	PxRaycastQueryResult& result = m_raycastQueryResult[ queryIndex ];
	if( queryHitIndex >= result.nbTouches ) return 0;

	PxRaycastHit& hit = result.touches[ queryHitIndex ];

	physx::PxActor* actor = hit.shape->getActor();

	PxMaterial* material = hit.shape->getMaterialFromInternalFaceIndex( hit.faceIndex );
	return ( SPhysicalMaterial* ) material->userData;
#endif
}


Bool CPhysicsWorldBatchQuery::IsSweepQueryCorrect( Uint32 queryIndex )
{
	if( queryIndex >= m_sweepSlotsUsed ) return false;

#ifndef USE_PHYSX
	return false;
#else
	PxSweepQueryResult& result = m_sweepQueryResult[ queryIndex ];
	return result.queryStatus != PxBatchQueryStatus::eOVERFLOW;
#endif
}

Int32 CPhysicsWorldBatchQuery::GetSweepResultHitCount( Uint32 queryIndex )
{
	if( queryIndex >= m_sweepSlotsUsed ) return -1;

#ifndef USE_PHYSX
	return false;
#else
	PxSweepQueryResult& result = m_sweepQueryResult[ queryIndex ];
	return result.nbTouches;
#endif
}

void* CPhysicsWorldBatchQuery::GetSweepResultHitUserData( Uint32 queryIndex )
{
	if( queryIndex >= m_sweepSlotsUsed ) return 0;
#ifndef USE_PHYSX
	return nullptr;
#else
	return m_sweepQueryResult[ queryIndex ].userData;
#endif
}

Float CPhysicsWorldBatchQuery::GetSweepResultHitDistance( Uint32 queryIndex, Uint32 queryHitIndex )
{
	if( queryIndex >= m_sweepSlotsUsed ) return 0.0f;

#ifndef USE_PHYSX
	return 0.0f;
#else
	PxSweepQueryResult& result = m_sweepQueryResult[ queryIndex ];
	if( queryHitIndex >= result.nbTouches ) return 0.0f;

	PxSweepHit& hit = result.touches[ queryHitIndex ];

	return hit.distance;
#endif
}

Bool CPhysicsWorldBatchQuery::GetSweepResultHitLocation( Uint32 queryIndex, Uint32 queryHitIndex, Float& outDistance, Vector& outPosition, Vector& outNormal )
{
	if( queryIndex >= m_sweepSlotsUsed ) return false;

#ifndef USE_PHYSX
	return false;
#else
	PxSweepQueryResult& result = m_sweepQueryResult[ queryIndex ];
	if( queryHitIndex >= result.nbTouches ) return false;

	PxSweepHit& hit = result.touches[ queryHitIndex ];

	outDistance = hit.distance;
	outPosition = TO_VECTOR( hit.position );
	outNormal = TO_VECTOR( hit.normal );

	return true;
#endif
}

CPhysicsWrapperInterface* CPhysicsWorldBatchQuery::GetSweepResultHitWrapper( Uint32 queryIndex, Uint32 queryHitIndex )
{
	if( queryIndex >= m_sweepSlotsUsed ) return 0;

#ifndef USE_PHYSX
	return 0;
#else
	PxSweepQueryResult& result = m_sweepQueryResult[ queryIndex ];
	if( queryHitIndex >= result.nbTouches ) return 0;

	PxSweepHit& hit = result.touches[ queryHitIndex ];
	CPhysicsWrapperInterface* wrapper = ( CPhysicsWrapperInterface* ) hit.shape->getActor()->userData;

	return wrapper;
#endif
}

SActorShapeIndex CPhysicsWorldBatchQuery::GetSweepResultHitActorShapeIndex( Uint32 queryIndex, Uint32 queryHitIndex )
{
	if( queryIndex >= m_sweepSlotsUsed ) return SActorShapeIndex();

#ifndef USE_PHYSX
	return SActorShapeIndex();
#else
	PxSweepQueryResult& result = m_sweepQueryResult[ queryIndex ];
	if( queryHitIndex >= result.nbTouches ) return SActorShapeIndex();

	PxSweepHit& hit = result.touches[ queryHitIndex ];
	SActorShapeIndex& index = ( SActorShapeIndex& ) hit.shape->userData;
	return index;
#endif
}

SPhysicalMaterial* CPhysicsWorldBatchQuery::GetSweepResultHitPhysicalMaterial( Uint32 queryIndex, Uint32 queryHitIndex )
{
	if( queryIndex >= m_sweepSlotsUsed ) return 0;

#ifndef USE_PHYSX
	return 0;
#else
	PxSweepQueryResult& result = m_sweepQueryResult[ queryIndex ];
	if( queryHitIndex >= result.nbTouches ) return 0;

	PxSweepHit& hit = result.touches[ queryHitIndex ];

	physx::PxActor* actor = hit.shape->getActor();

	PxMaterial* material = hit.shape->getMaterialFromInternalFaceIndex( hit.faceIndex );
	return ( SPhysicalMaterial* ) material->userData;
#endif
}
