/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "dlcMounter.h"
#include "dlcDefinition.h"
#include "dlcManager.h"
#include "../engine/entityTemplateModifier.h"
#include "guiManager.h"

#include "../core/depot.h"
#include "../core/configVarSystem.h"
#include "../engine/localizationManager.h"

//--------------------------------

IMPLEMENT_ENGINE_CLASS( CDLCManager );

RED_DEFINE_NAME( DisplayNewDlcInstalled );

const AnsiChar* CDLCManager::CONFIG_GROUP_NAME = "DLC";
const AnsiChar* CDLCManager::CONFIG_KEY_PREFIX = "DlcEnabled_";

CDLCManager::CDLCManager()
	: m_isNewContentAvailable( false )
	, m_simulateAllDLCsAvailable( false )
	, m_configsReloadedFirstTime( false )
	, m_activationState( EDLCActivationState::DAS_Inactive )
{
	if ( GIsEditor )
	{
		CONFIG_GROUP_NAME = "DLC_Editor";
	}
}

void CDLCManager::OnPackageAvailable( RuntimePackageID packageID )
{
	if ( packageID != BASE_RUNTIME_PACKAGE_ID )
	{
		LOG_GAME(TXT("[DLC] packageID %u available"), packageID );
		if( m_isNewContentAvailable == false)
		{
			//! display message only when game is active
			if( GGame->IsActive() )
			{
	#ifdef PLATFORM_PS4
				static const String message = TXT( "error_message_new_dlc_ps4" );
	#else
				static const String message = TXT( "error_message_new_dlc" );
	#endif
				// public function DisplayNewDlcInstalled( message : string ) : void
				CallFunction( GCommonGame->GetGuiManager(), CNAME( DisplayNewDlcInstalled ), message );
			}			
			m_isNewContentAvailable = true;
		}		
	}
}

void CDLCManager::Scan()
{
	CTimeCounter counter;

	RED_FATAL_ASSERT( SIsMainThread(), "DLC Manager can only be called from main thread" );

	m_isNewContentAvailable = false;

	// get DLC directory
	CDirectory* dir = GDepot->FindLocalDirectory( TXT("dlc") );
	if ( !dir )
	{
		LOG_GAME( TXT("[DLC] There's no DLC directory in the depot. No DLCs will be mounted.") );
		return;
	}

	// Know if we should only allow runtime installed *.reddlc files to avoid patched DLCs that are incomplete without the DLC itself installed
#ifdef RED_PLATFORM_CONSOLE
	static const Bool dlcRuntimeInstall = Red::System::StringSearch( SGetCommandLine(), TXT( "-dlcinstall" ) ) != nullptr;
#else
	static const Bool dlcRuntimeInstall = false;
#endif

	// look for DLC definition files - NOTE, we only look one directory deep
	TDynArray< CDiskFile* > dlcFiles;
	for ( CDirectory* dlcDir : dir->GetDirectories() )
	{
		for ( CDiskFile* dlcFile : dlcDir->GetFiles() )
		{
			// Support for patching *.reddlc files. If expecting runtime DLC, then don't count as available
			// until the DLC itself has been attached at runtime. Otherwise, we could just have a *.reddlc in a patch
			// but without the rest of the DLC available yet.
			const String ext = StringHelpers::GetFileExtension( dlcFile->GetFileName() );
			if ( ext == ResourceExtension< CDLCDefinition >() )
			{
				const Bool addDlcFile = dlcFile->IsRuntimeAttached() || !dlcRuntimeInstall;
				LOG_GAME(TXT("dlcFile '%ls', isRuntimeAttached=%d; addDlcFile=%d, dlcRuntimeInstall=%d"),
					dlcFile->GetDepotPath().AsChar(), 
					dlcFile->IsRuntimeAttached(),
					addDlcFile,
					dlcRuntimeInstall );

				if ( addDlcFile )
					dlcFiles.PushBack( dlcFile );
			}
		}
	}

	LOG_GAME( TXT("[DLC] Found %d DLC definition files"), dlcFiles.Size() );

	// Load the DLC definitions
	Uint32 numNewDLCFound = 0;
	for ( CDiskFile* file : dlcFiles )
	{
		THandle< CDLCDefinition > def = Cast< CDLCDefinition >( file->Load() );
		if ( !def )
		{
			ERR_GAME( TXT("[DLC] Failed to load DLC definition from '%ls'"), file->GetDepotPath().AsChar() );
		}
		else if ( def->GetID().Empty() )
		{
			ERR_GAME( TXT("[DLC] DLC definition from '%ls' has empty DLC ID"), file->GetDepotPath().AsChar() );
		}
		else
		{
			// add to list if not already there
			if ( !m_definitions.Exist( def ) )
			{
				LOG_GAME( TXT("[DLC] Found new DLC '%ls' (id: '%ls', file: '%ls')"), 
					def->GetName().AsChar(), def->GetID().AsChar(), file->GetDepotPath().AsChar() );

				// add to list
				m_definitions.PushBack( def );
				numNewDLCFound += 1;

				// get the enabled state - either from config or from default value
				Bool isEnabled = def->IsInitiallyEnabled();
				LoadConfig( def->GetID(), isEnabled );

				// use the initial state of the DLC
				m_enabledDLCs.Insert( def->GetID(), isEnabled );
				LOG_GAME( TXT("[DLC] Set tnitial state of DLC '%ls' to %ls"), 
					def->GetID().AsChar(), isEnabled ? TXT("ENABLED") : TXT("DISABLE") );
			}
		}
	}

	// Stats
	if ( numNewDLCFound > 0 )
	{
		LOG_GAME( TXT("[DLC] Found %d new DLCs since last scan"), numNewDLCFound );
		SLocalizationManager::GetInstance().InvalidateEmptyPacks();
		ApplyLanguagePacks( false ); // we'll do a refresh UI higher up because of new DLC
		StoreConfig();
	}
	else
	{
		LOG_GAME( TXT("[DLC] No new DLCs found since last scan") );
	}
}

