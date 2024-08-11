/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __EXPORT_BUNDLES_COMMANDLET_H__
#define __EXPORT_BUNDLES_COMMANDLET_H__

#include "../../common/core/commandlet.h"
#include "../../common/core/datetime.h"

#include "cookDataBase.h"
#include "cookSplitList.h"

/// Command let for building final bundle list from seed files, cooker data base and spatial distrubiution
class CExportBundlesCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CExportBundlesCommandlet, ICommandlet, 0 );

public:
	CExportBundlesCommandlet();
	~CExportBundlesCommandlet();

	// interface
	virtual Bool Execute( const CommandletOptions& options );
	virtual const Char* GetOneLiner() const { return TXT("Build final bundle lists"); }
	virtual void PrintHelp() const;

private:
	struct BundleInfo;

	typedef CCookerDataBase::TCookerDataBaseID				DbID;
	typedef Red::Core::ResourceManagement::CResourceId		ResourceId;
	typedef TDynArray< CName >								TChunkIDs;

	// file in bundle, wrapper, populated from cooker data base
	struct FileInfo
	{
		DbID					m_dbId;		// entry in the data base
		ResourceId				m_fileId;
		StringAnsi				m_filePath;
		Int32					m_fileIndex;  // original file index
		Uint32					m_fileSize;
		
		typedef TDynArray< BundleInfo* > TBundles;
		TBundles				m_seedBundles; // only for seed files
		TBundles				m_bundles; // final bundles

		typedef TDynArray< FileInfo* > TDependencies;
		TDependencies			m_hardDependencies;
		TDependencies			m_softDependencies;
		TDependencies			m_bufferDependencies;

		Bool					m_isUsed; // file is used at least once
		Bool					m_isOrphaned; // file is an orphan file
		Bool					m_isConsumed; // file was consumed by the earlier stages of the pipeline (dds, jpg, etc)

		Bool					m_isNotPropagating; // this file is not propagating dependencies (debug file)

		FileInfo( const DbID& dbId, const ResourceId& fileId, const StringAnsi& path )
			: m_dbId( dbId )
			, m_fileId( fileId )
			, m_filePath( path )
			, m_isUsed( false )
			, m_isOrphaned( false )
			, m_isConsumed( false )
			, m_isNotPropagating( false )
			, m_fileIndex( -1 )
		{}

		typedef std::function< bool( FileInfo* ) > VisitorFunction;

		template< bool H, bool S, bool B >
		RED_INLINE void Visit( VisitorFunction visitor )
		{
			if ( !visitor( this ) )
				return;

			if ( H )
			{
				for ( auto it : m_hardDependencies )
					it->Visit<H,S,B>( visitor );
			}

			if ( S )
			{
				for ( auto it : m_softDependencies )
					it->Visit<H,S,B>( visitor );
			}

			if ( B )
			{
				for ( auto it : m_bufferDependencies )
					it->Visit<H,S,B>( visitor );
			}
		}

		void AddToBundle( BundleInfo* bundle );
		void RemoveFromBundle( BundleInfo* bundle );
		void RemoveFromAllBundles();

		static Bool CompareFileIndex( const FileInfo* lh, const FileInfo* rh )
		{
			return ( lh->m_fileIndex < rh->m_fileIndex );
		}

		const Bool IsInBundles() const;
		const Bool IsInSomeAlwaysAccessibleBundle() const;
		const Bool IsInSeedBundles() const;
	};

	typedef TDynArray< FileInfo* > TAllFiles;
	typedef THashMap< ResourceId, FileInfo* > TAllFilesMap;

	// final bundle definition just before it's exported
	struct BundleInfo
	{
		StringAnsi				m_name;
		CName					m_forcedChunk;	// used if we cannot split this bundle

		typedef TDynArray< FileInfo* >		TFiles;
		TFiles					m_files;

		typedef TDynArray< BundleInfo* >	TBundles;
		TBundles				m_splitBundles;
		CName					m_splitTag;

		BundleInfo( const StringAnsi& name )
			: m_name( name )
		{}

		const Bool IsAlwaysAccessible() const
		{
			return (m_forcedChunk == CName::NONE);
		}

		Uint64 CalcDataSize() const
		{
			Uint64 fileSize = 0;

			for ( FileInfo* file : m_files )
			{
				fileSize += file->m_fileSize;
			}

			return fileSize;
		}
	};

	typedef TDynArray< BundleInfo* > TAllBundles;
	typedef THashMap< StringAnsi, BundleInfo* > TAllBundlesMap;
	typedef TDynArray< StringAnsi > TExclusiveFileTypes;

	// list of bundles
	class BundleListHelper
	{
	public:
		TDynArray< const BundleInfo* >	m_allBundles;
		TDynArray< const BundleInfo* >	m_scenarioBundles;

	public:
		BundleListHelper( const TAllBundles& bundles );

		RED_INLINE const TDynArray< const BundleInfo* >& GetScenarioBundles() const { return m_scenarioBundles; }

		void ResetScenario();
		void AddContentDir( const StringAnsi& contentDir );
		void AddWorld( const StringAnsi& worldPrefixName );
	};

	// files map
	TAllFiles				m_allFiles;
	TAllFilesMap			m_allFilesMap;

	// seed files
	TAllFiles				m_seedFiles;

	// bundles map
	TAllBundles				m_allBundles;
	TAllBundlesMap			m_allBundlesMap;

	// fast bundle compression (zlib)
	Bool					m_fastCompression;

	// read only files
	CCookerDataBase			m_dataBase;

	// create/get a bundle object
	BundleInfo* GetBundle( const StringAnsi& bundleName, const Bool createIfNotFound = true );

	// create a file wrapper
	FileInfo* GetFile( const ResourceId& id, const StringAnsi& depotPath );

	// mark internal bundle flags
	Bool MarkBundleFlags();

	// determine which files are used
	Bool MarkUsedFiles();

	// fill initial startup bundle
	Bool FillStartupBundle();

	// fill buffers bundle
	Bool FillBuffersBundle();

	// propagate resources to bundles
	Bool FillBundles();

	// optimize bundles
	Bool OptimizeBundles();

	// Make the bundle a base bundle in respect to other bundle or to all bundles, files in the base bundle are exclusive
	Bool MakeBaseBundle( BundleInfo* bundle, const BundleInfo* withRespectToBundle );

	// split bundle
	Bool SplitBundle( BundleInfo* bundle, const CCookerSplitFile& splitDB );

	// split bundles by the resolved chunk IDs of their files
	Bool SplitBundlesIntoChunks( const CCookerSplitFile& splitDB );

	// export bundle file definitions
	Bool WriteOutputFile( const String& outputFilePath );

	// split single bundle into multiple parts
	Bool SplitLargeBundle( BundleInfo* sourceBundleInfo, const Uint64 bundleSizeLimit );

	// split large bundles (over 4GB) into smaller bundles
	Bool SplitLargeBundles();

	// validate bundles
	Bool ValidateBundles();

	// validate bundle scenario - make sure all files are always accessible
	Bool ValidateBundleScenario( const StringAnsi& scenarioName, const TDynArray< const BundleInfo* >& mountedBundles );
};

BEGIN_CLASS_RTTI( CExportBundlesCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI();

#endif