/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "localizableObject.h"
#include "localizedStringsWithKeysCache.h"
#include "languagePack.h"
#include "localizationCache.h"
#include "../core/loadingJob.h"
#include "../core/set.h"

class IStringDBDataAccess;
class CLocalizationStorage;

#ifndef NO_DEBUG_PAGES

struct LanguagePackDebugSummary
{
	Uint32								m_maxSize;
	Uint32								m_usedSize;
	Uint32								m_createdPacks;
	Uint32								m_lockedLipsyncs;
	TDynArray< const LanguagePack* >	m_packsFromCache;
	TDynArray< Uint32	>					m_cacheStringIds;
};
#endif

enum ESearchOrder
{
	SearchOrder_Ids = 0,
	SearchOrder_Text
};

//////////////////////////////////////////////////////////////////////////

class CJobLoadLangPack : public ILoadJob
{
private:
	IStringDBDataAccess*	m_dataAccess;
	Uint32					m_stringId;
	LanguagePack*			m_loadedPack;
	Uint32					m_locale;

public:
	CJobLoadLangPack( Uint32 stringId, IStringDBDataAccess* dataAccess, Uint32 locale );
	virtual ~CJobLoadLangPack();

	RED_INLINE LanguagePack*	GetLoadedLanguagePack() const	{ return m_loadedPack; }

protected:
	//! Process the job, is called from job thread
	virtual EJobResult Process();

	virtual const Char* GetDebugName() const override { return TXT("IO LangPack"); }

};

class CJobLoadLangPackBatch : public ILoadJob
{
private:
	IStringDBDataAccess*		m_dataAccess;
	TDynArray< Uint32 >			m_stringIds;
	Uint32						m_locale;
	TDynArray< TPair< Uint32, LanguagePack* > >	m_languagePacks;

public:
	CJobLoadLangPackBatch( const TDynArray< Uint32 > stringIds, IStringDBDataAccess* dataAccess, Uint32 locale );
	virtual ~CJobLoadLangPackBatch();

	TDynArray< TPair< Uint32, LanguagePack* > >& GetLoadedPacks() { return m_languagePacks; }

protected:
	//! Process the job, is called from job thread
	virtual EJobResult Process();

	virtual const Char* GetDebugName() const override { return TXT("IO LangPack Batch"); }
};

/*
Localization manager.

Glossary:
1. stable string
   Stable string has an entry in strings db. Id of such string will not change.
2. unstable string (aka new string)
   Unstable string doesn't (yet) have an entry in strings db. This is true for
   new strings. At some point, unstable string is either discarded or committed
   to strings db in which case it gets new stable string id.
*/
class CLocalizationManager
{
public:
	static const String VOICEFILE_UNDEFINED;
	static const String VOICEFILE_UNDEFINED_VOICE;
	static const String VOICEFILE_UNDEFINED_GROUP;
	static const String VOICEFILE_UNDEFINED_ID;

	static const String STRINGS_FILE_PATH_POSTFIX;

	static const String SPEECH_FILE_PATH_POSTFIX_DURANGO;
	static const String SPEECH_FILE_PATH_POSTFIX_ORBIS;
	static const String SPEECH_FILE_PATH_POSTFIX_PC;
	static const String CURRENT_SPEECH_FILE_PATH_POSTFIX;

private:
	// Data access object 
	IStringDBDataAccess*		m_dataAccessObject;
	CLocalizationStorage		m_localizationStorage;

	THashMap< Uint32,	CJobLoadLangPack* >			m_loadingPackJobs;
	THashMap< Uint64,	CJobLoadLangPackBatch* >	m_loadingPackBatchJobs;
	Uint32									m_createdPacks;

	// Currently set locale if
	Uint32 m_currentLocaleId;

	// Currently set locale for speeches
	String m_speechLocale;

	// Currently set locale for subtitles - only on cook
	String m_textLocale;

	// Languages available to user
	THashMap< String, Uint32 > 	m_languagesIds;
	TDynArray< String >		m_textLanguages;
	TDynArray< String >		m_speechLanguages;
	String					m_defaultLanguage;

	// Is currently connected to SQL?
	Bool m_isConnected;

	// We never want to connect to SQL db
	Bool m_neverConnect;

	// Ids of strings that are fallbacked
	TSet< Uint32 >	m_fallbackIds;

