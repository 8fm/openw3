/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#ifndef _BLEND_STATE_TEST_H_
#define _BLEND_STATE_TEST_H_

#include "testEngine.h"

class CBlendStateTest : public CBaseTest
{
public:
	struct Vertex
	{
		Float x, y, z;		// Position
		Uint8  r, g, b, a;	// Color
	};

	struct CopyVertex
	{
		Float x, y, z;
		Uint8 r, g, b, a;
		Float u, v;
	};

public:
	CBlendStateTest( String& name )
		: CBaseTest( name )
	{

	}
	virtual ~CBlendStateTest()
	{
		GpuApi::SafeRelease( m_vertexBuffer );
		GpuApi::SafeRelease( m_bgVertexBuffer );
		GpuApi::SafeRelease( m_copyVertexBuffer );
		GpuApi::SafeRelease( m_constants );

		for ( Uint32 i = 0; i < 4; ++i )
		{
			GpuApi::SafeRelease( m_rts[ i ] );
		}
	}

	virtual Bool Initialize( )
	{
		// Create vertex buffer
		m_stride = sizeof( Vertex );
		m_offset = 0;

		{
			static const Vertex vertexData[] =
			{
				//   POSITION                COLOR
				{-0.75f, -0.75f, 0.0f,    255,   0,   0, 255 },
				{ 0.75f, -0.75f, 0.0f,      0, 255,   0, 255 },
				{-0.75f,  0.75f, 0.0f,      0,   0, 255, 255 },
				{ 0.75f,  0.75f, 0.0f,    255, 255, 255,   0 }
			};

			GpuApi::BufferInitData bufInitData;
			bufInitData.m_buffer = vertexData;
			bufInitData.m_elementCount = 0;
			m_vertexBuffer = GpuApi::CreateBuffer( 4 * sizeof( Vertex ), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );
		}


		{
			static const Vertex bgVertexData[] =
			{
				//   POSITION                COLOR
				{-1.0f, -1.0f, 0.0f,    127, 127, 127, 127 },
				{ 1.0f, -1.0f, 0.0f,    127, 127, 127, 127 },
				{-1.0f,  1.0f, 0.0f,    127, 127, 127, 127 },
				{ 1.0f,  1.0f, 0.0f,    127, 127, 127, 127 }
			};

			GpuApi::BufferInitData bufInitData;
			bufInitData.m_buffer = bgVertexData;
			bufInitData.m_elementCount = 0;
			m_bgVertexBuffer = GpuApi::CreateBuffer( 4 * sizeof( Vertex ), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );
		}


		{
			static const CopyVertex copyVertexData[] =
			{
				//   POSITION                COLOR                UV
				{-1.0f, -1.0f, 0.0f,   255, 255, 255, 255,    0.0f, 1.0f },
				{ 1.0f, -1.0f, 0.0f,   255, 255, 255, 255,    1.0f, 1.0f },
				{-1.0f,  1.0f, 0.0f,   255, 255, 255, 255,    0.0f, 0.0f },
				{ 1.0f,  1.0f, 0.0f,   255, 255, 255, 255,    1.0f, 0.0f }
			};

			GpuApi::BufferInitData bufInitData;
			bufInitData.m_buffer = copyVertexData;
			bufInitData.m_elementCount = 0;
			m_copyVertexBuffer = GpuApi::CreateBuffer( 4 * sizeof( CopyVertex ), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );
		}


		{
			CBSimpleDraw consts;
			consts.m_localToWorld = RedMatrix4x4::IDENTITY;
			consts.m_worldToView = RedMatrix4x4::IDENTITY;
			consts.m_viewToScreen = RedMatrix4x4::IDENTITY;

			GpuApi::BufferInitData bufInitData;
			bufInitData.m_buffer = &consts;
			bufInitData.m_elementCount = 0;
			m_constants = GpuApi::CreateBuffer(sizeof(CBSimpleDraw), GpuApi::BCC_Constant, GpuApi::BUT_Default, 0, &bufInitData );
		}


		{
			CBSimpleTexture copyConsts;
			copyConsts.m_localToWorld = RedMatrix4x4::IDENTITY;
			copyConsts.m_worldToView = RedMatrix4x4::IDENTITY;
			copyConsts.m_viewToScreen = RedMatrix4x4::IDENTITY;
			copyConsts.m_uvScale = 1.0f;

			GpuApi::BufferInitData bufInitData;
			bufInitData.m_buffer = &copyConsts;
			bufInitData.m_elementCount = 0;
			m_copyConstants = GpuApi::CreateBuffer(sizeof(CBSimpleTexture), GpuApi::BCC_Constant, GpuApi::BUT_Default, 0, &bufInitData );
		}


		const GpuApi::TextureDesc& bbDesc = GpuApi::GetTextureDesc( GpuApi::GetBackBufferTexture() );
		m_rtWidth = bbDesc.width / 2;
		m_rtHeight = bbDesc.height / 2;

		GpuApi::TextureDesc rtDesc;
		rtDesc.type = GpuApi::TEXTYPE_2D;
		rtDesc.width = m_rtWidth;
		rtDesc.height = m_rtHeight;
		rtDesc.initLevels = 1;
		rtDesc.sliceNum = 1;
		rtDesc.usage = GpuApi::TEXUSAGE_RenderTarget | GpuApi::TEXUSAGE_Samplable;
		rtDesc.format = GpuApi::TEXFMT_R8G8B8A8;
		for ( Uint32 i = 0; i < 4; ++i )
		{
			m_rts[ i ] = GpuApi::CreateTexture( rtDesc, GpuApi::TEXG_Generic, nullptr );
		}

		return true;
	}

