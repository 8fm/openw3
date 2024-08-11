/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _VIEWPORT_TEST_H_
#define _VIEWPORT_TEST_H_

#include "testEngine.h"

class CViewportTest : public CBaseTest
{
public:
	CViewportTest( String& name )
		: CBaseTest( name )
	{
		GpuApi::SafeRelease( g_constants );
	}
	virtual ~CViewportTest(){};

	virtual Bool Initialize( )
	{ 
		g_constants = GpuApi::CreateBuffer( sizeof( CBSimpleTriangle ), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		if ( !g_constants )
		{
			ERR_GAT( TXT( "[%s] Couldn't create constant buffer!" ), m_testName.AsChar() );
			return false;
		}

		return true; 
	};

	virtual Bool Execute( CTestUtils& context )
	{
		// Update constants
		{
			void* lockedConstantsData = GpuApi::LockBuffer( g_constants, GpuApi::BLF_Discard, 0, sizeof( CBSimpleTriangle ) );
			RED_ASSERT( lockedConstantsData );
			CBSimpleTriangle* lockedConstants = static_cast< CBSimpleTriangle* >( lockedConstantsData );

			lockedConstants->m_colors[0] = RedVector4( 1.0f, 0.0f, 0.0f, 1.0f );
			lockedConstants->m_colors[1] = RedVector4( 0.0f, 1.0f, 0.0f, 1.0f );
			lockedConstants->m_colors[2] = RedVector4( 0.0f, 0.0f, 1.0f, 1.0f );

			GpuApi::UnlockBuffer( g_constants );
		}

		GpuApi::BindConstantBuffer( 0, g_constants, GpuApi::VertexShader );

		context.GetEffect(ET_SimpleTriangle)->SetShaders();

		GpuApi::TextureDesc tdesc = GpuApi::GetTextureDesc(GpuApi::GetBackBufferTexture());

		GpuApi::ViewportDesc desc;
		desc.width = tdesc.width / 2;
		desc.height = tdesc.height / 2;

		for( Uint32 i = 0; i < 4; ++i )
		{
			desc.x = desc.width * (i % 2);
			desc.y = desc.height * (i / 2);
			GpuApi::SetViewport(desc);

			GpuApi::DrawPrimitiveNoBuffers( GpuApi::PRIMTYPE_TriangleList, 1 );
		}

		return true;
	}

private:
	static GpuApi::BufferRef		g_constants;
};

GpuApi::BufferRef CViewportTest::g_constants = GpuApi::BufferRef::Null();

#endif //_VIEWPORT_TEST_H_