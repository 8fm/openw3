#include "build.h"
#include "bundleParser.h"
#include "asyncLoadToken.h"
#include "bundlePreamble.h"
#include "bundleMetadataStoreBuilder.h"
#include "bundleMetadataStoreEntry.h"
#include "bundleMetadataStore.h"
#include "directory.h"
#include "depot.h"
#include "filePath.h"
#include "hashset.h"

namespace Red { namespace Core { namespace Bundle {

const String CBundleMetadataStoreBuilder::BUNDLE_FILE_EXTENTION( TXT( "bundle" ) );

//////////////////////////////////////////////////////////////////////////
CBundleMetadataStoreBuilder::CBundleMetadataStoreBuilder( CDeprecatedIO& asyncIO )
	: CBundleMetadataStore( String::EMPTY )
	, m_preambleParser( asyncIO )
	, m_asyncIO( asyncIO )
{
}

//////////////////////////////////////////////////////////////////////////
CBundleMetadataStoreBuilder::~CBundleMetadataStoreBuilder()
{
}

//////////////////////////////////////////////////////////////////////////
CBundleMetadataStoreBuilder::Settings::Settings()
	: m_isCensored( false )
{
}

//////////////////////////////////////////////////////////////////////////

namespace Helper
{
	class CCompressionHistogram
	{
	public:
		CCompressionHistogram( const StringAnsi& name )
			: m_name( name )
		{
			Red::MemoryZero( &m_totalFiles, sizeof(m_totalFiles) );
			Red::MemoryZero( &m_totalCompressedSize, sizeof(m_totalCompressedSize) );
			Red::MemoryZero( &m_totalUncompressedSize, sizeof(m_totalUncompressedSize) );
		}

		void Report( const Uint8 compressionType, const Uint32 compressedSize, const Uint32 uncompressedSize )
		{
			m_totalFiles[ compressionType ] += 1;
			m_totalCompressedSize[ compressionType ] += compressedSize;
			m_totalUncompressedSize[ compressionType ] += uncompressedSize;
		}

		StringAnsi	m_name;
		Uint32		m_totalFiles[ Red::Core::Bundle::CT_Max ];
		Uint64		m_totalCompressedSize[ Red::Core::Bundle::CT_Max ];
		Uint64		m_totalUncompressedSize[ Red::Core::Bundle::CT_Max ];

		const Uint32 GetTotalFiles() const
		{
			Uint32 numFiles = 0;
			for ( Uint32 i=0; i<Red::Core::Bundle::CT_Max; ++i )
			{
				numFiles += m_totalFiles[ i ]; 
			}
			return numFiles;
		}

		const Uint64 GetTotalCompressedSize() const
		{
			Uint64 numCompressedSize = 0;
			for ( Uint32 i=0; i<Red::Core::Bundle::CT_Max; ++i )
			{
				numCompressedSize += m_totalCompressedSize[ i ]; 
			}
			return numCompressedSize;
		}

		const Uint64 GetTotalUncompressedSize() const
		{
			Uint64 numUncompressedSize = 0;
			for ( Uint32 i=0; i<Red::Core::Bundle::CT_Max; ++i )
			{
				numUncompressedSize += m_totalUncompressedSize[ i ]; 
			}
			return numUncompressedSize;
		}

		const Double GetTotalRatio() const
		{
			const Uint64 compressed = GetTotalCompressedSize();
			const Uint64 uncompressed = GetTotalUncompressedSize();
			return uncompressed ? ( (Double)compressed / (Double)uncompressed ) : 1.0;
		}

		void PrintLine( const Uint32 index, const AnsiChar* name ) const
		{
			if ( m_totalUncompressedSize[index] )
			{
				RED_LOG( WCC, TXT(" %hs: %d files, %1.2fMB -> %1.2fMB (ratio: %1.3f)"), 
					name, m_totalFiles[index], 
					m_totalUncompressedSize[index] / (1024.0f*1024.0f),
					m_totalCompressedSize[index] / (1024.0f*1024.0f),
					(Double)m_totalCompressedSize[index] / (Double)m_totalUncompressedSize[index] );
			}
		}

		void Print()
		{
			RED_LOG( WCC, TXT("-------------------------------------------------------") );
			RED_LOG( WCC, TXT("-- Compression statistics for %hs"), m_name.AsChar() );
			RED_LOG( WCC, TXT("-------------------------------------------------------") );

			PrintLine( CT_Uncompressed, "  none" );
			PrintLine( CT_Zlib, "  zlib" );
			PrintLine( CT_Snappy, "snappy" );
			PrintLine( CT_Doboz, " doboz" );
			PrintLine( CT_LZ4, "   lz4" );
			PrintLine( CT_LZ4HC, " lz4hc" );

			RED_LOG( WCC, TXT("-------------------------------------------------------") );
		}
	};

	class CCompressionHistogramPerExt
	{
	public:
		CCompressionHistogramPerExt( const StringAnsi& title )
			: m_title( title )
		{
		}

		CCompressionHistogram* GetHistogram( const StringAnsi& ext )
		{
			CCompressionHistogram* ret = nullptr;
			m_map.Find( ext, ret );
			if ( !ret )
			{
				ret = new CCompressionHistogram( ext );
				m_map.Insert( ext, ret );
				m_list.PushBack( ret );
			}
			return ret;
		}

