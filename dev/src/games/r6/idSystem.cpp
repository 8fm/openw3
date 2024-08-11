/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "idThreadActivator.h"
#include "idResource.h"
#include "idInstance.h"
#include "idSystem.h"
#include "r6DialogDisplayManager.h"
#include "idInterlocutor.h"

IMPLEMENT_RTTI_ENUM( EIDPlayState )
IMPLEMENT_ENGINE_CLASS( SRunningDialog )
IMPLEMENT_ENGINE_CLASS( CInteractiveDialogSystem )

CInteractiveDialogSystem::CInteractiveDialogSystem()
	: m_currentInstanceID			( 0 )
	, m_dialogFocusedByHUD			( NULL )
	, m_cooldownFocusAutoRecalcMax	( 1.5f )
	, m_cooldownFocusAutoRecalcCur	( 0.0f )
{
}

CInteractiveDialogSystem::~CInteractiveDialogSystem()
{
}

void CInteractiveDialogSystem::Tick( Float timeDelta ) 
{
	// Tick all instances
	for ( Uint32 i = 0; i < m_runningDialogs.Size(); ++i )
	{
		CInteractiveDialogInstance* instance = m_runningDialogs[ i ].m_instance;
		EIDPlayState state = instance->GetPlayState();

		// don't tick the errored instances, but leave them on the list for debugging purposes
		if ( state == DIALOG_Error )
		{
			continue;
		}

		R6_ASSERT( state == DIALOG_Playing || state == DIALOG_Ready );

		// tick the instance
		instance->Tick( timeDelta );

		// Finished?
		state = instance->GetPlayState();
		if ( state == DIALOG_Finished )
		{
			m_finishedDialogs.PushBack( i );
			if ( instance == m_dialogFocusedByHUD )
			{
				m_dialogFocusedByHUD->OnLostHUDFocus();
				m_dialogFocusedByHUD = nullptr;
			}
		}
	}
										 
	// Delete finished instances
	for ( Int32 i = m_finishedDialogs.SizeInt() - 1; i >= 0 ; --i )
	{
		Uint32 index = m_finishedDialogs[ i ];
		CInteractiveDialogInstance* instance = m_runningDialogs[ index ].m_instance;
		R6_ASSERT( instance->GetPlayState() == DIALOG_Finished );
		m_runningDialogs.RemoveAt( index );
		delete instance;
	}
	m_finishedDialogs.ClearFast();

	// Check focus change
	UpdateFocusChangeOrEnd( timeDelta );
}

void CInteractiveDialogSystem::UpdateFocusChangeOrEnd( Float timeDelta )
{
	// Check if it is time to recalc the focus
	Bool needsRecalc( false );

	if( m_dialogFocusedByHUD && m_dialogFocusedByHUD->IsPlaying() )
	{
		m_cooldownFocusAutoRecalcCur	= 0.0f;
	}
	else
	{
		if( m_cooldownFocusAutoRecalcCur < m_cooldownFocusAutoRecalcMax )
		{
			m_cooldownFocusAutoRecalcCur	+= timeDelta;
			if( m_cooldownFocusAutoRecalcCur >= m_cooldownFocusAutoRecalcMax )
			{
				needsRecalc	= true;
			}
		}
	}


	// Recalc focus
	if( needsRecalc )
	{
		RequestFocusRecalc( );


		// No dialog to display? end the dialog
		if( m_dialogFocusedByHUD == NULL )
		{
			CR6DialogDisplay*	display	= GCommonGame->GetSystem< CR6DialogDisplay >();
			display->OnDialogEnded();
		}
	}
}

