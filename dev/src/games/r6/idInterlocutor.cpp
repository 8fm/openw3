/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "idInterlocutor.h"
#include "idSystem.h"
#include "eventRouterComponent.h"
#include "idResource.h"
#include "idGraphBlockText.h"
#include "idInstance.h"
#include "idLine.h"
#include "idConnector.h"
#include "idCondition.h"

#include "../../common/game/actorSpeech.h"

IMPLEMENT_ENGINE_CLASS( CIDInterlocutorComponent );

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
CIDInterlocutorComponent::CIDInterlocutorComponent()
	: m_voiceTag			( CName::NONE )
	, m_lineQueued			( NULL )
	, m_linePlaying			( NULL )
	, m_lineLastCompleted	( NULL )
	, m_displaysOnHUD		( false )
	, m_focusedDialog		( NULL )
	, m_talkingTo			( NULL )
{
}

CIDInterlocutorComponent::~CIDInterlocutorComponent()
{
	delete m_lineQueued;
	delete m_linePlaying;
	delete m_lineLastCompleted;
}

//---------------------------------------------------------------------------------------------------------------------------------
// Starts dialog when player looks at
//---------------------------------------------------------------------------------------------------------------------------------
void CIDInterlocutorComponent::OnLookAt()
{
	/* Nothing */

	// Dialogs are started now by the talk interaction, or by the quest block, or from the script
}