		void Print()
		{
			// sort list by number of files
			::Sort( m_list.Begin(), m_list.End(), [](CCompressionHistogram* a, CCompressionHistogram* b) { return a->GetTotalFiles() > b->GetTotalFiles(); } );

			RED_LOG( WCC, TXT("-------------------------------------------------------") );
			RED_LOG( WCC, TXT("-- Per extension stats for %hs"), m_title.AsChar() );
			RED_LOG( WCC, TXT("-------------------------------------------------------") );

			// per extension general stats
			for ( Uint32 i=0; i<m_list.Size(); ++i )
			{
				CCompressionHistogram* ext = m_list[i];

				RED_LOG( WCC, TXT("   %hs: %d files, %1.2f MB -> %1.2f MB (ratio: %1.3f)"), 
					ext->m_name.AsChar(), ext->GetTotalFiles(), 
					(Double)ext->GetTotalUncompressedSize() / (1024.0f*1024.0f),
					(Double)ext->GetTotalCompressedSize() / (1024.0f*1024.0f),
					ext->GetTotalRatio() );
			}

			RED_LOG( WCC, TXT("-------------------------------------------------------") );

			for ( Uint32 i=0; i<m_list.Size(); ++i )
			{
				CCompressionHistogram* ext = m_list[i];
				ext->Print();
			}
		}

	public:
		StringAnsi										m_title;
		TDynArray< CCompressionHistogram* >				m_list;
		THashMap< StringAnsi, CCompressionHistogram* >	m_map;
	};

	class CCompressionHistogramsFull
	{
	public:
		CCompressionHistogramsFull()
			: m_normal( "normal files" )
			, m_buffers( "buffer files" )
		{
		}

		void Report( const StringAnsi& fileName, const Uint8 compressionType, const Uint32 compressedSize, const Uint32 uncompressedSize )
		{
			// get extensions
			TDynArray< StringAnsi > parts = fileName.Split( "." );
			parts.RemoveAt(0);

			// single or no extension
			CCompressionHistogram* data = nullptr;
			if ( parts.Size() <= 1 )
			{
				const StringAnsi ext = parts.Empty() ? "none" : parts[0];
				data = m_normal.GetHistogram( ext );
			}			
			else if ( parts.Back() == "buffer" )
			{
				const StringAnsi ext = (parts.Size() < 3) ? "unknown.buffer" : parts[ parts.Size()-3 ];
				data = m_buffers.GetHistogram( ext );
			}

			if ( data )
			{
				data->Report( compressionType, compressedSize, uncompressedSize );
			}
		}

		void Print()
		{
			m_normal.Print();
			m_buffers.Print();
		}

