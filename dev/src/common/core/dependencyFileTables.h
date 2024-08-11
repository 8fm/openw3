/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "datetime.h"

/// File tables for current version only - ment for INPLACE LOADING ONLY
class CDependencyFileData
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Engine );

public:
	static const Uint32 FILE_MAGIC;
	static const Uint32 FILE_VERSION;

	// flags for the imports
	enum EImportFlags : Uint16
	{
		eImportFlags_Obligatory = FLAG( 0 ),			//!< This is a obligatory import - file will fail to load if this import fails
		eImportFlags_Template	= FLAG( 1 ),			//!< This resource is used as a template resource
		eImportFlags_Soft		= FLAG( 2 ),			//!< This is a soft import - do not load it directly (NOT USED YET)
		eImportFlags_HashedPath	= FLAG( 3 ),			//!< The path value is not an entry in the string table but the hash itself
		eImportFlags_Inplace    = FLAG( 4 ),			//!< This is the inplace import, that data is glued into the file itself
	};

	// chunk types
	enum EChunkType : Uint8
	{
		eChunkType_Strings = 0,
		eChunkType_Names = 1,
		eChunkType_Imports = 2,
		eChunkType_Properties = 3,
		eChunkType_Exports = 4,
		eChunkType_Buffers = 5,
		eChunkType_InplaceData = 6,

		eChunkType_MAX = 10, // reserved for the future, total chunks sizeof(Chunk)*10 = 120 bytes
	};

	// timestamp type
	typedef Red::System::DateTime TimeStamp;
	typedef Uint32				CRCValue;

	#pragma pack(push)
	#pragma pack(4)

	// file chunk
	struct Chunk
	{
		Uint32		m_offset;				// Offset to table
		Uint32		m_count;				// Number of entries in table
		CRCValue	m_crc;					// CRC of the data table (after loading)
	};

	// file header
	struct Header
	{
		Uint32		m_magic;					// File header
		Uint32		m_version;					// Version number
		Uint32		m_flags;					// Generic flags

		TimeStamp	m_timeStamp;				// When was this file saved
		Uint32		m_buildVersion;				// Version number of the tool used to save this file (CL number)
		Uint32		m_objectsEnd;				// Where the object data ends (deserializable data)
		Uint32		m_buffersEnd;				// Where the buffer data ends (total file size)

		CRCValue	m_crc;						// CRC of the header

		Uint32		m_numChunks;				// Number of valid chunks in file
		Chunk		m_chunks[ eChunkType_MAX ]; // Chunks
	};

	// name data
	struct Name
	{
		Uint32		m_string;		// Index in string table (dynamic)
		Uint32		m_hash;			// Name hash (static)
	};

	// import data
	struct Import
	{
		Uint32		m_path;			// Import path (index in string table or HASH if the bit it set)
		Uint16		m_className;	// Index to name table
		Uint16		m_flags;		// Import flags
	};

	// export data
	struct Export
	{
		Uint16		m_className;			// Export class (index to name table)
		Uint16		m_objectFlags;			// Object flags
		Uint32		m_parent;				// Index of the parent object
		Uint32		m_dataSize;				// Object data size
		Uint32		m_dataOffset;			// Object data offset
		Int32		m_template;				// Template owner object
		CRCValue	m_crc;					// CRC of the object data
	};

	// property info
	struct Property
	{
		Uint16		m_className;			// Parent class (index to name table)
		Uint16		m_typeName;				// Property type (index to name table)
		Uint16		m_propertyName;			// Property name (index to name table)
		Uint16		m_flags;				// Custom flags (0)
		Uint64		m_hash;					// Property hash to speedup lookup on loading
	};

	// buffer info
	struct Buffer
	{
		Uint16		m_name;					// Name of the buffer (index to name table) - placeholder for the future
		Uint16		m_flags;				// Buffer flags  - placeholder for the future
		Uint32		m_index;				// Internal buffer index
		Uint32		m_dataOffset;			// Offset to the buffer data (if resident)
		Uint32		m_dataSizeOnDisk;		// Size of the buffer data on disk (if we ever add native compression)
		Uint32		m_dataSizeInMemory;		// Size of the buffer data in memory
		CRCValue	m_crc;					// Calculated CRC of the buffer's data
	};

	// inplace file information
	struct InplaceData
	{
		Uint32		m_importIndex;			// Index of the import object that this data is for - NOTE, may be zero
		Uint32		m_path;					// File path name (in the strings table)
		Uint64		m_pathHash;				// File path hash
		Uint32		m_dataOffset;			// Offset to the data
		Uint32		m_dataSize;				// Size of the data
	};

	#pragma pack(pop)

	static_assert( sizeof(Name) == 8, "Structure is read directly into memory. Size cannot be changed without an adapter." );
	static_assert( sizeof(Import) == 8, "Structure is read directly into memory. Size cannot be changed without an adapter." );
	static_assert( sizeof(Export) == 24, "Structure is read directly into memory. Size cannot be changed without an adapter." );
	static_assert( sizeof(Property) == 16, "Structure is read directly into memory. Size cannot be changed without an adapter." );
	static_assert( sizeof(Buffer) == 24, "Structure is read directly into memory. Size cannot be changed without an adapter." );
	static_assert( sizeof(InplaceData) == 24, "Structure is read directly into memory. Size cannot be changed without an adapter." );

	typedef TDynArray< AnsiChar, MC_Linker >	TStringTable;
	typedef TDynArray< Name, MC_Linker >		TNameTable;
	typedef TDynArray< Import, MC_Linker >		TImportTable;
	typedef TDynArray< Export, MC_Linker >		TExportTable;
	typedef TDynArray< Property, MC_Linker >	TPropertyTable;
	typedef TDynArray< Buffer, MC_Linker >		TBufferTable;
	typedef TDynArray< InplaceData, MC_Linker >	TInplaceDataTable;

	TStringTable			m_strings;
	TNameTable				m_names;
	TImportTable			m_imports;
	TExportTable			m_exports;
	TPropertyTable			m_properties;
	TBufferTable			m_buffers;
	TInplaceDataTable		m_inplace;

	Uint32					m_softImportBase; // legacy only, first soft import index (for merged soft imports)

public:
	CDependencyFileData();

	// Store data to file
	void Save( IFile& file, const Uint64 headerOffset, const Bool zeroNonDeterministicData = false ) const;

	// Load data from file, will return false if validation fails
	Bool Load( IFile& file, Uint32& outVersion );

private:
	// Calculate header CRC
	static CRCValue CalcHeaderCRC( const Header& header );
};