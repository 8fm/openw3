/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "cookedbinStringDBDataAccess.h"

#include "localizationManager.h"

#include "../redSystem/crt.h"
#include "../core/dependencyLoader.h"
#include "../core/depot.h"
#include "../core/contentManifest.h"

void CCookedBinStringDBDataAccess::OnCookedSpeechesReady( void* context, const String& absoluteFilePath )
{
	CCookedBinStringDBDataAccess* self = static_cast< CCookedBinStringDBDataAccess* >( context );
	RED_FATAL_ASSERT( self, "Null context!");

	// If empty, don't waste a filehandle resource on it
	// Also delay opening this FH as late as possible to minimize spiking their count
	if ( !self->m_cookedSpeeches.IsEmpty() )
	{
		// We need to keep a handle of the file for runtime queries.
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( self->m_speechesMutex );
		IFile* file = GFileManager->CreateFileReader( absoluteFilePath, FOF_Buffered | FOF_AbsolutePath );
		if( !file )
		{
			ERR_ENGINE( TXT( "No speeches file!" ) );
		}
		self->m_speechesFile = file;
	}

	self->m_cookedSpeechesReady = true;
}

void CCookedBinStringDBDataAccess::OnCookedTextReady( void* context, const String& absoluteFilePath )
{
	RED_UNUSED( absoluteFilePath );

	CCookedBinStringDBDataAccess* self = static_cast< CCookedBinStringDBDataAccess* >( context );
	RED_FATAL_ASSERT( self, "Null context!");

	self->m_cookedStringsReady = true;
}

CCookedBinStringDBDataAccess::CCookedBinStringDBDataAccess( RuntimePackageID packageID /*= INVALID_RUNTIME_PACKAGE_ID*/, CName contentName /*=CName::NONE*/ )
	: m_cookedStringsReady( true )
	, m_cookedSpeechesReady( true )
	, m_speechesFile( nullptr )
	, m_packageID( packageID )
	, m_contentName( contentName )
{ }

CCookedBinStringDBDataAccess::~CCookedBinStringDBDataAccess( )
{
	if( m_speechesFile != nullptr )
	{
		delete m_speechesFile;
		m_speechesFile = nullptr;
	}
}

Bool CCookedBinStringDBDataAccess::AttachStrings( const String& filePath )
{
	m_cookedStringsReady = false;
	SCacheJobContext context( &OnCookedTextReady, this );
	CCookedCacheJob< CCookedStrings >* m_stringsLoadJob = new CCookedCacheJob< CCookedStrings >( &m_cookedStrings, filePath, context );
	SJobManager::GetInstance().Issue( m_stringsLoadJob );
	m_stringsLoadJob->Release( );

	return true;
}

Bool CCookedBinStringDBDataAccess::AttachSpeeches( const String& filePath )
{
	m_cookedSpeechesReady = false;

	{
		// TBD: probably doesn't need this mutex, more for guarding against offset changes and shouldn't be used while attaching anyway
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_speechesMutex );
		if( m_speechesFile )
		{
			delete m_speechesFile;
			m_speechesFile = nullptr;
		}
	}

	SCacheJobContext context( &OnCookedSpeechesReady, this );
	CCookedCacheJob< CCookedSpeeches >* m_speechesLoadJob = new CCookedCacheJob< CCookedSpeeches >( &m_cookedSpeeches, filePath, context );
	SJobManager::GetInstance().Issue( m_speechesLoadJob );
	m_speechesLoadJob->Release( );

	return true;
}

CCookedBinStringDBDataAccess::EResult CCookedBinStringDBDataAccess::GetLocalizedText( Uint32 stringId, String& outText )
{
	if( !m_cookedStringsReady )
		return eResult_NotReady;

	String text = String::EMPTY;

	if( !m_cookedStrings.GetString( stringId, text ) )
		return eResult_NotFinished;

	outText = text;
	return eResult_Finished;
}

CCookedBinStringDBDataAccess::EResult CCookedBinStringDBDataAccess::GetLocalizedTextByStringKey( const String& stringKey, String& outText )
{
	Uint32 stringId = 0;
	const EResult result = GetStringIdByStringKey( stringKey, stringId );
	if ( result != eResult_Finished )
	{
		return result;
	}

	return GetLocalizedText( stringId, outText );
}

CCookedBinStringDBDataAccess::EResult CCookedBinStringDBDataAccess::DoesStringKeyExist( const String& stringKey )
{
	Uint32 stringId = 0;
	EResult result = GetStringIdByStringKey( stringKey, stringId );
	if ( result == eResult_Finished && stringId == 0 )
	{
		result = eResult_NotFinished; // invalidate if stringId zero for whatever unknown reason
	}

	return result;
}

