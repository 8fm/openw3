/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "idGraph.h"
#include "idBasicBlocks.h"
#include "idGraphBlockConnector.h"
#include "idInterlocutor.h"
#include "idTopic.h"
#include "idInstance.h"
#include "idConnector.h"

#include "../../common/core/instanceDataLayoutCompiler.h"
#include "../../common/engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CIDGraphBlockConnector )

//------------------------------------------------------------------------------------------------------------------
// Graph block
//------------------------------------------------------------------------------------------------------------------
void CIDGraphBlockConnector::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CreateSocket( SIDGraphSocketSpawnInfo( CNAME( In ), LSD_Input ) );
	CreateSocket( SIDGraphSocketSpawnInfo( CNAME( Out ), LSD_Output ) );
}

void CIDGraphBlockConnector::OnInitInstance( InstanceBuffer& data ) const
{
	TBaseClass::OnInitInstance( data );

	data[ i_request ] = nullptr;
}

void CIDGraphBlockConnector::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_request;
}

const CIDGraphBlock* CIDGraphBlockConnector::ActivateInput( CIDTopicInstance* topicInstance, Float& timeDelta, const CIDGraphSocket* input, Bool evaluate /*= true*/ ) const
{
	CIDInterlocutorComponent* component = topicInstance->GetDialogInstance()->GetInterlocutor( m_speaker );
	if ( nullptr == component )
	{
		RED_LOG( Dialog, TXT("Speaker %s not found in dialog %s"), m_speaker.AsString().AsChar(), topicInstance->GetDialogInstance()->GetResourceHandle().GetPath().AsChar() );
		TBaseClass::ActivateInput( topicInstance, timeDelta, input, evaluate );
		return ActivateOutput( topicInstance, timeDelta, input, evaluate );
	}

	// delete previous request
	InstanceBuffer& data = topicInstance->GetInstanceData();
	SIDConnectorRequest *request = reinterpret_cast< SIDConnectorRequest* > ( data[ i_request ] );
	if ( request )
	{
		delete request;
		data[ i_request ] = nullptr;
	}

	// And place a new one
	request = new SIDConnectorRequest( m_category, m_speaker, topicInstance->GetDialogInstance()->GetInstanceID() );
	component->RequestConnector( *request );
	data[ i_request ] = reinterpret_cast< TGenericPtr > ( request );

	if ( request->m_state == DIALOG_Error )
	{
		RED_LOG( Dialog, TXT("Connector of category %s not found for speaker %s"), m_category.AsString().AsChar(), m_speaker.AsString().AsChar() );
		TBaseClass::ActivateInput( topicInstance, timeDelta, input, evaluate );
		return ActivateOutput( topicInstance, timeDelta, input, evaluate );
	}

	R6_ASSERT( request->m_state == DIALOG_Loading || request->m_state == DIALOG_Ready );

	// pass to base class
	return TBaseClass::ActivateInput( topicInstance, timeDelta, input, evaluate );
}

const CIDGraphBlock* CIDGraphBlockConnector::Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const
{
	InstanceBuffer&	data = topicInstance->GetInstanceData();
	SIDConnectorRequest *request = reinterpret_cast< SIDConnectorRequest* > ( data[ i_request ] );
	R6_ASSERT( request );

	CIDInterlocutorComponent* component = topicInstance->GetDialogInstance()->GetInterlocutor( m_speaker );
	if ( nullptr == component )
	{
		RED_LOG( Dialog, TXT("Speaker %s not found in dialog %s"), m_speaker.AsString().AsChar(), topicInstance->GetDialogInstance()->GetResourceHandle().GetPath().AsChar() );
		return ActivateOutput( topicInstance, timeDelta, Cast< const CIDGraphSocket > ( FindSocket( CNAME( Out ) ) ) );
	}

	// Update the request
	component->RequestConnector( *request );

	if ( request->m_state == DIALOG_Error )
	{
		RED_LOG( Dialog, TXT("Connector of category %s not found for speaker %s"), m_category.AsString().AsChar(), m_speaker.AsString().AsChar() );
		return ActivateOutput( topicInstance, timeDelta, Cast< const CIDGraphSocket > ( FindSocket( CNAME( Out ) ) ) );
	}
	else if ( request->m_state == DIALOG_Loading )
	{
		// have to wait...
		timeDelta = 0.f;
		return this;
	}
	
	// No error, not loading, so it should be ready now
	R6_ASSERT( request->m_state == DIALOG_Ready && request->m_result );
	const SIDConnectorLine* line = request->m_result;

	if ( nullptr == line )
	{
		RED_LOG( Dialog, TXT("Connector of category %s not found for speaker %s"), m_category.AsString().AsChar(), m_speaker.AsString().AsChar() );
		return ActivateOutput( topicInstance, timeDelta, Cast< const CIDGraphSocket > ( FindSocket( CNAME( Out ) ) ) );
	}

	Uint32 dialogId( topicInstance->GetDialogInstance()->GetInstanceID() );
	EIDLineState ls = component->GetLineStatus( *line, dialogId );
	switch ( ls )
	{
		case DILS_CancelledOrNotPlayed:
		{
			component->TryToPlayLine( *line, dialogId );
			break;
		}
		case DILS_Completed:
		{
			return ActivateOutput( topicInstance, timeDelta, Cast< const CIDGraphSocket > ( FindSocket( CNAME( Out ) ) ) );
		}
		case DILS_Playing:
		case DILS_Queued:
		{
			// Consume the time
			timeDelta = 0.f;
			break;
		}
	default:
		R6_ASSERT( false, TXT("Missing line state to implement in the class CIDGraphBlockConnector") );
		break;
	}

	return this;
 }

const CIDGraphBlock* CIDGraphBlockConnector::ActivateOutput( CIDTopicInstance* topicInstance, Float& timeDelta, const CIDGraphSocket* output, Bool evaluate /*= true*/ ) const
{
	InstanceBuffer& data = topicInstance->GetInstanceData();
	SIDConnectorRequest* request = reinterpret_cast< SIDConnectorRequest* > ( data[ i_request ] );
	const SIDConnectorLine* line = request ? request->m_result : NULL;
	CIDInterlocutorComponent* component = topicInstance->GetDialogInstance()->GetInterlocutor( m_speaker );

	// Release the Kraken
	if ( component && line )
	{
		// Since we can release the memory of a line in ReleaseConnector(), we have to make sure that hud don't use it anymore
		topicInstance->GetDialogInstance()->EndLine( component, *line );
		component->ReleaseConnector( line );
	}
	
	data[ i_request ] = nullptr;
	
	if ( request )
	{
		delete request;
	}

	return TBaseClass::ActivateOutput( topicInstance, timeDelta, output, evaluate );
}
  
Color CIDGraphBlockConnector::GetClientColor() const
{
	return Color( 0xff777777 );
}
