/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "sqlStringDBDataAccess.h"
#include "localizationManager.h"
#include "../core/depot.h"
#include "../core/configFileManager.h"
#include "../core/feedback.h"
#include "commonConfigs.h"

#ifndef NO_STRING_DB

#include "../../../external/stringdb/sqlStringDB.h"

#ifdef _WIN64
#	pragma comment ( lib, "../../../external/stringdb/x64/Release/stringdb_x64.lib" )
#elif defined(W2_PLATFORM_WIN32)
#	pragma comment ( lib, "../../../external/stringdb/x86/Release/stringdb_x86.lib" )
#endif

const String CSQLStringDBDataAccess::SPEECH_DIRECTORY = TXT( "\\speech\\" );
const String CSQLStringDBDataAccess::VOICEOVER_DIRECTORY = TXT( "\\audio\\" );
const String CSQLStringDBDataAccess::VOICEOVER_EXTENSION = TXT( ".wav" );
const String CSQLStringDBDataAccess::LIPSYNC_DIRECTORY = TXT( "\\lipsync\\" );

// Ehh, Dex's hack, if you have time, fix this...
String GSQLStringDBUserName = TXT("Unknown");

CSQLStringDBDataAccess::CSQLStringDBDataAccess()
{
#if defined(W2_PLATFORM_WIN32)
	Connect();
#endif
}

CSQLStringDBDataAccess::~CSQLStringDBDataAccess()
{
#if defined(W2_PLATFORM_WIN32)
	ASSERT( m_SqlConnection != NULL );
	sqlStringDbClose( m_SqlConnection );
	m_SqlConnection = NULL;
#endif
}

void CSQLStringDBDataAccess::Read( LocalizedString *localizedString )
{
#if defined(W2_PLATFORM_WIN32)
	if( IsNewString( localizedString->GetIndex() ) == true )
	{
//		WARN_ENGINE( TXT( "Trying to read string with id 0" ) );
		return;
	}

	if( sqlStringIsInDb( m_SqlConnection , localizedString->GetIndex() ) < 1 )
	{
		GFeedback->ShowError( TXT("String %u not in db or connection broken"), localizedString->GetIndex() );
		ERR_ENGINE( TXT("String %u not in db or connection broken"), localizedString->GetIndex() );
		//ApplyStringDBId( 0, localizedString );
		//localizedString->SetModified( true );
		return;
	}
	
	Bool fallback;
	localizedString->SetString( GetLocalizedText( localizedString->GetIndex(), SLocalizationManager::GetInstance().GetCurrentLocaleId(), &fallback ), false );

	SLocalizationManager::GetInstance().MarkFallbackString( localizedString->GetIndex(), fallback );
	SLocalizationManager::GetInstance().DiscardStringModification( localizedString->GetIndex() );
#endif
}

