/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "bundleDataCache.h"
#include "diskBundleIOCache.h"
#include "contentListener.h"

class CDiskBundleCache;

namespace Red 
{ 
	namespace Core 
	{ 
		namespace Bundle 
		{
			class CBundleMetadataStore;
		} 
		namespace ResourceManagement
		{
			class CResourceId;
		}
	}
}

/// Bundle support for depot
class CDepotBundles : public Red::NonCopyable
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Depot );

public:
	CDepotBundles( const String& rootPath, Bool splitCook );
	~CDepotBundles();

	// Are the bundles censored ?
	Bool IsCensored() const;

	// Initialize the bundles and map them to the depot
	Bool Initialize( TDepotFileMap& outFileMap );

	// Open a file reader for single bundle file
	// This reader is fully synchronous, this means that the content of the file 
	// will be fully loaded and decompressed before returning. SLOW SLOW SLOW!
	IFile* CreateFileReader( const Red::Core::Bundle::FileID fileID );

	// Create an asynchronous file reader for given file ID
	// Note, as all async file access this can return NotReady so please handle it
	EAsyncReaderResult CreateAsyncFileReader( const Red::Core::Bundle::FileID fileID, const Uint8 ioTag, IAsyncFile*& outAsyncReader );

	// Get bundle wrapper for given ID
	CDiskBundle* GetBundle( const Red::Core::Bundle::BundleID bundleID ) const;

	// Get bundle wrapper for given path (deprecated)
	CDiskBundle* GetBundle( const StringAnsi& bundlePath ) const;

	// Get startup bundle for given world, empty world name argument returns the default engine startup bandle
	CDiskBundle* GetStartupBundle( const StringAnsi& worldName ) const;

	// Enable world filter - allows the runtime bundle for given world to be accessed
	void SwitchWorldBundles( const StringAnsi& worldName );

	// Get buffer FileID for a given buffer in given file
	Red::Core::Bundle::FileID GetBufferFileID( Red::Core::Bundle::FileID fileID, const Uint32 bufferIndex ) const;

	// Get bundle burst read disk range
	void GetBundleBurstReadRange( const Red::Core::Bundle::BundleID bundleID, Uint32& outStartOffset, Uint32& outEndOffset ) const;
	
	// Get file placement information for given bundle for given file data range
	void GetBundleContent( const Red::Core::Bundle::BundleID bundleID, const Uint32 startOffset, const Uint32 endOffset, TDynArray< Red::Core::Bundle::SMetadataFileInBundlePlacement >& outFilesPlacement, TDynArray< CBundleDiskFile* >& outFiles ) const;

	// Get raw content information
	void GetBundleContentAll( const Red::Core::Bundle::BundleID bundleID, TDynArray< Red::Core::Bundle::SMetadataFileInBundlePlacement >& outFilesPlacement ) const;
	
	// Get all bundles registered in the system
	void GetAllBundles( TDynArray< CDiskBundle* >& outBundles ) const;

	// Is bundle mounted ?
	const Bool IsBundleMounted( const Red::Core::Bundle::BundleID bundleID ) const;

	// Get mapped depot file for given File ID
	CBundleDiskFile* GetBundleDiskFile( Red::Core::Bundle::FileID fileID ) const;

	// Get all entries for given file - will also return unmounted ones
	void GetFileEntries( Red::Core::Bundle::FileID fileID, TDynArray< Red::Core::Bundle::SMetadataFileInBundlePlacement >& outFilesPlacement ) const;

	// Cache initial content into the RAM cache
	void CacheContent();

	// Attach static bundle (happens during content installation)
	Bool AttachStaticBundle( const StringAnsi& bundlePath );

	// Attach dynamic bundles from given metadata store, actual bundles are assumed to be under given root directory
	Bool AttachDynamicBundles( const String& storeAbsolutePath, const String& bundleRootDirectory, TDepotFileMap& outFileMap );

	// Attach dynamic override bundles from given metadata store, actual bundles are assumed to be under given root directory
	// Content from those bundles will OVERRIDE any existing content, new FileIDs will not be created for such overridden files
	// Also files that were overridden will be removed from the burst-loading structures
	Bool AttachDynamicOverrideBundles( const String& storeAbsolutePath, const String& bundleRootDirectory, TDepotFileMap& outFileMap );

	// If DLC file was patched it return handler to original version
	CBundleDiskFile* GetOriginalDLCFile( const CBundleDiskFile& file ) const;

private:
	typedef Red::Core::Bundle::FileID FileID;
	typedef Red::Core::Bundle::BundleID BundleID;

	typedef TDynArray< CDiskBundle*, MC_Depot >				TBundleReaders;
	typedef TDynArray< CBundleDiskFile*, MC_Depot >			TBundleFiles;
	typedef Red::Core::Bundle::CBundleMetadataStore			CBundleMetadataStore;
	typedef Red::Core::Bundle::CBundleMetadataStoreFilter	CBundleMetadataStoreFilter;
	typedef TSortedMap< Red::Core::Bundle::FileID, CBundleDiskFile*, DefaultCompareFunc< Red::Core::Bundle::FileID >, MC_Depot > TPatchedDLCFiles;
	// Load meta data store
	Bool LoadMetadataStore();

	// Build metadata store
	Bool BuildMetadataStore( const String& rootPath, const String& outPath, const TDynArray< String >& baseBundles, const TDynArray< String >& patchBundles ) const;

	// Collect list of source bundle files
	Bool CollectSourceBundles( const String& rootPath, TDynArray< String >& baseBundles, TDynArray< String >& patchBundles ) const;

	// Populate depot from meta data store
	Bool PopulateDepot( TDepotFileMap& outFileMap );

	// Create the CDiskBundles for each known bundle
	Bool CreateWrappers();

	// Bake final filters - merged
	void MergeBundleFilters();

	// Handle added content (files & bundles)
	Bool HandleAddedContent( const CBundleMetadataStore& dynamicStore, FileID baseFileID, BundleID baseBundleID, const TDynArray< FileID >& addedFiles, const TDynArray< BundleID >& addedBundles, TDepotFileMap& outFileMap );

	// metastore 
	String									m_rootPath;
	CBundleMetadataStore*					m_metadataStore;

	// filtering
	CBundleMetadataStoreFilter				m_mergedFilter;		// merged filter data
	CBundleMetadataStoreFilter				m_installedFilter;	// installed content
	CBundleMetadataStoreFilter				m_worldFilter;		// world content (runtime bundle)

	// low level data cache
	CDiskBundleCache*						m_ramCache;

	// actual bundles access points
	TBundleReaders							m_bundles;

	// actual bundle files (only the depot files, no buffers)
	TBundleFiles							m_files;

	TPatchedDLCFiles						m_patchedDLCFiles;

	// are we running the split cook ?
	Bool									m_splitCook;

	// data cache used during file reading, shared between burst reads and normal reads
	// TODO: burst reads should use paged allocator
	CBundleDataCache m_dataCache;		
	CDiskBundleIOCache m_ioCache;
};