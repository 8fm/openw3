//
//Copyright © 2014 CD Projekt Red. All Rights Reserved.
//

#ifndef _DRAWS_INSTANCED_TEST_H_
#define _DRAWS_INSTANCED_TEST_H_

#include "testEngine.h"

#define INSTANCING_BUFFER_SIZE 64

class CDrawInstancedTest : public CBaseTest
{
public:
	struct SInstanceDescriptor
	{
		RedMatrix4x4 WorldMtx;
		RedVector4 Color;
	};	

	CDrawInstancedTest( String& name )
		: CBaseTest( name )
	{
	}
	virtual ~CDrawInstancedTest()
	{
		GpuApi::SafeRelease( m_instanceBuffer );
		GpuApi::SafeRelease( m_layout );
	};

	virtual Bool Initialize( )
	{
		// instance buffer
		const Uint32 chunkSize = sizeof( SInstanceDescriptor );
		const Uint32 instanceDataSize = chunkSize * INSTANCING_BUFFER_SIZE;
		m_instanceBuffer = GpuApi::CreateBuffer( instanceDataSize, GpuApi::BCC_Vertex, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		GpuApi::SetBufferDebugPath( m_instanceBuffer, "gpuapiDrawInstancedTest" );

		m_instances.Grow( 64 );
		for( Uint32 i = 0; i < 64; ++i )
		{
			RedMatrix4x4 worldMtx = worldMtx.IDENTITY;
			worldMtx.SetScale( 0.2f );
			worldMtx.SetTranslation( -1.0f + (i % 8) / 4.0f, 0, -0.5f + (i / 8) / 4.0f );
			m_instances[i].WorldMtx = worldMtx;
			m_instances[i].Color.X = 1.0f;
			m_instances[i].Color.Y = ( i % 8 ) / 8.0f;
			m_instances[i].Color.Z = ( i / 8 ) / 8.0f;
			m_instances[i].Color.W = 1.0f;
		}

		GpuApi::VertexPacking::PackingElement simpleInstanced[] =
		{
			{	GpuApi::VertexPacking::PT_Float3,		GpuApi::VertexPacking::PS_Position,				0,	0,	GpuApi::VertexPacking::ST_PerVertex		},
			{	GpuApi::VertexPacking::PT_Float4,		GpuApi::VertexPacking::PS_InstanceTransform,	0,	7,	GpuApi::VertexPacking::ST_PerInstance	},
			{	GpuApi::VertexPacking::PT_Float4,		GpuApi::VertexPacking::PS_InstanceTransform,	1,	7,	GpuApi::VertexPacking::ST_PerInstance	},
			{	GpuApi::VertexPacking::PT_Float4,		GpuApi::VertexPacking::PS_InstanceTransform,	2,	7,	GpuApi::VertexPacking::ST_PerInstance	},
			{	GpuApi::VertexPacking::PT_Float4,		GpuApi::VertexPacking::PS_InstanceTransform,	3,	7,	GpuApi::VertexPacking::ST_PerInstance	},
			{	GpuApi::VertexPacking::PT_Float4,		GpuApi::VertexPacking::PS_Color,				0,	7,	GpuApi::VertexPacking::ST_PerInstance	},
			GpuApi::VertexPacking::PackingElement::END_OF_ELEMENTS
		};

		GpuApi::VertexLayoutDesc desc;
		desc.AddElements( simpleInstanced );

		m_layout = GpuApi::CreateVertexLayout( desc );
		if( !m_layout )
		{
			ERR_GAT( TXT( "[%s] INITIALIZATION FAILURE" ), m_testName.AsChar() );
			return false;
		}

		return true;
	}

	virtual Bool Execute( CTestUtils& context  )
	{
		//View matrix
		RedMatrix4x4 lookAt = LookAtLH(RedVector3(0, 1, -2), RedVector3(0, 0, 0.5f), RedVector3(0,1,0));

		// Proj Matrix
		RedMatrix4x4 proj = RedMath::SIMD::BuildPerspectiveLH( DEG2RAD(45.f), 16.0f / 9.0f, 0.1f, 100.0f );

		//LightPos
		RedVector4 lightPos(0, 1, 0, 1);

		CBSimplePhongInstanced buf;
		buf.m_worldToView = lookAt;
		buf.m_viewToScreen = proj;
		buf.m_lightPos = lightPos;

		GpuApi::BufferInitData bufInitData;
		bufInitData.m_elementCount = 1;
		bufInitData.m_buffer = &buf;
		GpuApi::BufferRef constantBufferRef = GpuApi::CreateBuffer(sizeof(CBSimplePhongInstanced), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite, &bufInitData );
		GpuApi::BindConstantBuffer(0, constantBufferRef, GpuApi::VertexShader);

		context.GetEffect( ET_SimplePhongInstanced )->SetShaders();
		GpuApi::SetVertexFormatRaw( m_layout );

		// Set Context
		GpuApi::SetCustomDrawContext( GpuApi::DSSM_NoStencilFullDepthLE, GpuApi::RASTERIZERMODE_DefaultCullCCW, GpuApi::BLENDMODE_Set );

		//set instance buffer
		{
			Uint32 numInstances = m_instances.Size();
			if ( !m_instances.Empty() && numInstances <= INSTANCING_BUFFER_SIZE )
			{
				static const Uint32 streamStride = sizeof( SInstanceDescriptor );
				void *instancedPtr = nullptr;

				// Lock the instance buffer for write
				const Int32 lockOffset = 0;
				const Uint32 lockSize = streamStride * numInstances;
				instancedPtr = GpuApi::LockBuffer( m_instanceBuffer, GpuApi::BLF_Discard, lockOffset, lockSize );

				// Emit to instance buffer
				for ( Uint32 i = 0; i < m_instances.Size(); ++i )
				{
					// Import instance data
					Red::System::MemoryCopy( instancedPtr, &m_instances[i], streamStride );
					instancedPtr = static_cast<Uint8*>(instancedPtr) + streamStride;
				}

				// Return the buffer
				GpuApi::UnlockBuffer( m_instanceBuffer );

				// Bind
				Uint32 offset = 0;
				GpuApi::BindVertexBuffers( 7, 1, &m_instanceBuffer, &streamStride, &offset );
			}
		}

		GpuApi::TextureDesc tdesc = GpuApi::GetTextureDesc(GpuApi::GetBackBufferTexture());

		GpuApi::ViewportDesc desc;
		desc.width = tdesc.width / 2;
		desc.height = tdesc.height / 2;

		for( Uint32 i = 0; i < 2; ++i )
		{
			GpuApi::BindIndexBuffer( context.GetObjects()[1].IndexBuffer );

			desc.x = desc.width * (i % 2);
			desc.y = desc.height * (i / 2);
			GpuApi::SetViewport(desc);

			switch( i )
			{
			case 0:
				GpuApi::BindVertexBuffers( 0, 1, &context.GetObjects()[1].VertexBufferIndexed, &m_stride, &m_offset );
				GpuApi::DrawInstancedIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, context.GetObjects()[1].NumVertices, 0, context.GetObjects()[1].NumIndices / 3, m_instances.Size() );
				break;
			case 1:
				GpuApi::BindVertexBuffers( 0, 1, &context.GetObjects()[1].VertexBufferNonIndexed, &m_stride, &m_offset );
				GpuApi::DrawInstancedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, context.GetObjects()[1].NumIndices, context.GetObjects()[1].NumIndices / 3, m_instances.Size() );
				break;
			}
		}

		GpuApi::SafeRelease( constantBufferRef );

		return true;
	}

private:
	GpuApi::BufferRef m_instanceBuffer;
	GpuApi::VertexLayoutRef m_layout;
	TDynArray< SInstanceDescriptor > m_instances;
};

#endif