/*
Updates string db by creating new strings and modifying existing ones, assumes current locale.

\param List of strings (along with their info) to be created/modified.

Note that this Update() overload is the only one that is able to create new strings in string db.
*/
void CSQLStringDBDataAccess::Update( TDynArray< LocalizedStringEntry >& stringEntries )
{
#if defined(W2_PLATFORM_WIN32)
	for ( Uint32 i = 0; i < stringEntries.Size(); ++i )
	{
		LocalizedStringEntry& stringEntry = stringEntries[ i ];
		LocalizedString *localizedString = stringEntry.m_localizedString;
		//ASSERT( localizedString != NULL );

		// If string wasn't modified and is already in db than no need to insert it into db
		if( !localizedString->IsModified() && ( sqlStringIsInDb( m_SqlConnection , localizedString->GetIndex() ) > 0 ) )
		{
			continue;
		}

		Uint32 oldStringIndex = localizedString->GetIndex();
		Uint32 newStringIndex = oldStringIndex;

		String str = SLocalizationManager::GetInstance().GetString( newStringIndex, true );
		str.Trim();

		// Table: STRING_INFO 
		if( IsNewString( newStringIndex ) == true )
		{
			newStringIndex = GetNextId();
			if ( newStringIndex == -1 )
			{
				GFeedback->ShowError( TXT( "Unable to generate unique id for string %u. Problems with data base connection?" ), oldStringIndex );
				ERR_ENGINE( TXT( "Unable to generate unique id for string %u. Problems with data base connection?" ), oldStringIndex );
				ApplyStringDBId( 0, localizedString );
				return;
			}
			SLocalizationManager::GetInstance().UpdateStringId(oldStringIndex, newStringIndex);
			
			// if someone else gets here until second statement is executed, we possibly could be in trouble...
			// is not probable (milliseconds and rare operation), but if occurs then it can be solved 
			// by sql transactions or sql statement that assigns new id on the fly (lots of double checks/code)

			String resourceName = stringEntry.m_parentResource ? stringEntry.m_parentResource->GetFriendlyName() : TXT("CUSTOM");
			
			String voiceoverName = stringEntry.m_voiceoverName;
			if ( voiceoverName.Empty() == false )
			{
				voiceoverName.Replace( CLocalizationManager::VOICEFILE_UNDEFINED_ID, String::Printf( TXT( "%08d" ), newStringIndex ) );
			}

			if
			(
				sqlStringInfoInsert
				(
					m_SqlConnection,
					newStringIndex, 
					resourceName.AsChar(), 
					stringEntry.m_propertyName.AsChar(),
					voiceoverName.AsChar(),
					stringEntry.m_stringKey.AsChar()
				) < 0
			)
			{
				GFeedback->ShowError( TXT( "Unable to insert string %u info into db. Assigning new id." ), newStringIndex );
				ERR_ENGINE( TXT( "Unable to insert string %u info into db. Assigning new id." ), newStringIndex );
				ApplyStringDBId( 0, localizedString );
				return;
			}
			ApplyStringDBId( newStringIndex, localizedString );
		}

		const Uint32 currentLocaleNum = SLocalizationManager::GetInstance().GetCurrentLocaleId();

		// Table: STRINGS
		if
		(
			sqlStringInsert
			(
				m_SqlConnection,
				newStringIndex,
				currentLocaleNum, 
				str.AsChar(),
				GSQLStringDBUserName.AsChar()
			) < 0
		)
		{
			ERR_ENGINE( TXT( "Unable to insert string %u into db (connection: %x, locale: %ld, string: '%ls', user: '%ls')" ), 
				newStringIndex, m_SqlConnection, currentLocaleNum, str.AsChar(), GSQLStringDBUserName.AsChar() );
		}

		// Clear modified flag
		localizedString->SetModified( false );
		SLocalizationManager::GetInstance().DiscardStringModification( newStringIndex );
	}
#endif
}

/*
Updates modified strings in string db, assumes current locale for all modifications.

\param strings List of strings to update. All strings must be stable (they must already exist in db).

For each modified string, new entry in string db is created. This entry gets next version number and
newest string text. Entry locale is set to current locale. Strings that are not modified are skipped.
*/
void CSQLStringDBDataAccess::Update( TDynArray< LocalizedString* > strings )
{
#if defined(W2_PLATFORM_WIN32)
	for ( Uint32 i = 0; i < strings.Size(); ++i )
	{
		LocalizedString *localizedString = strings[ i ];
		ASSERT( localizedString != nullptr );
		Update( *localizedString );
	}
#endif
}

/*
Commits string modifications to string db, assumes current locale.

\param localizedString String whose modifications are to be committed to string db. Must be stable, i.e. must already exist in string db.
\return True - string db updated or no update needed. False - couldn't update string db (connection down?).

If string is modified then new entry in string db is created. This entry gets next version number and
newest string text. Entry locale is set to current locale. Nothing is done if string is not modified.
*/
Bool CSQLStringDBDataAccess::Update( LocalizedString& localizedString )
{
#if defined(W2_PLATFORM_WIN32)

	Uint32 stringId = localizedString.GetIndex();

	// String must already exist in string db. This function is unable to create new string in string db.
	ASSERT( !IsNewString( stringId ) );

	if( localizedString.IsModified() )
	{
		String str = localizedString.GetString();
		str.Trim();

		const Uint32 currLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();

		Int32 queryResult = sqlStringInsert( m_SqlConnection, stringId, currLocaleId, str.AsChar(), GSQLStringDBUserName.AsChar() );
		if( queryResult < 0 )
		{
			GFeedback->ShowError( TXT( "Unable to insert string %u into db" ), stringId );
			ERR_ENGINE( TXT( "Unable to insert string %u into db" ), stringId );
			return false;
		}

		localizedString.SetModified( false );
	}

	return true;

#endif // W2_PLATFORM_WIN32
}