		CCompressionHistogramPerExt		m_normal;
		CCompressionHistogramPerExt		m_buffers;
	};
}

//////////////////////////////////////////////////////////////////////////
Bool CBundleMetadataStoreBuilder::ProcessBundles( const Settings& settings )
{
	// process each bundle
	Bool result = true;
	for( const String& bundlePath : settings.m_bundlesPaths )
	{
		if ( !ProcessBundle( settings.m_rootPath, bundlePath, -1 /* base group */ ) )
		{
			ERR_CORE( TXT("Failed to load bundle from '%ls'. Data is corrupted."), bundlePath.AsChar() );
			return false;
		}
	}

	// apply patch
	for ( Uint32 patchGroupID=0; patchGroupID<settings.m_patchGroups.Size(); ++patchGroupID )
	{
		const auto& group = settings.m_patchGroups[patchGroupID];

		for ( const String& patchBundlePath : group.m_patchBundlesPaths )
		{
			if ( !ProcessBundle( group.m_patchRootPath, patchBundlePath, patchGroupID ) )
			{
				ERR_CORE( TXT("Failed to load bundle from '%ls'. Data is corrupted."), patchBundlePath.AsChar() );
				return false;
			}
		}
	}

	// stats
	Uint32 numTotalRawEntries = 0;
	for ( auto bundle : m_rawBundles )
		numTotalRawEntries += bundle->m_entries.Size();
	RED_LOG( WCC, TXT("Loaded %d bundles with %d entries"), m_rawBundles.Size(), numTotalRawEntries );

	bool buldingPatch = !settings.m_patchGroups.Empty();

	// apply patch
	// NOTE: patching must be applied before censorship kicks in because we are patching ORIGINAL content (non censored)
	// NOTE: TBH, this still needs a little bit more investigation why this is causing problems but for now, it works.
	if ( buldingPatch )
	{
		result &= ApplyPatch();
	}

	// apply the censorship
	if ( settings.m_isCensored )
	{
		result &= ApplyCensorship();
	}
	else
	{
		result &= DiscardCensorFiles();
	}

	// create the final data layout
	result &= CreateFinalLayout();

	// link buffers to files
	result &= LinkBuffers(buldingPatch);

	// create the fast initialization data (dir table & file table)
	result &= BuildInitData();

	// stats
	if ( result )
	{
		RED_LOG( WCC, TXT("Exported %1.2f KB of text (discardable)"), m_textTable.DataSize() / 1024.0f );
		RED_LOG( WCC, TXT("Exported %d bundles (%1.2f KB)"), m_bundles.Size()-1, m_bundles.DataSize() / 1024.0f );
		RED_LOG( WCC, TXT("Exported %d entries (%1.2f KB)"), m_entries.Size()-1, m_entries.DataSize() / 1024.0f );
		RED_LOG( WCC, TXT("Exported %d files (%1.2f KB)"), m_files.Size()-1, m_files.DataSize() / 1024.0f );
		RED_LOG( WCC, TXT("Created %d buffers (%1.2f KB)"), m_buffers.Size(), m_buffers.DataSize() / 1024.0f );
		RED_LOG( WCC, TXT("Created %1.2f KB in initialization data"), (m_initFiles.DataSize() + m_initDirs.DataSize() + m_hashes.DataSize()) / 1024.0f );

		// per bundle stat
		for ( Uint32 i=1; i<m_bundles.Size(); ++i )
		{
			const auto& bundle = m_bundles[i];

			Uint32 numBuffers = 0;
			Uint32 numNormalFiles = 0;
			Uint32 numFilesWithBuffers = 0;
			Uint32 totalDataSize = 0;

			for ( Uint32 j=0; j<bundle.m_numBundleEntries; ++j )
			{
				const auto& entry = m_entries[ bundle.m_firstFileEntry + j ];
				const auto& file = m_files[ entry.m_fileID ];

				totalDataSize += file.m_sizeInBundle;

				if ( file.m_numBuffers != 0 )
					numFilesWithBuffers += 1;

				const AnsiChar* filePath = &m_textTable[ file.m_name ];
				if ( Red::StringSearch( filePath, ".buffer" ) != nullptr )
				{
					numBuffers += 1;
				}
				else
				{
					numNormalFiles += 1;
				}
			}

			const AnsiChar* bundleName = &m_textTable[ bundle.m_name ];
			RED_LOG( WCC, TXT("  Bundle %hs:"), bundleName );
			RED_LOG( WCC, TXT("    NumEntries: %d"), bundle.m_numBundleEntries );
			RED_LOG( WCC, TXT("    NumBuffers: %d"), numBuffers );
			RED_LOG( WCC, TXT("    NumResources: %d (%d owning buffers)"), numNormalFiles, numFilesWithBuffers );
			RED_LOG( WCC, TXT("    TotalDataSize: %1.2f KB"), totalDataSize / 1024.0f );
			RED_LOG( WCC, TXT("    BurstDataSize: %1.2f KB"), bundle.m_dataBlockSize / 1024.0f );
		}		
	}

	// generate compression histogram
	{
		Helper::CCompressionHistogram shortStats( "totals" );
		Helper::CCompressionHistogramsFull fullStats;

		for ( Uint32 i=1; i<m_entries.Size(); ++i )
		{
			const auto& entry = m_entries[i];
			const auto& file = m_files[entry.m_fileID];
			const AnsiChar* filePath = &m_textTable[ file.m_name ];

			shortStats.Report( file.m_compressionType, file.m_sizeInBundle, file.m_sizeInMemory );
			fullStats.Report( filePath, file.m_compressionType, file.m_sizeInBundle, file.m_sizeInMemory );
		}

		fullStats.Print();
		shortStats.Print();
	}

	// return final status
	return result;
}

//////////////////////////////////////////////////////////////////////////
Bool CBundleMetadataStoreBuilder::ApplyCensorship()
{
	RED_LOG( WCC, TXT("Applying censorship...") );

	// Stats
	Uint32 numFilesChanged = 0;
	Uint32 numFilesFailed = 0;

	// Generate hashset of ALL FILES
	THashSet< StringAnsi > allFiles;
	for ( RawBundleData* bundle : m_rawBundles )
	{
		for ( RawBundleEntry* file : bundle->m_entries )
		{
			allFiles.Insert( file->m_path );
		}
	}

	// Files that were censored
	THashSet< StringAnsi > censoredFiles;

	// Check that the censored files are in the proper bundles (the same as the original files)
	for ( RawBundleData* bundle : m_rawBundles )
	{
		// create local file map
		THashMap< StringAnsi, RawBundleEntry* >	bundleFiles;
		for ( RawBundleEntry* file : bundle->m_entries )
		{
			bundleFiles.Insert( file->m_path, file );
		}

		// map censored files to their originals
		for ( RawBundleEntry* file : bundle->m_entries )
		{
			// get uncensored file path
			StringAnsi basePath, extension;
			if ( !file->m_path.Split( ".", &basePath, &extension, false ) )
				continue;

			// is this a censored file ?
			StringAnsi trimmedPath = basePath.StringBefore( "_censored" );
			if ( trimmedPath.Empty() )
				continue;

			// reassemble the full path
			trimmedPath += ".";
			trimmedPath += extension;

			// find in list
			if ( !allFiles.Exist( trimmedPath ) )
			{
				numFilesFailed += 1;

				ERR_CORE( TXT("Unable to apply censorship to '%hs': there is no orignal file in cook."),
					trimmedPath.AsChar() );

				// do not add the censored file to the bundles
				file->m_skip = true;
			}
			else
			{
				numFilesChanged += 1;

				// apply censorship
				RED_LOG( WCC, TXT("Applying censorshit to '%hs'"), 
					trimmedPath.AsChar() );

				// do not add the original file to the bundle
				censoredFiles.Insert( trimmedPath );

				// add the censored file with different name
				file->m_path = trimmedPath;
				file->m_censored = true;
			}
		}
	}

	// Discard originals for each censored file
	for ( RawBundleData* bundle : m_rawBundles )
	{
		// create local file map
		for ( RawBundleEntry* file : bundle->m_entries )
		{
			// already censored
			if ( file->m_censored )
				continue;

			// remove the original
			if ( censoredFiles.Exist( file->m_path ) )
			{
				RED_LOG( WCC, TXT("Discarding original '%hs' from '%hs'"), 
					file->m_path.AsChar(), bundle->m_name.AsChar() );

				file->m_skip = true;
			}
		}
	}

	// Stats
	RED_LOG( WCC, TXT("Censorship applied to %d files (%d files failed)"), numFilesChanged, numFilesFailed  );
	return true;
}

//////////////////////////////////////////////////////////////////////////
Bool CBundleMetadataStoreBuilder::ApplyPatch()
{
	RED_LOG( WCC, TXT("Applying patch...") );

	// Found range of patch group
	Int32 maxPatchGroup = -1;
	for ( RawBundleData* bundle : m_rawBundles )
	{
		maxPatchGroup = Max< Int32 >( maxPatchGroup, bundle->m_patchGroup );
	}

	// no patch
	if ( maxPatchGroup == -1 )
		return true;

	// report maximum patch group
	RED_LOG( WCC, TXT("Max patch group: %d"), maxPatchGroup );

	// Process patch groups in order
	for ( Int32 currentPatchGroupID = 0; currentPatchGroupID <= maxPatchGroup; ++currentPatchGroupID )
	{
		// Generate hashset of ALL FILES that are patched in this group
		THashSet< StringAnsi > patchedFiles;
		for ( RawBundleData* bundle : m_rawBundles )
		{
			// process only the true "patch" bundles (skip atomic bundles as not all of the file there are actually PATCHED)
			if ( bundle->m_patchGroup == currentPatchGroupID )
			{
				for ( RawBundleEntry* file : bundle->m_entries )
				{
					patchedFiles.Insert( file->m_path );
				}
			}
		}

		// Disable files that are patches but are not coming from patch bundles :)
		// NOTE: we only keep the LAST file available (so content of mod0 will override patch0, and mod2 will override mod1, etc)
		Uint32 numFilesPatched = 0;
		for ( RawBundleData* bundle : m_rawBundles )
		{
			for ( RawBundleEntry* file : bundle->m_entries )
			{
				// was this file patched in current group ?
				if ( patchedFiles.Exist( file->m_path ) )
				{
					// update max path
					if ( currentPatchGroupID > file->m_finalPatchGroup )
					{					 
						RED_LOG( WCC, TXT("File '%hs' in bundle '%hs' was patched and will not be acessible. Group %d->%d."),
							file->m_path.AsChar(), bundle->m_name.AsChar(),
							file->m_finalPatchGroup, currentPatchGroupID );

						file->m_skip = true;
						numFilesPatched += 1;
					}
				}
			}
		}

		// applied
		RED_LOG( WCC, TXT("Patching applied to %d files (%d intial set) in group %d"), 
			numFilesPatched, patchedFiles.Size(), currentPatchGroupID );
	}

	// mark every file that has a newer version as not accessible in the original bundle
	Uint32 numTotalFilesPatched = 0;
	for ( RawBundleData* bundle : m_rawBundles )
	{
		for ( RawBundleEntry* file : bundle->m_entries )
		{
			if ( file->m_skip )
			{
				numTotalFilesPatched += 1;
			}
		}
	}

	// final stats
	RED_LOG( WCC, TXT("Total %d files patched"), numTotalFilesPatched );

	// patching done
	return true;
}

//////////////////////////////////////////////////////////////////////////
Bool CBundleMetadataStoreBuilder::DiscardCensorFiles()
{
	Uint32 numCensorshipFiles = 0;

	for ( RawBundleData* bundle : m_rawBundles )
	{
		for ( RawBundleEntry* file : bundle->m_entries )
		{
			// get uncensored file path
			StringAnsi basePath = file->m_path.StringBefore( "." );
			if ( basePath.Empty() )
				continue;

			// is this a censored file ?
			if ( !basePath.EndsWith( "_censored" ) )
				continue;

			// it's a censorship file
			RED_LOG( WCC, TXT("Ignoring censorshit for '%hs'"), 
				file->m_path.AsChar() );

			// ignore the file
			file->m_skip = true;
			numCensorshipFiles += 1;
		}
	}

	// Stats
	RED_LOG( WCC, TXT("Ignored censorship for %d files"), numCensorshipFiles );
	return true;
}

//////////////////////////////////////////////////////////////////////////
Bool CBundleMetadataStoreBuilder::ProcessBundle( const String& rootPath, const String& relativeBundleFilePath, const Int32 patchGroupID )
{
	SBundleHeaderPreamble preamble;
	Red::MemoryZero( &preamble, sizeof(preamble) );

	const String absoluteBundleFilePath = rootPath + relativeBundleFilePath;
	CAsyncLoadToken* preambleToken = ReadPreamble( absoluteBundleFilePath, &preamble );
	if( preamble.m_configuration == EBundleHeaderConfiguration::BHC_Debug )
	{
		void* headerBuffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_BundleMetadata, preamble.m_headerSize );

		// create wrapper for bundle data
		RawBundleData* bundleData = new RawBundleData();
		bundleData->m_path = absoluteBundleFilePath;
		bundleData->m_burstSize = preamble.m_burstSize;
		bundleData->m_dataBlockOffset = preamble.m_headerSize + sizeof(SBundleHeaderPreamble);
		bundleData->m_dataBlockSize = preamble.m_fileSize - bundleData->m_dataBlockOffset; // NOT TRUE!
		bundleData->m_patchGroup = patchGroupID;		

		// bundle path is relative to the bundle directory and always lower case
		bundleData->m_name = UNICODE_TO_ANSI( relativeBundleFilePath.AsChar() ); // content0/blob.bundle

		// if this is a patch bundle remove any previously created bundle with the same name
		for ( RawBundleData* existingBundle : m_rawBundles )
		{
			if ( existingBundle->m_name == bundleData->m_name ) // direct bundle replacement
			{
				WARN_CORE( TXT("Whole bundle '%hs' was patched. Original will not be present in the metadata.store"),
					existingBundle->m_name.AsChar(), bundleData->m_name.AsChar() );

				// when a whole bundle is replaced we MAY have files there that are NOT necessarily patched
				// report the new bundle as being a part of the existing patch group
				bundleData->m_patchGroup = existingBundle->m_patchGroup;

				// remove original bundle
				m_rawBundles.Remove( existingBundle );
				delete existingBundle;
				break;
			}
		}

		// add to the bundle list
		m_rawBundles.PushBack( bundleData );

		// FIXME: Platform paths
#ifdef RED_PLATFORM_ORBIS
		bundleData->m_name.ReplaceAll("/","\\");
#endif

		// copy entries
		const auto headerItemCollection = ReadDebugBundleHeader( absoluteBundleFilePath, preamble, headerBuffer );
		const Uint32 headerItemCount = headerItemCollection.Size();
		for( Uint32 itemIndex = 0; itemIndex < headerItemCount; ++itemIndex )
		{
			const auto& fileHeaderInfo = *headerItemCollection[ itemIndex ];

			// convert path for safety
			StringAnsi filePath( fileHeaderInfo.m_rawResourcePath );
			filePath.MakeLower(); // safety :(

			// initialize new file structure
			RawBundleEntry* newFile = new RawBundleEntry;
			newFile->m_bundle = bundleData;
			newFile->m_path = fileHeaderInfo.m_rawResourcePath;
			newFile->m_compressionType = fileHeaderInfo.m_compressionType;
			newFile->m_compressedDataSize = fileHeaderInfo.m_compressedDataSize;
			newFile->m_dataSize = fileHeaderInfo.m_dataSize;
			newFile->m_dataOffset = fileHeaderInfo.m_dataOffset;
			newFile->m_skip = false;
			newFile->m_censored = false;
			newFile->m_finalPatchGroup = patchGroupID; // we ALWAYS report the file with the actual patch group
			bundleData->m_entries.PushBack( newFile );
		}

		// cleanup
		RED_MEMORY_FREE( MemoryPool_Default, MC_BundleMetadata, headerBuffer );

		return true;
	}
	else
	{
		return false;
	}