void CInteractiveDialogSystem::RequestDialog( SDialogStartRequest& info )
{
	// if the dialog is already started, or there was an error - do nothing
	if ( info.m_state != DIALOG_Loading )
	{
		return;
	}

	// maybe it's started already?
	CInteractiveDialogInstance* instance = FindRunningDialogByResourcePath( info.m_resource.GetPath() );
	if ( instance && instance->GetPlayState() != DIALOG_Error && instance->GetPlayState() != DIALOG_Finished )
	{
		info.m_state = instance->GetPlayState();
		info.m_startedInstanceID = instance->GetInstanceID();
		return;
	}

	if ( info.m_resource.IsEmpty() )
	{
		R6_ASSERT( false, TXT("Resource not specified, there should be a check for this inside a caller") );
		info.m_state = DIALOG_Error;
		return;
	}

	if ( false == info.m_resource.IsLoaded() )
	{
		// Load it
		BaseSoftHandle::EAsyncLoadingResult res = info.m_resource.GetAsync();
		if ( res == BaseSoftHandle::ALR_InProgress )
		{
			info.m_state = DIALOG_Loading;
			return;
		}
		else if ( res == BaseSoftHandle::ALR_Failed )
		{
			RED_LOG( Dialog, TXT("Loading of %s failed."), info.m_resource.GetPath().AsChar() );
			info.m_state = DIALOG_Error;
			return;
		}
		// Otherwise it's loaded now
	}

	// try to start the instance
	instance = RunDialogInternal( info ); 
	if ( NULL == instance )
	{
		info.m_state = DIALOG_Error;
		return;
	}

	// Check if the dialog can play
	if ( !instance->GetWantsToPlay() )
	{
		info.m_state = DIALOG_Error;
		return;
	}

	// Set the attention of the interlocutors to this dialog
	instance->GainFocusOfInterlocutors();

	// Check if it takes control over the HUD
	instance->SetWantsFocus( info.m_owner->GetWantsToDisplayOnHUD() || info.m_initiatior->GetWantsToDisplayOnHUD() );
	if ( instance->GetWantsFocus() )
	{
		SetHUDFocusTo( instance );
	}

	info.m_state = instance->GetPlayState();
}

void CInteractiveDialogSystem::RequestFocus( CInteractiveDialogInstance* dialogInstance )
{
	// If the dialog is not playing, we do nothing, if the dialog is NULL, we still want to remove focus on the old dialog
	if ( dialogInstance && !IsDialogInstanceRunning( dialogInstance ) )
	{
		return;
	}

	// Set the attention of the interlocutors to this dialog
	if( dialogInstance )
	{
		dialogInstance->GainFocusOfInterlocutors();
	}

	SetHUDFocusTo( dialogInstance );
}

void CInteractiveDialogSystem::RequestFocusRecalc( )
{
	RequestFocus( RecalcHighestPriorityDialog( NULL ) );
}

void CInteractiveDialogSystem::RequestFocusEnd( const CInteractiveDialogInstance*	dialog )
{
	CInteractiveDialogInstance*	dialogWithHighestPriority	= RecalcHighestPriorityDialog( dialog );

	RequestFocus( dialogWithHighestPriority );
}

CInteractiveDialogInstance* CInteractiveDialogSystem::RecalcHighestPriorityDialog( const CInteractiveDialogInstance*	dialogToIgnore )
{
	CInteractiveDialogInstance*	dialogWithHighestPriority	= NULL;
	EIDPriority					highestPriority				= IDP_Invalid;

	for ( Uint32 i = 0; i < m_runningDialogs.Size(); ++i )
	{
		CInteractiveDialogInstance*	currentDialog	= m_runningDialogs[ i ].m_instance;
		if( dialogToIgnore == currentDialog )
		{
			continue;
		}

		if( !currentDialog->GetWantsToPlay() )
		{
			continue;
		}

		EIDPriority	currentPriority	= currentDialog->GetHighestTopicPriority();
		if ( currentPriority != IDP_Invalid && currentPriority < highestPriority )
		{
			highestPriority				= currentPriority;
			dialogWithHighestPriority	= currentDialog;
		}
	}

	return dialogWithHighestPriority;
}

void CInteractiveDialogSystem::SetHUDFocusTo( CInteractiveDialogInstance*	dialogInstance )
{
	if( m_dialogFocusedByHUD == dialogInstance )
	{
		return;
	}

	if ( m_dialogFocusedByHUD )
	{
		m_dialogFocusedByHUD->OnLostHUDFocus();
	}

	if( dialogInstance )
	{
		dialogInstance->OnGainedHUDFocus();
	}

	m_dialogFocusedByHUD = dialogInstance;
}

