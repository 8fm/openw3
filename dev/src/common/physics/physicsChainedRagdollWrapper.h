/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "physicsRagdollWrapper.h"
#include "../physics/physicsWrapperPool.h"
#include "physicsRagdollState.h"

#define EP2_SHEEP_RAGDOLL_ANGULAR_DAMPING_HACK

class CPhysicsChainedRagdollWrapper : public CPhysicsRagdollWrapper
{
	friend class TWrappersPool< CPhysicsChainedRagdollWrapper, SWrapperContext >;

protected:
	struct BoneMapping
	{
		Float						m_buffer0;
		Float						m_buffer1;
		Float						m_buffer2;
		Float						m_buffer3;
		Float						m_depthHeight;
		Float						m_halfBoundsHeight;
		Float						m_floatingRatio;
		Int16						m_boneIndex;
		Int16						m_parentIndex;
		void*						m_rigidDynamic;
		void*						m_articulationLink;
		Matrix						m_initLocalPose;
		Float						m_linearDamper;
		Float						m_angularDamper;

		BoneMapping( Int16 boneIndex, Int16 parentIndex, const Matrix& initLocalPose, void* actor = 0, Float floatingRatio = 1) 
			: m_boneIndex( boneIndex ), m_parentIndex( parentIndex ), m_rigidDynamic( actor ), m_articulationLink( nullptr ), m_initLocalPose( initLocalPose ), m_floatingRatio( floatingRatio ), m_linearDamper( 0.0f ), m_angularDamper( 0.0f ) {}
	};
	TDynArray<BoneMapping>			m_mappings;

	struct BoneTransformBuffer
	{
		Int16						m_boneIndex;
		void*						m_rigidDynamic;
		Float						m_quatX;
		Float						m_quatY;
		Float						m_quatZ;
		Float						m_quatW;
		Float						m_posX;
		Float						m_posY;
		Float						m_posZ;
		BoneTransformBuffer( Int16 boneIndex, void* rigidDynamic = nullptr ) : m_boneIndex( boneIndex ), m_rigidDynamic( rigidDynamic ) {}
	};
	TDynArray<BoneTransformBuffer>	m_transformsBuffer;

	void*							m_articulation;
	CPhysicsEngine::CollisionMask	m_originalGroupMask;
	CPhysicsEngine::CollisionMask	m_customDynamicGroupMask;

#ifdef EP2_SHEEP_RAGDOLL_ANGULAR_DAMPING_HACK
	Float							m_angularDamping;
#endif

protected:

#ifdef EP2_SHEEP_RAGDOLL_ANGULAR_DAMPING_HACK
	CPhysicsChainedRagdollWrapper( SPhysicsRagdollState& ragdollState, const DataBuffer& buffer, TDynArray< BoneInfo >& bones, CPhysicsEngine::CollisionMask collisionType, CPhysicsEngine::CollisionMask collisionGroup, Uint32 visibiltyId = 0, Float angularDamping = -1.0f );
#else
	CPhysicsChainedRagdollWrapper( SPhysicsRagdollState& ragdollState, const DataBuffer& buffer, TDynArray< BoneInfo >& bones, CPhysicsEngine::CollisionMask collisionType, CPhysicsEngine::CollisionMask collisionGroup, Uint32 visibiltyId = 0 );
#endif
	virtual ~CPhysicsChainedRagdollWrapper();

	Bool MakeReadyToDestroy( TDynArray< void* >* toRemove );
	void PreSimulation( SWrapperContext* simulationContext, Float timeDelta, Uint64 tickMarker, TDynArray< void* >* toAdd, TDynArray< void* >* toRemove, TDynArray< void* >* toRelease, Bool allowAdd );

	Bool ApplyBuoyancyForce( float baseLinearDamper, float baseAngularDamper, Bool& waterLevelToDeep );
	void ApplyDampers( Float timeDelta );

public:
	virtual void Release( Uint32 actorIndex = 0 );

	virtual Bool IsReady() const;

	void SyncToAnimation( const TDynArray<Matrix>& bones, const Matrix& localToWorld );
	void SampleBonesModelSpace( TDynArray<Matrix>& poseInOut ) const;

	Bool GetCurrentPositionAndDeltaPosition( Vector & outCurrentPosition, Vector & outDeltaPosition ) const;
	
	virtual Int32 GetIndex( const char* actorName );
	SPhysicalMaterial* GetMaterial( Int32 bodyIndex );

	virtual Uint32 GetActorsCount() const;
	virtual void* GetActor( Uint32 actorIndex ) const;

	Int32 GetBoneIndex( Int32 actorIndex ) const override;

	Bool DisableRagdollParts( const TDynArray< Uint32 > & boneIndices );

	Box GetWorldBounds();

	Bool IsOk() { return !m_mappings.Empty(); }

private:
	void SetGroup( const CName& groupType );
	void SetGroup( const CPhysicsEngine::CollisionMask& groupTypeMask );
	CPhysicsEngine::CollisionMask GetGroup();

	virtual void OnContactModify( void* pair );

};