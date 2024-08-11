///////////////////////////////////////////////////////////////////////  
//  SkyRI.inl
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
//  SSkyRenderInfo::SSkyRenderInfo

inline SSkyRenderInfo::SSkyRenderInfo( ) :
	m_fFogStartDistance(0.0f),
	m_fFogEndDistance(1000.0f),
	m_fFogDensity(0.001f),
	m_fSkyFogMin(-0.5f),
	m_fSkyFogMax(1.0f),
	m_fFarClip(1000.0f)
{
}


///////////////////////////////////////////////////////////////////////  
//  CSkyRI::CSkyRI

CSkyRI_TemplateList
inline CSkyRI_t::CSkyRI( ) :
	m_tSkyGeometryBuffer(false, false, "sky")
{
}


///////////////////////////////////////////////////////////////////////  
//  CSkyRI::~CSkyRI

CSkyRI_TemplateList
inline CSkyRI_t::~CSkyRI( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CSkyRI::Init

CSkyRI_TemplateList
inline st_bool CSkyRI_t::Init(void)
{
	st_bool bSuccess = true;

	// vertex declaration
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
	
	// top-level initialization functions
	bSuccess &= InitRenderState(sVertexDecl);
	if (bSuccess)
		bSuccess &= InitSkyGeometryBuffer(sVertexDecl);

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CSkyRI::ReleaseGfxResources

CSkyRI_TemplateList
inline void CSkyRI_t::ReleaseGfxResources(void)
{
	m_cSkyState.ReleaseGfxResources( );
	m_tSkyGeometryBuffer.ReleaseGfxResources( );
}


///////////////////////////////////////////////////////////////////////  
//  CSkyRI::SetRenderInfo

CSkyRI_TemplateList
inline void CSkyRI_t::SetRenderInfo(const SSkyRenderInfo& sInfo)
{
	m_sRenderInfo = sInfo;
}


///////////////////////////////////////////////////////////////////////  
//  CSkyRI::GetRenderInfo

CSkyRI_TemplateList
inline const SSkyRenderInfo& CSkyRI_t::GetRenderInfo(void) const
{
	return m_sRenderInfo;
}


///////////////////////////////////////////////////////////////////////  
//  CSkyRI::Render
//
//	Depends upon these constants (among others), currently set in CForestRI_t::StartRender()

CSkyRI_TemplateList
inline st_bool CSkyRI_t::Render(CRenderStats& cRenderStats)
{
    ScopeTrace("CSkyRI_t::RenderSky");

	st_bool bSuccess = false;

	if (m_tSkyGeometryBuffer.EnableFormat( ) &&
		m_tSkyGeometryBuffer.BindVertexBuffer( ) &&
		m_tSkyGeometryBuffer.BindIndexBuffer( ) &&
		m_cSkyState.BindMaterialWhole(RENDER_PASS_MAIN, TEXTURE_BIND_ENABLED))
	{
		bSuccess = true;

		// how much to render
		const st_uint32 c_uiNumIndices = m_tSkyGeometryBuffer.NumIndices( );
		const st_uint32 c_uiNumVertices = m_tSkyGeometryBuffer.NumVertices( );
		const st_uint32 c_uiNumTriangles = c_uiNumIndices / 3;

		bSuccess &= m_tSkyGeometryBuffer.RenderIndexed(PRIMITIVE_TRIANGLES, 0, c_uiNumIndices, 0, c_uiNumVertices);

		bSuccess &= m_cSkyState.UnBind( );
		bSuccess &= m_tSkyGeometryBuffer.UnBindIndexBuffer( );
		bSuccess &= m_tSkyGeometryBuffer.UnBindVertexBuffer( );
		bSuccess &= m_tSkyGeometryBuffer.DisableFormat( );

		// update stats
		CRenderStats::SObjectStats& sStats = cRenderStats.GetObjectStats("Sky");
		sStats.m_nNumDrawCalls += 1;
		sStats.m_nNumShaderBinds += 1;
		sStats.m_nNumTrianglesRendered += c_uiNumTriangles;
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CSkyRI::InitRenderState

CSkyRI_TemplateList
inline st_bool CSkyRI_t::InitRenderState(const SVertexDecl& sVertexDecl)
{
	st_bool bSuccess = true;

	// build search path based on path of splat map texture filename
	CStaticArray<CFixedString> aSearchPaths(2, "CSkyRI_t::InitRenderState");
	aSearchPaths[0] = m_sRenderInfo.m_strShaderPath + c_szFolderSeparator + TShaderTechniqueClass::GetCompiledShaderFolder( );
	aSearchPaths[1] = m_sRenderInfo.m_strShaderPath;

	// create sky render state
	SAppState sSkyAppState(m_sRenderInfo.m_sAppState);
	sSkyAppState.m_bAlphaToCoverage = false;
	if (sSkyAppState.m_bDeferred)
		sSkyAppState.m_eOverrideDepthTest = SAppState::OVERRIDE_DEPTH_TEST_DISABLE;

	SRenderState sSkyState;
	m_cSkyState.m_eFaceCulling = CULLTYPE_NONE;
	m_cSkyState.m_sVertexDecl = sVertexDecl;
	m_cSkyState.m_eLightingModel = m_sRenderInfo.m_sAppState.m_bDeferred ? LIGHTING_MODEL_DEFERRED : LIGHTING_MODEL_PER_PIXEL;
	m_cSkyState.m_apTextures[TL_DIFFUSE] = m_sRenderInfo.m_strCloudTextureFilename.c_str( );
	bSuccess &= m_cSkyState.InitGfx(sSkyAppState, aSearchPaths, 0, 1.0f, "Sky", "Sky", NULL);

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CSkyRI::InitSkyGeometryBuffer

CSkyRI_TemplateList
inline st_bool CSkyRI_t::InitSkyGeometryBuffer(const SVertexDecl& sVertexDecl)
{
	st_bool bSuccess = false;

	if (m_tSkyGeometryBuffer.SetVertexDecl(sVertexDecl, &m_cSkyState.GetTechnique( )))
	{
		bSuccess = true;

		// avoid excessive heap allocations by reserving needed space up front
		const st_int32 c_nSteps = 16;
		const st_int32 c_nNumVertices = c_nSteps * (c_nSteps + 1);
		const st_int32 c_nNumIndices = c_nSteps * c_nSteps * 6;
		bSuccess &= m_tSkyGeometryBuffer.Reserve(c_nNumVertices, c_nNumIndices);

		// vertex data for sky dome
		CStaticArray<float> aVB(c_nNumVertices * 5, "CSkyRI::InitSkyGeometryBuffer", true);

		// set sky box vertices
		{
			for (st_int32 i = 0; i <= c_nSteps; ++i)
			{
				st_float32 fPercentageX = st_float32(i) / st_float32(c_nSteps);
				st_float32 fAngle = c_fTwoPi * fPercentageX;
				Vec3 vec2D(cosf(fAngle), sinf(fAngle), 0.0f);

				for (st_int32 j = 0; j < c_nSteps; ++j)
				{
					st_float32 fPercentageY = st_float32(j) / st_float32(c_nSteps);
					fAngle = c_fPi * fPercentageY;
					const st_float32 c_fSin= sinf(fAngle);
					const st_float32 c_fCos = cosf(fAngle);

					Vec3 vecPos = CCoordSys::ConvertFromStd(vec2D.x * c_fSin, vec2D.y * c_fSin, c_fCos);

					st_uint32 uiBase = (i * c_nSteps + j) * 5;
					aVB[uiBase + 0] = vecPos.x;
					aVB[uiBase + 1] = vecPos.y;
					aVB[uiBase + 2] = vecPos.z;
					aVB[uiBase + 3] = fPercentageX;
					aVB[uiBase + 4] = fPercentageY * 2.0f - 1.0f;
				}
			}

			bSuccess &= m_tSkyGeometryBuffer.AppendVertices(&aVB[0], c_nNumVertices);
			bSuccess &= m_tSkyGeometryBuffer.EndVertices( );
		}

		// set sky box indices
		{
			st_uint16 auiIndices[c_nNumIndices];
			st_uint16* pIndex = auiIndices;
			for (st_int32 i = 0; i <= c_nSteps; ++i)
			{
				const st_int32 c_nBase = i * c_nSteps;
				const st_int32 c_nNextBase = ((i == c_nSteps) ? 0 : c_nBase + c_nSteps);

				for (st_int32 j = 0; j < c_nSteps - 1; ++j)
				{
					*pIndex++ = st_uint16(c_nBase + j);
					*pIndex++ = st_uint16(c_nBase + j + 1);
					*pIndex++ = st_uint16(c_nNextBase + j);
					*pIndex++ = st_uint16(c_nNextBase + j);
					*pIndex++ = st_uint16(c_nBase + j + 1);
					*pIndex++ = st_uint16(c_nNextBase + j + 1);
				}
			}

			bSuccess &= m_tSkyGeometryBuffer.SetIndexFormat(INDEX_FORMAT_UNSIGNED_16BIT);
			bSuccess &= m_tSkyGeometryBuffer.AppendIndices(auiIndices, c_nNumIndices);
			bSuccess &= m_tSkyGeometryBuffer.EndIndices( );
		}
	}

	return bSuccess;
}
