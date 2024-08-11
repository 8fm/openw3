/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef BUNDLE_METADATA_CACHE_H_
#define BUNDLE_METADATA_CACHE_H_

#include "bundleMetadataStoreEntry.h"
#include "serializable.h"
#include "sortedmap.h"

class CDepot; //ehh
class CDiskFile; //ehh
class CBundleDiskFile; //ehh
class CDeprecatedIO;

namespace Red
{
	namespace Core
	{
		namespace Bundle
		{
			typedef TDynArray< const SMetadataBundleEntry*, MC_BundleMetadata > BundleEntries;
			typedef TDynArray< SMetadataFileEntry*, MC_BundleMetadata > BundleFileEntries;

			// System wide IDs
			typedef Uint16 BundleID;
			typedef Uint32 FileID;

			// bundle filtering MOJO
			class CBundleFiltering;

			//////////////////////////////////////////////////////////////////////////
			// SBundleBufferQuery
			// ----------------------------------------------------------------------
			// Used to make buffer queries for specific bundles.
			//////////////////////////////////////////////////////////////////////////
			struct SMetadataBundleBufferQuery
			{
				BundleID m_bundleID;
				Uint32 m_startOffset;
				Uint32 m_endOffset;
			};

			//////////////////////////////////////////////////////////////////////////
			// SMetadataFileInBundlePlacement
			// -----------------------------------------------------------------------
			// Tells us where we can file the data for a given file, how much memory we will need and how to decompress it
			//////////////////////////////////////////////////////////////////////////
			struct SMetadataFileInBundlePlacement
			{
				FileID				m_fileID;			// just for reference
				BundleID			m_bundleID;			// the bundle we should look into
				Uint32				m_offsetInBundle;	// offset in the bundle file we are located
				Uint32				m_sizeInBundle;		// size in the bundle file (compressed)
				Uint32				m_sizeInMemory;		// size of the file in memory
				ECompressionType	m_compression;		// compression type
			};

			//////////////////////////////////////////////////////////////////////////
			// SMetadataFileInBundleRange
			// -----------------------------------------------------------------------
			// Tells us about files at given offset range in bundle
			//////////////////////////////////////////////////////////////////////////
			struct SMetadataFileInBundleRange
			{
				FileID				m_fileID;			// just for reference
				Uint32				m_offsetInBundle;	// offset in the bundle file we are located
				Uint32				m_sizeInBundle;		// size in the bundle file (compressed)
				Bool				m_fullyInRange;		// is the file fully included in query range
			};

			//////////////////////////////////////////////////////////////////////////
			// CBundleMetadataStoreFilter
			// -----------------------------------------------------------------------
			// Allows to filter (ON/OFF) bundles from the meta store, used in queries
			//////////////////////////////////////////////////////////////////////////
			class CBundleMetadataStoreFilter
			{
				DECLARE_CLASS_MEMORY_ALLOCATOR( MC_BundleMetadata );

			public:
				CBundleMetadataStoreFilter();

				// initialize for given meta store
				void Initialize( const class CBundleMetadataStore& store );

				// adapt for given size
				void Resize( const class CBundleMetadataStore& store, const Bool newState );

				// enable all bundles in the filter
				RED_INLINE void EnableAll();

				// disable all bundles in the filter
				RED_INLINE void DisableAll();

				// is given bundle enabled ?
				RED_INLINE Bool IsBundleEnabled( const BundleID id ) const;

				// enable bundle with given ID
				RED_INLINE void EnableBundle( const BundleID id );

				// disable bundle with given ID
				RED_INLINE void DisableBundle( const BundleID id );

				// merge entries using AND operation
				void MergeAnd( const CBundleMetadataStoreFilter& other );

				// merge entries using OR operation
				void MergeOr( const CBundleMetadataStoreFilter& other );

				// enable bundles in given directory (slow)
				void EnableDirectory( const CBundleMetadataStore& store, const AnsiChar* baseDirectory );

				// disable bundles in given directory (slow)
				void DisableDirectory( const CBundleMetadataStore& store, const AnsiChar* baseDirectory );

			private:
				static const Uint32 SHIFT = 5;
				static const Uint32 MASK = (1<<SHIFT)-1;

				TDynArray< Uint32, MC_BundleMetadata >		m_mask;
				Uint32										m_maxBundles;
			};

			//////////////////////////////////////////////////////////////////////////
			// CBundleMetadataStore
			// -----------------------------------------------------------------------
			// Central storage place for all the data related to metadata gathered
			// from bundles
			//////////////////////////////////////////////////////////////////////////
			class CBundleMetadataStore : public System::NonCopyable
			{
				DECLARE_CLASS_MEMORY_ALLOCATOR( MC_BundleMetadata );