void CDLCManager::GetDLCs( TDynArray< CName >& outDefinitions ) const
{
	RED_FATAL_ASSERT( SIsMainThread(), "DLC Manager can only be called from main thread" );

	outDefinitions.Reserve( outDefinitions.Size() + m_definitions.Size() );

	for ( auto dlc : m_definitions )
	{
		outDefinitions.PushBack( dlc->GetID() );
	}
}

void CDLCManager::GetEnabledDefinitions( TDynArray< THandle< CDLCDefinition > >& outDefinitions ) const
{
	RED_FATAL_ASSERT( SIsMainThread(), "DLC Manager can only be called from main thread" );

	outDefinitions.Reserve( outDefinitions.Size() + m_definitions.Size() );

	for ( auto dlc : m_definitions )
	{
		if ( IsDLCEnabled( dlc->GetID() ) )
		{
			outDefinitions.PushBack( dlc );
		}
	}
}

void CDLCManager::GetEnabledContent( TDynArray< THandle< IGameplayDLCMounter > >& outContent ) const
{
	RED_FATAL_ASSERT( SIsMainThread(), "DLC Manager can only be called from main thread" );

	outContent.Reserve( outContent.Size() + m_definitions.Size() );

	for ( auto dlc : m_definitions )
	{
		if ( IsDLCEnabled( dlc->GetID() ) )
		{
			dlc->GetContentMounters( outContent );
		}
	}
}

namespace Config
{
	extern TConfigVar<String> cvTextLanguage;
	extern TConfigVar<String> cvSpeechLanguage;
	extern TConfigVar<String> cvRequestedTextLanguage;
	extern TConfigVar<String> cvRequestedSpeechLanguage;
}

