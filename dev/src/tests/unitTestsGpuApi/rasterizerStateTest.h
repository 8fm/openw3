/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RASTERIZER_STATE_TEST_H_
#define _RASTERIZER_STATE_TEST_H_

#include "testEngine.h"

class CRasterizerStateTest : public CBaseTest
{
public:
	CRasterizerStateTest( String& name )
		: CBaseTest( name )
	{

	}
	virtual ~CRasterizerStateTest()
	{
		GpuApi::SafeRelease( m_vertexBuffer );
		GpuApi::SafeRelease( m_indexBuffer );
		GpuApi::SafeRelease( m_bgVertexBuffer );
	}

	virtual Bool Initialize( )
	{
		// Create vertex buffer
		typedef struct Vertex
		{
			Float x, y, z;		// Position
			Uint8  r, g, b, a;	// Color
		} Vertex;
		m_stride = sizeof( Vertex );
		m_offset = 0;

		static const Vertex vertexData[] =
		{
			//   POSITION                COLOR

			// CCW
			{-0.75f, -0.75f, -0.75f,    255, 0, 0, 255 },
			{ 0.00f, -0.75f, -0.75f,    255, 0, 0, 255 },
			{-0.75f,  0.75f,  0.75f,    255, 0, 0, 255 },
			{ 0.00f,  0.75f,  0.75f,    255, 0, 0, 255 },

			{-0.75f, -0.75f,  0.75f,    0, 255, 0, 255 },
			{ 0.00f, -0.75f,  0.75f,    0, 255, 0, 255 },
			{-0.75f,  0.75f, -0.75f,    0, 255, 0, 255 },
			{ 0.00f,  0.75f, -0.75f,    0, 255, 0, 255 },

			// CW
			{ 0.00f, -0.75f, -0.75f,    255, 0, 255, 255 },
			{ 0.00f,  0.75f,  0.75f,    255, 0, 255, 255 },
			{ 0.75f, -0.75f, -0.75f,    255, 0, 255, 255 },
			{ 0.75f,  0.75f,  0.75f,    255, 0, 255, 255 },

			{ 0.00f, -0.75f,  0.75f,    0, 255, 255, 255 },
			{ 0.00f,  0.75f, -0.75f,    0, 255, 255, 255 },
			{ 0.75f, -0.75f,  0.75f,    0, 255, 255, 255 },
			{ 0.75f,  0.75f, -0.75f,    0, 255, 255, 255 },

			// CCW z=0
			{-1.0f, -0.6f, 0.0f,    255, 0, 255, 255 },
			{ 0.0f, -0.6f, 0.0f,    255, 0, 255, 255 },
			{-1.0f,  0.6f, 0.0f,    255, 0, 255, 255 },
			{ 0.0f,  0.6f, 0.0f,    255, 0, 255, 255 },
			// CW z=0
			{ 0.0f, -0.6f, 0.0f,    255, 255, 255, 255 },
			{ 1.0f, -0.6f, 0.0f,    255, 255, 255, 255 },
			{ 0.0f,  0.6f, 0.0f,    255, 255, 255, 255 },
			{ 1.0f,  0.6f, 0.0f,    255, 255, 255, 255 },
		};

		{
			GpuApi::BufferInitData bufInitData;
			bufInitData.m_buffer = vertexData;
			bufInitData.m_elementCount = 0;
			m_vertexBuffer = GpuApi::CreateBuffer( sizeof( vertexData ), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );
		}


		{
			// Create index buffer
			static const Uint16 indexData[] =
			{
				0, 1, 2,
				1, 3, 2,
				4, 5, 6,
				5, 7, 6,

				8, 9, 10,
				9, 11, 10,
				12, 13, 14,
				13, 15, 14,

				16, 17, 18,
				17, 19, 18,
				21, 20, 22,
				23, 21, 22
			};

			GpuApi::BufferInitData bufInitData;
			bufInitData.m_buffer = indexData;
			bufInitData.m_elementCount = 0;
			m_indexBuffer = GpuApi::CreateBuffer( sizeof( indexData ), GpuApi::BCC_Index16Bit, GpuApi::BUT_Immutable, 0, &bufInitData );
		}


		static const Vertex bgVertexData[] =
		{
			//   POSITION                COLOR
			{-30.0f, -30.0f, 0.0f,    63, 63, 63, 255 },
			{ 30.0f, -30.0f, 0.0f,    63, 63, 63, 255 },
			{-30.0f,  30.0f, 0.0f,    63, 63, 63, 255 },

			{-30.0f,  30.0f, 0.0f,    63, 63, 63, 255 },
			{ 30.0f, -30.0f, 0.0f,    63, 63, 63, 255 },
			{ 30.0f,  30.0f, 0.0f,    63, 63, 63, 255 },
		};

		{
			GpuApi::BufferInitData bufInitData;
			bufInitData.m_buffer = bgVertexData;
			bufInitData.m_elementCount = 0;
			m_bgVertexBuffer = GpuApi::CreateBuffer( 6 * sizeof( Vertex ), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );
		}

		return true;
	}

