/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "drawsTestUtils.h"
#include "meshGenerator.h"

// Effects
#include "vertexPixelEffect.h"
#include "computeEffect.h"

CTestUtils::CTestUtils()
{
}

CTestUtils::~CTestUtils()
{
	for( SDrawableObject o : m_objects )
	{
		delete[] o.Indices;
		delete[] o.Vertices;
		delete[] o.VerticesNonIndexed;
	}

	for( IEffect* e : m_effects )
	{
		delete e;
	}
}

Bool CTestUtils::Initialize( )
{
	return InitializeDrawableObjects() && InitializeEffects();
}

Bool CTestUtils::InitializeDrawableObjects()
{
	m_objects.Grow( DrawableObjectType::DOT_Max );
	
	{
		// Quad
		SDrawableObject& obj = m_objects[DOT_Quad];
		CMeshGenerator::GetQuad(2.0f, &obj.Indices, &obj.Vertices, obj.NumIndices, obj.NumVertices );
		CMeshGenerator::GetVerticesNonIndexed(&obj.VerticesNonIndexed, obj.Indices, obj.Vertices, obj.NumIndices);
		GpuApi::BufferInitData bufInitData;
		bufInitData.m_buffer = obj.Indices;
		obj.IndexBuffer = GpuApi::CreateBuffer( obj.NumIndices * sizeof( Uint16 ), GpuApi::BCC_Index16Bit, GpuApi::BUT_Immutable, 0, &bufInitData );

		bufInitData.m_buffer = obj.VerticesNonIndexed;
		obj.VertexBufferNonIndexed = GpuApi::CreateBuffer( obj.NumIndices * sizeof( SCENE_VERTEX ), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );

		bufInitData.m_buffer = obj.Vertices;
		obj.VertexBufferIndexed = GpuApi::CreateBuffer( obj.NumVertices * sizeof( SCENE_VERTEX ), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );
		
		obj.Color = RedVector4(0.5f,0.5f,0.5f,1.0f);
		obj.LocalMatrix = obj.LocalMatrix.IDENTITY;
		obj.LocalMatrix.SetRotX(DEG2RAD(90));
	}

	{
		// Sphere
		SDrawableObject& obj = m_objects[DOT_Sphere];
		CMeshGenerator::GetSphere(16, 16, 0.3f, &obj.Indices, &obj.Vertices, obj.NumIndices, obj.NumVertices );
		CMeshGenerator::GetVerticesNonIndexed(&obj.VerticesNonIndexed, obj.Indices, obj.Vertices, obj.NumIndices);

		GpuApi::BufferInitData bufInitData;
		bufInitData.m_buffer = obj.Indices;
		obj.IndexBuffer = GpuApi::CreateBuffer( obj.NumIndices * sizeof( Uint16 ), GpuApi::BCC_Index16Bit, GpuApi::BUT_Immutable, 0, &bufInitData );

		bufInitData.m_buffer = obj.VerticesNonIndexed;
		obj.VertexBufferNonIndexed = GpuApi::CreateBuffer( obj.NumIndices * sizeof( SCENE_VERTEX ), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );

		bufInitData.m_buffer = obj.Vertices;
		obj.VertexBufferIndexed = GpuApi::CreateBuffer( obj.NumVertices * sizeof( SCENE_VERTEX ), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );

		obj.Color = RedVector4(1.0f, 0, 0, 1.0f);
		obj.LocalMatrix = obj.LocalMatrix.IDENTITY;
		obj.LocalMatrix.SetTranslation(-0.8f, 0.3f, 0.0f);
	}

	{
		// Box
		SDrawableObject& obj = m_objects[DOT_Box];
		CMeshGenerator::GetBox( 0.4f, &obj.Indices, &obj.Vertices, obj.NumIndices, obj.NumVertices );
		CMeshGenerator::GetVerticesNonIndexed(&obj.VerticesNonIndexed, obj.Indices, obj.Vertices, obj.NumIndices);

		GpuApi::BufferInitData bufInitData;
		bufInitData.m_buffer = obj.Indices;
		obj.IndexBuffer =  GpuApi::CreateBuffer( obj.NumIndices * sizeof( Uint16 ), GpuApi::BCC_Index16Bit, GpuApi::BUT_Immutable, 0, &bufInitData );

		bufInitData.m_buffer = obj.VerticesNonIndexed;
		obj.VertexBufferNonIndexed = GpuApi::CreateBuffer( obj.NumIndices * sizeof( SCENE_VERTEX ), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );

		bufInitData.m_buffer = obj.Vertices;
		obj.VertexBufferIndexed = GpuApi::CreateBuffer( obj.NumVertices * sizeof( SCENE_VERTEX ), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );

		obj.Color = RedVector4(0.0f, 1.0f, 0, 1.0f);
		obj.LocalMatrix = obj.LocalMatrix.IDENTITY;
		obj.LocalMatrix.SetTranslation(0, 0.2f, 0.5f);
	}

	{
		// Cylinder
		SDrawableObject& obj = m_objects[DOT_Cylinder];
		CMeshGenerator::GetCylinder( 16, 16, 0.3f, 0.5f, &obj.Indices, &obj.Vertices, obj.NumIndices, obj.NumVertices );
		CMeshGenerator::GetVerticesNonIndexed(&obj.VerticesNonIndexed, obj.Indices, obj.Vertices, obj.NumIndices);

		GpuApi::BufferInitData bufInitData;
		bufInitData.m_buffer = obj.Indices;
		obj.IndexBuffer = GpuApi::CreateBuffer( obj.NumIndices * sizeof( Uint16 ), GpuApi::BCC_Index16Bit, GpuApi::BUT_Immutable, 0, &bufInitData );

		bufInitData.m_buffer = obj.VerticesNonIndexed;
		obj.VertexBufferNonIndexed = GpuApi::CreateBuffer( obj.NumIndices * sizeof( SCENE_VERTEX ), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );

		bufInitData.m_buffer = obj.Vertices;
		obj.VertexBufferIndexed = GpuApi::CreateBuffer( obj.NumVertices * sizeof( SCENE_VERTEX ), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );

		obj.Color = RedVector4(0.0f, 0.0f, 1.0f, 1.0f);
		obj.LocalMatrix = obj.LocalMatrix.IDENTITY;
		obj.LocalMatrix.SetTranslation(0.8f, 0.25f, 0.0f);
	}

	{
		// Disc
		SDrawableObject& obj = m_objects[DOT_Disc];
		CMeshGenerator::GetDisc( 16, 0.3f, &obj.Indices, &obj.Vertices, obj.NumIndices, obj.NumVertices );
		CMeshGenerator::GetVerticesNonIndexed(&obj.VerticesNonIndexed, obj.Indices, obj.Vertices, obj.NumIndices);

		GpuApi::BufferInitData bufInitData;
		bufInitData.m_buffer = obj.Indices;
		obj.IndexBuffer = GpuApi::CreateBuffer( obj.NumIndices * sizeof( Uint16 ), GpuApi::BCC_Index16Bit, GpuApi::BUT_Immutable, 0, &bufInitData );

		bufInitData.m_buffer = obj.VerticesNonIndexed;
		obj.VertexBufferNonIndexed = GpuApi::CreateBuffer( obj.NumIndices * sizeof( SCENE_VERTEX ), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );

		bufInitData.m_buffer = obj.Vertices;
		obj.VertexBufferIndexed = GpuApi::CreateBuffer( obj.NumVertices * sizeof( SCENE_VERTEX ), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );

		obj.Color = RedVector4(0.0f, 0.0f, 1.0f, 1.0f);
		obj.LocalMatrix = obj.LocalMatrix.IDENTITY;
		obj.LocalMatrix.SetTranslation(0.8f, 0.5f, 0.0f);
	}

	return true;
}