void CDLCManager::ApplyLanguagePacks( Bool refreshUI /*= false*/ )
{
	if ( !m_configsReloadedFirstTime )
	{
		LOG_GAME( TXT("[DLC] ApplyLanguagePacks: skipping until configs reloaded first time. Since won't remove already enabled languages.") );
		return;
	}

	// Let it add the languages, since we can't change them until back in the main menu anyway. We only request to change the language and it's up to the
	// main menu controller for now to actually do it. This way we'll have the change pending for when we go back to the main menu.
	if ( GGame->IsActive() )
	{
		WARN_GAME(TXT("[DLC] ApplyLanguagePacks: unable to switch languages while game is active. Did user settings reload during the game?!"));
	}

	RED_FATAL_ASSERT( SIsMainThread(), "DLC Manager can only be called from main thread" );

	// Note: we don't support disabling language packs at runtime, and the content will stay mounted and the speeches added.
	// This is a constraint to keep from breaking the game at this point.
	// get enabled language packs
	// 	m_enabledExtraTextLanguages.ClearFast();
	// 	m_enabledExtraSpeechLanguages.ClearFast();
	const Uint32 oldLanguageCount = m_enabledExtraTextLanguages.Size() + m_enabledExtraSpeechLanguages.Size();
	GetEnabledExtraLanguages( m_enabledExtraTextLanguages, m_enabledExtraSpeechLanguages );
	const Uint32 newLanguageCount = m_enabledExtraTextLanguages.Size() + m_enabledExtraSpeechLanguages.Size();

#ifdef RED_LOGGING_ENABLED
	LOG_GAME( TXT("[DLC] DLC content applying language packs") );
	LOG_GAME( TXT("[DLC] Available text languages (%u):"), m_enabledExtraTextLanguages.Size() );
	for ( const String& lang : m_enabledExtraTextLanguages )
	{
		LOG_GAME( TXT("[DLC]\t%ls"), lang.AsChar() );
	}
	LOG_GAME( TXT("[DLC] Available speech languages (%u):"), m_enabledExtraSpeechLanguages.Size() );
	for ( const String& lang : m_enabledExtraSpeechLanguages )
	{
		LOG_GAME( TXT("[DLC]\t%ls"), lang.AsChar() );
	}
#endif

	SLocalizationManager::GetInstance().AddRuntimeDLCLanguages( m_enabledExtraTextLanguages, m_enabledExtraSpeechLanguages );

	Bool requiresLocalizationChange = false;

	// Apply text language if possible
	const String& textLangToApply = Config::cvRequestedTextLanguage.Get();
	if ( !textLangToApply.Empty() && textLangToApply != Config::cvTextLanguage.Get() && m_enabledExtraTextLanguages.Exist( textLangToApply ) )
	{
		LOG_GAME(TXT("[DLC]Setting text language to %ls"), textLangToApply.AsChar());
		Config::cvTextLanguage.Set( textLangToApply );
		requiresLocalizationChange = true;
	}

	// Apply speech language if possible
	const String& speechLangToApply = Config::cvRequestedSpeechLanguage.Get();
	if ( !speechLangToApply.Empty() && speechLangToApply != Config::cvSpeechLanguage.Get() && m_enabledExtraSpeechLanguages.Exist( speechLangToApply ) )
	{
		LOG_GAME(TXT("[DLC]Setting speech language to %ls"), speechLangToApply.AsChar());
		Config::cvSpeechLanguage.Set( speechLangToApply );
		requiresLocalizationChange = true;
	}

	if ( requiresLocalizationChange )
	{
		LOG_GAME(TXT("[DLC]RequireLocalizationChange called"));
		GCommonGame->RequireLocalizationChange();
	}

	// Lame because this is how our UI rolls with config changes, knocking you back into the top level menu
	const Bool languageCountChanged = oldLanguageCount != newLanguageCount;
	if ( refreshUI && languageCountChanged && !GGame->IsActive() && GCommonGame->GetGuiManager() )
	{
		GCommonGame->GetGuiManager()->RefreshMenu();
	}
}

void CDLCManager::GetEnabledExtraLanguages( TDynArray< String >& outEnabledExtraStrings, TDynArray< String >& outEnabledExtraSpeeches ) const
{
	RED_FATAL_ASSERT( SIsMainThread(), "DLC Manager can only be called from main thread" );

	for ( const auto& dlc : m_definitions )
	{
		if ( IsDLCEnabled( dlc->GetID() ) )
		{
			const TDynArray< SDLCLanguagePack >& langPacks = dlc->GetLanguagePacks();
			for ( const SDLCLanguagePack& pack : langPacks )
			{
				outEnabledExtraStrings.PushBackUnique( pack.m_textLanguages );
				outEnabledExtraSpeeches.PushBackUnique( pack.m_speechLanguages );
			}
		}
	}
}

void CDLCManager::EnableDLC( const CName id, const Bool enable )
{
	RED_FATAL_ASSERT( SIsMainThread(), "DLC Manager can only be called from main thread" );

	if ( IsDLCEnabled(id) != enable )
	{
		m_enabledDLCs[ id ] = enable;
		if ( enable )
		{
			// Could keep a list of langpack DLCs and whether should bother
			ApplyLanguagePacks( true );
		}
		StoreConfig();
	}
}

const Bool CDLCManager::IsDLCEnabled( const CName id ) const
{
	RED_FATAL_ASSERT( SIsMainThread(), "DLC Manager can only be called from main thread" );

	Bool isEnabled = false;
	m_enabledDLCs.Find(id, isEnabled);
	return isEnabled;
}

const Bool CDLCManager::IsDLCAvaiable( const CName id ) const
{
	RED_FATAL_ASSERT( SIsMainThread(), "DLC Manager can only be called from main thread" );

	for ( auto dlc : m_definitions )
	{
		if ( dlc->GetID() == id )
		{
			return true;
		}
	}

	return false;
}

