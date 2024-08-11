/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "idGraph.h"
#include "idGraphBlockInteraction.h"
#include "../../common/engine/graphConnectionRebuilder.h"

IMPLEMENT_RTTI_ENUM( EIDBlockInteractionMode );
IMPLEMENT_ENGINE_CLASS( CIDGraphBlockInteraction );

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
CIDGraphBlockInteraction::CIDGraphBlockInteraction()
	: m_mode( IDBIM_StartInteraction )
{
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
String CIDGraphBlockInteraction::GetCaption() const
{
	String caption;
	if ( m_name )
	{
		caption = TXT("[");
		caption += m_name.AsString() + TXT("]\n\n");
	}

	switch ( m_mode )
	{
	case IDBIM_StartInteraction:
		caption += TXT("Start Interaction\n");
		caption += GetInteractionDisplayText();
		break;
	case IDBIM_StopInteraction:
		caption += TXT("Stop Interaction");
		break;
	case IDBIM_EnableInteraction:
		caption += TXT("Enable Interaction\n");
		caption += GetInteractionDisplayText();
		break;
	case IDBIM_DisableInteraction:
		caption += TXT("Disable Interaction\n");
		caption += GetInteractionDisplayText();
		break;
	}

	return caption;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
String const CIDGraphBlockInteraction::GetInteractionDisplayText()		const
{
	String name( TXT("") );

	if( m_entity.Get() )
	{
		CComponent*	component	=  m_entity.Get()->FindComponent( m_interactionComponent );
		if( component )
		{
			name	+= TXT(": ");
			CName	interactionName;
			CallFunctionRet< CName >( component, CNAME( GetInteractionName ), interactionName );
			//component->CallEvent< CName >( CNAME( GetInteractionName ), interactionName );
			if( interactionName )
			{
				name	+= interactionName.AsString();
			}
			else
			{
				name	+= m_interactionComponent.AsString();
			}
		}

		name	+= TXT(" on: ");
		name	+= m_entity.Get()->GetDisplayName();
	}

	return name;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
void CIDGraphBlockInteraction::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CreateSocket( SIDGraphSocketSpawnInfo( CNAME( In ), LSD_Input ) );
	m_output = Cast< CIDGraphSocket > ( CreateSocket( SIDGraphSocketSpawnInfo( CNAME( Out ), LSD_Output ) ) );
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
const CIDGraphBlock* CIDGraphBlockInteraction::Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const
{
	switch ( m_mode )
	{
	case IDBIM_StartInteraction:
		StartInteraction();
		break;
	case IDBIM_StopInteraction:
		StopInteraction();
		break;
	case IDBIM_EnableInteraction:
		SetInteractionEnabled( true );
		break;
	case IDBIM_DisableInteraction:
		SetInteractionEnabled( false );
		break;
	}

	return ActivateOutput( topicInstance, timeDelta, m_output );
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
void const CIDGraphBlockInteraction::StartInteraction() const
{
	CR6Player* player = Cast< CR6Player >( GGame->GetPlayerEntity() );

	if( m_entity.Get() )
	{
		CComponent*	component = m_entity.Get()->FindComponent( m_interactionComponent );
		player->CallEvent( CNAME( OnStartInteracting ), m_entity, component );
	}
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
void const CIDGraphBlockInteraction::StopInteraction() const
{
	CR6Player* player = Cast< CR6Player >( GGame->GetPlayerEntity() );
	player->CallEvent( CNAME( OnStopInteracting ) );
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
void const CIDGraphBlockInteraction::SetInteractionEnabled( Bool enabled ) const
{
	if( m_entity.Get() )
	{
		CComponent*	component	=  m_entity.Get()->FindComponent( m_interactionComponent );
		if( component )
		{
			component->SetEnabled( enabled );
		}
	}
}
	
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
Color CIDGraphBlockInteraction::GetTitleColor() const
{
	return Color( 0x22443300 );
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
Color CIDGraphBlockInteraction::GetClientColor() const
{
	return Color::GREEN;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
EGraphBlockShape CIDGraphBlockInteraction::GetBlockShape() const
{
	return GBS_Octagon;
}