Bool CTestUtils::InitializeEffects()
{
#define INIT_EFFECT_VS_PS( effect, name )					         \
	{														         \
		m_effects[ effect ] = new CVertexPixelEffect( TXT( name ) ); \
	}

#define INIT_EFFECT_CS( effect, name )					             \
	{														         \
	    m_effects[ effect ] = new CComputeEffect( TXT( name ) );     \
    }

	m_effects.Grow( (Uint32)EffectType::ET_Max );
	INIT_EFFECT_VS_PS( ET_FlickeringIllusion,		"opticalIllusion" );
	INIT_EFFECT_VS_PS( ET_SimpleTriangle,			"simpleTriangle" );
	INIT_EFFECT_VS_PS( ET_SimplePhong,				"simplePhongGlobal" );
	INIT_EFFECT_VS_PS( ET_SimplePhongRaw,			"simplePhongRaw" );
	INIT_EFFECT_VS_PS( ET_SimplePhongInstanced,		"instanced" );
	INIT_EFFECT_VS_PS( ET_SimpleTriangleInstanced,	"simpleTriangleInstanced" );
	INIT_EFFECT_VS_PS( ET_SimpleTexture,			"samplerTex" );
	INIT_EFFECT_VS_PS( ET_SimpleDraw,				"simpleDraw" );
#ifndef RED_PLATFORM_ORBIS
	//INIT_EFFECT_VS_PS( ET_SimpleDrawCsVs,			"simpleDrawCsVs" );

	//INIT_EFFECT_CS( ET_SimpleCull,					"simpleCull" );
#endif
	INIT_EFFECT_VS_PS( ET_ColoredTriangleGrid,		"coloredTriangleGrid" );

	for( IEffect* e : m_effects )
	{
		if( e && !e->Initialize() )
		{
			ERR_GAT( TXT( "Failed to initialize effects!" ) );
			return false;
		}
	}

	return true;
}