/*
Updates strings in string db.

\param strings List of strings to update. Maps string id to new string text. All strings must be stable (they must already exist in db).
\param lang Locale to use for all modifications.

For each string (even if it's not modified), new entry in string db is created. This entry gets next
version number and newest string text. Entry locale is set to locale specified as lang argument.
*/
void CSQLStringDBDataAccess::Update( const THashMap< Uint32, String > &strings, const String &lang )
{
#if defined(W2_PLATFORM_WIN32)

	Uint32 langNum;

	if( !SLocalizationManager::GetInstance().FindLocaleId( lang, langNum ) )
	{
		ERR_ENGINE( TXT( "Unable to find language %s" ), lang.AsChar() );
		return;
	}

	for ( THashMap< Uint32, String >::const_iterator it = strings.Begin(); it != strings.End(); ++it )
	{
		const String& localizedString = it->m_second;
		String str = localizedString;
		str.Trim();

		Uint32 stringIndex = it->m_first;
		ASSERT( IsNewString( stringIndex ) == false );

		if( sqlStringInsert( m_SqlConnection, stringIndex, langNum, 
			str.AsChar(), GSQLStringDBUserName.AsChar() ) < 0 )
		{
			GFeedback->ShowError( TXT( "Unable to insert string %u into db" ), stringIndex );
			ERR_ENGINE( TXT( "Unable to insert string %u into db" ), stringIndex );
		}
	}
#endif
}

void CSQLStringDBDataAccess::UpdateStringInfos( TDynArray< LocalizedStringEntry >& stringEntries )
{
#if defined(W2_PLATFORM_WIN32)
	for ( Uint32 i = 0; i < stringEntries.Size(); ++i )
	{
		Bool resultOk = UpdateStringInfo( stringEntries[ i ] );
		if( !resultOk )
		{
			Uint32 stringIndex = stringEntries[ i ].m_localizedString->GetIndex();
			GFeedback->ShowError( TXT( "Unable to update string %u info into db." ), stringIndex );
			ERR_ENGINE( TXT( "Unable to update string %u info into db." ), stringIndex );
		}
	}
#endif
}

/*
Creates string info entry in string db.

\param entry Contains entry data to be inserted. Must be valid, i.e. entry.m_localizedString must not be nullptr
and entry.m_localizedString->GetIndex() must be stable id (we don't allow unstable ids in string db).
\return True - string info inserted into string db, false - otherwise.
*/
Bool CSQLStringDBDataAccess::InsertStringInfo( const LocalizedStringEntry& entry ) const
{
#if defined(W2_PLATFORM_WIN32)

	ASSERT( entry.m_localizedString );
	// we don't allow unstable ids in string db
	ASSERT( !IsUnstableStringId( entry.m_localizedString->GetIndex() ) );

	String resName = entry.m_parentResource? entry.m_parentResource->GetFriendlyName() : TXT("CUSTOM");
	Uint32 strId = entry.m_localizedString->GetIndex();

	String voName = entry.m_voiceoverName;
	if ( !voName.Empty() )
	{
		// update voiceover name with string id
		voName.Replace( CLocalizationManager::VOICEFILE_UNDEFINED_ID, String::Printf( TXT( "%08d" ), strId ) );
	}

	// create string info entry in string db
	Int32 insertResult = sqlStringInfoInsert( m_SqlConnection, strId, resName.AsChar(), entry.m_propertyName.AsChar(), voName.AsChar(), entry.m_stringKey.AsChar() );
	if( insertResult < 0 )
	{
		return false;
	}

	return true;

#endif // W2_PLATFORM_WIN32
}

