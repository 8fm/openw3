/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _ENGINE_MESH_VERTEX_H_
#define _ENGINE_MESH_VERTEX_H_

#include "../core/enum.h"

enum EMeshVertexType : CEnum::TValueType
{
	MVT_StaticMesh,						//!< Static mesh without skinning data
	MVT_SkinnedMesh,					//!< Skinned mesh 
	MVT_DestructionMesh,
	MVT_Max
};

BEGIN_ENUM_RTTI( EMeshVertexType );
ENUM_OPTION( MVT_StaticMesh );
ENUM_OPTION( MVT_SkinnedMesh );
ENUM_OPTION( MVT_DestructionMesh );
END_ENUM_RTTI();

// Mesh vertex, bulk serialized
//CM: please make IPlatformsUtils.h in sync with this
struct SMeshVertex
{
	Float	m_position[3];			// Vertex position
	Uint8	m_indices[4];			// Skinning indices
	Float	m_weights[4];			// Skinning weights
	Float	m_normal[3];			// Vertex normal
	Uint32	m_color;				// Vertex color
	Float	m_uv0[2];				// First uv set
	Float	m_uv1[2];				// Second uv set
	Float	m_tangent[3];			// Vertex tangent vector
	Float	m_binormal[3];			// Vertex binormal vector
	Float	m_extraData0[4];		// Extra data
	Float	m_extraData1[4];		// Extra data
	Float	m_extraData2[4];		// Extra data
	Float	m_extraData3[4];		// Extra data

	RED_INLINE Vector GetPosition() const
	{
		return Vector( m_position[0], m_position[1], m_position[2], 1.0f );
	}
};

IFile& operator<<( IFile& file, SMeshVertex& v );

RED_INLINE Bool IsCollisionMeshVertexType( EMeshVertexType vertexType )
{
	return ( MVT_StaticMesh == vertexType ) 
		|| ( MVT_SkinnedMesh == vertexType )
		|| ( MVT_DestructionMesh == vertexType );
}

RED_INLINE Bool IsSkinnedMeshVertexType( EMeshVertexType vertexType )
{
	return ( MVT_SkinnedMesh == vertexType ) || ( MVT_DestructionMesh == vertexType );
}

#endif
