/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _MESH_GENERATOR_H_
#define _MESH_GENERATOR_H_

#include "build.h"

struct Vertex
{
	Float x, y, z;
	Vertex(){};
	Vertex( Float pX, Float pY, Float pZ )
	{
		x = pX;
		y = pY;
		z = pZ;
	}
};

struct SCENE_VERTEX
{
	Vertex Pos;
	Vertex Normal;
	SCENE_VERTEX(){}
	SCENE_VERTEX( Vertex pos, Vertex norm )
	{
		Pos = pos;
		Normal = norm;
	}

	static GpuApi::eBufferChunkType GetSystemVertexType()
	{
		return GpuApi::BCT_VertexSystemPosNormal;
	}
};

class CMeshGenerator
{
public:
	CMeshGenerator();
	~CMeshGenerator();

	static void GetSphere( Uint32 stacks, Uint32 slices, Float radius, Uint16** outIndexData, SCENE_VERTEX** outVertexData, Uint32& numIndices, Uint32& numVertices );
	static void GetCylinder( Uint32 stacks, Uint32 slices, Float radius, Float height, Uint16** outIndexData, SCENE_VERTEX** outVertexData, Uint32& numIndices, Uint32& numVertices );
	static void GetDisc( Uint32 slices, Float radius, Uint16** outIndexData, SCENE_VERTEX** outVertexData, Uint32& numIndices, Uint32& numVertices );
	static void GetBox( Float side, Uint16** outIndexData, SCENE_VERTEX** outVertexData, Uint32& numIndices, Uint32& numVertices );
	static void GetQuad( Float side,  Uint16** outIndexData, SCENE_VERTEX** outVertexData, Uint32& numIndices, Uint32& numVertices );

	static void GetVerticesNonIndexed( SCENE_VERTEX** outVertexData, const Uint16 * indexData, const SCENE_VERTEX * vertexData, const Uint32 numIndices );

};

#endif