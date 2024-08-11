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

	namespace Helper
	{
		struct AnsiToUnicode
		{
			Char		m_buf[ 512 ];

			const Char* Convert( const AnsiChar* txt )
			{
				Char* write = m_buf;
				const AnsiChar* read = txt;
				while ( *read )
				{
					*write++ = (AnsiChar) *read++;
				}
				*write = 0;
				return m_buf;
			}

			const Char* Get() const { return m_buf; }
		};
	}

	Bool CBundleMetadataStore::PopulateDepot( CDepot* depot, TDynArray< CBundleDiskFile*, MC_Depot >& outFileList, TDepotFileMap& outFileHashes, const Red::Core::Bundle::FileID baseFileID /*= 0*/, const Bool isDLC /*= false*/, TSortedMap< Red::Core::Bundle::FileID, CBundleDiskFile*, DefaultCompareFunc< Red::Core::Bundle::FileID >, MC_Depot >* patchedDLCFiles /*= nullptr*/ ) const
	{
		// no init data
		if ( m_initDirs.Empty() || m_initFiles.Empty() )
			return false;

		// setup directory map, the dir0 is the depot
		TDynArray< CDirectory* > depotDirs;
		depotDirs.Resize( m_initDirs.Size() );
		depotDirs[0] = depot;

		// conversion helper (much faster than normal ANSI_TO_UNICODE)
		Helper::AnsiToUnicode ansi2uni;

		// create directories, note that we skip the dir0 (depot)
		for ( Uint32 i=1; i<m_initDirs.Size(); ++i )
		{
			const auto& dirInfo = m_initDirs[i];
			const AnsiChar* dirName = &m_textTable[ dirInfo.m_name ];

			RED_FATAL_ASSERT( dirInfo.m_parent < i, "Invalid directory ordering" );
			CDirectory* parentDir = depotDirs[ dirInfo.m_parent ];
			RED_FATAL_ASSERT( parentDir != nullptr, "NULL parent directory" );
			CDirectory* curDir = parentDir->CreateNewDirectory( ansi2uni.Convert(dirName), /* batch */ true );
			RED_FATAL_ASSERT( parentDir != nullptr, "Failed to create directory '%hs'", dirName );
			depotDirs[i] = curDir;
		}

		// output list if indexed with the FileID
		const Uint32 prevSize = outFileList.Size();
		outFileList.Resize( m_files.Size() + baseFileID );
		for ( Uint32 i=prevSize; i<outFileList.Size(); ++i )
			outFileList[i] = nullptr;

		// create the file wrappers and bind them to the file IDs
		for ( Uint32 i=0; i<m_initFiles.Size(); ++i )
		{
			const auto& fileInfo = m_initFiles[i];

			CDirectory* parentDir = depotDirs[ fileInfo.m_dirID ];
			RED_FATAL_ASSERT( parentDir != nullptr, "No dir created for file" );
			const AnsiChar* fileName = &m_textTable[ fileInfo.m_name ];

			ansi2uni.Convert(fileName);
			if ( isDLC )
			{
				// File is already here from a patch or integrated as base content. Mark as runtime attached
				// so the DLC manager knows the intended source of the file. E.g., could be a *.reddlc file and
				// we shouldn't use this file until the DLC itself has been completely installed			
				CDiskFile* const existingDiskFile = parentDir->FindLocalFile( ansi2uni.Get() );
				if ( existingDiskFile )
				{
					existingDiskFile->MarkAsRuntimeAttached();
					//! we have to save handle to original file
					//! ex. in situation when we need get list of buffers from original DLC file 
					if( patchedDLCFiles != nullptr )
					{
						CBundleDiskFile* bundleFile = new CBundleDiskFile( parentDir, ansi2uni.Get(), fileInfo.m_fileID + baseFileID );
						patchedDLCFiles->Insert(((const CBundleDiskFile*)existingDiskFile)->GetFileID(), bundleFile);
					}
					continue;
				}
			}

			CBundleDiskFile* bundleFile = new CBundleDiskFile( parentDir, ansi2uni.Get(), fileInfo.m_fileID + baseFileID );
			if ( isDLC )
				bundleFile->MarkAsRuntimeAttached();

			outFileList[ fileInfo.m_fileID + baseFileID ] = bundleFile; // uses local id

			parentDir->AddFile( bundleFile, /*batch*/ true );
		}

		// extract file hashes
		outFileHashes.Reserve( outFileHashes.Size() + m_hashes.Size() );
		for ( const auto& it : m_hashes )
		{
			const auto hash = it.m_first;
			const auto fileId = it.m_second;

			CBundleDiskFile* file = outFileList[ fileId + baseFileID ];
			if ( file )
			{
				outFileHashes.BulkInsert( hash, file );
			}
		}

		// loaded
		return true;
	}

	// NOTE: this function is SLOW because it's not using the metadata store initialization data
	// on the other hand it's much safer and, since we are using the MergeDepot only for mods on PC it does not really matter
	Bool CBundleMetadataStore::MergeDepot( CDepot* depot, const TDynArray< Red::Core::Bundle::FileID >& addedFiles, const TDynArray< Red::Core::Bundle::FileID >& addedFilesOrginalIDs, const TDynArray< Red::Core::Bundle::FileID >& overridenFiles, TDynArray< CBundleDiskFile*, MC_Depot >& outFileList, TSortedMap< Uint64, CDiskFile*, DefaultCompareFunc< Uint64 >, MC_Depot >& outFileHashes, const Red::Core::Bundle::FileID baseFileID /*= 0*/ ) const
	{
		// conversion helper (much faster than normal ANSI_TO_UNICODE)
		Helper::AnsiToUnicode ansi2uni;

		// get the max FileID for the new files
		{
			Uint32 maxFileId = outFileList.Size();
			for ( Uint32 i=0; i<addedFiles.Size(); ++i )
			{
				const FileID newFileID = addedFiles[i]; // this is the new fileID
				maxFileId = Max< Uint32 >( maxFileId, newFileID+1 );
			}

			// reserve space in mapping for the new files
			const Uint32 prevSize = outFileList.Size();
			outFileList.Resize( maxFileId );

			for ( Uint32 i=prevSize; i<outFileList.Size(); ++i )
				outFileList[i] = nullptr;
		}

		// add new files (rare)
		for ( Uint32 i=0; i<addedFiles.Size(); ++i )
		{
			const FileID newFileID = addedFiles[i]; // this is the new fileID
			RED_FATAL_ASSERT( newFileID >= baseFileID, "Invalid file ID for new file" );

			const FileID originalFileId = addedFilesOrginalIDs[i];
			RED_FATAL_ASSERT( originalFileId >= 1 && originalFileId < m_files.Size(), "Invalid original file ID" );

			const auto& originalFileInfo = m_files[ originalFileId ];

			const AnsiChar* fileName = &m_textTable[ originalFileInfo.m_name ];
			const UniChar* fileNameUni = ansi2uni.Convert(fileName);

			CDirectory* parentDir = GDepot->CreatePath( fileNameUni );
			if ( parentDir )
			{
				// create new file entry and map it
				CBundleDiskFile* bundleFile = new CBundleDiskFile( parentDir, fileNameUni, newFileID );
				outFileList[ newFileID ] = bundleFile;

				// add to hash list
				const Uint64 pathHash = Red::CalculatePathHash64( fileNameUni );

				if( outFileHashes.KeyExist(pathHash) )
				{
					RED_FATAL_ASSERT( !outFileHashes.KeyExist(pathHash), "File that we think is new is already in the map" );
					continue;
				}

				RED_FATAL_ASSERT( !outFileHashes.KeyExist(pathHash), "File that we think is new is already in the map" );
				outFileHashes.Insert( pathHash, bundleFile );
			}
		}

		// for modified files we need to mark them as overridden
		for ( Uint32 i=0; i<overridenFiles.Size(); ++i )
		{
			const FileID overriddenFileID = overridenFiles[i];
			RED_FATAL_ASSERT( overriddenFileID != 0, "Invalid FileID" );

			CBundleDiskFile* bundleFile = (CBundleDiskFile*) outFileList[ overriddenFileID ];
			if ( bundleFile != nullptr )
			{
				RED_FATAL_ASSERT( bundleFile->GetFileID() == overriddenFileID, "No bundle file for overriden file" );
				bundleFile->MarkAsOverridden();
			}
		}

		// done
		return true;
	}

} } }

