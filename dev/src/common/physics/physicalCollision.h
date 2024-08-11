#pragma once

#include "physicsEngine.h"

class CPhysicalCollision
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_Physics, MC_PhysicalCollision );

	friend class CEdPhysicalCollisionTypeSelector;
	friend class CEdPhysicalCollisionGroupSelector;
	friend class CStaticMeshComponent;

public:
	static Uint32							PHYSICAL_COLLISION_VERSION;
	static CPhysicsEngine::CollisionMask	COLLIDES_ALL;

private:
	TDynArray< CName >						m_collisionTypeNames;
	CPhysicsEngine::CollisionMask			m_collisionTypeMask;
	CPhysicsEngine::CollisionMask			m_collisionGroupsMask;

	void ResolveCollisionMasks();

public:
	// used in characterControllerParam
	CPhysicalCollision( CName initCollisionTypeName ) 
		: m_collisionTypeMask( 0 )
		, m_collisionGroupsMask( 0 )
	{ 
		m_collisionTypeNames.PushBack( initCollisionTypeName );
		ResolveCollisionMasks();
	}

	CPhysicalCollision() : m_collisionTypeMask( 0 ), m_collisionGroupsMask( 0 ) {}
	CPhysicalCollision( CPhysicsEngine::CollisionMask collisionTypeMask, CPhysicsEngine::CollisionMask collisionGroupMask ) : m_collisionTypeMask( collisionTypeMask ), m_collisionGroupsMask( collisionGroupMask ) {}
	virtual ~CPhysicalCollision() {}

	void RetrieveCollisionMasks( CPhysicsEngine::CollisionMask& collsionTypeMask, CPhysicsEngine::CollisionMask& collisiontGroupMask );
	void RetrieveCollisionMasks( CPhysicsEngine::CollisionMask& collsionTypeMask, CPhysicsEngine::CollisionMask& collisiontGroupMask ) const;
	Bool HasCollisionTypeName(){ return m_collisionTypeNames.Size() > 0; }

	friend IFile& operator<<( IFile& file, CPhysicalCollision& physicalCollision );
	Bool operator==( const CPhysicalCollision& other ) const;
	void Serialize( IFile& file );
};

RED_DECLARE_RTTI_NAME( CPhysicalCollision );

class CSimpleRTTITypePhysicalCollision : public TSimpleRTTIType< CPhysicalCollision >
{
public:
	virtual void Destruct( void *object ) const
	{
		static_cast< CPhysicalCollision* >(object)->~CPhysicalCollision();
	}

	virtual const CName& GetName() const { return CNAME( CPhysicalCollision ); }

	virtual ERTTITypeType GetType() const { return RT_Simple; };

	virtual Bool Serialize( IFile& file, void* data ) const
	{
		CPhysicalCollision* coll = ( CPhysicalCollision* ) data;
		coll->Serialize( file );
		return true;
	}
};

template<>
struct TTypeName< CPhysicalCollision >
{													
	static const CName& GetTypeName() { return CNAME( CPhysicalCollision ); }	
};
