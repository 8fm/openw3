/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "meshGenerator.h"

CMeshGenerator::CMeshGenerator()
{

}

CMeshGenerator::~CMeshGenerator()
{

}

void CMeshGenerator::GetSphere(Uint32 stacks, Uint32 slices, Float radius, Uint16** outIndexData, SCENE_VERTEX** outVertexData, Uint32& numIndices, Uint32& numVertices)
{
	Uint32 n = (stacks - 1) * slices + 2;
	numVertices = n;
	*outVertexData = new SCENE_VERTEX[n];
	(*outVertexData)[0].Pos = Vertex(0.0f, radius, 0.0f);
	(*outVertexData)[0].Normal = Vertex(0.0f, 1.0f, 0.0f);
	Float dp = M_PI / stacks;
	Float phi = dp;
	Uint32 k = 1;
	for (Uint32 i = 0; i < stacks - 1; ++i, phi += dp)
	{
		Float cosp, sinp;
		sinp = sinf( phi );
		cosp = cosf( phi );
		Float thau = 0.0f;
		Float dt = 2.0f * M_PI / slices;
		Float stackR = radius * sinp;
		Float stackY = radius * cosp;
		for (Uint32 j = 0; j < slices; ++j, thau += dt)
		{
			Float cost, sint;
			sint = sinf( thau );
			cost = cosf( thau );
			(*outVertexData)[k].Pos = Vertex(stackR * cost, stackY, stackR * sint);
			(*outVertexData)[k++].Normal = Vertex(cost * sinp, cosp, sint * sinp);
		}
	}
	(*outVertexData)[k].Pos = Vertex(0.0f, -radius, 0.0f);
	(*outVertexData)[k].Normal = Vertex(0.0f, -1.0f, 0.0f);
	Uint32 in = (stacks - 1) * slices * 6;
	numIndices = in;
	*outIndexData = new Uint16[in];
	k = 0;
	for (Uint32 j = 0; j < slices - 1; ++j)
	{
		(*outIndexData)[k++] = 0;
		(*outIndexData)[k++] = j + 2;
		(*outIndexData)[k++] = j + 1;
	}
	(*outIndexData)[k++] = 0;
	(*outIndexData)[k++] = 1;
	(*outIndexData)[k++] = slices;
	Uint32 i = 0;
	for (; i < stacks - 2; ++i)
	{
		Uint32 j = 0;
		for (; j < slices - 1; ++j)
		{
			(*outIndexData)[k++] = i*slices + j + 1;
			(*outIndexData)[k++] = i*slices + j + 2;
			(*outIndexData)[k++] = (i + 1)*slices + j + 2;
			(*outIndexData)[k++] = i*slices + j + 1;
			(*outIndexData)[k++] = (i + 1)*slices + j + 2;
			(*outIndexData)[k++] = (i + 1)*slices + j + 1;
		}
		(*outIndexData)[k++] = i*slices + j + 1;
		(*outIndexData)[k++] = i*slices + 1;
		(*outIndexData)[k++] = (i + 1)*slices + 1;
		(*outIndexData)[k++] = i*slices + j + 1;
		(*outIndexData)[k++] = (i + 1)*slices + 1;
		(*outIndexData)[k++] = (i + 1)*slices + j + 1;
	}
	for (Uint32 j = 0; j < slices - 1; ++j)
	{
		(*outIndexData)[k++] = i*slices + j + 1;
		(*outIndexData)[k++] = i*slices + j + 2;
		(*outIndexData)[k++] = n - 1;
	}
	(*outIndexData)[k++] = (i + 1)*slices;
	(*outIndexData)[k++] = i*slices + 1;
	(*outIndexData)[k++] = n - 1;
}