CCookedBinStringDBDataAccess::EResult CCookedBinStringDBDataAccess::GetStringIdByStringKey( const String &stringKey, Uint32& outStringId )
{
	if( !m_cookedStringsReady )
		return eResult_NotReady;

	Uint32 stringId = 0;
	return m_cookedStrings.GetStringIdByKey( stringKey.ToLower( ), outStringId ) ? eResult_Finished : eResult_NotFinished;
}

namespace LanguagePackHelpers
{
	Bool HasTextData( LanguagePack* pack )
	{
		RED_FATAL_ASSERT( pack, "pack nullptr");
		return pack->GetTextSize() > 0;
	}

	Bool HasSpeechData( LanguagePack* pack )
	{
		RED_FATAL_ASSERT( pack, "pack nullptr");
		return pack->GetVoiceoverSize() > 0 || pack->GetLipsyncSize() > 0;
	}
}

CCookedBinStringDBDataAccess::EResult CCookedBinStringDBDataAccess::GetLanguagePack( Uint32 stringId, Bool textOnly, LanguagePack*& outLanguagePack )
{
	if( !m_cookedStringsReady || ( !textOnly && !m_cookedSpeechesReady ) )
		return eResult_NotReady;

	if ( !outLanguagePack )
	{
		outLanguagePack = SLocalizationManager::GetInstance( ).CreateLanguagePack( );
	}

	String text = String::EMPTY;
	if ( !LanguagePackHelpers::HasTextData( outLanguagePack ) && m_cookedStrings.GetString( stringId, text ) )
	{
		outLanguagePack->SetText( text );
	}

	if ( !textOnly && !LanguagePackHelpers::HasSpeechData(outLanguagePack) )
	{
		const EResult speechResult = RetrieveLanguagePackVoice( stringId, outLanguagePack );
		if ( speechResult == eResult_NotReady )
		{
			return eResult_NotReady;
		}
	}

	const Bool hasText = LanguagePackHelpers::HasTextData( outLanguagePack );
	const Bool hasSpeech = LanguagePackHelpers::HasSpeechData( outLanguagePack );

	if ( textOnly )
	{
		return hasText ? eResult_Finished : eResult_NotFinished;
	}

	return hasText && hasSpeech ? eResult_Finished : eResult_NotFinished;
}

CCookedBinStringDBDataAccess::EResult CCookedBinStringDBDataAccess::GetLanguagePackBatch( const TDynArray< Uint32 >& stringIds, TDynArray< TPair< Uint32, LanguagePack* > >& outLanguagePacks )
{
	if( !m_cookedStringsReady || !m_cookedSpeechesReady )
		return eResult_NotReady;

	typedef TPair< Uint32, CCookedSpeeches::SOffset > TSpeechPair;

	struct SpeechLoadData
	{
		TSpeechPair						m_speechPair;
		LanguagePack*					m_languagePack;

		SpeechLoadData()
		{}

		SpeechLoadData( const TSpeechPair& speechPair, LanguagePack* languagePack )
			: m_speechPair( speechPair )
			, m_languagePack( languagePack )
		{}
	};

	TDynArray< SpeechLoadData > speechLoadData;

	speechLoadData.Reserve( stringIds.Size( ) );

	Uint32 speechKey = m_cookedSpeeches.m_langKey;

	for( TDynArray< Uint32 >::const_iterator idIter = stringIds.Begin( ); idIter != stringIds.End( ); ++idIter )
	{
		if( *idIter == 0 )
			continue;

		LanguagePack* pack = nullptr;
		
		auto findIt = ::FindIf( outLanguagePacks.Begin(), outLanguagePacks.End(), 
			[idIter](const TPair<Uint32,LanguagePack*>& elem ){ return elem.m_first == *idIter; } );
		if ( findIt != outLanguagePacks.End() )
		{
			pack = findIt->m_second;
		}
		else
		{
			pack = SLocalizationManager::GetInstance( ).CreateLanguagePack( );
			// Push into list even if no speech or text - maintaining old behavior
			outLanguagePacks.PushBack( TPair< Uint32, LanguagePack* >( *idIter, pack ) );
		}

		String text = String::EMPTY;
		if ( !LanguagePackHelpers::HasTextData( pack ) && m_cookedStrings.GetString( *idIter, text ) )
		{
			pack->SetText( text );
		}

		if ( !LanguagePackHelpers::HasSpeechData( pack ) )
		{
			CCookedSpeeches::SOffset speechOffset;
			if( m_cookedSpeeches.GetOffset( *idIter, speechOffset ) )
			{
				speechLoadData.PushBack( SpeechLoadData( TSpeechPair( *idIter, speechOffset ), pack ) );
			}
		}
	}

	if( !speechLoadData.Empty( ) && m_speechesFile != nullptr )
	{
		// Sort entries by offset to read sequentially (???).
		Sort( speechLoadData.Begin( ), speechLoadData.End( ), 
			[]( const SpeechLoadData& a, const SpeechLoadData& b) { return a.m_speechPair.m_second.m_voOffset < b.m_speechPair.m_second.m_voOffset; } );

		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_speechesMutex );

		for( Uint32 pairIdx = 0; pairIdx < speechLoadData.Size( ); ++pairIdx )
		{
			const SpeechLoadData& data = speechLoadData[ pairIdx ];
			const TSpeechPair& pair = data.m_speechPair;
			Uint32 stringId = pair.m_first;
			const CCookedSpeeches::SOffset& speechData = pair.m_second;

			LanguagePack* pack = data.m_languagePack;
			RED_FATAL_ASSERT( pack, "nullptr language pack!");
			LoadSpeechData_NoLock( pack, speechData );
		}
	}

	// We created stub packs for each non zero stringID, so check here if all finished yet. Return value doesn't especially matter much here except
	// to stop processing the chain early
	for ( const auto& pair : outLanguagePacks )
	{
		LanguagePack* pack = pair.m_second;
		if ( !LanguagePackHelpers::HasTextData( pack ) || !LanguagePackHelpers::HasSpeechData(pack) )
		{
			return eResult_NotFinished;
		}
	}

	return eResult_Finished;
}

