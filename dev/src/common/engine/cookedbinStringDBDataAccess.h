/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "stringDBDataAccess.h"
#include "../core/sortedmap.h"
#include "../core/lazyCacheChain.h"
#include "../core/contentListener.h"
#include "../core/hashmap.h"
#include "../core/contentType.h"
#include "../core/contentManifest.h"

#include "cookedStrings.h"
#include "cookedSpeeches.h"

class CCookedBinStringDBDataAccess
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:

	typedef TSortedMap< Uint32, String > StringsMap;

private:
	static void OnCookedSpeechesReady( void* context, const String& absoluteFilePath );
	static void OnCookedTextReady( void* context, const String& absoluteFilePath );

private:

	CCookedStrings	m_cookedStrings;
	volatile Bool	m_cookedStringsReady;

	CCookedSpeeches	m_cookedSpeeches;
	volatile Bool	m_cookedSpeechesReady;

	IFile*					m_speechesFile;
	Red::Threads::CMutex	m_speechesMutex;

	RuntimePackageID	m_packageID;
	CName				m_contentName;

public:

	CCookedBinStringDBDataAccess( RuntimePackageID packageID = INVALID_RUNTIME_PACKAGE_ID, CName contentName = CName::NONE );
	~CCookedBinStringDBDataAccess( );

	CName GetContentName() const { return m_contentName; }
	RuntimePackageID GetPackageID() const { return m_packageID; }

	void LoadStringsFile( IFile *stringsFile );
	void LoadSpeechesFile( IFile *speechesFile );

	Bool AttachStrings( const String& filePath );
	Bool AttachSpeeches( const String& filePath );

	// Doesn't need to be atomic with attaching, since should only be attached on main thread
	Bool IsStringsAttached( ) const { return !m_cookedStrings.m_offsetMap.Empty( ); }
	Bool IsSpeechesAttached( ) const { return !m_cookedSpeeches.m_offsetMap.Empty( ); }

private:

	Bool GetLocalizedTextsBatch( TDynArray< Uint32 >& stringsToLoad, StringsMap& stringsWithKeys );

public:
	// Operation result
	enum EResult
	{
		eResult_NotReady=-1,				//!< Operation is still being processed, you must wait
		eResult_NotFinished=0,				//!< Operation failed (and/or) result is invalid/incomplete
		eResult_Finished=1,					//!< Operation succeeded (and/or) result is valid
	};

public:

	EResult GetLocalizedText( Uint32 stringId, String& outText );
	EResult GetLocalizedTextByStringKey( const String& stringKey, String& outText );

	EResult DoesStringKeyExist( const String &stringKey );
	EResult GetStringIdByStringKey( const String &stringKey, Uint32& outStringId );

	EResult GetLanguagePack( Uint32 stringId, Bool textOnly, LanguagePack*& outLanguagePack );
	EResult GetLanguagePackBatch( const TDynArray< Uint32 >& stringIds, TDynArray< TPair< Uint32, LanguagePack* > >& outLanguagePacks );

	EResult RetrieveLanguagePackVoice( Uint32 stringId, LanguagePack* pack );
	void LoadSpeechData_NoLock( LanguagePack* pack, const CCookedSpeeches::SOffset& speechData );

private:
	struct SCacheJobContext
	{
		typedef void (*CallbackFn)(void*,const String&);
		CallbackFn		m_readyCallback;
		void*			m_userData;

		SCacheJobContext( CallbackFn readyCallback, void* userData )
			: m_readyCallback( readyCallback )
			, m_userData( userData)
		{}
	};

	template< typename T >
	class CCookedCacheJob : public ILoadJob
	{
	public:

	private:
		T* m_cache;
		String m_filepath;
		SCacheJobContext m_context;

	public:
		CCookedCacheJob( T* cache, const String& filepath, SCacheJobContext context )
			: ILoadJob( JP_Speech, true ), m_cache( cache ), m_filepath( filepath ), m_context( context ) { }
		virtual ~CCookedCacheJob( ) { }
	protected:
		virtual EJobResult Process( ) override
		{
			RED_ASSERT( m_cache && m_filepath != String::EMPTY );
			Red::TScopedPtr< IFile > file( GFileManager->CreateFileReader( m_filepath, FOF_Buffered | FOF_AbsolutePath ) );
			if( file )
			{
				m_cache->Load( *file );
			}
			else
			{
				ERR_ENGINE( TXT( "Failed to open cooked cache file!" ) );
			}
			if ( m_context.m_readyCallback )
			{
				m_context.m_readyCallback( m_context.m_userData, m_filepath );
			}
			return JR_Finished;
		}
	public:
		virtual const Char* GetDebugName( ) const override { return TXT( "LocalizationCacheJob" ); }
	};
};

//////////////////////////////////////////////////////////////////////////
/// Resolver for collision caches

extern IStringDBDataAccess* CreateCookedSDBAccess( const String& textLang, const String& speechLang );

