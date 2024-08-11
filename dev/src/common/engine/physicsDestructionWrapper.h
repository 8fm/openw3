/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "..\physics\physicsWrapper.h"
#include "..\physics\physicalCollision.h"
#include "..\physics\physicsWrapperPool.h"
#include "apexWrapperBase.h"

#ifndef PHYSICS_DESTRUCTION_WRAPPER_H
#define PHYSICS_DESTRUCTION_WRAPPER_H

#define MAX_ACTORS_PER_FRAME 200
#define MAX_CREATED_PER_FRAME 40

namespace physx
{
	class PxRigidActor;
	class PxActor;
	class PxShape;
	class PxGeometry;
	class PxMaterial;
};

struct SPhysicsDestructionParameters 
{
	THandle< CPhysicsDestructionResource >		m_baseResource;
	THandle< CPhysicsDestructionResource >		m_fracturedResource;

	THandle< CComponent >						m_parent;
	Bool										m_isDestroyed;

	CPhysicalCollision							m_physicalCollisionType;
	CPhysicalCollision							m_fracturedPhysicalCollisionType;
	Matrix										m_pose;

	Bool										m_dynamic;
	Bool										m_kinematic;
	Bool										m_useWorldSupport;

	Bool										m_debrisTimeout;
	Float										m_debrisTimeoutMin;
	Float										m_debrisTimeoutMax;
	
	Bool										m_hasInitialFractureVelocity;

	Vector										m_initialBaseVelocity;

	Float										m_debrisMaxSeparation;
	Float										m_simulationDistance;

	Float										m_maxVelocity;
	Float										m_maxAngFractureVelocity;

	Float										m_fadeOutTime;

	Bool										m_accumulateDamage;
	Float										m_forceToDamage;
	Float										m_damageThreshold;
	Float										m_damageEndurance;

	SPhysicsDestructionParameters();
};

enum EDestructionActorFlags
{
	EDAF_IsStatic								= FLAG( 0 ),
	EDAF_IsDeleted								= FLAG( 1 ),
	EDAF_IsFadingOut							= FLAG( 2 )
};

// Helper struct, that holds debris chunk physX actor as well as additional per chunk data.
struct SDestructionActorInfo
{
	Vector					m_initialVelocity;
	Vector					m_initialAngularVelocity;
	Uint8					m_actorFlags;
	Float					m_debrisChunkTimeout;
	Float					m_chunkAlpha;

	void SetFlag( EDestructionActorFlags flag, Bool decision )
	{
		if( decision )
		{
			m_actorFlags |= flag ;
		}
		else
		{
			m_actorFlags &= ( 0xFF ^ flag );
		}
	}

	Bool GetFlag( EDestructionActorFlags flag ) const
	{
		return ( m_actorFlags & flag ) > 0;
	}

	SDestructionActorInfo()
		: m_actorFlags( 0 )
		, m_initialVelocity( Vector::ZEROS )
		, m_debrisChunkTimeout( 0.f )
		, m_chunkAlpha( 1.0f )
	{}
};

enum EPhysicsDestructionFlags
{
	EPDF_RequestSceneAdd				= PRBW_Reserved_1,
	EPDF_RequestSceneRemove				= PRBW_Reserved_2,
	EPDF_RequestFracture				= PRBW_Reserved_3,
	EPDF_IsFractured					= PRBW_Reserved_4,
	EPDF_UpdateBounds					= PRBW_Reserved_5,
	EPDF_IsReady						= PRBW_Reserved_6,
	EPDF_ScheduleUpdateTransform		= PRBW_Reserved_7,
	EPDF_ScheduleUpdateKinematicState	= PRBW_Reserved_8,
	EPDF_HasMaxVelocity					= PRBW_Reserved_9,
	EPDF_AccumulatesDamage				= PRBW_Reserved_10,
	EPDF_HasDebrisTimeout				= PRBW_Reserved_11,
	EPDF_HasInitialFractureVelocity		= PRBW_Reserved_12,
	EPDF_ScheduleUpdateSkinning			= PRBW_Reserved_13,
	EPDF_ScheduleEnableDissolve			= PRBW_Reserved_14,
	EPDF_ScheduleUpdateActiveIndices	= PRBW_Reserved_15
};

class CPhysicsDestructionWrapper : public CPhysicsWrapperInterface
{
	friend class CPhysicsWorld;
	friend class CPhysicsWorldPhysXImpl;
	friend class TWrappersPool< CPhysicsDestructionWrapper, SWrapperContext >;
protected:
	class CPhysicsWorldPhysXImpl*		m_world;
	CPhysicalCollision					m_basePhysicalCollisionType;
	CPhysicalCollision					m_fracturedPhysicalCollisionType;

