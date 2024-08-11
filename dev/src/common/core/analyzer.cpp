/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "analyzer.h"
#include "bundledefinition.h"
#include "filePath.h"

#ifndef NO_EDITOR

//---

CAnalyzerOutputList::CAnalyzerOutputList()
	: m_currentBundle( NULL )
{
}

CAnalyzerOutputList::~CAnalyzerOutputList()
{
	m_bundles.ClearPtr();
}

void CAnalyzerOutputList::SetBundleName( const StringAnsi& bundleName )
{
	// conform the bundle name (lower case)
	StringAnsi tempBundleName;
	const StringAnsi& conformedName = CFilePath::ConformPath( bundleName, tempBundleName );

	// current bundle
	if ( m_currentBundle && m_currentBundle->m_bundleName == conformedName )
		return;

	// find existing bundle
	BundleInfo* bundleInfo = nullptr;
	if ( !m_bundleMap.Find( conformedName, bundleInfo ) )
	{
		// create new bundle
		bundleInfo = new BundleInfo( conformedName );
		m_bundleMap.Insert( conformedName, bundleInfo );
		m_bundles.PushBack( bundleInfo );
	}

	// set new bundle
	m_currentBundle = bundleInfo;
}

void CAnalyzerOutputList::SetContentChunks( const TChunkIDs* fileChunkIDs /*= nullptr*/ )
{
	m_currentChunks.Clear();
	if ( fileChunkIDs )
	{
		m_currentChunks = *fileChunkIDs;
	}
}

void CAnalyzerOutputList::SetContentChunk( const CName contentChunk )
{
	m_currentChunks.Clear();
	if ( contentChunk )
	{
		m_currentChunks.PushBack( contentChunk );
	}
}

bool CAnalyzerOutputList::AddFile( const StringAnsi& fileDepotPath )
{
	// no bundle
	if ( !m_currentBundle )
	{
		WARN_CORE( TXT("No active bundle, file '%ls' will be ignored"), fileDepotPath.AsChar() );
		return false;
	}

	// conform the file path to common standard
	StringAnsi tempPathBuffer;
	const StringAnsi& conformedPath = CFilePath::ConformPath( fileDepotPath, tempPathBuffer );

	// already in list ?
	if ( nullptr != m_currentBundle->m_fileMap.FindPtr( conformedPath ) )
		return false;

	// add file to list
	FileInfo* fileInfo = new FileInfo( conformedPath, &m_currentChunks );
	m_currentBundle->m_fileMap.Insert( conformedPath, fileInfo );
	m_currentBundle->m_files.PushBack( fileInfo );

	// added
	return true;
}

bool CAnalyzerOutputList::FileExistsInCurrentBundle( const StringAnsi& fileDepotPath ) const
{
	// no bundle
	if ( !m_currentBundle )
		return false;

	// conform the file path to common standard
	StringAnsi tempPathBuffer;
	const StringAnsi& conformedPath = CFilePath::ConformPath( fileDepotPath, tempPathBuffer );

	// already in list ?
	return ( nullptr != m_currentBundle->m_fileMap.FindPtr( conformedPath ) );
}

bool CAnalyzerOutputList::FileExists( const StringAnsi& fileDepotPath ) const
{
	// conform the file path to common standard
	StringAnsi tempPathBuffer;
	const StringAnsi& conformedPath = CFilePath::ConformPath( fileDepotPath, tempPathBuffer );

	// search in all bundles we currently have
	for ( TBundles::const_iterator it = m_bundles.Begin();
		it != m_bundles.End(); ++it )
	{
		const BundleInfo* budleInfo = (*it);

		if ( nullptr != budleInfo->m_fileMap.FindPtr( conformedPath ) )
			return true;
	}

	// not found
	return false;
}

const Uint32 CAnalyzerOutputList::GetNumBundles() const
{
	return m_bundles.Size();
}

const StringAnsi& CAnalyzerOutputList::GetBundleName( const Uint32 bundleIndex ) const
{
	RED_ASSERT( bundleIndex < m_bundles.Size() );

	if ( bundleIndex >= m_bundles.Size() )
		return StringAnsi::EMPTY;

	return m_bundles[ bundleIndex ]->m_bundleName;
}

const Uint32 CAnalyzerOutputList::GetBundleFileCount( const Uint32 bundleIndex ) const
{
	RED_ASSERT( bundleIndex < m_bundles.Size() );

	if ( bundleIndex >= m_bundles.Size() )
		return 0;

	return m_bundles[ bundleIndex ]->m_files.Size();
}

const StringAnsi& CAnalyzerOutputList::GetBundleFileName( const Uint32 bundleIndex, const Uint32 fileIndex ) const
{
	RED_ASSERT( bundleIndex < m_bundles.Size() );

	if ( bundleIndex >= m_bundles.Size() )
		return StringAnsi::EMPTY;

	const BundleInfo* bundle = m_bundles[ bundleIndex ];

	RED_ASSERT( fileIndex < bundle->m_files.Size() );

	if ( fileIndex >= bundle->m_files.Size() )
		return StringAnsi::EMPTY;

	return bundle->m_files[ fileIndex ]->m_depotPath;
}

const CAnalyzerOutputList::TChunkIDs* CAnalyzerOutputList::GetBundleFileChunkIDs( const Uint32 bundleIndex, const Uint32 fileIndex ) const
{
	RED_ASSERT( bundleIndex < m_bundles.Size() );

	if ( bundleIndex >= m_bundles.Size() )
		return nullptr;

	const BundleInfo* bundle = m_bundles[ bundleIndex ];

	RED_ASSERT( fileIndex < bundle->m_files.Size() );

	if ( fileIndex >= bundle->m_files.Size() )
		return nullptr;

	return &bundle->m_files[ fileIndex ]->m_chunkIDs;
}

void CAnalyzerOutputList::OverrideChunkIds( const TChunkIDs& fileChunkIDs )
{
	for ( BundleInfo* bundle : m_bundles )
	{
		for ( FileInfo* file : bundle->m_files )
		{
			file->m_chunkIDs = fileChunkIDs;
		}
	}
}

void CAnalyzerOutputList::SortEntries( )
{
	for ( BundleInfo* bundle : m_bundles )
	{
		::Sort( bundle->m_files.Begin( ), bundle->m_files.End( ), [ ] ( const FileInfo* a, const FileInfo* b ) { return a->m_depotPath < b->m_depotPath; } );
	}
}

//---

IMPLEMENT_RTTI_CLASS(IAnalyzer);

IAnalyzer::IAnalyzer()
{
}

#endif