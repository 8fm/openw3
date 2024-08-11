/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "patchBuilder.h"
#include "patchBundles.h"
#include "patchUtils.h"

//-----------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CPatchBuilder_Bundles );

//-----------------------------------------------------------------------------

#include "../../common/core/dependencyFileTables.h"
#include "../../common/core/bundlePreamble.h"
#include "../../common/core/bundleFileReaderDecompression.h"
#include "../../common/core/scopedPtr.h"

namespace Helper
{
	/// Helper for writing a bundle composing of files
	class CPatchBundleWriter
	{
	public:
		CPatchBundleWriter()
			: m_totalDataSize( 0 )
		{
			// allocate space for header
			m_currentOffset = sizeof( Red::Core::Bundle::SBundleHeaderPreamble );
		}

		/// Add file entry
		void AddEntry( CPatchBundleFileToken* file )
		{
			// allocate space for the entry
			Entry info;
			info.m_file = file;
			info.m_entryOffset = m_currentOffset;
			info.m_dataSize = file->GetDataSize();
			info.m_order = file->GetFilePath().EndsWith( ".buffer" ) ? 1 : 0;
			m_entries.PushBack(info);

			// count total data size
			m_totalDataSize += file->GetDataSize();

			// advance entry
			m_currentOffset += Red::Core::Bundle::ALIGNED_DEBUG_BUNDLE_HEADER_SIZE;
		}

		/// Save the final bundle
		Bool Save( const String& absolutePath )
		{
			// open file
			Red::TScopedPtr< IFile > file( GFileManager->CreateFileWriter( absolutePath, FOF_AbsolutePath ) );
			if ( !file )
			{
				ERR_WCC( TXT("Unable to open file '%ls' for writing"), absolutePath.AsChar() );
				return false;
			}


			// sort entries
			::Sort( m_entries.Begin(), m_entries.End() );

			// write header
			Red::Core::Bundle::SBundleHeaderPreamble preamble;
			preamble.m_bundleStamp = Red::Core::Bundle::BUNDLE_STAMP;
			preamble.m_burstSize = 0; // no burst read for this bundle
			preamble.m_fileSize = (Uint32)( m_totalDataSize + m_currentOffset );
			preamble.m_headerSize = m_currentOffset - sizeof(preamble);
			file->Serialize( &preamble, sizeof(preamble) );

			// prepare a place for the data offset of the entries
			TDynArray< Uint64 > dataOffsets;
			dataOffsets.Reserve( m_entries.Size() );

			// write the elements allocating the space in the file as we go
			const Uint64 fileAlignment = 4096;
			Uint64 currentDataOffset = AlignOffset( m_currentOffset, fileAlignment );
			for ( const Entry& info : m_entries )
			{
				// prepare the data to write, use the same settings as in the original file (compression especially)
				Red::Core::Bundle::SBundleHeaderItem bundleItem;
				Red::MemoryZero( &bundleItem, sizeof(bundleItem) );
				bundleItem.m_compressionType = info.m_file->GetPlacement().m_compression;
				bundleItem.m_dataOffset = (Uint32) currentDataOffset;
				bundleItem.m_dataSize = info.m_file->GetPlacement().m_sizeInMemory;
				bundleItem.m_compressedDataSize = info.m_file->GetPlacement().m_sizeInBundle;

				// copy resource path
				Red::System::StringCopy( bundleItem.m_rawResourcePath, info.m_file->GetFilePath().AsChar(), Red::Core::Bundle::SBundleHeaderItem::RAW_RESOURCE_PATH_MAX_SIZE );

				// store it
				file->Seek( info.m_entryOffset );
				file->Serialize( (void*)&bundleItem, sizeof(bundleItem) );

				// advance the write offset for the actual file data
				dataOffsets.PushBack( currentDataOffset );
				currentDataOffset += AlignOffset( bundleItem.m_compressedDataSize, fileAlignment );
			}

			// store file data
			for ( Uint32 entryIndex = 0; entryIndex<m_entries.Size(); ++entryIndex )
			{
				const auto& info = m_entries[ entryIndex ];
				Red::TScopedPtr< IFile > srcFile( info.m_file->CreateRawReader() );
				if ( srcFile )
				{
					file->Seek( dataOffsets[ entryIndex ] );
					PatchUtils::CopyFileContent( *srcFile, *file );
				}
			}

			// bundle saved
			LOG_WCC( TXT("Bundle '%ls' saved, %d files, %1.2f KB of data"), 
				absolutePath.AsChar(), m_entries.Size(), currentDataOffset / 1024.0f );
			return true;
		}

