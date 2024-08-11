///////////////////////////////////////////////////////////////////////
//  MyOverlays.cpp
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

#include "MyOverlays.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  CMyOverlays::CMyOverlays

CMyOverlays::CMyOverlays( ) :
	m_cQuads(false, false, "overlays")
{
}


///////////////////////////////////////////////////////////////////////  
//  CMyOverlays::~CMyOverlays

CMyOverlays::~CMyOverlays( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CMyOverlays::InitGfx

st_bool CMyOverlays::InitGfx(const SMyCmdLineOptions& sCmdLine, const CMyConfigFile& cConfigFile)
{
	st_bool bSuccess = true;

	if (!cConfigFile.m_aOverlays.empty( ))
	{
		// copy overlay parameters and load textures
		m_aOverlays.resize(cConfigFile.m_aOverlays.size( ));
		for (size_t i = 0;  i < cConfigFile.m_aOverlays.size( ); ++i)
		{
			const CMyConfigFile::SOverlay& c_sSrcOverlay = cConfigFile.m_aOverlays[i];
			SOverlay& sDestOverlay = m_aOverlays[i];

			sDestOverlay.m_fSize = c_sSrcOverlay.m_fSize;
			sDestOverlay.m_eAnchor = EAnchorType(c_sSrcOverlay.m_nAnchor);
			sDestOverlay.m_afPos[0] = c_sSrcOverlay.m_afPos[0];
			sDestOverlay.m_afPos[1] = c_sSrcOverlay.m_afPos[1];

			// load texture
			if (!sDestOverlay.m_cImage.Load(c_sSrcOverlay.m_strFilename.c_str( )))
			{
				CCore::SetError("Failed to load overlay [%s]", c_sSrcOverlay.m_strFilename.c_str( ));
				bSuccess = false;
			}
		}

		if (bSuccess)
		{
			// set up view matrix
			{
				// set up simple projection
				Mat4x4 mProjection;
				mProjection.Ortho(0.0f, st_float32(sCmdLine.m_nWindowWidth), 0.0f, st_float32(sCmdLine.m_nWindowHeight), 0.1f, 2.0f);

				// set up simple modelview
				Mat4x4 mModelview;
				mModelview.Translate(0.0f, 0.0f, -1.0f);

				m_mCompositeView = mModelview * mProjection;
			}

			// set up render state & load shaders
			{
				// set up app state (user settings, configurable on cmd-line)
				SAppState sAppState;
				sAppState.m_bMultisampling = false;
				sAppState.m_bAlphaToCoverage = false;
				sAppState.m_bDeferred = cConfigFile.m_sDeferredRender.m_bEnabled;
				sAppState.m_eOverrideDepthTest = SAppState::OVERRIDE_DEPTH_TEST_DISABLE;

				// set up render state (should be as simple as possible)
				CStaticArray<CFixedString> aSearchPaths(2, "CMyApplication::InitDeferredRenderingGfx");
				aSearchPaths[0] = cConfigFile.m_strOverlayShaderPath + c_szFolderSeparator + CShaderTechnique::GetCompiledShaderFolder( );
				aSearchPaths[1] = cConfigFile.m_strOverlayShaderPath;

				// render state
				m_cRenderState.m_eFaceCulling = CULLTYPE_NONE;
				m_cRenderState.m_bBlending = true;
				m_cRenderState.m_eLightingModel = sAppState.m_bDeferred ? LIGHTING_MODEL_DEFERRED : LIGHTING_MODEL_PER_PIXEL;

				if (!m_cRenderState.InitGfx(sAppState, aSearchPaths, 0, 1.0f, "Fullscreen", "Overlay", NULL))
				{
					CCore::SetError("Failed to load overlay shaders");
					bSuccess = false;
				}
			}

			// set up geometry
			if (bSuccess)
				InitGeometry(sCmdLine.m_nWindowWidth, sCmdLine.m_nWindowHeight, m_cRenderState.GetTechnique( ));
		}
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyOverlays::ReleaseGfxResources

void CMyOverlays::ReleaseGfxResources(void)
{
	for (size_t i = 0;  i < m_aOverlays.size( ); ++i)
		m_aOverlays[i].m_cImage.ReleaseGfxResources( );

	m_cRenderState.ReleaseGfxResources( );
	m_cQuads.ReleaseGfxResources( );
}


///////////////////////////////////////////////////////////////////////  
//  CMyOverlays::Render

st_bool CMyOverlays::Render(void) const
{
	st_bool bSuccess = false;

	// set view
	if (!m_aOverlays.empty( ))
	{
		// set view
		CForestRender::GetFrameConstantBufferContents( ).m_mModelViewProj2d = m_mCompositeView.Transpose( );

		// update constant buffer and render
		if (CForestRender::GetFrameConstantBuffer( ).Update( ))
		{
			if (m_cRenderState.BindMaterialWhole(RENDER_PASS_MAIN, TEXTURE_BIND_DISABLED))
			{
				if (m_cQuads.EnableFormat( ) &&
					m_cQuads.BindVertexBuffer( ) &&
					m_cQuads.BindIndexBuffer( ))
				{
					bSuccess = true;

					for (size_t i = 0;  i < m_aOverlays.size( ); ++i)
					{
						const SOverlay& sOverlay = m_aOverlays[i];

						if (CShaderConstant::SetTexture(TEXTURE_REGISTER_DIFFUSE, sOverlay.m_cImage))
						{
							bSuccess &= m_cQuads.RenderIndexed(PRIMITIVE_TRIANGLES, st_uint32(i * 6), 6, st_uint32(i * 4), 4);
						}
					}

					bSuccess &= m_cQuads.UnBindIndexBuffer( );
					bSuccess &= m_cQuads.UnBindVertexBuffer( );
					bSuccess &= m_cQuads.DisableFormat( );
				}

				m_cRenderState.UnBind( );
			}
		}
	}
	else
	{
		bSuccess = true;
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyOverlays::InitGeometry

struct SOverlayVertex
{
    st_float32      m_afPos[3];
    st_float32      m_afTexCoords[2];
};

st_bool CMyOverlays::InitGeometry(st_int32 nScreenWidth, st_int32 nScreenHeight, const CShaderTechnique& cTechnique)
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

    if (m_cQuads.SetVertexDecl(sVertexDecl, &cTechnique))
    {
        bSuccess = true;

        // set up vertex buffer
		const st_int32 c_nNumVertices = st_int32(4 * m_aOverlays.size( ));
        if (m_cQuads.CreateUninitializedVertexBuffer(c_nNumVertices))
        {
			CStaticArray<SOverlayVertex> aVertices(c_nNumVertices, __FUNCTION__, false);

			for (size_t i = 0; i < m_aOverlays.size( ); ++i)
			{
				const SOverlay& sOverlay = m_aOverlays[i];
				const st_int32 c_nTextureWidth = sOverlay.m_cImage.GetInfo( ).m_nWidth;
				const st_int32 c_nTextureHeight = sOverlay.m_cImage.GetInfo( ).m_nHeight;
				const st_float32 c_fAspectRatio = st_float32(c_nTextureWidth) / c_nTextureHeight;
				const st_float32 c_fOverlayWidth = sOverlay.m_fSize * nScreenWidth;
				const st_float32 c_fOverlayHeight = c_fOverlayWidth / c_fAspectRatio;

				// compute offsets based on anchor types
				st_float32 fAnchorWidthOffset = 0.0f; // left
				if (sOverlay.m_eAnchor == ANCHOR_TOP_CENTER || sOverlay.m_eAnchor == ANCHOR_CENTER_CENTER || sOverlay.m_eAnchor == ANCHOR_BOTTOM_CENTER)
					fAnchorWidthOffset = -0.5f * c_fOverlayWidth;
				else if (sOverlay.m_eAnchor == ANCHOR_TOP_RIGHT || sOverlay.m_eAnchor == ANCHOR_CENTER_RIGHT || sOverlay.m_eAnchor == ANCHOR_BOTTOM_RIGHT)
					fAnchorWidthOffset = -c_fOverlayWidth;

				st_float32 fAnchorHeightOffset = 0.0f; // bottom
				if (sOverlay.m_eAnchor == ANCHOR_CENTER_LEFT || sOverlay.m_eAnchor == ANCHOR_CENTER_CENTER || sOverlay.m_eAnchor == ANCHOR_CENTER_RIGHT)
					fAnchorHeightOffset = -0.5f * c_fOverlayHeight;
				else if (sOverlay.m_eAnchor == ANCHOR_TOP_LEFT || sOverlay.m_eAnchor == ANCHOR_TOP_CENTER || sOverlay.m_eAnchor == ANCHOR_TOP_RIGHT)
					fAnchorHeightOffset = -c_fOverlayHeight;

				const SOverlayVertex c_asVertices[4] = 
				{
					{ { nScreenWidth * sOverlay.m_afPos[0] + fAnchorWidthOffset, nScreenHeight * sOverlay.m_afPos[1] + fAnchorHeightOffset, 0.0f }, { 0.0f, 1.0f } },
					{ { nScreenWidth * sOverlay.m_afPos[0] + fAnchorWidthOffset + c_fOverlayWidth, nScreenHeight * sOverlay.m_afPos[1] + fAnchorHeightOffset, 0.0f }, { 1.0f, 1.0f } },
					{ { nScreenWidth * sOverlay.m_afPos[0] + fAnchorWidthOffset + c_fOverlayWidth, nScreenHeight * sOverlay.m_afPos[1] + fAnchorHeightOffset + c_fOverlayHeight, 0.0f }, { 1.0f, 0.0f } },
					{ { nScreenWidth * sOverlay.m_afPos[0] + fAnchorWidthOffset, nScreenHeight * sOverlay.m_afPos[1] + fAnchorHeightOffset + c_fOverlayHeight, 0.0f }, { 0.0f, 0.0f } },
				};

				for (st_int32 j = 0; j < 4; ++j)
					aVertices.push_back(c_asVertices[j]);
			}

			bSuccess &= m_cQuads.OverwriteVertices(&aVertices[0], c_nNumVertices, 0);
        }


        // set up index buffer
        bSuccess &= m_cQuads.SetIndexFormat(INDEX_FORMAT_UNSIGNED_16BIT);

		const st_int32 c_nNumIndices = st_int32(6 * m_aOverlays.size( ));
        if (m_cQuads.CreateUninitializedIndexBuffer(c_nNumIndices))
        {
			CStaticArray<st_uint16> aIndices(c_nNumIndices, __FUNCTION__, false);

            const st_uint16 c_auiIndices[6] = { 0, 1, 2, 0, 2, 3 };
			for (size_t i = 0; i < m_aOverlays.size( ); ++i)
			{
				for (st_int32 j = 0; j < 6; ++j)
					aIndices.push_back(st_uint16(c_auiIndices[j] + 4 * i));
			}

			bSuccess &= m_cQuads.OverwriteIndices(&aIndices[0], c_nNumIndices, 0);
        }
    }

    return bSuccess;
}

