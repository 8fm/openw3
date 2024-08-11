/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "idGraphBlockRequestFocus.h"
#include "idTopic.h"
#include "idInstance.h"
#include "idSystem.h"
#include "../../common/engine/graphConnectionRebuilder.h"


IMPLEMENT_RTTI_ENUM( EIDFocusMode )
IMPLEMENT_ENGINE_CLASS( CIDGraphBlockRequestFocus )


//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
String CIDGraphBlockRequestFocus::GetCaption() const
{
	String name;
	if( m_name )
	{
		name	= m_name.AsString() + TXT(": ");
	}

	switch ( m_focus )
	{
	case IDFM_GetFocus:
		name += TXT("Get focus");
		break;
	case IDFM_LoseFocus:
		name += TXT("Lose focus");
		break;
	case IDFM_RecalcFocus:
		name += TXT("Recalc focus");
		break;
	}

	return name;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
Color CIDGraphBlockRequestFocus::GetClientColor() const
{
	return Color::LIGHT_YELLOW;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
Color CIDGraphBlockRequestFocus::GetTitleColor() const
{
	return Color( 0xff444400 );
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
EGraphBlockShape CIDGraphBlockRequestFocus::GetBlockShape() const
{
	return GBS_Octagon;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
void CIDGraphBlockRequestFocus::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CreateSocket( SIDGraphSocketSpawnInfo( CNAME( In ), LSD_Input ) );
	CreateSocket( SIDGraphSocketSpawnInfo( CNAME( Out ), LSD_Output ) );
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
const CIDGraphBlock* CIDGraphBlockRequestFocus::Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const
{
	CInteractiveDialogInstance*		dialog	= topicInstance->GetDialogInstance();
	RED_ASSERT( dialog, TXT("Evaluating a block with no dialog instance, should this be allowed? (Maybe when using connectors)"));
	if( dialog )
	{
		switch ( m_focus )
		{
		case IDFM_GetFocus:
			GCommonGame->GetSystem < CInteractiveDialogSystem > ()->RequestFocus( dialog );
			break;
		case IDFM_LoseFocus:
			if( dialog->GetHasFocus() )
			{
				GCommonGame->GetSystem < CInteractiveDialogSystem > ()->RequestFocusEnd( dialog );
			}
			break;
		case IDFM_RecalcFocus:
			if( dialog->GetHasFocus() )
			{
				GCommonGame->GetSystem < CInteractiveDialogSystem > ()->RequestFocusRecalc();
			}
			break;
		}
	}
	return ActivateOutput( topicInstance, timeDelta, Cast< const CIDGraphSocket > ( FindSocket( CNAME( Out ) ) ) );
}