	private:
		struct Entry
		{
			CPatchBundleFileToken*		m_file;
			Uint64						m_entryOffset;
			Uint64						m_dataSize; // on disk
			Uint32						m_order;

			RED_INLINE const Bool operator<( const Entry& other ) const
			{
				return m_order < other.m_order;
			}
		};

		Uint32							m_currentOffset;
		Uint64							m_totalDataSize;
		TDynArray< Entry >				m_entries;
	};
}

//-----------------------------------------------------------------------------

CPatchBundleFile::~CPatchBundleFile()
{
	// close the file
	if ( m_file )
	{
		delete m_file;
		m_file = nullptr;
	}
}

static Bool IsAtomicBundle( const String& bundleName )
{
	/*if ( bundleName == TXT("startup") )
		return true;*/

	if ( bundleName.ContainsSubstring( TXT("startup") ))
		return true;

	if ( bundleName == TXT("r4gui") )
		return true;

	if ( bundleName == TXT("r4items") )
		return true;

	return false;
}

CPatchBundleFile* CPatchBundleFile::LoadBundle( const String& absolutePath )
{
	// open access to bundle absolutePath
	IFile* bundleFile = GFileManager->CreateFileReader( absolutePath, FOF_AbsolutePath | FOF_Buffered );
	if ( !bundleFile )
	{
		ERR_WCC( TXT("Unable to open bundle file %ls for reading"), absolutePath.AsChar() );
		return false;
	}

	// load the preamble
	Red::Core::Bundle::SBundleHeaderPreamble preamble;
	Red::MemoryZero( &preamble, sizeof(preamble) );
	bundleFile->Serialize( &preamble, sizeof(preamble) );

	// not a bundle
	if ( preamble.m_bundleStamp != Red::Core::Bundle::BUNDLE_STAMP )
	{
		delete bundleFile;
		ERR_WCC( TXT("Bundle header stamp mismatch in '%ls'"), absolutePath.AsChar() );
		return nullptr;
	}

	// not a bundle
	if ( preamble.m_headerVersion != Red::Core::Bundle::BUNDLE_HEADER_VERSION )
	{
		delete bundleFile;
		ERR_WCC( TXT("Bundle header version mismatch in '%ls'"), absolutePath.AsChar() );
		return nullptr;
	}

	// the output file
	CPatchBundleFile* ret = new CPatchBundleFile();
	ret->m_file = bundleFile;
	ret->m_absoluteFilePath = absolutePath;

	// determine the bundle name
	const CFilePath bundleFilePath( absolutePath );
	ret->m_shortName = bundleFilePath.GetFileName();
	ret->m_isAtomic = IsAtomicBundle( ret->m_shortName );

	// determine the relative path
	if ( absolutePath.ContainsSubstring( TXT("content\\") ) )
	{
		ret->m_relativePath = String( TXT("content\\") ) + absolutePath.StringAfter( TXT("content\\") );
		ret->m_isDLC = false;
	}
	else if ( absolutePath.ContainsSubstring( TXT("dlc\\") ) )
	{
		ret->m_relativePath = String( TXT("dlc\\") ) + absolutePath.StringAfter( TXT("dlc\\") );
		ret->m_isDLC = true;
	}
	else
	{
		delete bundleFile;
		ERR_WCC( TXT("Bundle '%ls' is not in content or DLC directory. Contaminated build."), absolutePath.AsChar() );
		return nullptr;
	}

	// read the entries
	const Uint32 numItems = (preamble.m_headerSize / Red::Core::Bundle::ALIGNED_DEBUG_BUNDLE_HEADER_SIZE);
	ret->m_entries.Reserve( numItems );
	ret->m_names.Reserve( numItems );
	for ( Uint32 i=0; i<numItems; ++i )
	{
		// move to the right place in the file
		const Uint32 fileOffset = sizeof(preamble) + (i * Red::Core::Bundle::ALIGNED_DEBUG_BUNDLE_HEADER_SIZE);
		ret->m_file->Seek( fileOffset );

		// keep reading
		Red::Core::Bundle::SBundleHeaderItem fileInfo;
		Red::MemoryZero( &fileInfo, sizeof(fileInfo) );
		ret->m_file->Serialize( &fileInfo, sizeof(fileInfo) );

		// create placement entry
		Red::Core::Bundle::SMetadataFileInBundlePlacement placement;
		placement.m_bundleID = 0;
		placement.m_fileID = 0;
		placement.m_compression = fileInfo.m_compressionType;
		placement.m_offsetInBundle = fileInfo.m_dataOffset;
		placement.m_sizeInBundle = fileInfo.m_compressedDataSize;
		placement.m_sizeInMemory = fileInfo.m_dataSize;
		ret->m_entries.PushBack( placement );
		ret->m_names.PushBack( fileInfo.m_rawResourcePath );
	}

	// return loaded bundle file
	return ret;
}

