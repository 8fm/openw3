
#include "build.h"

#include "../../common/engine/behaviorGraphStack.h"

#include "expTraverser.h"
#include "expPlayer.h"

ExpTraverser::ExpTraverser( const SExplorationQueryToken& token, ExpManager* dir, CEntity* ent )
	: m_owner( ent )
{
	m_inputHor = CNAME( GI_AxisLeftX );
	m_inputVer = CNAME( GI_AxisLeftY );

	ASSERT( token.IsValid() );

	m_owner->CallEvent( CNAME( OnNewExploration ) );

	CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( m_owner->GetRootAnimatedComponent() );
	if ( mac )
	{
		mac->ForceEntityRepresentation( true );

		if ( mac->GetBehaviorStack() )
		{
			mac->GetBehaviorStack()->Lock( true );
		}
	}

	m_player = new ExpPlayer( token, dir, m_owner );
}

void ExpTraverser::Release()
{
	CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( m_owner->GetRootAnimatedComponent() );
	if ( mac )
	{
		if ( mac->GetBehaviorStack() )
		{
			mac->GetBehaviorStack()->Lock( false );
		}

		mac->ForceEntityRepresentation( false );
	}

	m_owner->CallEvent( CNAME( OnNewExplorationFinished ) );
}

ExpTraverser::~ExpTraverser()
{
	delete m_player;
	m_player = NULL;
}

void ExpTraverser::Update( Float dt )
{
	if ( m_player )
	{
		m_player->Update( dt );
	}
}

Bool ExpTraverser::IsRunning() const
{
	return m_player && m_player->IsRunning();
}

namespace
{
	RED_INLINE Bool IsKeyPressed( Float var )
	{
		return var > 0.5f;
	}
};

void ExpTraverser::GenerateDebugFragments( CRenderFrame* frame )
{
	if ( m_player )
	{
		m_player->GenerateDebugFragments( frame );
	}
}