void CMeshGenerator::GetCylinder(Uint32 stacks, Uint32 slices, Float radius, Float height, Uint16** outIndexData, SCENE_VERTEX** outVertexData, Uint32& numIndices, Uint32& numVertices)
{
	Uint32 n = (stacks + 1) * slices;
	*outVertexData = new SCENE_VERTEX[n];
	numVertices = n;
	Float y = height / 2;
	Float dy = height / stacks;
	Float dp = 2.0f * M_PI / slices;
	Uint32 k = 0;
	for (Uint32 i = 0; i <= stacks; ++i, y -= dy)
	{
		Float phi = 0.0f;
		for (Uint32 j = 0; j < slices; ++j, phi += dp)
		{
			Float sinp, cosp;
			sinp = sinf( phi );
			cosp = cosf( phi );
			(*outVertexData)[k].Pos = Vertex(radius*cosp, y, radius*sinp);
			(*outVertexData)[k++].Normal = Vertex(cosp, 0, sinp);
		}
	}
	Uint32 in = 6 * stacks * slices;
	numIndices = in;
	*outIndexData = new Uint16[in];
	k = 0;
	for (Uint32 i = 0; i < stacks; ++i)
	{
		Uint32 j = 0;
		for (; j < slices - 1; ++j)
		{
			(*outIndexData)[k++] = i*slices + j;
			(*outIndexData)[k++] = i*slices + j + 1;
			(*outIndexData)[k++] = (i + 1)*slices + j + 1;
			(*outIndexData)[k++] = i*slices + j;
			(*outIndexData)[k++] = (i + 1)*slices + j + 1;
			(*outIndexData)[k++] = (i + 1)*slices + j;
		}
		(*outIndexData)[k++] = i*slices + j;
		(*outIndexData)[k++] = i*slices;
		(*outIndexData)[k++] = (i + 1)*slices;
		(*outIndexData)[k++] = i*slices + j;
		(*outIndexData)[k++] = (i + 1)*slices;
		(*outIndexData)[k++] = (i + 1)*slices + j;
	}
}

void CMeshGenerator::GetDisc(Uint32 slices, Float radius, Uint16** outIndexData, SCENE_VERTEX** outVertexData, Uint32& numIndices, Uint32& numVertices)
{
	Uint32 n = slices + 1;
	*outVertexData = new SCENE_VERTEX[n];
	numVertices = n;
	(*outVertexData)[0].Pos = Vertex(0.0f, 0.0f, 0.0f);
	(*outVertexData)[0].Normal = Vertex(0.0f, 1.0f, 0.0f);
	Float phi = 0.0f;
	Float dp = 2.0f * M_PI / slices;
	Uint32 k = 1;
	for (Uint32 i = 1; i <= slices; ++i, phi += dp)
	{
		Float cosp, sinp;
		sinp = sinf( phi );
		cosp = cosf( phi );
		(*outVertexData)[k].Pos = Vertex(radius * cosp, 0.0f, radius * sinp);
		(*outVertexData)[k++].Normal = Vertex(0.0f, 1.0f, 0.0f);
	}
	Uint32 in = slices * 3;
	numIndices = in;
	*outIndexData = new Uint16[in];
	k = 0;
	for (Uint32 i = 0; i < slices - 1; ++i)
	{
		(*outIndexData)[k++] = 0;
		(*outIndexData)[k++] = i + 2;
		(*outIndexData)[k++] = i + 1;
	}
	(*outIndexData)[k++] = 0;
	(*outIndexData)[k++] = 1;
	(*outIndexData)[k++] = slices;
}