IFile* CPatchBundleFile::LoadFileData( const TFilePlacement& placement ) const
{
	if ( placement.m_compression == Red::Core::Bundle::CT_Uncompressed )
	{
		// direct access
		RED_FATAL_ASSERT( placement.m_sizeInBundle == placement.m_sizeInMemory, "Invalid bundle file placement" );
		return new PatchUtils::COffsetFileReader( m_file, placement.m_offsetInBundle, placement.m_sizeInBundle );
	}
	else
	{
		Uint8* diskMemory = (Uint8*) PatchUtils::AllocateTempMemory();

		// load compressed file data
		m_file->Seek( placement.m_offsetInBundle );
		m_file->Serialize( diskMemory, placement.m_sizeInBundle );

		// allocate decompressed memory
		Uint8* fileMemory = diskMemory + placement.m_sizeInBundle;
		if ( !BundleFileReaderDecompression::DecompressFileBufferSynch( 
			(Red::Core::Bundle::ECompressionType) placement.m_compression, 
			diskMemory, placement.m_sizeInBundle,
			fileMemory, placement.m_sizeInMemory ) )
		{
			return nullptr;
		}

		// create memory based reader
		return new PatchUtils::CSimpleMemoryFileReader( fileMemory, placement.m_sizeInMemory );
	}
}

IFile* CPatchBundleFile::LoadRawFileData( const TFilePlacement& placement ) const
{
	return new PatchUtils::COffsetFileReader( m_file, placement.m_offsetInBundle, placement.m_sizeInBundle );
}

Bool CPatchBundleFile::Copy( const String& absolutePath ) const
{
	m_file->Seek( 0 );
	return PatchUtils::CopyFileContent( *m_file, absolutePath );
}

//-----------------------------------------------------------------------------

void CPatchBundles::GetTokens( TDynArray< IBasePatchContentBuilder::IContentToken* >& outTokens ) const
{
	outTokens.Reserve( m_tokens.Size() );
	for ( auto* ptr : m_tokens )
		outTokens.PushBack( ptr );
}

const Uint64 CPatchBundles::GetDataSize() const
{
	Uint64 totalSize = 0;
	for ( auto* ptr : m_tokens )
		totalSize += ptr->GetDataSize();
	return totalSize;
}

const String CPatchBundles::GetInfo() const
{
	return TXT("Bundles");
}

extern Bool GPatchingMod;

