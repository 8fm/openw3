/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __COOK_COMMANDLET_H__
#define __COOK_COMMANDLET_H__

#include "../../common/core/commandlet.h"
#include "../../common/core/datetime.h"

#include "cookFramework.h"

class CCookerDataBase;
class CCookerSeedFile;
class CCookerErrorReporter;

/// List based resource cooker
class CCookCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CCookCommandlet, ICommandlet, 0 );

public:
	CCookCommandlet();
	~CCookCommandlet();

	// interface
	virtual Bool Execute( const CommandletOptions& options );
	virtual const Char* GetOneLiner() const { return TXT("Cook file lists, generate cooked data and bundle files for packing"); }
	virtual void PrintHelp() const;

private:
	struct Settings
	{
		Bool									m_verbose;

		ECookingPlatform						m_cookingPlatform;
		ECookingPlatform						m_physicalCookingPlatform;
		Bool									m_useTextureCache;

		String									m_outputPath;		// absolute output path (by default - temp/cooked<platform>/)
		String									m_stringIDsPath;
		String									m_stringKeysPath;

		String                                  m_trimDirPath;
		String                                  m_excludeDirPath;

		Bool									m_ignoreCookingErrors;
		Bool									m_cookDependencies; // follow file dependencies when cooking
		Bool									m_createDummyImports;

		Bool									m_skipExistingFiles;

		Bool									m_noDataBase;	// do not use cooking data base
		String									m_dataBaseFileName; // cook.db by default

		String									m_additionalDataBasePath;

		Uint32									m_dynamicMemoryLimitCPU; // in MB, GC is called once we reach this limit
		Uint32									m_dynamicMemoryLimitGPU; // in MB, GC is called once we reach this limit

		typedef THashSet< String > TIgnoredExtensions;
		TIgnoredExtensions						m_ignoreFileExtensions;

		TDynArray< String >						m_seedFilePaths;

		TDynArray< String >						m_manualFiles;		// manual list of files to cook (debug testing)

		String									m_customBaseDirectory; // custom base directory for resource loading (if they are NOT loaded from the depot)

		typedef THashSet< String > TExclusiveExtensions;
		TExclusiveExtensions					m_cookedExtension;	// custom list of cooked extension

		struct WorldRemap
		{
			String	m_depotPath;
			String	m_newAbsolutePath;
		};

		typedef TDynArray< WorldRemap >	TWorldRemaps;
		TWorldRemaps							m_worldRemapping;

		Settings();

		Bool Parse( const ICommandlet::CommandletOptions& options );
		Bool ShouldIgnoreExtension( const String& ext ) const;
	};

	// cooker settings
	Settings		m_settings;

	struct CookedBundleInfo;
	struct CookedResourceInfo;

	typedef Red::Threads::CMutex		Mutex;

	class CookingStats : public ICookerStatCollector
	{
	public:
		struct PerClassStat
		{
			const CClass*	m_class;
			Uint32			m_count;
			Double			m_time;
		};

		Double	m_totalLoadingTime;
		Double	m_totalSavingTime;
		Double	m_totalCopyTime;
		Double	m_totalCookingTime;
		Uint32	m_totalCookedObjects;

		typedef THashMap< const CClass*, PerClassStat > TPerClassStats;
		TPerClassStats	m_stats;

		CookingStats()
		{
			Clear();
		}

		void Clear();
		void AddLoading( Double time );
		void AddSaving( Double time );
		void AddCopying( Double time );
		void Dump();

	private:
		virtual void ReportCookingTime( const CClass* objectClass, const Double time ) override;
	};

	struct CookedResourceInfo
	{
		Bool			m_isSeedFile;	// is this a original seed file
		Bool			m_isInHotList;	// is this file in hot list
		Bool			m_isCooked;		// file was already cooked
		Bool			m_isPending;	// we still need to cook this file

		String			m_path;		// conformed path (lower case, UNIX separators)
		StringAnsi		m_pathAnsi; // just for the ease of conversion

		Uint64			m_sourceHash; // meta data - used for future DLC/patch tools
		CDateTime		m_sourceDateTime; // meta data - used for future DLC/patch tools

		typedef TDynArray<CookedBundleInfo*> TBundles;
		TBundles		m_bundles; // bundles that are using this file

		typedef TDynArray<CookedResourceInfo*> TDependency;
		TDependency		m_hardDependencies; // THandle, ptr - always put in the same bundle
		TDependency		m_softDependencies; // TSoftHandle - we may consider not putting it in the same bundle

		Uint32			m_numHardImports; // number of time this file is used as a hard dependency in some other file
		Uint32			m_numSoftImports; // number of time this file is used as a soft dependency in some other file

		CDiskFile*		m_depotFile; // cached depot file entry (only for files from depot)

		CookedResourceInfo(const String& depotFilePath);
		~CookedResourceInfo();
	};

	typedef THashMap< String, CookedResourceInfo* >		TCookedResources;
	TCookedResources		m_files;

	typedef TDynArray< CookedResourceInfo* >			TPendingCookFiles;
	TPendingCookFiles		m_pendingFilesToCook;

	typedef TList< CookedResourceInfo* >			THotList;
	THotList				m_hotList;
	Mutex					m_hotListLock;

	String					m_dataBasePath;
	CCookerDataBase*		m_dataBase;	// valid only if needed

	CCookerDataBase*		m_additionalDataBase;	// other database that provides already cooked data (i.e. game cook.db for dlc)

	CCookerErrorReporter*	m_errors;	// error reporter

	Uint32					m_fileCounter; // number of files processes (forces a DB flush)
	Uint32					m_lastFlushCounter;

	typedef TDynArray< Uint32 > TSpeechIDs;
	typedef THashMap< String, TSpeechIDs > TSpeechMap;
	TSpeechMap				m_speechMap;

	typedef TDynArray< String > TStringKeys;
	typedef THashMap< String, TStringKeys > TStringKeysMap;
	TStringKeysMap			m_stringKeysMap;

	IDependencyImportLoader*	m_dependencyLoaderWithFakeFiles;

	CookingStats			m_stats;

	// internal state clean
	void Reset();

	// check memory constraints and cleanup it up if needed
	void CleanupMemory();

	// add source files from cook list
	Bool AddSeedFile( const String& cookListFile );

	// add file to final cooking output 
	CookedResourceInfo* AddFile( const String& depotPath );

	// get file to process (uses hot list if available)
	CookedResourceInfo* GetFileToProcess( Uint32& localPendingIndex, TPendingCookFiles& localPendingList );

	// cook the pending files
	Bool CookPendingFiles();

	// load resource for cooking
	THandle< CResource > LoadResourceForCooking( const String& path );

	// cook single files
	Bool CookSingleFile( CookedResourceInfo* resourceInfo );

	// post process created file
	Bool PostProcessFile( const String& sourcePath, const String& outputPath, IFile* outputFile = nullptr );

	// process existing file
	Bool ProcessExistingFile( CookedResourceInfo* resourceInfo );

	// is this file a popper resource file ?
	Bool IsResourceFile( const CookedResourceInfo* resourceInfo, const String& fileExtension ) const;

	// cook this file as a non-resource file (just copy)
	Bool CookNonResourceFile( CookedResourceInfo* resourceInfo );

	// HACK!! cook loose texture file HACK!! 
	Bool CookLooseTextureFile( CookedResourceInfo* resourceInfo );

	// Directly copy file from source location to target location, updates the DB
	Bool CopyLooseFile( CookedResourceInfo* resourceInfo );

	// resolve hard dependency
	CookedResourceInfo* ResolveHardDependency( const CookedResourceInfo* fileBeingCooked, const String& depdenencyFile );

	// resolve soft dependency
	CookedResourceInfo* ResolveSoftDependency( const CookedResourceInfo* fileBeingCooked, const String& depdenencyFile );

	// check if a file type can be cooked if it's referenced just as as dependency
	Bool FileTypeRequiresDirectDependency( const String& fileExtension ) const;

	// save speech map contents into a json file
	Bool SaveSpeechMap( );

	// save string keys map contents into a json file
	Bool SaveStringKeysMap( );

	// process a candidate for a hot list
	void ProcessHotListCandidate( CookedResourceInfo* file );

	// create output file for writing cooked result
	IFile* CreateOutputFile( const String& depotPath, const Bool asyncWrite );

	// preserve cooker DB (flush it from time to time)
	void PreserveState();
};

BEGIN_CLASS_RTTI( CCookCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI();

#endif