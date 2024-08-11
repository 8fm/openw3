/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "..\..\common\game\selfUpdatingComponent.h"

#include "idGraphBlockText.h"
#include "idEventSenderDataStructs.h"

struct	SIDInterlocutorDefinition;
struct	GeneralEventData;
struct	AIEventData;
struct	AnimationEventData;
struct	SGeneralEventData;
struct	SAIEventData;
struct	SAnimationEventData;
struct	SIDConnectorPackDefinition;
struct	SIDConnectorRequest;
struct	SDialogStartRequest;
class	SIDLineInstance;
class	CInteractiveDialogInstance;

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CIDInterlocutorComponent 
	: public CSelfUpdatingComponent
	, public ISceneActorInterface 
{
	DECLARE_ENGINE_CLASS( CIDInterlocutorComponent, CSelfUpdatingComponent, 0 );

private:
	//------------------------------------------------------------------------------------------------------------------
	// Connector instance - managed by CIDInterlocutorComponent
	//------------------------------------------------------------------------------------------------------------------
	struct SConnectorInstance : public Red::System::NonCopyable // ...and that's for a reason
	{
		const SIDConnectorLine&	m_line;
		Int32					m_refCount;
		Uint32					m_packIndex;
		Uint32					m_dialogId;

		SConnectorInstance( const SIDConnectorLine&	line, Uint32 packIndex, Uint32 dialogId )
			: m_line( line )
			, m_refCount( 1 )
			, m_packIndex( packIndex )
		{
		}

		SConnectorInstance::~SConnectorInstance()
		{
			R6_ASSERT( m_refCount == 0 )
		}
	};

	//------------------------------------------------------------------------------------------------------------------
	// Variables
	//------------------------------------------------------------------------------------------------------------------
protected:
	TDynArray< Uint32 >						m_dialogInstances;	
	CEntity*								m_entityPayingAttentionTo;
	CInteractiveDialogInstance*				m_focusedDialog;
	TDynArray< SConnectorInstance* >		m_connectorInstances;
	TDynArray< SDialogStartRequest* >		m_scriptDialogStartRequests;

	Bool									m_displaysOnHUD;

	// Line playing
	SIDLineInstance*						m_lineLastCompleted;
	SIDLineInstance*						m_linePlaying;
	SIDLineInstance*						m_lineQueued;
	CIDInterlocutorComponent*				m_talkingTo;

	// Properties
	CName									m_voiceTag;				//!< Voice tag of the actor
	TSoftHandle< CInteractiveDialog >		m_defaultDialog;
	TDynArray< SIDConnectorPackDefinition >	m_connectors;
	

	//------------------------------------------------------------------------------------------------------------------
	// Methods
	//------------------------------------------------------------------------------------------------------------------
public:
	CIDInterlocutorComponent();
	virtual ~CIDInterlocutorComponent();

	virtual void						OnSerialize( IFile& file );

	// Editor stuff
	virtual void						OnPropertyPostChange( IProperty* prop );

	// Dialog
	CName								GetVoiceTag					( ) const										{ return m_voiceTag;				};
	String								GetLocalizedName			( ) const;
	Bool								MatchesDefinition			( const SIDInterlocutorDefinition& def ) const;
	const CGameplayEntity*				GetOwnerEntity				( ) const;
	template< typename TLineType > void TryToPlayLine				( const TLineType& line, Uint32 dialogId );
	Bool								GetWantsToDisplayOnHUD		( )												{ return m_displaysOnHUD;			};
	void								SetWantsToDisplayOnHUD		( Bool showOnHUD )								{ m_displaysOnHUD	= showOnHUD;	};
	CIDInterlocutorComponent*			GetToWhomAmITalking			( )	;

	// Flow
	void								OnLookAt					( );
	void								OnInteractiveDialogStarted	( Uint32 instanceID ); 
	void								OnInteractiveDialogEnded	( Uint32 instanceID ); 
	void								OnInterlocutorDetached		( );
	void								CustomTick					( Float deltaTime );
	void								OnLineInterrupted			( );

	// Completion checks
	EIDLineState						GetLineStatus				( const SIDBaseLine& line, Uint32 dialogId );

	// Attention to dialog and interlocutors
	void								SetAttentionToDialog		( CInteractiveDialogInstance* dialogInstance );
	CInteractiveDialogInstance*			GetDialogWithAttention		( ) const;
	Bool								IsInDialog					( ) const;
	Bool								IsSpeakingWith				( CIDInterlocutorComponent* other ) const;
	Bool								HasSomethingToTalkWith		( CIDInterlocutorComponent* other ) const;

	// Attention to objects
	RED_INLINE void					SetAttentionToEntity		( CEntity* entity )								{ m_entityPayingAttentionTo = entity;	};
	RED_INLINE CEntity*				GetEntityWithAttention		( ) const										{ return m_entityPayingAttentionTo;		};

	// Event Interaction
	void								RaiseGeneralEvent			( GeneralEventData* data );
	void								RaiseAIEvent				( SAIEventData* data );
	void								RaiseAIEvent				( AIEventData* data );
	void								RaiseBehaviorEvent			( SAnimationEventData* data );
	void								RaiseBehaviorEvent			( AnimationEventData* data );

	// Connector system
	void								RequestConnector			( SIDConnectorRequest& request );
	void								ReleaseConnector			( const SIDConnectorLine* connector );

	// CComponent interface																													
	virtual void						OnDetached					( CWorld* world );

	// ISceneActorInterface
	virtual Bool						HasMimicAnimation( const CName& slot = CNAME( MIMIC_SLOT ) ) const override;
	virtual Bool						PlayLipsyncAnimation( CSkeletalAnimationSetEntry* anim, Float offset = 0.0f ) override;
	virtual Bool						PlayMimicAnimation( const CName& anim, const CName& slot = CNAME( MIMIC_SLOT ), Float blentTime = 0.0f, Float offset = 0.0f ) override;
	virtual Bool						StopMimicAnimation( const CName& slot = CNAME( MIMIC_SLOT ) ) override;
	virtual Bool						HasSceneMimic() const;
	virtual Bool						SceneMimicOn();
	virtual void						SceneMimicOff();
	virtual CEntity*					GetSceneParentEntity();
	virtual Vector						GetSceneHeadPosition() const;
	virtual Int32						GetSceneHeadBone() const;
	virtual Bool						WasSceneActorVisibleLastFrame() const;
	virtual Vector						GetBarPosition() const;
	virtual Vector						GetAimPosition() const;
	virtual CName						GetSceneActorVoiceTag() const;

	// Shower scene tempshit
	void								DoSimpleLookAt( const CIDInterlocutorComponent*	target );
	void								EndLookingAt();

private:
	template< typename TLineType > void	QueueLine					( const TLineType& line, Uint32 dialogId );
	void								PlayQueuedLine				( );

	void								CheckEndSayingLine			( Float timeDelta );
	void								EndedSayingLine				( );
	void								OnEndedTalking				( );

	// Connector packs
	void								ValidateConnectors			( );
	void								UnloadUnneededConnectorPacks( );

	void								FindReceiver				( CInteractiveDialogInstance* dialog, const SIDLineInstance& line );

	// Scripts
	void								funcOnLookAt				( CScriptStackFrame& stack, void* result );
	void								funcGetEntityWithAttention	( CScriptStackFrame& stack, void* result );
	void								funcSetEntityWithAttention	( CScriptStackFrame& stack, void* result );
	void								funcGetIsFocused			( CScriptStackFrame& stack, void* result );
	void								funcIsInDialog				( CScriptStackFrame& stack, void* result );
	void								funcIsSpeakingWith			( CScriptStackFrame& stack, void* result );
	void								funcHasSomethingToTalkWith	( CScriptStackFrame& stack, void* result );
	//void								funcGetDialogWithAttention	( CScriptStackFrame& stack, void* result );

	void								funcRequestDialog			( CScriptStackFrame& stack, void* result );
};