void CMeshGenerator::GetBox(Float side, Uint16** outIndexData, SCENE_VERTEX** outVertexData, Uint32& numIndices, Uint32& numVertices)
{
	side /= 2;
	SCENE_VERTEX vertices[] =
	{
		//Front face
		SCENE_VERTEX( Vertex(-side, -side, -side), Vertex(0.0f, 0.0f, -1.0f) ),
		SCENE_VERTEX( Vertex(-side, side, -side), Vertex(0.0f, 0.0f, -1.0f) ),
		SCENE_VERTEX( Vertex(side, side, -side), Vertex(0.0f, 0.0f, -1.0f) ),
		SCENE_VERTEX( Vertex(side, -side, -side), Vertex(0.0f, 0.0f, -1.0f) ),

		//Left face
		SCENE_VERTEX( Vertex(-side, -side, -side), Vertex(-1.0f, 0.0f, 0.0f) ),
		SCENE_VERTEX( Vertex(-side, -side, side), Vertex(-1.0f, 0.0f, 0.0f) ),
		SCENE_VERTEX( Vertex(-side, side, side), Vertex(-1.0f, 0.0f, 0.0f) ),
		SCENE_VERTEX( Vertex(-side, side, -side), Vertex(-1.0f, 0.0f, 0.0f) ),

		//Bottom face
		SCENE_VERTEX( Vertex(-side, -side, -side), Vertex(0.0f, -1.0f, 0.0f) ),
		SCENE_VERTEX( Vertex(side, -side, -side), Vertex(0.0f, -1.0f, 0.0f) ),
		SCENE_VERTEX( Vertex(side, -side, side), Vertex(0.0f, -1.0f, 0.0f) ),
		SCENE_VERTEX( Vertex(-side, -side, side), Vertex(0.0f, -1.0f, 0.0f) ),

		//Back face
		SCENE_VERTEX( Vertex(-side, -side, side), Vertex(0.0f, 0.0f, 1.0f) ),
		SCENE_VERTEX( Vertex(side, -side, side), Vertex(0.0f, 0.0f, 1.0f) ),
		SCENE_VERTEX( Vertex(side, side, side), Vertex(0.0f, 0.0f, 1.0f) ),
		SCENE_VERTEX( Vertex(-side, side, side), Vertex(0.0f, 0.0f, 1.0f) ),

		//Right face
		SCENE_VERTEX( Vertex(side, -side, -side), Vertex(1.0f, 0.0f, 0.0f) ),
		SCENE_VERTEX( Vertex(side, side, -side), Vertex(1.0f, 0.0f, 0.0f) ),
		SCENE_VERTEX( Vertex(side, side, side), Vertex(1.0f, 0.0f, 0.0f) ),
		SCENE_VERTEX( Vertex(side, -side, side), Vertex(1.0f, 0.0f, 0.0f) ),

		//Top face
		SCENE_VERTEX( Vertex(-side, side, -side), Vertex(0.0f, 1.0f, 0.0f) ),
		SCENE_VERTEX( Vertex(-side, side, side), Vertex(0.0f, 1.0f, 0.0f) ),
		SCENE_VERTEX( Vertex(side, side, side), Vertex(0.0f, 1.0f, 0.0f) ),
		SCENE_VERTEX( Vertex(side, side, -side), Vertex(0.0f, 1.0f, 0.0f) ),
	};
	numVertices = 24;
	*outVertexData = new SCENE_VERTEX[numVertices];
	for( Uint16 i = 0; i < numVertices; ++i )
	{
		(*outVertexData)[i] = vertices[i];
	}

	Uint16 indices[] = 
	{
		0, 1, 2, 0, 2, 3,		//Front face
		4, 5, 6, 4, 6, 7,		//Left face
		8, 9, 10, 8, 10, 11,	//Botton face
		12, 13, 14, 12, 14, 15,	//Back face
		16, 17, 18, 16, 18, 19,	//Right face
		20, 21, 22, 20, 22, 23	//Top face
	};
	numIndices = 36;
	*outIndexData = new Uint16[numIndices];
	for( Uint16 i = 0; i < numIndices; ++i )
	{
		(*outIndexData)[i] = indices[i];
	}
}

void CMeshGenerator::GetQuad(Float side, Uint16** outIndexData, SCENE_VERTEX** outVertexData, Uint32& numIndices, Uint32& numVertices)
{
	side /= 2;
	SCENE_VERTEX vertices[] =
	{
		SCENE_VERTEX( Vertex(-side, -side, 0.0f), Vertex(0.0f, 0.0f, -1.0f) ),
		SCENE_VERTEX( Vertex(-side, side, 0.0f), Vertex(0.0f, 0.0f, -1.0f) ),
		SCENE_VERTEX( Vertex(side, side, 0.0f), Vertex(0.0f, 0.0f, -1.0f) ),
		SCENE_VERTEX( Vertex(side, -side, 0.0f), Vertex(0.0f, 0.0f, -1.0f) )
	};
	numVertices = 4;
	*outVertexData = new SCENE_VERTEX[numVertices];
	for( Uint16 i = 0; i < numVertices; ++i )
	{
		(*outVertexData)[i] = vertices[i];
	}

	Uint16 indices[] = { 0, 1, 2, 0, 2, 3 };
	numIndices = 6;
	*outIndexData = new Uint16[numIndices];
	for( Uint16 i = 0; i < numIndices; ++i )
	{
		(*outIndexData)[i] = indices[i];
	}
}

void CMeshGenerator::GetVerticesNonIndexed(SCENE_VERTEX** outVertexData, const Uint16 * indexData, const SCENE_VERTEX * vertexData, const Uint32 numIndices)
{
	*outVertexData = new SCENE_VERTEX[numIndices];
	for( Uint32 i = 0; i < numIndices; ++i )
	{
		(*outVertexData)[i] = vertexData[ indexData[i] ];
	}
}
