//
//Copyright © 2014 CD Projekt Red. All Rights Reserved.
//

#ifndef _DRAWS_INSTANCED_NOBUFFERS_TEST_H_
#define _DRAWS_INSTANCED_NOBUFFERS_TEST_H_

#include "testEngine.h"

#define INSTANCING_BUFFER_SIZE 64

class CDrawInstancedNoBuffersTest : public CBaseTest
{
public:
	struct SInstanceDescriptor
	{
		RedMatrix4x4 WorldMtx;
		RedVector4 Color;
	};	

	CDrawInstancedNoBuffersTest( String& name )
		: CBaseTest( name )
	{
	}
	virtual ~CDrawInstancedNoBuffersTest(){};

	virtual Bool Initialize( )
	{
		return true;
	}

	virtual Bool Execute( CTestUtils& context  )
	{
		context.GetEffect( ET_SimpleTriangleInstanced )->SetShaders();

		// Update constants
		CBSimpleTriangleInstanced buf;
		buf.m_colors[0] = RedVector4( 1.0f, 0.0f, 0.0f, 1.0f );
		buf.m_colors[1] = RedVector4( 0.0f, 1.0f, 0.0f, 1.0f );
		buf.m_colors[2] = RedVector4( 0.0f, 0.0f, 1.0f, 1.0f );

		GpuApi::BufferInitData bufInitData;
		bufInitData.m_buffer = &buf;
		bufInitData.m_elementCount = 1;
		GpuApi::BufferRef constantBufferRef = GpuApi::CreateBuffer(sizeof(CBSimpleTriangleInstanced), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite, &bufInitData );
		GpuApi::BindConstantBuffer(0, constantBufferRef, GpuApi::VertexShader);

		// Set Context
		GpuApi::SetDrawContext( GpuApi::DRAWCONTEXT_SimpleNoCull, 1 );
				
		GpuApi::DrawInstancedPrimitiveNoBuffers( GpuApi::PRIMTYPE_TriangleList, 3, 64 );

		GpuApi::SafeRelease( constantBufferRef );

		return true;
	}
};

#endif