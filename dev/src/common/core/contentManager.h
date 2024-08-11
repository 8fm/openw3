/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "streamingInstaller.h"
#include "tagList.h"
#include "bitset.h"
#include "hashmap.h"
#include "contentManifest.h"

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class IContentListener;
class IContentInstaller;
class CContentManifestAsyncLoader;
struct SContentPackageEvent;
class CDepotBundles;

// Alias for the launch content including associated language chunks. E.g., content3 for the current language.
// Maybe not be fully installed if switching languages in game on the PS4 or even when launching on the Xbox.
//RED_DECLARE_NAME( launch0 );

//////////////////////////////////////////////////////////////////////////
// EContentMask
//////////////////////////////////////////////////////////////////////////
enum EContentMask
{
	eContentMask_BootUp,
	eContentMask_Default,
};

//////////////////////////////////////////////////////////////////////////
// EContentSource
//////////////////////////////////////////////////////////////////////////
enum EContentSource
{
	eContentSource_Installer,	//!< Registered installers
	eContentSource_LooseFiles,	//!< Loose files, e.g., the editor
	eContentSource_Cook,		//!< Cooked files
	eContentSource_SplitCook,	//!< Split cook for streaming content installers
};

//////////////////////////////////////////////////////////////////////////
// EContentTaintedFlags
//////////////////////////////////////////////////////////////////////////
enum EContentTaintedFlags
{
	// NOTE: These flags are serialized into savegames, don't alter the values unless you later create some mapping function

	eContentTaintedFlag_BeforeTaintedFlags		= FLAG(0),	//!< Old save, don't trust
	eContentTaintedFlag_AllContentActivated		= FLAG(1),	//!< All content activated, probably debug quest or cheats
	eContentTaintedFlag_AllContentAvailable		= FLAG(2),	//!< All content available, probably editor, x64, or cheats
	eContentTaintedFlag_Scanned					= FLAG(3),	//!< Scanned for content instead of using installer
	eContentTaintedFlag_Skipped					= FLAG(4),	//!< Skipped content stalling through cheats.
	eContentTaintedFlag_ContentActivated		= FLAG(5),	//!< Individual content was activated. Necessary for now as a debug cheat to fix a savegame.
	eContentTaintedFlag_Mods					= FLAG(6),	//!< Mods were loaded.

	eContentTaintedFlagGroup_Activated = eContentTaintedFlag_Skipped | eContentTaintedFlag_AllContentActivated |
										 eContentTaintedFlag_AllContentAvailable | eContentTaintedFlag_ContentActivated,
};

//////////////////////////////////////////////////////////////////////////
// CContentManager
//////////////////////////////////////////////////////////////////////////
class CContentManager
{
	DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_Engine );

public:
	const TDynArray< CName >& GetBaseContent() const { return m_baseContent; }

public:
	void									RegisterContentListener( IContentListener* listener, const AnsiChar* fileFilterWildcards = "" );
	void									RegisterContentListener( IContentListener* listener, const AnsiChar* fileFilterWildcards[], Uint32 numFileFilterWildcards );

	void									UnregisterContentListener( IContentListener* listener );

public:
	void									RegisterContentInstaller( IContentInstaller* contentInstaller );

public:
											CContentManager();
	virtual									~CContentManager();
	virtual Bool							Init();

	void									ScanFoldersForContent( EContentSource contentSource );

public:
	Bool									IsReady( EContentMask contentMask = eContentMask_Default ) const;

public:
	Uint32									GetTaintedFlags() const { return m_taintedFlags; }
	void									AddTaintedFlags( Uint32 flags );

public:
	void									DumpProgressToLog();

public:
	virtual void							OnSuspend();
	virtual void							OnResume();
	virtual void							OnEnterConstrain();
	virtual void							OnExitConstrain();

public:
	void									Update();
	Bool									ActivateContent( CName content );
	void									RefreshActivatedContent();
	void									ResetActivatedContent();
	const TDynArray< CName >&				GetActivatedContent() const { return m_activatedContent.GetTags(); }
	Bool									GetValidDefaultGameLanguage( String& outTextLanguage, String& outSpeechLanguage ) const;
	Bool									GetSupportedLanguages( TDynArray< String >& outTextLangauges, TDynArray< String >& outSpeechLanguages, String& outDefaultLanguage ) const;
	void									SetSpeechInstallLanguage( const String& speechLang );
	void									SetSystemLanguages( const String& text, const String& speech );

	// For now no update event as such, but on user sign out should go back to the start menu
	// Can recheck licensed packages when new content is attached
	void									GetLicensedPackages( TDynArray< RuntimePackageID >& outPackagesIDs ) const;

public:
	// This is for custom quest definitions until they modify the redgame to activate content as needed. Probably never to be removed.
	void									ActivateAllContentForDebugQuests();

public:
	Bool									IsContentAvailable( CName contentName ) const;
	Bool									IsContentActivated( CName contentName ) const;
	EContentStallType						GetStallForMoreContent() const;
	Uint32									GetPercentCompleted( CName contentName ) const;
	CName									GetResolvedLaunchZero() const { return m_resolvedLaunchZero; }

public:
	Bool									RequiresUncookedScripts() const { return m_requiresUncookedScripts; }
	void									EnumScriptFiles( TDynArray<String>& outScriptFiles ) const;
	void									EnumScriptDirectories( THashMap< String, String >& outScriptDirectories ) const;

public:
	Bool									HasPatchLevel( Uint32 patchLevel ) { return true; }

