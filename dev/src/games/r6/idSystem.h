/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../../common/game/gameSystem.h"
#include "idResource.h"

class CInteractiveDialogInstance;
class CIDTopicInstance;
class CIDThreadInstance;
class CIDInterlocutorComponent;
class CInteractiveDialog;
class IDialogHud;
enum EHudChoicePosition;

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
enum EIDPlayState
{
	DIALOG_Error			,
	DIALOG_Loading			,
	DIALOG_Ready			,
	DIALOG_Playing			,
	DIALOG_Interrupted		,
	DIALOG_Finished		
};

BEGIN_ENUM_RTTI( EIDPlayState		)
	ENUM_OPTION( DIALOG_Error		)
	ENUM_OPTION( DIALOG_Loading		)
	ENUM_OPTION( DIALOG_Ready		)
	ENUM_OPTION( DIALOG_Playing		)
	ENUM_OPTION( DIALOG_Interrupted	)
	ENUM_OPTION( DIALOG_Finished	)	
END_ENUM_RTTI()


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CIDStateMachine
{
private:
	EIDPlayState m_state;

protected:
	virtual void	SetPlayState(  EIDPlayState newState )			
	{
		/*
		#ifndef RED_FINAL_BUILD
			if( m_state == newState )
			{
				Bool happened;
				switch( newState )
				{
				case DIALOG_Error:
					happened	= true;
					break;
				case DIALOG_Loading:
					happened	= true;
					break;
				case DIALOG_Finished:
					happened	= true;
					break;
				case DIALOG_Playing:
					happened	= true;
					break;
				case DIALOG_Interrupted:
					happened	= true;
					break;
				case DIALOG_Ready:
					happened	= true;
					break;
				}
			}
		#endif
		*/
		m_state = newState;	
	}

public:
	RED_INLINE	EIDPlayState	GetPlayState( )	const			{	return m_state;		}
	RED_INLINE	Bool			IsPlaying( )	const			{	return GetPlayState() == DIALOG_Playing;		}
};

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
struct Debug_SIDThreadInfo
{
	const CInteractiveDialogInstance*	m_instance;
	const CIDTopicInstance*				m_topic;
	const CIDThreadInstance*			m_thread;
};

struct SRunningDialog
{
	DECLARE_RTTI_STRUCT( SRunningDialog );

	THandle< CInteractiveDialog >		m_resource;
	CInteractiveDialogInstance*			m_instance;
	Uint32								m_instanceID;
};

BEGIN_NODEFAULT_CLASS_RTTI( SRunningDialog );
	PROPERTY( m_resource );
END_CLASS_RTTI();

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
struct SDialogStartRequest : public Red::System::NonCopyable
{
	CIDInterlocutorComponent*				m_owner;
	CIDInterlocutorComponent*				m_initiatior;
	TSoftHandle< CInteractiveDialog >		m_resource;
	Uint32									m_startedInstanceID;
	EIDPlayState							m_state;

	SDialogStartRequest( CIDInterlocutorComponent* owner, CIDInterlocutorComponent* initiator, TSoftHandle< CInteractiveDialog > resource )
		: m_owner( owner )
		, m_initiatior( initiator )
		, m_resource( resource )
		, m_startedInstanceID( Uint32( -1 ) )
		, m_state( DIALOG_Loading )
	{
	}
};

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CInteractiveDialogSystem : public IGameSystem
{
	DECLARE_ENGINE_CLASS( CInteractiveDialogSystem, IGameSystem, 0 );

protected:
	Uint32										m_currentInstanceID;
	TDynArray< SRunningDialog >					m_runningDialogs;
	TDynArray< Uint32 >							m_finishedDialogs;

	CInteractiveDialogInstance*					m_dialogFocusedByHUD;
	TDynArray< TPair< CWorld*, IDialogHud* > >	m_huds;

	Float										m_cooldownFocusAutoRecalcMax;
	Float										m_cooldownFocusAutoRecalcCur;

public:
	CInteractiveDialogSystem();
	virtual ~CInteractiveDialogSystem();

	// IGameSystem interface
	virtual void OnGameStart	( const CGameInfo& gameInfo )		{};
	virtual void OnGameEnd		( const CGameInfo& gameInfo )		{};
	virtual void OnWorldStart	( const CGameInfo& gameInfo );
	virtual void OnWorldEnd		( const CGameInfo& gameInfo );
	virtual void Tick			( Float timeDelta );
	virtual void OnGenerateDebugFragments( CRenderFrame* frame )	{};

	// Own interface
	void RequestDialog			( SDialogStartRequest& info );
	void RequestFocus			( CInteractiveDialogInstance*	dialog );
	void RequestFocusRecalc		( );
	void RequestFocusEnd		( const CInteractiveDialogInstance*	dialog );
	void OnInterlocutorDetached	( CIDInterlocutorComponent* interlocutor );
	void AttachHud				( CWorld* world, IDialogHud* hud );
	void DetachHud				( CWorld* world, IDialogHud* hud );
	void OnChoiceSelected		( EHudChoicePosition option );
	CIDInterlocutorComponent*	FindActorForDialog		( CInteractiveDialogInstance* dialog, const SIDInterlocutorDefinition& actorDef ) const;
	CIDInterlocutorComponent*	GetPlayerInterlocutor	( ) const;

	CIDInterlocutorComponent*	GetInterlocutorOnDialog	( Uint32 dialogID, const CName& interlocutor ) const;
	CInteractiveDialogInstance*	GetDialogInstance		( Uint32 dialogID ) const;

	void						Debug_GatherThreadInfos( TDynArray< Debug_SIDThreadInfo >& infos ) const;

protected:

	CInteractiveDialogInstance* RunDialogInternal				( SDialogStartRequest& info );
	CInteractiveDialogInstance* FindRunningDialogByResourcePath	( const String& path )	const;
	Bool						IsDialogInstanceRunning			( const CInteractiveDialogInstance* dialog )	const;
	CInteractiveDialogInstance* FindRunningDialogByInstanceID	( Uint32 indtanceID )	const;
	Uint32						InstanceIdToIndex				( Uint32 instanceId )	const;
	void						SetHUDFocusTo					( CInteractiveDialogInstance*	dialogInstance );
	CInteractiveDialogInstance* RecalcHighestPriorityDialog		( const CInteractiveDialogInstance*	dialogToIgnore );
	void						UpdateFocusChangeOrEnd			( Float timeDelta );

	ASSIGN_GAME_SYSTEM_ID( GS_InteractiveDialog )
};

BEGIN_CLASS_RTTI( CInteractiveDialogSystem )
	PARENT_CLASS( IGameSystem )
	PROPERTY( m_runningDialogs )
	PROPERTY_EDIT( m_cooldownFocusAutoRecalcMax, TXT("Time before changing focus to a new dialog when there is no dialog focused on HUD"))
END_CLASS_RTTI()