	RED_UNUSED( preambleToken );
}

//////////////////////////////////////////////////////////////////////////
Bool CBundleMetadataStoreBuilder::CreateFinalLayout()
{
	// prepare the tables
	// the indices 0 have special meaning
	m_textTable.Clear();
	m_textTable.Reserve( 1 << 20 );
	m_textTable.PushBack( 0 );
	m_bundles.Clear();
	m_bundles.Reserve( 1 << 10 );
	m_bundles.PushBack( CBundleMetadataStore::BundleInfo() );
	m_files.Clear();
	m_files.Reserve( 1 << 18 ); // ~100k
	m_files.PushBack( CBundleMetadataStore::FileInfo() );
	m_entries.Clear();
	m_entries.Reserve( 1 << 20 ); // ~1M
	m_entries.PushBack( CBundleMetadataStore::FileEntryInfo() );
	m_buffers.Clear();
	m_buffers.Reserve( 1 << 20 ); // ~1M
	m_hashes.Clear();
	m_hashes.Reserve( 1 << 20 ); // ~1M

	// process bundles
	for ( RawBundleData* bundle : m_rawBundles )
	{
		if ( !CreateFinalBundleLayout( *bundle ) )
			return false;
	}

	// created
	return true;
}

//////////////////////////////////////////////////////////////////////////
Uint32 CBundleMetadataStoreBuilder::AddString( const AnsiChar* txt )
{
	const Uint32 length = (Uint32) Red::StringLength( txt );
	const Uint32 offset = m_textTable.Size();
	m_textTable.Grow( length + 1 );
	Red::MemoryCopy( &m_textTable[ offset ], txt, sizeof(AnsiChar) * (length+1) );
	return offset;
}

//////////////////////////////////////////////////////////////////////////
Uint32 CBundleMetadataStoreBuilder::AddFile( const RawBundleEntry& fileHeaderInfo )
{
	// use existing entry if we have one
	Uint32 fileID = 0;
	if ( m_fileMap.Find( fileHeaderInfo.m_path, fileID ))
		return fileID;

	// allocate file ID
	fileID = (Uint32) m_files.Size();

	// initialize new file structure
	CBundleMetadataStore::FileInfo* newFile = new ( m_files ) CBundleMetadataStore::FileInfo;
	newFile->m_name = AddString( fileHeaderInfo.m_path.AsChar() );
	newFile->m_pathHash = 0;
	newFile->m_compressionType = fileHeaderInfo.m_compressionType;
	newFile->m_sizeInBundle = fileHeaderInfo.m_compressedDataSize;
	newFile->m_sizeInMemory = fileHeaderInfo.m_dataSize;
	newFile->m_firstEntry = 0;

	// map the path to file ID for future use in the builder
	m_fileMap[ fileHeaderInfo.m_path ] = fileID;
	return fileID;
}

//////////////////////////////////////////////////////////////////////////
Bool CBundleMetadataStoreBuilder::CreateFinalBundleLayout( const RawBundleData& bundle )
{
	// we create the following tables in this step:
	//  - text table ( bundle names & file names )
	//  - bundle table ( one entry for each bundle )
	//  - file table ( one entry for each depot file )
	//  - file entry table ( mapping table between files & bundles )

	// allocate bundle ID
	const BundleID bundleID = (BundleID) m_bundles.Size();

	// create bundle entry
	CBundleMetadataStore::BundleInfo* newBundle = new ( m_bundles ) CBundleMetadataStore::BundleInfo();
	newBundle->m_name = AddString( bundle.m_name.AsChar() );
	newBundle->m_firstFileEntry = m_entries.Size(); // current file entry index
	newBundle->m_burstDataBlockSize = bundle.m_burstSize;
	newBundle->m_dataBlockOffset = bundle.m_dataBlockOffset;
	newBundle->m_dataBlockSize = bundle.m_dataBlockSize;
	newBundle->m_numBundleEntries = 0;

	// sort the entries before adding them
	auto sortedEntries = bundle.m_entries;
	::Sort( sortedEntries.Begin(), sortedEntries.End(), [](RawBundleEntry* a, RawBundleEntry* b) { return a->m_dataOffset < b->m_dataOffset; } );

	// first position of buffer in file
	Int32 firstBufferOffset = -1;

	// create file entries
	// for each file in bundle we create a "FileEntryInfo"
	// if a file is seen for the first time we create the "FileInfo"
	for ( RawBundleEntry* entry : sortedEntries )
	{
		const auto& info = *entry;

		// skip this entry
		if ( info.m_skip )
			continue;

		// map the file to a file ID
		const FileID fileID = AddFile( info );

		// previous bundle name
		const AnsiChar* prevBundleName = "None";
		if ( m_files[ fileID ].m_firstEntry != 0 )
			prevBundleName = &m_textTable[ m_bundles[ m_entries[ m_files[ fileID ].m_firstEntry ].m_bundleID ].m_name ];

		// file compression must be the same
		if ( m_files[ fileID ].m_compressionType != info.m_compressionType )
		{
			ERR_CORE( TXT("Mismatched file compression for file '%hs' in bundle '%hs'. Current compression = %d, previous = %d, previous bundle = %hs. File will be ignored."), 
				info.m_path.AsChar(), bundle.m_name.AsChar(), 
				info.m_compressionType, m_files[ fileID ].m_compressionType,
				prevBundleName );

			continue;
		}

		// file size MUST be the same - if it's not the file is ignored
		if ( m_files[ fileID ].m_sizeInBundle != info.m_compressedDataSize )
		{
			ERR_CORE( TXT("Mismatched file description for file '%hs' in bundle '%hs'. Current size = %d, previous = %d, previous bundle = %hs. File will be ignored."),
				info.m_path.AsChar(), bundle.m_name.AsChar(), 
				info.m_compressedDataSize, m_files[ fileID ].m_sizeInBundle,
				prevBundleName );
			continue;
		}

		// buffer ?
		if ( info.m_path.EndsWith( ".buffer") )
		{
			if ( firstBufferOffset == -1 || (Int64)info.m_dataOffset < firstBufferOffset )
			{
				firstBufferOffset = info.m_dataOffset;
			}
		}

		// create the entry describing placement of the file in the bundle
		// this links the file & the bundle together
		const Uint32 entryID = m_entries.Size();
		CBundleMetadataStore::FileEntryInfo* newEntry = new ( m_entries ) CBundleMetadataStore::FileEntryInfo;
		newEntry->m_bundleID = bundleID;
		newEntry->m_fileID = fileID;
		newEntry->m_offsetInBundle = info.m_dataOffset; // WHERE IS THIS FILE IN THIS BUNDLE ?
		newEntry->m_sizeInBundle = info.m_compressedDataSize; // a copy of the data stored in the File table, just for faster queries

		// update the linked list of entries for given file
		newEntry->m_nextEntry = m_files[ fileID ].m_firstEntry;
		m_files[ fileID ].m_firstEntry = entryID;
	}

	// compute data read size
	if ( firstBufferOffset != -1 )
	{
		newBundle->m_dataBlockSize = firstBufferOffset - newBundle->m_dataBlockOffset;
	}

	// count how many entries we allocated for the bundle
	newBundle->m_numBundleEntries = ( m_entries.Size() - newBundle->m_firstFileEntry );

	// cleanup
	return true;
}

//////////////////////////////////////////////////////////////////////////
Bool CBundleMetadataStoreBuilder::LinkBuffers(bool buildingPatch)
{
	Bool valid = true;

	RED_LOG( WCC, TXT("Analyzing buffers....") );

	// create initial file entries
	TDynArray< EntryInfo* > entries;
	THashMap< Uint64, EntryInfo* > entriesMap;
	entries.Reserve( m_files.Size() );
	entriesMap.Reserve( m_files.Size() );
	for ( Uint32 fileID=0; fileID<m_files.Size(); ++fileID )
	{
		const auto& fileInfo = m_files[fileID];
		if ( !fileInfo.m_firstEntry )
			continue;

		EntryInfo* entry = new EntryInfo();
		entry->m_ignore = false;
		entry->m_fileId = fileID;
		entry->m_path = &m_textTable[ fileInfo.m_name ];
		entry->m_path.MakeLower(); // safety :(
		entry->m_bufferId = -1;
		entry->m_maxBufferId = -1;
		entry->m_bufferOwner = nullptr;

		// is a buffer ?
		if ( entry->m_path.EndsWith( ".buffer") )
		{
			const StringAnsi bufferIndexText = entry->m_path.StringBefore(".buffer", true).StringAfter(".", true);
			if ( bufferIndexText.Empty() )
			{
				ERR_CORE( TXT("Metastore build error: Buffer file without a number (path = '%ls')"), ANSI_TO_UNICODE(entry->m_path.AsChar()) );
				valid = false;
				continue;
			}

			Uint32 bufferIndex = 0;
			if ( Red::System::StringToInt( bufferIndex, bufferIndexText.AsChar(), nullptr, Red::System::BaseTen ) )
			{
				entry->m_bufferId = (Int32) bufferIndex;
			}
			else
			{
				ERR_CORE( TXT("Metastore build error: Buffer file without a number (path = '%ls', id = '%ls')"), ANSI_TO_UNICODE(entry->m_path.AsChar()), ANSI_TO_UNICODE(bufferIndexText.AsChar()) );
				valid = false;
				continue;
			}

			// get the buffer prefix name
			entry->m_bufferPrefix = entry->m_path.StringBefore( ".buffer" ).StringBefore( ".", true );
			if ( entry->m_bufferPrefix.Empty() )
			{
				ERR_CORE( TXT("Invalid buffer file path (%ls)"), ANSI_TO_UNICODE(entry->m_path.AsChar()));
				valid = false;
				continue;
			}
		}

		const Uint64 pathHash = Red::CalculateHash64( entry->m_path.AsChar() );

		entries.PushBack( entry );
		entriesMap.Insert( pathHash, entry );
	}

	// link buffers entries with the parent files
	Uint32 maxBufferId = 0;
	Uint32 numBuffers = 0;
	Uint32 numFilesWithBuffers = 0;
	for ( Uint32 i=0; i<entries.Size(); ++i )
	{
		EntryInfo* entry = entries[i];

		// it's a buffer ?
		if ( entry->m_bufferId != -1 )
		{
			// find the master file
			const Uint64 masterHash = Red::CalculateHash64( entry->m_bufferPrefix.AsChar() );
			EntryInfo* masterBufferInfo = nullptr;
			entriesMap.Find( masterHash, masterBufferInfo );

			// master not there - we have a buffer without the parent file
			if ( !masterBufferInfo )
			{
				//! if building patch we do not update buffers.bundle so it is normal that some entity could be removed but buffers are present
				if ( buildingPatch )
				{
					WARN_CORE( TXT("Metastore build error: No master file for buffer '%ls'"), ANSI_TO_UNICODE( entry->m_path.AsChar() ) );
				}
				else
				{
					ERR_CORE( TXT("Metastore build error: No master file for buffer '%ls'"), ANSI_TO_UNICODE( entry->m_path.AsChar() ) );
					valid = false;
				}
				entry->m_ignore = true;				
			}
			else
			{
				// first buffer in the file
				if ( masterBufferInfo->m_buffers.Empty() )
					numFilesWithBuffers += 1;

				// map the buffer in the mask
				masterBufferInfo->m_buffers.PushBack( entry );
				entry->m_bufferOwner = masterBufferInfo;

				// update buffer id
				masterBufferInfo->m_maxBufferId = Max< Int32 >( masterBufferInfo->m_maxBufferId, entry->m_bufferId );
				maxBufferId = Max< Uint32 >( masterBufferInfo->m_maxBufferId, maxBufferId );
				numBuffers += 1;
			}
		}
	}

	// data is not valid
	if ( !valid )
		return false;

	// generate buffer entries
	RED_LOG( WCC, TXT("Number of buffers: %d"), numBuffers );
	RED_LOG( WCC, TXT("Number of files with buffers: %d"), numFilesWithBuffers );
	RED_LOG( WCC, TXT("Max buffer ID: %d"), maxBufferId );

	// validate that we do not have missing buffers
	for ( Uint32 i=0; i<entries.Size(); ++i )
	{
		EntryInfo* entry = entries[i];

		if ( entry->m_buffers.Empty() )
			continue;

		// collect buffer IDs
		TDynArray< EntryInfo* > bufferIds;
		bufferIds.Resize( entry->m_maxBufferId+1 );
		Red::MemoryZero( bufferIds.Data(), bufferIds.DataSize() );
		RED_FATAL_ASSERT( entry->m_maxBufferId >= (Int32)entry->m_buffers.Size(), "Invalid buffer table" );

		for ( EntryInfo* buffer : entry->m_buffers )
		{
			// already added, WTF ?
			if ( bufferIds[ buffer->m_bufferId ] != nullptr )
			{
				ERR_CORE( TXT("BufferID %d for file '%ls' exists twice in the meta data store"), 
					buffer->m_bufferId, ANSI_TO_UNICODE( entry->m_path.AsChar() ) );

				valid = false;
				continue;
			}

			// mount
			bufferIds[ buffer->m_bufferId ] = buffer;
		}

		// do we have holes ?
		const Uint32 numBuffers = (bufferIds.Size() - 1);
		if ( numBuffers != entry->m_buffers.Size() )
		{
			ERR_CORE( TXT("There are %d missing buffers for file '%ls':"), 
				(Int32)numBuffers - (Int32)entry->m_buffers.Size(), 
				ANSI_TO_UNICODE( entry->m_path.AsChar() ) );

			for ( Uint32 i=1; i<bufferIds.Size(); ++i )
			{
				if ( !bufferIds[i] )
				{
					ERR_CORE( TXT("  Buffer ID: %d (missing)"), i );
				}
			}
		}

		// allocate buffer entry
		const Uint32 numBufferEntries = entry->m_maxBufferId;
		const Uint32 firstBufferEntry = (Uint32) m_buffers.Grow( numBufferEntries );

		// fill the FileIDs of the buffers for given master FileID
		for ( Uint32 i=1; i<bufferIds.Size(); ++i )
		{
			const Uint32 bufferEntryIndex = i-1;

			Int32 bufferFileId = -1;
			if ( bufferIds[i] != nullptr )
				bufferFileId = bufferIds[i]->m_fileId;

			m_buffers[ firstBufferEntry + bufferEntryIndex ] = bufferFileId;
		}

		// store the number of buffers and count in the metadata store entry
		m_files[ entry->m_fileId ].m_firstBuffer = firstBufferEntry;
		m_files[ entry->m_fileId ].m_numBuffers = numBufferEntries;
	}

	// data is valid
	return true;
}

namespace Helper
{
	class TempDir;

