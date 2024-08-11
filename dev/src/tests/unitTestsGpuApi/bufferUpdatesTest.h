//
// Copyright © 2014 CD Projekt Red. All Rights Reserved.
//	

#ifndef _BUFFER_UPDATE_TEST_H_
#define _BUFFER_UPDATE_TEST_H_

#include "testEngine.h"

class CBufferUpdatesTest : public CBaseTest
{
public:
	CBufferUpdatesTest( String& name )
		: CBaseTest( name )
	{
	}

	virtual ~CBufferUpdatesTest()
	{
		GpuApi::SafeRelease( g_defaultConstants );
		GpuApi::SafeRelease( g_dynamicConstants );
	};

	virtual Bool Initialize( )
	{ 
		g_defaultConstants = GpuApi::CreateBuffer( sizeof( CBSimpleTriangle ), GpuApi::BCC_Constant, GpuApi::BUT_Default, GpuApi::BAF_CPUWrite );
		if ( !g_defaultConstants )
		{
			ERR_GAT( TXT( "[%s] Couldn't create constant buffer!" ), m_testName.AsChar() );
			return false;
		}

		g_dynamicConstants = GpuApi::CreateBuffer( sizeof( CBSimpleTriangle ), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		if ( !g_dynamicConstants )
		{
			ERR_GAT( TXT( "[%s] Couldn't create constant buffer!" ), m_testName.AsChar() );
			return false;
		}

		return true; 
	};

	virtual Bool Execute( CTestUtils& context )
	{
		GpuApi::SetCustomDrawContext( GpuApi::DSSM_NoStencilFullDepthLE, GpuApi::RASTERIZERMODE_DefaultCullCCW, GpuApi::BLENDMODE_Set );

		context.GetEffect(ET_SimpleTriangle)->SetShaders();

		GpuApi::TextureDesc tdesc = GpuApi::GetTextureDesc(GpuApi::GetBackBufferTexture());

		GpuApi::ViewportDesc desc;
		desc.width = tdesc.width / 2;
		desc.height = tdesc.height;

		{
			void* lockedConstantsData = GpuApi::LockBuffer( g_defaultConstants, GpuApi::BLF_Discard, 0, sizeof( CBSimpleTriangle ) );
			RED_ASSERT( lockedConstantsData );
			CBSimpleTriangle* lockedConstants = static_cast< CBSimpleTriangle* >( lockedConstantsData );

			lockedConstants->m_colors[0] = RedVector4(1.0f, 0.0f, 0.0f, 1.0f);
			lockedConstants->m_colors[1] = RedVector4(0.0f, 1.0f, 0.0f, 1.0f);
			lockedConstants->m_colors[2] = RedVector4(0.0f, 0.0f, 1.0f, 1.0f);

			GpuApi::UnlockBuffer( g_defaultConstants );

			GpuApi::BindConstantBuffer( 0, g_defaultConstants, GpuApi::VertexShader );

			desc.x = 0;
			desc.y = 0;
			GpuApi::SetViewport(desc);

			GpuApi::DrawPrimitiveNoBuffers( GpuApi::PRIMTYPE_TriangleList, 1 );
		}

		{
			void* lockedConstantsData = GpuApi::LockBuffer( g_dynamicConstants, GpuApi::BLF_Discard, 0, sizeof( CBSimpleTriangle ) );
			RED_ASSERT( lockedConstantsData );
			CBSimpleTriangle* lockedConstants = static_cast< CBSimpleTriangle* >( lockedConstantsData );

			lockedConstants->m_colors[0] = RedVector4(1.0f, 0.0f, 0.0f, 1.0f);
			lockedConstants->m_colors[1] = RedVector4(0.0f, 1.0f, 0.0f, 1.0f);
			lockedConstants->m_colors[2] = RedVector4(0.0f, 0.0f, 1.0f, 1.0f);

			GpuApi::UnlockBuffer( g_dynamicConstants );

			GpuApi::BindConstantBuffer( 0, g_dynamicConstants, GpuApi::VertexShader );

			desc.x = desc.width;
			desc.y = 0;
			GpuApi::SetViewport(desc);

			GpuApi::DrawPrimitiveNoBuffers( GpuApi::PRIMTYPE_TriangleList, 1 );
		}

		return true;
	}

private:
	static GpuApi::BufferRef		g_defaultConstants;
	static GpuApi::BufferRef		g_dynamicConstants;
};

GpuApi::BufferRef CBufferUpdatesTest::g_defaultConstants = GpuApi::BufferRef::Null();
GpuApi::BufferRef CBufferUpdatesTest::g_dynamicConstants = GpuApi::BufferRef::Null();

#endif