CCookedBinStringDBDataAccess::EResult CCookedBinStringDBDataAccess::RetrieveLanguagePackVoice( Uint32 stringId, LanguagePack* pack )
{
	if( !m_cookedSpeechesReady )
		return eResult_NotReady;

	ASSERT( pack != NULL );

	CCookedSpeeches::SOffset* speechData = NULL;

	if( !m_cookedSpeeches.m_offsetMap.Empty( ) )
	{
		Uint32	key = m_cookedSpeeches.m_langKey;
		Uint32	decodedStringId = stringId ^ key;

		speechData = m_cookedSpeeches.m_offsetMap.FindPtr( decodedStringId );
	}

	if( speechData != NULL )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_speechesMutex );
		LoadSpeechData_NoLock( pack, *speechData );
		return eResult_Finished;
	}

	return eResult_NotFinished;
}

void CCookedBinStringDBDataAccess::LoadSpeechData_NoLock( LanguagePack* pack, const CCookedSpeeches::SOffset& speechData )
{
	if( speechData.m_voSize > 0 )
	{
		m_speechesFile->Seek( speechData.m_voOffset );
		pack->GetSpeechBuffer( ).Serialize( *m_speechesFile );
	}

	if( speechData.m_lipsyncSize > 0 )
	{
		m_speechesFile->Seek( speechData.m_lipsyncOffset );
		CSkeletalAnimation* lipsync = nullptr;

		DependencyLoadingContext context;
		context.m_getAllLoadedObjects = true;
		CDependencyLoader loader( *m_speechesFile, nullptr );
		if( loader.LoadObjects( context ) )
		{
			lipsync = Cast< CSkeletalAnimation >( context.m_loadedObjects[ 0 ].GetSerializablePtr( ) );
		}
		else
		{
			lipsync = ClassID< CSkeletalAnimation >( )->CreateObject< CSkeletalAnimation >( );
		}
		lipsync->EnableReferenceCounting();
		pack->SetLipsync( lipsync );
	}
}

//////////////////////////////////////////////////////////////////////////
// CreateCookedSDBAccess
//////////////////////////////////////////////////////////////////////////
IStringDBDataAccess* CreateCookedSDBAccess( const String& textLang, const String& speechLang )
{
	LOG_ENGINE(TXT("CreateCookedSDBAccess: text=%ls, speech=%ls"), textLang.AsChar(), speechLang.AsChar() );
	GCookedStringsDB->UpdateLanguage( textLang, speechLang );
	return new CCookedSDBAccess;
}

//////////////////////////////////////////////////////////////////////////
// CCookedSDBAccess
//////////////////////////////////////////////////////////////////////////
String CCookedSDBAccess::GetLocalizedText( Uint32 stringId, const String& locale, Bool *fallback /*= NULL */ )
{
	RED_UNUSED( locale );
	RED_UNUSED( fallback );

	return GCookedStringsDB->GetLocalizedText( stringId );
}

