/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../redio/redIO.h"

/// Preloader for disk bundles
class CDiskBundlePreloader
{
public:
	CDiskBundlePreloader();

	// Preload given bundle, blocks calling thread, internals are asynchronous
	void Preload( Red::IO::CAsyncFileHandleCache::TFileHandle fileHandle, Red::Core::Bundle::BundleID bundleID, TDynArray< THandle< CResource > >& outLoadedResources );

private:
	/// IO chunk
	struct IOChunk
	{
		Uint32				m_diskOffset;
		Uint32				m_diskSize;
		Uint32				m_firstFile;
		Uint32				m_numFiles;
	};

	/// IO state
	struct ReadingState
	{
		void*				m_bufferBase;
		Uint32				m_readStartPos;
		Uint32				m_readEndPos;
		Uint32				m_readBlockSize;
		volatile Uint32*	m_fileMarkerPos;
	};

	// reading functions
	static Red::IO::ECallbackRequest OnDataLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred );

	// build the optimal chunk map
	static void BuildChunkMap( const Uint32 maxChunkSize, const TDynArray< Red::Core::Bundle::SMetadataFileInBundlePlacement >& placement, TDynArray< IOChunk >& outChunks );

	//
	void LoadMissingDependencies( const TDynArray< Red::Core::Bundle::SMetadataFileInBundlePlacement >& filesPlacement );
};