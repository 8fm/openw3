/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _SIMPLE_CULL_TEST_H_
#define _SIMPLE_CULL_TEST_H_

#include "testEngine.h"

class CSimpleCullTest : public CBaseTest
{
public:
	CSimpleCullTest( String& name )
		: CBaseTest( name )
	{
	}

	virtual ~CSimpleCullTest()
	{
		GpuApi::SafeRelease( m_constants );
		GpuApi::SafeRelease( m_constantsCsVs );
		GpuApi::SafeRelease( m_vertexBuffer );
		GpuApi::SafeRelease( m_vertexBufferTriangle );
		GpuApi::SafeRelease( m_vertexBufferCamPlane );
		GpuApi::SafeRelease( m_indexBuffer );
		GpuApi::SafeRelease( m_indexBufferTriangle );
		GpuApi::SafeRelease( m_appendBuffer );
		GpuApi::SafeRelease( m_indirectArgs );
	};

	virtual Bool Initialize( )
	{ 
		m_constants = GpuApi::CreateBuffer( sizeof( CBSimpleDraw ), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		if ( !m_constants )
		{
			ERR_GAT( TXT( "[%s] Couldn't create constant buffer!" ), m_testName.AsChar() );
			return false;
		}
		m_constantsCsVs = GpuApi::CreateBuffer( sizeof( CBSimpleDrawCsVs ), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		if ( !m_constantsCsVs )
		{
			ERR_GAT( TXT( "[%s] Couldn't create constant buffer!" ), m_testName.AsChar() );
			return false;
		}

		// Create vertex buffer
		typedef struct Vertex
		{
			Float x, y, z;		// Position
		} Vertex;
		m_stride = sizeof(Vertex);
		m_offset = 0;

		const Float halfSide = 0.2f;
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

			//   POSITION         
			{-halfSide, -halfSide, 0.0f },
			{ halfSide, -halfSide, 0.0f },
			{-halfSide,  halfSide, 0.0f },
			{ halfSide,  halfSide, 0.0f },
		};

		{
			GpuApi::BufferInitData bufInitData;
			bufInitData.m_buffer = &vertexData;
			bufInitData.m_elementCount = sizeof( Vertex );
			m_vertexBuffer = GpuApi::CreateBuffer( 4 * sizeof( Vertex ), GpuApi::BCC_Raw, GpuApi::BUT_Default, 0, &bufInitData );
		}

		static const Vertex vertexDataTriangle[] =
		{   
			{0.0f, 0.0f, 0.0f },
			{0.0f, 0.0f, 0.0f },
			{0.0f, 0.0f, 0.0f },
		};

		{
			GpuApi::BufferInitData bufInitData;
			bufInitData.m_buffer = &vertexDataTriangle;
			bufInitData.m_elementCount = 3 * sizeof( Vertex ) / 4;
			m_vertexBufferTriangle = GpuApi::CreateBuffer( 3 * sizeof( Vertex ), GpuApi::BCC_Vertex, GpuApi::BUT_Default, 0, &bufInitData );
		}

		typedef struct CamPlaneVertex
		{
			Float x, y, z;		// Position
			Uint8  r, g, b, a;	// Color
		} CamPlaneVertex;
		m_camPlaneStride = sizeof( CamPlaneVertex );
		m_camPlaneOffset = 0;
		const Float halfSideCamPlane = 0.5f;
		static const CamPlaneVertex vertexDataCamPlane[] =
		{
			// 2    3
			// +----+
			// |\   |
			// | \  |
			// |  \ |
			// |   \|
			// +----+
			// 0    1

			//   POSITION         
			{-halfSideCamPlane, -halfSideCamPlane, 0.0f,    127, 127, 127, 255 },
			{ halfSideCamPlane, -halfSideCamPlane, 0.0f,    127, 127, 127, 255 },
			{-halfSideCamPlane,  halfSideCamPlane, 0.0f,    127, 127, 127, 255 },
			{ halfSideCamPlane,  halfSideCamPlane, 0.0f,    127, 127, 127, 255 },
		};

		{
			GpuApi::BufferInitData bufInitData;
			bufInitData.m_buffer = &vertexDataCamPlane;
			bufInitData.m_elementCount = sizeof( CamPlaneVertex );
			m_vertexBufferCamPlane = GpuApi::CreateBuffer( 4 * sizeof( CamPlaneVertex ), GpuApi::BCC_Vertex, GpuApi::BUT_Default, 0, &bufInitData );
		}

		{
			// Create index buffer
			static const Uint16 indexData[] =
			{
				0, 1, 2,
				1, 3, 2
			};

			GpuApi::BufferInitData bufInitData;
			bufInitData.m_buffer = &indexData;
			bufInitData.m_elementCount = 8 * sizeof( Uint16 ) / 4;
			m_indexBuffer = GpuApi::CreateBuffer( 8 * sizeof( Uint16 ), GpuApi::BCC_Raw, GpuApi::BUT_Default, 0, &bufInitData );
		}

		{
			// Create index buffer
			static const Uint16 indexData[] =
			{
				0, 1, 2
			};

			GpuApi::BufferInitData bufInitData;
			bufInitData.m_buffer = &indexData;
			bufInitData.m_elementCount = 3 * sizeof( Uint16 ) / 4;
			m_indexBufferTriangle = GpuApi::CreateBuffer( 3 * sizeof( Uint16 ), GpuApi::BCC_Index16Bit, GpuApi::BUT_Default, 0, &bufInitData );
		}

		{
			// Create index buffer

			#define MAX_APPEND_ELEMENTS 1024
			GpuApi::BufferInitData bufInitData;
			Uint32 byteSize = 6 * sizeof( Uint32 );
			bufInitData.m_elementCount = byteSize / 4;
			m_appendBuffer = GpuApi::CreateBuffer( byteSize, GpuApi::BCC_Raw, GpuApi::BUT_Default, 0, &bufInitData );
		}

		{
			// Create indirect args buffer

			GpuApi::BufferInitData bufInitData;
			Uint32 byteSize = 5 * sizeof( Uint32 );
			bufInitData.m_elementCount = byteSize / 4;
			m_indirectArgs = GpuApi::CreateBuffer( byteSize, GpuApi::BCC_IndirectUAV, GpuApi::BUT_Default, 0, &bufInitData );
		}

#define MAX_OBJECTS 16

		for(Uint16 i = 0; i < MAX_OBJECTS; ++i)
		{
			m_weights[i] = 5.0f * (float)rand() / RAND_MAX;
		}

		return true; 
	};

	virtual Bool Execute( CTestUtils& context )
	{
		static_assert( sizeof( CBSimpleCull ) == sizeof( CBSimpleDraw ), "CBSimpleCull and CBSimpleDraw are different sizes. Probably need to create different bufferrefs here" );

		// ------------------------- PART0 ---------------------------------
		// --------------------- DRAW BACKGROUND ---------------------------
		context.GetEffect(ET_SimpleDraw)->SetShaders();

		GpuApi::SetCustomDrawContext( GpuApi::DSSM_NoStencilFullDepthLE, GpuApi::RASTERIZERMODE_DefaultNoCull, GpuApi::BLENDMODE_Set );
		GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemPosColor );

		// Update constants
		{
			void* lockedConstantsData = GpuApi::LockBuffer( m_constants, GpuApi::BLF_Discard, 0, sizeof( CBSimpleDraw ) );
			RED_ASSERT( lockedConstantsData );
			CBSimpleDraw* lockedConstants = static_cast< CBSimpleDraw* >( lockedConstantsData );

			lockedConstants->m_localToWorld = RedMatrix4x4::IDENTITY.AsFloat();
			lockedConstants->m_worldToView = RedMatrix4x4::IDENTITY.AsFloat();
			lockedConstants->m_viewToScreen = RedMatrix4x4::IDENTITY.AsFloat();

			GpuApi::UnlockBuffer( m_constants );
		}
		GpuApi::BindConstantBuffer( 0, m_constants, GpuApi::VertexShader );

		GpuApi::BindVertexBuffers(0, 1, &m_vertexBufferCamPlane, &m_stride, &m_offset );
		GpuApi::BindIndexBuffer(m_indexBuffer);

		GpuApi::DrawIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, 4, 0, 2 );