String CCookedSDBAccess::GetLocalizedText( Uint32 stringId, Uint32 locale, Bool *fallback /*= NULL */ )
{
	RED_UNUSED( locale );
	RED_UNUSED( fallback );

	return GCookedStringsDB->GetLocalizedText( stringId );
}

String CCookedSDBAccess::GetLocalizedTextByStringKey( const String &stringKey, const String& locale, Bool *fallback /*= NULL */ )
{
	RED_UNUSED( locale );
	RED_UNUSED( fallback );

	return GCookedStringsDB->GetLocalizedTextByStringKey( stringKey );
}

String CCookedSDBAccess::GetLocalizedTextByStringKey( const String &stringKey, Uint32 locale, Bool *fallback /*= NULL */ )
{
	RED_UNUSED( locale );
	RED_UNUSED( fallback );

	return GCookedStringsDB->GetLocalizedTextByStringKey( stringKey );
}

Bool CCookedSDBAccess::DoesStringKeyExist( const String &stringKey )
{
	return GCookedStringsDB->DoesStringKeyExist( stringKey );
}

Uint32 CCookedSDBAccess::GetStringIdByStringKey( const String &stringKey )
{
	return GCookedStringsDB->GetStringIdByStringKey( stringKey );
}

LanguagePack* CCookedSDBAccess::GetLanguagePack( Uint32 stringId, Bool textOnly, Uint32 locale )
{
	RED_UNUSED( locale );

	return GCookedStringsDB->GetLanguagePack( stringId, textOnly );
}

void CCookedSDBAccess::GetLanguagePackBatch( const TDynArray< Uint32 >& stringIds, Uint32 locale, TDynArray< TPair< Uint32, LanguagePack* > >& languagePacks )
{
	RED_UNUSED( locale );

	GCookedStringsDB->GetLanguagePackBatch( stringIds, languagePacks );
}

Bool CCookedSDBAccess::RetrieveLanguagePackVoice( Uint32 stringId, LanguagePack* pack )
{
	return GCookedStringsDB->RetrieveLanguagePackVoice( stringId, pack );
}

//////////////////////////////////////////////////////////////////////////
// CCookedStringsDBResolver
//////////////////////////////////////////////////////////////////////////
CCookedStringsDBResolver::CCookedStringsDBResolver()
{
}

CCookedStringsDBResolver::~CCookedStringsDBResolver()
{
}

#ifdef RED_LOGGING_ENABLED
static const Char* GetResultTextForLog( CCookedBinStringDBDataAccess::EResult result )
{
	switch( result )
	{
	case CCookedBinStringDBDataAccess::eResult_NotReady: return TXT("eResult_NotReady");
	case CCookedBinStringDBDataAccess::eResult_NotFinished: return TXT("eResult_NotFinished");
	case CCookedBinStringDBDataAccess::eResult_Finished: return TXT("eResult_Finished");
	default:
		break;
	}

	return TXT("<Unknown>");
}

static String GetPackStringForLog( LanguagePack* pack )
{
	if ( !pack )
	{
		return TXT("<Null pack>");
	}

	return String::Printf(TXT("pack text='%ls', has speech=%u, has lipsync=%u"), pack->GetText().AsChar(), pack->GetVoiceoverSize() > 0, pack->GetLipsyncSize() > 0 );
}

#endif // RED_LOGGING_ENABLED

String CCookedStringsDBResolver::GetLocalizedText( Uint32 stringId )
{
	String text;
	EResult result = CCookedBinStringDBDataAccess::eResult_NotFinished;
	for ( CCookedBinStringDBDataAccess* dbAccess : m_dbAccessChain )
	{
		result = dbAccess->GetLocalizedText( stringId, text );
		RED_FATAL_ASSERT( result != CCookedBinStringDBDataAccess::eResult_NotReady, "dbAccess must have been ready before use!");
		if ( result == CCookedBinStringDBDataAccess::eResult_Finished )
		{
			break;
		}
	}

	if ( result != CCookedBinStringDBDataAccess::eResult_Finished )
	{
		ERR_ENGINE( TXT("GetLocalizedText(stringID=%u) failed - %ls"), stringId, GetResultTextForLog(result) );
	}

	return text;
}

