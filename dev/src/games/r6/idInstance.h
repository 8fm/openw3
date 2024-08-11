/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "idResource.h"
#include "idTopic.h"

enum EIDPriority;
struct SIDBaseLine;
class CIDInterlocutorComponent;
enum EDialogDisplayMode;

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
struct CIDPage
{	
	struct SSpeakerLine
	{
		CIDInterlocutorComponent*	m_speaker;
		const SIDBaseLine*			m_line;
		RED_INLINE Bool operator==( const SSpeakerLine& other ) const { return m_speaker == other.m_speaker && m_line == other.m_line; }
	};

	TDynArray< SSpeakerLine >		m_lines;
	TDynArray< const SIDOption* >	m_options;
	EDialogDisplayMode				m_communicatorMode;
};

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CInteractiveDialogInstance : public CIDStateMachine 
{
#ifndef NO_DEBUG_PAGES
	friend class CDebugPageInteractiveDialogs; 
#endif

protected:
	Uint32											m_instanceID;
	TSoftHandle< CInteractiveDialog >				m_dialog;

	CIDInterlocutorComponent*						m_owner;
	CIDInterlocutorComponent*						m_initiatior;
	THashMap< CName, CIDInterlocutorComponent* >		m_knownInterlocutors;

	TDynArray< CIDTopicInstance* >					m_allTopics;
	CIDTopicInstance*								m_activeTopic;
	
	TDynArray< IDialogHud* >						m_huds;
	CIDPage*										m_displayingPage;

	Bool											m_wantsFocusOnHUD	: 1;
	Bool											m_focusedOnHud		: 1;
	Bool											m_isOnComunicator	: 1;

public:
	CInteractiveDialogInstance						( SDialogStartRequest& info );

	void		Tick								( Float timeDelta );
	void		OnFinished							( );

	Bool		PlayLine							( CIDInterlocutorComponent* interlocutor, const SIDBaseLine& line );
	Bool		EndLine								( CIDInterlocutorComponent* interlocutor, const SIDBaseLine& line );

	Bool		GetWantsToPlay						( )	const;
	EIDPriority	GetHighestTopicPriority				( ) const;

	void		AddChoice							( const SIDOption& choice );
	void		RemoveChoice						( const SIDOption& choice );
	void		RemoveAllChoices					( );
	void		OnChoiceSelected					( EHudChoicePosition position );

	void		GainFocusOfInterlocutors			( );

	void		AttachHud							( IDialogHud* hud );
	void		DetachHud							( IDialogHud* hud );
	void		OnGainedHUDFocus					( );
	void		OnLostHUDFocus						( );

	CIDInterlocutorComponent* GetInterlocutor		( CName name )	const;
	Bool		HasInterlocutor						( CName name )	const;
	Bool		HasInterlocutor						( const CIDInterlocutorComponent* interlocutor ) const;
	void		Debug_GatherThreadInfos				( TDynArray< Debug_SIDThreadInfo >& infos ) const;
	void		SetComunicator						( EDialogDisplayMode comunicatorMode );
	void		OnInterlocutorDetached				( CIDInterlocutorComponent* interlocutor );

	RED_INLINE Bool												IsInComunicator		( )	const			{ return m_isOnComunicator;			};
	RED_INLINE TSoftHandle< CInteractiveDialog >&					GetResourceHandle	( )					{ return m_dialog;					};
	RED_INLINE const TSoftHandle< CInteractiveDialog >&			GetResourceHandle	( ) const			{ return m_dialog;					};
	RED_INLINE Uint32												GetInstanceID		( )	const			{ return m_instanceID;				};
	RED_INLINE CIDInterlocutorComponent*							GetOwner			( )	const			{ return m_owner;					};
	RED_INLINE CIDInterlocutorComponent*							GetInitiator		( )	const			{ return m_initiatior;				};
	RED_INLINE const THashMap< CName, CIDInterlocutorComponent* >&	GetInterlocutorsMap	( )	const			{ return m_knownInterlocutors;		};
	RED_INLINE void												SetWantsFocus		( Bool wantsFocus )	{ m_wantsFocusOnHUD	= wantsFocus;	};
	RED_INLINE Bool												GetWantsFocus		( ) const			{ return m_wantsFocusOnHUD;			};
	RED_INLINE Bool												GetHasFocus			( ) const			{ return m_focusedOnHud;			};

private:
	void						CreateAllTopicInstances		( );
	void						CreateTopicInstance			( const CIDTopic* topic );
	void						SortTopicInstances			( );
	void						SetPageOnHUD				( );

	void						HandleInterruptions			( Float& timeDelta );
	CIDTopicInstance*			FindTopicToPlay				( )	const;
	void						PlayNewTopic				( CIDTopicInstance* topicInstance, Float& timeDelta );
};
