/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "localizationManager.h"

#include "sqlStringDBDataAccess.h"
#include "nullStringDBDataAccess.h"
#include "localizationCache.h"
#include "registryAccess.h"
#include "../core/loadingJobManager.h"
#include "../core/depot.h"
#include "../core/feedback.h"
#include "game.h"
#include "../core/tokenizer.h"
#include "../core/configVar.h"
#include "../core/contentManager.h"
#include "inGameConfigListingFunction.h"
#include "../redSystem/unitTestMode.h"

const String CLocalizationManager::VOICEFILE_UNDEFINED_VOICE = TXT( "????" );
const String CLocalizationManager::VOICEFILE_UNDEFINED_GROUP = TXT( "@@@@" );
const String CLocalizationManager::VOICEFILE_UNDEFINED_ID = TXT( "####" );
const String CLocalizationManager::VOICEFILE_UNDEFINED = String::Printf( TXT( "%s_%s_%s" ),
	CLocalizationManager::VOICEFILE_UNDEFINED_VOICE.AsChar(),
	CLocalizationManager::VOICEFILE_UNDEFINED_GROUP.AsChar(),
	CLocalizationManager::VOICEFILE_UNDEFINED_ID.AsChar() );

const String CLocalizationManager::STRINGS_FILE_PATH_POSTFIX = TXT( ".w3strings" );

const String CLocalizationManager::SPEECH_FILE_PATH_POSTFIX_DURANGO = TXT( "xboxone.w3speech" );
const String CLocalizationManager::SPEECH_FILE_PATH_POSTFIX_ORBIS = TXT( "ps4.w3speech" );
const String CLocalizationManager::SPEECH_FILE_PATH_POSTFIX_PC = TXT( "pc.w3speech" );
#if defined(RED_PLATFORM_DURANGO)
const String CLocalizationManager::CURRENT_SPEECH_FILE_PATH_POSTFIX = CLocalizationManager::SPEECH_FILE_PATH_POSTFIX_DURANGO;
#elif defined(RED_PLATFORM_ORBIS)
const String CLocalizationManager::CURRENT_SPEECH_FILE_PATH_POSTFIX = CLocalizationManager::SPEECH_FILE_PATH_POSTFIX_ORBIS;
#elif defined(RED_PLATFORM_WINPC)
const String CLocalizationManager::CURRENT_SPEECH_FILE_PATH_POSTFIX = CLocalizationManager::SPEECH_FILE_PATH_POSTFIX_PC;
#elif
#error Unsupported platform!
#endif

namespace Config
{
	// Blank by default so the consoles know to try inferring the default language from the system
	// Fallback is still English eventually if all else fails
	TConfigVar<String> cvTextLanguage( "Localization", "TextLanguage", TXT(""), eConsoleVarFlag_Save );
	TConfigVar<String> cvSpeechLanguage( "Localization", "SpeechLanguage", TXT(""), eConsoleVarFlag_Save );
	
	// Runtime requested languages for DLC to later try to set
	// Saved in case the DLC isn't mounted yet and the config saved but the app gets closed in order to help avoid annoyingly losing language settings
	TConfigVar<String> cvRequestedTextLanguage( "Localization", "RequestedTextLanguage", TXT(""), eConsoleVarFlag_Save );
	TConfigVar<String> cvRequestedSpeechLanguage( "Localization", "RequestedSpeechLanguage", TXT(""), eConsoleVarFlag_Save );

	TConfigVar<Bool> cvUsePlaceholdersForMissingStrings( "Localization", "UsePlaceholdersForMissingStrings", false, eConsoleVarFlag_ReadOnly );
}

struct SLanguageCode
{
	const Char* m_name;
	Uint32		m_localeId;
	Uint32		m_fallbackId;
	const Char*	m_languageCode;			// for the locale shot codes, like cs-CZ, en-US or zh-CN, this is the first part
	const Char*	m_defaultRegionCode;	// for the locale shot codes, like cs-CZ, en-US or zh-CN, this is the second part
};

static SLanguageCode LANGUAGE_CODES[] =
{
	{ TXT("PL"),	1,	20,	TXT("pl"),	TXT("PL") },
	{ TXT("EN"),	2,	1,	TXT("en"),	TXT("US") },
	{ TXT("DE"),	3,	2,	TXT("de"),	TXT("DE") },
	{ TXT("IT"),	4,	2,	TXT("it"),	TXT("IT") },
	{ TXT("FR"),	5,	2,	TXT("fr"),	TXT("FR") },
	{ TXT("CZ"),	6,	2,	TXT("cs"),	TXT("CZ") },
	{ TXT("ES"),	7,	2,	TXT("es"),	TXT("ES") },
	{ TXT("ZH"),	8,	2,	TXT("zh"),	TXT("HK") },
	{ TXT("RU"),	9,	2,	TXT("ru"),	TXT("RU") },
	{ TXT("HU"),	10,	2,	TXT("hu"),	TXT("HU") },
	{ TXT("JP"),	11,	2,	TXT("ja"),	TXT("JP") },
	{ TXT("TR"),	12,	2,	TXT("tr"),	TXT("TR") },
	{ TXT("KR"),	13,	2,	TXT("ko"),	TXT("KR") },
	{ TXT("BR"),	14,	2,	TXT("pt"),	TXT("BR") },
	{ TXT("ESMX"),	15,	2,	TXT("es"),	TXT("MX") },
	{ TXT("CN"),	16,	2,	TXT("zh"),	TXT("CN") },
	{ TXT("AR"),   	17,	2,	TXT("ar"),	TXT("AE") },
	{ TXT("DEBUG"),	20,	1,	TXT("pl"),	TXT("PL") },	 
};