			public:
				typedef Red::Core::ResourceManagement::CResourceId CResourceId;

				//////////////////////////////////////////////////////////////////////////
				// Constructor(s)
				//////////////////////////////////////////////////////////////////////////
				CBundleMetadataStore( const String& rootPath );

				//////////////////////////////////////////////////////////////////////////
				// Destructor
				//////////////////////////////////////////////////////////////////////////
				~CBundleMetadataStore();

				//////////////////////////////////////////////////////////////////////////
				// Public Methods
				//////////////////////////////////////////////////////////////////////////

				// validate internal consistency
				Bool Validate() const;

				// initialize meta store data from builder data (moves the data)
				void InitializeFromBuilder( const CBundleMetadataStore& builder );

				// Discard non-persistend data (text table)
				void DiscardNonPeristentData();

				// loading and saving
				void Save( IFile& file ) const;
				Bool Load( IFile& file );

				// populate depot with content of this metadata file, returns created bundle file wrappers for future reference
				Bool PopulateDepot( CDepot* depot, TDynArray< CBundleDiskFile*, MC_Depot >& outFileList, TSortedMap< Uint64, CDiskFile*, DefaultCompareFunc< Uint64 >, MC_Depot >& outFileHashes, const Red::Core::Bundle::FileID baseFileID = 0, const Bool isDLC = false, TSortedMap< Red::Core::Bundle::FileID, CBundleDiskFile*, DefaultCompareFunc< Red::Core::Bundle::FileID >, MC_Depot >* patchedDLCFiles = nullptr ) const;

				// merge meta data stores (slow, for mods only)
				Bool MergeDepot( CDepot* depot, const TDynArray< Red::Core::Bundle::FileID >& addedFiles, const TDynArray< Red::Core::Bundle::FileID >& addedFilesOrginalIDs, const TDynArray< Red::Core::Bundle::FileID >& overridenFiles, TDynArray< CBundleDiskFile*, MC_Depot >& outFileList, TSortedMap< Uint64, CDiskFile*, DefaultCompareFunc< Uint64 >, MC_Depot >& outFileHashes, const Red::Core::Bundle::FileID baseFileID = 0 ) const;

				// given a file path find a CFileBundleID matching that (SLOW)
				FileID FindFileID( const StringAnsi& depotPath ) const;

				// given a bundle file path find a CBundleID matching that (SLOW)
				BundleID FindBundleID( const StringAnsi& bundlePath ) const;

				// given a file ID provide file path, note - the string is persistent
				// returns NULL on invalid file IDs
				const AnsiChar* GetFilePath( const FileID id ) const;

				// given a file ID provide file hash (MURMUR32)
				// returns 0 on invalid file IDs
				const Uint32 GetFileHash( const FileID id ) const;

				// given a bundle ID provide bundle file path, note - the string is persistent
				// returns NULL on invalid bundle IDs
				const AnsiChar* GetBundlePath( const BundleID id ) const;

				// given a file ID provide generic file information, returns false if file does not exist in the meta store
				// TODO: this function is part of the old interface and contains excessive fields (like String path etc)
				Bool GetFileInfo( const FileID id, SMetadataFileEntry& outFileEntry ) const;

				// given a bundle ID provide generic bundle information, returns false if bundle does not exist in the meta store
				// TODO: this function is part of the old interface and contains excessive fields (like String etc)
				Bool GetBundleInfo( const BundleID id, SMetadataBundleEntry& outBundleEntry ) const;

				// give a file ID give the size of the file when loaded into the memory
				// returns zero for unknown or invalid IDs
				Uint32 GetFileSize( const FileID id ) const;

				// give a file ID give the compression used by the file
				// returns zero for unknown or invalid IDs
				// NOTE: this is deprecated (legacy) usage, probably will be deleted
				ECompressionType GetFileCompressionType( const FileID id ) const;

				// given a filtering provide the access information for a given file
				// will return false if there is no access information (file is filtered out)
				// NOTE: filtering can be NULL
				Bool GetFileAccessInfo( const FileID id, const CBundleMetadataStoreFilter& filter, SMetadataFileInBundlePlacement& outInfo ) const;

				// given a bundle ID give all non buffers in that bundle
				Bool GetFilesInBundle( const BundleID id, const Uint32 startOffset, const Uint32 endOffset, TDynArray< SMetadataFileInBundlePlacement >& outList ) const;