	const SPhysicalMaterial*			m_basePhysicalMaterial;
	const SPhysicalMaterial*			m_fracturedPhysicalMaterial;
	Matrix								m_componentOffsetInverse;

	TDynArray< physx::PxRigidActor* >	m_actors;
	TDynArray< SDestructionActorInfo >	m_actorsAdditionalInfo;

	Uint16								m_actorsCount;

	TQueue< Uint16 >					m_toAddIndexes;
	TQueue< Uint16 >					m_toRemoveIndexes;

	Float								m_maxVelocity;
	Float								m_maxAngFractureVelocity;
	Float								m_accumulatedDamage;
	Float								m_debrisLifetimeAcc;

	Float								m_forceToDamage;
	Float								m_damageThreshold;
	Float								m_autohideDistanceSquared;

	Box									m_bounds;
	TDynArray< Box >					m_boundsFractured;
	Vector								m_weightedCenter;

	Vector								m_forceOnDestruction;
	Vector								m_scale;
	
	Bool								m_updateKinematicStateDecision;
	Bool								m_isDissolveEnabled;
public:
	CPhysicsDestructionWrapper( const SPhysicsDestructionParameters& parameters, CPhysicsWorld* world, TRenderVisibilityQueryID visibiltyId, CompiledCollisionPtr compiledCollision, CompiledCollisionPtr compiledCollisionFractured );
	~CPhysicsDestructionWrapper( );

	void								Destroy();

	virtual void						Release(Uint32 actorIndex = 0);
	virtual Bool						IsReady() const;

	virtual Matrix						GetPose( Uint32 actorIndex = 0 ) const;
	virtual void						SetPose( const Matrix& localToWorld, Uint32 actorIndex = 0 );

	virtual void*						GetActor( Uint32 actorIndex = 0 ) const;
	virtual CPhysicsWorld*				GetPhysicsWorld();

	virtual Float						GetMass( Uint32 actorIndex = 0 ) const;

	Bool								GetDestructionFlag( EPhysicsDestructionFlags flag ) const;
	void								SetDestructionFlag( EPhysicsDestructionFlags flag, Bool decision );

	Bool								Create( const SPhysicsDestructionParameters& parameters, CompiledCollisionPtr compiledCollision, CompiledCollisionPtr compiledCollisionFractured );
	Bool								CreateActors( CompiledCollisionPtr compiledCollision, const SPhysicsDestructionParameters &parameters, Uint16 &actorIndex );
	physx::PxShape*						CreateShapeSingleMaterial( Uint32 actorIndex, physx::PxRigidActor* rigidActor, physx::PxGeometry* phxGeometry, physx::PxMaterial* material, const Matrix& localPose );

	void								PreSimulation( SWrapperContext* simulationContext, Float timeDelta, Uint64 tickMarker, TDynArray< void* >* toAdd, TDynArray< void* >* toRemove, TDynArray< void* >* toRelease, Bool allowAdd );

	void								RequestAddingOfFracturedActors( Float timeDelta, Int32 flags );

	void								PostSimulationUpdateTransform( const Matrix& transformOrg, void* actor );
	
	Bool								PushActorsToUpdate( TDynArray< void* >* toUpdate, Bool isAdding );
	void								UpdateScene( TDynArray< void* >* toAdd, TDynArray< void* >* toRemove, Float timeDelta, Float fadeOutTime );

	Bool								MakeReadyToDestroy( TDynArray< void* >* toRemove );

	virtual Bool						ApplyForce( const Vector& force, Uint32 actorIndex = 0 ) override;
	virtual void						ApplyFractureByVolume(Uint16 triggeredActorIndex, Float applyFractureDamage, CPhantomComponent* triggerComponent, Vector forceOnDestruction);
	virtual void						ApplyDamage( Float damage );
	virtual Bool						UpdatesEntity() override { return true; }

	Box									GetBounds();
	Bool								CalculateBounds();

#ifndef NO_EDITOR_FRAGMENTS
	Float								GetAccumulatedDamage() { return m_accumulatedDamage; }
#endif

	Int32								GetFracturedActorsCount() const { return m_actorsCount - 1; } 
	Bool								SampleBonesModelSpace( TDynArray<Matrix>& poseInOut, TDynArray< Bool >& bonesActive );
	void								ScheduleSkinningUpdate();
	void								SetAutoHideDistance(Float desiredDistance );

	virtual void						SwitchToKinematic(Bool decision);
	Bool								ApplyFracture();

#ifndef NO_EDITOR 
	const TDynArray<Float>				GetMasses();
#endif
};

#endif