/*
Updates string info entry in string db.

\param entry Contains entry data to be inserted. Must be valid, i.e. entry.m_localizedString must not be nullptr
and entry.m_localizedString->GetIndex() must be stable id (we don't allow unstable ids in string db).
\return True - string info updated in string db, false - otherwise.
*/
Bool CSQLStringDBDataAccess::UpdateStringInfo( const LocalizedStringEntry& entry ) const
{
#if defined(W2_PLATFORM_WIN32)

	ASSERT( entry.m_localizedString );
	// we don't allow unstable ids in string db
	ASSERT( !IsUnstableStringId( entry.m_localizedString->GetIndex() ) );

	String resName = entry.m_parentResource? entry.m_parentResource->GetFriendlyName() : TXT("CUSTOM");
	Uint32 strId = entry.m_localizedString->GetIndex();

	String voName = entry.m_voiceoverName;
	if ( !voName.Empty() )
	{
		// update voiceover name with string id
		voName.Replace( CLocalizationManager::VOICEFILE_UNDEFINED_ID, String::Printf( TXT( "%08d" ), strId ) );
	}

	// update string info entry in string db
	Int32 updateResult = sqlStringInfoUpdate( m_SqlConnection, strId, resName.AsChar(), entry.m_propertyName.AsChar(), voName.AsChar(), entry.m_stringKey.AsChar() );
	if( updateResult < 0 )
	{
		return false;
	}

	return true;

#endif // W2_PLATFORM_WIN32
}

void CSQLStringDBDataAccess::ReadLanguageList( THashMap< String, Uint32 >& availableLanguages )
{
#if defined(W2_PLATFORM_WIN32)
	sqlLanguagesStruct *langs = sqlStringReadLanguages( m_SqlConnection );
	ASSERT( langs );

	if( !langs )
	{
		return;
	}

	availableLanguages.Clear();

	for( Uint32 i = 0; i < langs->languageNum; ++i )
	{
		ASSERT( langs->languages[i] );
		availableLanguages.Insert( langs->languages[ i ], langs->langIds[ i ] );
	}

	sqlStringFreeLanguagesResult( langs );
#endif
}


void CSQLStringDBDataAccess::ReadAllStringsWithKeys( TDynArray< Uint32 > &stringsIds /* out */, TDynArray< String > &stringsKeys /* out */, TDynArray< String > &stringsCategories /* out */ )
{
#if defined(W2_PLATFORM_WIN32)
	sqlReadAllStringsWithStringKeyStruct* readResult = sqlReadAllStringsWithStringKey( m_SqlConnection );

	if ( readResult != NULL )
	{
		for ( Uint32 i = 0; i < readResult->packsCount; ++i )
		{
			Uint32 id = readResult->ids[ i ];
			String stringKey( String::EMPTY );
			String stringCategory( String::EMPTY );

			if ( readResult->categories[ i ] != NULL )
			{
				stringCategory = String( readResult->categories[ i ] );
				stringCategory.Trim();
			}

			if ( readResult->stringKeys[ i ] != NULL )
			{
				stringKey = String( readResult->stringKeys[ i ] );
				stringKey.Trim();
			}

			stringsIds.PushBack( id );
			stringsKeys.PushBack( stringKey );
			stringsCategories.PushBack( stringCategory );
		}

		sqlFreeReadAllStringsWithStringKeyResult( readResult );
	}
#endif
}

void CSQLStringDBDataAccess::ReadAllStringsCategories( TDynArray< String > &stringsCategories /* out */, Bool keysOnly  )
{
#if defined(W2_PLATFORM_WIN32)
	sqlReadAllCategoriesStruct* readResult = sqlReadAllCategories( m_SqlConnection, keysOnly );

	if ( readResult != NULL )
	{
		for ( Uint32 i = 0; i < readResult->packsCount; ++i )
		{
			String stringCategory( String::EMPTY );

			if ( readResult->categories[ i ] != NULL )
			{
				stringCategory = String( readResult->categories[ i ] );
				stringCategory.Trim();
			}

			stringsCategories.PushBack( stringCategory );
		}

		sqlFreeReadAllCategoriesResult( readResult );
	}
#endif
}