const Bool CDLCManager::IsDLCVisibleInMenu( const CName id ) const
{
	RED_FATAL_ASSERT( SIsMainThread(), "DLC Manager can only be called from main thread" );

	for ( auto dlc : m_definitions )
	{
		if ( dlc->GetID() == id )
		{
			return dlc->IsVisibleInMenu();
		}
	}

	return false;
}

const String CDLCManager::GetDLCName( const CName id ) const
{
	RED_FATAL_ASSERT( SIsMainThread(), "DLC Manager can only be called from main thread" );

	for ( auto dlc : m_definitions )
	{
		if ( dlc->GetID() == id )
		{
			return dlc->GetName();
		}
	}

	return String::EMPTY;
}

const String CDLCManager::GetDLCDescription( const CName id ) const
{
	RED_FATAL_ASSERT( SIsMainThread(), "DLC Manager can only be called from main thread" );

	for ( auto dlc : m_definitions )
	{
		if ( dlc->GetID() == id )
		{
			return dlc->GetDescription();
		}
	}

	return String::EMPTY;
}

void CDLCManager::MountDLCs()
{
	RED_FATAL_ASSERT( SIsMainThread(), "DLC Manager can only be called from main thread" );
	RED_FATAL_ASSERT( m_activationState == EDLCActivationState::DAS_Inactive, "CDLCManager::MountDLCs(): no DLCs can be active during DLC mounting." );

	// get content to mount
	m_mountedContent.ClearFast();
	GetEnabledContent( m_mountedContent );

	LOG_GAME( TXT("[DLC] Mounted %d DLC mounter(s)."), m_mountedContent.Size() );
}

void CDLCManager::UnmountDLCs()
{
	RED_FATAL_ASSERT( SIsMainThread(), "DLC Manager can only be called from main thread" );
	RED_FATAL_ASSERT( m_activationState == EDLCActivationState::DAS_Inactive, "CDLCManager::UnmountDLCs(): no DLCs can be active during DLC unmounting." );

	const Uint32 numMounters = m_mountedContent.Size();
	m_mountedContent.ClearFast();
	LOG_GAME( TXT("[DLC] Unmounted %d DLC mounter(s)."), numMounters );
}

/// Game is starting, call mounters callbacks
void CDLCManager::OnGameStarting()
{
	RED_FATAL_ASSERT( SIsMainThread(), "DLC Manager can only be called from main thread" );
	RED_FATAL_ASSERT( m_activationState == EDLCActivationState::DAS_Inactive, "CDLCManager::OnGameStarting(): no DLCs can be active during DLC activation." );

	MountDLCs();

	// call events
	for ( auto& content : m_mountedContent )
	{
		if ( content )
			content->OnGameStarting();
	}

	SEntityTemplateModifierManager::GetInstance().Activate();

	LOG_GAME( TXT("[DLC] OnGameStarting: %d DLC mounters processed."), m_mountedContent.Size() );
}

/// Game is after game subsystems started, call mounters callbacks
void CDLCManager::OnGameStarted()
{
	RED_FATAL_ASSERT( m_activationState == EDLCActivationState::DAS_Inactive, "CDLCManager::OnGameStarted(): no DLCs can be active during DLC activation." );

	// call events
	for ( auto& content : m_mountedContent )
	{
		if ( content )
			content->OnGameStarted();
	}

	m_activationState = EDLCActivationState::DAS_ActivatedByGame;
	LOG_GAME( TXT("[DLC] OnGameStarted: %d DLC mounters processed."), m_mountedContent.Size() );
}

void CDLCManager::OnGameEnding()
{
	RED_FATAL_ASSERT( SIsMainThread(), "DLC Manager can only be called from main thread" );
	RED_FATAL_ASSERT( m_activationState == EDLCActivationState::DAS_ActivatedByGame || m_activationState == EDLCActivationState::DAS_Inactive,
		"CDLCManager::OnGameEnding(): DLCs are activated by normal/editor game (not by editor itself) or no DLCs are activated." );

	if( m_activationState == EDLCActivationState::DAS_ActivatedByGame )
	{
		SEntityTemplateModifierManager::GetInstance().Deactivate();

		// call events
		for ( auto& content : m_mountedContent )
		{
			if ( content )
				content->OnGameEnding();
		}

		m_activationState = EDLCActivationState::DAS_Inactive;
		LOG_GAME( TXT("[DLC] OnGameEnding: %d DLC mounters processed."), m_mountedContent.Size() );

		UnmountDLCs();
	}
}

#ifndef NO_EDITOR

