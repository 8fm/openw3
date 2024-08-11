/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "resourcePath.h"
#include "names.h"
#include "pointer.h"
#include "handleMap.h"
#include "object.h"
#include "datetime.h"
#include "depot.h"
#include "filePath.h"

/// LEGACY file format adapter
/// This structure loads legacy file tables and convert them into the new file tables
class CDependencyFileDataAdapter_Legacy
{
public:
	// Load the legacy format
	Bool Load( IFile& file, CDependencyFileData& data )
	{
		PC_SCOPE( LegacyHeaderLoad );

		// file is to small
		const Uint64 baseOffset = file.GetOffset();
		if ( (file.GetSize() - baseOffset) < sizeof(LegacyHeader) )
		{
			WARN_CORE( TXT("FileLegacy: File is to small to be dependnect file (size: %d). Failed to load '%ls'"), 
				file.GetSize() - baseOffset, file.GetFileNameForDebug() );
			return false;
		}

		// load legacy header
		LegacyHeader header;
		file << header;

		// validate legacy header
		if ( header.m_magic != LEGACY_FILE_MAGIC )
		{
			WARN_CORE( TXT("FileLegacy: Not valid package (0%08X != 0x%08X). Failed to load '%ls'"), 
				header.m_magic, LEGACY_FILE_MAGIC, file.GetFileNameForDebug() );
			return false;
		}

		// validate legacy version
		if ( header.m_version > VER_UPDATED_RESOURCE_FORMAT )
		{
			WARN_CORE( TXT("FileLegacy: Version %i is not supported. Highest supported version is %i. Failed to load '%ls'."), 
				header.m_version, VER_CURRENT, file.GetFileNameForDebug() );
			return false;
		}
		else if ( header.m_version < VER_MINIMAL )
		{
			WARN_CORE( TXT("FileLegacy: Version %i is no longer supported. Minimal supported version is %i. Failed to load '%ls'."), 
				header.m_version, VER_MINIMAL, file.GetFileNameForDebug() );
			return false;
		}

		// Prepare legacy data tables
		TDynArray< LegacyName > legacyNames;
		TDynArray< LegacyImport > legacyImports;
		TDynArray< LegacyExport > legacyExports;
		TDynArray< LegacySoftImport > legacySoftImports;

		// Resize data
		legacyNames.Resize( header.m_namesCount+1 );
		legacyImports.Resize( header.m_importsCount );
		legacyExports.Resize( header.m_exportsCount );
		legacySoftImports.Resize( header.m_softDependenciesCount ? header.m_softDependenciesCount-1 : 0 );

		// Load names
		{
			file.Seek( baseOffset + header.m_namesOffset );
			legacyNames[ 0 ].m_name = CName::NONE;
			for ( Uint32 i=0; i<header.m_namesCount; i++ )
			{
				LegacyName& name = legacyNames[i+1];
				if ( !name.Load( file, header.m_version ) )
				{
					ERR_CORE( TXT("FileLegacy: Failed to load name %d of %d from %ls"), i, header.m_namesCount, file.GetFileNameForDebug() );
					return false;
				}
			}
		}

		// Load imports
		{
			file.Seek( baseOffset + header.m_importsOffset );
			for ( Uint32 i=0; i<header.m_importsCount; i++ )
			{
				LegacyImport& import = legacyImports[i];;
				if ( !import.Load( file, legacyNames ) )
				{
					ERR_CORE( TXT("FileLegacy: Failed to load import %d of %d from %ls"), i, header.m_importsCount, file.GetFileNameForDebug() );
					return false;
				}
			}
		}

		// Load soft dependencies
		{
			file.Seek( baseOffset + header.m_softDependenciesOffset );
			for ( Uint32 i=1; i<header.m_softDependenciesCount; i++ )
			{
				LegacySoftImport& softImport = legacySoftImports[i-1];
				if ( !softImport.Load( file ) )
				{
					ERR_CORE( TXT("FileLegacy: Failed to load soft import %d of %d from %ls"), i, header.m_softDependenciesCount, file.GetFileNameForDebug() );
					return false;
				}
			}
		}

		// Load exports
		{
			file.Seek( baseOffset + header.m_exportsOffset );
			for ( Uint32 i=0; i<header.m_exportsCount; i++ )
			{
				LegacyExport& ex = legacyExports[i];
				if ( !ex.Load( file, header.m_version, legacyNames ) )
				{
					ERR_CORE( TXT("FileLegacy: Failed to load legacy export %d of %d from %ls"), i, header.m_exportsCount, file.GetFileNameForDebug() );
					return false;
				}
			}
		}

		// Initialize string builder
		TStringMap stringMap;
		data.m_strings.Reserve( 8192 );
		data.m_strings.PushBack( 0 ); // empty string

		// Convert names
		data.m_names.Resize( legacyNames.Size()+1 );
		data.m_names[0].m_hash = 0;
		data.m_names[0].m_string = 0;
		for ( Uint32 i=1; i<legacyNames.Size(); ++i )
		{
			const CName legacyName = legacyNames[i].m_name;
			if ( !legacyName )
			{
				ERR_CORE( TXT("FileLegacy: Unexpected empty name at index %d in file %ls"), i, file.GetFileNameForDebug() );
				return false;
			}

			data.m_names[i].m_string = AddString( data, stringMap, UNICODE_TO_ANSI( legacyName.AsChar() ) );
			data.m_names[i].m_hash = legacyName.GetSerializationHash().GetValue();
		}

		// Convert imports
		data.m_imports.Resize( legacyImports.Size() + legacySoftImports.Size() );
		for ( Uint32 i=0; i<legacyImports.Size(); ++i )
		{
			const LegacyImport& legacyImport = legacyImports[i];
			CDependencyFileData::Import& newImport = data.m_imports[i];
			if ( !legacyImport.m_path )
			{
				WARN_CORE( TXT("FileLegacy: Unexpected empty import at index %d in file %ls"), i, file.GetFileNameForDebug() );
			}

			newImport.m_className = legacyImport.m_classIndex;
			newImport.m_flags = 0;
			newImport.m_path = AddString( data, stringMap, legacyImport.m_path.ToAnsiString() );
		}

		// Convert soft imports
		data.m_softImportBase = legacyImports.Size();
		for ( Uint32 i=0; i<legacySoftImports.Size(); ++i )
		{
			const LegacySoftImport& legacyImport = legacySoftImports[i];
			CDependencyFileData::Import& newImport = data.m_imports[data.m_softImportBase + i];
			if ( legacyImport.m_path.Empty() )
			{
				ERR_CORE( TXT("FileLegacy: Unexpected empty soft import at index %d in file %ls"), i, file.GetFileNameForDebug() );
				return false;
			}

			newImport.m_className = 0; // unknown
			newImport.m_flags = CDependencyFileData::eImportFlags_Soft;
			newImport.m_path = AddString( data, stringMap, UNICODE_TO_ANSI( legacyImport.m_path.AsChar() ) );
		}

		// Convert exports
		data.m_exports.Resize( legacyExports.Size() );
		for ( Uint32 i=0; i<legacyExports.Size(); ++i )
		{
			const LegacyExport& legacyExport = legacyExports[i];
			CDependencyFileData::Export& newExport = data.m_exports[i];
			if ( !legacyExport.m_dataSize )
			{
				ERR_CORE( TXT("FileLegacy: Unexpected empty export at index %d in file %ls"), i, file.GetFileNameForDebug() );
				return false;
			}

			newExport.m_className = legacyExport.m_classIndex;
			newExport.m_crc = 0; // unknown
			newExport.m_dataOffset = legacyExport.m_dataOffset;
			newExport.m_dataSize = legacyExport.m_dataSize;
			newExport.m_parent = legacyExport.m_parent;
			newExport.m_template = legacyExport.m_template;
			newExport.m_objectFlags = legacyExport.m_flags;;
		}

		// Done
		return true;
	}

private:
	static const Uint32 LEGACY_FILE_MAGIC = 'W2RC';