String CSQLStringDBDataAccess::GetLocalizedText( Uint32 stringId, Uint32 locale, Bool *fallback /* = NULL */ )
{
#if defined(W2_PLATFORM_WIN32)
	sqlReadStruct *readStr = sqlStringRead( m_SqlConnection, stringId, locale, true );

	if( readStr )
	{
		String ret( readStr->text );

		if( fallback != NULL )
		{
			*fallback = readStr->isFallback;
		}

		sqlStringFreeReadResult( readStr );

		return ret;
	}
	else
	{
		if ( IsNewString( stringId ) == false && sqlStringIsInDb( m_SqlConnection , stringId ) < 1 )
		{
			LOG_ENGINE( TXT( "Unable to read string %u" ), stringId );
			return TXT("");
		}
		else
		{
			return TXT("");
		}
	}
#else
	return TXT("");
#endif
}


String CSQLStringDBDataAccess::GetLocalizedText( Uint32 stringId, const String& locale, Bool *fallback /*= NULL*/ )
{
#if defined(W2_PLATFORM_WIN32)
	Uint32 localeNum; 
	VERIFY ( SLocalizationManager::GetInstance().FindLocaleId( locale, localeNum ) );

	return GetLocalizedText( stringId, localeNum, fallback );
#else
	return String();
#endif
}

String CSQLStringDBDataAccess::GetLocalizedTextByStringKey( const String &stringKey, const String& locale, Bool *fallback /*= NULL */ )
{
#if defined(W2_PLATFORM_WIN32)
	Uint32 localeNum; 
	VERIFY ( SLocalizationManager::GetInstance().FindLocaleId( locale, localeNum ) );	

	return GetLocalizedTextByStringKey( stringKey, localeNum, fallback );
#else
	return String();
#endif
}

String CSQLStringDBDataAccess::GetLocalizedTextByStringKey( const String &stringKey, Uint32 locale, Bool *fallback /*= NULL */ )
{
#if defined(W2_PLATFORM_WIN32)
	Uint32 stringId = 0;

	// Get string ID
	sqlStringIdStruct *readStr = sqlStringReadId( m_SqlConnection, stringKey.AsChar() );
	if( readStr )
	{
		stringId = readStr->stringId;
		sqlStringFreeReadId( readStr );
	}

	// Get string value
	if ( stringId != 0 )
	{
		return GetLocalizedText( stringId, locale, fallback );
	}
	else
	{
		return String::EMPTY;
	}
#else
	return String::EMPTY;
#endif
}

void CSQLStringDBDataAccess::ReadLanguagePack( Uint32 stringId, LanguagePack& pack, Bool immediate, Bool& textLoaded, Bool& voiceLoaded )
{
#if defined(W2_PLATFORM_WIN32) && ! defined( NO_EDITOR )
	sqlReadLanguagePackStruct* readResult = sqlReadLanguagePack( 
		m_SqlConnection, stringId, SLocalizationManager::GetInstance().GetCurrentLocaleId(), true );

	if ( readResult != NULL )
	{
		pack.SetText( String( readResult->text ).TrimCopy() );
		pack.SetVoiceoverFileName( String( readResult->voiceoverName ).TrimCopy() );
		pack.SetLipsyncFileName( String( readResult->lipsyncName ).TrimCopy() );

		sqlStringFreeReadLanguagePackResult( readResult );

		textLoaded = true;
		voiceLoaded = FillLanguagePackResources( pack, SLocalizationManager::GetInstance().GetCurrentLocaleId() );
		return;
	}

	textLoaded = false;
	voiceLoaded = false;
#endif
}

LanguagePack* CSQLStringDBDataAccess::GetLanguagePack( Uint32 stringId, Bool textOnly, Uint32 locale )
{
	LanguagePack* pack = NULL;

#if defined(W2_PLATFORM_WIN32) && ! defined( NO_EDITOR )
	sqlReadLanguagePackStruct* readResult = sqlReadLanguagePack( 
		m_SqlConnection, stringId, locale, true );

	if ( readResult != NULL )
	{
		pack = SLocalizationManager::GetInstance().CreateLanguagePack();

		pack->SetText( String( readResult->text ).TrimCopy() );
		pack->SetVoiceoverFileName( String( readResult->voiceoverName ).TrimCopy() );
		pack->SetLipsyncFileName( String( readResult->lipsyncName ).TrimCopy() );

		sqlStringFreeReadLanguagePackResult( readResult );

		if ( textOnly == false )
		{
			FillLanguagePackResources( *pack, locale );
		}
	}

#endif

	return pack;
}

