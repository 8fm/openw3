/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeConditionExternalToggle.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionExternalToggleDefinition )

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionExternalToggleDefinition
////////////////////////////////////////////////////////////////////////
String CBehTreeNodeConditionExternalToggleDefinition::GetNodeCaption() const
{
	String baseCaption =
		String::Printf(TXT("ConditionToggle: %s"),
		m_switchName.Empty() ? TXT("EMPTY") : m_switchName.AsChar() );

	return DecorateCaption( baseCaption );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionExternalToggleInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeConditionExternalToggleInstance::CBehTreeNodeConditionExternalToggleInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_switchName( def.m_switchName )
	, m_initialValue( def.m_initialValue.GetVal( context ) )
	, m_value( m_initialValue )
{
	if ( !m_switchName.Empty() )
	{
		SBehTreeEvenListeningData e;
		e.m_eventName = m_switchName;
		e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		context.AddEventListener( e, this );
	}	
}
void CBehTreeNodeConditionExternalToggleInstance::OnDestruction()
{
	if ( !m_switchName.Empty() )
	{
		SBehTreeEvenListeningData e;
		e.m_eventName = m_switchName;
		e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		m_owner->RemoveEventListener( e, this );
	}	

	Super::OnDestruction();
}

Bool CBehTreeNodeConditionExternalToggleInstance::ConditionCheck()
{
	return m_value;
}

Bool CBehTreeNodeConditionExternalToggleInstance::OnListenedEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == m_switchName )
	{
		SGameplayEventParamInt* data = e.m_gameplayEventData.Get< SGameplayEventParamInt >();
		if ( data )
		{
			Bool request;
			switch( data->m_value )
			{
			case TOGGLE_OFF:
				request = false;
				break;
			case TOGGLE_ON:
				request = true;
				break;
			default:
			case TOGGLE_RESET_TO_DEFAULTS:
				request = m_initialValue;
				break;
			}
			if ( m_value != request )
			{
				m_value = request;
				return true;
			}
		}
	}
	return false;
}