CLocalizationManager::CLocalizationManager() 
	: m_isConnected( false )
	, m_neverConnect( false )
	, m_newStringsCount( 0 )
	, m_ignoreLanguagePackResources( false )
	, m_locStringsWithKeysManager( NULL )
	, m_dataAccessObject( NULL )
	, m_isDisabled( false )
	, m_createdPacks( 0 )
	, m_speechLocale()
	, m_textLocale()
	, m_defaultLanguage( TXT("EN") )
{

	RED_FATAL_ASSERT( Red::System::UnitTestMode() || GContentManager, "GContentManager is nullptr!" );

	// Only needed for cooked strings?!
	for ( const SLanguageCode& langCode : LANGUAGE_CODES )
	{
		m_languagesIds.Insert( langCode.m_name, langCode.m_localeId );
	}
	
	if ( !ParseCommandLine() || m_textLocale.Empty() )
	{
		GetLanguageFromUserSettings( m_textLocale, m_speechLocale );
		if ( GContentManager && ( m_textLocale.Empty() || m_speechLocale.Empty() ) )
		{
			GContentManager->GetValidDefaultGameLanguage( m_textLocale, m_speechLocale );
			LOG_ENGINE(TXT("CLocalizationManager: Using text=%ls and speech=%ls as chosen by the streaming installer based on the console language settings!"), m_textLocale.AsChar(), m_speechLocale.AsChar() );
		}
	}

	// Default settings
	if ( m_textLocale.Empty() )
	{
		m_textLocale = m_defaultLanguage;
		LOG_ENGINE(TXT("CLocalizationManager: text still empty! Defaulting to %ls"), m_textLocale.AsChar() );
	}
	if ( m_speechLocale.Empty() )
	{
		m_speechLocale = m_defaultLanguage;
		LOG_ENGINE(TXT("CLocalizationManager: speech still empty! Defaulting to %ls"), m_speechLocale.AsChar() );
	}

	Bool dataAccessCreated = false;
	if ( m_isDisabled == true )
	{
		ListLanguagesSupported();
		dataAccessCreated = OpenNullDataAccess();
	}

#ifndef NO_STRING_DB
	if ( !dataAccessCreated && !m_neverConnect )
	{
		// Connect to the SQLDB if using editor and not specifing lnaguages from command line
		dataAccessCreated = OpenSQLDataAccessInternal();
		if ( dataAccessCreated == false )
		{
			GFeedback->ShowError( TXT( "Unable to open any lang or SQL connection" ) );
			ERR_ENGINE( TXT( "Unable to open lang file(s)" ) );
		}
	}
#endif 

	class CDeferredInit_LocalizationManager : public IDeferredInitDelegate
	{
	public:
		CLocalizationManager* mMgr;

		CDeferredInit_LocalizationManager(CLocalizationManager* mgr)
			: mMgr (mgr)
		{}

		// anything which needs ContentMgr should be put in here
		void OnPostContentManagerInit() override
		{
			GContentManager->GetSupportedLanguages( mMgr->m_textLanguages, mMgr->m_speechLanguages, mMgr->m_defaultLanguage );
			GContentManager->SetSpeechInstallLanguage( mMgr->m_speechLocale );

			// Check if files are present
			mMgr->ValidateLanguageSelection( mMgr->m_textLocale, mMgr->m_speechLocale );

			Bool dataAccessCreated = mMgr->OpenCookedDataAccessInternal( mMgr->m_textLocale, mMgr->m_speechLocale );
			if ( dataAccessCreated == false )
			{
				LOG_ENGINE( TXT( "Unable to open lang %s specified in command line from cooked data" ), mMgr->m_textLocale.AsChar() );

				// No language was selected, open default NULL access
				dataAccessCreated = mMgr->OpenNullDataAccess();
			}
		}
	};

	if ( dataAccessCreated == false && GContentManager )
	{
		if (GDeferredInit)
		{
			GDeferredInit->AddDelegate(new CDeferredInit_LocalizationManager(this));
		}
		else
		{
			CDeferredInit_LocalizationManager d(this);
			d.OnPostContentManagerInit();
		}
	}

	FindLocaleId( m_textLocale, m_currentLocaleId );

#ifdef RED_LOGGING_ENABLED
	LOG_ENGINE(TXT("Localization manager using: text=%ls, speech=%ls"), m_textLocale.AsChar(), m_speechLocale.AsChar() );
	LOG_ENGINE(TXT("\tAvailable text languages (%u):"), m_textLanguages.Size());
	for ( const String& textLang : m_textLanguages )
	{
		LOG_ENGINE(TXT("\t\t%ls"), textLang.AsChar());
	}
	LOG_ENGINE(TXT("\tAvailable speech languages (%u):"), m_speechLanguages.Size());
	for ( const String& speechLang : m_speechLanguages )
	{
		LOG_ENGINE(TXT("\t\t%ls"), speechLang.AsChar());
	}
	LOG_ENGINE(TXT("\tDefault langauge: %ls"), m_defaultLanguage.AsChar());
#endif

	SetLanguageInUserSettings( m_textLocale, m_speechLocale );

	// Register listing functions for in game configs, so the in game configs knows what languages are available
	auto& listingFuncRegister = InGameConfig::Listing::GListingFunctionRegister::GetInstance();
	listingFuncRegister.RegisterListingFunction( CNAME( ListTextLanguages ),
		[=] ( TDynArray< InGameConfig::Listing::SEngineConfigPresetDesc >& optionsDesc )
	{
		TDynArray<String> textLanguages;
		TDynArray<String> speechLanguages;
		SLocalizationManager::GetInstance().GetAllAvailableLanguages( textLanguages, speechLanguages );

		for( Uint32 i=0; i<textLanguages.Size(); ++i )
		{
			InGameConfig::Listing::SEngineConfigPresetDesc optDesc;
			optDesc.optionDisplayName = textLanguages[i];
			optDesc.entriesDesc.PushBack( InGameConfig::Listing::SEngineConfigPresetEntryDesc( CName(TXT("Localization")), CName(TXT("TextLanguage")), textLanguages[i] ));
			optionsDesc.PushBack( optDesc );
		}
	} );

	listingFuncRegister.RegisterListingFunction( CNAME( ListSpeechLanguages ),
		[=] ( TDynArray< InGameConfig::Listing::SEngineConfigPresetDesc >& optionsDesc )
	{
		TDynArray<String> textLanguages;
		TDynArray<String> speechLanguages;
		SLocalizationManager::GetInstance().GetAllAvailableLanguages( textLanguages, speechLanguages );

		for( Uint32 i=0; i<speechLanguages.Size(); ++i )
		{
			InGameConfig::Listing::SEngineConfigPresetDesc optDesc;
			optDesc.optionDisplayName = speechLanguages[i];
			optDesc.entriesDesc.PushBack( InGameConfig::Listing::SEngineConfigPresetEntryDesc( CName(TXT("Localization")), CName(TXT("SpeechLanguage")), speechLanguages[i] ));
			optionsDesc.PushBack( optDesc );
		}
	} );
}

void CLocalizationManager::Shutdown()
{
	if ( m_dataAccessObject )
	{
		delete m_dataAccessObject;
		m_dataAccessObject = NULL;
	}

	if ( m_locStringsWithKeysManager )
	{
		delete m_locStringsWithKeysManager;
		m_locStringsWithKeysManager = NULL;
	}

	m_localizationStorage.ClearCacheMemory();

	ASSERT( m_createdPacks == 0 );
}

void CLocalizationManager::AddRuntimeDLCLanguages( const TDynArray<String>& textLanguages, const TDynArray<String>& speechLanguages )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Main thread only!" );
	m_textLanguages.PushBackUnique( textLanguages );
	m_speechLanguages.PushBackUnique( speechLanguages );
}

void CLocalizationManager::InvalidateEmptyPacks()
{
	m_localizationStorage.InvalidateEmptyPacks();
	m_missingIds.Clear();
}

void CLocalizationManager::UpdateCopiedStrings( TDynArray< LocalizedStringEntry >* localizedEntries )
{
	if ( m_copiedStringIds.Empty() )
	{
		return;
	}

	// Update strings for all but current locale

	for ( TDynArray< String >::iterator it = m_textLanguages.Begin(); it != m_textLanguages.End(); ++it )
	{
		const String& locale = *it;
		if ( locale == GetCurrentLocale() )
		{
			continue;
		}

		// Build set of strings to update

		THashMap< Uint32, String > copiedStrings;
		for ( THashMap< Uint32, Uint32 >::iterator it2 = m_copiedStringIds.Begin(); it2 != m_copiedStringIds.End(); ++it2 )
		{
			if ( localizedEntries )
			{
				// Only include this string if contained in localized entries
				bool includeString = false;
				for ( TDynArray< LocalizedStringEntry >::iterator it3 = localizedEntries->Begin(); it3 != localizedEntries->End(); ++it3 )
				{
					if ( it3->m_localizedString->GetIndex() == it2->m_second )
					{
						includeString = true;
						break;
					}
				}

				if ( !includeString )
				{
					continue;
				}
			}

			Bool isFallback = false;
			const String localizedText = GetLocalizedText( it2->m_second, locale, &isFallback );
			if ( !localizedText.Empty() && !isFallback )
			{
				copiedStrings.Set( it2->m_first, localizedText );
			}
		}

		// Update database

		if ( !copiedStrings.Empty() )
		{
			m_dataAccessObject->Update( copiedStrings, locale );
		}
	}

	// Clean up copied strings set

	if ( localizedEntries )
	{
		for ( TDynArray< LocalizedStringEntry >::iterator it = localizedEntries->Begin(); it != localizedEntries->End(); ++it )
		{
			m_copiedStringIds.Erase( it->m_localizedString->GetIndex() );
		}
	}
}

