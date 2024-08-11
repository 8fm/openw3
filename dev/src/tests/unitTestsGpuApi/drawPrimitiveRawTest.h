//
// Copyright © 2014 CD Projekt Red. All Rights Reserved.
//	

#ifndef _DRAW_PRIMITIVE_RAW_TEST_H_
#define _DRAW_PRIMITIVE_RAW_TEST_H_

#include "testEngine.h"

class CDrawPrimitiveRawTest : public CBaseTest
{
public:
	CDrawPrimitiveRawTest( String& name )
		: CBaseTest( name )
	{
	}
	virtual ~CDrawPrimitiveRawTest(){};

	virtual Bool Initialize( )
	{
		return true;
	}

	virtual Bool Execute( CTestUtils& context )
	{
		GpuApi::SetCustomDrawContext( GpuApi::DSSM_NoStencilFullDepthLE, GpuApi::RASTERIZERMODE_DefaultCullCCW, GpuApi::BLENDMODE_Set );

		context.GetEffect(ET_SimplePhongRaw)->SetShaders();

		//View matrix
		RedMatrix4x4 lookAt = LookAtLH(RedVector3(0, 1, -2), RedVector3(0, 0, 0.5f), RedVector3(0,1,0));

		// Proj Matrix
		RedMatrix4x4 proj = RedMath::SIMD::BuildPerspectiveLH( DEG2RAD(45.f), 16.0f / 9.0f, 0.1f, 100.0f );

		//LightPos
		RedVector4 lightPos(0, 1, 0, 1);

		CBSimplePhongRaw buf;
		buf.m_worldToView = lookAt;
		buf.m_viewToScreen = proj;
		buf.m_lightPos = lightPos;

		for( Uint32 i = 0; i < context.GetObjects().Size(); ++i )
		{
			buf.m_localToWorld = context.GetObjects()[i].LocalMatrix;
			buf.m_surfaceColor = context.GetObjects()[(i+1)%context.GetObjects().Size()].Color;

			GpuApi::BufferInitData bufInitData;
			bufInitData.m_buffer = &buf;
			bufInitData.m_elementCount = 1;
			m_constantBufferRef = GpuApi::CreateBuffer(sizeof(CBSimplePhongRaw), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite, &bufInitData );

			GpuApi::BindConstantBuffer(3, m_constantBufferRef, GpuApi::VertexShader);
			GpuApi::BindConstantBuffer(3, m_constantBufferRef, GpuApi::PixelShader);

			GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemPosNormal);
			GpuApi::BindVertexBuffers( 0, 1, &context.GetObjects()[i].VertexBufferNonIndexed, &m_stride, &m_offset );

			GpuApi::DrawPrimitiveRaw( GpuApi::PRIMTYPE_TriangleList, 0, context.GetObjects()[i].NumIndices / 3 );

			GpuApi::Release( m_constantBufferRef );
		}

		return true;
	}

private:
	GpuApi::BufferRef m_constantBufferRef;
};

#endif