/*
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once


#ifndef APEX_CLOTH_PARAMETERS_H
#define APEX_CLOTH_PARAMETERS_H

#include "apexWrapperBase.h"
#include "renderVertices.h"

enum EClothTeleportMode
{
	Continuous,
	Teleport,
	TeleportAndReset
};

#ifdef USE_APEX

namespace physx
{
	class PxMat44;
	class PxVec3;
	namespace apex
	{
		class NxClothingActor;
		class NxClothingAsset;
		class NxClothingCollision;
	}
};

enum EClothVisibilityOverride
{
	ECVO_NoOverride,
	ECVO_Visible,
	ECVO_NotVisible
};

class CApexClothWrapper : public CApexWrapper
{
	friend class CPhysicsWorld;
	friend class CPhysicsWorldPhysXImpl;
	friend class TWrappersPool< CApexClothWrapper, SWrapperContext >;
protected:
	physx::apex::NxClothingActor*	m_actor;
	Float							m_simulationMaxDistance;
	Float							m_targetMaxDistanceScale;
	Float							m_currentMaxDistanceScale;
	Float							m_visibilityMaxDistance;
	Float							m_maxDistanceBlendTime;
	Float							m_windScaler;
	Float							m_windAdaptationCurrent;
	Float							m_windAdaptationTarget;
	Float							m_motionIntensity;
	TDynArray< physx::PxVec3 >		m_previousPositions;
	Bool							m_updateMotionIntensity;
	Bool							m_isFrozen;
	Bool							m_isClothSkinned;
	Matrix							m_wrapperTransform;
	Bool							m_isSimulating;
	Float							m_wetness;
	Bool							m_skipMaxDistanceUpdate;
	EClothVisibilityOverride		m_isVisibleOverride;

	// -1 for not forced. Atomic, so that render proxy can set it from the render thread.
	Red::Threads::CAtomic< Int32 >	m_forcedLod;

	CApexClothWrapper( CPhysicsWorld* physicalWorld, struct SClothParameters* parameters, Uint32 visibiltyId = 0 );
	virtual ~CApexClothWrapper();

	virtual void DestroySelfIfRefCountZero() { Release(); }

	virtual void SetPose( const Matrix& localToWorld, Uint32 actorIndex = 0 );

	struct SSceneCollider
	{
		STriggeringInfo m_info;
		TDynArray< physx::apex::NxClothingCollision* > m_collisionShapes;
		Int32 m_i0, m_imax, m_j0, m_jmax;

		Bool m_isToRemove;
		SSceneCollider() : m_info(), m_isToRemove( false ) {}
		SSceneCollider( const STriggeringInfo& info ) : m_info( info ), m_isToRemove( false ) {}

		RED_INLINE Bool operator==( const SSceneCollider& other )
		{
			return m_info == other.m_info;
		}
	};
	TDynArray< SSceneCollider > m_currentColliders;

	static physx::apex::NxClothingAsset* GetResourceAsset( const SClothParameters& parameters );

	void PreSimulation( SWrapperContext* simulationContext, Float timeDelta, Uint64 tickMarker, TDynArray< void* >* toAdd, TDynArray< void* >* toRemove, TDynArray< void* >* toRelease, Bool allowAdd );
	void PostSimulation( Uint8 visibilityQueryResult );

	Bool Create( const SClothParameters& parameters );
	Bool Destroy();

	void UpdateWind( const Vector& wind, Float windAdaptation );

	void UpdateGraphicalLod( float distanceFromViewportSquared );

public:

	Bool Create_Hack( const SClothParameters& parameters ){ return Create(parameters); }

	virtual void Release( Uint32 actorIndex = 0 );

	virtual void SetFlag( EPhysicsRigidBodyWrapperFlags flag, Bool decision );

	Bool MakeReadyToDestroy( TDynArray< void* >* toRemove );

	void SetVisible( EClothVisibilityOverride visOverride );
	void SetCurrentMaxDistanceScale( Float ratio );
	void SetMaxDistanceScale( Float ratio );
	void SetMaxDistanceBlendTime( Float ratio );
	void FreezeCloth( Bool shouldFreeze = false );

	void RequestReCreate( const SClothParameters& parameters );

	static Bool IsClothGPUSimulationEnabled( CPhysicsWorld* physicsWorld );

	virtual float GetMotionIntensity();

	void UpdateStateFromBonesMatricesBuffer( const Matrix& pose, const Matrix* matrices, Uint32 numMatrices, EClothTeleportMode teleportMode, const Box& box );

	void AddCollider( const STriggeringInfo& info );
	void RemoveCollider( const STriggeringInfo& info );

	Matrix GetBoneBaseModelSpacePose( Uint32 index );

	virtual physx::apex::NxApexActor* GetApexActor();
	virtual physx::apex::NxApexRenderable* AcquireRenderable();
	void ReleaseRenderable( physx::apex::NxApexRenderable* renderable );

	Vector GetFirstSimulationPosition();

	Bool ShouldBeSimulated( Float distanceFromViewportSquared ) const;
	Bool IsSimulating() const { return m_isSimulating; }
#ifndef NO_EDITOR
	void GetDebugVertex( TDynArray< DebugVertex >& debugVertex );
#endif

	virtual Bool IsReady() const { return m_actor != 0; }

	Bool IsFrozen();

	void GetDebugTriangles( TDynArray< Vector >& verts );

	Uint32 SelectVertex( const Vector& worldPos, const Vector& worldDir, Vector& hitPos );
	void MoveVertex( Uint32 vertexIndex, const Vector& worldPos );
	void FreeVertex( Uint32 vertexIndex );

	// Internal value is wrapped in an Atomic, safe to call from across threads.
	void ForceLODLevel( Int32 lodOverride );

	static Bool FillResourceParameters( SClothParameters& parameters );

	void				SetWrapperTransform( const Matrix& transform ){ m_wrapperTransform = transform; }
	void				SetIsClothSkinned( Bool val ){ m_isClothSkinned = val; }

	void				SetWetness( Float w ){ m_wetness = w; }

};

#endif

#endif