CInteractiveDialogInstance* CInteractiveDialogSystem::RunDialogInternal( SDialogStartRequest& info )
{
	SRunningDialog rd;
	rd.m_instanceID = info.m_startedInstanceID = ++m_currentInstanceID;
	rd.m_instance = new CInteractiveDialogInstance( info );
	rd.m_resource = info.m_resource.Get();
	
	if ( rd.m_instance->GetPlayState() != DIALOG_Error )
	{
		for ( Uint32 i = 0; i < m_huds.Size(); ++i )
		{
			const CGameplayEntity* ent = info.m_owner->GetOwnerEntity();
			if ( !ent || !ent->IsAttached() )
			{
				ERR_R6( TXT("Dialog interlocutors should be placed in CGameplayEntity! ALWAYS!\n%s"), info.m_owner->GetFriendlyName().AsChar() );
				delete rd.m_instance;
				return NULL;
			}

			if ( m_huds[ i ].m_first == ent->GetLayer()->GetWorld() )
			{
				rd.m_instance->AttachHud( m_huds[ i ].m_second );
			}
		}

		m_runningDialogs.PushBack( rd );
	}
	else
	{
		delete rd.m_instance;
		return NULL;
	}

	return rd.m_instance;
}

CInteractiveDialogInstance* CInteractiveDialogSystem::FindRunningDialogByResourcePath( const String& path )	const
{
	for ( Uint32 i = 0; i < m_runningDialogs.Size(); ++i )
	{
		if ( m_runningDialogs[ i ].m_instance->GetResourceHandle().GetPath().EqualsNC( path ) )
		{
			return m_runningDialogs[ i ].m_instance;
		}
	}
	return NULL;
}

Bool CInteractiveDialogSystem::IsDialogInstanceRunning( const CInteractiveDialogInstance* dialog )	const
{
	for ( Uint32 i = 0; i < m_runningDialogs.Size(); ++i )
	{
		if ( m_runningDialogs[ i ].m_instance == dialog )
		{
			return true;
		}
	}
	return false;
}

CInteractiveDialogInstance* CInteractiveDialogSystem::FindRunningDialogByInstanceID( Uint32 instanceID ) const
{
	Uint32 index = InstanceIdToIndex( instanceID );
	return index < m_runningDialogs.Size() ? m_runningDialogs[ index ].m_instance : NULL; 
}

Uint32 CInteractiveDialogSystem::InstanceIdToIndex( Uint32 instanceID ) const
{
	for ( Uint32 i = 0; i < m_runningDialogs.Size(); ++i )
	{
		if ( m_runningDialogs[ i ].m_instanceID == instanceID )
		{
			return i;
		}
	}
	return ( Uint32 ) -1; // invalid index
}

void CInteractiveDialogSystem::OnInterlocutorDetached( CIDInterlocutorComponent* interlocutor )
{
	Uint32 numDialogs( 0 );
	for ( Uint32 i = 0; i < m_runningDialogs.Size(); ++i )
	{
		if ( nullptr != m_runningDialogs[ i ].m_instance )
		{
			if ( m_runningDialogs[ i ].m_instance->HasInterlocutor( interlocutor ) )
			{
				++numDialogs;
				m_runningDialogs[ i ].m_instance->OnInterlocutorDetached( interlocutor );
			}
		}
	}

	RED_LOG( CNAME( Dialog ), TXT("Interlocutor %s detached from dialog system. Took part in %ld running dialogs."), interlocutor->GetFriendlyName().AsChar(), numDialogs );
}

void CInteractiveDialogSystem::AttachHud( CWorld* world, IDialogHud* hud )
{
	TPair< CWorld*, IDialogHud* > pair( world, hud );
	R6_ASSERT( !m_huds.Exist( pair ) );
	m_huds.PushBack( pair );

	// TODO: update instances
}

void CInteractiveDialogSystem::DetachHud( CWorld* world, IDialogHud* hud )
{
	TPair< CWorld*, IDialogHud* > pair( world, hud );
	R6_ASSERT( m_huds.Exist( pair ) );
	m_huds.Remove( pair );

	// TODO: update instances
}