void CDLCManager::OnEditorStarted()
{
	RED_FATAL_ASSERT( SIsMainThread(), "DLC Manager can only be called from main thread" );
	RED_FATAL_ASSERT( m_activationState == EDLCActivationState::DAS_Inactive, "CDLCManager::OnEditorStarted(): no DLCs can be active during DLC acitvation." );

	MountDLCs();

	for( auto& content : m_mountedContent )
	{
		if( content )
		{
			content->OnEditorStarted();
		}
	}

	SEntityTemplateModifierManager::GetInstance().Activate();

	m_activationState = EDLCActivationState::DAS_ActivatedByEditor;
	LOG_GAME( TXT("[DLC] OnEditorStarted: %d DLC mounters processed."), m_mountedContent.Size() );
}

void CDLCManager::OnEditorStopped()
{
	RED_FATAL_ASSERT( SIsMainThread(), "DLC Manager can only be called from main thread" );

	if( m_activationState == EDLCActivationState::DAS_ActivatedByEditor )
	{
		SEntityTemplateModifierManager::GetInstance().Deactivate();

		for( auto& content : m_mountedContent )
		{
			if( content )
			{
				content->OnEditorStopped();
			}
		}

		m_activationState = EDLCActivationState::DAS_Inactive;
		LOG_GAME( TXT("[DLC] OnEditorStopped: %d DLC mounters processed."), m_mountedContent.Size() );

		UnmountDLCs();
	}
}

#endif // !NO_EDITOR

void CDLCManager::LoadConfig( const CName id, Bool& isEnabled )
{
	RED_FATAL_ASSERT( SIsMainThread(), "DLC Manager can only be called from main thread" );

	// format the config key name
	StringAnsi key = CONFIG_KEY_PREFIX;
	key += id.AsAnsiChar();

	// get value from config system
	String value;
	if ( SConfig::GetInstance().GetValue( CONFIG_GROUP_NAME, key.AsChar(), value ) )
	{
		// we accept anything different than 0 as the true
		isEnabled = !value.Empty() && (value != TXT("0") );
	}
}

void CDLCManager::StoreConfig() const
{
	RED_FATAL_ASSERT( SIsMainThread(), "DLC Manager can only be called from main thread" );

	// store the flags for the known DLCs only
	for ( auto it = m_enabledDLCs.Begin(); it != m_enabledDLCs.End(); ++it )
	{
		StringAnsi key = CONFIG_KEY_PREFIX;
		key += it->m_first.AsAnsiChar();

		const Bool isEnabled = it->m_second;
		const Char* value = isEnabled ? TXT("1") : TXT("0");
		SConfig::GetInstance().SetValue( CONFIG_GROUP_NAME, key.AsChar(), value );
	}

	// flush config
	SConfig::GetInstance().Save();
}

//--------------------------------

void CDLCManager::funcGetDLCs( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< CName >, names, TDynArray< CName >() );
	FINISH_PARAMETERS;
	GetDLCs( names );
}

void CDLCManager::funcEnableDLC( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, id, CName::NONE );
	GET_PARAMETER( Bool, isEnabled, true );
	FINISH_PARAMETERS;
	EnableDLC( id, isEnabled );
}

void CDLCManager::funcIsDLCEnabled( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, id, CName::NONE );
	FINISH_PARAMETERS;
	RETURN_BOOL( IsDLCEnabled( id ) );
}

void CDLCManager::funcIsDLCAvaiable( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, id, CName::NONE );
	FINISH_PARAMETERS;

	if (m_simulateAllDLCsAvailable)
	{
		RETURN_BOOL( true );
	}
	else
	{
		RETURN_BOOL( IsDLCEnabled( id ) );
	}
}

void CDLCManager::funcGetDLCName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, id, CName::NONE );
	FINISH_PARAMETERS;
	RETURN_STRING( GetDLCName( id ) );
}

void CDLCManager::funcGetDLCDescription( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, id, CName::NONE );
	FINISH_PARAMETERS;
	RETURN_STRING( GetDLCDescription( id ) );
}

void CDLCManager::funcSimulateDLCsAvailable( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, shouldSimulate, true );
	FINISH_PARAMETERS;

	//m_simulateAllDLCsAvailable = shouldSimulate;
}

void CDLCManager::ReloadDLCConfigs()
{
	for( THandle< CDLCDefinition > def : m_definitions )
	{
		// get the enabled state - either from config or from default value
		Bool isEnabled = def->IsInitiallyEnabled();
		LoadConfig( def->GetID(), isEnabled );

		m_enabledDLCs[ def->GetID() ] = isEnabled;
	}

	if ( !m_configsReloadedFirstTime )
	{
		m_configsReloadedFirstTime = true;
		ApplyLanguagePacks( true );
	}
}

//--------------------------------
