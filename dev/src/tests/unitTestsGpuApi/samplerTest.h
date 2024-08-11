//
// Copyright © 2014 CD Projekt Red. All Rights Reserved.
//	

#ifndef _SAMPLER_TEST_H_
#define _SAMPLER_TEST_H_

#include "testEngine.h"

class CSamplerTest : public CBaseTest
{
public:
	CSamplerTest( String& name )
		: CBaseTest( name )
	{

	}
	virtual ~CSamplerTest()
	{
		GpuApi::SafeRelease( m_vertexBuffer );
		GpuApi::SafeRelease( m_indexBuffer );
	}

	virtual Bool Initialize( )
	{
		// Create vertex buffer
		typedef struct Vertex
		{
			Float x, y, z;		// Position
			Uint8  r, g, b, a;	// Color
			Float u, v;			// UVs
		} Vertex;

		static const Vertex vertexData[] =
		{
			// 2    3
			// +----+
			// |\   |
			// | \  |
			// |  \ |
			// |   \|
			// +----+
			// 0    1

			//   POSITION                COLOR               UV
			{-0.75f, -1.5f, 0.0f,    255, 0,   0,   1,    0.0f, 1.0f},
			{ 0.75f, -1.5f, 0.0f,    0,	  255, 0,   1,    1.0f, 1.0f},
			{-0.75f,  1.5f, 0.0f,    0,   0,   255, 1,    0.0f, 0.0f},
			{ 0.75f,  1.5f, 0.0f,    100, 0,   100, 1,    1.0f, 0.0f},
		};

		{
			GpuApi::BufferInitData bufInitData;
			bufInitData.m_buffer = vertexData;
			bufInitData.m_elementCount = 0;
			m_vertexBuffer = GpuApi::CreateBuffer( 4 * sizeof( Vertex ), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );
		}

		

		{
			// Create index buffer
			static const Uint16 indexData[] =
			{
				0, 1, 2,
				1, 3, 2
			};

			GpuApi::BufferInitData bufInitData;
			bufInitData.m_buffer = indexData;
			bufInitData.m_elementCount = 0;
			m_indexBuffer = GpuApi::CreateBuffer( 6 * sizeof( Uint16 ), GpuApi::BCC_Index16Bit, GpuApi::BUT_Immutable, 0, &bufInitData );
		}

		Char path[128];
		Red::System::SNPrintF( path, ARRAY_COUNT( path ), TXT( "%ls/TILES.dds" ), SHADERS_PATH );

		Red::IO::CNativeFileHandle file;
		if ( !file.Open( path, Red::IO::eOpenFlag_Read ) )
		{
			RED_HALT( "Missing texture file.'" );
			return false;
		}

		GpuApi::Uint64 siTextureFileSize = file.GetFileSize();
		if (siTextureFileSize > 0)
		{
			Uint8* pTextureBuffer = new Uint8[siTextureFileSize];
			Uint32 readBytes;
			file.Read( pTextureBuffer, static_cast< GpuApi::Uint32 >( siTextureFileSize ), readBytes );
			if ( !pTextureBuffer )
			{
				ERR_GAT( TXT( "[%s] Couldn't read data of the texture file!" ), m_testName.AsChar() );
				return false;
			}

			m_texture = GpuApi::CreateTextureFromMemoryFile( pTextureBuffer, static_cast< GpuApi::Uint32 >( siTextureFileSize ), GpuApi::TEXG_Generic );
			if( !m_texture )
			{
				ERR_GAT( TXT( "Couldn't create texture out of the dds file!" ), m_testName.AsChar() );
				return false;
			}
			delete[] pTextureBuffer;
		}

		return true;
	}

	virtual Bool Execute( CTestUtils& context )
	{
		//World mtx
		RedMatrix4x4 world = world.IDENTITY;
		world.SetRotX( M_PI / 2.0f - 0.3f );

		// View matrix
		RedMatrix4x4 lookAt = LookAtLH(RedVector3(0, 0, -2), RedVector3(0, 0, 0.0f), RedVector3(0,1,0));

		// Proj matrix
		RedMatrix4x4 proj = RedMath::SIMD::BuildPerspectiveLH( DEG2RAD(45.f), 16.0f / 9.0f, 0.1f, 100.0f );

		CBSimpleTexture buf;
		buf.m_worldToView = lookAt;
		buf.m_viewToScreen = proj;
		buf.m_localToWorld = world;
		buf.m_uvScale = 8.0f;

		GpuApi::BufferInitData bufInitData;
		bufInitData.m_elementCount = 1;
		bufInitData.m_buffer = &buf;
		GpuApi::BufferRef constantBufferRef = GpuApi::CreateBuffer(sizeof(CBSimpleTexture), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite, &bufInitData );
		GpuApi::BindConstantBuffer(0, constantBufferRef, GpuApi::VertexShader);

		GpuApi::SetDrawContext( GpuApi::DRAWCONTEXT_SimpleNoCull, 1 );

		context.GetEffect(ET_SimpleTexture)->SetShaders();

		// set sampler
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_WrapAnisoMip, GpuApi::PixelShader );

		// set texture
		GpuApi::BindTextures( 0, 1, &m_texture, GpuApi::PixelShader );

		GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemPosColorUV);
		GpuApi::BindVertexBuffers( 0, 1, &m_vertexBuffer, &m_stride, &m_offset );
		GpuApi::BindIndexBuffer( m_indexBuffer, m_offset );

		GpuApi::DrawIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, 4, 0, 2 );

		GpuApi::SafeRelease( constantBufferRef );

		return true;
	}

private:
	GpuApi::BufferRef m_vertexBuffer;
	GpuApi::BufferRef m_indexBuffer;
	GpuApi::TextureRef m_texture;
};

#endif