CIDInterlocutorComponent* CInteractiveDialogSystem::FindActorForDialog( CInteractiveDialogInstance* dialog, const SIDInterlocutorDefinition& actorDef ) const
{
	struct InterlocutorFunctor : public CGameplayStorage::DefaultFunctor
	{
		TDynArray< CIDInterlocutorComponent* >& m_out;
		const SIDInterlocutorDefinition& m_actorDef;
		InterlocutorFunctor( TDynArray< CIDInterlocutorComponent* >& out, const SIDInterlocutorDefinition& actorDef ) : m_out( out ), m_actorDef( actorDef ) { }

		RED_INLINE Bool operator()( const TPointerWrapper< CGameplayEntity >& ptr )
		{
			for ( ComponentIterator< CIDInterlocutorComponent > it( ptr.Get() ); it; ++it )	
			{
				if ( ( *it )->MatchesDefinition( m_actorDef ) )
				{
					m_out.PushBack( *it );
				}
			}
			return true;
		}
	};

	if ( dialog->GetOwner()->MatchesDefinition( actorDef ) )
	{
		if ( false == dialog->HasInterlocutor( dialog->GetOwner() ) )
		{
			return dialog->GetOwner();
		}
	}

	if ( dialog->GetInitiator()->MatchesDefinition( actorDef ) )
	{
		if ( false == dialog->HasInterlocutor( dialog->GetInitiator() ) )
		{
			return dialog->GetInitiator();
		}
	}

	// ---------------------------------------------------------
	// < tempshit >
	// crappy, unoptimal and totally not shipable code
	// TODO: to be refactored later 

	const Float ACTOR_SEARCH_RADIUS = 1000.f; 

	TDynArray< TPointerWrapper< CGameplayEntity > > entities;
	TDynArray< CIDInterlocutorComponent* > interlocutors;
	InterlocutorFunctor func( interlocutors, actorDef );

	const CGameplayEntity* ent = dialog->GetOwner()->GetOwnerEntity();
	if ( ent == NULL )
	{
		RED_LOG( Dialog, TXT("Dialog interlocutors should be placed in CGameplayEntity! ALWAYS!\n%s"), dialog->GetOwner()->GetFriendlyName().AsChar() );
		return NULL;
	}

	GCommonGame->GetGameplayStorage()->TQuery( *ent, func, Box( Vector::ZERO_3D_POINT, ACTOR_SEARCH_RADIUS ), true, NULL, 0 );

	if ( interlocutors.Empty() )
	{
		return NULL;
	}

	for ( Uint32 i = 0; i < interlocutors.Size(); ++i )
	{
		if ( false == dialog->HasInterlocutor( interlocutors[ i ] ) )
		{
			return interlocutors[ i ];
		}
	}

	return NULL;

	// < /tempshit >
	// ---------------------------------------------------------
}

CIDInterlocutorComponent* CInteractiveDialogSystem::GetInterlocutorOnDialog( Uint32 dialogID, const CName& interlocutor ) const
{
	CInteractiveDialogInstance* dialogInstance	= FindRunningDialogByInstanceID( dialogID );
	return dialogInstance->GetInterlocutor( interlocutor );
}

CInteractiveDialogInstance* CInteractiveDialogSystem::GetDialogInstance( Uint32 dialogID ) const
{
	return FindRunningDialogByInstanceID( dialogID );
}

void CInteractiveDialogSystem::OnWorldStart( const CGameInfo& gameInfo )
{
	m_currentInstanceID	= 0;
	m_dialogFocusedByHUD = NULL;
	m_cooldownFocusAutoRecalcMax = 1.5f;
	m_cooldownFocusAutoRecalcCur = 0.0f;
}

void CInteractiveDialogSystem::OnWorldEnd( const CGameInfo& gameInfo )
{
	// End all playing dialogs
	for ( Uint32 i = 0; i < m_runningDialogs.Size(); ++i )
	{
		delete m_runningDialogs[ i ].m_instance;
	}
	m_runningDialogs.Clear();
}

void CInteractiveDialogSystem::OnChoiceSelected( EHudChoicePosition position )
{
	if ( m_dialogFocusedByHUD && m_dialogFocusedByHUD->GetPlayState() == DIALOG_Playing )
	{
		m_dialogFocusedByHUD->OnChoiceSelected( position );
	}
}

CIDInterlocutorComponent* CInteractiveDialogSystem::GetPlayerInterlocutor() const
{
	return GGame ? static_cast< CR6Game* >( GGame )->GetPlayerInterlocutorComponent() : nullptr;
}

void CInteractiveDialogSystem::Debug_GatherThreadInfos( TDynArray< Debug_SIDThreadInfo >& infos ) const
{
	for ( Uint32 i = 0; i < m_runningDialogs.Size(); ++i )
	{
		m_runningDialogs[ i ].m_instance->Debug_GatherThreadInfos( infos );
	}
}

