/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeConditionSpeedEngineVal.h"

#include "behTreeInstance.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionSpeedEngineValDefinition )

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionSpeedEngineValDefinition
///////////////////////////////////////////////////////////////////////////////
String CBehTreeNodeConditionSpeedEngineValDefinition::GetNodeCaption() const
{
	String valDesc = !m_speed.m_varName.Empty() ? m_speed.m_varName.AsString() : String::Printf( TXT("%0.2f"), m_speed.m_value );
	return String::Printf( TXT("EngineVal speed %s %s"), m_invertAvailability ? TXT("<") : TXT(">="), valDesc.AsChar() );
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionSpeedEngineValInstance
///////////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeConditionSpeedEngineValInstance::ConditionCheck()
{
	CActor* actor = m_owner->GetActor();
	CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return false;
	}
	return mac->GetRelativeMoveSpeed() >= m_speed;
}