/*
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "apexWrapperBase.h"
#include "../physics/physicalCollision.h"
#include "../physics/physicsWrapperPool.h"

class IRenderResource;


#ifdef USE_APEX

namespace physx
{
	namespace apex
	{
		class NxDestructibleActor;
		class NxDestructibleAsset;
	}
};

class CApexDestructionWrapper : public CApexWrapper
{
	friend class CApexPhysX3Interface;
	friend class CPhysicsWorld;
	friend class CPhysicsWorldPhysXImpl;
	friend class TWrappersPool< CApexDestructionWrapper, SWrapperContext >;

protected:
	CPhysicalCollision					m_basePhysicalCollisionType;
	CPhysicalCollision					m_fracturedPhysicalCollisionType;
	Float								m_lowestKinematicsCount;
	physx::apex::NxDestructibleActor*	m_actor;
	Matrix								m_componentOffsetInverse;

	struct SFractureRequest
	{
		THandle< IScriptable >			m_fracturingVolumeComponent;
		Uint32							m_actorIndex;
		Float							m_amount;
		void*							m_actor;

		Bool operator==( const SFractureRequest& other )
		{
			return m_fracturingVolumeComponent == other.m_fracturingVolumeComponent && m_actor == other.m_actor;
		}

		SFractureRequest( const THandle< IScriptable >& fracturingVolumeComponent, Uint32 actorIndex, Float amount, void* actor ) : m_fracturingVolumeComponent( fracturingVolumeComponent ), m_actorIndex( actorIndex ), m_amount( amount ), m_actor( actor ) {}
		SFractureRequest() {}
	};

	TDynArray< SFractureRequest >		m_toProcessFracture;

	const SPhysicalMaterial*			m_physicalMaterial;
	const SPhysicalMaterial*			m_fracturedPhysicalMaterial;
	Red::Threads::CAtomic< Uint32 >		m_amountOfBaseFractures;

	Bool								m_forceFracture;

	CApexDestructionWrapper( struct SDestructionParameters* parameters, CPhysicsWorld* world, Uint32 visibiltyId );
	virtual ~CApexDestructionWrapper();

	virtual Bool UpdatesEntity() { return true; }

public:
	virtual void Release( Uint32 actorIndex = 0 );

	virtual physx::apex::NxApexActor* GetApexActor();
	virtual physx::apex::NxApexRenderable* AcquireRenderable();
	virtual void ReleaseRenderable( physx::apex::NxApexRenderable* renderable );

	virtual void SetPose( const Matrix& localToWorld, Uint32 actorIndex = 0 );


	virtual void DestroySelfIfRefCountZero() { Release(); }

	Bool MakeReadyToDestroy( TDynArray< void* >* toRemove );

#ifndef NO_EDITOR

protected:
	TDynArray< IRenderResource* >			m_chunkDebugMeshes;
	TDynArray< Vector >						m_debugFracturePoints;
	Bool									m_debugFracturePointsWhereTaken;

public:
	IRenderResource* GetChunkDebugMesh( Uint32 chunkIndex );
	const TDynArray< Vector >& GetDebugFracturePoints();
#endif

	virtual Uint32 GetActorsCount() const;
	virtual void* GetActor( Uint32 actorIndex ) const;

private:
	static physx::apex::NxDestructibleAsset* GetResourceAsset( const SDestructionParameters& parameters );

	void* GetChunkShape( Uint32 chunkIndex ) const;

	void PreSimulation( SWrapperContext* simulationContext, Float timeDelta, Uint64 tickMarker, TDynArray< void* >* toAdd, TDynArray< void* >* toRemove, TDynArray< void* >* toRelease, Bool allowAdd );
	virtual void PostSimulationUpdateTransform( const Matrix& transform, void* actor );

	Bool Destroy();

public:
	Bool Create( const SDestructionParameters& parameters );
	void RequestReCreate( const SDestructionParameters& parameters );

	virtual Bool IsReady() const;

	void ForceDynamicState();

	Bool ApplyFracture();
	Bool ApplyFractureByVolume( Uint32 actorIndex, Float damageAmount, IScriptable* fracturingVolume = 0 );
	Bool ApplyDamageAtPoint( Float amount, Float momentum, const Vector& position, const Vector& direction );   
	Bool ApplyRadiusDamage( Float amount, Float momentum, const Vector& position, Float radius, Bool falloff );   

	virtual Float GetMass( Uint32 actorIndex = 0 ) const;

	virtual Matrix GetPose( Uint32 actorIndex = 0 ) const;
	virtual Vector GetPosition( Uint32 actorIndex = 0 ) const;
	virtual Vector GetCenterOfMassPosition( Uint32 actorIndex = 0 ) const;

	static Bool FillResourceParameters( SDestructionParameters& parameters );

	Matrix GetChunkLocalToWorld( Uint32 chunkIndex ) const;

	const SPhysicalMaterial* GetPhysicalMaterial( bool fractured = false );
	CPhysicalCollision& GetPhysicalCollisionType( Uint16 depth );

	Float GetFractureRatio();

	Float GetThresholdLeft( Uint32 actorIndex );

};
#endif