Bool CSQLStringDBDataAccess::ReadLanguagePackByStringKey( const String &stringKey, LanguagePack& pack, Bool immediate /* = false */ )
{
#if defined(W2_PLATFORM_WIN32)
	Uint32 stringId = GetStringIdByStringKey( stringKey );

	if ( stringId != 0 )
	{
		Bool textLoaded, voiceLoaded;
		ReadLanguagePack( stringId, pack, false, textLoaded, voiceLoaded );
		return textLoaded;
	}
	return false;
#else
	return false;
#endif
}

void CSQLStringDBDataAccess::ReadLanguagePacksForCook( Uint32 locale, const String& stringsView, THashMap< Uint32, LanguagePack* >& packs )
{
#if defined(W2_PLATFORM_WIN32) && ! defined( NO_EDITOR )
	String viewForCook = stringsView;
	if ( viewForCook.Empty() == true )
	{
		viewForCook = TXT( "STRINGS_VIEW" );
	}

	sqlReadAllLanguagePacksStruct* readResult = sqlReadAllLanguagePacks( m_SqlConnection, locale, viewForCook.AsChar() );

	if ( readResult != NULL )
	{
		for ( Uint32 i = 0; i < readResult->packsCount; ++i )
		{
			Uint32 id = readResult->ids[ i ];

			LanguagePack* pack = SLocalizationManager::GetInstance().CreateLanguagePack();
			if ( readResult->texts[ i ] != NULL )
			{
				pack->SetText( String( readResult->texts[ i ] ).TrimCopy() );
			}

			if ( readResult->voiceovers[ i ] != NULL )
			{
				pack->SetVoiceoverFileName( String( readResult->voiceovers[ i ] ).TrimCopy() );
			}

			if ( readResult->lipsyncs[ i ] != NULL )
			{
				pack->SetLipsyncFileName( String( readResult->lipsyncs[ i ] ).TrimCopy() );
			}

			if ( readResult->stringKeys[ i ] != NULL )
			{
				pack->SetStringKey( String( readResult->stringKeys[ i ] ).TrimCopy() );
			}

			//FillLanguagePackResources( pack );

			packs.Set( id, pack );
		}

		sqlFreeReadAllLanguagePacksResult( readResult );
	}
#endif
}

Bool CSQLStringDBDataAccess::DoesStringKeyExist( const String &stringKey )
{
#if defined(W2_PLATFORM_WIN32)
	Uint32 stringId = 0;

	// Get string ID
	sqlStringIdStruct *readStr = sqlStringReadId( m_SqlConnection, stringKey.AsChar() );
	if ( readStr )
	{
		stringId = readStr->stringId;
		sqlStringFreeReadId( readStr );
	}

	if ( stringId != 0 )
	{
		return true;
	}
	else
	{
		return false;
	}
#else
	return false;
#endif
}

Uint32 CSQLStringDBDataAccess::GetStringIdByStringKey( const String &stringKey )
{
	Uint32 stringId = 0;

#if defined(W2_PLATFORM_WIN32)
	// Get string ID
	sqlStringIdStruct *readStr = sqlStringReadId( m_SqlConnection, stringKey.AsChar() );
	if( readStr )
	{
		stringId = readStr->stringId;
		sqlStringFreeReadId( readStr );
	}
#endif

	return stringId;
}

String CSQLStringDBDataAccess::GetStringKeyById( Uint32 stringId )
{
	String stringKey = String::EMPTY;

#if defined(W2_PLATFORM_WIN32)
	// Get string key
	sqlStringKeyStruct *readStr = sqlStringReadStringKey( m_SqlConnection, stringId );
	if( readStr )
	{
		stringKey = String( readStr->stringKey );
		stringKey.Trim();
		sqlStringFreeReadStringKey( readStr );
	}
#endif
	return stringKey;
}

