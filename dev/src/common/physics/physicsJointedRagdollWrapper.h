/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "physicsRagdollWrapper.h"
#include "../physics/physicsWrapperPool.h"
#include "physicsRagdollState.h"

class CPhysicsJointedRagdollWrapper : public CPhysicsRagdollWrapper
{
	friend class TWrappersPool< CPhysicsJointedRagdollWrapper, SWrapperContext >;

protected:
	struct BoneMapping
	{
		Float					m_buffer0;
		Float					m_buffer1;
		Float					m_buffer2;
		Float					m_buffer3;
		Float					m_depthHeight;
		Float					m_halfBoundsHeight;
		Float					m_floatingRatio;

		Int16					m_boneIndex;
		Int16					m_parentIndex;
		void*					m_actor;
		Matrix					m_initLocalPose;
		RedVector4				m_outTranslation;
		RedQuaternion			m_outRotation;
		BoneMapping( Int16 boneIndex, Int16 parentIndex, const Matrix& initLocalPose, void* actor = 0, Float floatingRatio = 1) : m_boneIndex( boneIndex ), m_parentIndex( parentIndex ), m_actor( actor ), m_initLocalPose( initLocalPose ), m_floatingRatio( floatingRatio ) {}
	};

	TDynArray<BoneMapping>		m_mappings;
	Float						m_windAdaptationCurrent;
	Float						m_windAdaptationTarget;
	Float						m_speedLimitForSleep;
	Float						m_windScaler;
	Bool						m_forceWakeUp;

protected:
	CPhysicsJointedRagdollWrapper( SPhysicsRagdollState& ragdollState, const DataBuffer& buffer, TDynArray< BoneInfo >& bones, CPhysicsEngine::CollisionMask collisionType, CPhysicsEngine::CollisionMask collisionGroup, Uint32 visibiltyId = 0 );
	virtual ~CPhysicsJointedRagdollWrapper();

	Bool MakeReadyToDestroy( TDynArray< void* >* toRemove );
	void PreSimulation( SWrapperContext* simulationContext, Float timeDelta, Uint64 tickMarker, TDynArray< void* >* toAdd, TDynArray< void* >* toRemove, TDynArray< void* >* toRelease, Bool allowAdd );

	void UpdateWindOnActors();
	Bool ApplyBuoyancyForce( float baseLinearDamper, float baseAngularDamper, Bool& waterLevelToDeep );

public:
	virtual void Release( Uint32 actorIndex = 0 );

	virtual Bool IsReady() const;

	void SyncToAnimation( const TDynArray<Matrix>& bones, const Matrix& localToWorld );
	void SampleBonesModelSpace( TDynArray<Matrix>& poseInOut ) const;

	Bool GetCurrentPositionAndDeltaPosition( Vector & outCurrentPosition, Vector & outDeltaPosition ) const;

	virtual Int32 GetIndex( const char* actorName );
	SPhysicalMaterial* GetMaterial( Int32 bodyIndex );

	virtual Uint32 GetActorsCount() const;
	void* GetActor( Uint32 actorIndex ) const;

	void* GetMappingActor( Uint32 index ) const;
	Int16 GetMappingActorIndex( Uint32 index ) const;

	void SetSpeedLimitForSleep( Float speed = 0.0f ) { m_speedLimitForSleep = speed; }

	Int32 GetBoneIndex( Int32 actorIndex ) const override;

	Bool DisableRagdollParts( const TDynArray< Uint32 > & boneIndices );

	Box GetWorldBounds();

};