CPatchBundles* CPatchBundles::LoadBundles( const String& baseDirectory )
{
	// enumerate bundle files at given directory
	TDynArray< String > bundlePaths;
	GFileManager->FindFiles( baseDirectory, TXT("*.bundle"), bundlePaths, true );

	// map with current tokens
	THashMap< Uint64, CPatchBundleFileToken* > fileTokensMap;

	// loaded bundles
	CPatchBundles* bundles = new CPatchBundles();
	bundles->m_basePath = baseDirectory;

	// load the bundles
	LOG_WCC( TXT("Found %d bundles in build"), bundlePaths.Size() );
	for ( const String& bundlePath : bundlePaths )
	{
		// skip non content files
		if ( !PatchUtils::IsContentOrDlcPath( bundlePath ) && !GPatchingMod )
		{
			LOG_WCC( TXT("File '%ls' will be skipped because it's not part of the main content or dlc"), bundlePath.AsChar() );
			continue;
		}
		
		// load the bundle
		LOG_WCC( TXT("Loading bundle '%ls'..."), bundlePath.AsChar() );
		CPatchBundleFile* bundle = CPatchBundleFile::LoadBundle( bundlePath );
		if ( !bundle )
		{
			delete bundles;
			ERR_WCC( TXT("Failed to load bundle from '%ls'. The build is not valid."), bundlePath.AsChar() );
			return nullptr;
		}

		// add to bundle list
		bundles->m_bundles.PushBack( bundle );

		// process bundle entries
		const Uint32 numFilesInBundle = bundle->GetFileEntries().Size();
		LOG_WCC( TXT("Found %d files in bundle"), numFilesInBundle );
		for ( Uint32 i=0; i<numFilesInBundle; ++i )
		{
			// get file name
			const StringAnsi fileName = bundle->GetFileNames()[i];

			// already has a token ?
			const Uint64 fileNameHash = Red::CalculateHash64( fileName.AsChar() );
			CPatchBundleFileToken* fileToken = nullptr;
			if ( fileTokensMap.Find( fileNameHash, fileToken ) )
			{
				fileToken->m_owningBundles.PushBack( bundle ); // remember all owning bundles
				continue;
			}

			// create file mapping
			const auto& placement = bundle->GetFileEntries()[i];
			fileToken = new CPatchBundleFileToken( fileName, bundle, placement );
			fileToken->m_owningBundles.PushBack( bundle );

			// insert into token map
			fileTokensMap.Insert( fileNameHash, fileToken );
			bundles->m_tokens.PushBack( fileToken );
		}
	}

	// get the UNCOMPRESSED data CRC
	LOG_WCC( TXT("Found %d tokens in all of the bundles"), bundles->m_tokens.Size() );
	Int32 lastPrc = -1;
	for ( Uint32 i=0; i<bundles->m_tokens.Size(); ++i )
	{
		auto* token = bundles->m_tokens[i];

		// refresh task progress
		const Int32 prc = (100*i) / bundles->m_tokens.Size();
		if ( prc != lastPrc )
		{
			LOG_WCC( TXT("Status: Calculating CRC... %d%%..."), prc );
			lastPrc = prc;
		}

		// refresh CRC
		if ( !token->RefreshCRC() )
		{
			ERR_WCC( TXT("Failed to compute CRC for bundle file '%hs'"),
				token->GetFilePath().AsChar(),
				token->GetBundleFile()->GetAbsolutePath().AsChar() );

			delete bundles;
			return nullptr;
		}
	}

	// return loaded bundles
	return bundles;
}

//-----------------------------------------------------------------------------

CPatchBundleFileToken::CPatchBundleFileToken( const StringAnsi& filePath, CPatchBundleFile* bundleFile, const TFilePlacement& placement )
	: m_filePath( filePath )
	, m_fileHash( Red::CalculateHash64( filePath.AsChar() ) )
	, m_bundlePlacement( placement )
	, m_bundleFile( bundleFile )
	, m_dataCRC( 0 )
{
}

CPatchBundleFileToken::~CPatchBundleFileToken()
{
}

IFile* CPatchBundleFileToken::CreateReader() const
{
	return m_bundleFile->LoadFileData( m_bundlePlacement );
}

IFile* CPatchBundleFileToken::CreateRawReader() const
{
	return m_bundleFile->LoadRawFileData( m_bundlePlacement );
}

