/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../redSystem/crc.h"
#include "../core/version.h"
#include "dependencyFileTables.h"

// adapters
#include "dependencyFileTables_AdapterLegacy.h"

const Uint32 CDependencyFileData::FILE_MAGIC = 'W2RC';
const Uint32 CDependencyFileData::FILE_VERSION = VER_CURRENT;

#ifndef RED_FINAL_BUILD
	#define DEBUG_MEM_ZERO( x ) Red::MemoryZero( &(x), sizeof(x) )
#else
	#define DEBUG_MEM_ZERO( x ) 
#endif

namespace Helpers
{
	// helper for loading data table with validation
	template< typename T >
	static Bool LoadChunkData( IFile& file, TDynArray< T, MC_Linker >& outData, const CDependencyFileData::Chunk& chunk, const Uint64 baseOffset )
	{
		if ( chunk.m_count )
		{
			outData.Resize( chunk.m_count );

			file.Seek( baseOffset + chunk.m_offset );
			file.Serialize( outData.Data(), outData.DataSize() );

#ifndef RED_FINAL_BUILD
			// compute buffer CRC
			Red::System::CRC32 crc;
			const Uint32 crcValue = crc.Calculate( outData.Data(), (const Uint32) outData.DataSize() );

			// validate data by computing CRC of the loaded buffer
			if ( chunk.m_crc != crcValue )
			{
				ERR_CORE( TXT("!!! FILE CORRUPTION !!!") );
				ERR_CORE( TXT("Chunk CRC mismatch in '%ls'"), file.GetFileNameForDebug() );
				return false;
			}
#endif
		}

		// valid
		return true;
	}

	// helper for saving data table
	template< typename T >
	static void SaveChunkData( IFile& file, const TDynArray< T, MC_Linker >& data, CDependencyFileData::Chunk& outChunk, const Uint64 baseOffset )
	{
		if ( data.Empty() )
		{
			outChunk.m_count = 0;
			outChunk.m_crc = 0;
			outChunk.m_offset = 0;
		}
		else
		{
			// setup chunk
			outChunk.m_offset = (const Uint32)( file.GetOffset() - baseOffset );
			outChunk.m_count = data.Size();

			// compute CRC of the saved data
			Red::System::CRC32 crc;
			outChunk.m_crc = crc.Calculate( data.Data(), (const Uint32) data.DataSize() );		

			// save data
			file.Serialize( (void*)data.Data(), data.DataSize() );
		}
	}
}

CDependencyFileData::CDependencyFileData()
	: m_softImportBase( 0 )
{
}

void CDependencyFileData::Save( IFile& file, const Uint64 headerOffset, const Bool zeroNonDeterministicData /*= false*/ ) const
{
	PC_SCOPE( SaveFileData );

	// write placeholder header
	Header header;
	Red::MemoryZero( &header, sizeof(header) );

	// save the initial header
	{
		DEBUG_MEM_ZERO( header );
		file.Seek( headerOffset );
		file.Serialize( &header, sizeof(header) );
	}

	// setup header identification data
	header.m_version = file.GetVersion();
	header.m_magic = FILE_MAGIC;

	// save file identification data - only when the file is saved to disk
	// never store this data when saved to memory (as a sub-part of another file)
	if ( file.IsFileBased() && !zeroNonDeterministicData )
	{
		// get time stamp
		Red::System::Clock::GetInstance().GetUTCTime( header.m_timeStamp );
		RED_FATAL_ASSERT( header.m_version >= VER_UPDATED_RESOURCE_FORMAT, "Trying to save new data using old file version (%d)", header.m_version );

		// get the build version
		header.m_buildVersion = atoi( APP_LAST_P4_CHANGE );
	}

	// save data tables
	header.m_numChunks = 6; // keep up to date
	Helpers::SaveChunkData( file, m_strings, header.m_chunks[ eChunkType_Strings ], headerOffset );
	Helpers::SaveChunkData( file, m_names, header.m_chunks[ eChunkType_Names ], headerOffset );
	Helpers::SaveChunkData( file, m_imports, header.m_chunks[ eChunkType_Imports ], headerOffset );
	Helpers::SaveChunkData( file, m_properties, header.m_chunks[ eChunkType_Properties ], headerOffset );
	Helpers::SaveChunkData( file, m_exports, header.m_chunks[ eChunkType_Exports ], headerOffset );
	Helpers::SaveChunkData( file, m_buffers, header.m_chunks[ eChunkType_Buffers ], headerOffset );
	Helpers::SaveChunkData( file, m_inplace, header.m_chunks[ eChunkType_InplaceData ], headerOffset );	

	// compute the size of the object data (preloaded)
	header.m_objectsEnd = (Uint32)( file.GetOffset() - headerOffset );
	for ( const Export& e : m_exports )
	{
		const Uint32 endOffset = e.m_dataOffset + e.m_dataSize;
		header.m_objectsEnd = Red::Math::NumericalUtils::Max< Uint32 >( header.m_objectsEnd, endOffset );
	}

	// compute the end position of the buffered data (preloaded)
	header.m_buffersEnd = header.m_objectsEnd;
	for ( const Buffer& b : m_buffers )
	{
		// only locally stored data
		if ( b.m_dataOffset )
		{
			const Uint32 endOffset = b.m_dataOffset + b.m_dataSizeOnDisk;
			header.m_buffersEnd = Red::Math::NumericalUtils::Max< Uint32 >( header.m_buffersEnd, endOffset );
		}
	}

	// compute header CRC
	header.m_crc = CalcHeaderCRC( header );

	// save it again, preserve file offset at the end of the data
	const Uint64 currentOffset = file.GetOffset();
	file.Seek( headerOffset );
	file.Serialize( &header, sizeof(header) );
	file.Seek( currentOffset );
}