BEGIN_CLASS_RTTI( CIDInterlocutorComponent )
	PARENT_CLASS( CSelfUpdatingComponent )	
	PROPERTY_CUSTOM_EDIT( m_voiceTag, TXT( "Default voice of this interlocutor" ), TXT( "EntityVoiceTagSelect" ) );
	PROPERTY_EDIT( m_defaultDialog, TXT("Dialog to play on look at") )
	PROPERTY_EDIT( m_connectors, TXT("Default dialog connectors") )
	NATIVE_FUNCTION( "I_OnLookAt"					, funcOnLookAt );
	NATIVE_FUNCTION( "I_GetEntityWithAttention"		, funcGetEntityWithAttention );
	NATIVE_FUNCTION( "I_SetEntityWithAttention"		, funcGetEntityWithAttention );
	NATIVE_FUNCTION( "I_GetIsFocused"				, funcGetIsFocused );
	NATIVE_FUNCTION( "I_IsInDialog"					, funcIsInDialog );
	NATIVE_FUNCTION( "I_IsSpeakingWith"				, funcIsSpeakingWith );
	NATIVE_FUNCTION( "I_HasSomethingToTalkWith"		, funcHasSomethingToTalkWith );
	//NATIVE_FUNCTION( "I_GetDialogWithAttention"	, funcGetDialogWithAttention);
	NATIVE_FUNCTION( "I_RequestDialog"				, funcRequestDialog )
END_CLASS_RTTI()

#include "idInterlocutor.inl"