void CLocalizationManager::UpdateStringId(Uint32 oldStringId, Uint32 newStringId)
{
	String value;
	if ( m_modifiedStrings.Find( oldStringId, value ) )
	{
		m_modifiedStrings.Erase( oldStringId );
		m_modifiedStrings.Set( newStringId, value );
	}

	Uint32 srcStringId;
	if ( m_copiedStringIds.Find( oldStringId, srcStringId ) )
	{
		m_copiedStringIds.Erase( oldStringId );
		m_copiedStringIds.Set( newStringId, srcStringId );
	}

	for ( THashMap< Uint32, Uint32 >::iterator it = m_copiedStringIds.Begin(); it != m_copiedStringIds.End(); ++it )
	{
		if ( it->m_second == oldStringId )
		{
			m_copiedStringIds.Set( it->m_first, newStringId );
		}
	}
}

void CLocalizationManager::UpdateStringDatabase( /*const*/ ILocalizableObject* localizableObject, Bool updateInfo /*= false */ )
{

	TDynArray< LocalizedStringEntry > localizedEntries;
	localizableObject->GetLocalizedStrings( localizedEntries );

	if ( !OpenSomeDataAccessIfNeeded() ) return;

	if( m_dataAccessObject == NULL )
	{
		ERR_ENGINE( TXT( "stringDB access object not available" ) );
		return;
	}

	m_dataAccessObject->Update( localizedEntries );
	UpdateCopiedStrings( &localizedEntries );
	if ( updateInfo == true )
	{
		m_dataAccessObject->UpdateStringInfos( localizedEntries );
	}
}

void CLocalizationManager::UpdateStringDatabase( const THashMap< Uint32, String > &strings, const String &lang )
{
	if ( !OpenSomeDataAccessIfNeeded() ) return;

	m_dataAccessObject->Update( strings, lang );
}

/*
Commits string modifications to string db.

\param stringId String whose modifications are to be committed to string db. Must be stable, i.e. must already exist in string db.
\return True - string db updated or no update needed. False - couldn't update string db (connection down?).
*/
Bool CLocalizationManager::UpdateStringDatabase( Uint32 stringId )
{
	// String must already exist in string db. This function is unable to create new string in string db.
	ASSERT( !IsUnstableStringId( stringId ) );

	if( IsModifiedString( stringId ) )
	{
		if ( !OpenSomeDataAccessIfNeeded() )
		{
			// couldn't connect to string db
			return false;
		}

		// Wrap index in LocalizedString because this is what IStringDBDataAccess expects.
		LocalizedString localizedString;
		localizedString.SetIndex( stringId );

		return m_dataAccessObject->Update( localizedString );
	}

	// no update needed
	return true;
}

void CLocalizationManager::ResetIDs( /*const*/ ILocalizableObject* localizableObject )
{
	if ( !OpenSomeDataAccessIfNeeded() ) return;

	TDynArray< LocalizedStringEntry > localizedEntries;

	ASSERT( localizableObject );

	localizableObject->GetLocalizedStrings( localizedEntries );

	for ( Uint32 i = 0; i < localizedEntries.Size(); ++i )
	{
		if ( localizedEntries[ i ].m_localizedString != NULL )
		{
			localizedEntries[ i ].m_localizedString->SetIndex( 0 );
		}

	}
}

Bool CLocalizationManager::FindLocaleId( const String& lang, Uint32& id ) const
{
	return m_languagesIds.Find( lang, id );
}

Bool CLocalizationManager::FindLocaleStr( const Uint32& id, String& lang ) const
{
	for ( THashMap< String, Uint32 >::const_iterator it = m_languagesIds.Begin(), end = m_languagesIds.End(); it != end; ++it )
	{
		if ( it->m_second == id )
		{
			lang = it->m_first;
			return true;
		}
	}

	return false;
}

void CLocalizationManager::SetCurrentLocaleForce( const String &lang )
{
	Uint32 id;

	if( FindLocaleId( lang, id ) )
	{
		if( m_currentLocaleId != id )
		{
			m_currentLocaleId = id;
			SynchronizeWithStringDatabase();

			m_localizationStorage.ClearCacheMemory();

			if ( m_locStringsWithKeysManager != NULL )
			{
				m_locStringsWithKeysManager->ClearCache();
			}

			// release any outstanding jobs that were issued in previous locale
			for( auto itJob = m_loadingPackJobs.Begin(), endJobs = m_loadingPackJobs.End(); itJob != endJobs; ++itJob )
			{
				ILoadJob* job = itJob->m_second;
				job->Cancel();
				job->Release();
			}
			m_loadingPackJobs.Clear();

			// release any outstanding batch jobs that were issued in previous locale
			for( auto itJob = m_loadingPackBatchJobs.Begin(), endJobs = m_loadingPackBatchJobs.End(); itJob != endJobs; ++itJob )
			{
				ILoadJob* job = itJob->m_second;
				job->Cancel();
				job->Release();
			}
			m_loadingPackBatchJobs.Clear();

			SetLanguageInUserSettings( lang, m_speechLocale );
		}

		m_textLocale = lang;
		m_speechLocale = lang; // Because it is used only in editor, it is safe here to set speech language like same as text
		m_currentLocaleId = id;

		if ( GGame )
		{
			GGame->OnLanguageChange();
		}
	}
	else
	{
		ERR_ENGINE( TXT( "Cannot find locale '%ls'" ), lang.AsChar() );
	}

}

/*
Sets current locale.

\param lang Locale to be set.
\return True - locale successfully changed or lang denotes locale that is already current.
False - couldn't set locale (possible reason - some localized content is modified and needs to be saved first).
*/
Bool CLocalizationManager::SetCurrentLocale( const String& lang )
{
	if ( m_textLanguages.Empty() )
	{
		RED_WARNING( m_isDisabled, "SetCurrentLocale(): languages list is empty and localization manager is not disabled." );
		return false;
	}

	if( lang == m_textLocale )
	{
		if( m_isConnected )
		{
			// TODO: this will discard new strings and modifications to existing strings - do we really want this?
			SynchronizeWithStringDatabase();
		}

		return true;
	}

	if ( SLocalizationManager::GetInstance().IsAnyContentModified() )
	{
		GFeedback->ShowMsg( TXT( "Locale not changed" ), TXT( "Locale was not changed as some localized content is modified. Please save it before changing locale." ) );
		return false;
	}

	SLocalizationManager::GetInstance().SetCurrentLocaleForce( lang );
	return true;
}

const String & CLocalizationManager::GetCurrentLocale()
{
	return GetTextLocale();
}

void CLocalizationManager::SetSpeechLocale( const String &lang )
{
	m_speechLocale = lang;
}

const String & CLocalizationManager::GetSpeechLocale()
{
	return m_speechLocale;
}

const Uint32 CLocalizationManager::GetCurrentLocaleId() const
{
	return m_currentLocaleId;
}

const String& CLocalizationManager::GetTextLocale() const
{
	return m_textLocale;
}

void CLocalizationManager::GetLanguageAndDefaultRegionCodes( const Char** languageCode, const Char** defaultRegionCode ) const
{
	for ( const auto& codes : LANGUAGE_CODES )
	{
		if ( m_textLocale.EqualsNC( codes.m_name ) )
		{
			if ( languageCode )
			{
				*languageCode = codes.m_languageCode;
			}

			if ( defaultRegionCode )
			{
				*defaultRegionCode = codes.m_defaultRegionCode;
			}

			return;
		}
	}

	ASSERT( false, TXT("Unsupported language, please add it to LANGUAGE_CODES. Defaulting to en-US.") );

	if ( languageCode )
	{
		*languageCode = TXT("en");
	}

	if ( defaultRegionCode )
	{
		*defaultRegionCode = TXT("US");
	}
}

Bool CLocalizationManager::IsAnyContentModified()
{
	return !m_modifiedStrings.Empty() || !m_copiedStringIds.Empty();
}