	virtual Bool Execute( CTestUtils& context )
	{

		const Uint32 vertexStride = sizeof( Vertex );
		const Uint32 copyVertexStride = sizeof( CopyVertex );

		GpuApi::BindMainConstantBuffers();


		GpuApi::RenderTargetSetup oldRT = GpuApi::GetRenderTargetSetup();

		GpuApi::RenderTargetSetup rt;
		rt.SetColorTarget( 0, m_rts[ 0 ] );
		rt.SetColorTarget( 1, m_rts[ 1 ] );
		rt.SetColorTarget( 2, m_rts[ 2 ] );
		rt.SetColorTarget( 3, m_rts[ 3 ] );
		rt.SetViewportFromTarget( m_rts[ 0 ] );
		GpuApi::SetupRenderTargets( rt );


		context.GetEffect( ET_SimpleDraw )->SetShaders();

		GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemPosColor);

		GpuApi::BindConstantBuffer( 0, m_constants, GpuApi::VertexShader );

		static_assert( GpuApi::BLENDMODE_Max <= 81, "Probably want to change the arrangement of these..." );

		GpuApi::ViewportDesc vp;
		vp.Set( m_rtWidth / 9 - 1, m_rtHeight / 9 - 1 );

		for ( Uint32 i = 0; i < GpuApi::BLENDMODE_Max; ++i )
		{
			GpuApi::EBlendMode mode = (GpuApi::EBlendMode)i;

			Uint32 x = i % 9;
			Uint32 y = i / 9;
			vp.x = x * ( vp.width + 1 );
			vp.y = y * ( vp.height + 1 );
			GpuApi::SetViewport( vp );


			// Draw background plane
			GpuApi::SetDrawContext( GpuApi::DRAWCONTEXT_SimpleNoCull, 0 );
			GpuApi::BindVertexBuffers( 0, 1, &m_bgVertexBuffer, &vertexStride, &m_offset );
			GpuApi::DrawPrimitive( GpuApi::PRIMTYPE_TriangleStrip, 0, 2 );


			// Draw the blended shape
			GpuApi::SetCustomDrawContext( GpuApi::DSSM_NoStencilNoDepth, GpuApi::RASTERIZERMODE_DefaultNoCull, mode );
			GpuApi::BindVertexBuffers( 0, 1, &m_vertexBuffer, &vertexStride, &m_offset );
			GpuApi::DrawPrimitive( GpuApi::PRIMTYPE_TriangleStrip, 0, 2 );
		}


		GpuApi::SetupRenderTargets( oldRT );


		// Copy RTs to main render target

		context.GetEffect( ET_SimpleTexture )->SetShaders();
		GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemPosColorUV);
		GpuApi::BindConstantBuffer( 0, m_copyConstants, GpuApi::VertexShader );
		GpuApi::SetDrawContext( GpuApi::DRAWCONTEXT_SimpleNoCull, 0 );
		GpuApi::BindVertexBuffers( 0, 1, &m_copyVertexBuffer, &copyVertexStride, &m_offset );
		vp.Set( m_rtWidth, m_rtHeight );
		for ( Uint32 i = 0; i < 4; ++i )
		{
			Uint32 x = i % 2;
			Uint32 y = i / 2;
			vp.x = x * vp.width;
			vp.y = y * vp.height;
			GpuApi::SetViewport( vp );

			GpuApi::BindTextures( 0, 1, &m_rts[ i ], GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );
			GpuApi::DrawPrimitive( GpuApi::PRIMTYPE_TriangleStrip, 0, 2 );
		}


		return true;
	}

private:

	GpuApi::TextureRef m_rts[ 4 ];
	Uint32 m_rtWidth;
	Uint32 m_rtHeight;

	GpuApi::BufferRef m_vertexBuffer;
	GpuApi::BufferRef m_bgVertexBuffer;

	GpuApi::BufferRef m_copyVertexBuffer;

	GpuApi::BufferRef m_constants;
	GpuApi::BufferRef m_copyConstants;
};

#endif