private:
	Bool									ScanForContentFiles( EContentSource source, SContentPack& outContentPack );
	Bool									ScanForLooseFiles( SContentPack& outContentPack ) const;
	Bool									ScanForCookFiles( SContentPack& outContentPack ) const;
	Bool									ScanForSplitCookFiles( SContentPack& outContentPack ) const;
	Bool									ScanForDlcFiles( TDynArray< SContentPack >& outContentPacks ) const;

#ifdef RED_MOD_SUPPORT
	Bool									ScanForModFiles( TDynArray< SContentPack >& outContentPacks ) const;
#endif

public:
	Bool									ResolveOverridePath( const String& path, String& outOverridePath ) const;

private:
	Bool									ShouldForceContentAvailable() const;

private:
	void									ProcessPackageEvents( const TDynArray< SContentPackageEvent >& contentPackageEvents );
	void									ProcessFinishedPackages( const TDynArray< SContentPackInstance* >& finishedPackages );
	void									ProcessPendingAttachedContent();

private:
	void									OnPackageMounted( RuntimePackageID packageID, const String& mountPath, Uint32 mountFlags );
	void									OnPackageLicenseChanged( RuntimePackageID packageID, Bool isLicensed );
	void									OnPackageChunkInstalled( RuntimePackageID packageID, CName chunkName );

private:
	void									NotifyListeners();

private:
	void									ScanDiskLanguageFiles( TDynArray< String >& outTextLanguages, TDynArray< String >& outSpeechLanguages ) const;
	Bool									ParseStringsLanguage( const String& path, String& outLang ) const;
	Bool									ParseSpeechesLanguage( const String& path, String& outLang ) const;

private:
	struct SContentChunkContext
	{
		CName						m_contentName;
		SContentPackInstance*		m_packInstance;
		const SContentChunk*		m_chunkPtr;

		SContentChunkContext()
			: m_packInstance( nullptr )
			, m_chunkPtr( nullptr )
		{}

		SContentChunkContext( SContentPackInstance* contentPackInstance, const SContentChunk* chunkPtr )
			: m_packInstance( contentPackInstance )
			, m_chunkPtr( chunkPtr )
		{
			if ( chunkPtr )
			{
				m_contentName = chunkPtr->m_chunkLabel;
			}
		}

		Bool operator==( const SContentChunkContext& rhs ) const 
			{ return m_packInstance->m_packageID == rhs.m_packInstance->m_packageID && m_contentName == rhs.m_contentName; }
	};

private:
	struct SContentListenerContext
	{
		IContentListener*					m_contentListener;
		TDynArray< StringAnsi >				m_fileFilterWildcards;
		Int32								m_maxNotifiedContentChunkIndex;
		Int32								m_maxNotifiedPackageIndex;

		SContentListenerContext()
			: m_contentListener( nullptr )
			, m_maxNotifiedContentChunkIndex( -1 )
			, m_maxNotifiedPackageIndex( -1 )
		{}

		SContentListenerContext( IContentListener* listener, const TDynArray< StringAnsi >& fileFilterWildcards )
			: m_contentListener( listener )
			, m_fileFilterWildcards( fileFilterWildcards )
			, m_maxNotifiedContentChunkIndex( -1 )
			, m_maxNotifiedPackageIndex( -1 )
		{}

		SContentListenerContext( IContentListener* listener, const TDynArray< StringAnsi >&& fileFilterWildcards )
			: m_contentListener( listener )
			, m_fileFilterWildcards( Move( fileFilterWildcards ) )
			, m_maxNotifiedContentChunkIndex( -1 )
			, m_maxNotifiedPackageIndex( -1 )
		{}

		Bool operator==( const SContentListenerContext& rhs ) const { return m_contentListener == rhs.m_contentListener; }
		Bool operator!=( const SContentListenerContext& rhs ) const { return !(*this == rhs); }
	};

private:
	TDynArray< SContentListenerContext >					m_contentListeners;

private:
	EContentSource											m_contentSource;
	TDynArray< IContentInstaller* >							m_registeredContentInstallers;
	TDynArray< String >										m_scannedTextLanguages;
	TDynArray< String >										m_scannedSpeechLanguages;
	String													m_scannedDefaultLanguage;
	String													m_systemTextLanguage;
	String													m_systemSpeechLanguage;

private:
	TagList													m_activatedContent;
	EContentStallType										m_contentStallType;
	Bool													m_listenerAdded;

private:
	TDynArray< SContentChunkContext >						m_attachedChunkContexts;
	TDynArray< RuntimePackageID >							m_attachedPackageIDs;
	CName													m_resolvedLaunchZero;
	mutable Uint32											m_taintedFlags;
	mutable Bool											m_requiresUncookedScripts;

private:
	CContentManifestAsyncLoader*							m_manifestAsyncLoader;
	TArrayMap< RuntimePackageID, SContentPackInstance* >	m_packageInstanceMap;
	TDynArray< TPair< RuntimePackageID, CName > >			m_pendingAttachedContent;

	TDynArray< CName >										m_baseContent;

private:
#ifdef RED_MOD_SUPPORT
	// Hack because this just scans the content dir
	friend class CDepotBundles;

	struct SModConfig
	{
		Uint32	m_priority;
		Bool	m_enabled;

		SModConfig()
			: m_priority(UINT_MAX)
			, m_enabled(true)
		{}
	};

	typedef THashMap< String, SModConfig >	TModConfigMap;
	TModConfigMap							m_modConfigMap;
#endif
};

//////////////////////////////////////////////////////////////////////////
extern CContentManager* GContentManager;
