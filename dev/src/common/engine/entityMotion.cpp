#include "build.h"
#include "entityMotion.h"
#include "entity.h"
////////////////
// CEntityMotion

void CEntityMotion::SerializeForGC( IFile& file )
{
	// Make sure the collector wont collect the entity (if any)
	if ( m_entity )
	{
		file << m_entity;
	}
}

void CEntityMotion::OnStart()
{
	// Subclasses are expected to do something here
}

void CEntityMotion::OnEnd()
{
	// Subclasses are expected to do something here
}

void CEntityMotion::UpdateMotion()
{
	// Subclasses are expected to use m_time and do something here
}

void CEntityMotion::Tick( Float timeDelta )
{
	Float currentTime = m_motionManager->GetMotionTime();

	// Make sure we still have time
	if ( currentTime < m_endTime )
	{
		// Calculate normalized time
		m_time = ( currentTime - m_startTime ) / ( m_endTime - m_startTime );

		// Update the motion
		UpdateMotion();
	}
	else // Nope, ran out
	{
		// Last update with time=1
		m_time = 1.0f;
		UpdateMotion();

		// Inform the motion
		OnEnd();

		// Request destruction
		RequestDestroy();
	}
}

void CEntityMotion::RequestDestroy()
{
	m_motionManager->RequestDestroy( this );
}


///////////////////////
// CEntityMotionManager

CEntityMotionManager::CEntityMotionManager( class CWorld* world )
	: m_world( world )
	, m_motionTime( 0 )
{
}

CEntityMotionManager::~CEntityMotionManager()
{
	// Destroy all motions
	for ( CEntityMotion* motion : m_motions )
	{
		delete motion;
	}

	// Destroy them
	DestroyMotions();
}

void CEntityMotionManager::SerializeForGC( IFile& file )
{
	// Ask the motions to serialize their stuff
	for ( CEntityMotion* motion : m_motions )
	{
		// Only serialize entities with valid handle
		if ( motion->m_entityHandle.Get() != NULL )
		{
			motion->SerializeForGC( file );
		}
	}
}

void CEntityMotionManager::Tick( Float timeDelta )
{
	// Update time
	m_motionTime += timeDelta;

	// Tick the motions
	for ( CEntityMotion* motion : m_motions )
	{
		// Only update entities with a valid handle (there is a posibility for an entity
		// to be destroyed by a layer while a motion is still active)
		if ( motion->m_entityHandle.Get() != NULL )
		{
			motion->Tick( timeDelta );
		}
		else
		{
			// Invalid entity handle: remove the entity motion from the motion queue
			RequestDestroy( motion );
		}
	}

	// Destroy any motions requested to be destroyed
	DestroyMotions();
}

void CEntityMotionManager::AddMotion( CEntityMotion* motion, CEntity* entity, Float duration )
{
	// Make sure there is some duration to the motion
	if ( duration <= 0.0f )
	{
		return;
	}

	ASSERT( motion, TXT("Invalid motion passed in CEntityMotionManager::AddMotion") );
	ASSERT( !m_motions.Exist( motion ), TXT("Motion added twice in motion manager!") );

	if ( motion )
	{
		// Fill in motion values
		motion->m_startTime = GetMotionTime();
		motion->m_endTime = motion->m_startTime + duration;
		motion->m_entity = entity;
		motion->m_entityHandle = THandle< CEntity >( entity );
		motion->m_initialPosition = entity->GetPosition();
		motion->m_initialRotation = entity->GetRotation();
		motion->m_initialScale = entity->GetScale();
		motion->m_motionManager = this;
		motion->m_time = 0.0f;

		// Prepare the motion
		motion->OnStart();

		// Add the motion to the set
		m_motions.Insert( motion );
	}
}

Bool CEntityMotionManager::IsRunning( CEntityMotion* motion ) const
{
	return m_motions.Exist( motion );
}

void CEntityMotionManager::RequestDestroy( CEntityMotion* motion )
{
	m_toDestroy.PushBackUnique( motion );
}

void CEntityMotionManager::DestroyMotions()
{
	for ( CEntityMotion* motion : m_toDestroy )
	{
		m_motions.Erase( motion );
		delete motion;
	}

	m_toDestroy.ClearFast();
}
