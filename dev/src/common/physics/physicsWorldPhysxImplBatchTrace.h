#pragma once

#include "physicsWorldPhysXImpl.h"

#ifdef USE_PHYSX
#include "PxBatchQueryDesc.h"
#include "PxBatchQuery.h"
#endif

enum EBatchQueryQueryFlag
{
	EQQF_IMPACT					= FLAG( 0 ),	//!< "impact" member of #PxSceneQueryHit is valid
	EQQF_NORMAL					= FLAG( 1 ),	//!< "normal" member of #PxSceneQueryHit is valid
	EQQF_DISTANCE				= FLAG( 2 ),	//!< "distance" member of #PxSceneQueryHit is valid
	EQQF_UV						= FLAG( 3 ),	//!< "u" and "v" barycentric coordinates of #PxSceneQueryHit are valid. Note: Not applicable for sweep queries.
	EQQF_NO_INITIAL_OVERLAP		= FLAG( 4 ),	//!< Enable/disable initial overlap tests in sweeps. Also mark returned hits as initially overlapping.
	EQQF_TOUCHING_HIT			= FLAG( 5 ),	//!< specified the hit object as a touching hit. See also #PxSceneQueryHitType
	EQQF_BLOCKING_HIT			= FLAG( 6 ),	//!< specified the hit object as a blocking hit. See also #PxSceneQueryHitType
	EQQF_MESH_BOTH_SIDES		= FLAG( 7 ),	//!< Report hits with back faces of triangles.
	EQQF_PRECISE_SWEEP			= FLAG( 8 ),	//!< Use more accurate sweep function, but slower
};

BEGIN_ENUM_RTTI( EBatchQueryQueryFlag )
	ENUM_OPTION( EQQF_IMPACT )
	ENUM_OPTION( EQQF_NORMAL )
	ENUM_OPTION( EQQF_DISTANCE )
	ENUM_OPTION( EQQF_UV )
	ENUM_OPTION( EQQF_NO_INITIAL_OVERLAP )
	ENUM_OPTION( EQQF_TOUCHING_HIT )
	ENUM_OPTION( EQQF_BLOCKING_HIT )
	ENUM_OPTION( EQQF_MESH_BOTH_SIDES )
	ENUM_OPTION( EQQF_PRECISE_SWEEP )
END_ENUM_RTTI()

class CPhysicsWorldBatchQuery
{
	friend class CPhysicsWorldPhysXImpl;

	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_PhysicsEngine );

protected:
	Uint32 m_raycastSlotsUsed;
	Uint32 m_sweepSlotsUsed;

#ifdef USE_PHYSX
	TDynArray< physx::PxRaycastQueryResult > m_raycastQueryResult;
	TDynArray< physx::PxRaycastHit > m_raycastHit;
	TDynArray< physx::PxSweepQueryResult > m_sweepQueryResult;
	TDynArray< physx::PxSweepHit > m_sweepHit;
	physx::PxScene* m_scene;
	physx::PxBatchQuery* m_query;
#endif

	CPhysicsWorldBatchQuery();

public:
	virtual ~CPhysicsWorldBatchQuery();

	void Init( Uint32 raycastPrealocate, Uint32 maxRaycastHits, Uint32 sweepPrealocate, Uint32 maxSweepHits );
	Int8 Process();

	Uint32 IsSlotLeftForNewRaycast();
	Uint32 GetRaycastBufferSize();
	Uint32 GetRaycastBufferUsedSize() { return m_raycastSlotsUsed; }

	Uint32 IsSlotLeftForNewSweep();
	Uint32 GetSweepBufferSize();
	Uint32 GetSweepBufferUsedSize() { return m_sweepSlotsUsed; }

	void Raycast( const Vector& origin, const Vector& unitDir, Float distance, const SPhysicalFilterData& filterData, Uint16 outputFlags = EBatchQueryQueryFlag::EQQF_IMPACT|EBatchQueryQueryFlag::EQQF_NORMAL|EBatchQueryQueryFlag::EQQF_DISTANCE|EBatchQueryQueryFlag::EQQF_UV, void* userData = NULL);

	void Sweep( const Vector& origin, Float radius, const Vector& unitDir, Float distance, const SPhysicalFilterData& filterData, Uint16 outputFlags = EBatchQueryQueryFlag::EQQF_IMPACT|EBatchQueryQueryFlag::EQQF_NORMAL|EBatchQueryQueryFlag::EQQF_DISTANCE, void* userData = NULL );
	
	// TO DO WHEN NEEDED
	// overlap

	Bool						IsRaycastQueryCorrect( Uint32 queryIndex );
	Int32						GetRaycastResultHitCount( Uint32 queryIndex );
	void*						GetRaycastResultHitUserData( Uint32 queryIndex );
	Float						GetRaycastResultHitDistance( Uint32 queryIndex, Uint32 queryHitIndex );
	Bool						GetRaycastResultHitLocation( Uint32 queryIndex, Uint32 queryHitIndex, Float& outDistance, Vector& outPosition, Vector& outNormal );
	CPhysicsWrapperInterface*	GetRaycastResultHitWrapper( Uint32 queryIndex, Uint32 queryHitIndex );
	SActorShapeIndex			GetRaycastResultHitActorShapeIndex( Uint32 queryIndex, Uint32 queryHitIndex );
	SPhysicalMaterial*			GetRaycastResultHitPhysicalMaterial( Uint32 queryIndex, Uint32 queryHitIndex );

	Bool						IsSweepQueryCorrect( Uint32 queryIndex );
	Int32						GetSweepResultHitCount( Uint32 queryIndex );
	void*						GetSweepResultHitUserData( Uint32 queryIndex );
	Float						GetSweepResultHitDistance( Uint32 queryIndex, Uint32 queryHitIndex );
	Bool						GetSweepResultHitLocation( Uint32 queryIndex, Uint32 queryHitIndex, Float& outDistance, Vector& outPosition, Vector& outNormal );
	CPhysicsWrapperInterface*	GetSweepResultHitWrapper( Uint32 queryIndex, Uint32 queryHitIndex );
	SActorShapeIndex			GetSweepResultHitActorShapeIndex( Uint32 queryIndex, Uint32 queryHitIndex );
	SPhysicalMaterial*			GetSweepResultHitPhysicalMaterial( Uint32 queryIndex, Uint32 queryHitIndex );
};