Bool CDependencyFileData::Load( IFile& file, Uint32& outVersion )
{
	PC_SCOPE( LoadFileData );

	// size of the header preamble
	const Uint32 preamble = sizeof(Uint32) * 3;

	// load header preamble
	Header header;
	DEBUG_MEM_ZERO( header );
	const Uint64 baseOffset = file.GetOffset();
	file.Serialize( &header, preamble );

	// validate header and magic
	if ( header.m_magic != FILE_MAGIC )
	{
		WARN_CORE( TXT("File magic is invalid (%08X != %08X), file is not a resource."), header.m_magic, FILE_MAGIC );
		return false;
	}

	// validate header version - should not be older than the CRC version
	if ( header.m_version > VER_CURRENT )
	{
		WARN_CORE( TXT("File version is invalid (%d > %d), file is from newer version of the engine."), header.m_version, VER_CURRENT );
		return false;
	}

	// legacy adapter
	if ( header.m_version < VER_UPDATED_RESOURCE_FORMAT )
	{
		CDependencyFileDataAdapter_Legacy adapter;
		file.Seek( baseOffset ); // rewind file
		if ( !adapter.Load( file, *this ) )
			return false;

		// use loaded version
		outVersion = header.m_version;
		return true;
	}

	// read rest of the header
	file.Serialize( (Uint8*)&header + preamble, sizeof(header)-preamble );

#ifndef RED_FINAL_BUILD
	// calculate current header CRC
	CRCValue crc = CalcHeaderCRC( header );
	if ( crc != header.m_crc )
	{
		ERR_CORE( TXT("!!! FILE CORRUPTION !!!") );
		ERR_CORE( TXT("Header CRC mismatch in '%ls'"), file.GetFileNameForDebug() );
		return false;
	}
#endif

	// create the tables using the chunk information and load chunk data
	if ( !Helpers::LoadChunkData( file, m_strings, header.m_chunks[ eChunkType_Strings ], baseOffset ) ) return false;
	if ( !Helpers::LoadChunkData( file, m_names, header.m_chunks[ eChunkType_Names ], baseOffset ) ) return false;
	if ( !Helpers::LoadChunkData( file, m_imports, header.m_chunks[ eChunkType_Imports ], baseOffset ) ) return false;
	if ( !Helpers::LoadChunkData( file, m_properties, header.m_chunks[ eChunkType_Properties ], baseOffset ) ) return false;
	if ( !Helpers::LoadChunkData( file, m_exports, header.m_chunks[ eChunkType_Exports ], baseOffset ) ) return false;
	if ( !Helpers::LoadChunkData( file, m_buffers, header.m_chunks[ eChunkType_Buffers ], baseOffset ) ) return false;
	if ( !Helpers::LoadChunkData( file, m_inplace, header.m_chunks[ eChunkType_InplaceData ], baseOffset ) ) return false;

	// check for data overrides
#ifndef RED_FINAL_BUILD
	{
		const Uint64 minOffset = (file.GetOffset() - baseOffset);
		Uint64 curOffset = minOffset;

		// check exports
		for ( Uint32 i=0; i<m_exports.Size(); ++i )
		{
			if ( m_exports[i].m_dataOffset != curOffset )
			{
				ERR_CORE( TXT("!!! FILE CORRUPTION !!!") );
				ERR_CORE( TXT("Export %d at offset %d aliased with previous data (overlap: %d) in '%ls'"), 
					i, m_exports[i].m_dataOffset, curOffset - m_exports[i].m_dataOffset, file.GetFileNameForDebug() );

				return false;
			}

			curOffset += m_exports[i].m_dataSize;
		}

		// validate header inter-offset
		if ( curOffset != header.m_objectsEnd )
		{
			ERR_CORE( TXT("!!! FILE CORRUPTION !!!") );
			ERR_CORE( TXT("Size of the object data: %d, expected %d in '%ls'"), 
				curOffset, header.m_objectsEnd, file.GetFileNameForDebug() );

			return false;
		}

		// check buffers
		for ( Uint32 i=0; i<m_buffers.Size(); ++i )
		{
			if ( !m_buffers[i].m_dataOffset )
				continue;

			if ( m_buffers[i].m_dataOffset != curOffset )
			{
				ERR_CORE( TXT("!!! FILE CORRUPTION !!!") );
				ERR_CORE( TXT("Buffer %d at offset %d aliased with previous data (overlap: %d) in '%ls'"), 
					i, m_buffers[i].m_dataOffset, curOffset - m_buffers[i].m_dataOffset, file.GetFileNameForDebug() );

				return false;
			}

			curOffset += m_buffers[i].m_dataSizeOnDisk;
		}

		// validate header final offset
		/*if ( curOffset != header.m_buffersEnd )
		{
			ERR_CORE( TXT("!!! FILE CORRUPTION !!!") );
			ERR_CORE( TXT("Size of the buffer data: %d, expected %d in '%ls'"), 
				curOffset, header.m_buffersEnd, file.GetFileNameForDebug() );

			return false;
		}*/
	}
#endif

	// done
	outVersion = header.m_version;
	return true;
}

CDependencyFileData::CRCValue CDependencyFileData::CalcHeaderCRC( const Header& header )
{
	// tricky bit - the CRC field cannot be included in the CRC calculation - caluculate the CRC without it
	Header tempHeader;
	tempHeader = header;
	tempHeader.m_crc = 0xDEADBEEF; // special hacky stuff

	Red::System::CRC32 crc;
	return crc.Calculate( &tempHeader, sizeof(tempHeader) );
}