	// string map
	typedef THashMap< StringAnsi, Uint32 > TStringMap;

	// legacy header data
	struct LegacyHeader
	{
		Uint32		m_magic;					// File header
		Uint32		m_version;					// Version number
		Uint32		m_flags;					// Generic flags
		Uint32		m_namesOffset;				// Offset to name table
		Uint32		m_namesCount;				// Number of names
		Uint32		m_exportsOffset;			// Offset to export table
		Uint32		m_exportsCount;				// Number of exports
		Uint32		m_importsOffset;			// Offset to import table
		Uint32		m_importsCount;				// Number of imports
		Uint32		m_softDependenciesOffset;	// Offset to soft dependencies table
		Uint32		m_softDependenciesCount;	// Number of soft dependencies
		Uint32		m_bufferOffset;				// Offset to first buffer (0 if there are no buffers)
		Uint32		m_bufferTableOffset;		// Offset to the buffer table
		Uint32		m_bufferTableEntryCount;	// Number of buffer entries in the table

		RED_FORCE_INLINE friend IFile &operator<<( IFile &file, LegacyHeader &header )
		{
			file << header.m_magic << header.m_version << header.m_flags;
			file << header.m_namesOffset << header.m_namesCount;
			file << header.m_exportsOffset << header.m_exportsCount;
			file << header.m_importsOffset << header.m_importsCount;
			file << header.m_softDependenciesOffset << header.m_softDependenciesCount;

			if( file.GetVersion() >= VER_DEPENDENCY_LINKER_HEADER_BUFFER_ENTRY )
			{
				file << header.m_bufferOffset;
				file << header.m_bufferTableOffset;
				file << header.m_bufferTableEntryCount;
			}

			return file;
		}
	};

	// legacy name data
	class LegacyName
	{
	public:
		CName				m_name;

	public:
		RED_FORCE_INLINE LegacyName()
			: m_name( CName::NONE )
		{}