				// given a file ID get all placement descriptions for the file
				void GetFileAccessInfos( const FileID id, TDynArray< SMetadataFileInBundlePlacement >& outInfo ) const;

				// given a file ID and buffer index for that file retrieve FileID of the buffer data
				// returns 0 on errors
				FileID GetBufferFileID( FileID fileID, const Uint32 bufferIndex ) const;

				// add dynamic content (glue it at the end), returns new bundle id
				const Bool AddDynamicBundles( const CBundleMetadataStore& dynamicStore, TDynArray< FileID >& outAddedFiles, TDynArray< BundleID >& outAddedBundles, FileID& outBaseFileID, BundleID& outBaseBundleID );

				// add override content, any existing file will be overridden with new content, that's slow in general and should be used on PC only. 
				// original FileIDs are preserved. Returns a list of fresh files (if any) and a list of overriden files
				const Bool AddOverrideBundles( const CBundleMetadataStore& dynamicStore, TDynArray< FileID >& outAddedFiles, TDynArray< FileID >& outAddedFilesOriginalIDs, TDynArray< BundleID >& outAddedBundles, TDynArray< FileID >& outOverridenFiles, FileID& outBaseFileID, BundleID& outBaseBundleID );

				// get the root path to the bundle directory
				RED_INLINE const String& GetRootPath() const { return m_rootPath; }

				// get the number of bundles in the meta store
				RED_INLINE const Uint32 BundleCount() const { return m_bundles.Empty() ? 0 : (m_bundles.Size()-1); }

				// get the number of files in the meta store
				RED_INLINE const Uint32 ItemCount() const { return m_files.Empty() ? 0 : (m_files.Size()-1); }

				// get the number of files<->bundle entries in the meta store
				RED_INLINE const Uint32 EntryCount() const { return m_entries.Empty() ? 0 : (m_entries.Size()-1); }

				// get the max FileID
				RED_INLINE const FileID GetMaxFileID() const { return (FileID) m_files.Size(); }

				// get the max BunldID
				RED_INLINE const BundleID GetMaxBundleID() const { return (BundleID) m_bundles.Size(); }

				// stats - larger files
				RED_INLINE Uint32 GetMaxFileSizeInBundle() const { return m_info.m_maxFileSizeInBundle; }
				RED_INLINE Uint32 GetMaxFileSizeInMemory() const { return m_info.m_maxFileSizeInMemory; }

				RED_INLINE Bool IsCensored() const { return m_info.m_censored; }
				RED_INLINE void SetCensored( Bool isCensored ) { m_info.m_censored = isCensored; }

			protected:
				void BuildFallbackFileLookupMap() const;
				void BuildFallbackBundleLookupMap() const;
				void UpdateInfo();

				// file structure:
				//   Header (magic + version)
				//   TextArray (discardable)
				//   BundleInfo *
				//   FileInfo *
				//   FileEntryInfo *

				static const Uint32 METADATA_STORE_MAGIC;
				static const Uint32 METADATA_STORE_VERSION;

				struct MetadataStoreInfo
				{
					Uint32 m_maxFileSizeInBundle;
					Uint32 m_maxFileSizeInMemory;
					Bool m_censored;
				
					MetadataStoreInfo()
						: m_maxFileSizeInBundle( 0 )
						, m_maxFileSizeInMemory( 0 )
						, m_censored( false )
					{}

					static const Uint32 CENSORSHIP_FLAG = 0x80000000;

					void Serialize( IFile & file )
					{
						file << m_maxFileSizeInBundle;

						// The last bit of m_maxFileSizeInMemory is now used to store the censorship flag
						// Which indicates whether or not this metadata store is censored or not
						if ( file.IsWriter() )
						{
							Uint32 flagsAndSize = 0;

							flagsAndSize = m_maxFileSizeInMemory;
							flagsAndSize |= m_censored ? CENSORSHIP_FLAG : 0;

							file << flagsAndSize;
						}
						else if ( file.IsReader() )
						{
							Uint32 flagsAndSize = 0;
							file << flagsAndSize;

							m_maxFileSizeInMemory = flagsAndSize & ~CENSORSHIP_FLAG;
							m_censored = (flagsAndSize & CENSORSHIP_FLAG) != 0;
						}
					}
				};

				struct BundleInfo
				{
					Uint32	m_name;				// bundle file path - index in the text table
					Uint32	m_firstFileEntry;	// first file entry for this bundle
					Uint32	m_numBundleEntries;	// number of file entries in the bundle
					Uint32  m_dataBlockSize;
					Uint32  m_dataBlockOffset;
					Uint32  m_burstDataBlockSize;