void CLocalizationManager::SynchronizeWithStringDatabase()
{
	if ( !OpenSomeDataAccessIfNeeded( false ) ) return;

	m_modifiedStrings.Clear();
	m_copiedStringIds.Clear();
}

String CLocalizationManager::GetLocalizedText( Uint32 stringId, const String& locale, Bool* isFallback )
{
	if( m_dataAccessObject == NULL )
	{
		if( !OpenSomeDataAccess( true ) )
		{
			ERR_ENGINE( TXT( "stringDB access object not available" ) );
			return TXT("");
		}
	}

	//LanguagePack* pack = GetLanguagePackSync( stringId, true, locale );

	return m_dataAccessObject->GetLocalizedText( stringId, locale, isFallback );
}

String CLocalizationManager::GetLocalizedText( Uint32 stringId )
{
	if( m_dataAccessObject == NULL )
	{
		if( !OpenSomeDataAccess( true ) )
		{
			ERR_ENGINE( TXT( "stringDB access object not available" ) );
			return TXT("");
		}
	}

	// 10.01.2010 mcinek - changed to reading all packs.
	// This is for loading all packs to cache and
	// to prevent errors witch not-complete packs in cache
	LanguagePack* pack = GetLanguagePackSync( stringId, !GIsEditor );

	// Extract string from the string pack
	if ( pack )
	{
		return pack->GetText();
	}
	else
	{
		return String::EMPTY;
	}
}

//////////////////////////////////////////////////////////////////////////
// Data access creation methods



bool CLocalizationManager::OpenSQLDataAccessInternal()
{
#ifndef NO_STRING_DB
	if ( m_isDisabled ) 
	{
		return true;
	}

	if ( m_dataAccessObject )
	{
		return true;
	}

	ASSERT ( !m_dataAccessObject );

	m_dataAccessObject = new CSQLStringDBDataAccess();

	if( m_dataAccessObject )
	{
		m_dataAccessObject->ReadLanguageList( m_languagesIds );

		m_textLanguages.Clear();
		m_speechLanguages.Clear();

		TDynArray< TPair< String, Uint32 > > arrayMap;
		arrayMap.Reserve( m_languagesIds.Size() );
		for ( THashMap< String, Uint32 >::iterator langIter = m_languagesIds.Begin(); langIter != m_languagesIds.End(); ++langIter )
		{
			arrayMap.PushBack( *langIter );
		}

		struct LanguageListSortPredicate
		{
			Bool operator()( const TPair< String, Uint32 >& a, const TPair< String, Uint32 >& b ) const
			{
				return a.m_second < b.m_second;
			}
		} sortPredicate;
		::Sort( arrayMap.Begin(), arrayMap.End(), sortPredicate );

		for ( TDynArray< TPair< String, Uint32 > >::iterator langIter = arrayMap.Begin(); langIter != arrayMap.End(); ++langIter )
		{
			m_textLanguages.PushBack( langIter->m_first );
			if ( m_textLocale == langIter->m_first )
			{
				m_currentLocaleId = langIter->m_second;
			}
		}

		m_speechLanguages = m_textLanguages;
		m_isConnected = true;
		return true;
	}
#endif // #ifndef NO_STRING_DB
	return false;
}

bool CLocalizationManager::OpenCookedDataAccessInternal( const String &lang, const String& speech )
{
	if ( m_isDisabled ) 
	{
		return true;
	}

	ASSERT ( !m_dataAccessObject );
	
	extern IStringDBDataAccess* CreateCookedSDBAccess( const String&, const String& );
	m_dataAccessObject = CreateCookedSDBAccess( lang, speech );

	if( m_dataAccessObject )
	{
		return true;
	}

	return false;
}

bool CLocalizationManager::OpenNullDataAccess()
{
	ASSERT ( !m_dataAccessObject );

	m_dataAccessObject = new CNullStringDBDataAccess();

	m_currentLocaleId = 0;
	m_isConnected = false;
	return true;
}

bool CLocalizationManager::OpenSomeDataAccessIfNeeded( Bool writableOrSynchronized /* = true */ )
{
	if( m_dataAccessObject == NULL )
	{
		if( !OpenSomeDataAccess( writableOrSynchronized ) )
		{
			return false;
		}
	}

	return true;
}

bool CLocalizationManager::OpenSQLDataAccess( Bool writableOrSynchronized /*= false*/, Bool silent /*= false */ )
{
	if( writableOrSynchronized && !GFeedback->IsNullFeedback() && silent == false )
	{
		EFeedbackYesNoCancelResult res = GFeedback->AskYesNoCancel( TXT("Do you want to connect to remote strings database to synchronize or edit strings?") );
		if( res == FeedbackCancel )
		{
			m_neverConnect = true;
			return false;
		}
		if( res == FeedbackNo )
		{
			return false;
		}
	}

	// do not call OpenSomeDataAccess when connection is already opened
	if( m_isConnected )
	{
		return true;
	}

	if( m_dataAccessObject != NULL )
	{
		delete m_dataAccessObject;
		m_dataAccessObject = NULL;
	}

	return OpenSQLDataAccessInternal();
}

bool CLocalizationManager::OpenSomeDataAccess( Bool writableOrSynchronized /*= false*/, Bool silent /*= false */ )
{
	if ( m_textLanguages.Size() > 0 && ( !writableOrSynchronized || m_neverConnect ) )
	{
		if( m_dataAccessObject != NULL )
		{
			delete m_dataAccessObject;
			m_dataAccessObject = NULL;
		}

		// let's open local cached dao
		return OpenCookedDataAccessInternal( m_textLocale, m_speechLocale );;
	}
	else
	{
		return OpenSQLDataAccess( writableOrSynchronized, silent );
	}
}

//////////////////////////////////////////////////////////////////////////
// Cooker methods

void CLocalizationManager::GetPacksToCook( Uint32 locale, const String& stringsView, THashMap< Uint32, LanguagePack* >& packs )
{
	if( m_isConnected == false )
	{
		if( OpenSomeDataAccess( true ) == false )
		{
			return;
		}
	}

	m_dataAccessObject->ReadLanguagePacksForCook( locale, stringsView, packs );
}

//////////////////////////////////////////////////////////////////////////
// Language selection methods

void CLocalizationManager::ListLanguagesSupported()
{
	m_textLanguages.Clear();
	m_speechLanguages.Clear();

	m_textLanguages.PushBack( TXT( "PL" ) );
	m_textLanguages.PushBack( TXT( "EN" ) );
	m_textLanguages.PushBack( TXT( "DE" ) );
	m_textLanguages.PushBack( TXT( "FR" ) );
	m_textLanguages.PushBack( TXT( "IT" ) );
	m_textLanguages.PushBack( TXT( "ES" ) );
	m_textLanguages.PushBack( TXT( "BR" ) );
	m_textLanguages.PushBack( TXT( "MX" ) );
	m_textLanguages.PushBack( TXT( "RU" ) );
	m_textLanguages.PushBack( TXT( "CZ" ) );	
	m_textLanguages.PushBack( TXT( "HU" ) );
	m_textLanguages.PushBack( TXT( "ZH" ) );
	m_textLanguages.PushBack( TXT( "CN" ) );
	m_textLanguages.PushBack( TXT( "JP" ) );
	m_textLanguages.PushBack( TXT( "KR" ) );
	m_textLanguages.PushBack( TXT( "AR" ) );

	m_speechLanguages = m_textLanguages;
}

void CLocalizationManager::ValidateLanguageSelection( String& textLang, String& speechLang )
{
	if ( m_textLanguages.Empty() == false && m_textLanguages.Exist( textLang ) == false )
	{
		// If selected language is not available use default
		textLang = m_defaultLanguage;	
	}

	if ( m_speechLanguages.Empty() == false && m_speechLanguages.Exist( speechLang ) == false )
	{
		// If selected speech language is not available try using the same language as text
		speechLang = textLang;
		if ( m_speechLanguages.Exist( speechLang ) == false )
		{
			// If selected language is not available use default
			speechLang = m_defaultLanguage;
		}
	}
}