String CCookedStringsDBResolver::GetLocalizedTextByStringKey( const String &stringKey )
{
	String text;
	EResult result = CCookedBinStringDBDataAccess::eResult_NotFinished;
	for ( CCookedBinStringDBDataAccess* dbAccess : m_dbAccessChain )
	{
		result = dbAccess->GetLocalizedTextByStringKey( stringKey, text );
		RED_FATAL_ASSERT( result != CCookedBinStringDBDataAccess::eResult_NotReady, "dbAccess must have been ready before use!");
		if ( result == CCookedBinStringDBDataAccess::eResult_Finished )
		{
			break;
		}
	}

	if ( result != CCookedBinStringDBDataAccess::eResult_Finished )
	{
		ERR_ENGINE( TXT("GetLocalizedTextByStringKey(stringKey=%ls) failed - %ls"), stringKey.AsChar(), GetResultTextForLog(result) );
	}

	return text;
}

Bool CCookedStringsDBResolver::DoesStringKeyExist( const String &stringKey )
{
	Bool retval = false;
	EResult result = CCookedBinStringDBDataAccess::eResult_NotFinished;
	for ( CCookedBinStringDBDataAccess* dbAccess : m_dbAccessChain )
	{
		result = dbAccess->DoesStringKeyExist( stringKey );
		RED_FATAL_ASSERT( result != CCookedBinStringDBDataAccess::eResult_NotReady, "dbAccess must have been ready before use!");
		if ( result == CCookedBinStringDBDataAccess::eResult_Finished )
		{
			retval = true;
			break;
		}
	}

	return retval;
}

Uint32 CCookedStringsDBResolver::GetStringIdByStringKey( const String &stringKey )
{
	Uint32 foundStringId = 0;
	EResult result = CCookedBinStringDBDataAccess::eResult_NotFinished;
	for ( CCookedBinStringDBDataAccess* dbAccess : m_dbAccessChain )
	{
		result = dbAccess->GetStringIdByStringKey( stringKey, foundStringId );
		RED_FATAL_ASSERT( result != CCookedBinStringDBDataAccess::eResult_NotReady, "dbAccess must have been ready before use!");
		if ( result == CCookedBinStringDBDataAccess::eResult_Finished )
		{
			break;
		}
	}

	if ( result != CCookedBinStringDBDataAccess::eResult_Finished )
	{
		ERR_ENGINE( TXT("GetStringIdByStringKey(stringKey=%ls) failed - %ls"), stringKey.AsChar(), GetResultTextForLog(result) );
	}

	return foundStringId;
}

LanguagePack* CCookedStringsDBResolver::GetLanguagePack( Uint32 stringId, Bool textOnly )
{
	LanguagePack* pack = nullptr;
	EResult result = CCookedBinStringDBDataAccess::eResult_NotFinished;
	for ( CCookedBinStringDBDataAccess* dbAccess : m_dbAccessChain )
	{
		result = dbAccess->GetLanguagePack( stringId, textOnly, pack );
		RED_FATAL_ASSERT( result != CCookedBinStringDBDataAccess::eResult_NotReady, "dbAccess must have been ready before use!");
		if ( result == CCookedBinStringDBDataAccess::eResult_Finished )
		{
			break;
		}
	}

	if ( result != CCookedBinStringDBDataAccess::eResult_Finished )
	{
		// Keep if it has some data. It seems a lot of things don't set textOnly but really have no VOs (like dialog choices).
		//ERR_ENGINE( TXT("GetLanguagePack(stringID=%u, textOnly=%d) failed - %ls (%ls)"), stringId, textOnly, GetResultTextForLog(result), GetPackStringForLog( pack ).AsChar() );
		if ( pack && pack->GetPackSize() == 0 )
		{
			SLocalizationManager::GetInstance().DeleteLanguagePack( pack );
			pack = nullptr;
		}
	}

	if ( !pack )
	{
		pack = SLocalizationManager::GetInstance().GetEmptyLanguagePack();
	}

	return pack;
}

void CCookedStringsDBResolver::GetLanguagePackBatch( const TDynArray< Uint32 >& stringIds, TDynArray< TPair< Uint32, LanguagePack* > >& languagePacks )
{
	// Clearing to maintaining previous interface
	languagePacks.Clear();
	languagePacks.Reserve( stringIds.Size() );

	EResult result = CCookedBinStringDBDataAccess::eResult_NotFinished;
	for ( CCookedBinStringDBDataAccess* dbAccess : m_dbAccessChain )
	{
		result = dbAccess->GetLanguagePackBatch( stringIds, languagePacks );
		RED_FATAL_ASSERT( result != CCookedBinStringDBDataAccess::eResult_NotReady, "dbAccess must have been ready before use!");
		if ( result == CCookedBinStringDBDataAccess::eResult_Finished )
		{
			break;
		}
	}

	// Hard to say if really failed not knowing if it should have speech or not
// 	if ( result != CCookedBinStringDBDataAccess::eResult_Finished )
// 	{
// 		ERR_ENGINE( TXT("GetLanguagePackBatch failed - %ls"), GetResultTextForLog(result) );
// 	}
}

