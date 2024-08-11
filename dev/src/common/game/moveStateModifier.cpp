#include "build.h"
#include "moveStateModifier.h"


///////////////////////////////////////////////////////////////////////////////

CMoveCollidabilityModifier::CMoveCollidabilityModifier( Bool collidable )
	: m_wasCollidable( true )
	, m_collidable( collidable )
{
}

void CMoveCollidabilityModifier::Activate( CMovingAgentComponent& agent )
{
	m_wasCollidable = agent.IsCollidable();
	agent.SetCollidable( m_collidable );
}

void CMoveCollidabilityModifier::Deactivate( CMovingAgentComponent& agent )
{
	agent.SetCollidable( m_wasCollidable );
}

///////////////////////////////////////////////////////////////////////////////