	virtual Bool Execute( CTestUtils& context )
	{
		GpuApi::BindMainConstantBuffers();

		context.GetEffect( ET_SimpleDraw )->SetShaders();

		RedMatrix4x4 lookAt = LookAtLH(RedVector3(0, 0, -20), RedVector3(0, 0, 0.0f), RedVector3(0,1,0));
		RedMatrix4x4 proj = RedMath::SIMD::BuildPerspectiveLH( DEG2RAD(45.f), 16.0f / 9.0f, 0.1f, 100.0f );


		GpuApi::BufferRef constantBufferRef = GpuApi::CreateBuffer(sizeof(CBSimpleDraw), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite, nullptr );


		// Draw background plane
		{
			CBSimpleDraw* buf = (CBSimpleDraw*)GpuApi::LockBuffer( constantBufferRef, GpuApi::BLF_Discard, 0, sizeof( CBSimpleDraw ) );
			buf->m_localToWorld = RedMatrix4x4::IDENTITY;
			buf->m_worldToView = lookAt;
			buf->m_viewToScreen = proj;
			GpuApi::UnlockBuffer(constantBufferRef );


			GpuApi::SetCustomDrawContext( GpuApi::DSSM_NoStencilFullDepthLE, GpuApi::RASTERIZERMODE_DefaultNoCull, GpuApi::BLENDMODE_Set );

			GpuApi::BindConstantBuffer(0, constantBufferRef, GpuApi::VertexShader);

			GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemPosColor);
			GpuApi::BindVertexBuffers( 0, 1, &m_bgVertexBuffer, &m_stride, &m_offset );

			GpuApi::DrawPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 2 );
		}

		GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemPosColor);
		GpuApi::BindVertexBuffers( 0, 1, &m_vertexBuffer, &m_stride, &m_offset );
		GpuApi::BindIndexBuffer( m_indexBuffer, m_offset );

		GpuApi::SetupShadowDepthBias( 0.0f, 2.0f );


		static_assert( GpuApi::RASTERIZERMODE_Max <= 32, "Probably want to change the arrangement of these..." );
		for ( Uint32 i = 0; i < GpuApi::RASTERIZERMODE_Max; ++i )
		{
			GpuApi::ERasterizerMode mode = (GpuApi::ERasterizerMode)i;

			// Use Less instead of LE, to be able to check depth bias.
			GpuApi::SetCustomDrawContext( GpuApi::DSSM_NoStencilFullDepthLess, mode, GpuApi::BLENDMODE_Set );

			Uint32 idx = i;

			// Put a gap in the indexes, so that the wireframe versions line up with the non-wireframe ones.
			if ( mode >= GpuApi::RASTERIZERMODE_WireframeOffset )
			{
				const Uint32 idxForFirstWireframe = ( ( GpuApi::RASTERIZERMODE_WireframeOffset - 1 ) | 7 ) + 1;
				idx += idxForFirstWireframe - GpuApi::RASTERIZERMODE_WireframeOffset;
			}

			//World mtx
			RedMatrix4x4 world = world.IDENTITY;
			Uint32 x = idx % 8;
			Uint32 y = idx / 8;
			world.SetTranslation( RedVector4( 3.0f * ( x - 3.5f ), -3.0f * ( y - 1.5f ), 0.0f, 1.0f ) );


			CBSimpleDraw* buf = (CBSimpleDraw*)GpuApi::LockBuffer( constantBufferRef, GpuApi::BLF_Discard, 0, sizeof( CBSimpleDraw ) );
			buf->m_localToWorld = world;
			buf->m_worldToView = lookAt;
			buf->m_viewToScreen = proj;
			GpuApi::UnlockBuffer( constantBufferRef );

			GpuApi::BindConstantBuffer(0, constantBufferRef, GpuApi::VertexShader);

			GpuApi::DrawIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, 24, 0, 12 );

		}

		GpuApi::SafeRelease( constantBufferRef );

		return true;
	}

private:
	GpuApi::BufferRef m_vertexBuffer;
	GpuApi::BufferRef m_indexBuffer;

	GpuApi::BufferRef m_bgVertexBuffer;
};

#endif
