/**
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../engine/flashPlayer.h"
#include "../engine/flashValueStorage.h"

#include "storySceneSystem.h"

#define DEFAULT_HUD_LAYER			1
#define DEFAULT_MENU_LAYER_START	10
#define DEFAULT_POPUP_LAYER_START	1000

//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
class CGameplayEntity;
class CGuiObject;
class CFlashValueStorage;
class CHud;
class CGuiConfigResource;
class IFlashRenderSceneProvider;
class IRenderGameplayRenderTarget;

//////////////////////////////////////////////////////////////////////////
// CNames
//////////////////////////////////////////////////////////////////////////
RED_DECLARE_NAME( OnRequestMenu );
RED_DECLARE_NAME( OnCloseMenu );
RED_DECLARE_NAME( OnRequestPopup );
RED_DECLARE_NAME( OnClosePopup );
RED_DECLARE_NAME( DefaultHud );

//////////////////////////////////////////////////////////////////////////
// SGuiEventArg
//////////////////////////////////////////////////////////////////////////
struct SGuiEventArg
{
public:
	enum Type
	{
		Type_Undefined,
		Type_Object,
		Type_String,
		Type_Item,
		Type_Name,
		Type_Bool,
		Type_Int32,
		Type_Uint32,
		Type_Float,
	};

private:
	THandle< IScriptable >		m_objectValue;
	String						m_stringValue;

	union
	{
		Bool			m_boolValue;
		Int32			m_intValue;
		Uint32			m_uintValue;
		Float			m_floatValue;
	} u;

	Type				m_argType;

public:
	SGuiEventArg();
	SGuiEventArg( const THandle< IScriptable >& value );
	SGuiEventArg( const String& value );
	SGuiEventArg( const CName& value );
	SGuiEventArg( const SItemUniqueId& value );
	SGuiEventArg( Bool value );
	SGuiEventArg( Int32 value );
	SGuiEventArg( Uint32 value );
	SGuiEventArg( Float value );

public:
	typedef THandle< IScriptable > TObjectValue;

	RED_INLINE Type					GetArgType() const { return m_argType; }
	RED_INLINE Bool					IsValid() const { return m_argType != Type_Undefined; }
	RED_INLINE const TObjectValue&	GetObject() const { ASSERT(m_argType == Type_Object); return m_objectValue; }
	RED_INLINE const String&			GetString() const { ASSERT(m_argType == Type_String); return m_stringValue; }
	RED_INLINE SItemUniqueId			GetItem() const { ASSERT(m_argType == Type_Item); return SItemUniqueId( static_cast< SItemUniqueId::TValue >( u.m_uintValue ) ); }
	RED_INLINE CName					GetName() const { ASSERT(m_argType == Type_Name); return CName( u.m_uintValue ); }
	RED_INLINE Bool					GetBool() const { ASSERT(m_argType == Type_Bool); return u.m_boolValue; }
	RED_INLINE Int32					GetInt32() const { ASSERT(m_argType == Type_Int32); return u.m_intValue; }
	RED_INLINE Uint32					GetUint32() const { ASSERT(m_argType == Type_Uint32); return u.m_uintValue; }
	RED_INLINE Float					GetFloat() const { ASSERT(m_argType == Type_Float); return u.m_floatValue; }
};

//////////////////////////////////////////////////////////////////////////
// CGuiManager
//////////////////////////////////////////////////////////////////////////
// NOTE: This can't be an IGameSystem object mainly because of the tick order
// and this should tick post update transform
class CGuiManager
	: public CObject
	, public IFlashPlayerStatusListener
	, public IFlashExternalInterfaceHandler
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CGuiManager, CObject )

public:
	static const Uint32 MAX_GUI_EVENT_ARGS = 3;

	struct SGuiEvent
	{
		typedef TDynArray< SGuiEventArg > TGuiEventArgList;

		CName				m_eventName;
		CName				m_eventEx;
		THandle< CGuiObject > m_parent;
		TGuiEventArgList	m_args;
	};

private:
	typedef TDynArray< SGuiEvent >						TGuiEventQueue;

	typedef Bool (CGuiManager::*TExternalInterfaceFunc )( const String& methodName, CFlashMovie* flashMovie, const TDynArray< CFlashValue >& args );
	typedef Bool (CGuiManager::*TGuiEventFunc )( const SGuiEvent& event );

private:
	typedef TDynArray< CFlashValueStorage* >					TFlashValueStorageList;
	typedef TDynArray< CFlashRenderTarget* >					TFlashRenderTargetList;
	typedef TDynArray< THandle< CGuiObject > >					TGuiObjectHandleList;

private:
	struct SExternalInterfaceFuncDesc
	{
		const Char*				m_methodName;
		TExternalInterfaceFunc	m_func;	
	};

	struct SGuiEventDesc
	{
		CName					m_eventName;
		TGuiEventFunc			m_func;
	};

private:
	static SExternalInterfaceFuncDesc		sm_externalInterfaceTable[];
	static SGuiEventDesc					sm_guiEventTable[];


private:
	TGuiObjectHandleList					m_guiObjectTickList;
	TFlashValueStorageList					m_flashValueStorageList;
	TFlashRenderTargetList					m_flashRenderTargetList;

private:
	TGuiEventQueue							m_guiEventQueue;
	Bool									m_guiEventsProcessed;

private:
	CGuiConfigResource*						m_guiConfigResource;

private:
	CFlashRenderTarget*						m_flashRenderTarget;
	IFlashRenderSceneProvider*				m_flashRenderSceneProvider;
	THashMap< String, CFlashValueStorage::THandlerList > m_invalidatedKeysMap;

public:
											CGuiManager();
	virtual Bool							Initialize();
	virtual Bool							Deinitialize() { return true; }
	
public:
	virtual void							Tick( Float timeDelta );
	virtual void							CallGuiEvent( const SGuiEvent& event );

public:
	virtual void							OnGameStart( const CGameInfo& gameInfo );
	virtual void							OnGameEnd( const CGameInfo& gameInfo );
	virtual void							OnWorldStart( const CGameInfo& gameInfo );
	virtual void							OnWorldEnd( const CGameInfo& gameInfo );

public:
	virtual void							OnVideoStarted( CName videoClient ) {}
	virtual void							OnVideoStopped( CName videoClient ) {}
	virtual void							OnVideoSubtitles( CName videoClient, const String& subtitles ) {}

public:
	virtual void							OnEnteredConstrainedState() {}
	virtual void							OnExitedConstrainedState() {}

public:
	//! CObject functions
	virtual void							OnFinalize() override;
	virtual void							OnSerialize( IFile& file ) override;

public:
	//! IFlashPlayerStatusListener functions
//	virtual void							OnFlashPlayerShuttingDown() override;

public:
	//! IFlashExternalInterfaceHandler functions
	virtual void					OnFlashExternalInterface( const String& methodName, CFlashMovie* flashMovie, const TDynArray< CFlashValue >& args, CFlashValue& outRetval ) override;

public:
	virtual void					OnGenerateDebugFragments( CRenderFrame* frame ) {}

public:
	Bool							RegisterFlashValueStorage( CFlashValueStorage* flashValueStorage );
	Bool							UnregisterFlashValueStorage( CFlashValueStorage* flashValueStorage );

public:
	Bool							RegisterForTick( CGuiObject* guiObject );
	Bool							UnregisterForTick( CGuiObject* guiObject );

public:
	Bool							RegisterFlashRenderTarget( CFlashRenderTarget* flashRenderTarget );
	Bool							UnregisterFlashRenderTarget( CFlashRenderTarget* flashRenderTarget );

protected:
	void							SetRenderSceneProvider( IFlashRenderSceneProvider* renderSceneProvider );
	void							ClearRenderSceneProvider();

// public:
// 	RED_FORCE_INLINE Float GetHeight() const { return 720.f; }
// 	RED_FORCE_INLINE Float GetWidth() const { return 1280.f; }

public:
	virtual CHud* GetHud() const { return nullptr; }

public:
	virtual Bool NeedsInputCapture() const { return false; }

	virtual void RequestClosePanel() {}

	virtual void SetHudInput( Bool value ) {} // FIXME: Demo hack around broken script input system

public:
	virtual Bool IsAnyMenu() const { return false; }
	virtual void RefreshMenu() {}
	virtual const CMenu* GetRootMenu() { return nullptr; }

public:
	virtual CInteractionComponent* GetActiveInteraction() const { return nullptr; }
	virtual void SetActiveInteraction( CInteractionComponent* interaction, Bool force ) { }

public:
	virtual void DialogHudShow() {}
	virtual void DialogHudHide() {}
	virtual void DialogSentenceSet( const String& htmlText, bool alternativeUI ) {}
	virtual	void DialogPreviousSentenceSet( const String& htmlText ) {}
	virtual void DialogPreviousSentenceHide() {}
	virtual void DialogSentenceHide() {}
	virtual void DialogChoicesSet( const TDynArray< SSceneChoice >& choices, Bool alternativeUI ) {}
	virtual void DialogChoiceSelectionSet( Int32 selectionId ) {}
	virtual Int32  DialogChoiceSelectionGet() { return -1; }
	virtual void DialogChoiceTimeoutSet( Float timeOutPercent ) {}
	virtual void DialogChoiceTimeoutHide() {}
	virtual void DialogSkipConfirmShow() {}
	virtual void DialogSkipConfirmHide() {}
	virtual Bool IsDialogHudInitialized() const { return false; }

	virtual void SubtitleShow( ISceneActorInterface * actor, const String & text, Bool alternativeUI  ) {}
	virtual void SubtitleHide( ISceneActorInterface * actor ) {}

	virtual void DebugTextShow( const String& text ) {}
	virtual void DebugTextHide() {}

	virtual void ShowOneliner( const String& plainText, const CEntity* entity ) {}
	virtual void HideOneliner( const CEntity* entity ) {}

protected:
	RED_INLINE CGuiConfigResource*		GetGuiConfig() const { return m_guiConfigResource; }

protected:
	virtual Bool							RequestMenu( const CName& menuName, const THandle< IScriptable >& scriptInitData )=0;
	virtual Bool							CloseMenu( const CName& menuName )=0;
	virtual Bool							RequestPopup( const CName& popupName, const THandle< IScriptable >& scriptInitData )=0;
	virtual Bool							ClosePopup( const CName& popuName )=0;

protected:
	virtual Bool							RegisterFlashHud( const String& hudName, const CFlashValue& flashHud )=0;
	virtual Bool							RegisterFlashMenu( const String& menuName, const CFlashValue& flashMenu )=0;
	virtual Bool							RegisterFlashPopup( const String& popupName, const CFlashValue& flashPopup )=0;

private:
	void									ProcessGuiEventQueue();

private:
	Bool									RegisterExternalInterfaces();

private:
	Bool									ExternalInterfaceRegisterHud( const String& methodName, CFlashMovie* flashMovie, const TDynArray< CFlashValue >& args );
	Bool									ExternalInterfaceRegisterMenu( const String& methodName, CFlashMovie* flashMovie, const TDynArray< CFlashValue >& args );
	Bool									ExternalInterfaceRegisterPopup( const String& methodName, CFlashMovie* flashMovie, const TDynArray< CFlashValue >& args );
	Bool									ExternalInterfaceIsUsingPad( const String& methodName, CFlashMovie* flashMovie, const TDynArray< CFlashValue >& args );
	Bool									ExternalInterfaceIsPadConnected( const String& methodName, CFlashMovie* flashMovie, const TDynArray< CFlashValue >& args );

private:
	Bool									GuiEventOnRequestMenu( const SGuiEvent& event );
	Bool									GuiEventOnCloseMenu( const SGuiEvent& event );
	Bool									GuiEventOnRequestPopup( const SGuiEvent& event );
	Bool									GuiEventOnClosePopup( const SGuiEvent& event );
};

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
BEGIN_ABSTRACT_CLASS_RTTI( CGuiManager )
PARENT_CLASS( CObject )
END_CLASS_RTTI()