Bool CCookedStringsDBResolver::RetrieveLanguagePackVoice( Uint32 stringId, LanguagePack* pack )
{
	Bool retval = false;
	EResult result = CCookedBinStringDBDataAccess::eResult_NotFinished;
	for ( CCookedBinStringDBDataAccess* dbAccess : m_dbAccessChain )
	{
		result = dbAccess->RetrieveLanguagePackVoice( stringId, pack );
		RED_FATAL_ASSERT( result != CCookedBinStringDBDataAccess::eResult_NotReady, "dbAccess must have been ready before use!");
		if ( result == CCookedBinStringDBDataAccess::eResult_Finished )
		{
			retval = true;
			break;
		}
	}

	if ( result != CCookedBinStringDBDataAccess::eResult_Finished )
	{
		ERR_ENGINE( TXT("RetrieveLanguagePackVoice(stringID=%u) failed - %ls"), stringId, GetResultTextForLog(result) );
	}

	return retval;
}

void CCookedStringsDBResolver::UpdateStringsEntry( RuntimePackageID packageID, CName contentName, const String& mountPath, const SContentFile& entry )
{
	const String entryFilePath = ANSI_TO_UNICODE( entry.m_path.AsChar() );
	CFilePath filePath( entryFilePath );
	const String fileName = filePath.GetFileNameWithExt();

	const String lang = fileName.StringBefore( CLocalizationManager::STRINGS_FILE_PATH_POSTFIX ).ToLower();
	if ( lang.Empty() )
	{
		ERR_ENGINE(TXT("fileName '%ls' did not map to any language!"), lang.AsChar() );
		return;
	}

	const String stringsFilePath = mountPath + entryFilePath;

	SSplitDatabaseInfo& info = UpdateSplitDatabaseInfo( packageID, contentName, entry.m_isPatch );
	info.m_langStringsFileMap[ lang ] = stringsFilePath;
}

void CCookedStringsDBResolver::UpdateSpeechesEntry( RuntimePackageID packageID, CName contentName, const String& mountPath, const SContentFile& entry )
{
	const String entryFilePath = ANSI_TO_UNICODE( entry.m_path.AsChar() );
	CFilePath filePath( entryFilePath );
	const String fileName = filePath.GetFileNameWithExt();

	if ( !fileName.EndsWith( CLocalizationManager::CURRENT_SPEECH_FILE_PATH_POSTFIX) )
	{
		WARN_ENGINE(TXT("CCookedStringsDBResolver: speech file '%ls' not for current platform"), entryFilePath.AsChar() );
		return;
	}

	const String lang = fileName.StringBefore( CLocalizationManager::CURRENT_SPEECH_FILE_PATH_POSTFIX ).ToLower();
	if ( lang.Empty() )
	{
		ERR_ENGINE(TXT("fileName '%ls' did not map to any language!"), lang.AsChar() );
		return;
	}

	const String speechesFilePath = mountPath + entryFilePath;

	SSplitDatabaseInfo& info = UpdateSplitDatabaseInfo( packageID, contentName, entry.m_isPatch );
	info.m_langSpeechesFileMap[ lang ] = speechesFilePath;
}

