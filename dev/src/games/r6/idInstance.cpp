/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "idResource.h"
#include "idInstance.h"
#include "idThread.h"
#include "idInterlocutor.h"
#include "idGraphBlockText.h"
#include "idCondition.h"
#include "idThreadActivator.h"
#include "idHud.h"
#include "r6DialogDisplayManager.h"


//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
CInteractiveDialogInstance::CInteractiveDialogInstance( SDialogStartRequest& info )
	: m_owner( info.m_owner )
	, m_initiatior( info.m_initiatior )
	, m_dialog( info.m_resource )
	, m_instanceID( info.m_startedInstanceID )
	, m_activeTopic( NULL )
	, m_focusedOnHud( false )
	, m_wantsFocusOnHUD( false )
	, m_displayingPage( NULL )
	, m_isOnComunicator( false )
	, m_huds(  NULL )
{
	R6_ASSERT( m_dialog.IsLoaded() ); // it needs to be loaded by now

	CInteractiveDialogSystem* system = GCommonGame->GetSystem< CInteractiveDialogSystem > ();

	const TDynArray< SIDInterlocutorDefinition >& actors = m_dialog.Get()->GetInterlocutorDefinitions(); 
	for ( Uint32 i = 0; i < actors.Size(); ++i )
	{
		CIDInterlocutorComponent* foundActor = system->FindActorForDialog( this, actors[ i ] );
		if ( foundActor == NULL )
		{
			RED_LOG( Dialog, TXT("Cannot find actor %s (voicetag %s, tag %s) for dialog %s"), 
				actors[ i ].m_interlocutorId.AsString().AsChar(), actors[ i ].m_voiceTagToMatch.AsString().AsChar(), actors[ i ].m_tagToMatch.AsString().AsChar(), m_dialog.Get()->GetFriendlyName().AsChar() );
			SetPlayState( DIALOG_Error );
			return;
		}

		m_knownInterlocutors.Insert( actors[ i ].m_interlocutorId, foundActor );
	}

	CreateAllTopicInstances();		
	m_displayingPage = new CIDPage();

	SetPlayState( DIALOG_Ready );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CInteractiveDialogInstance::Tick( Float timeDelta )
{
	R6_ASSERT( GetPlayState() != DIALOG_Error && GetPlayState() != DIALOG_Finished );

	HandleInterruptions( timeDelta );

	Bool allFinished	= true;

	// Tick all topics
	for ( Uint32 i = 0; i < m_allTopics.Size(); ++i )
	{
		CIDTopicInstance* topicInstance = m_allTopics[ i ];

		topicInstance->Tick( timeDelta );

		// Check topic endings
		EIDPlayState state = topicInstance->GetPlayState();
		if ( state == DIALOG_Playing || state == DIALOG_Interrupted )
		{
			allFinished	= false;
		}
		else if ( topicInstance == m_activeTopic )
		{
			m_activeTopic	= NULL;
		}
	}

	// state transitions
	if ( allFinished )
	{
		if( m_dialog.Get()->IsAutoFinish() )
		{
			OnFinished();
		}
	}	 
	else if ( GetPlayState() != DIALOG_Playing )
	{
		SetPlayState( DIALOG_Playing );
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CInteractiveDialogInstance::CreateAllTopicInstances()
{
	if ( m_owner == NULL || m_dialog.IsLoaded() == false )
	{
		R6_ASSERT( false ); // You should not be here...
		return;
	}

	const CInteractiveDialog* dialog = m_dialog.Get();

	const TDynArray< CIDTopic* >& topics = dialog->GetTopics();
	for( TDynArray< CIDTopic* >::const_iterator it	= topics.Begin(); it != topics.End(); ++it )
	{
		const CIDTopic* topicToRun = *it;

		CreateTopicInstance( topicToRun );
	}

	SortTopicInstances();
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CInteractiveDialogInstance::CreateTopicInstance( const CIDTopic* topic )
{
	if ( NULL == topic )
	{
		RED_LOG( CNAME( Dialog ), TXT("CInteractiveDialogInstance::RequestTopic(): topic %s not found. Owner: %s, Resource: %s"),
			topic->GetName().AsString().AsChar(), m_owner->GetFriendlyName().AsChar(), m_dialog.GetPath().AsChar() );
		return;
	}

	CIDTopicInstance* instance = new CIDTopicInstance( this, topic );
	EIDPlayState state = instance->GetPlayState();
	if ( state == DIALOG_Error )
	{
		delete instance;
		RED_LOG( CNAME( Dialog ), TXT("CInteractiveDialogInstance::RequestTopic(): error while trying to play topic %s. Owner: %s, Resource: %s"),
			topic->GetName().AsString().AsChar(), m_owner->GetFriendlyName().AsChar(), m_dialog.GetPath().AsChar() );
		return;
	}

	// If the instance is valid, add it to the list
	m_allTopics.PushBack( instance );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CInteractiveDialogInstance::SortTopicInstances()
{
	SIDTopicSortingPredicate predicate;
	Sort( m_allTopics.Begin(), m_allTopics.End(), predicate );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CInteractiveDialogInstance::HandleInterruptions( Float& timeDelta )
{
	CIDTopicInstance* topic	= FindTopicToPlay();

	if ( topic != nullptr && m_activeTopic != topic )
	{
		PlayNewTopic( topic, timeDelta );
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
CIDTopicInstance* CInteractiveDialogInstance::FindTopicToPlay() const
{
	if ( m_allTopics.Empty() )
	{
		return nullptr;
	}

	// They are already sorted by priority so we only need to play the first one
	for ( TDynArray< CIDTopicInstance* >::const_iterator it = m_allTopics.Begin(); it != m_allTopics.End(); ++it )
	{
		CIDTopicInstance* topic	= *it;

		if ( topic != nullptr && topic->GetWantsToPlay() )
		{
			return topic;
		}
	}

	return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CInteractiveDialogInstance::PlayNewTopic( CIDTopicInstance* topicInstance, Float& timeDelta )
{
	if ( m_activeTopic != topicInstance )
	{
		topicInstance->OnStart( timeDelta );

		if( m_activeTopic )
		{
			m_activeTopic->OnInterrupt();

			// Communicate to the HUD
			if( m_focusedOnHud )
			{
				for ( Uint32 i = 0; i < m_huds.Size(); ++i )
				{
					m_huds[i]->OnInterrupted();
				}
			}
		}

		m_activeTopic	= topicInstance;
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
Bool CInteractiveDialogInstance::GetWantsToPlay() const
{
	EIDPlayState state = GetPlayState();

	switch( state )
	{
	case DIALOG_Finished:
		return false;
	case DIALOG_Playing:
	case DIALOG_Interrupted:
		return true;
	case DIALOG_Ready:
		{
			CInteractiveDialog*	dialog		= m_dialog.Get();
			const IIDContition*	condition	= dialog->GetCondition();
			return ( condition == NULL ) ? true : condition->IsFulfilled( m_instanceID );
		}
	default:
		return false;
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
EIDPriority CInteractiveDialogInstance::GetHighestTopicPriority() const
{
	CIDTopicInstance*	topic = FindTopicToPlay();
	if( topic )
	{
		return topic->GetPriority();
	}
	return IDP_Invalid;
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CInteractiveDialogInstance::GainFocusOfInterlocutors( )
{
	for( THashMap< CName, CIDInterlocutorComponent* >::iterator it = m_knownInterlocutors.Begin(); it != m_knownInterlocutors.End(); ++it )
	{
		it->m_second->SetAttentionToDialog( this );
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CInteractiveDialogInstance::AttachHud( IDialogHud* hud )
{
	R6_ASSERT( !m_huds.Exist( hud ) );

	if( hud )
	{
		m_huds.PushBack( hud );
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CInteractiveDialogInstance::DetachHud( IDialogHud* hud )
{
	R6_ASSERT( m_huds.Exist( hud ) );
	m_huds.Remove( hud );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CInteractiveDialogInstance::OnGainedHUDFocus( )
{
	if( m_focusedOnHud )
	{
		return;
	}

	m_focusedOnHud	= true;

	// Add the current text to the HUD
	SetPageOnHUD();
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CInteractiveDialogInstance::OnLostHUDFocus( )
{
	if( !m_focusedOnHud )
	{
		return;
	}

	// CLear HUD
	for ( Uint32 i = 0; i < m_huds.Size(); ++i )
	{
		m_huds[ i ]->ClearDialogLines();
		m_huds[ i ]->RemoveAllChoices();
		//m_huds[ i ]->SetMode( DDM_ActiveCall );
	}

	m_focusedOnHud	= false;
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CInteractiveDialogInstance::SetPageOnHUD()
{
	for ( Uint32 i = 0; i < m_huds.Size(); ++i )
	{
		// Mode
		m_huds[ i ]->SetMode( m_displayingPage->m_communicatorMode );

		// Lines
		m_huds[ i ]->ClearDialogLines();

		for ( Uint32 j = 0; j < m_displayingPage->m_lines.Size(); ++j )
		{
			// Add on all huds
			m_huds[ i ]->PlayDialogLine( m_displayingPage->m_lines[ j ].m_speaker,  *( m_displayingPage->m_lines[ j ].m_line ) );
		}

		// Choices
		m_huds[ i ]->RemoveAllChoices();
		for ( Uint32 j = 0; j < m_displayingPage->m_options.Size(); ++j )
		{
			// Add on all huds
			m_huds[ i ]->AddChoice( *( m_displayingPage->m_options[ j ] ) );
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
Bool CInteractiveDialogInstance::PlayLine( CIDInterlocutorComponent* interlocutor, const SIDBaseLine& line )
{
	// Save it on the display page
	CIDPage::SSpeakerLine l;
	l.m_line = &line;
	l.m_speaker = interlocutor;
	m_displayingPage->m_lines.PushBackUnique( l );

	// Check if we don't want to show the text on screen
	if( !m_focusedOnHud )
	{
		return false;
	}

	for ( Uint32 i = 0; i < m_huds.Size(); ++i )
	{
		// Play on all huds
		m_huds[ i ]->PlayDialogLine( interlocutor, line );
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
Bool CInteractiveDialogInstance::EndLine( CIDInterlocutorComponent* interlocutor, const SIDBaseLine& line )
{
	CIDPage::SSpeakerLine l;
	l.m_line = &line;
	l.m_speaker = interlocutor;
	m_displayingPage->m_lines.Remove( l );
	/*
	// Check if we don't want to show the text on screen
	if( !m_focusedOnHud )
	{
		return false;
	}
	*/
	for ( Uint32 i = 0; i < m_huds.Size(); ++i )
	{
		// Play on all huds
		m_huds[ i ]->EndDialogLine( interlocutor );
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CInteractiveDialogInstance::AddChoice( const SIDOption& choice )
{
	// Save it on the display page
	m_displayingPage->m_options.PushBackUnique( &choice );

	if( !m_focusedOnHud )
	{
		return;
	}

	for ( Uint32 i = 0; i < m_huds.Size(); ++i )
	{
		// Add on all huds
		m_huds[ i ]->AddChoice( choice );
	}	
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CInteractiveDialogInstance::RemoveChoice( const SIDOption& choice )
{
	// Save it on the display page
	m_displayingPage->m_options.Remove( &choice );

	if( !m_focusedOnHud )
	{
		return;
	}

	for ( Uint32 i = 0; i < m_huds.Size(); ++i )
	{
		// Remove from all huds
		m_huds[ i ]->RemoveChoice( choice );
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CInteractiveDialogInstance::RemoveAllChoices()
{
	// Save it on the display page
	m_displayingPage->m_options.Clear();

	if( !m_focusedOnHud )
	{
		return;
	}

	for ( Uint32 i = 0; i < m_huds.Size(); ++i )
	{
		m_huds[ i ]->RemoveAllChoices();
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CInteractiveDialogInstance::OnChoiceSelected( EHudChoicePosition position )
{
	if ( m_activeTopic )
	{
		m_activeTopic->OnChoiceSelected( position );
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
CIDInterlocutorComponent* CInteractiveDialogInstance::GetInterlocutor( CName name ) const
{ 
	THashMap< CName, CIDInterlocutorComponent* >::const_iterator it = m_knownInterlocutors.Find( name ); 
	CIDInterlocutorComponent* interlocutor = ( it != m_knownInterlocutors.End() ) ? ( *it ).m_second : NULL;
	return interlocutor;
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CInteractiveDialogInstance::OnFinished()
{
	m_activeTopic = nullptr;
	SetPlayState( DIALOG_Finished );

	if ( m_focusedOnHud )
	{
		for ( Uint32 i = 0; i < m_huds.Size(); ++i )
		{
			m_huds[ i ]->RemoveAllChoices();
			m_huds[ i ]->ClearDialogLines();
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
Bool CInteractiveDialogInstance::HasInterlocutor( CName name ) const
{
	return m_knownInterlocutors.Find( name ) != m_knownInterlocutors.End(); 
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
Bool CInteractiveDialogInstance::HasInterlocutor( const CIDInterlocutorComponent* interlocutor ) const
{
	for ( THashMap< CName, CIDInterlocutorComponent* >::const_iterator it = m_knownInterlocutors.Begin(); it != m_knownInterlocutors.End(); ++it )
	{
		if ( it->m_second == interlocutor )
		{
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CInteractiveDialogInstance::SetComunicator( EDialogDisplayMode comunicatorMode )
{ 
	m_displayingPage->m_communicatorMode	= comunicatorMode;

	m_isOnComunicator = comunicatorMode != DDM_ActiveDialog;
	for ( Uint32 i = 0; i < m_huds.Size(); ++i )
	{
		m_huds[i]->SetMode( comunicatorMode );
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CInteractiveDialogInstance::Debug_GatherThreadInfos( TDynArray< Debug_SIDThreadInfo >& infos ) const
{
	Uint32 oldSize = infos.Size();
	for ( Uint32 i = 0; i < m_allTopics.Size(); ++i )
	{		
		m_allTopics[ i ]->Debug_GatherThreadInfos( infos );
	}

	for ( Uint32 i = oldSize; i < infos.Size(); ++i )
	{
		infos[ i ].m_instance = this;
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CInteractiveDialogInstance::OnInterlocutorDetached( CIDInterlocutorComponent* interlocutor )
{
	// TODO: add some more reasonable handling of such situation here
	if ( GetPlayState() == DIALOG_Finished || GetPlayState() == DIALOG_Error )
	{
		return;	
	}

	// if topic was running, interrupt it
	if ( m_activeTopic )
	{
		m_activeTopic->OnInterrupt();

		// Communicate to the HUD
		if ( m_focusedOnHud )
		{
			for ( Uint32 i = 0; i < m_huds.Size(); ++i )
			{
				m_huds[ i ]->OnInterrupted();
			}
		}

		m_activeTopic = nullptr;
	}

	// remove interlocutor from the map
	m_knownInterlocutors.EraseByValue( interlocutor );

	RED_LOG( CNAME( Dialog ), TXT("Instance %ld of dialog '%ls' ERROR: interlocutor '%ls' just detached from world!"), m_instanceID, m_dialog.GetPath().AsChar(), interlocutor->GetFriendlyName().AsChar() ); 
	SetPlayState( DIALOG_Error );	
}
