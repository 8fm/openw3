/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SPLIT_FILES_COMMANDLET_H__
#define __SPLIT_FILES_COMMANDLET_H__

#include "../../common/core/commandlet.h"
#include "../../common/core/datetime.h"

#include "cookDataBase.h"
#include "cookSplitList.h"

/// Command let for building final file distribution file list
class CSplitFilesCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CSplitFilesCommandlet, ICommandlet, 0 );

public:
	CSplitFilesCommandlet();
	~CSplitFilesCommandlet();

	// interface
	virtual Bool Execute( const CommandletOptions& options );
	virtual const Char* GetOneLiner() const { return TXT("Split the files into content buckets"); }
	virtual void PrintHelp() const;

private:
	struct BundleInfo;

	typedef CCookerDataBase::TCookerDataBaseID				DbID;
	typedef Red::Core::ResourceManagement::CResourceId		ResourceId;
	
	// chunk mask
	struct ChunkMask
	{
		static const Uint32 MAX_CHUNKS	= 32;

		TBitSet< MAX_CHUNKS >	m_chunks;

		RED_INLINE ChunkMask()
		{}

		RED_INLINE ChunkMask( const CName& chunkName )
		{
			const Int32 chunkIndex = GetChunkIndex( chunkName );
			if ( chunkIndex != -1 )
			{
				m_chunks.Set( chunkIndex );
			}
		}

		RED_INLINE const Int32 GetFinalChunk() const
		{
			for ( Uint32 i=0; i<MAX_CHUNKS; ++i )
			{
				if ( m_chunks.Get(i) )
					return i;
			}

			return -1;
		}

		RED_INLINE Bool Merge( const ChunkMask& other )
		{
			Bool wasMerged = false;
			for ( Uint32 i=0; i<MAX_CHUNKS; ++i )
			{
				if ( other.m_chunks.Get(i) && !m_chunks.Get(i) )
				{
					m_chunks.Set(i);
					wasMerged = true;
				}
			}
			
			return wasMerged;
		}

		static Int32 GetChunkIndex( const CName& chunkName );
		static CName GetChunkdName( const Uint32 chunkIndex );
	};

	// file in bundle, wrapper, populated from cooker data base
	struct FileInfo
	{
		DbID					m_dbId;		// entry in the data base
		ResourceId				m_fileId;
		StringAnsi				m_filePath;
		CName					m_fileClass;
		Uint32					m_fileSize;

		typedef TDynArray< FileInfo* > TDependencies;
		TDependencies			m_hardDependencies;
		TDependencies			m_softDependencies;

		Bool					m_isSeedFile;
		ChunkMask				m_chunks;
		ChunkMask				m_tempChunks;

		Int32					m_resolvedChunkIndex;
		CName					m_resolvedChunk;

		RED_INLINE FileInfo( const DbID& dbId, const ResourceId& fileId, const StringAnsi& path )
			: m_dbId( dbId )
			, m_fileId( fileId )
			, m_filePath( path )
			, m_fileSize( 0 )
			, m_isSeedFile( false )
			, m_resolvedChunkIndex( -1 )
			, m_resolvedChunk( CName::NONE )
		{}

		static Bool ShouldPropagateChunkDependency( CName fromResourceType, CName toResourceType, const Bool secondPass );
		
		void PropagateChunks( const FileInfo* parentFile, ChunkMask chunkMask, const Bool isSoftDependency, const Bool secondPass  );
	};

	typedef TDynArray< FileInfo* > TAllFiles;
	typedef THashMap< ResourceId, FileInfo* > TAllFilesMap;

	// files map
	TAllFiles				m_allFiles;
	TAllFilesMap			m_allFilesMap;

	// fast bundle compression (zlib)
	Bool					m_fastCompression;

	// read only files
	CCookerDataBase			m_dataBase;

	// create a file wrapper
	FileInfo* GetFile( const ResourceId& id, const StringAnsi& depotPath );

	// find a wrapper file
	FileInfo* FindFileInfo( const ResourceId& id ) const;

	// load stuff from cook.db
	Bool LoadCookDB( const String& absolutePath );

	// load stuff from seed files
	Bool LoadSeedFiles( const TList< String >& seedFiles );

	// resolve final chunk for each file
	Bool ResolveFinalChunks();

	// validate
	Bool ValidateSplit();

	// export bundle file definitions
	Bool WriteOutputFile( const String& outputFilePath );
};

BEGIN_CLASS_RTTI( CSplitFilesCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI();

#endif