	class TempDir
	{
	public:
		StringAnsi					m_name;
		TempDir*					m_parent;
		TDynArray< TempDir* >		m_children;
		Int32						m_index;

	public:
		TempDir( TempDir* parent, const AnsiChar* name )
			: m_parent( parent )
			, m_name( name )
			, m_index( -1 )
		{
		}

		~TempDir()
		{
			m_children.ClearPtr();
		}

		TempDir* GetSubDir( const AnsiChar* name )
		{
			for ( TempDir* child : m_children )
			{
				if ( child->m_name == name )
					return child;
			}

			TempDir* child = new TempDir( this, name );
			m_children.PushBack( child );
			return child;
		}

		void Collect( TDynArray< TempDir* >& outDirs )
		{
			// always collect parent dir before children
			m_index = outDirs.SizeInt();
			outDirs.PushBack( this );

			for ( Uint32 i=0; i<m_children.Size(); ++i )
				m_children[i]->Collect( outDirs );
		}
	};

	class TempDepot
	{
	public:
		TempDir*		m_depot;

	public:
		TempDepot()
		{
			m_depot = new TempDir( nullptr, "" );
		}

		~TempDepot()
		{
			delete m_depot;
		}

		TempDir* GetDir( const AnsiChar* path )
		{
			// create the depot directory
			TempDir* directory = m_depot;
			const AnsiChar* start = path;
			const AnsiChar* cur = path;
			while ( *cur )
			{
				if ( *cur == '\\' || *cur == '/' )
				{
					const StringAnsi dirName( start, cur-start );
					directory = directory->GetSubDir( dirName.AsChar() );
					RED_FATAL_ASSERT( directory != nullptr, "Failed to create sub dir - WTF ?" );

					++cur;
					start = cur;
				}
				else
				{
					++cur;
				}
			}

			// return final directory
			return directory;
		}