		RED_FORCE_INLINE Bool Load( IFile& file, const Uint32 version )
		{
			if ( version >= VER_HASH_CNAMES )
			{
				// Just load a name directly
				file.IFile::operator<<( m_name );
			}
			else
			{
				// Load the string
				StringBuffer< 512 > nameBuf;
				file << nameBuf;
				m_name = CName( nameBuf.AsChar() );
			}

			return true;
		}
	};

	// legacy import data
	class LegacyImport
	{
	public:
		CResourcePath			m_path;				// Resource name
		CName					m_class;			// Resource class name
		Uint16					m_classIndex;		// Name index
		Uint16					m_flags;			// Import flags

	public:
		RED_FORCE_INLINE LegacyImport()
			: m_flags( 0 )
		{};

		RED_FORCE_INLINE Bool Load( IFile& file, const TDynArray< LegacyName >& names )
		{
			file << m_path;

			if ( !LoadName( file, m_class, m_classIndex, names ) )
				return false;

			file << m_flags;

			// "repair" the import path of the resource
			AnsiChar* path = const_cast< AnsiChar* >( m_path.ToAnsiString() );
			while ( *path )
			{
				if ( *path >= 'A' && *path <= 'Z' )
				{
					*path = 'a' + (*path - 'A');
				}
				else if ( *path == '/' )
				{
					*path = '\\';
				}
				++path;
			}

			return true;
		}   
	};

	// legacy export data
	class LegacyExport
	{
	public:
		CName			m_class;				// Name of the object class
		Uint16			m_classIndex;			// Class name index
		Uint32			m_parent;				// Index of the parent object
		Uint32			m_dataSize;				// Object data size
		Uint32			m_dataOffset;			// Object data offset
		Int32			m_template;				// Template owner object
		Uint16			m_flags;				// Object flags

	public:
		RED_FORCE_INLINE LegacyExport()
			: m_parent( 0 )
			, m_dataSize( 0 )
			, m_dataOffset( 0 )
			, m_template( 0 )
			, m_flags( 0 )
		{};

		RED_INLINE Bool Load( IFile &file, const Uint32 version, const TDynArray< LegacyName >& names )
		{
			if ( !LoadName( file, m_class, m_classIndex, names ) )
				return false;

			file << m_parent;
			file << m_dataSize;
			file << m_dataOffset;

			if ( version < VER_16BIT_OBJECT_FLAGS )
			{
				Uint32 flags;
				file << flags;
				m_flags = CObject::Remap32BitFlagsTo16Bit( flags );
			}
			else
			{
				file << m_flags;
			}
			file << m_template;

			// legacy stuff - removed after CRC was added so we are not calculating CRC from crap
			String embeddedPathDummy;
			CDateTime embeddedTimeStampDummp;
			file << embeddedPathDummy;
			file << embeddedTimeStampDummp;

			return true;
		}
	};

	// legacy soft import data
	class LegacySoftImport
	{
	public:
		String		m_path;

		RED_FORCE_INLINE LegacySoftImport()
		{};

		RED_FORCE_INLINE Bool Load( IFile &file )
		{
			// conform the path to new rules
			String unsafePath, safePath;
			file  << unsafePath;
			CFilePath::GetConformedPath( unsafePath, safePath );

			// follow links...
			CDiskFile* diskFile = GDepot->FindFile( safePath );
			if ( !diskFile )
			{
				diskFile = GDepot->FindFileUseLinks( safePath, 0 );
				if ( diskFile ) 
				{
					safePath = diskFile->GetDepotPath();
				}
			}

			// use the safe path
			m_path = safePath;
			return true;
		}
	};

	// Load legacy name
	static Bool LoadName( IFile& file, CName& outName, Uint16& outNameIndex, const TDynArray< LegacyName >& legacyNames )
	{
		// read name index
		Uint16 nameIndex = 0;
		file << nameIndex;

		// invalid name index ?
		if ( nameIndex >= legacyNames.Size() )
		{
			WARN_CORE( TXT("Invalid name index (%d/%d) in '%ls'"), nameIndex, legacyNames.Size(), file.GetFileNameForDebug() );
			return false;
		}

		// get the name
		outNameIndex = nameIndex;
		outName = legacyNames[ nameIndex ].m_name;
		return true;
	}

	// Append string to string table
	static Uint32 AddString( CDependencyFileData& data, TStringMap& stringMap, const StringAnsi& string )
	{
		// empty string case
		if ( string.Empty() )
			return 0;

		// find in string map
		Uint32 index = 0;
		if ( stringMap.Find( string, index ) )
			return index;

		// add to data
		const Uint32 length = string.GetLength();
		const Uint32 offset = data.m_strings.Size();
		data.m_strings.Grow( length + 1 );
		Red::MemoryCopy( &data.m_strings[ offset ], string.AsChar(), sizeof(AnsiChar) * (length+1) );

		// add to map
		stringMap.Insert( string, offset );
		return offset;
	}
};