					BundleInfo()
						: m_name(0)
						, m_firstFileEntry(0)
						, m_numBundleEntries(0)
						, m_dataBlockSize(0)
						, m_dataBlockOffset(0)
						, m_burstDataBlockSize(0)
					{}
				};

				struct FileEntryInfo
				{
					FileID		m_fileID;				// ID of the file
					BundleID	m_bundleID;				// ID of the bundle that contains this file
					Uint32		m_offsetInBundle;		// file offset inside the bundle
					Uint32		m_sizeInBundle;			// duplicated for one of the queries (TO REMOVE)
					Uint32		m_nextEntry;			// next file entry

					FileEntryInfo()
						: m_fileID( 0 )
						, m_bundleID( 0 )
						, m_offsetInBundle( 0 )
						, m_sizeInBundle( 0 )
						, m_nextEntry( 0 )
					{}
				};

				struct FileInfo
				{
					Uint32			m_name;					// file path - index in the text table
					Uint32			m_pathHash;				// file path hash
					Uint32			m_sizeInBundle;			// size in the bundle file (compressed)
					Uint32			m_sizeInMemory;			//	size of the file in memory
					Uint32			m_firstEntry;			// first entry for this file that describes where the file is placed in the bundles
					Uint8			m_compressionType;		// type of compression
					Uint8			m_numEntries;			// number of file entries (just for reference)
					Uint32			m_firstBuffer:20;		// first buffer index (at least 20 bits)
					Uint32			m_numBuffers:20;		// number of buffers

					FileInfo()
						: m_name( 0 )
						, m_pathHash( 0 )
						, m_sizeInBundle( 0 )
						, m_sizeInMemory( 0 )
						, m_compressionType( 0 )
						, m_firstBuffer( 0 )
						, m_numBuffers( 0 )
						, m_numEntries( 0 )
						, m_firstEntry( 0 )
					{}
				};

				struct DirInitInfo
				{
					Uint32			m_name;					// directory name (text table entry)
					Uint32			m_parent;				// parent directory index 0 means root
				};

				struct FileInitInfo
				{
					Uint32			m_fileID;				// file ID
					Uint32			m_dirID;				// directory the file is in
					Uint32			m_name;					// file name (text table entry)
				};

				// visit all files from given bundle
				template < typename Func >
				bool VisitFilesFromBundle( const BundleID& id, const Func& func ) const
				{
					if ( !id || id >= m_bundles.Size() )
						return false;

					const BundleInfo& bundleInfo = m_bundles[ id ];

					// linear scan the files from this bundle
					const Uint32 firstEntry = bundleInfo.m_firstFileEntry;
					const Uint32 lastEntry = firstEntry + bundleInfo.m_numBundleEntries;
					for ( Uint32 i=firstEntry; i<lastEntry; ++i )
					{
						const FileEntryInfo& entry = m_entries[ i ];
						if ( !func( entry, i - firstEntry, bundleInfo.m_numBundleEntries ) )
							break;
					}

					return true;
				}

				// root path of the meta data store
				String											m_rootPath;

				MetadataStoreInfo								m_info;

				typedef TSortedMap< Uint64, Uint32, DefaultCompareFunc< Uint64 >, MC_BundleMetadata >		TFileMapping;

				// tables
				TDynArray< AnsiChar, MC_BundleMetadata >		m_textTable; // only needed for mapping and debug - not in FINAL game
				TDynArray< BundleInfo, MC_BundleMetadata >		m_bundles;
				TDynArray< FileInfo, MC_BundleMetadata >		m_files;
				TDynArray< FileEntryInfo, MC_BundleMetadata >	m_entries;
				TDynArray< Uint32, MC_BundleMetadata >			m_buffers;
				TDynArray< DirInitInfo, MC_BundleMetadata >		m_initDirs;
				TDynArray< FileInitInfo, MC_BundleMetadata >	m_initFiles;
				TFileMapping									m_hashes; // computed hash -> FileID mapping

				// fallback maps - only added because of the legacy code
				typedef THashMap< StringAnsi, FileID, DefaultHashFunc< StringAnsi >, DefaultEqualFunc< StringAnsi >, MC_BundleMetadata >	TFileMap;
				typedef THashMap< StringAnsi, BundleID, DefaultHashFunc< StringAnsi >, DefaultEqualFunc< StringAnsi >, MC_BundleMetadata >	TBundleMap;
				mutable TFileMap								m_fileMap;
				mutable TBundleMap								m_bundleMap;
			};
		}
	}
}

#include "bundleMetadataStore.inl"

#endif // BUNDLE_METADATA_CACHE_H_
