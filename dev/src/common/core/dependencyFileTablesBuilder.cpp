/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "class.h"
#include "property.h"
#include "dependencyFileTables.h"
#include "dependencyFileTablesBuilder.h"

CDependencyFileDataBuilder::CDependencyFileDataBuilder( class CDependencyFileData& outData )
	: m_data( &outData )
{
	// cleanup
	m_data->m_buffers.ClearFast();
	m_data->m_buffers.Reserve( 100 );
	m_data->m_exports.ClearFast();
	m_data->m_exports.Reserve( 1000 );
	m_data->m_imports.ClearFast();
	m_data->m_imports.Reserve( 100 );
	m_data->m_names.ClearFast();
	m_data->m_names.Reserve( 200 );
	m_data->m_properties.ClearFast();
	m_data->m_properties.Reserve( 400 );
	m_data->m_strings.ClearFast();
	m_data->m_strings.Reserve( 4096 );

	// allocate empty string entry
	m_data->m_strings.PushBack( 0 );

	// allocate empty name entry
	CDependencyFileData::Name emptyName;
	emptyName.m_hash = 0;
	emptyName.m_string = 0;
	m_data->m_names.PushBack( emptyName );

	// allocate empty property entry
	CDependencyFileData::Property emptyProp;
	Red::MemoryZero( &emptyProp, sizeof(emptyProp) );
	m_data->m_properties.PushBack( emptyProp );
}

CDependencyFileDataBuilder::~CDependencyFileDataBuilder()
{
}

Uint32 CDependencyFileDataBuilder::MapString( const StringAnsi& string )
{
	// empty string always maps to zero
	if ( string.Empty() )
		return 0;

	// find in map
	Uint32 index = 0;
	if ( m_stringMap.Find( string, index ) )
		return index;

	// append
	const Uint32 length = string.GetLength();
	const Uint32 offset = m_data->m_strings.Size();
	m_data->m_strings.Grow( length + 1 );
	Red::MemoryCopy( &m_data->m_strings[ offset ], string.AsChar(), sizeof(AnsiChar) * (length+1) );

	// add to map
	m_stringMap.Insert( string, offset );
	return offset;
}

Uint16 CDependencyFileDataBuilder::MapName( const CName name )
{
	// null name is always mapped to 0
	if ( !name )
		return 0;

	// find in map
	Uint16 index = 0;
	if ( m_nameMap.Find( name, index ) )
		return index;

	// add new name
	index = (Uint16) m_data->m_names.Size();
	CDependencyFileData::Name nameData;
	nameData.m_hash = name.GetSerializationHash().GetValue();
	nameData.m_string = MapString( name.AsAnsiChar() );
	m_data->m_names.PushBack( nameData );

	// add to map
	m_nameMap.Insert( name, index );
	return index;
}

Uint16 CDependencyFileDataBuilder::MapProperty( const CProperty* prop )
{
	// null property is always mapped to 0
	if ( !prop )
		return 0;

	// find in map
	Uint16 index = 0;
	if ( m_propertyMap.Find( prop, index ) )
		return index;

	// add new property
	index = (Uint16) m_data->m_properties.Size();
	CDependencyFileData::Property propertyData;
	propertyData.m_className = MapName( prop->GetParent()->GetName() );
	propertyData.m_typeName = MapName( prop->GetType()->GetName() );
	propertyData.m_propertyName = MapName( prop->GetName() );
	propertyData.m_flags = 0;
	propertyData.m_hash = prop->GetHash();
	m_data->m_properties.PushBack( propertyData );

	// add to map
	m_propertyMap.Insert( prop, index );
	return index;
}