void CLocalizationManager::SwitchGameLanguage( const String& newAudioLang, const String& newSubtitleLang )
{
	//RED_FATAL_ASSERT
	RED_ASSERT( !GGame || !GGame->IsActive(), TXT("Can't switch languages while a game is active! Fix this!") );

	if ( m_localizationStorage.GetLockedLipsyncsCount() != 0 )
	{
		LOG_ENGINE( TXT( "SWITCHING GAME LANGUAGE NOT AVAILABLE NOW" ) );
		//return;
	}

	if ( newAudioLang == m_speechLocale && newSubtitleLang == m_textLocale )
	{
		return;
	}


	// Maybe async loader...
	//GGame->ToggleLoadingOverlay( true, TXT("Switching game language") );

	/*LOG_ENGINE( TXT( "SWITCHING GAME LANGUAGE - Text : %s->%s; Speech %s->%s" ), 
	m_textLocale.AsChar(), newSubtitleLang.AsChar(), m_speechLocale.AsChar(), newAudioLang.AsChar() );*/


	// Cleanup current language
	m_localizationStorage.ClearCacheMemory();
	for( THashMap< Uint32, CJobLoadLangPack* >::iterator jobIter = m_loadingPackJobs.Begin();
		jobIter != m_loadingPackJobs.End(); ++jobIter )
	{
		ILoadJob* job = jobIter->m_second;
		job->Cancel();
		job->Release();
	}
	m_loadingPackJobs.Clear();
	for( THashMap< Uint64, CJobLoadLangPackBatch* >::iterator jobIter = m_loadingPackBatchJobs.Begin();
		jobIter != m_loadingPackBatchJobs.End(); ++jobIter )
	{
		ILoadJob* job = jobIter->m_second;
		job->Cancel();
		job->Release();
	}
	m_loadingPackBatchJobs.Clear();

	// FIXME: There's so much win in how we could read in the offsets by changing the data format!
	// We can't sit here blocking because of PLM, so good-bye loading job and waiting for it.

	// Switch DAO

	// Would still be a race if we delete the dataAccessObject
	SJobManager::GetInstance().FlushPendingJobs();
	
	if ( m_dataAccessObject )
	{
		delete m_dataAccessObject;
		m_dataAccessObject = NULL;
	}
	extern IStringDBDataAccess* CreateCookedSDBAccess( const String&, const String& );
	m_dataAccessObject = CreateCookedSDBAccess( newSubtitleLang, newAudioLang );

	m_textLocale = newSubtitleLang;
	m_speechLocale = newAudioLang;
	FindLocaleId( m_textLocale, m_currentLocaleId );

	SetLanguageInUserSettings( m_textLocale, m_speechLocale );
	GContentManager->SetSpeechInstallLanguage( m_speechLocale );

	// Wait till everything is loaded. At this point we can't proceed without strings.
	SJobManager::GetInstance().FlushPendingJobs();

	if ( GGame )
	{
		GGame->OnLanguageChange();
	}

#if 0 // GFx 3
	SLoadSaveIndicator::GetInstance().HideLoading();
#endif
}

void CLocalizationManager::SwitchGameLanguageByIndex( Int32 newAudioLang, Int32 newSubtitleLang )
{
	String newAudioLangName = m_speechLocale;
	String newSubtitleLangName = m_textLocale;

	if ( newAudioLang >= 0 && newAudioLang < (Int32) m_speechLanguages.Size() )
	{
		newAudioLangName = m_speechLanguages[ newAudioLang ];
	}
	if ( newSubtitleLang >= 0 && newSubtitleLang < (Int32) m_textLanguages.Size() )
	{
		newSubtitleLangName = m_textLanguages[ newSubtitleLang ];
	}

	SwitchGameLanguage( newAudioLangName, newSubtitleLangName );
}

//////////////////////////////////////////////////////////////////////////
// Language retrieval methods

void CLocalizationManager::GetGameLanguageId( Int32 &audioLang /* out */, Int32 &subtitleLang /* out */ )
{
	Uint32 subLocaleNum = 0;
	Uint32 audioLocaleNum = 0;

	const String &subLocale = GetTextLocale();
	FindLocaleId( GetTextLocale(), subLocaleNum );

	const String &speechLocale = GetSpeechLocale();
	if ( speechLocale != String::EMPTY )
	{
		FindLocaleId( speechLocale, audioLocaleNum );
	}
	else
	{
		audioLocaleNum = subLocaleNum;
	}

	// We don't have all audio, so for audio other than Polish, English, and so on, use English audio
	//if ( audioLocaleNum!=LN_PL && audioLocaleNum!=LN_EN && audioLocaleNum!=LN_DE && audioLocaleNum!=LN_FR && audioLocaleNum!=LN_RU )
	//{
	//	audioLocaleNum = LN_EN;
	//}

	audioLang = (Int32)audioLocaleNum;
	subtitleLang = (Int32)subLocaleNum;
}

void CLocalizationManager::GetGameLanguageName( String& audioLang /* out */, String& subtitleLang /* out */ )
{
	audioLang = m_speechLocale;
	subtitleLang = m_textLocale;
}

void CLocalizationManager::GetGameLanguageIndex( Int32& audioLang /* out */, Int32& subtitleLang /* out */ )
{
	audioLang = static_cast< Int32 >( m_speechLanguages.GetIndex( m_speechLocale ) );
	subtitleLang = static_cast< Int32 >( m_textLanguages.GetIndex( m_textLocale ) );
}

void CLocalizationManager::GetAllAvailableLanguages( TDynArray< String >& textLanguages, TDynArray< String >& speechLanguages ) const
{
	textLanguages = m_textLanguages;
	speechLanguages = m_speechLanguages;
}

Bool CLocalizationManager::IsTextLanguageAvailable( const String& language ) const
{
	return m_textLanguages.Exist( language );
}

Bool CLocalizationManager::IsSpeechLanguageAvailable( const String& language ) const
{
	return m_speechLanguages.Exist( language );
}

void CLocalizationManager::ReloadLanguageFromUserSettings()
{
	String newTextLocale;
	String newSpeechLocale;

	GetLanguageFromUserSettings( newTextLocale, newSpeechLocale );

	// Save the requested languages from the config, then the DLC manager will try to restore them if and when the DLC becomes available
	Config::cvRequestedTextLanguage.Set( newTextLocale );
	Config::cvRequestedSpeechLanguage.Set( newSpeechLocale );

	if ( newTextLocale.Empty() || newSpeechLocale.Empty() )
	{
		GContentManager->GetValidDefaultGameLanguage( newTextLocale, newSpeechLocale );
		LOG_ENGINE(TXT("ReloadLanguageFromUserSettings: Using text=%ls and speech=%ls as chosen by the streaming installer based on the console language settings!"), newTextLocale.AsChar(), newSpeechLocale.AsChar() );
	}

	ValidateLanguageSelection( newTextLocale, newSpeechLocale );
	SwitchGameLanguage( newSpeechLocale, newTextLocale );
}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// 

void CLocalizationManager::MarkFallbackString( Uint32 stringId, Bool isFallback )
{
	if ( isFallback == true )
	{
		m_fallbackIds.Insert( stringId );
	}
	else
	{
		m_fallbackIds.Erase( stringId );
	}
}

Bool CLocalizationManager::IsMissingString( Uint32 stringId )
{
	return m_missingIds.Find( stringId ) != m_missingIds.End();
}

Bool CLocalizationManager::IsFallbackString( Uint32 stringId )
{
	return m_fallbackIds.Find( stringId ) != m_fallbackIds.End();
}

