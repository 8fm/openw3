/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "meshVertex.h"

IMPLEMENT_RTTI_ENUM( EMeshVertexType );

IFile& operator<<( IFile& file, SMeshVertex& v )
{
	// this is only called if we are serializing one by one

	file << v.m_position[0] << v.m_position[1] << v.m_position[2];
	file << v.m_indices[0] << v.m_indices[1] << v.m_indices[2] << v.m_indices[3];
	file << v.m_weights[0] << v.m_weights[1] << v.m_weights[2] << v.m_weights[3];
	file << v.m_normal[0] << v.m_normal[1] << v.m_normal[2];
	file << v.m_color;
	file << v.m_uv0[0] << v.m_uv0[1];
	file << v.m_uv1[0] << v.m_uv1[1];
	file << v.m_tangent[0] << v.m_tangent[1] << v.m_tangent[2];
	file << v.m_binormal[0] << v.m_binormal[1] << v.m_binormal[2];

	if (file.GetVersion() < VER_ADDITIONAL_VERTEX_DATA)
	{
		file << v.m_extraData0[0] << v.m_extraData0[1] << v.m_extraData0[2] << v.m_extraData0[3];
		file << v.m_extraData1[0] << v.m_extraData1[1];
	}
	else
	{
		file << v.m_extraData0[0] << v.m_extraData0[1] << v.m_extraData0[2] << v.m_extraData0[3];
		file << v.m_extraData1[0] << v.m_extraData1[1] << v.m_extraData1[2] << v.m_extraData1[3];
		file << v.m_extraData2[0] << v.m_extraData2[1] << v.m_extraData2[2] << v.m_extraData2[3];
		file << v.m_extraData3[0] << v.m_extraData3[1] << v.m_extraData3[2] << v.m_extraData3[3];
	}

	return file;
}