void CSQLStringDBDataAccess::Connect()
{
#if defined(W2_PLATFORM_WIN32)
	String databaseAddress = TXT("CDPRS-MSSQL\\sqlexpress");	// default address for Warsaw
	String databaseName = TXT("EditorStringDataBaseW3");		// default name for Witcher
	databaseAddress = Config::cvDatabaseAddress.Get();
	databaseName = Config::cvDatabaseName.Get();
	m_SqlConnection = sqlStringDbInit( databaseAddress.AsChar(), databaseName.AsChar() );
	ASSERT( m_SqlConnection != NULL );
#endif
}

void CSQLStringDBDataAccess::Reconnect()
{
#if defined(W2_PLATFORM_WIN32)
	ASSERT( m_SqlConnection != NULL );
	sqlStringDbClose( m_SqlConnection );
	Connect();
#endif
}

Uint32 CSQLStringDBDataAccess::GetNextId()
{
	Uint32 id( 0 );
#if defined(W2_PLATFORM_WIN32)
	Int32 result = sqlStringGetNextID( m_SqlConnection, &id );
#endif
	return id;
}

Bool CSQLStringDBDataAccess::IsNewString( Uint32 stringId )
{
	return stringId >= ( 1 << 31 ) || stringId == 0;
}

namespace
{
	String GetLipsPath( const String& stringId, const String& languageId )
	{
		String outPath;

		GDepot->GetAbsolutePath( outPath );

		outPath += TXT("speech\\");

		outPath += languageId + TXT("\\lipsync\\") + stringId + TXT(".re");

		return outPath;
	}

	String GetVOPath( const String& stringId, const String& languageId, const String& extension )
	{
		String outPath;

		GDepot->GetAbsolutePath( outPath );

		outPath += TXT("speech\\");

		outPath += languageId + TXT("\\audio\\") + stringId + extension;

		return outPath;
	}
}

Bool CSQLStringDBDataAccess::FillLanguagePackResources( LanguagePack &pack, Uint32 localeId )
{
#if defined(W2_PLATFORM_WIN32) && !defined( NO_EDITOR )
	if ( SLocalizationManager::GetInstance().ShouldIgnoreLanguagePackResources() == true )
	{
		return true;
	}

	String depotPath;
	GDepot->GetAbsolutePath( depotPath );

	String speechLang;
	if ( !SLocalizationManager::GetInstance().FindLocaleStr( localeId, speechLang ) )
	{
		return false;
	}
	speechLang = speechLang.ToLower();

	// TODO: Implement getting fallback language information, for getting voiceovers and lipsync

	if ( pack.GetVoiceoverFileName().Empty() )
	{
		return true;
	}

#if !defined(NO_RESOURCE_IMPORT)
	const String lipsyncPath = GetLipsPath( pack.GetVoiceoverFileName(), speechLang );
	const CFilePath filePath( lipsyncPath );

	ISkeletalAnimationImporter* importer = ISkeletalAnimationImporter::FindImporter( filePath.GetExtension() );
	if ( importer )
	{
		AnimImporterParams params;
		params.m_filePath = lipsyncPath;
		params.m_preferBetterQuality = true;

		if ( !importer->PrepareForImport( lipsyncPath, params ) )
		{
			if ( params.m_errorCode == IImporter::ImportOptions::EEC_FileToReimport )
			{
				RED_LOG_WARNING( RED_LOG_CHANNEL( LIPSYNC ), TXT("This file %s is old but you can import it. Change lipsync generator exe for GOTY."), lipsyncPath.AsChar() );
				CSkeletalAnimation* lipsyncAnimation = importer->DoImport( params );
				pack.SetLipsync( lipsyncAnimation );
			}
			else if ( params.m_errorCode == IImporter::ImportOptions::EEC_BadVersion )
			{
				RED_LOG_ERROR( RED_LOG_CHANNEL( LIPSYNC ), TXT("This file can be broken: %s"), lipsyncPath.AsChar() );
				pack.SetLipsync( nullptr );
			}
			else if ( params.m_errorCode == IImporter::ImportOptions::EEC_FileNotExist )
			{
				RED_LOG_ERROR( RED_LOG_CHANNEL( LIPSYNC ), TXT("This file not exists: %s"), lipsyncPath.AsChar() );
				pack.SetLipsync( nullptr );
			}
		}
		else
		{
			CSkeletalAnimation* lipsyncAnimation = importer->DoImport( params );
			pack.SetLipsync( lipsyncAnimation );
		}
	}
	else
	{
		ASSERT( 0 );
		pack.SetLipsync( NULL );
	}
#else	// !defined(NO_RESOURCE_IMPORT)
		pack.SetLipsync( NULL );
#endif	// !defined(NO_RESOURCE_IMPORT)
	

	String voiceoverPath = GetVOPath( pack.GetVoiceoverFileName(), speechLang, TXT( ".ogg" ) );

	Bool voiceLoaded = pack.GetSpeechBuffer().LoadFromFile( voiceoverPath );
	if ( voiceLoaded == false )
	{
		voiceoverPath = GetVOPath( pack.GetVoiceoverFileName(), speechLang, TXT( ".wav" ) );
		voiceLoaded = pack.GetSpeechBuffer().LoadFromFile( voiceoverPath );
	}
	
	return voiceLoaded;
#else
	return true;
#endif
}