void CCookedStringsDBResolver::AttachContentIfNeeded( RuntimePackageID packageID, CName contentName )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Main thread only" );

	const SSplitDatabaseInfo* info = GetSplitDatabaseInfoPtr( packageID, contentName );
	if ( !info )
	{
		// speech file was for another platform or speech/strings didn't map to any known language?
		ERR_ENGINE(TXT("No info for %ls"), contentName.AsChar());
		return;
	}

	CCookedBinStringDBDataAccess* dbAccessToUpdate = nullptr;
	for ( CCookedBinStringDBDataAccess* dbAccess : m_dbAccessChain )
	{
		if ( dbAccess->GetPackageID() == packageID && dbAccess->GetContentName() == contentName )
		{
			dbAccessToUpdate = dbAccess;
			break;
		}
	}
	
	// Create new if needed. It's OK that we create, push, and then load the files. The internal CCookedBinStringDBDataAccess mutexes
	// will protect against simultaneous loading and accessing. We have to make the DB available even if speeches are installed yet.
	if ( ! dbAccessToUpdate )
	{
#ifdef RED_LOGGING_ENABLED
		LOG_ENGINE(TXT("CCookedStringsDBResolver::AttachContentIfNeeded: creating new DB for contentName: %ls"), contentName.AsChar());
#endif // RED_LOGGING_ENABLED
		CCookedBinStringDBDataAccess* newDbAccess = new CCookedBinStringDBDataAccess( packageID, contentName );

		typedef Bool (CacheChain::* PushFunc)(CCookedBinStringDBDataAccess*);
		const PushFunc Push = info->m_isPatch ? &CacheChain::PushFront : &CacheChain::PushBack;
		if ( !(m_dbAccessChain.*Push)( newDbAccess ) )
		{
			delete newDbAccess;
			ERR_ENGINE(TXT("CCookedStringsDBResolver::AttachContentIfNeeded: Failed to attach content for '%ls'. Reached chain length limit %u"), contentName.AsChar(), MAX_CACHE_CHAIN_LENGTH );
			return;
		}

		dbAccessToUpdate = newDbAccess;
	}

	String speechesFilePath;
	if ( !dbAccessToUpdate->IsSpeechesAttached() && info->m_langSpeechesFileMap.Find( m_speechLang, speechesFilePath ) )
	{
		LOG_ENGINE(TXT("Attaching speeches '%ls' to %ls"), speechesFilePath.AsChar(), dbAccessToUpdate->GetContentName().AsChar() );
		if( !dbAccessToUpdate->AttachSpeeches( speechesFilePath ) )
		{
			ERR_ENGINE(TXT("Failed to attach '%ls'."), speechesFilePath.AsChar() );
		}
	}

	String stringsFilePath;
	if ( !dbAccessToUpdate->IsStringsAttached() && info->m_langStringsFileMap.Find( m_textLang, stringsFilePath ) )
	{
		LOG_ENGINE(TXT("Attaching strings '%ls' to %ls"), stringsFilePath.AsChar(), dbAccessToUpdate->GetContentName().AsChar() );
		if( !dbAccessToUpdate->AttachStrings( stringsFilePath ) )
		{
			ERR_ENGINE(TXT("Failed to attach '%ls'."), stringsFilePath.AsChar() );
		}
	}
}

void CCookedStringsDBResolver::OnContentAvailable( const SContentInfo& contentInfo )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Mounting off the main thread" );

	typedef Bool (CacheChain::* PushFunc)(CCookedBinStringDBDataAccess*);

	// FIXME: Put into content manifest. This is a really shitty hack.
	// Get without language suffix. E.g., content0_en -> content0
	// Note that a contentManifest could have multiple language strings in it.
	// After refactor, contentManifest can also have patch files in it
	const AnsiChar* id = contentInfo.m_chunkName.AsAnsiChar();
	const AnsiChar* ch = Red::System::StringSearch( id, "_" );
	String baseName;
	if ( ch )
	{
		// Language name like content0_en
		const Uint32 len = static_cast< Uint32 >( ch - id );
		baseName = String( ANSI_TO_UNICODE(id), len );
	}
	else
	{
		// Already a base name like just content0
		baseName = ANSI_TO_UNICODE(id);
	}

	const CName contentName( baseName );

	const RuntimePackageID packageID = contentInfo.m_packageID;

	Bool hasNewContent = false;
	for ( const SContentFile* contentFile : contentInfo.m_contentFiles )
	{
		if ( contentFile->m_path.EndsWith( ".w3speech" ) )
		{
			UpdateSpeechesEntry( packageID, contentName, contentInfo.m_mountPath, *contentFile );
			hasNewContent = true;
		}
		else if ( contentFile->m_path.EndsWith( ".w3strings" ) )
		{	
			UpdateStringsEntry( packageID, contentName, contentInfo.m_mountPath, *contentFile );
			hasNewContent = true;
		}
		else
		{
			ERR_CORE(TXT("CCookedStringsDBResolver::OnContentAvailable: Unexpected file '%hs'"), contentFile->m_path.AsChar() );
		}
	}

	// FIXME: somewhat performance wasteful, make slightly less so by checking if new content
	if ( hasNewContent )
	{
		AttachContentIfNeeded( packageID, contentName );
	}
}

void CCookedStringsDBResolver::UpdateLanguage( const String& newTextLang, const String& newSpeechLang )
{
	const String lowerTextLang = newTextLang.ToLower();
	const String lowerSpeechLang = newSpeechLang.ToLower();

	if ( m_textLang != lowerTextLang || m_speechLang != lowerSpeechLang )
	{
		m_textLang = lowerTextLang;
		m_speechLang = lowerSpeechLang;

		ReattachContent();
	}
	else
	{
		LOG_ENGINE(TXT("UpdateLanguage called without change: text=%ls, speech=%ls"), m_textLang.AsChar(), m_speechLang.AsChar() );
	}
}