		void CollectDirectories( TDynArray< TempDir* >& outDirs )
		{
			m_depot->Collect( outDirs ); // depot is directory 0
		}
	};
}

//////////////////////////////////////////////////////////////////////////

Bool CBundleMetadataStoreBuilder::BuildInitData()
{
	// root dir (depot)
	Helper::TempDepot depotHelper;

	// helper data to store in which directory every file is stored
	TDynArray< Helper::TempDir* > fileDirs;
	fileDirs.Resize( m_files.Size() );
	Red::MemoryZero( fileDirs.Data(), fileDirs.DataSize() );

	// reconstruct the directory structure of the files stored in the bundles
	Uint32 numFilesInDirs = 0;
	for ( Uint32 fileID=0; fileID<m_files.Size(); ++fileID )
	{
		const auto& fileInfo = m_files[fileID];
		if ( !fileInfo.m_name )
			continue;

		// get the file path of the file, skip the buffers because we are not adding them to the depot structure
		const AnsiChar* filePath = &m_textTable[ fileInfo.m_name ];
		if ( Red::StringSearch( filePath, ".buffer") != nullptr )
			continue;

		// compute the file path hash - note, we only hash the paths for normal files, not buffers
		// NOTE: the hash collisions are checked in the validation step
		const TDepotFileHash pathHash = Red::System::CalculatePathHash64( filePath );
		m_hashes.Insert( pathHash, fileID );

		// generate directory entry
		Helper::TempDir* dir = depotHelper.GetDir( filePath );
		fileDirs[ fileID ] = dir;
		numFilesInDirs += 1;
	}

	// collect all created directories and create the dir mapping
	TDynArray< Helper::TempDir* > allDirs;
	depotHelper.CollectDirectories( allDirs );
	LOG_CORE( TXT("Found %d directories in bundle file mapping (%d files)"), allDirs.Size(), numFilesInDirs );

	// create the directory entries
	{
		const Uint32 textStartSize = m_textTable.Size();
		m_initDirs.Resize( allDirs.Size() );
		for ( Uint32 i=0; i<allDirs.Size(); ++i )
		{
			Helper::TempDir* tempDir = allDirs[i];
			m_initDirs[i].m_parent = i ? tempDir->m_parent->m_index : 0;
			m_initDirs[i].m_name = AddString( tempDir->m_name.AsChar() );
		}
		LOG_CORE( TXT("Used %1.2fKB for directory names"), (m_textTable.Size() - textStartSize) / 1024.0f );
	}

	// create the file entries (only for non buffered files)
	{
		const Uint32 textStartSize = m_textTable.Size();
		m_initFiles.Reserve( numFilesInDirs );
		for ( Uint32 fileID=0; fileID<m_files.Size(); ++fileID )
		{
			Helper::TempDir* tempDir = fileDirs[fileID];
			if ( !tempDir )
				continue;

			// get the file name with extension part
			const auto& fileInfo = m_files[fileID];
			const AnsiChar* filePath = &m_textTable[ fileInfo.m_name ];
			const AnsiChar* fileName = Red::StringSearchLast( filePath, '\\' );
			fileName = fileName ? (fileName+1) : filePath;

			FileInitInfo initInfo;
			initInfo.m_dirID = tempDir->m_index;
			initInfo.m_fileID = fileID;
			initInfo.m_name = AddString(fileName);
			m_initFiles.PushBack( initInfo );
		}
		LOG_CORE( TXT("Used %1.2fKB for file names"), (m_textTable.Size() - textStartSize) / 1024.0f );
	}

	// done
	return true;
}

//////////////////////////////////////////////////////////////////////////
CAsyncLoadToken* CBundleMetadataStoreBuilder::ReadPreamble( const String& absoluteFilePath, SBundleHeaderPreamble* preamble )
{
	CAsyncLoadToken* preambleToken = m_preambleParser.CreateLoadToken( absoluteFilePath, preamble );
	m_preambleParser.Parse();
	return preambleToken;
}

//////////////////////////////////////////////////////////////////////////
CDebugBundleHeaderParser::HeaderCollection CBundleMetadataStoreBuilder::ReadDebugBundleHeader( const String& absoluteFilePath, const SBundleHeaderPreamble& preamble, void* dstBuffer )
{
	CDebugBundleHeaderParser headerParser( m_asyncIO, preamble );
	CAsyncLoadToken* headerToken = headerParser.CreateLoadToken( absoluteFilePath, dstBuffer );
	// For later use.
	RED_UNUSED( headerToken );
	headerParser.Parse();
	return headerParser.GetHeaderItems();
}

} } }