	// Ids of strings that are missing
	TSet< Uint32 >	m_missingIds;

	THashMap< Uint32, String >	m_modifiedStrings; // List of modified strings. Includes new strings. Maps string id to its new contents.
	THashMap< Uint32, Uint32 >	m_copiedStringIds; // List of copied strings. Maps destination string id to source string id.
	Uint32					m_newStringsCount; // Number of new strings.
	
	Bool	m_ignoreLanguagePackResources;
	
	Bool	m_isDisabled; // NULL strings db is in use - no SQL connection, no cook

	Red::Threads::CMutex	m_packAccessMutex;

	// Empty packs optimization
	LanguagePack	m_emptyPack;
	
public:
	CLocalizationManager();

	void Shutdown();

public:
	void AddRuntimeDLCLanguages( const TDynArray<String>& textLanguages, const TDynArray<String>& speechLanguages );
	void InvalidateEmptyPacks();

private:
	Bool ParseCommandLine();
	void GetLanguageFromUserSettings( String& textLang, String& speechLang );
	void SetLanguageInUserSettings( const String& textLang, const String& speechLang );

private:
	// Different data access methods
	bool OpenSQLDataAccessInternal();

	bool OpenCookedDataAccessInternal( const String &lang, const String& speech );

	bool OpenNullDataAccess();

	// Fills available languages list with a predefined values
	void ListLanguagesSupported();

	// Checks if selected languages are available and switches selection to default if they are not
	void ValidateLanguageSelection( String& textLang, String& speechLang );

	bool OpenSomeDataAccessIfNeeded( Bool writableOrSynchronized = true );

public:
	// If writableOrSynchronized = false some cached lang, else opens SQL connection
	bool OpenSomeDataAccess( Bool writableOrSynchronized = false, Bool silent = false );

	bool OpenSQLDataAccess( Bool writableOrSynchronized = false, Bool silent = false  );

public:
	RED_INLINE Bool IsConnected() { return m_isConnected; };

	void Reconnect();

	// Locale methods

	const String &GetCurrentLocale();
	const Uint32 GetCurrentLocaleId() const;

	Bool FindLocaleId( const String& lang, Uint32& id ) const;
	Bool FindLocaleStr( const Uint32& id, String& lang ) const;

	Bool SetCurrentLocale( const String& lang );
	
	const String& GetSpeechLocale();
	void SetSpeechLocale( const String& lang );

	const String& GetTextLocale() const;

	void GetLanguageAndDefaultRegionCodes( const Char** languageCode, const Char** defaultRegionCode ) const;

	// Sets current locale by force (not caring if any content was modified and therefore possibly discarding all changes)
	void SetCurrentLocaleForce( const String &locale );

	//////////////////////////////////////////////////////////////////////////
	// In-game language management

	// Sets game languages using language names - possibly it won't be needed
	void SwitchGameLanguage( const String& newAudioLang, const String& newSubtitleLang );
	
	// Sets game languages using indices from available languages lists 
	void SwitchGameLanguageByIndex( Int32 newAudioLang, Int32 newSubtitleLang );

	// Gets game languages in form of string database language ids
	void GetGameLanguageId( Int32 &audioLang /* out */, Int32 &subtitleLang /* out */ );

	// Gets game languages in form of language names
	void GetGameLanguageName( String& audioLang /* out */, String& subtitleLang /* out */ );

	// Gets game languages in form of indices on available languages list
	void GetGameLanguageIndex( Int32& audioLang /* out */, Int32& subtitleLang /* out */ );

	// Gets a list of languages available for user
	void GetAllAvailableLanguages( TDynArray< String >& textLanguages, TDynArray< String >& speechLanguages ) const;

	Bool IsTextLanguageAvailable( const String& language ) const;
	Bool IsSpeechLanguageAvailable( const String& language ) const;

	RED_INLINE Uint32 GetNumberOfTextLanguages() const { return m_textLanguages.Size(); }
	RED_INLINE Uint32 GetNumberOfSpeechLanguages() const { return m_speechLanguages.Size(); }

	void ReloadLanguageFromUserSettings();

	//////////////////////////////////////////////////////////////////////////

	// Methods about the modification flag of localized content and synchronization
	Bool IsAnyContentModified();
	void SynchronizeWithStringDatabase();

	// Methods reading and updating localizable objects / localized strings

