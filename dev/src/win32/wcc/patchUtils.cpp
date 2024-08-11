/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "patchBuilder.h"
#include "patchUtils.h"

#include "../../common/core/dependencyFileTables.h"
#include "../bundlebuilder/bundle.h"
#include "../../common/core/compression/zlib.h"
#include "../../common/core/compression/lz4hc.h"
#include "../../common/core/compression/doboz.h"

namespace PatchUtils
{

	//-------------------------------------------------------------------------------

	void* AllocateTempMemory()
	{
		static void* tempMemory = nullptr;
		if ( !tempMemory )
		{
			// our memory allocator is CRAP, we need to allocate form system directly
			tempMemory = VirtualAlloc( NULL, 512 * 1024 * 1024, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
			if ( !tempMemory )
			{
				const auto errCode = GetLastError();
				ERR_WCC( TXT("VirtualAllocError: %d (0x%08X)"), errCode, errCode );
			}
			//RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_Temporary, , 1024 );
		}
		return tempMemory;
	}
	//-------------------------------------------------------------------------------

	COffsetFileReader::COffsetFileReader( IFile* base, Uint64 offset, Uint32 size )
		: IFile( FF_Reader | FF_FileBased )
		, m_base( base )
		, m_offset( offset )
		, m_size( size )
	{
		m_base->Seek( offset );
		RED_FATAL_ASSERT( GetOffset() == 0, "Invalid shit" );
	}

	void COffsetFileReader::Serialize( void* buffer, size_t size )
	{
		return m_base->Serialize( buffer, size );
	}

	Uint64 COffsetFileReader::GetOffset() const
	{
		return m_base->GetOffset() - m_offset;
	}

	Uint64 COffsetFileReader::GetSize() const
	{
		return m_size;
	}

	void COffsetFileReader::Seek( Int64 offset )
	{
		RED_FATAL_ASSERT( offset <= (Int64)m_size, "Invalid seek offset %d", offset );
		m_base->Seek( offset + m_offset );
	}

	//-------------------------------------------------------------------------------

	CSimpleMemoryFileReader::CSimpleMemoryFileReader( void* memory, Uint32 size )
		: IFile( FF_Reader | FF_FileBased )
		, m_data( memory )
		, m_offset( 0 )
		, m_size( size )
	{}

	void CSimpleMemoryFileReader::Serialize( void* buffer, size_t size )
	{
		RED_FATAL_ASSERT( m_offset + size <= m_size, "Out of bounds file read %d+%d > %d", m_offset, size, m_size );
		Red::MemoryCopy( buffer, OffsetPtr(m_data, m_offset), size );
		m_offset += size;
	}

	Uint64 CSimpleMemoryFileReader::GetOffset() const
	{
		return m_offset;
	}

	Uint64 CSimpleMemoryFileReader::GetSize() const
	{
		return m_size;
	}

	void CSimpleMemoryFileReader::Seek( Int64 offset )
	{
		RED_FATAL_ASSERT( offset <= (Int64)m_size, "Invalid seek offset %d", offset );
		m_offset = (Uint64)offset;
	}

	//-------------------------------------------------------------------------------

	/// Copy file content
	Bool CopyFileContent( IFile& srcFile, IFile& destFile )
	{
		Uint8 copyBlock[ 64*1024 ];

		srcFile.Seek(0);

		const Uint64 fileSize = srcFile.GetSize();
		while ( srcFile.GetOffset() < fileSize )
		{
			if ( srcFile.HasErrors() || destFile.HasErrors() )
				return false;

			const Uint64 maxRead = Min< Uint64 >( sizeof(copyBlock), fileSize - srcFile.GetOffset() );
			srcFile.Serialize( copyBlock, maxRead );
			destFile.Serialize( copyBlock, maxRead );
		}

		return true;
	}

	/// Copy file content and compute its crc
	Bool CopyFileContent( IFile& srcFile, IFile& destFile, Uint32& outCrc )
	{
		Red::System::CRC32 crcCalculator;

		Uint8 copyBlock[ 64*1024 ];

		srcFile.Seek(0);

		outCrc = 0;

		const Uint64 fileSize = srcFile.GetSize();
		while ( srcFile.GetOffset() < fileSize )
		{
			if ( srcFile.HasErrors() || destFile.HasErrors() )
				return false;

			const Uint64 maxRead = Min< Uint64 >( sizeof(copyBlock), fileSize - srcFile.GetOffset() );
			srcFile.Serialize( copyBlock, maxRead );
			destFile.Serialize( copyBlock, maxRead );

			outCrc = crcCalculator.Calculate( copyBlock, static_cast< Uint32 >( maxRead ), outCrc );
		}

		return true;
	}

	/// Copy file content to absolute path
	Bool CopyFileContent( IFile& srcFile, const String& destPath )
	{
		Red::TScopedPtr<IFile> destFile( GFileManager->CreateFileWriter( destPath, FOF_AbsolutePath ) );
		if ( !destFile )
		{
			ERR_WCC( TXT("Unable to open file '%ls' for writing"), destPath.AsChar() );
			return false;
		}

		if ( !CopyFileContent( srcFile, *destFile ) )
		{
			ERR_WCC( TXT("Unable to copy file '%ls' from '%ls'"), destPath.AsChar(), srcFile.GetFileNameForDebug() );
			return false;
		}

		// stats
		const Uint64 copiedSize = srcFile.GetSize();
		LOG_WCC( TXT("Copied '%ls' (%1.2f KB)"), destPath.AsChar(), copiedSize / 1024.0f );

		// file copied
		return true;
	}

	/// Copy file content from one absolute place to other to absolute place
	Bool CopyFileContent( const String& srcPath, const String& destPath )
	{
		Red::TScopedPtr<IFile> srcFile( GFileManager->CreateFileReader( srcPath, FOF_AbsolutePath ) );
		if ( !srcFile )
		{
			ERR_WCC( TXT("Unable to open file '%ls' for reading"), srcPath.AsChar() );
			return false;
		}

		Red::TScopedPtr<IFile> destFile( GFileManager->CreateFileWriter( destPath, FOF_AbsolutePath ) );
		if ( !destFile )
		{
			ERR_WCC( TXT("Unable to open file '%ls' for writing"), destPath.AsChar() );
			return false;
		}

		if ( !CopyFileContent( *srcFile, *destFile ) )
		{
			ERR_WCC( TXT("Unable to copy file '%ls' from '%ls'"), destPath.AsChar(), srcPath.AsChar() );
			return false;
		}

		// stats
		const Uint64 copiedSize = srcFile->GetSize();
		LOG_WCC( TXT("Copied '%ls' (%1.2f KB)"), destPath.AsChar(), copiedSize / 1024.0f );

		// file copied
		return true;
	}

	/// Copy data from one file and compress it
	Bool CompressFileContentZLIB( IFile& srcFile, IFile& destFile )
	{
		// load source data
		TDynArray< Uint8 > sourceData( srcFile.GetSize() );
		srcFile.Serialize( sourceData.Data(), sourceData.DataSize() );

		// compress data using the compressor
		Red::Core::Compressor::CZLib zlib;
		if ( !zlib.Compress( sourceData.Data(), (Uint32) sourceData.DataSize(), 9 ) )
			return false;

		// save compressed data
		destFile.Serialize( (void*)zlib.GetResult(), zlib.GetResultSize() );
		return true;
	}

	/// Copy data from one file and compress it
	Bool CompressFileContentDoboz( IFile& srcFile, IFile& destFile )
	{
		// load source data
		TDynArray< Uint8 > sourceData( srcFile.GetSize() );
		srcFile.Serialize( sourceData.Data(), sourceData.DataSize() );

		// compress data using the compressor
		Red::Core::Compressor::CDoboz zlib;
		if ( !zlib.Compress( sourceData.Data(), (Uint32) sourceData.DataSize() ) )
			return false;

		// save compressed data
		destFile.Serialize( (void*)zlib.GetResult(), zlib.GetResultSize() );
		return true;
	}

	/// Copy data from one file and compress it
	Bool CompressFileContentLZ4HC( IFile& srcFile, IFile& destFile )
	{
		// load source data
		TDynArray< Uint8 > sourceData( srcFile.GetSize() );
		srcFile.Serialize( sourceData.Data(), sourceData.DataSize() );

		// compress data using the compressor
		Red::Core::Compressor::CLZ4HC zlib;
		if ( !zlib.Compress( sourceData.Data(), (Uint32) sourceData.DataSize() ) )
			return false;

		// save compressed data
		destFile.Serialize( (void*)zlib.GetResult(), zlib.GetResultSize() );
		return true;
	}

	/// Calculate CRC of data block in file
	Uint64 CalcFileBlockCRC( IFile* file, const Uint64 size, Uint64 crc )
	{
		const Uint64 endOffset = file->GetOffset() + size;

		// calculate the CRC
		Uint8 crcBlock[ 1024*1024 ];
		while ( file->GetOffset() < endOffset )
		{
			const Uint64 maxRead = Min< Uint64 >( sizeof(crcBlock), endOffset - file->GetOffset() );
			file->Serialize( crcBlock, maxRead );

			crc = Red::CalculateHash64( crcBlock, maxRead, crc );
		}

		// return updated CRc
		return crc;
	}

	/// Check is property is in the skip list
	Bool IsIgnored( const AnsiChar* name, const AnsiChar** ignoredProperties, Uint32 numIgnoredProperties )
	{
		for ( Uint32 i=0; i<numIgnoredProperties; ++i )
		{
			if ( 0 == Red::StringCompare( name, ignoredProperties[i] ) )
			{
				return true;
			}
		}

		return false;
	}

	/// Calculate CRC based on the property data (slow)
	Uint64 CalcPropertyBasedCRC( IFile* file, const Uint64 expectedEndOffset, const TDynArray< const AnsiChar* >& resolvedNames, const AnsiChar** ignoredProperties, Uint32 numIgnoredProperties, Uint64 crc )
	{		
		// read serialization flags
		Uint8 serializationFlags = 0;
		*file << serializationFlags;

		// properties or data stream
		if ( 0 == (serializationFlags & FLAG(2) /*eSerializationFlag_CookedStream*/ ) )
		{
			// parse the properties
			while ( 1 )
			{
				RED_FATAL_ASSERT( file->GetOffset() < expectedEndOffset, "Unexpected end of data stream" );

				// read property name
				Uint16 nameIndex = 0;
				*file << nameIndex;

				// end of property list
				if ( !nameIndex )
					break;

				// get property name
				RED_ASSERT( nameIndex  < resolvedNames.Size(), TXT("Property name index out of range (%d/%d)"), nameIndex  , resolvedNames.Size() );
				const AnsiChar* propertyName = resolvedNames[ nameIndex ];

				// get type index
				Uint16 typeIndex = 0;
				*file << typeIndex;
				RED_ASSERT( (typeIndex > 0) && (typeIndex < resolvedNames.Size()), TXT("Property %hs type index out of range (%d/%d)"), propertyName, nameIndex, resolvedNames.Size() );
				const AnsiChar* typeName = resolvedNames[ typeIndex ];

				// read skip offset (that will determine property data size)
				Uint32 dataSize = 0;
				* file << dataSize;
				RED_ASSERT( dataSize >= 4, TXT("Invalid skip offset: %d"), dataSize );
				dataSize -= sizeof(Uint32);

				// is this a struct ?
				const IRTTIType* resolvedType = SRTTI::GetInstance().FindType( CName( ANSI_TO_UNICODE( typeName ) ) ) ;
				if ( !resolvedType )
				{
					ERR_WCC( TXT("PatchCRC: Unresolved type for prop: '%hs', type '%hs', size %d"), propertyName, typeName, dataSize );
					file->Seek( file->GetOffset() + dataSize );
					continue;
				}

				// is this property skipped ?
				if ( IsIgnored( propertyName, ignoredProperties, numIgnoredProperties ) )
				{
					//WARN_WCC( TXT("PatchCRC: Skipping property '%hs', type '%hs', size %d"), propertyName, typeName, dataSize );
					file->Seek( file->GetOffset() + dataSize );
				}
				// structures :)
				else if ( resolvedType->GetType() == RT_Class )
				{
					const Uint64 propertyStartOffset = file->GetOffset();

					// recurse
					const Uint64 expectedEndOffsetForStruct = propertyStartOffset + dataSize;
					crc = CalcPropertyBasedCRC( file, expectedEndOffsetForStruct, resolvedNames, ignoredProperties, numIgnoredProperties, crc );

					// make sure everything was parsed
					RED_ASSERT( file->GetOffset() == (propertyStartOffset + dataSize), TXT("Structure parsing problem for '%hs', type '%hs', size %d"), propertyName, typeName, dataSize );
					file->Seek( propertyStartOffset + dataSize );
				}
				// generic property
				else
				{
					// calculate the CRC from the property name and type INDEX
					crc = Red::CalculateHash64( &nameIndex, sizeof(nameIndex), crc );

					// calculate CRC from the data
					crc = CalcFileBlockCRC( file, dataSize, crc );
				}
			}
		}

		// calculate CRC of the rest of the file (whatever was not parsed as "property")
		if ( file->GetOffset() < expectedEndOffset )
		{
			const Uint64 blobDataSize = expectedEndOffset - file->GetOffset();
			crc = CalcFileBlockCRC( file, blobDataSize, crc );
		}

		// return final crc
		return crc;
	}

	/// Calculate CRC of object export
	Uint64 CalcExportCRC( IFile* file, const CDependencyFileData::Export& obj, const TDynArray< const AnsiChar* >& resolvedNames, Uint64 crc )
	{
		// Get object class name
		const AnsiChar* objectClass = resolvedNames[ obj.m_className ];

		// We need a special hack for meshes :(
		if ( 0 == Red::StringCompare( objectClass, "CMesh" ) )
		{
			const AnsiChar* ignoreList[] = { "collisionInitPositionOffset" };

			const Uint64 baseOffset = file->GetOffset();
			const Uint64 endOffset = baseOffset + obj.m_dataSize;

			return CalcPropertyBasedCRC( file, endOffset, resolvedNames, ignoreList, ARRAY_COUNT(ignoreList), crc );
		}
		else
		{
			// just calculate the CRC the old way
			return CalcFileBlockCRC( file, obj.m_dataSize, crc );
		}
	}

	/// Calculate CRC of resource file
	Uint64 CalcDependencyFileCRC( IFile* file, Uint64 crc )
	{
		const Uint64 baseOffset = file->GetOffset();

		// read header
		CDependencyFileData::Header header;
		Red::MemoryZero( &header, sizeof(header) );
		file->Serialize( &header, sizeof(header) );

		// invalid file
		if ( header.m_magic != CDependencyFileData::FILE_MAGIC )
		{
			ERR_WCC( TXT("Invalid file magic in file '%ls', offset %d"),
				file->GetFileNameForDebug(), file->GetOffset() );
			return crc;
		}

		// Chunk data size
		Uint32 chunkSize[ CDependencyFileData::eChunkType_MAX ] = {
			sizeof(AnsiChar), // eChunkType_Strings
			sizeof(CDependencyFileData::Name), // eChunkType_Names
			sizeof(CDependencyFileData::Import), // eChunkType_Imports
			sizeof(CDependencyFileData::Property), // eChunkType_Properties,
			sizeof(CDependencyFileData::Export), // eChunkType_Exports,
			sizeof(CDependencyFileData::Buffer), // eChunkType_Buffers,
			sizeof(CDependencyFileData::InplaceData), // eChunkType_InplaceData
		};

		// CRC from chunks
		for ( Uint32 i=0; i<header.m_numChunks; ++i )
		{
			// handled separately
			if ( i == CDependencyFileData::eChunkType_Strings )
				continue;
			if ( i == CDependencyFileData::eChunkType_Exports )
				continue;
			if ( i == CDependencyFileData::eChunkType_Names )
				continue;

			file->Seek( baseOffset + header.m_chunks[i].m_offset );

			const Uint32 size = chunkSize[i] * header.m_chunks[i].m_count;
			crc = CalcFileBlockCRC( file, size, crc );
		}

		// Load string
		TDynArray< AnsiChar > stringData;
		stringData.Resize( header.m_chunks[ CDependencyFileData::eChunkType_Strings ].m_count );
		file->Seek( baseOffset + header.m_chunks[ CDependencyFileData::eChunkType_Strings ].m_offset );
		file->Serialize( stringData.Data(), stringData.DataSize() );

		// Resolved names
		TDynArray< const AnsiChar* > resolvedNames;

		// CRC of the names
		THashSet< const AnsiChar* > processedStrings;
		{
			// load the names
			TDynArray< CDependencyFileData::Name > names;
			names.Resize( header.m_chunks[ CDependencyFileData::eChunkType_Names ].m_count );
			file->Seek( baseOffset + header.m_chunks[ CDependencyFileData::eChunkType_Names ].m_offset );
			file->Serialize( names.Data(), names.DataSize() );

			// prepare output names
			resolvedNames.Resize( header.m_chunks[ CDependencyFileData::eChunkType_Names ].m_count );

			// process the names
			Uint32 nameIndex = 0;
			for ( const auto& it : names )
			{
				const AnsiChar* txt = &stringData[ it.m_string ];
				CName name( ANSI_TO_UNICODE(txt) );

				// remember resolved name
				resolvedNames[ nameIndex ] = txt;
				nameIndex += 1;

				// so we will ignore it
				processedStrings.Insert( txt );

				// translate type id's into strings if needed
				name = GetSafeTypeName( name );

				const Uint32 length = static_cast< const Uint32 >( Red::StringLength( name.AsChar( ) ) );
				crc = Red::CalculateHash64( name.AsChar( ), length, crc );
			}
		}

		// CRC of the string data
		{
			const AnsiChar* cur = static_cast< const AnsiChar* >( stringData.Data() );
			const AnsiChar* end = static_cast< const AnsiChar* >( stringData.End() );
			while ( cur < end )
			{
				const Uint32 length = static_cast< const Uint32 >( Red::StringLength( cur ) );

				// crc from the normal string (path)
				if ( !processedStrings.Exist( cur ) )
				{
					crc = Red::CalculateHash64( cur, length, crc );
				}

				// go to the next string
				cur += (length+1);
			}
		}

		// CRC from export data
		{
			// load exports
			const auto& chunk = header.m_chunks[ CDependencyFileData::eChunkType_Exports ];

			TDynArray< CDependencyFileData::Export > exports;
			exports.Resize( chunk.m_count );

			file->Seek( baseOffset + chunk.m_offset );
			file->Serialize( exports.Data(), exports.DataSize() );

			// process exports and calculate merged cRC
			for ( const auto& exportInfo : exports )
			{
				file->Seek( exportInfo.m_dataOffset + baseOffset );
				crc = CalcExportCRC( file, exportInfo, resolvedNames, crc );
			}
		}

		// CRC from inplace files
		{
			// load inplace files
			const auto& chunk = header.m_chunks[ CDependencyFileData::eChunkType_InplaceData ];

			TDynArray< CDependencyFileData::InplaceData > inplaceData;
			inplaceData.Resize( chunk.m_count );

			file->Seek( baseOffset + chunk.m_offset );
			file->Serialize( inplaceData.Data(), inplaceData.DataSize() );

			// process inplace files
			for ( const auto& inplaceInfo : inplaceData )
			{
				file->Seek( inplaceInfo.m_dataOffset + baseOffset );
				crc = CalcDependencyFileCRC( file, crc );
			}
		}

		// return final CRC
		return crc;
	}

	CName GetSafeTypeName( CName name )
	{
		IRTTIType* type = SRTTI::GetInstance( ).FindType( name );
		if( type )
		{
			// if it's an array, we translate it's pool name and ignore it's class name
			if( type->IsArrayType() )
			{
				CRTTIArrayType* baseArrayType = static_cast< CRTTIArrayType* >( type );
				const Char* memPoolName = ANSI_TO_UNICODE( Memory::GetPoolName( baseArrayType->GetMemoryPool( ) ) );
				const Char* innerTypeName = GetSafeTypeName( baseArrayType->ArrayGetInnerType( )->GetName( ) ).AsChar( );
				name = CName( String::Printf( TXT( "array:%s,%s" ), memPoolName, innerTypeName ) );
			}
		}

		return name;
	}

	Bool IsContentOrDlcPath( const String& contentPath ) 
	{
		bool isContentOrDlcPath = true;

		// make sure we are NOT in a patch or mod directory
		const CFilePath fullPath( contentPath );
		const auto dirs = fullPath.GetDirectories();
		for ( const auto& dir : dirs )
		{
			if ( dir.BeginsWith( TXT("patch") ) )
			{
				isContentOrDlcPath = false;
				break;
			}
			else if ( dir.BeginsWith( TXT("mod") ) )
			{
				isContentOrDlcPath = false;
				break;
			}
			else if ( dir.BeginsWith( TXT("dlc") ) )
			{
				isContentOrDlcPath = true;
				break;
			}
		}

		return isContentOrDlcPath;
	}

} // PatchUtils
