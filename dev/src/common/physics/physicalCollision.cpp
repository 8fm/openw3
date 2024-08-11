#include "build.h"
#include "physicsEngine.h"
#include "physicalCollision.h"

CPhysicsEngine::CollisionMask CPhysicalCollision::COLLIDES_ALL = ( CPhysicsEngine::CollisionMask ) 0x00FFFFFFFFFFFFFF;
Uint32 CPhysicalCollision::PHYSICAL_COLLISION_VERSION( 2 );

//FIXME>>>>>: NAMES Hacks because the class was named CPhysicalCollision instead of PhysicalCollision...
RED_DEFINE_RTTI_NAME( CPhysicalCollision ); // Needs to come before IMPLEMENT_RTTI_TYPE so CName is initialized
IMPLEMENT_RTTI_TYPE( CSimpleRTTITypePhysicalCollision ); // <- should have been IMPLEMENT_SIMPLE_RTTI_TYPE( PhysicalCollision );

void CPhysicalCollision::Serialize( IFile& file )
{
	Uint32 version;
	if ( file.IsWriter() )
	{
		version = PHYSICAL_COLLISION_VERSION;
	}
	file << version;
	if ( file.IsReader() )
	{
		if ( version != PHYSICAL_COLLISION_VERSION )
		{
			return;
		}
	}
	if ( file.IsWriter() )
	{
		// This should only happen with old data (i.e. non-cooked builds), but since we have so much old data right now, we leave it on
		ResolveCollisionMasks();
	}
	file << m_collisionTypeNames;
	file << m_collisionTypeMask;
	file << m_collisionGroupsMask;

	if( file.IsReader() )
	{
		// This should only happen with old data (i.e. non-cooked builds), but since we have so much old data right now, we leave it on
		ResolveCollisionMasks();
	}
}

void CPhysicalCollision::ResolveCollisionMasks()
{
	if( !GPhysicEngine )
	{
		return;
	}

	m_collisionTypeMask = 0;
	for( Int32 i = 0; i != ( Int32 ) m_collisionTypeNames.Size(); ++i )
	{
		CName name = m_collisionTypeNames[ i ];

		CPhysicsEngine::CollisionMask mask = GPhysicEngine->GetCollisionTypeBit( name );
		if( !mask )
		{
			m_collisionTypeNames.RemoveAt( i );
			i--;
			continue;
		}
		m_collisionTypeMask |= mask;
	}
	m_collisionGroupsMask = GPhysicEngine->GetCollisionGroupMask( m_collisionTypeMask );

}

void CPhysicalCollision::RetrieveCollisionMasks( CPhysicsEngine::CollisionMask& collsionTypeMask, CPhysicsEngine::CollisionMask& collisiontGroupMask )
{
	if( !m_collisionTypeMask && !m_collisionGroupsMask )
	{
		// This should only happen with old data (i.e. non-cooked builds), but since we have so much old data right now, we leave it on
		ResolveCollisionMasks();
	}

	collsionTypeMask = m_collisionTypeMask;
	collisiontGroupMask = m_collisionGroupsMask;
}

void CPhysicalCollision::RetrieveCollisionMasks( CPhysicsEngine::CollisionMask& collsionTypeMask, CPhysicsEngine::CollisionMask& collisiontGroupMask ) const
{
	collsionTypeMask = m_collisionTypeMask;
	collisiontGroupMask = m_collisionGroupsMask;
}

IFile& operator<<( IFile& file, CPhysicalCollision& physicalCollision )
{
	physicalCollision.Serialize( file );
	return file;
}

Bool CPhysicalCollision::operator==( const CPhysicalCollision& other ) const
{
	return other.m_collisionTypeNames == m_collisionTypeNames;
}

