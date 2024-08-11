/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "idGraph.h"
#include "idTopic.h"
#include "idInstance.h"
#include "idGraphBlockComunicatorSwitch.h"
#include "idHud.h"
#include "../../common/engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CIDGraphBlockComunicatorSwitch )

//------------------------------------------------------------------------------------------------------------------
// Graph block
//------------------------------------------------------------------------------------------------------------------
CIDGraphBlockComunicatorSwitch::CIDGraphBlockComunicatorSwitch()
	: m_communicatorMode( DDM_ActiveCall )
{
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
String CIDGraphBlockComunicatorSwitch::GetCaption() const
{
	if ( m_name )
	{
		return m_name.AsString();
	}

	switch ( m_communicatorMode )
	{
	case DDM_ActiveDialog:
		return TXT("Live Dialog");
	case DDM_IncomingCall:
		return TXT("Incomming Call");
	case DDM_ActiveCall:
		return TXT("Communicator");
	case DDM_SMSReceived:
		return TXT("SMS");
	}

	return TXT("Live Dialog");
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIDGraphBlockComunicatorSwitch::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CreateSocket( SIDGraphSocketSpawnInfo( CNAME( In ), LSD_Input ) );
	m_output = Cast< CIDGraphSocket > ( CreateSocket( SIDGraphSocketSpawnInfo( CNAME( Out ), LSD_Output ) ) );
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
const CIDGraphBlock* CIDGraphBlockComunicatorSwitch::Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const
{
	CInteractiveDialogInstance*	dialog	= topicInstance->GetDialogInstance();
	dialog->SetComunicator( m_communicatorMode );

	return ActivateOutput( topicInstance, timeDelta, m_output );
}
	
//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
Color CIDGraphBlockComunicatorSwitch::GetTitleColor() const
{
	return Color( 0xff444400 );
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
Color CIDGraphBlockComunicatorSwitch::GetClientColor() const
{
	return Color::MAGENTA;
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
EGraphBlockShape CIDGraphBlockComunicatorSwitch::GetBlockShape() const
{
	return GBS_Octagon;
}
