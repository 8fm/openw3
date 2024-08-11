//
// Copyright © 2014 CD Projekt Red. All Rights Reserved.
//	

#ifndef _DRAW_SYSTEM_PRIMITIVE_TEST_H_
#define _DRAW_SYSTEM_PRIMITIVE_TEST_H_

#include "testEngine.h"

class CDrawSystemPrimitiveTest : public CBaseTest
{
public:
	CDrawSystemPrimitiveTest( String& name )
		: CBaseTest( name )
	{

	}
	virtual ~CDrawSystemPrimitiveTest(){};

	virtual Bool Initialize( )
	{
		return true;
	}

	virtual Bool Execute( CTestUtils& context )
	{
		//View matrix
		RedMatrix4x4 lookAt = LookAtLH(RedVector3(0, 1, -2), RedVector3(0, 0, 0.5f), RedVector3(0,1,0));

		// Proj Matrix
		RedMatrix4x4 proj = RedMath::SIMD::BuildPerspectiveLH( DEG2RAD(45.f), 16.0f / 9.0f, 0.1f, 100.0f );

		//LightPos
		RedVector4 lightPos(0, 1, 0, 1);

		CBSimplePhongVS0 buf;
		buf.m_worldToView = lookAt;
		buf.m_viewToScreen = proj;
		buf.m_lightPos = lightPos;

		GpuApi::BufferInitData bufInitData;
		bufInitData.m_elementCount = 1;
		bufInitData.m_buffer = &buf;
		GpuApi::BufferRef constantBufferRef = GpuApi::CreateBuffer(sizeof(CBSimplePhongVS0), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite, &bufInitData );
		GpuApi::BindConstantBuffer(0, constantBufferRef, GpuApi::VertexShader);

		GpuApi::SetCustomDrawContext( GpuApi::DSSM_NoStencilFullDepthLE, GpuApi::RASTERIZERMODE_DefaultCullCCW, GpuApi::BLENDMODE_Set );

		context.GetEffect(ET_SimplePhong)->SetShaders();

		for( Uint32 i = 0; i < context.GetObjects().Size(); ++i )
		{
			// worldMtx
			CBSimplePhongVS1 bufVS;
			bufVS.m_localToWorld = context.GetObjects()[i].LocalMatrix;

			GpuApi::BufferInitData bufInitDataVS;
			bufInitDataVS.m_elementCount = 1;
			bufInitDataVS.m_buffer = &bufVS;
			GpuApi::BufferRef constantBufferVSRef = GpuApi::CreateBuffer(sizeof(CBSimplePhongVS1), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite, &bufInitDataVS );
			BindConstantBuffer( 1, constantBufferVSRef, GpuApi::VertexShader );

			// surface color
			CBSimplePhongPS0 bufPS;
			bufPS.m_surfaceColor = context.GetObjects()[i].Color;

			GpuApi::BufferInitData bufInitDataPS;
			bufInitDataPS.m_elementCount = 1;
			bufInitDataPS.m_buffer = &bufPS;
			GpuApi::BufferRef constantBufferPSRef = GpuApi::CreateBuffer(sizeof(CBSimplePhongPS0), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite, &bufInitDataPS );
			BindConstantBuffer( 0, constantBufferPSRef, GpuApi::PixelShader );

			GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemPosNormal);
			
			GpuApi::TextureDesc tdesc = GpuApi::GetTextureDesc(GpuApi::GetBackBufferTexture());

			GpuApi::ViewportDesc desc;
			desc.width = tdesc.width / 2;
			desc.height = tdesc.height / 2;

			for( Uint32 j = 0; j < 2; ++j )
			{
				desc.x = j * desc.width;
				desc.y = 0;
				GpuApi::SetViewport(desc);

				switch( j )
				{
				case 0:
					GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleList, context.GetObjects()[i].NumIndices / 3, context.GetObjects()[i].VerticesNonIndexed );
					break;
				case 1:
					GpuApi::DrawSystemIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, context.GetObjects()[i].NumVertices, context.GetObjects()[i].NumIndices / 3, context.GetObjects()[i].Indices, context.GetObjects()[i].Vertices );
					break;
				}				
			}

			GpuApi::SafeRelease( constantBufferVSRef );
			GpuApi::SafeRelease( constantBufferPSRef );
		}

		GpuApi::SafeRelease( constantBufferRef );

		return true;
	}
};

#endif