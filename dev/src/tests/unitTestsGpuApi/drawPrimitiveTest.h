//
// Copyright © 2014 CD Projekt Red. All Rights Reserved.
//	

#ifndef _DRAW_PRIMITIVE_TEST_H_
#define _DRAW_PRIMITIVE_TEST_H_

#include "testEngine.h"

class CDrawPrimitiveTest : public CBaseTest
{
public:
	CDrawPrimitiveTest( String& name )
		: CBaseTest( name )
	{

	}
	virtual ~CDrawPrimitiveTest()
	{
		GpuApi::SafeRelease( g_constantsFrame );
		GpuApi::SafeRelease( g_constantsVSObject );
		GpuApi::SafeRelease( g_constantsPSObject );
	};

	virtual Bool Initialize( )
	{
		g_constantsFrame = GpuApi::CreateBuffer( sizeof( CBSimplePhongVS0 ), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		if ( !g_constantsFrame )
		{
			ERR_GAT( TXT( "[%s] Couldn't create frame-constant buffer!" ), m_testName.AsChar() );
			return false;
		}

		g_constantsVSObject = GpuApi::CreateBuffer( sizeof( CBSimplePhongVS1 ), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		if ( !g_constantsVSObject )
		{
			ERR_GAT( TXT( "[%s] Couldn't create object-constant buffer!" ), m_testName.AsChar() );
			return false;
		}

		g_constantsPSObject = GpuApi::CreateBuffer( sizeof( CBSimplePhongPS0 ), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		if ( !g_constantsPSObject )
		{
			ERR_GAT( TXT( "[%s] Couldn't create object-constant buffer!" ), m_testName.AsChar() );
			return false;
		}

		return true;
	}

	virtual Bool Execute( CTestUtils& context )
	{
		// View matrix
		RedMatrix4x4 lookAt = LookAtLH(RedVector3(0, 1, -2), RedVector3(0, 0, 0.5f), RedVector3(0,1,0));

		// Proj matrix
		RedMatrix4x4 proj = RedMath::SIMD::BuildPerspectiveLH( DEG2RAD(45.f), 16.0f / 9.0f, 0.1f, 100.0f );

		//LightPos
		RedVector4 lightPos(0, 1, 0, 1);

		// Update constants
		{
			void* lockedConstantsData = GpuApi::LockBuffer( g_constantsFrame, GpuApi::BLF_Discard, 0, sizeof( CBSimplePhongVS0 ) );
			RED_ASSERT( lockedConstantsData );
			CBSimplePhongVS0* lockedConstants = static_cast< CBSimplePhongVS0* >( lockedConstantsData );

			lockedConstants->m_worldToView = lookAt;
			lockedConstants->m_viewToScreen = proj;
			lockedConstants->m_lightPos = lightPos;

			GpuApi::UnlockBuffer( g_constantsFrame );
		}
		GpuApi::BindConstantBuffer( 0, g_constantsFrame, GpuApi::VertexShader );

		GpuApi::SetCustomDrawContext( GpuApi::DSSM_NoStencilFullDepthLE, GpuApi::RASTERIZERMODE_DefaultCullCCW, GpuApi::BLENDMODE_Set );

		context.GetEffect(ET_SimplePhong)->SetShaders();

		for( Uint32 i = 0; i < context.GetObjects().Size(); ++i )
		{
			// Update constants
			{
				void* lockedConstantsData = GpuApi::LockBuffer( g_constantsVSObject, GpuApi::BLF_Discard, 0, sizeof( CBSimplePhongVS1 ) );
				RED_ASSERT( lockedConstantsData );
				CBSimplePhongVS1* lockedConstants = static_cast< CBSimplePhongVS1* >( lockedConstantsData );

				lockedConstants->m_localToWorld = context.GetObjects()[i].LocalMatrix.AsFloat();

				GpuApi::UnlockBuffer( g_constantsVSObject );
			}
			GpuApi::BindConstantBuffer( 1, g_constantsVSObject, GpuApi::VertexShader );

			// Update constants
			{
				void* lockedConstantsData = GpuApi::LockBuffer( g_constantsPSObject, GpuApi::BLF_Discard, 0, sizeof( CBSimplePhongPS0 ) );
				RED_ASSERT( lockedConstantsData );
				CBSimplePhongPS0* lockedConstants = static_cast< CBSimplePhongPS0* >( lockedConstantsData );

				lockedConstants->m_surfaceColor = context.GetObjects()[i].Color;

				GpuApi::UnlockBuffer( g_constantsPSObject );
			}
			GpuApi::BindConstantBuffer( 0, g_constantsPSObject, GpuApi::PixelShader );

			GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemPosNormal );
			GpuApi::BindVertexBuffers( 0, 1, &context.GetObjects()[i].VertexBufferNonIndexed, &m_stride, &m_offset );

			GpuApi::DrawPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, context.GetObjects()[i].NumIndices / 3 );
		}

		return true;
	}

	static GpuApi::BufferRef g_constantsFrame;
	static GpuApi::BufferRef g_constantsVSObject;
	static GpuApi::BufferRef g_constantsPSObject;
};

GpuApi::BufferRef CDrawPrimitiveTest::g_constantsFrame = GpuApi::BufferRef::Null();
GpuApi::BufferRef CDrawPrimitiveTest::g_constantsVSObject = GpuApi::BufferRef::Null();
GpuApi::BufferRef CDrawPrimitiveTest::g_constantsPSObject = GpuApi::BufferRef::Null();

#endif