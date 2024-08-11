/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeDecoratorGlide.h"

#include "../../common/engine/behaviorGraphStack.h"


BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeDecoratorGlideDefinition )

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorGlideInstance
///////////////////////////////////////////////////////////////////////////////
void CBehTreeNodeDecoratorGlideInstance::SetBehaviorVal( Float v ) const
{
	CActor* actor = m_owner->GetActor();
	CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
	auto* behaviorStack = mac->GetBehaviorStack();
	if ( behaviorStack )
	{
		behaviorStack->SetBehaviorVariable( CNAME( Fly2Glide ), v );
	}
}
void CBehTreeNodeDecoratorGlideInstance::Update()
{
	Float time = m_owner->GetLocalTime();
	if ( m_isGliding && m_flightData->IsInEmergencyMode() )
	{
		m_testTimeout = time + m_minChangeDelay;
		SetBehaviorVal( 0.f );
		m_isGliding = false;
	}
	else if ( m_testTimeout < time )
	{
		m_testTimeout = time + 1.f;
		auto& randomGenerator = GEngine->GetRandomNumberGenerator();
		if ( m_isGliding )
		{
			if ( randomGenerator.Get< Float >() < m_stopGlideChance )
			{
				// stop glide
				SetBehaviorVal( 0.f );
				m_isGliding = false;
			}
		}
		else
		{
			if ( randomGenerator.Get< Float >() < m_glideChance )
			{
				// start glide
				SetBehaviorVal( 1.f );
				m_isGliding = true;
			}
		}
	}
	Super::Update();
}

void CBehTreeNodeDecoratorGlideInstance::Deactivate()
{
	if ( m_isGliding )
	{
		CActor* actor = m_owner->GetActor();
		if ( actor )
		{
			CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
			auto* behaviorStack = mac ? mac->GetBehaviorStack() : nullptr;
			if ( behaviorStack )
			{
				behaviorStack->SetBehaviorVariable( CNAME( Fly2Glide ), 0.f );
			}
		}
		m_isGliding = false;
	}
	
	Super::Deactivate();
}