String CSQLStringDBDataAccess::GetVOFilename( Uint32 stringId )
{

	String voFilename = String::EMPTY;
#if defined(W2_PLATFORM_WIN32) && ! defined( NO_EDITOR )
	
	sqlStringInfoStruct* readResult = sqlGetStringInfo( m_SqlConnection, stringId);

	if ( readResult != NULL )
	{
		voFilename = String( readResult->voiceoverName ).TrimCopy();

		sqlFreeGetStringInfoResult( readResult );
	}

#endif
	return voFilename;
}

void CSQLStringDBDataAccess::GetLanguagePackBatch( const TDynArray< Uint32 >& stringIds, Uint32 locale, TDynArray< TPair< Uint32, LanguagePack* > >& languagePacks )
{
	for ( TDynArray< Uint32 >::const_iterator idIter = stringIds.Begin(); idIter != stringIds.End();
		++idIter)
	{
		LanguagePack* pack = GetLanguagePack( *idIter, false, locale );
		languagePacks.PushBack( TPair< Uint32, LanguagePack* >( *idIter, pack ) );
	}
}

void CSQLStringDBDataAccess::SearchForLocalizedStrings( const String& searchQuery, const TDynArray< String >& categories, TDynArray< Uint32 >& ids, TDynArray< String >* keys, TDynArray< String >* strings, Bool searchKeys, const Char* orderByCol )
{
	sqlSearchParams params;

	params.lang =  SLocalizationManager::GetInstance().GetCurrentLocaleId();
	params.searchKeys = searchKeys;
	params.searchQuery = searchQuery.AsChar();

	if( orderByCol )
	{
		params.orderBy = orderByCol;
	}
	else
	{
		params.orderBy = TXT( "STRING_ID" );
	}

	params.numCategories = categories.Size();
	params.categories = new const UniChar*[ params.numCategories ];
	for( Uint32 i = 0; i < params.numCategories; ++i )
	{
		params.categories[ i ] = categories[ i ].AsChar();
	}

	sqlSearchResults* results = sqlSearchStrings( m_SqlConnection, params );
	
	if( results )
	{
		for( Uint32 i = 0; i < results->size; ++i )
		{
			ids.PushBack( results->ids[ i ] );

			if( keys != NULL )
			{
				if( results->keys[ i ] )
				{
					keys->PushBack( String( results->keys[ i ] ).TrimCopy() );
				}
				else
				{
					keys->PushBack( String() );
				}
			}

			if( strings != NULL )
			{
				strings->PushBack( results->texts[ i ] );
			}
		}

		sqlFreeSearchResults( results );
	}
	else
	{
		ASSERT( false && "There was a problem connecting to the strings database" );
	}
}

#endif // #ifndef NO_STRING_DB