Bool CPatchBundleFileToken::RefreshCRC()
{
	RED_FATAL_ASSERT( !m_owningBundles.Empty(), "We need to know our bundles" );

	// open file access
	Red::TScopedPtr< IFile > file( m_bundleFile->LoadFileData( m_bundlePlacement ) );
	if ( !file )
	{
		ERR_WCC( TXT("Failed to compute CRC for '%hs'"), m_filePath.AsChar() );
		return false;
	}

	// Get file size
	const Uint64 fileSize = file->GetSize();

	// CRC
	Uint64 crc = RED_FNV_OFFSET_BASIS64;

	// is this a resource file ? If so - skip the header :)
	Uint32 magic = 0;
	file->Serialize( &magic, sizeof(magic) );
	file->Seek( 0 ); 

	// in case of the resource file we will process it a little differently
	// basically we will only gather the CRC of the exports/imports + inplace file
	if ( magic == CDependencyFileData::FILE_MAGIC )
	{
		// dependency file CRC
		crc = PatchUtils::CalcDependencyFileCRC( file.Get(), crc );
	}
	else
	{
		// whole file buffer
		crc = PatchUtils::CalcFileBlockCRC( file.Get(), file->GetSize(), crc );
	}

	// take the data size and compression into account
	// this is required because of the LACK OF DETERMINISM in the compression selection :(
	if ( m_owningBundles.Size() > 1 )
	{
		crc = Red::CalculateHash64( &m_bundlePlacement.m_sizeInMemory, sizeof(m_bundlePlacement.m_sizeInMemory), crc );
		crc = Red::CalculateHash64( &m_bundlePlacement.m_sizeInBundle, sizeof(m_bundlePlacement.m_sizeInBundle), crc );
		crc = Red::CalculateHash64( &m_bundlePlacement.m_compression, sizeof(m_bundlePlacement.m_compression), crc );
	}

	// calculated
	m_dataCRC = crc;
	return true;
}	

const Uint64 CPatchBundleFileToken::GetTokenHash() const
{
	return m_fileHash;
}

const Uint64 CPatchBundleFileToken::GetDataCRC() const
{
	RED_FATAL_ASSERT( m_dataCRC != 0, "Token used with invalid CRC" );
	return m_dataCRC;
}

const Uint64 CPatchBundleFileToken::GetDataSize() const
{
	return m_bundlePlacement.m_sizeInBundle;
}

const String CPatchBundleFileToken::GetInfo() const
{
	return String::Printf( TXT("Bundle file '%hs'"), m_filePath.AsChar() );
}

const String CPatchBundleFileToken::GetAdditionalInfo() const
{
	return String::Printf( TXT("(mem size: %d, compression: %d, bundles: %d)"), m_bundlePlacement.m_sizeInMemory, m_bundlePlacement.m_compression, m_owningBundles.Size() );
}

void CPatchBundleFileToken::DebugDump( const String& dumpPath, const Bool isBase ) const
{
	String absoluePath = dumpPath;
	absoluePath += ANSI_TO_UNICODE( m_filePath.AsChar() );
	absoluePath += isBase ? TXT(".base") : TXT(".current");

	Red::TScopedPtr< IFile > srcFile( m_bundleFile->LoadFileData( m_bundlePlacement ) );
	Red::TScopedPtr< IFile > destFile( GFileManager->CreateFileWriter( absoluePath.AsChar(), FOF_AbsolutePath ) );
	if ( srcFile && destFile )
	{
		Uint8 copyBlock[ 64*1024 ];
		
		const Uint64 fileSize = srcFile->GetSize();
		LOG_WCC( TXT("Dumping %d bytes to '%ls'"), fileSize, absoluePath.AsChar() );

		while ( srcFile->GetOffset() < fileSize )
		{
			const Uint64 maxRead = Min< Uint64 >( sizeof(copyBlock), fileSize - srcFile->GetOffset() );
			srcFile->Serialize( copyBlock, maxRead );
			destFile->Serialize( copyBlock, maxRead );
		}
	}
}

//-----------------------------------------------------------------------------

CPatchBuilder_Bundles::CPatchBuilder_Bundles()
{
}

CPatchBuilder_Bundles::~CPatchBuilder_Bundles()
{
}

String CPatchBuilder_Bundles::GetContentType() const
{
	return TXT("bundles");
}

CPatchBuilder_Bundles::IContentGroup* CPatchBuilder_Bundles::LoadContent( const ECookingPlatform platform, const String& absoluteBuildPath )
{
	return CPatchBundles::LoadBundles( absoluteBuildPath );
}

