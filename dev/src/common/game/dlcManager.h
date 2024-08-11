/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/contentListener.h"
#include "dlcLanguagePack.h"

#ifndef NO_EDITOR
#include "../core/analyzer.h"
#endif

class CDLCDefinition;

/// DLC manager - controls how and when the extra stuff is mounted with game
/// Accessible though GGame
class CDLCManager : public CObject, public IContentListener
{
	DECLARE_ENGINE_CLASS( CDLCManager, CObject, 0 );

private:
	typedef TDynArray< THandle< CDLCDefinition > >			TDefinitions;
	typedef TDynArray< THandle< IGameplayDLCMounter > >		TMountedContent;
	typedef TSortedMap< CName, Bool >						TEnabledDLCs;

	enum class EDLCActivationState
	{
		DAS_Inactive,										/// DLCs are inactive.
		DAS_ActivatedByGame,								/// DLCs were activated by normal/editor game.
		DAS_ActivatedByEditor								/// DLCs were activated by editor.
	};

	static const AnsiChar*	CONFIG_GROUP_NAME;
	static const AnsiChar*	CONFIG_KEY_PREFIX;

	TDefinitions			m_definitions;					/// Loaded DLC definitions	
	TEnabledDLCs			m_enabledDLCs;					/// Enabled DLCs (stored/loaded from user config)
	TMountedContent			m_mountedContent;				/// Mounted content (active during game session or in editor)
	EDLCActivationState		m_activationState;				/// Activation state of enabled DLCs.
	TDynArray< String >		m_enabledExtraTextLanguages;	/// Enabled additional base text languages
	TDynArray< String >		m_enabledExtraSpeechLanguages;	/// Enabled additional base speech languages
	Bool					m_isNewContentAvailable;		/// New packages are available and should rescan for DLC when possible
	Bool					m_configsReloadedFirstTime;		/// Whether the configs have been reloaded for the first time. NOTE: not reset on user change.

public:
	CDLCManager();

	/// Get enabled DLC definitions
	void GetEnabledDefinitions( TDynArray< THandle< CDLCDefinition > >& outDefinitions ) const;

	/// Get enabled DLC content mounters
	void GetEnabledContent( TDynArray< THandle< IGameplayDLCMounter > >& outContent ) const;

	/// Get enabled extra languages. E.g., DLC pack to add entirely new base game language support
	void GetEnabledExtraLanguages( TDynArray< String >& outEnabledExtraStrings, TDynArray< String >& outEnabledExtraSpeeches ) const;

	/// (Re)Scan depot for DLCs - NOTE: DLCs will NEVER BE REMOVED, only added
	/// NOTE: this will synchronously load resources, don't call it unless hidden by loading screen
	void Scan();

	Bool IsNewContentAvailable() const { return m_isNewContentAvailable; }

	/// Get available DLCs (mounted by the system)
	void GetDLCs( TDynArray< CName >& outDefinitions ) const;

	/// Enable/Disable DLC - not, will not work when game is active
	void EnableDLC( const CName id, const Bool enable );

	/// Is DLC enabled ?
	const Bool IsDLCEnabled( const CName id ) const;

	/// Is given DLC there ? (mounted by the system)
	const Bool IsDLCAvaiable( const CName id ) const;

	/// Sould given DLC settings be visible in the menu ?
	const Bool IsDLCVisibleInMenu( const CName id ) const;

	/// Get DLC display name
	const String GetDLCName( const CName id ) const;

	EDLCActivationState GetActivationState() const;

	/// Get DLC description
	const String GetDLCDescription( const CName id ) const;

	/// Game is starting, call mounters callbacks
	void OnGameStarting();

	/// Game is after game subsystems started, call mounters callbacks
	void OnGameStarted();

	/// Game has finished, unmount DLCs
	void OnGameEnding();

#ifndef NO_EDITOR

	void OnEditorStarted();
	void OnEditorStopped();

#endif // !NO_EDITOR

	/// Reload configs for DLC (in case configs are loaded later - like on PS4 or XB1)
	void ReloadDLCConfigs();

private:
	Bool					m_simulateAllDLCsAvailable;

	/// Configuration
	void StoreConfig() const;
	void LoadConfig( const CName id, Bool& isEnabled );

	virtual void OnPackageAvailable( RuntimePackageID packageID ) override;
	virtual const Char* GetName() const override { return TXT("DLCManager"); }

	/// Language packs
	void ApplyLanguagePacks( Bool refreshUI = false );
	
	void MountDLCs();
	void UnmountDLCs();

private:
	void funcGetDLCs( CScriptStackFrame& stack, void* result );
	void funcEnableDLC( CScriptStackFrame& stack, void* result );
	void funcIsDLCEnabled( CScriptStackFrame& stack, void* result );
	void funcIsDLCAvaiable( CScriptStackFrame& stack, void* result );
	void funcGetDLCName( CScriptStackFrame& stack, void* result );
	void funcGetDLCDescription( CScriptStackFrame& stack, void* result );
	void funcSimulateDLCsAvailable( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CDLCManager );
	PARENT_CLASS( CObject );
	PROPERTY( m_definitions );
	PROPERTY( m_mountedContent );
	NATIVE_FUNCTION( "GetDLCs", funcGetDLCs );
	NATIVE_FUNCTION( "EnableDLC", funcEnableDLC );
	NATIVE_FUNCTION( "IsDLCEnabled", funcIsDLCEnabled );
	NATIVE_FUNCTION( "IsDLCAvailable", funcIsDLCAvaiable );
	NATIVE_FUNCTION( "GetDLCName", funcGetDLCName );
	NATIVE_FUNCTION( "GetDLCDescription", funcGetDLCDescription );
	NATIVE_FUNCTION( "SimulateDLCsAvailable", funcSimulateDLCsAvailable );
END_CLASS_RTTI();

RED_INLINE CDLCManager::EDLCActivationState CDLCManager::GetActivationState() const
{
	return m_activationState;
}