CIDInterlocutorComponent* CIDInterlocutorComponent::GetToWhomAmITalking()
{
	return m_talkingTo;				
};

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIDInterlocutorComponent::PlayQueuedLine()
{
	// ED TODO:
	// If the dialog exists
	//if( m_dialogInstances.Size() >= m_LineQueued->m_DialogInstance )
	{
		R6_ASSERT( nullptr == m_linePlaying );
		R6_ASSERT( m_lineQueued );

		m_linePlaying = m_lineQueued;
		m_linePlaying->SetActive( true );
		m_lineQueued = nullptr;

		CInteractiveDialogInstance*	dialogInstance = GCommonGame->GetSystem< CInteractiveDialogSystem > ()->GetDialogInstance( m_linePlaying->GetDialogId() );
		if ( dialogInstance )
		{
			// Set to whom is it talking to
			FindReceiver( dialogInstance, *m_linePlaying );

			// get the line data
			const SIDBaseLine& line = m_linePlaying->GetLine();

			// Show on HUD
			dialogInstance->PlayLine( this, line );

			// Do the lookat
			DoSimpleLookAt( m_talkingTo );

			// Interrupt the interlocutor
			if( m_talkingTo && m_linePlaying->ShouldInterruptReceiver() )
			{
				m_talkingTo->OnLineInterrupted();
			}
		}
	}
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIDInterlocutorComponent::CustomTick( Float timeDelta )
{
	// tick async loading of dialogs
	for ( Int32 i = m_scriptDialogStartRequests.SizeInt() - 1; i >= 0; --i )
	{
		GCommonGame->GetSystem< CInteractiveDialogSystem > ()->RequestDialog( *m_scriptDialogStartRequests[ i ] );
		if ( m_scriptDialogStartRequests[ i ]->m_state != DIALOG_Loading )
		{
			delete m_scriptDialogStartRequests[ i ];
			m_scriptDialogStartRequests.RemoveAt( i );
		}
	}

	if ( m_linePlaying )
	{
		m_linePlaying->Update( timeDelta );
		CheckEndSayingLine( timeDelta );
	}
	// At the moment, we will stop updating if the actor is not speaking
	else
	{
		OnEndedTalking();
	}
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIDInterlocutorComponent::OnLineInterrupted( )
{
	const static Float	TIME_TO_INTERRUPT_LINE	= 0.3f;
	if( m_linePlaying )
	{
		m_linePlaying->SetTimeToDie( TIME_TO_INTERRUPT_LINE );
	}
	if( m_lineQueued && m_lineQueued->IsActive() )
	{
		m_lineQueued->SetActive( false );
	}
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIDInterlocutorComponent::CheckEndSayingLine( Float timeDelta )
{
	// Skip
	const static Float DIALOG_MIN_TIME_TO_SKIP_LINE	= .2f;
	if(		GCommonGame->GetInputManager()->GetActionValue( CNAME( GI_Crouch ) )  > 0.9f 
		&&	m_linePlaying->GetProgress() * m_linePlaying->GetDuration() > DIALOG_MIN_TIME_TO_SKIP_LINE ) 
	{
		EndedSayingLine();
	}

	// Attempt interrupt
	else if ( m_lineQueued != nullptr && m_lineQueued->IsActive() )
	{
		// ED TODO: Check the interruption of a playing line
		//if ( m_linePlaying->GetProgress() >= 0.5f )
		{
			EndedSayingLine();
		}
	}

	// Check line end
	else if ( m_linePlaying->GetProgress() >= 1.f || false == m_linePlaying->IsActive() )
	{
		EndedSayingLine();
	}

	// Check line timed death
	else if( m_linePlaying->UpdateNeedsToDie( timeDelta ) )
	{
		EndedSayingLine();
	}
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIDInterlocutorComponent::EndedSayingLine()
{
	if ( m_linePlaying )
	{
		// Get the dialog instance
		CInteractiveDialogInstance*	dialogInstance = GCommonGame->GetSystem< CInteractiveDialogSystem > ()->GetDialogInstance( m_linePlaying->GetDialogId() );
		if ( dialogInstance )
		{
			// Stop displaying on HUD
			dialogInstance->EndLine( this, m_linePlaying->GetLine() ); 
			
			// Stop the sound
			m_linePlaying->StopPlaying();
		}

		// Mark as inactive
		m_linePlaying->SetActive( false );

		// swap
		delete m_lineLastCompleted;
		m_lineLastCompleted	= m_linePlaying;
		m_linePlaying = nullptr;
	}

	if ( m_lineQueued && m_lineQueued->IsActive() )
	{
		PlayQueuedLine();
	}
	else
	{
		OnEndedTalking();
	}
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
EIDLineState CIDInterlocutorComponent::GetLineStatus( const SIDBaseLine& line, Uint32 dialogId )
{
	if ( m_lineQueued && m_lineQueued->LineMatches( line, dialogId ) )
	{
		return DILS_Queued;
	}
	else if ( m_linePlaying && m_linePlaying->LineMatches( line, dialogId ) )
	{
		if ( m_linePlaying->IsActive() )
		{
			return DILS_Playing;
		}
		else
		{
			return DILS_Completed;
		}
	}
	else if ( m_lineLastCompleted && m_lineLastCompleted->LineMatches( line, dialogId ) )
	{
		return DILS_Completed;
	}

	return DILS_CancelledOrNotPlayed;
}


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIDInterlocutorComponent::SetAttentionToDialog( CInteractiveDialogInstance* dialogInstance )	
{ 
	m_focusedDialog	= dialogInstance;		
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
CInteractiveDialogInstance* CIDInterlocutorComponent::GetDialogWithAttention( ) const
{ 
	if ( IsInDialog( ) )
	{
		return m_focusedDialog;					
	}
	//EIDPlayState state	= m_focusedDialog->GetPlayState(); // TODO: Remove this line
	return nullptr;
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
Bool CIDInterlocutorComponent::IsInDialog( ) const
{
	return m_focusedDialog && m_focusedDialog->IsPlaying(); 
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
Bool CIDInterlocutorComponent::IsSpeakingWith( CIDInterlocutorComponent* other ) const
{
	CInteractiveDialogInstance* dialog = GetDialogWithAttention();
	if ( nullptr == dialog )
	{
		return false;
	}

	return dialog->HasInterlocutor( other );
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
Bool CIDInterlocutorComponent::HasSomethingToTalkWith( CIDInterlocutorComponent* other ) const
{
	CInteractiveDialogInstance* dialog = GetDialogWithAttention();
	if ( nullptr == dialog )
	{
		return false;
	}

	if( dialog->HasInterlocutor( other ) )
	{
		if( dialog->GetHighestTopicPriority() )
		{
			return true;
		}
	}

	return false;
}


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIDInterlocutorComponent::RaiseAIEvent( SAIEventData* data )
{
	CEntity* entityParent =	GetEntity();
	if ( nullptr == entityParent )
	{
		return;
	}

	CActor*	actor =	Cast< CActor > ( entityParent );
	if ( nullptr == actor )
	{
		return;
	}

	data->m_eventAndParam->CallEventOnActor( actor );
	//actor->SignalGameplayEvent( data->m_EventName, data->m_AdditionalData, data->m_AdditionalDataType );
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIDInterlocutorComponent::RaiseBehaviorEvent( SAnimationEventData* data )
{
	CEntity* entityParent =	GetEntity();
	if ( nullptr == entityParent )
	{
		return;
	}

	if ( data->m_Force )
	{
		if ( data->m_ToAll )
		{
			entityParent->RaiseBehaviorForceEventForAll( data->m_EventName );
		}
		else
		{
			entityParent->RaiseBehaviorForceEvent( data->m_EventName );
		}
	}
	else
	{
		if( data->m_ToAll )
		{
			entityParent->RaiseBehaviorEventForAll( data->m_EventName );
		}
		else
		{
			entityParent->RaiseBehaviorEvent( data->m_EventName );
		}
	}
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
Bool CIDInterlocutorComponent::MatchesDefinition( const SIDInterlocutorDefinition& def ) const
{
	return m_voiceTag == def.m_voiceTagToMatch && ( def.m_tagToMatch == CName::NONE || m_tags.HasTag( def.m_tagToMatch ) );
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
const CGameplayEntity* CIDInterlocutorComponent::GetOwnerEntity() const
{
	return Cast< CGameplayEntity > ( GetEntity() );
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIDInterlocutorComponent::OnInteractiveDialogStarted( Uint32 instanceID )
{
	R6_ASSERT( !m_dialogInstances.Exist( instanceID ) );
	m_dialogInstances.PushBack( instanceID );
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIDInterlocutorComponent::OnInteractiveDialogEnded( Uint32 instanceID )
{
	R6_ASSERT( m_dialogInstances.Exist( instanceID ) );
	m_dialogInstances.Erase( Find( m_dialogInstances.Begin(), m_dialogInstances.End(), instanceID ) );

	delete m_lineLastCompleted;
	delete m_linePlaying;
	delete m_lineQueued;
	m_lineLastCompleted	= NULL;
	m_linePlaying		= NULL;
	m_lineQueued		= NULL;
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIDInterlocutorComponent::OnInterlocutorDetached()
{
	CInteractiveDialogSystem* system = GCommonGame->GetSystem< CInteractiveDialogSystem > ();
	if ( system )
	{
		system->OnInterlocutorDetached( this );
	}

	delete m_lineLastCompleted;
	delete m_linePlaying;
	delete m_lineQueued;
	m_lineLastCompleted	= NULL;
	m_linePlaying		= NULL;
	m_lineQueued		= NULL;
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDInterlocutorComponent::OnDetached( CWorld* world )
{
	OnInterlocutorDetached();
	TBaseClass::OnDetached( world );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
String CIDInterlocutorComponent::GetLocalizedName() const
{
	// try getting it from the entity
	const CEntity* entity = GetEntity();
	if ( entity )
	{
		const CGameplayEntity* gmplEnt = Cast< const CGameplayEntity > ( entity );
		if ( gmplEnt )
		{
			return gmplEnt->GetDisplayName(); 
		}
	}

	// if failed, return Voice tag
	if ( m_voiceTag )
	{
		return m_voiceTag.AsString();
	}

	// if this also faile, return something generic
	return TXT("[UNKNOWN]");
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIDInterlocutorComponent::FindReceiver( CInteractiveDialogInstance* dialog, const SIDLineInstance& line )
{
	const THashMap< CName, CIDInterlocutorComponent* >	interlocutors	= dialog->GetInterlocutorsMap();
	Uint32											size			= interlocutors.Size();

	// Default
	m_talkingTo	= NULL;

	// Try to get the receiver
	if ( line.GetReceiver() && line.GetReceiver() != CNAME( Default ) )
	{
		m_talkingTo	= dialog->GetInterlocutor( line.GetReceiver() );
	}

	// Or try to find an automated one
	else
	{
		// Dialog between 2 people, talk to the other interlocutor
		if ( size == 2 )
		{
			for( THashMap< CName, CIDInterlocutorComponent* >::const_iterator it = interlocutors.Begin(); it != interlocutors.End(); ++it )
			if ( it->m_second != this )
			{
				m_talkingTo	= it->m_second;
			}
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDInterlocutorComponent::funcOnLookAt( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	OnLookAt();
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDInterlocutorComponent::funcGetEntityWithAttention( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetEntityWithAttention() );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDInterlocutorComponent::funcSetEntityWithAttention( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( CEntity*, entity, NULL );
	FINISH_PARAMETERS;

	SetAttentionToEntity( entity );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDInterlocutorComponent::funcGetIsFocused( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( IsInDialog() );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDInterlocutorComponent::funcIsInDialog( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( IsInDialog() );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDInterlocutorComponent::funcIsSpeakingWith( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CIDInterlocutorComponent>, interlocutor, NULL );
	FINISH_PARAMETERS;

	RETURN_BOOL( IsSpeakingWith( interlocutor.Get() ) );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDInterlocutorComponent::funcHasSomethingToTalkWith( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CIDInterlocutorComponent>, interlocutor, NULL );
	FINISH_PARAMETERS;

	RETURN_BOOL( HasSomethingToTalkWith( interlocutor.Get() ) );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDInterlocutorComponent::funcRequestDialog( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CIDInterlocutorComponent >, initiator, NULL );
	GET_PARAMETER( THandle< CInteractiveDialog >, dialog, NULL ); // TODO: should be a soft handle, unsupported (?) right now 
	FINISH_PARAMETERS;

	SDialogStartRequest *info = new SDialogStartRequest( this, initiator.Get(), dialog.Get() ? dialog.Get() : m_defaultDialog );
	GCommonGame->GetSystem< CInteractiveDialogSystem > ()->RequestDialog( *info );
	if ( info->m_state == DIALOG_Error )
	{
		delete info;
	}
	else
	{
		m_scriptDialogStartRequests.PushBack( info );
		StartTicking();
	}
}

/*
//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDInterlocutorComponent::funcGetDialogWithAttention( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_HANDLE( CInteractiveDialogInstance, GetDialogWithAttention() );
}*/

//---------------------------------------------------------------------------------------------------------------------------------
// ISceneActorInterface
//---------------------------------------------------------------------------------------------------------------------------------
Bool CIDInterlocutorComponent::HasMimicAnimation( const CName& slot /*= CNAME( MIMIC_SLOT ) */ ) const
{
	const CActor *actor = Cast< const CActor > ( GetEntity() );
	if ( actor )
	{
		return actor->HasMimicAnimation( slot );
	}
	return false;
}

Bool CIDInterlocutorComponent::PlayLipsyncAnimation( CSkeletalAnimationSetEntry* anim, Float offset )
{
	CActor *actor = Cast< CActor > ( GetEntity() );
	if ( actor )
	{
		return actor->PlayLipsyncAnimation( anim, offset );
	}
	return false;
}

Bool CIDInterlocutorComponent::PlayMimicAnimation( const CName& anim, const CName& slot /*= CNAME( MIMIC_SLOT )*/, Float blentTime /*= 0.0f*/, Float offset /*= 0.0f */ )
{
	CActor *actor = Cast< CActor > ( GetEntity() );
	if ( actor )
	{
		return actor->PlayMimicAnimation( anim, slot, blentTime, offset );
	}
	return false;
}

Bool CIDInterlocutorComponent::StopMimicAnimation( const CName& slot /*= CNAME( MIMIC_SLOT ) */ )
{
	CActor *actor = Cast< CActor > ( GetEntity() );
	if ( actor )
	{
		return actor->StopMimicAnimation( slot );
	}
	return false;
}

Bool CIDInterlocutorComponent::HasSceneMimic() const
{
	const CActor *actor = Cast< const CActor > ( GetEntity() );
	if ( actor )
	{
		return actor->HasSceneMimic();
	}
	return false;
}

Bool CIDInterlocutorComponent::SceneMimicOn()
{
	CActor *actor = Cast< CActor > ( GetEntity() );
	if ( actor )
	{
		return actor->SceneMimicOn();
	}
	return false;
}

void CIDInterlocutorComponent::SceneMimicOff()
{
	CActor *actor = Cast< CActor > ( GetEntity() );
	if ( actor )
	{
		actor->SceneMimicOff();
	}
}

CEntity* CIDInterlocutorComponent::GetSceneParentEntity()
{
	return GetEntity();
}

Vector CIDInterlocutorComponent::GetSceneHeadPosition() const
{
	const CActor *actor = Cast< const CActor > ( GetEntity() );
	if ( actor )
	{
		return actor->GetSceneHeadPosition();
	}
	return GetWorldPosition();
}

Int32 CIDInterlocutorComponent::GetSceneHeadBone() const
{
	const CActor *actor = Cast< const CActor > ( GetEntity() );
	if ( actor )
	{
		return actor->GetHeadBone();
	}
	return -1;
}

Bool CIDInterlocutorComponent::WasSceneActorVisibleLastFrame() const
{
	const CActor *actor = Cast< const CActor > ( GetEntity() );
	if ( actor )
	{
		return actor->WasSceneActorVisibleLastFrame();
	}
	return true;
}

Vector CIDInterlocutorComponent::GetBarPosition() const
{
	const CActor *actor = Cast< const CActor > ( GetEntity() );
	if ( actor )
	{
		return actor->GetBarPosition();
	}
	return GetWorldPosition();
}

Vector CIDInterlocutorComponent::GetAimPosition() const
{
	const CActor *actor = Cast< const CActor > ( GetEntity() );
	if ( actor )
	{
		return actor->GetAimPosition();
	}
	return GetWorldPosition();
}

CName CIDInterlocutorComponent::GetSceneActorVoiceTag() const
{
	return GetVoiceTag();
}

void CIDInterlocutorComponent::OnPropertyPostChange( IProperty* prop )
{
	if ( prop->GetName() == CNAME( connectors ) || prop->GetName() == CNAME( voiceTag ) )
	{
		ValidateConnectors();
	}

	TBaseClass::OnPropertyPostChange( prop );
}

void CIDInterlocutorComponent::ValidateConnectors()
{
	// ! Editor ONLY code !

#ifndef NO_EDITOR
#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) 
	if ( !m_voiceTag )
	{
		MessageBox( NULL, TXT("Define a voicetag first! Removing all connectors due to lack of a voicetag!"), TXT("Error"), MB_OK | MB_ICONERROR );
		m_connectors.Clear();
		return;
	}

	for ( Int32 i = m_connectors.Size() - 1; i >=0; --i )
	{
		if ( m_connectors[ i ].m_pack.IsEmpty() )
		{
			continue;
		}

		const Bool wasLoaded = m_connectors[ i ].m_pack.IsLoaded();
		if ( !wasLoaded )
		{
			m_connectors[ i ].m_pack.Load();
		}

		CIDConnectorPack* pack = m_connectors[ i ].m_pack.Get();
		if ( nullptr == pack )
		{
			RED_LOG( Dialog, TXT("Removing invalid connector pack: %s"), m_connectors[ i ].m_pack.GetPath().AsChar() );
			m_connectors[ i ].m_pack = NULL;
			continue;
		}

		if ( false == pack->MatchesVoiceTag( m_voiceTag ) )
		{
			RED_LOG( Dialog, TXT("Connector pack: %s does not define a voiceTag %s"), m_connectors[ i ].m_pack.GetPath().AsChar(), m_voiceTag.AsString().AsChar() );
			MessageBox( NULL, String::Printf( TXT("Connector pack: %s does not define a voiceTag %s"), m_connectors[ i ].m_pack.GetPath().AsChar(), m_voiceTag.AsString().AsChar() ).AsChar(), TXT("Error"), MB_OK | MB_ICONERROR );
			m_connectors[ i ].m_pack.Release();
			m_connectors[ i ].m_pack = NULL;
			continue;
		}

		if ( !wasLoaded )
		{
			m_connectors[ i ].m_pack.Release();
		}
	}
#endif
#endif
}

void CIDInterlocutorComponent::RequestConnector( SIDConnectorRequest& request )
{
	// If there was an error, or the connector is ready to use, do nothing more
	if ( request.m_state == DIALOG_Error || request.m_state == DIALOG_Ready )
	{
		return;
	}

	// Is the resource specified?
	if ( request.m_resource.IsEmpty() )
	{
		// Collect all connectors matching the category and fullfilling the condition, then pick one randomly
		TDynArray< TSoftHandle< CIDConnectorPack > > matchingPacks;
		for ( Uint32 i = 0; i < m_connectors.Size(); ++i )
		{
			if ( m_connectors[ i ].m_category == request.m_category && ( m_connectors[ i ].m_condition == NULL || m_connectors[ i ].m_condition->IsFulfilled( request.m_dialogId ) ) )
			{
				matchingPacks.PushBack( m_connectors[ i ].m_pack );
			}
		}

		// Not found?
		if ( matchingPacks.Empty() )
		{
			for ( Uint32 i = 0; i < m_connectors.Size(); ++i )
			{
				// Try default or empty category
				if ( ( m_connectors[ i ].m_category == CName::NONE || m_connectors[ i ].m_category == CNAME( Default ) || m_connectors[ i ].m_category == CNAME( default ) )
					&& ( m_connectors[ i ].m_condition == NULL || m_connectors[ i ].m_condition->IsFulfilled( request.m_dialogId ) ) )
				{
					matchingPacks.PushBack( m_connectors[ i ].m_pack );
				}
			}
		}

		// Still not found?
		if ( matchingPacks.Empty() )
		{
			request.m_state = DIALOG_Error;
			return;
		}

		// Pick one pack randomly
		Uint32 idx = 0;
		if ( matchingPacks.Size() > 1 )
		{
			idx = ( GEngine->GetRandomNumberGenerator().Get< Uint32 >( matchingPacks.Size() ) );
		}
		request.m_resource = matchingPacks[ idx ];
		matchingPacks.Clear();
	}

	// Is the resource loaded?
	if ( false == request.m_resource.IsLoaded() )
	{
		BaseSoftHandle::EAsyncLoadingResult res = request.m_resource.GetAsync();
		if ( res == BaseSoftHandle::ALR_InProgress )
		{
			request.m_state = DIALOG_Loading;
			return;
		}
		else if ( res == BaseSoftHandle::ALR_Failed )
		{
			RED_LOG( Dialog, TXT("Loading of %s failed."), request.m_resource.GetPath().AsChar() );
			request.m_state = DIALOG_Error;
			return;
		}
		// Otherwise it's loaded now
	}

	CIDConnectorPack* pack = request.m_resource.Get();
	if ( nullptr == pack )
	{
		RED_LOG( Dialog, TXT("Loading of %s failed."), request.m_resource.GetPath().AsChar() );
		request.m_state = DIALOG_Error;
		return;
	}

	// Check the voice tag... should be fine because of the editor checks, but just in case...
	if ( false == pack->MatchesVoiceTag( m_voiceTag ) )
	{
		R6_ASSERT( false, TXT("WTF? Is the editor code ok? DEBUG ME.") );
		RED_LOG( Dialog, TXT("Connector pack %s doesn't match the voicetag of %s"), pack->GetFriendlyName().AsChar(), GetFriendlyName().AsChar() );
		request.m_state = DIALOG_Error;
		return;
	}
	  
	// Check if the line have been selected
	if ( request.m_index < 0 )
	{
		// Pick one line randomly
		Uint32 numLines = pack->GetNumLines();
		if ( 0 == numLines )
		{
			RED_LOG( Dialog, TXT("Connector pack %s doesn't contain any line"), pack->GetFriendlyName().AsChar() );
			request.m_state = DIALOG_Error;
			return;
		}
		if ( numLines == 1 )
		{
			request.m_index = 0;
		}
		else
		{
			request.m_index = ( GEngine->GetRandomNumberGenerator().Get< Int32 >( numLines ) );
		}
	}

	// Get the text
	const LocalizedString& text = pack->GetText( Uint32( request.m_index ) );
	R6_ASSERT( text.GetIndex() < Uint32( -1 ) );

	// Ok, see if we already have an instance of this line
	for ( Uint32 i = 0; i < m_connectorInstances.Size(); ++i )
	{
		SConnectorInstance& inst = ( *m_connectorInstances[ i ] );
		if ( inst.m_dialogId == request.m_dialogId && inst.m_line.m_text == text )
		{
			// Add ref and return
			++inst.m_refCount;
			request.m_result = &inst.m_line;
			request.m_state = DIALOG_Ready;
			return;
		}
	}

	// Find packIndex
	Uint32 packIndex;
	for ( packIndex = 0; packIndex < m_connectors.Size(); ++packIndex )
	{
		if ( m_connectors[ packIndex ].m_pack == request.m_resource )
		{
			break;
		}
	}
	R6_ASSERT( packIndex < m_connectors.Size() );

	// Currently no instance - add one
	SConnectorInstance *inst = new SConnectorInstance( pack->GetLine( Uint32( request.m_index ) ), packIndex, request.m_dialogId );

	m_connectorInstances.PushBack( inst ); 
	request.m_result = &inst->m_line;
	request.m_state = DIALOG_Ready;
}

void CIDInterlocutorComponent::ReleaseConnector( const SIDConnectorLine* connector )
{
	for ( Uint32 i = 0; i < m_connectorInstances.Size(); ++i )
	{
		if ( &m_connectorInstances[ i ]->m_line == connector )
		{
			if ( --m_connectorInstances[ i ]->m_refCount <= 0 )
			{
				delete m_connectorInstances[ i ];
				m_connectorInstances.RemoveAt( i );

				UnloadUnneededConnectorPacks();
			}
			return;
		}
	}

	R6_ASSERT( false, TXT("Releasing non-existant connector. Please DEBUG!") );
}

void CIDInterlocutorComponent::UnloadUnneededConnectorPacks()
{
	for ( Uint32 i = 0; i < m_connectors.Size(); ++i )
	{
		// Already unloaded?
		if ( false == m_connectors[ i ].m_pack.IsLoaded() )
		{
			continue;
		}

		// Count references to this pack
		Int32 refCount( 0 );
		for ( Uint32 k = 0; k < m_connectorInstances.Size(); ++k )
		{
			if ( m_connectorInstances[ k ]->m_packIndex == i )
			{
				refCount += max( 0, m_connectorInstances[ k ]->m_refCount );
			}
		}

		if ( 0 == refCount )
		{
			// This pack is not used, unload it
			m_connectors[ i ].m_pack.Release();
		}
	}
}

void CIDInterlocutorComponent::DoSimpleLookAt( const CIDInterlocutorComponent* target )
{
	if ( nullptr == target )
	{
		EndLookingAt();
		return;
	}

	CActor* thisActor = Cast< CActor > ( GetEntity() );
	if ( nullptr == thisActor )
	{
		RED_LOG( Dialog, TXT( "'%ls' (entity class '%ls') cannot look at '%ls'." ), GetFriendlyName().AsChar(), GetEntity()->GetClass()->GetName().AsString().AsChar(), target->GetFriendlyName().AsChar() );
		return;
	}

	Int32 bone = -1;
	const CAnimatedComponent* ac =  target->GetEntity()->GetRootAnimatedComponent();
	if ( ac )
	{
		const CActor *targetActor = Cast< const CActor > ( target->GetEntity() ); 
		if ( targetActor )
		{
			bone = targetActor->GetHeadBone();
		}
	}
	
	SLookAtDialogBoneInfo bi;
	SLookAtDialogDynamicInfo di;
	IDialogLookAtData& data = bi;
	SLookAtInfo& info = bi;

	if ( bone >= 0 )
	{
		bi.m_boneIndex = bone;
		bi.m_targetOwner = ac;
	}
	else
	{
		di.m_target = target;
		data = di;
		info = di;
	}

	// this will be exposed in timeline event, for now - hardcoded
	data.m_speed = 1.f;
	data.m_level = LL_Body;
	data.m_instant = false;
	data.m_autoLimitDeact = false;
	data.m_range = 180.f;
	data.m_speedOverride = 0.f;
	data.m_eyesLookAtConvergenceWeight = 0.f;
	data.m_eyesLookAtIsAdditive = true;
	data.m_eyesLookAtDampScale = 1.f;
	info.m_desc = TXT("Interactive dialog simple lookat");

	Bool ret = false;
	if ( bone >= 0 )
	{
		ret = thisActor->EnableLookAt( bi );
	}
	else
	{
		ret = thisActor->EnableLookAt( di );
	}
	R6_ASSERT( ret );
}

void CIDInterlocutorComponent::EndLookingAt()
{
	CActor* thisActor = Cast< CActor > ( GetEntity() );
	if ( thisActor )
	{
		thisActor->DisableDialogsLookAts( 0.f );
	}
}

void CIDInterlocutorComponent::OnEndedTalking()
{
	EndLookingAt();

	if ( m_scriptDialogStartRequests.Empty() )
	{
/*
		CActor *actor = Cast< CActor > ( GetEntity() );
		if ( actor )
		{
			actor->SceneMimicOff();
		}*/

		StopTicking();
	}
}

// this is needed to prevent GC from deleting lipsync animation from actorspeech
void CIDInterlocutorComponent::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( file.IsGarbageCollector() )
	{
		if ( nullptr != m_lineLastCompleted )
		{
			file << *m_lineLastCompleted;
		}

		if ( nullptr != m_linePlaying )
		{
			file << *m_linePlaying;
		}

		if ( nullptr != m_lineQueued )
		{
			file << *m_lineQueued;
		}
	}
}

