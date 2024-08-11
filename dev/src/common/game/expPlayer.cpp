
#include "build.h"
#include "expPlayer.h"
#include "expOracle.h"
#include "expComponent.h"


ExpPlayer::ExpPlayer( const SExplorationQueryToken& token, const ExpManager* dir, CEntity* ent, const THandle< IScriptable >& listener )
	: m_executor( NULL )
	, m_owner( ent )
	, m_director( dir )
	, m_token( token )
	, m_isFinished( false )
	, m_listener( listener )
{
}

ExpPlayer::~ExpPlayer()
{
	delete m_executor;

	SendEndNotification();
}

Bool ExpPlayer::Init()
{
	ExecutorSetup setup( m_owner, m_token );

	m_executor = m_director->CreateTransition( NULL, m_token.GetExploration(), setup );
	if( !m_executor ) return false;

	SendStartNotification();

	return true;
}

void ExpPlayer::Update( Float dt )
{
	ProcessNNQuery();

	Bool running = true;

	while ( running && m_executor && !m_isFinished )
	{
		running = false;

		ExpExecutorContext context;
		context.m_dt = dt;
		context.m_nnQueryLastResult = GetNNQueryLastResult();

		ExpExecutorUpdateResult result;

		m_executor->Update( context, result );

		if ( result.m_ragdoll )
		{

		}
		else if ( result.m_dead )
		{

		}
		else if ( result.m_jumpEnd )
		{

		}

		if ( result.m_nextExe )
		{
			delete m_executor;
			m_executor = result.m_nextExe;

			if ( result.m_timeRest > 0.f )
			{
				dt = result.m_timeRest;
				running = true;
			}
		}
		else if ( result.m_finished )
		{
			m_isFinished = true;
		}

		if ( result.m_nnQueryRequest.HasQuery() )
		{
			RequestNNQuery( result.m_nnQueryRequest );

			if ( m_pendingQuery.IsReady() )
			{
				delete m_executor;

				ExecutorSetup setup( m_owner, m_token );
				m_executor = m_director->CreateTransition( m_pendingQuery.m_from, m_pendingQuery.m_to, setup );
			}
		}
		else
		{
			m_pendingQuery.Reset();
		}

		SendNotifications( result );
	}
}

void ExpPlayer::RequestNNQuery( const ExpNNQueryData& query )
{
	if ( !(m_pendingQuery.m_from == query.m_from && m_pendingQuery.m_direction == query.m_direction ) )
	{
		// Set new query
		m_pendingQuery = query;
	}
}

void ExpPlayer::ProcessNNQuery()
{
	if ( m_pendingQuery.HasQuery() && !m_pendingQuery.m_to )
	{
		m_pendingQuery.m_to = m_director->GetNNForExploration( m_pendingQuery.m_from, m_pendingQuery.m_direction, m_owner );
	}
}

ExpNNQueryData ExpPlayer::GetNNQueryLastResult() const
{
	return m_pendingQuery;
}

void ExpPlayer::ConnectListener( const THandle< IScriptable >& object )
{
	ASSERT( !m_listener );
	m_listener = object;
}

void ExpPlayer::DisconnectListener()
{
	m_listener = NULL;
}

Bool ExpPlayer::HasListener() const
{
	return m_listener.IsValid();
}

Bool ExpPlayer::IsRunning() const
{
	return !m_isFinished;
}

void ExpPlayer::OnActionStoped()
{
	if ( m_executor )
	{
		m_executor->OnActionStoped();
	}
}

RED_DEFINE_STATIC_NAME( OnAnimationStarted );
RED_DEFINE_STATIC_NAME( OnAnimationFinished );
RED_DEFINE_STATIC_NAME( OnExplorationEvent );
RED_DEFINE_STATIC_NAME( OnSlideFinished );
RED_DEFINE_STATIC_NAME( OnExplorationStarted );
RED_DEFINE_STATIC_NAME( OnExplorationFinished );

void ExpPlayer::SendStartNotificationToObject( const THandle< IScriptable >& object )
{
	if ( object )
	{
		THandle< CEntity > entityH( const_cast< CEntity* >( m_owner ) );
		object->CallEvent( CNAME( OnExplorationStarted ), entityH );
	}
}

void ExpPlayer::SendEndNotificationToObject( const THandle< IScriptable >& object )
{
	if ( object )
	{
		THandle< CEntity > entityH( const_cast< CEntity* >( m_owner ) );
		object->CallEvent( CNAME( OnExplorationFinished ), entityH );
	}
}

void ExpPlayer::SendNotificationsToObject( const ExpExecutorUpdateResult& result, const THandle< IScriptable >& object )
{
	if ( !object )
	{
		return;
	}

	// We cannot use const in scripts
	THandle< CEntity > entityH( const_cast< CEntity* >( m_owner ) );

	const Uint32 size = result.m_notifications.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const ExplorationNotification& notification = result.m_notifications[ i ];

		switch ( notification.m_first )
		{
		case ENT_AnimationStarted:
			{
				object->CallEvent( CNAME( OnAnimationStarted ), entityH, notification.m_second.m_name );
				break;
			}
		case ENT_AnimationFinished:
			{
				object->CallEvent( CNAME( OnAnimationFinished ), entityH, notification.m_second.m_name );
				break;
			}
		case ENT_Event:
			{
				object->CallEvent( CNAME( OnExplorationEvent ), entityH, notification.m_second.m_name );
				break;
			}
		case ENT_SlideFinished:
			{
				object->CallEvent( CNAME( OnSlideFinished ), entityH );
				break;
			}
		}
	}
}

void ExpPlayer::SendStartNotification()
{
	if ( m_token.GetExploration() )
	{
		CObject* object = m_token.GetExploration()->GetObjectForEvents();
		SendStartNotificationToObject( object );
	}
	SendStartNotificationToObject( m_listener );
}

void ExpPlayer::SendEndNotification()
{
	if ( m_token.GetExploration() )
	{
		CObject* object = m_token.GetExploration()->GetObjectForEvents();
		SendEndNotificationToObject( object );
	}
	SendEndNotificationToObject( m_listener );
}

void ExpPlayer::SendNotifications( const ExpExecutorUpdateResult& result )
{
	if ( m_token.GetExploration() )
	{
		CObject* object = m_token.GetExploration()->GetObjectForEvents();
		SendNotificationsToObject( result, object );
	}
	SendNotificationsToObject( result, m_listener );
}

void ExpPlayer::GenerateDebugFragments( CRenderFrame* frame )
{
	if ( m_executor )
	{
		m_executor->GenerateDebugFragments( frame );
	}
}