Bool CPatchBuilder_Bundles::SaveContent( const ECookingPlatform platform, IContentGroup* baseGroup, IContentGroup* patchGroup, TDynArray< IContentToken* >& patchContent, const String& absoluteBuildPath, const String& patchName )
{
	// Create a map of original content
	THashMap< Uint64, CPatchBundleFileToken* > baseFiles;
	{
		CPatchBundles* baseBundles = static_cast< CPatchBundles* >( baseGroup );

		TDynArray< CPatchBundleFileToken* > tokens;
		baseBundles->GetTokens( (TDynArray< IContentToken*> &) tokens );

		for ( CPatchBundleFileToken* token : tokens )
		{
			baseFiles.Insert( token->GetTokenHash(), token );
		}
	}

	// Copy the atomic bundles
	TDynArray< const CPatchBundleFile* > atomicBundles;
	TDynArray< CPatchBundleFileToken* > looseFiles;
	for ( Uint32 i=0; i<patchContent.Size(); ++i )
	{
		// determine if we should copy the whole bundle or just add this file to the "difference" bundle
		CPatchBundleFileToken* token = static_cast< CPatchBundleFileToken* >( patchContent[i] );

		// are we in any of the atomic bundles ?
		Bool isInNormalBundle = false;
		for ( const auto* bundle : token->m_owningBundles )
		{
			if ( bundle->IsAtomic() )
			{
				atomicBundles.PushBackUnique( bundle );
				LOG_WCC( TXT("Pached file '%hs' is in atomic bundle '%ls'"), token->GetFilePath().AsChar(), bundle->GetShortName().AsChar() );
			}
			else
			{
				isInNormalBundle = true;
				LOG_WCC( TXT("Pached file '%hs' is in normal bundle '%ls'"), token->GetFilePath().AsChar(), bundle->GetShortName().AsChar() );
			}
		}

		// ok, another edge case: if the file WAS or IS in any other non-atomic bundles we need to add it to the loose file blob
		if ( !isInNormalBundle )
		{
			CPatchBundleFileToken* baseFile = nullptr;
			baseFiles.Find( token->GetTokenHash() );

			if ( baseFile )
			{
				for ( const auto* bundle : token->m_owningBundles )
				{
					if ( !bundle->IsAtomic() )
					{
						LOG_WCC( TXT("Pached file '%hs' WAS in normal bundle '%ls'. Ha Ha."), token->GetFilePath().AsChar(), bundle->GetShortName().AsChar() );
						isInNormalBundle = true;
						break;
					}
				}
			}
		}

		// if a file was only used in atomic bundles than we don't have to include it as a separate file
		if ( isInNormalBundle )
		{
			looseFiles.PushBack( token );
			LOG_WCC( TXT("Pached file '%hs' will be added as loosely patched file"), token->GetFilePath().AsChar() );
		}
	}

	// Copy atomic bundles
	if ( !atomicBundles.Empty() )
	{
		LOG_WCC( TXT("Found %d atomic bundles in patch. Dex is sad. EdgeCast is happy."), atomicBundles.Size() );

		for ( const auto* bundle : atomicBundles )
		{
			// when patching atomic bundles place them in the same directory because they are patched directly
			const String outputFilePath = absoluteBuildPath + bundle->GetRelativePath();
			if ( !bundle->Copy( outputFilePath ) )
			{
				ERR_WCC( TXT("Failed to copy patch bundle '%ls'"), outputFilePath.AsChar() );
				return false;
			}
		}
	}

	// Create merged bundle with all of the loose files
	if ( !looseFiles.Empty() )
	{
		Helper::CPatchBundleWriter writer;

		// add entries
		for ( auto* file : looseFiles )
		{
			writer.AddEntry( file );
		}

		// save the bundle
		const String outputFilePath = absoluteBuildPath + patchName + TXT("\\bundles\\patch.bundle");
		if ( !writer.Save( outputFilePath ) )
		{
			ERR_WCC( TXT("Failed to save patch bundle '%ls'"), outputFilePath.AsChar() );
			return false;
		}
	}

	// Done, patch data saved!
	return true;
}

//-----------------------------------------------------------------------------
