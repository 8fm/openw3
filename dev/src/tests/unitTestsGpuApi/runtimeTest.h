/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RUNTIME_TEST_H_
#define _RUNTIME_TEST_H_

#include "testEngine.h"

#define VERTS_PER_EDGE 64

class CRuntimeTest : public CBaseTest
{
public:
	CRuntimeTest( String& name )
		: CBaseTest( name )
	{
	}

	virtual ~CRuntimeTest()
	{
		GpuApi::SafeRelease( m_object.IndexBuffer );
		GpuApi::SafeRelease( m_object.VertexBufferIndexed );
		GpuApi::SafeRelease( g_constantsFrame );
		GpuApi::SafeRelease( g_constantsObject );
	};

	virtual Bool Initialize( )
	{
		m_time = 0.5f;

		m_object.NumVertices = VERTS_PER_EDGE * VERTS_PER_EDGE;
		m_object.NumIndices = 6 * ( VERTS_PER_EDGE - 1 ) * ( VERTS_PER_EDGE - 1 );
		m_stride = sizeof( GpuApi::SystemVertex_Pos );

		// Create index buffer
		Uint16* pIndexData = new Uint16[ m_object.NumIndices ];
		Uint16* pIndices = pIndexData;
		for( Uint16 y = 1; y < VERTS_PER_EDGE; y++ )
		{
			for( Uint32 x = 1; x < VERTS_PER_EDGE; x++ )
			{
				*pIndices++ = ( Uint16 )( ( y - 1 ) * VERTS_PER_EDGE + ( x - 1 ) );
				*pIndices++ = ( Uint16 )( ( y - 0 ) * VERTS_PER_EDGE + ( x - 1 ) );
				*pIndices++ = ( Uint16 )( ( y - 1 ) * VERTS_PER_EDGE + ( x - 0 ) );

				*pIndices++ = ( Uint16 )( ( y - 1 ) * VERTS_PER_EDGE + ( x - 0 ) );
				*pIndices++ = ( Uint16 )( ( y - 0 ) * VERTS_PER_EDGE + ( x - 1 ) );
				*pIndices++ = ( Uint16 )( ( y - 0 ) * VERTS_PER_EDGE + ( x - 0 ) );
			}
		}

		// Create vertex buffer
		GpuApi::SystemVertex_Pos* pVertData = new GpuApi::SystemVertex_Pos[ m_object.NumVertices ];
		GpuApi::SystemVertex_Pos* pVertices = pVertData;
		int i = 0;
		for( Uint32 y = 0; y < VERTS_PER_EDGE; y++ )
		{
			for( Uint32 x = 0; x < VERTS_PER_EDGE; x++, ++i )
			{
				pVertices[i].x = (( float )x / ( float )( VERTS_PER_EDGE - 1 ) - 0.5f ) * M_PI;
				pVertices[i].y = (( float )y / ( float )( VERTS_PER_EDGE - 1 ) - 0.5f ) * M_PI;
				pVertices[i].z = 0;

			}
		}

		{
			GpuApi::BufferInitData bufInitData;
			bufInitData.m_buffer = pIndexData;
			m_object.IndexBuffer = GpuApi::CreateBuffer( m_object.NumIndices * sizeof( Uint16 ), GpuApi::BCC_Index16Bit, GpuApi::BUT_Immutable, 0, &bufInitData );
		}
		
		{
			GpuApi::BufferInitData bufInitData;
			bufInitData.m_buffer = pVertData;
			m_object.VertexBufferIndexed = GpuApi::CreateBuffer( m_object.NumVertices * sizeof( GpuApi::SystemVertex_Pos ), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );
		}
		
		delete [] pIndexData;
		delete [] pVertData;

		g_constantsFrame = GpuApi::CreateBuffer( sizeof( CBFlickeringIllusion0 ), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		if ( !g_constantsFrame )
		{
			ERR_GAT( TXT( "[%s] Couldn't create frame-constant buffer!" ), m_testName.AsChar() );
			return false;
		}

		g_constantsObject = GpuApi::CreateBuffer( sizeof( CBFlickeringIllusion1 ), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		if ( !g_constantsObject )
		{
			ERR_GAT( TXT( "[%s] Couldn't create object-constant buffer!" ), m_testName.AsChar() );
			return false;
		}

		return true;
	}

	virtual Bool Execute( CTestUtils& context )
	{
		m_time += 0.001;

		RedMatrix4x4 axesConversion( RedVector4( 1,0,0,0), RedVector4( 0,0,1,0), RedVector4( 0,1,0,0), RedVector4( 0,0,0,1 ) );
		RedMatrix4x4 rotMatrix = RedMatrix4x4::IDENTITY;
		RedMatrix4x4 transMatrix = RedMatrix4x4::IDENTITY;
		RedMatrix4x4 worldToCamera = Mul( transMatrix, rotMatrix );
		RedMatrix4x4 worldToView = Mul( worldToCamera, axesConversion );

		RedMatrix4x4 viewToScreen = RedMath::SIMD::BuildPerspectiveLH( DEG2RAD(70.f), 1.65426695f, 0.2f, 1900.f );

		RedMatrix4x4 worldToScreen = Mul( worldToView, viewToScreen );
		GpuApi::BindMainConstantBuffers();
		// Update constants
		{
			void* lockedConstantsData = GpuApi::LockBuffer( g_constantsFrame, GpuApi::BLF_Discard, 0, sizeof( CBFlickeringIllusion0 ) );
			RED_ASSERT( lockedConstantsData );
			CBFlickeringIllusion0* lockedConstants = static_cast< CBFlickeringIllusion0* >( lockedConstantsData );

			lockedConstants->m_worldToScreen = worldToScreen.AsFloat();
			lockedConstants->m_time = m_time;

			GpuApi::UnlockBuffer( g_constantsFrame );
		}
		GpuApi::BindConstantBuffer( 0, g_constantsFrame, GpuApi::VertexShader );

		context.GetEffect(ET_FlickeringIllusion)->SetShaders();

		// Set Context
		GpuApi::SetCustomDrawContext( GpuApi::DSSM_NoStencilFullDepthLE, GpuApi::RASTERIZERMODE_DefaultNoCull, GpuApi::BLENDMODE_Set );

		GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemPos );
		GpuApi::BindVertexBuffers(0, 1, &m_object.VertexBufferIndexed, &m_stride, &m_offset);
		GpuApi::BindIndexBuffer( m_object.IndexBuffer );

		RedVector4 objectPos(0, 13, 0, 0);
		// Update constants
		{
			void* lockedConstantsData = GpuApi::LockBuffer( g_constantsObject, GpuApi::BLF_Discard, 0, sizeof( CBFlickeringIllusion1 ) );
			RED_ASSERT( lockedConstantsData );
			CBFlickeringIllusion1* lockedConstants = static_cast< CBFlickeringIllusion1* >( lockedConstantsData );

			lockedConstants->m_objectPosition = objectPos;

			GpuApi::UnlockBuffer( g_constantsObject );
		}
		GpuApi::BindConstantBuffer( 1, g_constantsObject, GpuApi::VertexShader );

		GpuApi::SetDrawContext(GpuApi::DRAWCONTEXT_Default, 0);
		GpuApi::DrawIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, m_object.NumVertices, 0, m_object.NumIndices / 3 );

		objectPos.Y -= 10;
		// Update constants
		{
			void* lockedConstantsData = GpuApi::LockBuffer( g_constantsObject, GpuApi::BLF_Discard, 0, sizeof( CBFlickeringIllusion1 ) );
			RED_ASSERT( lockedConstantsData );
			CBFlickeringIllusion1* lockedConstants = static_cast< CBFlickeringIllusion1* >( lockedConstantsData );

			lockedConstants->m_objectPosition = objectPos;

			GpuApi::UnlockBuffer( g_constantsObject );
		}
		GpuApi::BindConstantBuffer( 1, g_constantsObject, GpuApi::VertexShader );

		GpuApi::SetDrawContext(GpuApi::DRAWCONTEXT_DebugTransparent, 0);
		GpuApi::DrawIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, m_object.NumVertices, 0, m_object.NumIndices / 3 );

		return true;
	}

private:
	SDrawableObject			 m_object;
	static GpuApi::BufferRef g_constantsFrame;
	static GpuApi::BufferRef g_constantsObject;
};

GpuApi::BufferRef CRuntimeTest::g_constantsFrame = GpuApi::BufferRef::Null();
GpuApi::BufferRef CRuntimeTest::g_constantsObject = GpuApi::BufferRef::Null();

#endif