/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

IMPLEMENT_RTTI_ENUM( EActorActionType );

ActorAction::ActorAction( CActor* actor, EActorActionType type )
	: m_actor( actor )
	, m_type( type )
{};

ActorAction::~ActorAction()
{
}

Bool ActorAction::Update( Float timeDelta )
{
	return true;
}

void ActorAction::OnBeingPushed( const Vector& direction, Float rotation, Float speed, EPushingDirection animDirection )
{
	// actor's not performing any action - so just slide it
	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( mac )
	{
		mac->Slide( direction, EulerAngles( 0, 0, rotation ), &speed );
	}
}