Bool CLocalizationManager::IsModifiedString( Uint32 stringId )
{
	return
		m_modifiedStrings.Find( stringId ) != m_modifiedStrings.End() ||
		m_copiedStringIds.Find( stringId ) != m_copiedStringIds.End();
}

String CLocalizationManager::GetString( Uint32 stringId, Bool getModified /*= true */ )
{
	if ( stringId == 0 )
	{
		return String::EMPTY;
	}

	String stringData;
	if ( getModified == true && m_modifiedStrings.Find( stringId, stringData ) == true )
	{
		return stringData;
	}

	return GetLocalizedText( stringId );
}

void CLocalizationManager::ModifyString( Uint32 stringId, const String& newString )
{
	m_modifiedStrings.Set( stringId, newString );
	m_localizationStorage.Remove( stringId );
}

/*
Creates new string as a copy of another string.

\param stringId Id of a string that is to be copied. May denote either stable or unstable string.
\return Id of created string copy. This string is unstable.
*/
Uint32 CLocalizationManager::CopyString( Uint32 stringId )
{
	Uint32 copyId = RegisterNewString();

	// store copied string in new/modified list and mark it as a copy
	m_modifiedStrings.Set( copyId, GetString( stringId ) );
	m_copiedStringIds.Set( copyId, stringId );

	return copyId;
}

void CLocalizationManager::DiscardStringModification( Uint32 stringId )
{
	if ( m_modifiedStrings.KeyExist( stringId ) == true )
	{
		VERIFY( m_modifiedStrings.Erase( stringId ) );
	}
}

Uint32 CLocalizationManager::RegisterNewString()
{
	m_newStringsCount += 1;
	return ( 1 << 31 ) + m_newStringsCount;
}

//////////////////////////////////////////////////////////////////////////
// Methods for accessing localized strings by keys

void CLocalizationManager::InitializeLocStringsWithKeysCache()
{
	OpenSomeDataAccessIfNeeded( false );

	// cannot connect to data base
	if ( m_dataAccessObject == NULL ) 
		return;

	// data base doesn't need cache
	if ( !m_dataAccessObject->ShouldCache() )
		return;

	if (  m_locStringsWithKeysManager == NULL )
	{
		m_locStringsWithKeysManager = new CLocalizedStringsWithKeysCache();
	}
	m_locStringsWithKeysManager->InitializeCache();
}

Uint32 CLocalizationManager::GetStringIdByStringKey( const String &stringKey )
{
	if ( !OpenSomeDataAccessIfNeeded() )
	{
		return 0;
	}

	if ( stringKey == String::EMPTY )
	{
		return 0;
	}

	return m_dataAccessObject->GetStringIdByStringKey( stringKey );
}

Bool CLocalizationManager::DoesStringKeyExist( const String &stringKey )
{
	if ( !OpenSomeDataAccessIfNeeded() )
	{
		return false;
	}

	if ( stringKey == String::EMPTY )
	{
		return false;
	}

	return m_dataAccessObject->DoesStringKeyExist( stringKey );
}

String CLocalizationManager::GetStringByStringKeyCached( const String &stringKey )
{
	if ( !OpenSomeDataAccessIfNeeded() )
	{
		return String::EMPTY;
	}

	// Use cache if it is available
	if ( m_dataAccessObject && m_dataAccessObject->ShouldCache() && false )
	{
		// Create cache if it hasn't been already created
		if (  m_locStringsWithKeysManager == NULL )
		{
			m_locStringsWithKeysManager = new CLocalizedStringsWithKeysCache();
			m_locStringsWithKeysManager->InitializeCache();
		}

		return m_locStringsWithKeysManager->GetCachedStringByKey( stringKey );
	}
	else
	{
		return GetStringByStringKey( stringKey );
	}
}

String CLocalizationManager::GetStringByStringKeyCachedWithFallback( const String &stringKey )
{
	String res = GetStringByStringKeyCached( stringKey );

	if ( res.Empty() )
	{
		RED_LOG( LocalizationManager, TXT( "Could not find string with key:\t%ls" ), stringKey.AsChar() );

		if( GetConfig_UsePlaceholdersForMissingStrings() )
		{
			#ifdef RED_FINAL_BUILD
				return stringKey;
			#else
				return TXT("#") + stringKey;
			#endif
		}
	}

	return res;
}

String CLocalizationManager::GetStringByStringKey( const String &stringKey )
{
	return GetStringByStringKey( stringKey, GetCurrentLocale() );
}

String CLocalizationManager::GetStringByStringKey( const String &stringKey, const String &locale )
{
	if ( !OpenSomeDataAccessIfNeeded() )
	{
		return String::EMPTY;
	}

	if ( stringKey == String::EMPTY )
	{
		return String::EMPTY;
	}

	return m_dataAccessObject->GetLocalizedTextByStringKey( stringKey, locale );
	//Uint32 stringId = m_dataAccessObject->GetStringIdByStringKey( stringKey );
	//return GetString( stringId, false );
}

void CLocalizationManager::ReadAllStringsWithStringKeys( TDynArray< Uint32 > &stringsIds /* out */, TDynArray< String > &stringsKeys /* out */, TDynArray< String > &stringsCategories /* out */ )
{
	if ( OpenSomeDataAccessIfNeeded() )
	{
		m_dataAccessObject->ReadAllStringsWithKeys( stringsIds, stringsKeys, stringsCategories );
	}
}

void CLocalizationManager::ReadAllStringsCategories( TDynArray< String > &stringsCategories /* out */, Bool OnlyKeys /* in */  )
{
	if ( OpenSomeDataAccessIfNeeded() )
	{
		m_dataAccessObject->ReadAllStringsCategories( stringsCategories, OnlyKeys );
	}
}

void CLocalizationManager::SearchForStringsByCategory( const String& searchQuery, const TDynArray< String > &categories /* in */, TDynArray< Uint32 > &ids /* out */, TDynArray< String >* keys /* out */, TDynArray< String >* strings /* out */, Bool searchKeys /* in */, ESearchOrder order /* in */ )
{
	if ( OpenSomeDataAccessIfNeeded() )
	{
		const Char* orderBy = NULL;

		switch( order )
		{
		case SearchOrder_Ids:
			orderBy = TXT( "STRING_ID" );
			break;

		case SearchOrder_Text:
			orderBy = TXT( "TEXT" );
			break;
		}

		m_dataAccessObject->SearchForLocalizedStrings( searchQuery, categories, ids, keys, strings, searchKeys, orderBy );
	}
}

bool CLocalizationManager::IsSQLConnectionValid()
{
	if(m_isConnected)
	{
		return m_dataAccessObject->IsSQLConnectionValid();
	}
	return false;
}

void CLocalizationManager::Reconnect()
{
	ASSERT( m_isConnected );
	m_dataAccessObject->Reconnect();
}

LanguagePack* CLocalizationManager::GetLanguagePackSync( Uint32 stringId, Bool onlyText /*= false */, const String& locale /* = String::EMPTY */ )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_packAccessMutex );

	LanguagePack* loadedPack = NULL;
	if ( locale.Empty() )
	{
		loadedPack = m_localizationStorage.GetLanguagePack( stringId );
	}

	if ( loadedPack != NULL )
	{
		return loadedPack;
	}


	// Cleanup if the same string is being loaded from job
	if ( m_loadingPackJobs.KeyExist( stringId ) == true )
	{
		CJobLoadLangPack* packJob = NULL;
		m_loadingPackJobs.Find( stringId, packJob );
		if ( packJob != NULL )
		{
			if ( packJob->HasFinishedWithoutErrors() == true && packJob->IsCanceled() == false )
			{
				loadedPack = packJob->GetLoadedLanguagePack();	
			}

			if ( loadedPack )
			{
				m_localizationStorage.Store( loadedPack, stringId );
			}
			else
			{
				packJob->Cancel();
			}
			packJob->Release();
			m_loadingPackJobs.Erase( stringId );
		}
	}



	if ( m_dataAccessObject != NULL && loadedPack == NULL )
	{
		Uint32 localeNum = GetCurrentLocaleId();
		if ( locale.Empty() == false )
		{
			FindLocaleId( locale, localeNum );
		}

		loadedPack = m_dataAccessObject->GetLanguagePack( stringId, onlyText, localeNum );
		if ( loadedPack != NULL )
		{
			if ( locale.Empty() )
			{
				m_localizationStorage.Store( loadedPack, stringId );
			}
		}
	}

	return loadedPack;
}

