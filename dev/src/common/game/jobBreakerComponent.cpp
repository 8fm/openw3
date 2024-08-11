#include "build.h"

#include "jobBreakerComponent.h"

#include "actorsManager.h"
#include "gameplayStorage.h"
#include "../engine/tickManager.h"

const Float CJobBreakerComponent::UPDATE_INTERVAL		= 0.25f;
const Int32 CJobBreakerComponent::MAX_PUSHED_ACTORS	= 5;

IMPLEMENT_ENGINE_CLASS( CJobBreakerComponent );

void CJobBreakerComponent::OnAttachFinished( CWorld* world )
{
	TBaseClass::OnAttachFinished( world );

	m_movingAgentComponent = GetEntity()->FindComponent< CMovingAgentComponent >( );
	if( !m_movingAgentComponent.Get() )
	{
		return;
	}
	m_timeToTick = 0;
	world->GetTickManager()->AddToGroup( this, TICK_Main );
}

void CJobBreakerComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	world->GetTickManager()->RemoveFromGroup( this, TICK_Main );
}

void CJobBreakerComponent::OnTick( Float timeDelta )
{
	PC_SCOPE( CJobBreakerComponent );

	TBaseClass::OnTick( timeDelta );

	if( m_timeToTick <= 0 )
	{
		TickImpl( );
		m_timeToTick = UPDATE_INTERVAL;
	}
	else
	{
		m_timeToTick -= timeDelta;
	}
}

void CJobBreakerComponent::TickImpl( )
{
	if( !m_movingAgentComponent.Get() )
	{
		return;
	}
	Vector diraction = EulerAngles::YawToVector( m_movingAgentComponent.Get()->GetMoveDirectionWorldSpace() );
	Vector position = GetEntity()->GetWorldPosition();

	CActor* pushedActors[ MAX_PUSHED_ACTORS ];

	Int32 results = GCommonGame->GetActorsManager()->CollectActorsAtLine( position, position + diraction * m_distance, m_radius, pushedActors, MAX_PUSHED_ACTORS );	

	for( Int32 i=0; i<results; ++i )
	{
		CNewNPC* npc = Cast< CNewNPC >( pushedActors[ i ] );
		if( npc )
		{
			npc->SignalGameplayEvent( CNAME( AI_GetOutOfTheWay ) );
		}

	}
}