void CCookedStringsDBResolver::ReattachContent()
{
	LOG_ENGINE(TXT("CCookedStringsDBResolver reattaching content"));

	// FIXME TBD: If you can't map the split of languages one-to-one, then how was it going to work anyway mixing different strings/speeches
	// However, could have a patch for one language and not another and so that content needs to be added to the chain
	// So for now just clearing the whole chain, but could possibly just reattach strings/speech separately.
	m_dbAccessChain.ClearPtr();

	typedef Bool (CacheChain::* PushFunc)(CCookedBinStringDBDataAccess*);

	for ( const SSplitDatabaseInfo& info : m_contentDatabase )
	{
		const RuntimePackageID packageID = info.m_packageID;
		const CName contentName = info.m_contentName;
		PushFunc const Push = info.m_isPatch ? &CacheChain::PushFront : &CacheChain::PushBack;
		CCookedBinStringDBDataAccess* newDbAccess = new CCookedBinStringDBDataAccess( packageID, contentName );
		if ( !(m_dbAccessChain.*Push)( newDbAccess ) )
		{
			delete newDbAccess;
			ERR_ENGINE(TXT("CCookedStringsDBResolver::UpdateLanguage: Failed to attach content for '%ls'. Reached chain length limit %u"), contentName.AsChar(), MAX_CACHE_CHAIN_LENGTH );
			return;
		}

		String stringsFilePath;
		if ( info.m_langStringsFileMap.Find( m_textLang, stringsFilePath ) )
		{
			LOG_ENGINE(TXT("Attaching strings '%ls'"), stringsFilePath.AsChar() );
			if( !newDbAccess->AttachStrings( stringsFilePath ) )
			{
				ERR_ENGINE(TXT("Failed to attach '%ls'."), stringsFilePath.AsChar() );
			}
		}
		else
		{
			WARN_ENGINE(TXT("Failed to find stringsFilePath for %ls"), m_textLang.AsChar() );
		}

		String speechesFilePath;
		if ( info.m_langSpeechesFileMap.Find( m_speechLang, speechesFilePath ) )
		{
			LOG_ENGINE(TXT("Attaching speeches '%ls'"), speechesFilePath.AsChar() );
			if( !newDbAccess->AttachSpeeches( speechesFilePath ) )
			{
				ERR_ENGINE(TXT("Failed to attach '%ls'."), speechesFilePath.AsChar() );
			}
		}
		else
		{
			WARN_ENGINE(TXT("Failed to find speechesFilePath for %ls"), m_speechLang.AsChar() );
		}
	}
}

CCookedStringsDBResolver::SSplitDatabaseInfo& CCookedStringsDBResolver::UpdateSplitDatabaseInfo( RuntimePackageID packageID, CName contentName, Bool isPatch )
{
	auto findIt = ::FindIf( m_contentDatabase.Begin(), m_contentDatabase.End(), [packageID,contentName](const SSplitDatabaseInfo& info){ return info.IsMatch( packageID, contentName ); });
	if ( findIt != m_contentDatabase.End() )
	{
		RED_FATAL_ASSERT( findIt->m_isPatch == isPatch, "Patch mismatch for %ls", contentName.AsChar());
		return *findIt;
	}

	SSplitDatabaseInfo info( packageID, contentName, isPatch );

	// Even push patches back, when attaching, will use PushFront on cache chain
	m_contentDatabase.PushBack( Move( info ) );
	return m_contentDatabase.Back();
}

const CCookedStringsDBResolver::SSplitDatabaseInfo* CCookedStringsDBResolver::GetSplitDatabaseInfoPtr( RuntimePackageID packageID, CName contentName ) const
{
	auto findIt = ::FindIf( m_contentDatabase.Begin(), m_contentDatabase.End(), [packageID,contentName](const SSplitDatabaseInfo& info){ return info.IsMatch( packageID, contentName ); });
	if ( findIt != m_contentDatabase.End() )
	{
		return findIt;
	}

	return nullptr;
}

void CCookedStringsDBResolver::Shutdown()
{
	// FIXME: Cleanup DBs!
}

static CCookedStringsDBResolver GCookedStringsDBResolver;
CCookedStringsDBResolver* GCookedStringsDB = &GCookedStringsDBResolver;
