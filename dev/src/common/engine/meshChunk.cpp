/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "meshChunk.h"
#include "../core/file.h"

// Old function to support legacy crap
void SerializeVertexType( IFile& file, EMeshVertexType& vt )
{
	// Serialize using enum definition
	static CEnum* enumType = SRTTI::GetInstance().FindEnum( GetTypeName< EMeshVertexType >() );
	enumType->SerializeSimple( file, &vt );
}

IFile& operator<<( IFile& file, SMeshChunk& c )
{
	// Special code to serialize vertex type, a lot of old legacy...
	SerializeVertexType( file, c.m_vertexType );

	file << c.m_materialID;
	file << c.m_numBonesPerVertex;
	file << c.m_numVertices;
	file << c.m_numIndices;

	if ( file.GetVersion() >= VER_SEPARATED_PAYLOAD_FROM_MESH_CHUNKS )
	{
		file << c.m_renderMask;
	}

	if ( file.GetVersion() < VER_ADDITIONAL_VERTEX_DATA )
	{
		// Serialize the slow way
		file << c.m_vertices;
		file << c.m_indices;
	}
	else
	{
		// Serialize as bulk data
		c.m_vertices.SerializeBulk( file );
		c.m_indices.SerializeBulk( file );
	}

	// Restore tables sizes
	if ( file.IsReader() )
	{
		c.m_numVertices = c.m_vertices.Size();
		c.m_numIndices = c.m_indices.Size();
	}

	// Loaded
	return file;
}