	void UpdateStringDatabase( /*const */ILocalizableObject* localizableObject, Bool updateInfo = false );
	void UpdateCopiedStrings( TDynArray< LocalizedStringEntry >* localizedEntries = NULL );
	void UpdateStringDatabase( const THashMap< Uint32, String > &strings, const String &lang );
	Bool UpdateStringDatabase( Uint32 stringId );
	void ResetIDs( /*const */ILocalizableObject* localizableObject );
	void UpdateStringId(Uint32 oldStringId, Uint32 newStringId);

	// Gets string directly from DB
	String GetLocalizedText( Uint32 stringId, const String& locale, Bool* isFallback = NULL );
	String GetLocalizedText( Uint32 stringId );

	// Get date string based on current locale
	String GetDateString( CSystemFile::CTime& fileTime ) const;

	// Get all strings
	virtual void GetPacksToCook( Uint32 locale, const String& stringsView, THashMap< Uint32, LanguagePack* >& packs );

	// New, better language pack access
	LanguagePack*	GetLanguagePackSync( Uint32 stringId, Bool onlyText = false, const String& locale = String::EMPTY );
	LanguagePack*	GetLanguagePackAsync( Uint32 stringId );
	void			ReleaseLanguagePack( Uint32 stringId );
	Bool			PreloadLanguagePacksAsync( const Uint64 batchId, const TDynArray< Uint32 >& stringIds );
	Bool			PreloadLanguagePacksSync( const Uint64 batchId, const TDynArray< Uint32 >& stringIds );

	Bool			ValidateLanguagePackLoad( Uint32 stringId );
	Bool			GetCachedLanguagePackDuration( Uint32 stringId, Float& duration ) const;

	LanguagePack*	CreateLanguagePack();
	void			DeleteLanguagePack( LanguagePack* pack );
	LanguagePack*	GetEmptyLanguagePack();

public:
	// Localized contents management methods
	String GetString( Uint32 stringId, Bool getModified = true );

	void MarkFallbackString( Uint32 stringId, Bool isFallback );

	Uint32 RegisterNewString();
	void ModifyString( Uint32 stringId, const String& newString );
	Uint32 CopyString( Uint32 stringId );
	void DiscardStringModification( Uint32 stringId );

	Bool IsMissingString( Uint32 stringId );
	Bool IsFallbackString( Uint32 stringId );
	Bool IsModifiedString( Uint32 stringId );
	String	GetVoiceoverFilename( Uint32 stringId );

	Bool ShouldIgnoreLanguagePackResources() const { return m_ignoreLanguagePackResources; }
	void SetIgnoreLanguagePackResources( Bool ignore ) { m_ignoreLanguagePackResources = ignore; }

	CLocalizationStorage& GetStorage() { return m_localizationStorage; }

public:
	void			InitializeLocStringsWithKeysCache();
	Uint32			GetStringIdByStringKey( const String &stringKey );
	Bool			DoesStringKeyExist( const String &stringKey );
	String			GetStringByStringKey( const String &stringKey, const String &locale );
	String			GetStringByStringKey( const String &stringKey );
	String			GetStringByStringKeyCached( const String &stringKey );
	String			GetStringByStringKeyCachedWithFallback( const String &stringKey );
	void			ReadAllStringsWithStringKeys( TDynArray< Uint32 > &stringsIds /* out */, TDynArray< String > &stringsKeys /* out */, TDynArray< String > &stringsCategories /* out */ );
	void			ReadAllStringsCategories( TDynArray< String > &stringsCategories /* out */, Bool OnlyKeys = true /* in */ );
	void			SearchForStringsByCategory( const String& searchQuery, const TDynArray< String > &categories /* in */, TDynArray< Uint32 > &ids /* out */, TDynArray< String >* keys = NULL /* out */, TDynArray< String >* strings = NULL /* out */, Bool searchKeys = false /* in */, ESearchOrder order = SearchOrder_Ids /* in */ );

	bool			IsSQLConnectionValid();

	Bool GetConfig_UsePlaceholdersForMissingStrings() const;

private:
	CLocalizedStringsWithKeysCache *m_locStringsWithKeysManager;

#ifndef NO_DEBUG_PAGES
public:
	void FillDebugSummary( LanguagePackDebugSummary& summary );
#endif
};

typedef TSingleton< CLocalizationManager > SLocalizationManager;