		GpuApi::BindNullVertexBuffers();
		GpuApi::BindIndexBuffer(GpuApi::BufferRef::Null());

		// ------------------------- PART1 ---------------------------------
		// ------------------------ CULL OBJECTS ---------------------------
		static float time = 0.0f;
		time += 0.0004f;

		// Proj matrix
		RedMatrix4x4 proj = RedMath::SIMD::BuildPerspectiveLH( DEG2RAD(45.f), 16.0f / 9.0f, 0.1f, 100.0f );

		// View matrix
		RedMatrix4x4 lookAt2 = LookAtLH(RedVector3(0, 0, -2), RedVector3(0, 0, 0.0f), RedVector3(0,1,0));
		RedMatrix4x4 lookAt4 = LookAtLH(RedVector3(0, 0, -4), RedVector3(0, 0, 0.0f), RedVector3(0,1,0));

		for( Uint16 i = 0; i < MAX_OBJECTS; ++i )
		{
			// ------------------------- PART1a - COMPUTE PART ------------------------------

			// Local matrix
			const float r = 2.0f * sin(time);
			RedMatrix4x4 local = RedMatrix4x4::IDENTITY;
			local.SetRotZ( time * m_weights[i] + i );
			local.SetTranslation(r*cos(time+i), r*sin(time+i),0);
			
			// Update constants
			{
				void* lockedConstantsData = GpuApi::LockBuffer( m_constants, GpuApi::BLF_Discard, 0, sizeof( CBSimpleCull ) );
				RED_ASSERT( lockedConstantsData );
				CBSimpleCull* lockedConstants = static_cast< CBSimpleCull* >( lockedConstantsData );

				lockedConstants->m_localToWorld = local.AsFloat();
				lockedConstants->m_worldToView = lookAt2.AsFloat();
				lockedConstants->m_viewToScreen = proj.AsFloat();

				GpuApi::UnlockBuffer( m_constants );
			}
			GpuApi::BindConstantBuffer( 0, m_constants, GpuApi::ComputeShader );

			GpuApi::BindBufferUAV(m_indirectArgs, 0 );
			GpuApi::BindBufferUAV(m_vertexBuffer, 1 );
			GpuApi::BindBufferUAV(m_indexBuffer, 2 );
			GpuApi::BindBufferUAV(m_appendBuffer, 3 );

			context.GetEffect(ET_SimpleCull)->SetShaders();
			context.GetEffect(ET_SimpleCull)->Dispatch(2,1,1);

			GpuApi::BindBufferUAV(GpuApi::BufferRef::Null(), 0);
			GpuApi::BindBufferUAV(GpuApi::BufferRef::Null(), 1);
			GpuApi::BindBufferUAV(GpuApi::BufferRef::Null(), 2);
			GpuApi::BindBufferUAV(GpuApi::BufferRef::Null(), 3);

			// ------------------------- PART1b - VISUALISE ----------------------------
			context.GetEffect(ET_SimpleDrawCsVs)->SetShaders();
			GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemPos );

			// Update constants
			{
				void* lockedConstantsData = GpuApi::LockBuffer( m_constantsCsVs, GpuApi::BLF_Discard, 0, sizeof( CBSimpleDrawCsVs ) );
				RED_ASSERT( lockedConstantsData );
				CBSimpleDrawCsVs* lockedConstants = static_cast< CBSimpleDrawCsVs* >( lockedConstantsData );

				lockedConstants->m_localToWorld = local.AsFloat();
				lockedConstants->m_worldToView = lookAt4.AsFloat();
				lockedConstants->m_viewToScreen = proj.AsFloat();
				lockedConstants->m_color = RedVector4(1.0f, 0.0f, 0.0f, 1.0f);

				GpuApi::UnlockBuffer( m_constantsCsVs );
			}
			GpuApi::BindConstantBuffer( 0, m_constantsCsVs, GpuApi::VertexShader );

			GpuApi::SetCustomDrawContext( GpuApi::DSSM_NoStencilFullDepthLE, GpuApi::RASTERIZERMODE_DefaultNoCull_Wireframe, GpuApi::BLENDMODE_Set );

			GpuApi::BindVertexBuffers(0, 1, &m_vertexBufferTriangle, &m_stride, &m_offset );
			GpuApi::BindIndexBuffer(m_indexBufferTriangle);
			GpuApi::BindBufferSRV(m_appendBuffer, 0, GpuApi::VertexShader);
			GpuApi::BindBufferSRV(m_vertexBuffer, 1, GpuApi::VertexShader);

			GpuApi::DrawInstancedIndexedPrimitiveIndirect( GpuApi::PRIMTYPE_TriangleList );

			GpuApi::BindNullVertexBuffers();
			GpuApi::BindIndexBuffer(GpuApi::BufferRef::Null());
			GpuApi::BindBufferSRV(GpuApi::BufferRef::Null(), 0, GpuApi::VertexShader);
			GpuApi::BindBufferSRV(GpuApi::BufferRef::Null(), 1, GpuApi::VertexShader);
		}

		return true;
	}

private:
	GpuApi::BufferRef m_constants;
	GpuApi::BufferRef m_constantsCsVs;

	GpuApi::BufferRef m_vertexBuffer;
	GpuApi::BufferRef m_vertexBufferCamPlane;
	GpuApi::BufferRef m_vertexBufferTriangle;

	GpuApi::BufferRef m_indexBuffer;
	GpuApi::BufferRef m_indexBufferTriangle;

	GpuApi::BufferRef m_appendBuffer;

	GpuApi::BufferRef m_indirectArgs;

	Uint32 m_stride;
	Uint32 m_offset;

	Uint32 m_camPlaneStride;
	Uint32 m_camPlaneOffset;

	float m_weights[MAX_OBJECTS];
};

#endif //_SIMPLE_CULL_TEST_H_