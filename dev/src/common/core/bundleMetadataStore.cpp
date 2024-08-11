/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "profiler.h"
#include "bundlePreamble.h"
#include "bundleMetadataStore.h"

#include "filePath.h"
#include "depot.h"
#include "directory.h"
#include "diskFile.h"

namespace Red { namespace Core { namespace Bundle {

//////////////////////////////////////////////////////////////////////////
CBundleMetadataStoreFilter::CBundleMetadataStoreFilter()
	: m_maxBundles( 0 )
{
}

//////////////////////////////////////////////////////////////////////////
void CBundleMetadataStoreFilter::Initialize( const CBundleMetadataStore& store )
{
	if ( m_maxBundles != store.GetMaxBundleID() )
	{
		m_maxBundles = store.GetMaxBundleID();

		// create initial mask
		const Uint32 numWords = ( m_maxBundles + MASK ) >> SHIFT; // round up to the nearest number of words
		m_mask.Resize( numWords );

		// clear initial mask
		DisableAll();
	}
}

//////////////////////////////////////////////////////////////////////////
void CBundleMetadataStoreFilter::Resize( const class CBundleMetadataStore& store, const Bool newState )
{
	if ( m_maxBundles != store.GetMaxBundleID() )
	{
		const Uint32 oldMaxBundles = m_maxBundles;
		m_maxBundles = store.GetMaxBundleID();

		// resize the mask
		const Uint32 numWords = ( m_maxBundles + MASK ) >> SHIFT; // round up to the nearest number of words
		m_mask.Resize( numWords );

		// setup state
		for ( Uint32 i=oldMaxBundles; i<m_maxBundles; ++i )
		{
			if ( newState )
			{
				EnableBundle( i );
			}
			else
			{
				DisableBundle( i );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CBundleMetadataStoreFilter::MergeAnd( const CBundleMetadataStoreFilter& other )
{
	RED_FATAL_ASSERT( m_mask.Size() == other.m_mask.Size(), "Incompatible bundle filters" );
	for ( Uint32 i=0; i<m_mask.Size(); ++i )
	{
		m_mask[i] &= other.m_mask[i];
	}
}

//////////////////////////////////////////////////////////////////////////
void CBundleMetadataStoreFilter::MergeOr( const CBundleMetadataStoreFilter& other )
{
	RED_FATAL_ASSERT( m_mask.Size() == other.m_mask.Size(), "Incompatible bundle filters" );
	for ( Uint32 i=0; i<m_mask.Size(); ++i )
	{
		m_mask[i] |= other.m_mask[i];
	}
}

//////////////////////////////////////////////////////////////////////////
void CBundleMetadataStoreFilter::EnableDirectory( const CBundleMetadataStore& store, const AnsiChar* baseDirectory )
{
	const Uint32 baseDirLen = (Uint32) Red::StringLength( baseDirectory );

	for ( BundleID id=1; id<m_maxBundles; ++id )
	{
		const AnsiChar* bundlePath = store.GetBundlePath(id);
		if ( 0 == Red::StringCompare( bundlePath, baseDirectory, baseDirLen ) )
		{
			EnableBundle( id );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CBundleMetadataStoreFilter::DisableDirectory( const CBundleMetadataStore& store, const AnsiChar* baseDirectory )
{
	const Uint32 baseDirLen = (Uint32) Red::StringLength( baseDirectory );

	for ( BundleID id=1; id<m_maxBundles; ++id )
	{
		const AnsiChar* bundlePath = store.GetBundlePath(id);
		if ( 0 == Red::StringCompare( bundlePath, baseDirectory, baseDirLen ) )
		{
			DisableBundle( id );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Version of the metadata store
// -----------------------------------------------------------------------
// Increment the final byte to change the version number, it also acts
// as a stamp.
//////////////////////////////////////////////////////////////////////////
const Uint32 CBundleMetadataStore::METADATA_STORE_MAGIC = 0x4d545603;
const Uint32 CBundleMetadataStore::METADATA_STORE_VERSION = 6;

//////////////////////////////////////////////////////////////////////////
CBundleMetadataStore::CBundleMetadataStore( const String& rootPath )
:	m_rootPath( rootPath )
{
}

//////////////////////////////////////////////////////////////////////////
CBundleMetadataStore::~CBundleMetadataStore()
{
}

//////////////////////////////////////////////////////////////////////////
void CBundleMetadataStore::BuildFallbackFileLookupMap() const
{
	CTimeCounter timer;

	m_fileMap.Clear();
	m_fileMap.Reserve( m_files.Size() );

	// fileID 0 is not used
	for ( Uint32 i=1; i<m_files.Size(); ++i )
	{
		const AnsiChar* filePath = &m_textTable[ m_files[i].m_name ];
		m_fileMap.Set( StringAnsi( filePath ), i );
	}

	LOG_CORE( TXT("BuildFallbackFileLookupMap took %1.2fs"), timer.GetTimePeriod() );
}

//////////////////////////////////////////////////////////////////////////
void CBundleMetadataStore::BuildFallbackBundleLookupMap() const
{
	CTimeCounter timer;

	m_bundleMap.Clear();
	m_bundleMap.Reserve( m_bundles.Size() );

	// fileID 0 is not used
	for ( Uint32 i=1; i<m_bundles.Size(); ++i )
	{
		const AnsiChar* filePath = &m_textTable[ m_bundles[i].m_name ];
		m_bundleMap.Set( StringAnsi( filePath ), i );
	}
	
	LOG_CORE( TXT("BuildFallbackBundleLookupMap took %1.2fs"), timer.GetTimePeriod() );
}

//////////////////////////////////////////////////////////////////////////
FileID CBundleMetadataStore::FindFileID( const StringAnsi& depotPath ) const
{
	if ( m_fileMap.Empty() )
		BuildFallbackFileLookupMap();

	FileID id = 0;
	m_fileMap.Find( depotPath, id );
	return id;
}

//////////////////////////////////////////////////////////////////////////
BundleID CBundleMetadataStore::FindBundleID( const StringAnsi& bundlePath ) const
{
	if ( m_bundleMap.Empty() )
		BuildFallbackBundleLookupMap();

	BundleID id = 0;
	m_bundleMap.Find( bundlePath, id );
	return id;
}

//////////////////////////////////////////////////////////////////////////
const AnsiChar* CBundleMetadataStore::GetFilePath( const FileID id ) const
{
	// invalid file ID was passed
	if ( !id || id >= m_files.Size() )
		return nullptr;

	const auto& fileInfo = m_files[ id ];
	return &m_textTable[ fileInfo.m_name ];
}

//////////////////////////////////////////////////////////////////////////
const Uint32 CBundleMetadataStore::GetFileHash( const FileID id ) const
{
	// invalid file ID was passed
	if ( !id || id >= m_files.Size() )
		return 0;

	const auto& fileInfo = m_files[ id ];
	return fileInfo.m_pathHash;
}

//////////////////////////////////////////////////////////////////////////
const AnsiChar* CBundleMetadataStore::GetBundlePath( const BundleID id ) const
{
	// invalid bundle ID was passed
	if ( !id || id >= m_bundles.Size() )
		return nullptr;

	const auto& bundleInfo = m_bundles[ id ];
	return &m_textTable[ bundleInfo.m_name ];
}

//////////////////////////////////////////////////////////////////////////
Bool CBundleMetadataStore::GetFileInfo( const FileID id, SMetadataFileEntry& outFileEntry ) const
{
	// invalid file ID was passed
	if ( !id || id >= m_files.Size() )
		return false;

	// fill the legacy file info
	const FileInfo& fileInfo = m_files[ id ];
	outFileEntry.m_filename = ANSI_TO_UNICODE( &m_textTable[ fileInfo.m_name ] );
	outFileEntry.m_sizeInMemory = fileInfo.m_sizeInMemory;
	outFileEntry.m_sizeInBundle = fileInfo.m_sizeInBundle;
	outFileEntry.m_compressionType = (ECompressionType) fileInfo.m_compressionType;
	outFileEntry.m_lastModified = DateTime(); // not used
	outFileEntry.m_internalBundleOffset = 0; // not allowed
	return true;
}

//////////////////////////////////////////////////////////////////////////
Uint32 CBundleMetadataStore::GetFileSize( const FileID id ) const
{
	// invalid file ID was passed
	if ( !id || id >= m_files.Size() )
		return 0;

	// fill the legacy file info
	const FileInfo& fileInfo = m_files[ id ];
	return fileInfo.m_sizeInMemory;
}

//////////////////////////////////////////////////////////////////////////
ECompressionType CBundleMetadataStore::GetFileCompressionType( const FileID id ) const
{
	// invalid file ID was passed
	if ( !id || id >= m_files.Size() )
		return CT_Uncompressed; // we don't have the CT_Invalid

	// fill the legacy file info
	const FileInfo& fileInfo = m_files[ id ];
	return (ECompressionType) fileInfo.m_compressionType;
}

//////////////////////////////////////////////////////////////////////////
Bool CBundleMetadataStore::GetBundleInfo( const BundleID id, SMetadataBundleEntry& outBundleEntry ) const
{
	// invalid bundle ID was passed
	if ( !id || id >= m_bundles.Size() )
		return false;

	// fill the legacy bundle info
	const BundleInfo& bundleInfo = m_bundles[ id ];
	outBundleEntry.m_filename = ANSI_TO_UNICODE( &m_textTable[ bundleInfo.m_name ] );
	outBundleEntry.m_burstDataBlockSize = bundleInfo.m_burstDataBlockSize;
	outBundleEntry.m_dataBlockSize = bundleInfo.m_dataBlockSize;
	outBundleEntry.m_dataBlockOffset = bundleInfo.m_dataBlockOffset;
	outBundleEntry.m_elementCount = bundleInfo.m_numBundleEntries;
	return true;
}

//////////////////////////////////////////////////////////////////////////
Bool CBundleMetadataStore::GetFileAccessInfo( const FileID id, const CBundleMetadataStoreFilter& filter, SMetadataFileInBundlePlacement& outInfo ) const
{
	// invalid file ID was passed
	if ( !id || id >= m_files.Size() )
		return false;

	const FileInfo& fileInfo = m_files[ id ];

	// walk the linked list
	Uint32 entryIndex = fileInfo.m_firstEntry;
	while ( entryIndex )
	{
		const FileEntryInfo& entryInfo = m_entries[ entryIndex ];

		// is the bundle enabled ?
		const BundleID bundleID = entryInfo.m_bundleID;
		if ( filter.IsBundleEnabled( bundleID ) )
		{
			// if so, we can read the file from this bundle, generate the access data
			outInfo.m_bundleID = bundleID;
			outInfo.m_fileID = id;
			outInfo.m_compression = (ECompressionType) fileInfo.m_compressionType;
			outInfo.m_sizeInBundle = fileInfo.m_sizeInBundle;
			outInfo.m_sizeInMemory = fileInfo.m_sizeInMemory;
			outInfo.m_offsetInBundle = entryInfo.m_offsetInBundle;

			return true;
		}

		// go to next
		entryIndex = entryInfo.m_nextEntry;
	}

	// there are not active bundles that contain this file
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CBundleMetadataStore::GetFileAccessInfos( const FileID id, TDynArray< SMetadataFileInBundlePlacement >& outInfo ) const
{
	// invalid file ID was passed
	if ( !id || id >= m_files.Size() )
		return;

	const FileInfo& fileInfo = m_files[ id ];

	// walk the linked list
	Uint32 entryIndex = fileInfo.m_firstEntry;
	while ( entryIndex )
	{
		const FileEntryInfo& entryInfo = m_entries[ entryIndex ];

		SMetadataFileInBundlePlacement info;
		info.m_bundleID = entryInfo.m_bundleID;
		info.m_fileID = id;
		info.m_compression = (ECompressionType) fileInfo.m_compressionType;
		info.m_sizeInBundle = fileInfo.m_sizeInBundle;
		info.m_sizeInMemory = fileInfo.m_sizeInMemory;
		info.m_offsetInBundle = entryInfo.m_offsetInBundle;
		outInfo.PushBack( info );

		// go to next
		entryIndex = entryInfo.m_nextEntry;
	}
}

//////////////////////////////////////////////////////////////////////////
namespace MetaDatastoreHelpers
{
	RED_INLINE Bool ValidateBuffer( const Uint32 startOffset, const Uint32 endOffset, const Uint32 startBuffer, const Uint32 endBuffer )
	{
		return startBuffer >= startOffset || ( startOffset > startBuffer && startOffset < endBuffer );
	}
}

//////////////////////////////////////////////////////////////////////////
FileID CBundleMetadataStore::GetBufferFileID( FileID fileID, const Uint32 bufferIndex ) const
{
	// invalid file
	if ( !fileID || fileID >= m_files.Size() )
		return 0;

	// invalid buffer index
	const FileInfo& fileInfo = m_files[ fileID ];
	if ( !bufferIndex || bufferIndex > fileInfo.m_numBuffers )
		return 0;

	// get the buffer file ID
	const Uint32 firstBufferIndex = fileInfo.m_firstBuffer;
	return m_buffers[ firstBufferIndex + (bufferIndex-1) ];
}

//////////////////////////////////////////////////////////////////////////
Bool CBundleMetadataStore::GetFilesInBundle( const BundleID bundleID, const Uint32 startOffset, const Uint32 endOffset, TDynArray< SMetadataFileInBundlePlacement >& outList ) const
{
	// invalid file ID was passed
	if ( !bundleID || bundleID >= m_bundles.Size() )
		return false;

	// preallocate some space in the output vector
	const BundleInfo& bundleInfo = m_bundles[ bundleID ];
	outList.Reserve( bundleInfo.m_numBundleEntries );

	// linear scan the files from this bundle
	const Uint32 firstEntry = bundleInfo.m_firstFileEntry;
	const Uint32 lastEntry = firstEntry + bundleInfo.m_numBundleEntries;
	for ( Uint32 i=firstEntry; i<lastEntry; ++i )
	{
		const FileEntryInfo& entry = m_entries[ i ];

		// Filter away the buffers
		if ( entry.m_offsetInBundle >= endOffset )
			break;
		if ( entry.m_offsetInBundle + entry.m_sizeInBundle <= startOffset )
			continue;

		// Skip entries that are unmapped (file was overridden by dynamic patch)
		if ( entry.m_fileID == 0 )
			continue;

		// Emit the entry
		SMetadataFileInBundlePlacement info;
		info.m_fileID = entry.m_fileID;
		info.m_bundleID = entry.m_bundleID;
		info.m_offsetInBundle = entry.m_offsetInBundle;
		info.m_sizeInBundle = entry.m_sizeInBundle;

		// Get file information
		const FileInfo& fileInfo = m_files[ info.m_fileID ];
		info.m_sizeInMemory = fileInfo.m_sizeInMemory;
		info.m_compression = (ECompressionType)fileInfo.m_compressionType;
		outList.PushBack( info );
	}

	// data extracted
	return true;
}

//////////////////////////////////////////////////////////////////////////
const Bool CBundleMetadataStore::AddDynamicBundles( const CBundleMetadataStore& dynamicStore, TDynArray< FileID >& outAddedFiles, TDynArray< BundleID >& outAddedBundles, FileID& outBaseFileID, BundleID& outBaseBundleID )
{
	// prepare renumeration entries
	const FileID baseFileId = m_files.Size() - 1;// good luck figuring out why
	const BundleID baseBundleId = m_bundles.Size() - 1;// good luck figuring out why
	const Uint32 baseEntry = m_entries.Size() - 1; // good luck figuring out why
	const Uint32 baseBuffer = m_buffers.Size(); // good luck figuring out why NOT

	// reserve space
	m_files.Reserve( m_files.Size() + dynamicStore.m_files.Size() );
	m_buffers.Reserve( m_buffers.Size() + dynamicStore.m_buffers.Size() );
	m_bundles.Reserve( m_bundles.Size() + dynamicStore.m_bundles.Size() );
	m_entries.Reserve( m_entries.Size() + dynamicStore.m_entries.Size() );

	// merge text tables (PC only)
#ifdef RED_MOD_SUPPORT
	RED_FATAL_ASSERT( !m_textTable.Empty(), "Base text table should not be empty. Make sure it was not stripped." );

	const Uint32 textTableBase = m_textTable.Size();
	m_textTable.PushBack( dynamicStore.m_textTable );
#endif

	// reserve space in the output lists
	outAddedFiles.Reserve( dynamicStore.m_files.Size() );
	outAddedBundles.Reserve( dynamicStore.m_bundles.Size() );

	// inform caller what is the base for the dynamic content insertion
	outBaseFileID = m_files.Size() - 1;
	outBaseBundleID = m_bundles.Size() - 1;

	// add file entries
	for ( Uint32 id=1; id<dynamicStore.m_entries.Size(); ++id )
	{
		const auto& dynamicEntry = dynamicStore.m_entries[id];

		FileEntryInfo entry;
		entry.m_fileID = dynamicEntry.m_fileID + baseFileId;
		entry.m_bundleID = dynamicEntry.m_bundleID + baseBundleId;
		entry.m_nextEntry = dynamicEntry.m_nextEntry ? (dynamicEntry.m_nextEntry + baseEntry) : 0;
		entry.m_offsetInBundle = dynamicEntry.m_offsetInBundle;
		entry.m_sizeInBundle = dynamicEntry.m_sizeInBundle;
		m_entries.PushBack( entry );
	}

	// add files
	for ( Uint32 id=1; id<dynamicStore.m_files.Size(); ++id )
	{
		const auto& dynamicFile = dynamicStore.m_files[id];
	
		FileInfo file;
#ifdef RED_MOD_SUPPORT
		file.m_name = textTableBase + dynamicFile.m_name; // Preserve names of the files when mod tool are supported
#else
		file.m_name = 0; // NO NAMES FROM DYNAMIC CONTENT
#endif
		file.m_pathHash = dynamicFile.m_pathHash;
		file.m_sizeInBundle = dynamicFile.m_sizeInBundle;
		file.m_sizeInMemory = dynamicFile.m_sizeInMemory;
		file.m_compressionType = dynamicFile.m_compressionType;
		file.m_firstEntry = dynamicFile.m_firstEntry + baseEntry;
		file.m_numEntries = dynamicFile.m_numEntries;
		file.m_firstBuffer = dynamicFile.m_firstBuffer + baseBuffer;
		file.m_numBuffers = dynamicFile.m_numBuffers;
		m_files.PushBack( file );
		outAddedFiles.PushBack( m_files.Size() - 1 );
	}

	// add buffers
	for ( const auto& dynamicBufferFileID : dynamicStore.m_buffers )
	{
		m_buffers.PushBack( baseFileId + dynamicBufferFileID );
	}

	// add bundles
	for ( Uint32 id=1; id<dynamicStore.m_bundles.Size(); ++id )
	{
		const auto& dynamicBundle = dynamicStore.m_bundles[id];

		BundleInfo bundle;
#ifdef RED_MOD_SUPPORT
		bundle.m_name = textTableBase + dynamicBundle.m_name; // Preserve names of the files when mod tool are supported
#else
		bundle.m_name = 0; // NO NAMES FROM DYNAMIC CONTENT
#endif
		bundle.m_firstFileEntry = dynamicBundle.m_firstFileEntry + baseEntry;
		bundle.m_numBundleEntries = dynamicBundle.m_numBundleEntries;
		bundle.m_dataBlockSize = dynamicBundle.m_dataBlockSize;
		bundle.m_dataBlockOffset = dynamicBundle.m_dataBlockOffset;
		bundle.m_burstDataBlockSize = dynamicBundle.m_burstDataBlockSize;
		m_bundles.PushBack( bundle );

		// return bundle ID of the added bundle
		outAddedBundles.PushBack( m_bundles.Size() - 1 );
	}

	// validate the layout
	if ( !Validate() )
	{
		ERR_CORE( TXT("Failed to validate metadata store after insertion of addtional DLC content") );
	}

	// valid
	return true;
}

//////////////////////////////////////////////////////////////////////////
const Bool CBundleMetadataStore::AddOverrideBundles( const CBundleMetadataStore& dynamicStore, TDynArray< FileID >& outAddedFiles, TDynArray< FileID >& outAddedFilesOriginalIDs, TDynArray< BundleID >& outAddedBundles, TDynArray< FileID >& outOverridenFiles, FileID& outBaseFileID, BundleID& outBaseBundleID )
{
	// we cannot insert override content if we DONT HAVE the text table
	if ( m_textTable.Size() <= 1 )
	{
		ERR_CORE( TXT("TextTable is required in the metadata store to apply file overriddes") );
		return false;
	}

	// prepare renumeration entries
	const FileID baseFileId = m_files.Size() - 1;// good luck figuring out why
	const BundleID baseBundleId = m_bundles.Size() - 1;// good luck figuring out why
	const Uint32 baseEntry = m_entries.Size() - 1; // good luck figuring out why
	const Uint32 baseBuffer = m_buffers.Size(); // good luck figuring out why NOT

	// reserve space
	m_files.Reserve( m_files.Size() + dynamicStore.m_files.Size() );
	m_buffers.Reserve( m_buffers.Size() + dynamicStore.m_buffers.Size() );
	m_bundles.Reserve( m_bundles.Size() + dynamicStore.m_bundles.Size() );
	m_entries.Reserve( m_entries.Size() + dynamicStore.m_entries.Size() );

	// reserve space in the output lists
	outAddedFiles.Reserve( dynamicStore.m_files.Size() );
	outAddedBundles.Reserve( dynamicStore.m_bundles.Size() );

	// inform caller what is the base for the dynamic content insertion
	outBaseFileID = m_files.Size() - 1;
	outBaseBundleID = m_bundles.Size() - 1;

	// merge text tables
	const Uint32 textTableBase = m_textTable.Size();
	m_textTable.PushBack( dynamicStore.m_textTable );

	// add bundles
	for ( Uint32 id=1; id<dynamicStore.m_bundles.Size(); ++id )
	{
		const auto& dynamicBundle = dynamicStore.m_bundles[id];

		BundleInfo bundle;
		bundle.m_name = dynamicBundle.m_name + textTableBase; // preserve names from dynamic content
		bundle.m_firstFileEntry = dynamicBundle.m_firstFileEntry + baseEntry;
		bundle.m_numBundleEntries = dynamicBundle.m_numBundleEntries;
		bundle.m_dataBlockSize = dynamicBundle.m_dataBlockSize;
		bundle.m_dataBlockOffset = dynamicBundle.m_dataBlockOffset;
		bundle.m_burstDataBlockSize = dynamicBundle.m_burstDataBlockSize;
		m_bundles.PushBack( bundle );

		// return bundle ID of the added bundle
		outAddedBundles.PushBack( m_bundles.Size() - 1 );
	}

	// build a name map for each EXISTING file in the metadata store
	// note: we cannot override nameless files
	THashMap< Uint64, FileID > fileIdMap;
	{
		fileIdMap.Reserve( m_files.Size() );

		Uint32 numDuplicatedFiles = 0;
		for ( Uint32 i=0; i<m_files.Size(); ++i )
		{
			const auto& fileInfo = m_files[i];
			if ( fileInfo.m_name != 0 )
			{
				const AnsiChar* fileName = &m_textTable[ fileInfo.m_name ];
				const Uint64 fileNameHash = Red::CalculatePathHash64( fileName );
				if ( !fileIdMap.Insert( fileNameHash, i ) )
				{
					ERR_CORE( TXT("File '%hs' ID %d already registered"), fileName, i );
					numDuplicatedFiles += 1;
				}
			}
		}

		LOG_CORE( TXT("Found %d named files in existing metadata (%d duplicates)"), 
			fileIdMap.Size(), numDuplicatedFiles );
	}

	// add/override files, we store new (or old) FileID for each file added
	TDynArray< FileID > mappedFileIDs;
	{
		Uint32 numNewFiles = 0;
		Uint32 numOverridenFiles = 0;
		Uint32 numUnlinkedFiles = 0;

		mappedFileIDs.Resize( dynamicStore.m_files.Size() );
		for ( Uint32 id=1; id<dynamicStore.m_files.Size(); ++id )
		{
			const auto& dynamicFile = dynamicStore.m_files[id];

			// file has no name, will not be overridden
			if ( !dynamicFile.m_name )
			{
				ERR_CORE( TXT("File %d in dynamic metadata has no file name. Will not be overridden."), id );
				continue;
			}

			// get the file name 
			const AnsiChar* fileName = &dynamicStore.m_textTable[ dynamicFile.m_name ];
			const Uint64 fileNameHash = Red::CalculatePathHash64( fileName );

			// do we have such file in the original content ?
			const FileID* originalFileID = fileIdMap.FindPtr( fileNameHash );
			if ( originalFileID )
			{
				LOG_CORE( TXT("Overridden file '%hs' at FileID %d (source ID %d)"), fileName, *originalFileID, id );

				auto& file = m_files[ *originalFileID ];

				// unlink overridden file from it's place in the bundle burst read
				{
					Uint32 entryIndex =  file.m_firstEntry;
					while ( entryIndex )
					{
						auto& existingEntry = m_entries[ entryIndex ];
						RED_FATAL_ASSERT( existingEntry.m_fileID == *originalFileID, "File entry already unlinked" );

						existingEntry.m_fileID = 0; // unlink

						const AnsiChar* bundleName = &m_textTable[ m_bundles[ existingEntry.m_bundleID ].m_name ];
						LOG_CORE( TXT("File '%hs' unlinked form bundle '%hs' (ID %d)"), fileName, bundleName, existingEntry.m_bundleID );

						entryIndex = existingEntry.m_nextEntry;
						existingEntry.m_nextEntry = 0; // unlink from entry chain

						numUnlinkedFiles += 1;
					}
				}

				// unlink overridden buffers
				for ( Uint32 i=0; i<file.m_numBuffers; ++i )
				{
					auto& existingBuffer = m_buffers[ file.m_firstBuffer + i ];
					existingBuffer = 0;
				}

				// update file info
				file.m_sizeInBundle = dynamicFile.m_sizeInBundle;
				file.m_sizeInMemory = dynamicFile.m_sizeInMemory;
				file.m_compressionType = dynamicFile.m_compressionType;
				file.m_firstEntry = dynamicFile.m_firstEntry + baseEntry; // setup new entries
				file.m_numEntries = dynamicFile.m_numEntries;
				file.m_firstBuffer = dynamicFile.m_firstBuffer + baseBuffer; // when overriding a file we override it WITH it's buffers
				file.m_numBuffers = dynamicFile.m_numBuffers;

				// save in mapping
				mappedFileIDs[id] = *originalFileID;
				outOverridenFiles.PushBack( *originalFileID );
				numOverridenFiles += 1;
			}
			else
			{
				LOG_CORE( TXT("New file in overridden content: '%hs' mapped as FileID %d, original FileID %d"), fileName, m_files.Size(), id );

				// it's a new file
				FileInfo file;
				file.m_name = dynamicFile.m_name + textTableBase; // preserve names from dynamic content
				file.m_pathHash = dynamicFile.m_pathHash;
				file.m_sizeInBundle = dynamicFile.m_sizeInBundle;
				file.m_sizeInMemory = dynamicFile.m_sizeInMemory;
				file.m_compressionType = dynamicFile.m_compressionType;
				file.m_firstEntry = dynamicFile.m_firstEntry + baseEntry;
				file.m_numEntries = dynamicFile.m_numEntries;
				file.m_firstBuffer = dynamicFile.m_firstBuffer + baseBuffer;
				file.m_numBuffers = dynamicFile.m_numBuffers;
				m_files.PushBack( file );
				outAddedFiles.PushBack( m_files.Size() - 1 );
				outAddedFilesOriginalIDs.PushBack( id );

				// save in mapping
				mappedFileIDs[id] = m_files.Size() - 1;
				numNewFiles += 1;
			}
		}

		LOG_CORE( TXT("Bundle override applied: %d new files, %d overriden, %d unlinked"), 
			numNewFiles, numOverridenFiles, numUnlinkedFiles );
	}

	// remap buffer indices to actual files
	for ( const auto& dynamicBufferFileID : dynamicStore.m_buffers )
	{
		RED_FATAL_ASSERT( dynamicBufferFileID != 0, "Invalid file ID" );

		const Uint32 mappedFileID = mappedFileIDs[ dynamicBufferFileID ];
		RED_FATAL_ASSERT( mappedFileID != 0, "Invalid mapped file ID" );

		m_buffers.PushBack( mappedFileID ); // we use mapped files
	}

	// remap bundle entries
	for ( Uint32 id=1; id<dynamicStore.m_entries.Size(); ++id )
	{
		const auto& dynamicEntry = dynamicStore.m_entries[id];

		const Uint32 mappedFileID = mappedFileIDs[ dynamicEntry.m_fileID ];
		RED_FATAL_ASSERT( mappedFileID != 0, "Invalid mapped file ID" );

		FileEntryInfo entry;
		entry.m_fileID = mappedFileID;
		entry.m_bundleID = dynamicEntry.m_bundleID + baseBundleId;
		entry.m_nextEntry = dynamicEntry.m_nextEntry ? (dynamicEntry.m_nextEntry + baseEntry) : 0;
		entry.m_offsetInBundle = dynamicEntry.m_offsetInBundle;
		entry.m_sizeInBundle = dynamicEntry.m_sizeInBundle;
		m_entries.PushBack( entry );
	}

	// done
	return true;
}

//////////////////////////////////////////////////////////////////////////

namespace Helper
{
	static Bool Validate( const AnsiChar* expr, const Char* txt, ... )
	{
		Char buffer[512];
		va_list args;

		va_start( args, txt );
		Red::VSNPrintF( buffer, ARRAY_COUNT_U32(buffer), txt, args );
		va_end( args );

		ERR_CORE( TXT("Meta store validation failed: %s."), ANSI_TO_UNICODE( expr ) );
		ERR_CORE( TXT("Contect: %ls"), buffer );
		return false;
	}
}


//////////////////////////////////////////////////////////////////////////
Bool CBundleMetadataStore::Validate() const
{
	#define VALIDATE( x, ... ) if (!(x)) { Helper::Validate( #x, __VA_ARGS__ ); return false; }

	// check file table
	for ( Uint32 fileID=1; fileID<m_files.Size(); ++fileID )
	{
		const FileInfo& info = m_files[ fileID ];

		VALIDATE( info.m_firstEntry < m_entries.Size(), TXT("FileID: %d, EntryID: %d/%d"), 
			fileID, info.m_firstEntry, m_entries.Size() );

		VALIDATE( info.m_name < m_textTable.Size(), TXT("FileID: %d, TextEntry: %d/%d"), 
			fileID, info.m_name, m_textTable.Size() );

		const AnsiChar* fileName = &m_textTable[ info.m_name ];
		VALIDATE( info.m_sizeInBundle <= info.m_sizeInMemory, TXT("FileID: %d, FileName: %ls, SizeInMemory: %d, SizeInBundle: %d"), 
			fileID, ANSI_TO_UNICODE( fileName ), info.m_sizeInMemory, info.m_sizeInBundle );

		if ( info.m_pathHash )
		{
			Uint32 mappedFileID = 0;
			VALIDATE( m_hashes.Find( info.m_pathHash, mappedFileID ), TXT("FileID: %d, not hash entry for this file"), fileID );
			VALIDATE( mappedFileID == fileID, TXT("FileID: %d, HASH CLASH between '%ls' and '%ls' (ID%d and ID%d)"), 
				fileName, &m_textTable[ m_files[ mappedFileID ].m_name ], fileID, mappedFileID );
		}

		// check entries
		Uint32 entryIndex = info.m_firstEntry;
		while ( entryIndex )
		{
			const FileEntryInfo& entryInfo = m_entries[ entryIndex ];

			VALIDATE( entryInfo.m_nextEntry < m_entries.Size(), TXT("FileID: %d, EntryID: %d/%d"), 
				fileID, entryInfo.m_nextEntry, m_entries.Size() );

			VALIDATE( entryInfo.m_fileID == fileID, TXT("FileID: %d, EntryID: %d, EntryFileID: %d"), 
				fileID, entryIndex, entryInfo.m_fileID );

			VALIDATE( entryInfo.m_sizeInBundle == info.m_sizeInBundle, TXT("FileID: %d, EntryID: %d, SizeFromFile: %d, SizeFromEntry: %d"), 
				fileID, entryIndex, info.m_sizeInBundle, entryInfo.m_sizeInBundle );

			entryIndex = entryInfo.m_nextEntry;
		}
	}

	// check bundle table
	for ( Uint32 bundleID=1; bundleID<m_bundles.Size(); ++bundleID )
	{
		const BundleInfo& info = m_bundles[ bundleID ];

		VALIDATE( info.m_name < m_textTable.Size(), TXT("BundleID: %d"), 
			bundleID );
		
		const AnsiChar* bundleName = &m_textTable[ info.m_name ];

		VALIDATE( info.m_numBundleEntries >= 0, TXT("BundleID: %d, BundleName: %hs"),
			bundleID, bundleName );

		VALIDATE( info.m_firstFileEntry < m_entries.Size(), TXT("BundleID: %d, BundleName: %hs, Entry: %d/%d"),
			bundleID, bundleName, info.m_firstFileEntry, m_entries.Size() );

		VALIDATE( info.m_firstFileEntry + info.m_numBundleEntries <= m_entries.Size(), TXT("BundleID: %d, BundleName: %hs, Count: %d, First:%d, Max: %d"),
			bundleID, bundleName, info.m_numBundleEntries, info.m_firstFileEntry, m_entries.Size() );

		VALIDATE( info.m_dataBlockOffset > 0, TXT("BundleID: %d, BundleName: %hs: Invalid start offset"),
			bundleID, bundleName );

		// check entries
		Int64 firstBufferOffset = -1;
		Int64 lastEntryOffset = -1;
		for ( Uint32 j=0; j<info.m_numBundleEntries; ++j )
		{
			const FileEntryInfo& entryInfo = m_entries[ j + info.m_firstFileEntry ];
			const AnsiChar* fileName = &m_textTable[ m_files[ entryInfo.m_fileID ].m_name ];

			VALIDATE( entryInfo.m_offsetInBundle >= info.m_dataBlockOffset, TXT("BundleID: %d, BundleName: %hs, Entry: %d, EntryBundleID: %d, EntryFile: %hs"),
				bundleID, bundleName, j, entryInfo.m_bundleID, fileName );

			VALIDATE( entryInfo.m_bundleID == bundleID, TXT("BundleID: %d, BundleName: %hs, Entry: %d, EntryBundleID: %d, EntryFile: %hs"),
				bundleID, bundleName, j, entryInfo.m_bundleID, fileName );

			if ( lastEntryOffset != -1 )
			{
				VALIDATE( entryInfo.m_offsetInBundle > (Uint64)lastEntryOffset, TXT("BundleID: %d, BundleName: %hs, Entry: %d, EntryBundleID: %d, EntryFile: %hs, EntryOffset: %d, PreviousOffset: %d"),
					bundleID, bundleName, j, entryInfo.m_bundleID, fileName,
					entryInfo.m_offsetInBundle, lastEntryOffset );
			}

			lastEntryOffset = entryInfo.m_offsetInBundle;

			// is this a buffer ?
			const Bool isBuffer = ( nullptr != Red::StringSearch( fileName, ".buffer" ) );
			if ( isBuffer )
			{
				if ( firstBufferOffset == -1 )
				{
					firstBufferOffset = entryInfo.m_offsetInBundle;
				}
			}
			else
			{
				VALIDATE( firstBufferOffset == -1, TXT("BundleID: %d, BundleName: %hs, Entry: %d, EntryBundleID: %d, EntryFile: %hs: BUFFERS ARE NOT AT THE END OF THE BUNDLE"),
					bundleID, bundleName, j, entryInfo.m_bundleID, fileName );
			}
		}

		// we should not read into the buffers
		if ( firstBufferOffset != -1 )
		{
			const Int64 effectiveReadSize = firstBufferOffset - (Int64)info.m_dataBlockOffset;
			VALIDATE( effectiveReadSize >= 0, TXT("BundleID: %d, BundleName: %hs: Invalid file bundle placements") );
			VALIDATE( info.m_dataBlockSize == effectiveReadSize, TXT("BundleID: %d, BundleName: %hs, FirstBuffer: %d, DataBlockSize: %d, DataBlockOffset %d : Mismatched size of burst read data block and buffer placement"),
				bundleID, bundleName, firstBufferOffset, info.m_dataBlockSize, info.m_dataBlockOffset );
		}
	}

	#undef VALIDATE

	// seems ok
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CBundleMetadataStore::InitializeFromBuilder( const CBundleMetadataStore& builder )
{
	m_info = builder.m_info;
	m_textTable = std::move( builder.m_textTable );
	m_entries = std::move( builder.m_entries );
	m_files = std::move( builder.m_files );
	m_bundles = std::move( builder.m_bundles );
	m_buffers = std::move( builder.m_buffers );
	m_initDirs = std::move( builder.m_initDirs );
	m_initFiles = std::move( builder.m_initFiles );
	m_hashes = std::move( builder.m_hashes );
}

//////////////////////////////////////////////////////////////////////////
void CBundleMetadataStore::DiscardNonPeristentData()
{
	BuildFallbackBundleLookupMap();

	// remove text table (note: NOT ON PC)
#if !defined(RED_MOD_SUPPORT) && !defined(RED_PLATFORM_WINPC)
	m_textTable.Clear();
	m_textTable.PushBack(0);

	// clear file names
	for ( Uint32 i=0; i<m_files.Size(); ++i )
		m_files[i].m_name = 0;

	// clear bundle names
	for ( Uint32 i=0; i<m_bundles.Size(); ++i )
		m_bundles[i].m_name = 0;
#endif

	// remove initialization data
	m_initDirs.Clear();
	m_initFiles.Clear();
	m_hashes.Clear();
}

//////////////////////////////////////////////////////////////////////////
void CBundleMetadataStore::Save( IFile& file ) const
{
	// Metadata header
	Uint32 magic = METADATA_STORE_MAGIC;
	file << magic;

	// Metadata stamp
	Uint32 version = METADATA_STORE_VERSION;
	file << version;

	// Save the data tables
	CBundleMetadataStore* nonConstThis = const_cast< CBundleMetadataStore* >(this);
	nonConstThis->UpdateInfo();
	nonConstThis->m_info.Serialize( file );
	nonConstThis->m_textTable.SerializeBulk( file );
	nonConstThis->m_files.SerializeBulk( file );
	nonConstThis->m_entries.SerializeBulk( file );
	nonConstThis->m_bundles.SerializeBulk( file );
	nonConstThis->m_buffers.SerializeBulk( file );
	nonConstThis->m_initDirs.SerializeBulk( file );
	nonConstThis->m_initFiles.SerializeBulk( file );
	nonConstThis->m_hashes.SerializeBulk( file );
}

//////////////////////////////////////////////////////////////////////////
Bool CBundleMetadataStore::Load( IFile& file )
{
	// Check metadata header
	Uint32 metadataMagic = 0;
	file << metadataMagic;
	if ( metadataMagic != METADATA_STORE_MAGIC )
	{
		WARN_CORE( TXT("Your metadata store is invalid, please delete!") );
		return false;
	}

	// Check metadata version
	Uint32 metadataVersion = 0;
	file << metadataVersion;
	if ( metadataVersion != METADATA_STORE_VERSION )
	{
		WARN_CORE( TXT("Your metadata store is out of date, please delete!") );
		return false;
	}

	// Load the data
	m_info.Serialize( file );
	m_textTable.SerializeBulk( file );
	m_files.SerializeBulk( file );
	m_entries.SerializeBulk( file );
	m_bundles.SerializeBulk( file );
	m_buffers.SerializeBulk( file );
	m_initDirs.SerializeBulk( file );
	m_initFiles.SerializeBulk( file );
	m_hashes.SerializeBulk( file );

	LOG_CORE( TXT("Bundle meta data store: (%1.2fKB strings, %d bundles, %d files, %d entries, %d buffers)"),
		m_textTable.Size() / 1024.0f, m_bundles.Size(), m_files.Size(), m_entries.Size(), m_buffers.Size() );

	LOG_CORE( TXT("Bundle meta data store: %1.2fKB persistent data size"),
		(m_files.DataSize() + m_entries.DataSize() + m_buffers.DataSize() + m_bundles.DataSize()) / 1024.0f );

	// Validate meta data
#if defined(RED_PLATFORM_WINPC) && !defined(RED_FINAL_BUILD)
	WARN_CORE( TXT("Validation got turned off!") );
	//if ( !Validate() )
	//{
	//	ERR_CORE( TXT("Validation of meta data store failed - data is corrupted and will not be used.") );
	//	return false;
	//}
#endif

	// Use the loaded data
	return true;
}

void CBundleMetadataStore::UpdateInfo()
{
	Uint32 maxFileSizeInBundle = 0;
	Uint32 maxFileSizeInMemory = 0;

	for( const FileInfo & fileInfo : m_files )
	{
		const AnsiChar * filePath = &m_textTable[ fileInfo.m_name ];
		if( Red::System::StringSearch( filePath, ".usm" ) == nullptr && Red::System::StringSearch( filePath, ".dat" ) == nullptr )
		{
			maxFileSizeInBundle = Max( maxFileSizeInBundle, fileInfo.m_sizeInBundle );
			maxFileSizeInMemory = Max( maxFileSizeInMemory, fileInfo.m_sizeInMemory );
		}
	}

	m_info.m_maxFileSizeInBundle = maxFileSizeInBundle;
	m_info.m_maxFileSizeInMemory = maxFileSizeInMemory;
}

} } }

