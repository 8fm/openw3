/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_EDITOR

#include "commandlet.h"

/// Output list from analyzer file
class CAnalyzerOutputList
{
public:
	CAnalyzerOutputList();
	~CAnalyzerOutputList();

	typedef TDynArray< CName > TChunkIDs;

	// Set current bundle name (all files are added to the same output bundle)
	void SetBundleName( const StringAnsi& bundleName );

	// Set current content chunks
	void SetContentChunks( const TChunkIDs* fileChunkIDs = nullptr );

	// Set current content chunk (single chunk)
	void SetContentChunk( const CName contentChunk );

	// Add file to the list under current bundle, returns false if file is already there, true if file is new
	bool AddFile( const StringAnsi& fileDepotPath );

	// Check if file exists in the current bundle
	bool FileExistsInCurrentBundle( const StringAnsi& fileDepotPath ) const;

	// Check if file exists in the current bundle
	bool FileExists( const StringAnsi& fileDepotPath ) const;

	// Get number of defined bundles
	const Uint32 GetNumBundles() const;

	// Get name of the bundle
	const StringAnsi& GetBundleName( const Uint32 bundleIndex ) const;

	// Get number of files defined in bundle
	const Uint32 GetBundleFileCount( const Uint32 bundleIndex ) const;

	// Get number of files defined in bundle
	const StringAnsi& GetBundleFileName( const Uint32 bundleIndex, const Uint32 fileIndex ) const;

	// Get the list of chunk IDs of a file in bundle.
	const TChunkIDs* GetBundleFileChunkIDs( const Uint32 bundleIndex, const Uint32 fileIndex ) const;

	// Override chunk IDs for all files currently collected
	void OverrideChunkIds( const TChunkIDs& fileChunkIDs );

	// Sort entries alphanumerically
	void SortEntries( );

private:
	struct BundleInfo;

	struct FileInfo
	{
		StringAnsi	m_depotPath;	// Conformed.
		TChunkIDs	m_chunkIDs;		// Data chunks in which this file appears.

		FileInfo( const StringAnsi& path, const TChunkIDs* chunkIDs  )
			: m_depotPath( path )
		{
			if( chunkIDs != nullptr && !chunkIDs->Empty() )
			{
				m_chunkIDs = *chunkIDs;
			}
		}
	};

	typedef THashMap<StringAnsi, FileInfo*>		TFileMap;
	typedef TDynArray<FileInfo*>				TFiles;

	struct BundleInfo
	{
		StringAnsi	m_bundleName;
		TFiles		m_files;
		TFileMap	m_fileMap;

		BundleInfo( const StringAnsi& name )
			: m_bundleName( name )
		{}

		~BundleInfo()
		{
			m_files.ClearPtr();
		}
	};

	typedef THashMap<StringAnsi, BundleInfo*>	TBundleMap;
	typedef TDynArray<BundleInfo*>				TBundles;

	BundleInfo*	m_currentBundle;
	TChunkIDs	m_currentChunks;

	TBundleMap	m_bundleMap;
	TBundles	m_bundles;
};

/// Base class for analyzer tool
class IAnalyzer
{
	DECLARE_RTTI_SIMPLE_CLASS( IAnalyzer );

public:
	IAnalyzer();

	// entry point
	virtual bool DoAnalyze( const ICommandlet::CommandletOptions& options, CAnalyzerOutputList& outputList ) = 0;

	// interface
	virtual const Char* GetName() const = 0;
	virtual const Char* GetDescription() const = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI(IAnalyzer);
END_CLASS_RTTI();

#endif