class CCookedSDBAccess : public IStringDBDataAccess
{
private:
	//!< Unsupported IStringDBDataAccess functions
	virtual void Read( LocalizedString *localizedString ) override {}
	virtual void Update( TDynArray< LocalizedStringEntry >& stringEntries ) override {}
	virtual void Update( TDynArray< LocalizedString* > strings ) override {}
	virtual Bool Update( LocalizedString& localizedString ) override { return false; }
	virtual void Update( const THashMap< Uint32, String > &strings, const String &lang ) override {}
	virtual void UpdateStringInfos( TDynArray< LocalizedStringEntry >& stringEntries ) override {}
	virtual void ReadLanguageList( THashMap< String, Uint32 >& availableLanguages ) override {}
	virtual void ReadAllStringsWithKeys( TDynArray< Uint32 > &stringsIds /* out */, TDynArray< String > &stringsKeys /* out */, TDynArray< String > &stringsCategories /* out */ ) override {}
	virtual void ReadAllStringsCategories( TDynArray< String > &stringsCategories /* out */, Bool keysOnly /* in */  ) override {}
	virtual void ReadLanguagePack( Uint32 stringId, LanguagePack& pack, Bool immediate, Bool& textLoaded, Bool& voiceLoaded ) override {}
	virtual Bool ReadLanguagePackByStringKey( const String &stringKey, LanguagePack& pack, Bool immediate = false ) override { return true; } // Why true? Because apparently.
	virtual void ReadLanguagePacksForCook( Uint32 locale, const String& stringsView, THashMap< Uint32, LanguagePack* >& packs ) override {}
	virtual IStringDBDataAccess* DecorateDataAccess( IStringDBDataAccess* decorator ) override { return nullptr; }
	virtual Uint32 GetNextId() override { return 0; }
	virtual String GetStringKeyById( Uint32 stringId ) override { return String::EMPTY; }
	virtual String GetVOFilename( Uint32 stringId ) override { return String::EMPTY; }

private:
	//!< Simple IStringDBDataAccess functions
	virtual Bool ShouldCache() { return false; }

private:
	//!< Chained IStringDBDataAccess functions
	virtual String GetLocalizedText( Uint32 stringId, const String& locale, Bool *fallback = NULL ) override;
	virtual String GetLocalizedText( Uint32 stringId, Uint32 locale, Bool *fallback = NULL ) override;
	virtual String GetLocalizedTextByStringKey( const String &stringKey, const String& locale, Bool *fallback = NULL ) override;
	virtual String GetLocalizedTextByStringKey( const String &stringKey, Uint32 locale, Bool *fallback = NULL ) override;
	virtual Bool DoesStringKeyExist( const String &stringKey ) override;
	virtual Uint32 GetStringIdByStringKey( const String &stringKey ) override;
	virtual LanguagePack* GetLanguagePack( Uint32 stringId, Bool textOnly, Uint32 locale ) override;
	virtual void GetLanguagePackBatch( const TDynArray< Uint32 >& stringIds, Uint32 locale, TDynArray< TPair< Uint32, LanguagePack* > >& languagePacks ) override;
	virtual Bool RetrieveLanguagePackVoice( Uint32 stringId, LanguagePack* pack ) override;
};

// FIXME: Should make CCookedBinStringDBDataAccess non virtual and don't chain calls just for some stub function!
class CCookedStringsDBResolver : public IContentListener
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	static const Uint32 MAX_CACHE_CHAIN_LENGTH = 64;

public:
	CCookedStringsDBResolver();
	virtual ~CCookedStringsDBResolver();
	void Shutdown();

private:
	typedef CCookedBinStringDBDataAccess::EResult EResult;

private:

	struct SSplitDatabaseInfo
	{
		RuntimePackageID			m_packageID;
		CName						m_contentName;
		Bool						m_isPatch;
		THashMap< String, String >	m_langStringsFileMap;
		THashMap< String, String >	m_langSpeechesFileMap;

		// Matching doesn't consider isPatch. Should be content0 vs patch0, so uniquely named contentNames anyway.
		Bool IsMatch( RuntimePackageID packageID, CName contentName ) const { return m_packageID == packageID && m_contentName == contentName; }

		SSplitDatabaseInfo()
			: m_packageID( INVALID_RUNTIME_PACKAGE_ID )
			, m_isPatch( false )
		{ }

		SSplitDatabaseInfo( RuntimePackageID packageID, CName contentName, Bool isPatch )
			: m_packageID( packageID )
			, m_contentName( contentName )
			, m_isPatch( isPatch )
		{ }
	};

private:
	//!< IContentListener functions
	virtual const Char* GetName() const override { return TXT("CCookedStringsDBResolver"); }
	virtual void OnContentAvailable( const SContentInfo& contentInfo ) override;

public:
	void UpdateLanguage( const String& newTextLang, const String& newSpeechLang );

public:
	//!< Chained IStringDBDataAccess functions
	String GetLocalizedText( Uint32 stringId );
	String GetLocalizedTextByStringKey( const String &stringKey );
	Bool DoesStringKeyExist( const String &stringKey );
	Uint32 GetStringIdByStringKey( const String &stringKey );
	LanguagePack* GetLanguagePack( Uint32 stringId, Bool textOnly );
	void GetLanguagePackBatch( const TDynArray< Uint32 >& stringIds, TDynArray< TPair< Uint32, LanguagePack* > >& languagePacks );
	Bool RetrieveLanguagePackVoice( Uint32 stringId, LanguagePack* pack );

private:
	void UpdateStringsEntry( RuntimePackageID packageID, CName contentName, const String& mountPath, const SContentFile& entry );
	void UpdateSpeechesEntry( RuntimePackageID packageID, CName contentName, const String& mountPath, const SContentFile& entry );
	void AttachContentIfNeeded( RuntimePackageID packageID, CName contentName );
	void ReattachContent();

private:
	SSplitDatabaseInfo& UpdateSplitDatabaseInfo( RuntimePackageID packageID, CName contentName, Bool isPatch );
	const SSplitDatabaseInfo* GetSplitDatabaseInfoPtr( RuntimePackageID packageID, CName contentName ) const;

private:
	String										m_textLang;
	String										m_speechLang;

	TDynArray< SSplitDatabaseInfo > m_contentDatabase;

private:
	typedef Helper::CLazyCacheChain< CCookedBinStringDBDataAccess, MAX_CACHE_CHAIN_LENGTH > CacheChain;
	CacheChain m_dbAccessChain;
};

//-------------------------------------------------

extern CCookedStringsDBResolver* GCookedStringsDB;