/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "sectorData.h"
#include "../core/fileSkipableBlock.h"

IMPLEMENT_ENGINE_CLASS( CSectorData );

CSectorData::CSectorData()
{
}

Bool CSectorData::HasData() const
{
	return !m_objects.Empty();
}

const Uint32 CSectorData::CalcMemorySize() const
{
	Uint32 memSize = 0;
	memSize += (Uint32) m_objects.DataSize();
	memSize += (Uint32) m_resources.DataSize();
	memSize += (Uint32) m_dataStream.DataSize();
	memSize += sizeof(CSectorData);
	return memSize;
}

const Uint32 CSectorData::GetNumResources() const
{
	return m_resources.Size();
}

const Uint64 CSectorData::GetResourcePathHash( const Uint32 resourceIndex ) const
{
	// invalid resource
	if ( resourceIndex == 0 || resourceIndex >= m_resources.Size() )
		return 0;

	// get resource path
	return m_resources[ resourceIndex ].m_pathHash;
}

const Box CSectorData::GetResourceLocalBounds( const Uint32 resourceIndex ) const
{
	// invalid resource
	if ( resourceIndex == 0 || resourceIndex >= m_resources.Size() )
		return Box(Vector::ZEROS, 0.5f);

	// get resource box
	const auto& box = m_resources[ resourceIndex ].m_box;
	return Box( 
		Vector( box[SectorData::eBox_MinX], box[SectorData::eBox_MinY], box[SectorData::eBox_MinZ], 1.0f ),
		Vector( box[SectorData::eBox_MaxX], box[SectorData::eBox_MaxY], box[SectorData::eBox_MaxZ], 1.0f ) );
}

const Int32 CSectorData::GetResourceIndexForObject( const Uint32 objectIndex ) const
{
	// valid object ?
	if ( objectIndex < m_objects.Size() )
	{
		const auto& obj = m_objects[ objectIndex ];
		if ( obj.m_type == SectorData::eObject_Mesh )
		{
			const SectorData::PackedMesh* mesh = (const SectorData::PackedMesh*) &m_dataStream[ obj.m_offset ];
			return mesh->m_mesh.m_resourceIndex;
		}
	}

	// invalid
	return -1;
}

void CSectorData::OnSerialize( IFile& file )
{
	ISerializable::OnSerialize( file );

	// no data is processed for GC
	if ( file.IsGarbageCollector() )
		return;

	// store version
	Uint8 version = SectorData::DATA_VERSION;
	file << version;

	// internal data is wrapped in a skip block
	CFileSkipableBlock block( file );
	if ( version == SectorData::DATA_VERSION )
	{
		// bulk data
		m_resources.SerializeBulk( file );
		m_objects.SerializeBulk( file );
		m_dataStream.SerializeBulk( file );
	}
	else
	{
		// skip the data - it's better to have nothing than crash
		block.Skip();
	}
}