LanguagePack* CLocalizationManager::GetLanguagePackAsync( Uint32 stringId )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_packAccessMutex );

	LanguagePack* loadedPack = m_localizationStorage.GetLanguagePack( stringId );
	if ( loadedPack != NULL )
	{
		return loadedPack;
	}

	CJobLoadLangPack* packJob = NULL;
	if ( m_loadingPackJobs.KeyExist( stringId ) == true )
	{
		m_loadingPackJobs.Find( stringId, packJob );
		if ( packJob != NULL )
		{
			if ( packJob->HasFinishedWithoutErrors() == true && packJob->IsCanceled() == false )
			{
				loadedPack = packJob->GetLoadedLanguagePack();
				if ( loadedPack == NULL )
				{
					if ( !m_missingIds.Exist( stringId ) )
					{
						m_missingIds.Insert( stringId );
					}
				}
			}

			if ( loadedPack )
			{
				m_localizationStorage.Store( loadedPack, stringId );
			}

			if ( packJob->HasEnded() == true || packJob->IsCanceled() == true )
			{
				packJob->Release();
				m_loadingPackJobs.Erase( stringId );
			}
		}
	}
	else if ( m_dataAccessObject != NULL )
	{
		packJob = new CJobLoadLangPack( stringId, m_dataAccessObject, GetCurrentLocaleId() );
		SJobManager::GetInstance().Issue( packJob );
		m_loadingPackJobs.Insert( stringId, packJob );
	}

	return loadedPack;
}

/*
Note that this function will return true if all language packs are fully loaded, i.e. together with speeches.
*/
Bool CLocalizationManager::PreloadLanguagePacksAsync( const Uint64 batchId, const TDynArray< Uint32 >& stringIds )
{
	Bool isPreloadComplete = true;
	for ( Uint32 stringId : stringIds )
	{
		const LanguagePack* langPack = m_localizationStorage.GetLanguagePack( stringId );
		isPreloadComplete &= ( langPack? langPack->GetSpeechBuffer().IsLoaded() : false );
	}

	if ( isPreloadComplete == true )
	{
		return true;
	}

	CJobLoadLangPackBatch* batchJob = NULL;
	if ( m_loadingPackBatchJobs.Find( batchId, batchJob ) == true )
	{
		if ( batchJob->HasFinishedWithoutErrors() == true )
		{
			TDynArray< TPair< Uint32, LanguagePack* > > loadedPacks = batchJob->GetLoadedPacks();
			for ( TDynArray< TPair< Uint32, LanguagePack* > >::iterator packIter = loadedPacks.Begin();
				packIter != loadedPacks.End(); ++packIter )
			{
				m_localizationStorage.Store( packIter->m_second, packIter->m_first );
			}

			batchJob->Release();
			m_loadingPackBatchJobs.Erase( batchId );

			return true;
		}
		else if ( batchJob->IsCanceled() )
		{
			batchJob->Release();
			m_loadingPackBatchJobs.Erase( batchId );
			return true;
		}
	}
	else
	{
		batchJob = new CJobLoadLangPackBatch( stringIds, m_dataAccessObject, GetCurrentLocaleId() );
		SJobManager::GetInstance().Issue( batchJob );
		m_loadingPackBatchJobs.Insert( batchId, batchJob );
	}
	return false;
}

/*
Note that this function will return true if all language packs are fully loaded, i.e. together with speeches.
*/
Bool CLocalizationManager::PreloadLanguagePacksSync( const Uint64 batchId, const TDynArray< Uint32 >& stringIds )
{
	Bool isPreloadComplete = true;
	for ( Uint32 stringId : stringIds )
	{
		const LanguagePack* langPack = m_localizationStorage.GetLanguagePack( stringId );
		isPreloadComplete &= ( langPack? langPack->GetSpeechBuffer().IsLoaded() : false );
	}

	if ( isPreloadComplete == true )
	{
		return true;
	}

	CJobLoadLangPackBatch* batchJob = NULL;
	if ( m_loadingPackBatchJobs.Find( batchId, batchJob ) == true )
	{
		if ( batchJob->HasFinishedWithoutErrors() )
		{
			TDynArray< TPair< Uint32, LanguagePack* > > loadedPacks = batchJob->GetLoadedPacks();
			for ( TDynArray< TPair< Uint32, LanguagePack* > >::iterator packIter = loadedPacks.Begin(); packIter != loadedPacks.End(); ++packIter )
			{
				m_localizationStorage.Store( packIter->m_second, packIter->m_first );
			}

			batchJob->Release();
			m_loadingPackBatchJobs.Erase( batchId );

			return true;
		}
		else
		{
			batchJob->Cancel();
		}
	}

	for ( TDynArray< Uint32 >::const_iterator idIter = stringIds.Begin(); idIter != stringIds.End(); ++idIter )
	{
		GetLanguagePackSync( *idIter );
	}

	//isPreloadComplete = true;
	//for ( TDynArray< Uint32 >::const_iterator idIter = stringIds.Begin(); idIter != stringIds.End(); ++idIter )
	//{
	//	isPreloadComplete &= m_localizationStorage.IsPackStored( *idIter );
	//}
	//ASSERT( isPreloadComplete );

	return true;
}

void CLocalizationManager::ReleaseLanguagePack( Uint32 stringId )
{
	if ( m_loadingPackJobs.KeyExist( stringId ) == true )
	{
		CJobLoadLangPack* packJob = NULL;
		m_loadingPackJobs.Find( stringId, packJob );
		if ( packJob != NULL )
		{
			packJob->Cancel();
			packJob->Release();
			m_loadingPackJobs.Erase( stringId );
		}
	}

	m_localizationStorage.Remove( stringId );
}

LanguagePack* CLocalizationManager::CreateLanguagePack()
{
	m_createdPacks += 1;
	return new LanguagePack();
}

void CLocalizationManager::DeleteLanguagePack( LanguagePack* pack )
{
	if ( pack != &m_emptyPack )
	{
		m_createdPacks -= 1;
		delete pack;
	}
}

LanguagePack* CLocalizationManager::GetEmptyLanguagePack()
{
	return &m_emptyPack;
}

Bool CLocalizationManager::ParseCommandLine( )
{
	Bool textLanguageSet = false;
	Bool speechLanguageSet = false;

	CTokenizer tok( SGetCommandLine(), TXT(" ") );
	for( Uint32 i=0; i<tok.GetNumTokens(); ++i )
	{
		// All the string access is disabled
		if ( tok.GetToken( i ) == TXT( "-nostrings" ) )
		{
			m_isDisabled = true;
			LOG_ENGINE( TXT( "Strings DB is disabled. Do not save strings, as strings keys will be always 0 and you can damage resources."  ), m_textLocale.AsChar() );
			break;
		}else if( tok.GetToken( i ) == TXT("-useCookedLocale") )
		{
			m_neverConnect = true;
		}
		else if ( tok.GetToken( i ) == TXT("-lang") )
		{
			textLanguageSet = true;
			m_textLocale = tok.GetToken( i + 1 ).ToUpper();
		}
		else if ( tok.GetToken( i ) == TXT("-speech") )
		{
			speechLanguageSet = true;
			m_speechLocale = tok.GetToken( i + 1 ).ToUpper();
		}
	}

	if ( speechLanguageSet == false && textLanguageSet == true )
	{
		m_speechLocale = TXT( "EN" ); //English is the default for all languages without audio localization
	}else if ( speechLanguageSet == true && textLanguageSet == false )
	{
		m_textLocale = m_speechLocale;
	}

	if ( m_textLocale.EqualsNC(TXT("mx")))
	{
		m_textLocale = TXT("ESMX");
	}

	if ( m_speechLocale.EqualsNC(TXT("mx")))
	{
		m_speechLocale = TXT("ESMX");
	}

	return true;
}