Uint32 CDependencyFileDataBuilder::MapImport( const ImportInfo& importInfo, const Bool hashPath )
{
	// invalid resource path
	if ( importInfo.m_path.Empty() )
		return (Uint32)-1;

	// invalid class name
	if ( importInfo.m_className.Empty() )
		return (Uint32)-1;

	// create import info
	CDependencyFileData::Import importData;
	importData.m_className = MapName( importInfo.m_className );
	importData.m_flags = 0;

	// save the file path - either as hash or as full string
	if ( hashPath )
	{
		importData.m_path = Red::System::CalculatePathHash32( importInfo.m_path.AsChar() );
		importData.m_flags |= CDependencyFileData::eImportFlags_HashedPath;
	}
	else
	{
		importData.m_path = MapString( importInfo.m_path );
	}

	// setup flags
	if ( importInfo.m_isObligatory )
		importData.m_flags |= CDependencyFileData::eImportFlags_Obligatory;
	if ( importInfo.m_isTemplate )
		importData.m_flags |= CDependencyFileData::eImportFlags_Template;
	if ( importInfo.m_isSoft )
		importData.m_flags |= CDependencyFileData::eImportFlags_Soft;

	// add to list
	const Uint32 index = (Uint32) m_data->m_imports.Size();
	m_data->m_imports.PushBack( importData );
	return index;
}

Uint32 CDependencyFileDataBuilder::MapExport( const ExportInfo& exportInfo )
{
	// invalid class name
	if ( exportInfo.m_className.Empty() )
		return (Uint32)-1;

	// create export info
	CDependencyFileData::Export exportData;
	exportData.m_className = MapName( exportInfo.m_className );
	exportData.m_dataOffset = 0; // not set yet
	exportData.m_dataSize = 0; // not set yet
	exportData.m_objectFlags = exportInfo.m_objectFlags;
	exportData.m_parent = exportInfo.m_parent;
	exportData.m_template = exportInfo.m_template;
	exportData.m_crc = 0; // not set yet

	// add to list
	const Uint32 index = (Uint32) m_data->m_exports.Size();
	m_data->m_exports.PushBack( exportData );
	return index;
}

Uint32 CDependencyFileDataBuilder::MapBuffer( const BufferInfo& bufferInfo )
{
	// Allocate index
	const Uint32 bufferIndex = (Uint32) m_data->m_buffers.Size() + 1;

	// create export info
	CDependencyFileData::Buffer bufferData;
	bufferData.m_dataOffset = 0; // not set yet
	bufferData.m_flags = 0; // not used
	bufferData.m_name = 0; // not used
	bufferData.m_index = bufferIndex;
	bufferData.m_dataSizeInMemory = bufferInfo.m_dataSizeInMemory; 
	bufferData.m_dataSizeOnDisk = 0; // not known yet
	bufferData.m_crc = 0; // not set yet

	// add to list
	m_data->m_buffers.PushBack( bufferData );
	return bufferIndex;
}

void CDependencyFileDataBuilder::PatchExport( const Uint32 exportIndex, const Uint32 dataOffset, const Uint32 dataSize, const Uint32 dataCRC )
{
	RED_FATAL_ASSERT( exportIndex < m_data->m_exports.Size(), "Export index out of range" );
	m_data->m_exports[ exportIndex ].m_dataOffset = dataOffset;
	m_data->m_exports[ exportIndex ].m_dataSize = dataSize;
	m_data->m_exports[ exportIndex ].m_crc = dataCRC;
}

void CDependencyFileDataBuilder::PatchBuffer( const Uint32 bufferIndex, const Uint32 dataOffset, const Uint32 dataSizeOnDisk, const Uint32 bufferCRC )
{
	RED_FATAL_ASSERT( bufferIndex > 0 && bufferIndex <= m_data->m_buffers.Size(), "Buffer index out of range" );

	const Uint32 entryIndex = bufferIndex - 1;
	RED_FATAL_ASSERT( dataSizeOnDisk <= m_data->m_buffers[ entryIndex ].m_dataSizeInMemory, "Buffer is smaller in memory than on disk - compression fuckup" );
	m_data->m_buffers[ entryIndex ].m_dataOffset = dataOffset;
	m_data->m_buffers[ entryIndex ].m_dataSizeOnDisk = dataSizeOnDisk;
	m_data->m_buffers[ entryIndex ].m_crc = bufferCRC;
}