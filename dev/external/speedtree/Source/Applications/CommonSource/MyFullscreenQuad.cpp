///////////////////////////////////////////////////////////////////////
//  MyFullscreenQuad.cpp
//
//	All source files prefixed with "My" indicate an example implementation, 
//	meant to detail what an application might (and, in most cases, should)
//	do when interfacing with the SpeedTree SDK.  These files constitute 
//	the bulk of the SpeedTree Reference Application and are not part
//	of the official SDK, but merely example or "reference" implementations.
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with 
//  the terms of that agreement.
//
//      Copyright (c) 2003-2014 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      http://www.idvinc.com


///////////////////////////////////////////////////////////////////////  
//  Preprocessor

#include "MyFullscreenQuad.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  CMyFullscreenQuad::CMyFullscreenQuad

CMyFullscreenQuad::CMyFullscreenQuad( ) :
	m_cQuad(false, false, "fullscreen")
{
}


///////////////////////////////////////////////////////////////////////  
//  CMyFullscreenQuad::~CMyFullscreenQuad

CMyFullscreenQuad::~CMyFullscreenQuad( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CMyFullscreenQuad::InitGfx

st_bool CMyFullscreenQuad::InitGfx(st_int32 nScreenWidth, st_int32 nScreenHeight, const CShaderTechnique& cTechnique)
{
    st_bool bSuccess = false;

    if (nScreenWidth > 0 && nScreenHeight > 0)
    {
        InitView(nScreenWidth, nScreenHeight);
        bSuccess = InitGeometry(nScreenWidth, nScreenHeight, cTechnique);
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyFullscreenQuad::InitView

void CMyFullscreenQuad::InitView(st_int32 nScreenWidth, st_int32 nScreenHeight)
{
    // set up simple projection
    Mat4x4 mProjection;
    mProjection.Ortho(0.0f, st_float32(nScreenWidth), 0.0f, st_float32(nScreenHeight), 0.1f, 2.0f);

    // set up simple modelview
    Mat4x4 mModelview;
    mModelview.Translate(0.0f, 0.0f, -1.0f);

    m_mCompositeView = mModelview * mProjection;
}


///////////////////////////////////////////////////////////////////////  
//  CMyFullscreenQuad::InitGeometry

struct SOverlayVertex
{
    st_float32      m_afPos[3];
    st_float32      m_afTexCoords[2];
};

st_bool CMyFullscreenQuad::InitGeometry(st_int32 nScreenWidth, st_int32 nScreenHeight, const CShaderTechnique& cTechnique)
{
    st_bool bSuccess = false;

    // set up vertex declaration
	const SVertexDecl::SAttribDesc c_asVertexDecl[ ] =
	{
		{ 0, VERTEX_ATTRIB_0, VERTEX_FORMAT_FULL_FLOAT, 3,
			{ { VERTEX_PROPERTY_POSITION, VERTEX_COMPONENT_X }, 
		      { VERTEX_PROPERTY_POSITION, VERTEX_COMPONENT_Y }, 
			  { VERTEX_PROPERTY_POSITION, VERTEX_COMPONENT_Z }, 
			  { VERTEX_PROPERTY_UNASSIGNED, 0 } } },
		{ 0, VERTEX_ATTRIB_1, VERTEX_FORMAT_FULL_FLOAT, 2,
			{ { VERTEX_PROPERTY_DIFFUSE_TEXCOORDS, VERTEX_COMPONENT_X }, 
			  { VERTEX_PROPERTY_DIFFUSE_TEXCOORDS, VERTEX_COMPONENT_Y }, 
              { VERTEX_PROPERTY_UNASSIGNED, 0 },
			  { VERTEX_PROPERTY_UNASSIGNED, 0 } } },
		VERTEX_DECL_END( )
	};
    SVertexDecl sVertexDecl;
    sVertexDecl.Set(c_asVertexDecl);

    if (m_cQuad.SetVertexDecl(sVertexDecl, &cTechnique))
    {
        bSuccess = true;

        // set up vertex buffer
        if (m_cQuad.CreateUninitializedVertexBuffer(4))
        {
			#ifdef SPEEDTREE_OPENGL
				const st_float32 c_fTopTexCoord = 0.0f;
			#else
				const st_float32 c_fTopTexCoord = 1.0f;
			#endif
			const st_float32 c_fBottomTexCoord = 1.0f - c_fTopTexCoord;

            const SOverlayVertex c_asVertices[4] = 
            {
                //  x                         y                          z         s     t
                { { 0.0f,                     0.0f,                      0.0f }, { 0.0f, c_fTopTexCoord } },
                { { st_float32(nScreenWidth), 0.0f,                      0.0f }, { 1.0f, c_fTopTexCoord } },
                { { st_float32(nScreenWidth), st_float32(nScreenHeight), 0.0f }, { 1.0f, c_fBottomTexCoord } },
                { { 0.0f,                     st_float32(nScreenHeight), 0.0f }, { 0.0f, c_fBottomTexCoord } }
            };
            bSuccess &= m_cQuad.OverwriteVertices(c_asVertices, 4, 0);
        }

        // set up index buffer
        bSuccess &= m_cQuad.SetIndexFormat(INDEX_FORMAT_UNSIGNED_16BIT);
        if (m_cQuad.CreateUninitializedIndexBuffer(6))
        {
            const st_uint16 c_auiIndices[6] = { 0, 1, 2, 0, 2, 3 };
            bSuccess &= m_cQuad.OverwriteIndices(c_auiIndices, 6, 0);
        }
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyFullscreenQuad::ReleaseGfxResources

void CMyFullscreenQuad::ReleaseGfxResources(void)
{
    m_cQuad.ReleaseGfxResources( );
}


///////////////////////////////////////////////////////////////////////  
//  CMyFullscreenQuad::Render

st_bool CMyFullscreenQuad::Render(const CRenderState& cRenderState) const
{
    st_bool bSuccess = false;

    // set view
	CForestRender::GetFrameConstantBufferContents( ).m_mModelViewProj2d = m_mCompositeView.Transpose( );

	// update constant buffer and render
	if (CForestRender::GetFrameConstantBuffer( ).Update( ))
    {
        // bind render state (includes shader)
        if (cRenderState.BindMaterialWhole(RENDER_PASS_MAIN, TEXTURE_BIND_DISABLED))
        {
            // bind VB & IB
            if (m_cQuad.EnableFormat( ) &&
                m_cQuad.BindVertexBuffer( ) &&
                m_cQuad.BindIndexBuffer( ))
            {
                bSuccess = m_cQuad.RenderIndexed(PRIMITIVE_TRIANGLES, 0, 6, 0, 4);

                bSuccess &= m_cQuad.UnBindIndexBuffer( );
                bSuccess &= m_cQuad.UnBindVertexBuffer( );
                bSuccess &= m_cQuad.DisableFormat( );
            }

			cRenderState.UnBind( );
        }
    }

    return bSuccess;
}