void CLocalizationManager::GetLanguageFromUserSettings( String& textLang, String& speechLang )
{
	textLang = Config::cvTextLanguage.Get();
	speechLang = Config::cvSpeechLanguage.Get();
}

void CLocalizationManager::SetLanguageInUserSettings( const String& textLang, const String& speechLang )
{
	Config::cvTextLanguage.Set( textLang );
	Config::cvSpeechLanguage.Set( speechLang );
}

String CLocalizationManager::GetDateString( CSystemFile::CTime& fileTime ) const
{
	// Get data from CTime structure
	Int32 year, month, day, hour, minute, second;
	fileTime.Extract( year, month, day, hour, minute, second );

	String locale( m_textLocale.ToUpper() );

	if ( locale == TXT("PL") )
	{
		return String::Printf( TXT( "%02d/%02d/%d  %02d:%02d" ), day, month, year, hour, minute );
	}
	else if ( locale == TXT("EN") )
	{
		return String::Printf( TXT( "%d/%02d/%02d  %02d:%02d" ), year, month, day, hour, minute );
	}
	else if ( locale == TXT("DE") )
	{
		return String::Printf( TXT( "%02d/%02d/%d  %02d:%02d" ), day, month, year, hour, minute );
	}
	else if ( locale == TXT("IT") )
	{
		return String::Printf( TXT( "%02d/%02d/%d  %02d:%02d" ), day, month, year, hour, minute );
	}
	else if ( locale == TXT("FR") )
	{
		return String::Printf( TXT( "%02d/%02d/%d  %02d:%02d" ), day, month, year, hour, minute );
	}
	else if ( locale == TXT("CZ") )
	{
		return String::Printf( TXT( "%02d/%02d/%d  %02d:%02d" ), day, month, year, hour, minute );
	}
	else if ( locale == TXT("ES") )
	{
		return String::Printf( TXT( "%02d/%02d/%d  %02d:%02d" ), day, month, year, hour, minute );
	}
	else if ( locale == TXT("ZH") )
	{
		return String::Printf( TXT( "%02d/%02d/%d  %02d:%02d" ), month, day, year, hour, minute );
	}
	else if ( locale == TXT("RU") )
	{
		return String::Printf( TXT( "%02d/%02d/%d  %02d:%02d" ), day, month, year, hour, minute );
	}
	else if ( locale == TXT("HU") )
	{
		return String::Printf( TXT( "%d/%02d/%02d  %02d:%02d" ), year, month, day, hour, minute );
	}
	else if ( locale == TXT("JP") )
	{
		return String::Printf( TXT( "%d/%02d/%02d  %02d:%02d" ), year, month, day, hour, minute );
	}
	else
	{
		ASSERT( ! "GetDateString(): Invalid language" );
		return String::Printf( TXT( "%d/%02d/%02d  %02d:%02d" ), year, month, day, hour, minute );
	}
}

String CLocalizationManager::GetVoiceoverFilename( Uint32 stringId )
{
	if ( m_dataAccessObject != NULL )
	{
		return m_dataAccessObject->GetVOFilename( stringId );
	}
	return String::EMPTY;
}

Bool CLocalizationManager::GetCachedLanguagePackDuration( Uint32 stringId, Float& duration ) const
{
	duration = 1.f;
	return m_localizationStorage.GetCachedLanguagePackDuration( stringId, duration );
}

Bool CLocalizationManager::ValidateLanguagePackLoad( Uint32 stringId )
{
	LanguagePack* loadedPack = GetLanguagePackAsync( stringId );
	if ( loadedPack == NULL )
	{
		ERR_ENGINE(TXT("ValidateLanguagePackLoad: GetLanguagePackAsync(%u) failed!"), stringId );
		return false;
	}
	if ( loadedPack->GetSpeechBuffer().IsLoaded() == false )
	{
		Uint32 requiredSize = static_cast< Uint32 >( loadedPack->GetSpeechBuffer().GetSize() );
		m_localizationStorage.Remove( stringId, true );
		//m_localizationStorage.FreeSpeechMemory( requiredSize );
		//		GetLanguagePackAsync( stringId );
		ERR_ENGINE(TXT("ValidateLanguagePackLoad: speech buffer not loaded for stringID %u"), stringId );
		return false;
	}
	return true;
}

#ifndef NO_DEBUG_PAGES

void CLocalizationManager::FillDebugSummary( LanguagePackDebugSummary& summary )
{
	summary.m_usedSize = m_localizationStorage.GetUsedCacheSize();
	summary.m_maxSize = m_localizationStorage.GetMaxCacheSize();
	summary.m_createdPacks = m_createdPacks;
	summary.m_lockedLipsyncs = m_localizationStorage.GetLockedLipsyncsCount();
	m_localizationStorage.GetCachedPacks( summary.m_packsFromCache, summary.m_cacheStringIds );
}

#endif

Bool CLocalizationManager::GetConfig_UsePlaceholdersForMissingStrings() const
{
	return Config::cvUsePlaceholdersForMissingStrings.Get();
}

//////////////////////////////////////////////////////////////////////////

CJobLoadLangPack::CJobLoadLangPack( Uint32 stringId, IStringDBDataAccess* dataAccess, Uint32 locale )
	: ILoadJob( JP_Speech, true )
	, m_stringId( stringId )
	, m_dataAccess( dataAccess )
	, m_loadedPack( NULL )
	, m_locale( locale )
{
}

CJobLoadLangPack::~CJobLoadLangPack()
{
	if ( IsCanceled() == true && m_loadedPack != NULL )
	{
		SLocalizationManager::GetInstance().DeleteLanguagePack( m_loadedPack );
	}
}

EJobResult CJobLoadLangPack::Process()
{
	CTimeCounter langPackTimer;
	if ( m_dataAccess != NULL )
	{
		m_loadedPack = m_dataAccess->GetLanguagePack( m_stringId, false, m_locale );
	}
	//LOG_ENGINE( TXT( "Language pack '%d' loaded in %.2f" ), m_stringId, langPackTimer.GetTimePeriod() );
	return JR_Finished;
}

//////////////////////////////////////////////////////////////////////////

CJobLoadLangPackBatch::CJobLoadLangPackBatch( const TDynArray< Uint32 > stringIds, IStringDBDataAccess* dataAccess, Uint32 locale )
	: ILoadJob( JP_Speech, true )
	, m_dataAccess( dataAccess )
	, m_locale( locale )
	, m_stringIds( stringIds )
{
}

CJobLoadLangPackBatch::~CJobLoadLangPackBatch()
{
	if ( IsCanceled() == true && m_languagePacks.Empty() == false )
	{
		for ( TDynArray< TPair< Uint32, LanguagePack* > >::iterator packIter = m_languagePacks.Begin();
			packIter != m_languagePacks.End(); ++packIter )
		{
			SLocalizationManager::GetInstance().DeleteLanguagePack( packIter->m_second );
		}
		m_languagePacks.Clear();
	}
}

EJobResult CJobLoadLangPackBatch::Process()
{
	if ( m_dataAccess != NULL )
	{
		m_dataAccess->GetLanguagePackBatch( m_stringIds, m_locale, m_languagePacks );
	}
	return JR_Finished;
}

//////////////////////////////////////////////////////////////////////////
