/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once


#ifndef PHYSICS_TILE_WRAPPER_H
#define PHYSICS_TILE_WRAPPER_H

#include "../physics/physicsEngine.h"
#include "../physics/physicsWrapper.h"
#include "physicsDataProviders.h"

class CPhysicsTileWrapper : public CPhysicsWrapperInterface
{
	friend class CPhysicsWorld;
	friend class CPhysicsWorldPhysXImpl;
	friend class TWrappersPool< CPhysicsTileWrapper, SWrapperContext >;

	struct SStaticBodyStruct
	{
		char m_parentHook[ 16 ];
		void* m_actor;
		Uint64 m_sectorMask;
		CompiledCollisionPtr m_compiledCollision;
		Bool m_isAttached;
		Bool m_qued;

#ifndef NO_EDITOR
		StringAnsi m_debugName;
#endif
		SStaticBodyStruct() : m_actor( 0 ), m_sectorMask( 0 ), m_isAttached( false ), m_qued( false ) { ::new(m_parentHook) CPhysicsWrapperParentComponentProvider( nullptr ); };
		SStaticBodyStruct( void* actor ) : m_actor( actor ), m_sectorMask( 0 ), m_isAttached( false ), m_qued( false ) { ::new(m_parentHook) CPhysicsWrapperParentComponentProvider( nullptr ); }
		SStaticBodyStruct( void* actor, CompiledCollisionPtr& compiledCollision ) : m_actor( actor ), m_sectorMask( 0 ), m_compiledCollision( compiledCollision ), m_isAttached( false ), m_qued( false ) { ::new(m_parentHook) CPhysicsWrapperParentComponentProvider( nullptr ); }
	};
	typedef TDynArray< SStaticBodyStruct, MC_PhysTileStaticBodies, MemoryPool_Physics > StaticBodiesStructDynArray;
	StaticBodiesStructDynArray					m_staticBodies;
	Box2										m_box;
	Uint32										m_minIndex;
	Uint64										m_sectorMaskProcessed;
	static class CPhysxStaticActorCreateTask*	m_task;

	Bool MakeReadyToDestroy( TDynArray< void* >* toRemove );
	void PreSimulation( SWrapperContext* simulationContext, Float timeDelta, Uint64 tickMarker, TDynArray< void* >* toAdd, TDynArray< void* >* toRemove, TDynArray< void* >* toRelease, Bool allowAdd );
	Bool CountGetSectorMask( Uint32 actorIndex = 0 );

	Bool SetOcclusionParameters( Uint32 actorIndex = 0, Float diagonalLimit = 1, Float attenuation = -1 );

protected:
	CPhysicsTileWrapper( CPhysicsWorld* world, Box2 box );
	virtual ~CPhysicsTileWrapper();

public:
	static void PrepereStaticBodyCreationQue( CPhysicsWorld* world );
	static void FlushStaticBodyCreationQue();
	void AddRef() { m_ref.Increment(); }

	virtual void Release( Uint32 actorIndex = 0 );

	virtual IPhysicsWrapperParentProvider* GetParentProvider( Uint32 actorIndex = 0 ) const;

	void Dispose( const CObject* owner );
	void DisposeArea( const CObject* owner, const Box2& box );

	virtual Bool IsReady() const;

	virtual void SetPose( const Matrix& localToWorld, Uint32 actorIndex = 0 );
	virtual Matrix GetPose( Uint32 actorIndex = 0 ) const;
	virtual Uint32 GetActorsCount() const;
	virtual void* GetActor( Uint32 actorIndex = 0 ) const;

	Bool GetOcclusionParameters( Uint32 actorIndex = 0, Float* diagonalLimit = 0, Float* attenuation = 0 );

	Bool IsSectorReady( const Vector& position ) const;

	const Box2& GetBounds2D() { return m_box; }

	template< typename T >
	Int32 AddTerrainBody( T owner, const Vector& position, const SPhysicalMaterial* material, void* geometry /*PxHeightFieldGeometry*/ )
	{
		Int32 result = AddTerrainBody( position, material, geometry );
		if( result == -1 ) return -1;
		SStaticBodyStruct& staticBodyStruct = m_staticBodies[ result ];
		::new(staticBodyStruct.m_parentHook) T( owner );
		return result;
	}

	template< typename T >
	Int32 AddStaticBody( T owner, const Matrix& pose, CompiledCollisionPtr& compiledCollision, CPhysicsEngine::CollisionMask collisionType, CPhysicsEngine::CollisionMask collisionGroup )
	{
		Int32 result = AddStaticBody( pose, compiledCollision, collisionType, collisionGroup );
		if( result == -1 ) return -1;
		SStaticBodyStruct& staticBodyStruct = m_staticBodies[ result ];
		::new(staticBodyStruct.m_parentHook) T( owner );
		return result;
	}

	template< typename T >
	Int32 AddFoliageBody( T owner, const void* instance, const TDynArray< Sphere >& shapes )
	{
		Int32 result = AddFoliageBody( instance, shapes );
		if( result == -1 ) return -1;
		SStaticBodyStruct& staticBodyStruct = m_staticBodies[ result ];
		::new(staticBodyStruct.m_parentHook) T( owner );
		return result;
	}

private:
	Int32 AddTerrainBody( const Vector& position, const SPhysicalMaterial* material, void* geometry /*PxHeightFieldGeometry*/ );
	Int32 AddStaticBody( const Matrix& pose, CompiledCollisionPtr& compiledCollision, CPhysicsEngine::CollisionMask collisionType, CPhysicsEngine::CollisionMask collisionGroup );
	Int32 AddFoliageBody( const void* instance, const TDynArray< Sphere >& shapes );


};

#endif
