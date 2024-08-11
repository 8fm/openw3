///////////////////////////////////////////////////////////////////////  
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
//  STerrainRenderInfo::STerrainRenderInfo

inline STerrainRenderInfo::STerrainRenderInfo( ) :
	m_fNormalMapBlueScalar(1.0f),
	m_bShadowsEnabled(false),
	m_bCastShadows(false),
	m_nNumLodLevels(5),
	m_nMaxTerrainRes(33),
	m_fCellSize(500.0f),
	m_bDepthOnlyPrepass(false),
	m_nMaxAnisotropy(1),
	m_bTexturingEnabled(true)
{
	for (st_int32 nSplatLayer = 0; nSplatLayer < c_nNumTerrainSplatLayers; ++nSplatLayer)
		m_afSplatTileValues[nSplatLayer] = 1.0f;
}


///////////////////////////////////////////////////////////////////////  
//  CTerrainRI::CTerrainRI

CTerrainRI_TemplateList
inline CTerrainRI_t::CTerrainRI( ) :
	  m_nMaxAnisotropy(1)
    , m_tIndexBuffer(false, false, "terrain")
	, m_pVertexDecl(NULL)
{
	m_aDynamicVboPool.SetHeapDescription("Terrain dynamic VBO pool");
	m_aOriginalVboPool.SetHeapDescription("Terrain original VBO pool");
}


///////////////////////////////////////////////////////////////////////  
//  CTerrainRI::~CTerrainRI

CTerrainRI_TemplateList
inline CTerrainRI_t::~CTerrainRI( )
{
	// release VBO pool
	for (st_int32 i = 0; i < st_int32(m_aOriginalVboPool.size( )); ++i)
		st_delete<TGeometryBufferClass>(m_aOriginalVboPool[i]);
}


///////////////////////////////////////////////////////////////////////  
//  CTerrainRI::SetRenderInfo

CTerrainRI_TemplateList
inline void CTerrainRI_t::SetRenderInfo(const STerrainRenderInfo& sInfo)
{
	m_sRenderInfo = sInfo;

}


///////////////////////////////////////////////////////////////////////  
//  CTerrainRI::GetRenderInfo

CTerrainRI_TemplateList
inline const STerrainRenderInfo& CTerrainRI_t::GetRenderInfo(void) const
{
	return m_sRenderInfo;
}


///////////////////////////////////////////////////////////////////////  
//  CTerrainRI::SetMaxAnisotropy

CTerrainRI_TemplateList
inline void CTerrainRI_t::SetMaxAnisotropy(st_int32 nMaxAnisotropy)
{
	m_nMaxAnisotropy = nMaxAnisotropy;
}


///////////////////////////////////////////////////////////////////////  
//  CTerrainRI::InitGfx

CTerrainRI_TemplateList
inline st_bool CTerrainRI_t::InitGfx(st_int32 nNumLods, st_int32 nMaxTileRes, st_float32 fCellSize, const SVertexDecl& sVertexDecl)
{
	st_bool bSuccess = true;

	// copy vertex declaration, need for render state init and VB creation later
	m_pVertexDecl = &sVertexDecl;

	// load fallback grey texture (for use when texturing is 'disabled' or textures don't load correctly)
	if (!m_tGreyTexture.IsValid( ))
		bSuccess &= m_tGreyTexture.LoadColor(0x666666ff);

	// render states
	bSuccess &= InitRenderStates( );

	// if cells exist, then this is a subsequent InitGfx() call and we need to destroy the terrain cells,
	// saving off their respective VBOs in the process
	if (bSuccess && !m_cTerrainCellMap.empty( ))
	{
		for (TTerrainCellMap::iterator iMap = m_cTerrainCellMap.begin( ); iMap != m_cTerrainCellMap.end( ); ++iMap)
		{
			CTerrainCell* pCell = &iMap->second;
			assert(pCell);
			TGeometryBufferClass* pVbo = (TGeometryBufferClass*) pCell->GetVbo( );
			assert(pVbo);

			m_aDynamicVboPool.push_back(pVbo);
		}
	}

	// init graphics-independent portion from Forest library
	if (bSuccess)
		bSuccess &= CTerrain::InitGeometry(nNumLods, nMaxTileRes, fCellSize);

	// vbos, allocated ahead of time
	if (bSuccess && m_aDynamicVboPool.empty( ) && m_cTerrainCellMap.empty( ))
		bSuccess &= InitVboPool( );

	// initialize the single index buffer that all terrain tiles will share
	if (bSuccess)
		bSuccess &= InitIndexBuffer( );

	// constant buffer
	if (bSuccess)
	{
		if (m_cConstantBuffer.Init(&m_sConstantBufferLayout, sizeof(m_sConstantBufferLayout), CONST_BUF_REGISTER_TERRAIN))
		{	
			// these values won't change again
			m_sConstantBufferLayout.m_vSplatTiles = Vec3(m_sRenderInfo.m_afSplatTileValues);
			m_sConstantBufferLayout.m_fNormalMapBlueScalar = m_sRenderInfo.m_fNormalMapBlueScalar;

			bSuccess &= m_cConstantBuffer.Update( );
		}
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CTerrainRI::ReleaseGfxResources

CTerrainRI_TemplateList
inline void CTerrainRI_t::ReleaseGfxResources(void)
{
	// release index buffer
	m_tIndexBuffer.ReleaseGfxResources( );

	// release VBO pool
	for (size_t i = 0; i < m_aOriginalVboPool.size( ); ++i)
		m_aOriginalVboPool[i]->ReleaseGfxResources( );

	// release render states
	for (st_int32 nPass = 0; nPass < RENDER_PASS_COUNT; ++nPass)
		m_aRenderStates[nPass].ReleaseGfxResources( );

	// release textures
	m_tGreyTexture.ReleaseGfxResources( );

	// release constant buffer
	m_cConstantBuffer.ReleaseGfxResources( );
}


///////////////////////////////////////////////////////////////////////  
//  CTerrainRI::Render

CTerrainRI_TemplateList
inline st_bool CTerrainRI_t::Render(ERenderPass ePass,
									const STerrainCullResults& sCullResults, 
									const SSimpleMaterial& sLighting,
									CRenderStats& cRenderStats)
{
	ScopeTrace(ePass == RENDER_PASS_MAIN ? "CTerrainRI_t::Render(Lit)" : "CTerrainRI_t::Render(Depth-only");

	st_bool bSuccess = true;

	// first and foremost, see if any of the terrain is visible
	if (!sCullResults.m_aVisibleCells.empty( ))
	{
		// stats tracking
		CRenderStats::SObjectStats& cStats = cRenderStats.GetObjectStats("Terrain");

		if (StartRender(ePass, sLighting))
		{
			// enable the master index buffer
			bSuccess &= m_tIndexBuffer.BindIndexBuffer( );

			// enable the vertex format
			TGeometryBufferClass* pFirstVbo = NULL;
			for (st_int32 nCell = 0; nCell < st_int32(sCullResults.m_aVisibleCells.size( )); ++nCell)
			{
				pFirstVbo = (TGeometryBufferClass*) sCullResults.m_aVisibleCells[nCell]->GetVbo( );
				if (pFirstVbo)
					break;
			}
			if (pFirstVbo)
				bSuccess &= pFirstVbo->EnableFormat( );

			// render each terrain cell
			for (st_int32 nCell = 0; nCell < st_int32(sCullResults.m_aVisibleCells.size( )); ++nCell)
			{
				const CTerrainCell* pCell = sCullResults.m_aVisibleCells[nCell];
				assert(pCell);

				if (pCell->IsPopulated( ))
				{
					TGeometryBufferClass* pVbo = (TGeometryBufferClass*) pCell->GetVbo( );

					if (pVbo)
					{
						if (pVbo->BindVertexBuffer( ))
						{
							// query offset into master index buffer
							st_uint32 uiOffset = 0, uiNumIndices = 0, uiMinIndex = 0, uiNumVertices = 0;
							pCell->GetIndices(uiOffset, uiNumIndices, uiMinIndex, uiNumVertices);

							// render the tile at the correct LOD and transitional borders
							m_tIndexBuffer.RenderIndexed(PRIMITIVE_TRIANGLES, uiOffset, uiNumIndices, uiMinIndex, uiNumVertices);

							cStats.m_nNumDrawCalls += 1;
							cStats.m_nNumTrianglesRendered += uiNumIndices - 2;

							bSuccess &= pVbo->UnBindVertexBuffer( );
						}
						else
						{
							CCore::SetError("CTerrainRI::Render, BindVertexBuffer() failed");
							bSuccess = false;
						}
					}
				}
			}

			// with the render loop complete, unbind the index buffer & disable the vbo vertex format
			bSuccess &= m_tIndexBuffer.UnBindIndexBuffer( );
			bSuccess &= TGeometryBufferClass::DisableFormat( );
		}
		else
		{
			CCore::SetError("CTerrainRI::Render, StartRender() failed");
			bSuccess = false;
		}

		// final state changes, if any
		bSuccess &= EndRender( );
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CTerrainRI::CullAndComputeLOD

CTerrainRI_TemplateList
inline void CTerrainRI_t::CullAndComputeLOD(const CView& cView, st_int32 nFrameIndex, STerrainCullResults& sCullResults)
{
	// determine which cells are visible and which need to be populated
	CTerrain::CullAndComputeLOD(cView, nFrameIndex, sCullResults);

	// add the newly-available VBOs to the list of available VBOs
	st_int32 i = 0;
	for (i = 0; i < st_int32(sCullResults.m_aFreedVbos.size( )); ++i)
		m_aDynamicVboPool.push_back((TGeometryBufferClass*) sCullResults.m_aFreedVbos[i]);

	// assign VBOs to the newly-created cells
	for (i = 0; i < st_int32(sCullResults.m_aCellsToUpdate.size( )); ++i)
		sCullResults.m_aCellsToUpdate[i]->SetVbo((void*) NextVbo( ));
}


///////////////////////////////////////////////////////////////////////  
//  CTerrainRI::InitIndexBuffer

CTerrainRI_TemplateList
inline st_bool CTerrainRI_t::InitIndexBuffer(void)
{
	bool bSuccess = false;

	const CArray<st_uint16>& aCompositeIndices = GetCompositeIndices( );
	if (!aCompositeIndices.empty( ))
	{
		m_tIndexBuffer.SetIndexFormat(INDEX_FORMAT_UNSIGNED_16BIT);
		bSuccess = m_tIndexBuffer.AppendIndices((const st_byte*) &aCompositeIndices[0], st_uint32(aCompositeIndices.size( )));
		bSuccess &= m_tIndexBuffer.EndIndices( );
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CTerrainRI::InitRenderStates

CTerrainRI_TemplateList
inline st_bool CTerrainRI_t::InitRenderStates(void)
{
	bool bSuccess = false;

	// build search path based on path of splat map texture filename
	CStaticArray<CFixedString> aSearchPaths(3, "CTerrainRI_t::InitRenderStates");
	aSearchPaths[0] = m_sRenderInfo.m_strShaderPath + c_szFolderSeparator + TShaderTechniqueClass::GetCompiledShaderFolder( );
	aSearchPaths[1] = m_sRenderInfo.m_strShaderPath;
	aSearchPaths[2] = m_sRenderInfo.m_strSplatMap.Path( );

	// init main render state
	m_aRenderStates[RENDER_PASS_MAIN].m_eLightingModel = m_sRenderInfo.m_sAppState.m_bDeferred ? LIGHTING_MODEL_DEFERRED : LIGHTING_MODEL_PER_PIXEL;
	m_aRenderStates[RENDER_PASS_MAIN].m_vAmbientColor = m_sRenderInfo.m_vMaterialAmbient;
	m_aRenderStates[RENDER_PASS_MAIN].m_vDiffuseColor = m_sRenderInfo.m_vMaterialDiffuse;
	m_aRenderStates[RENDER_PASS_MAIN].m_eFaceCulling = CCoordSys::IsLeftHanded( ) ? CULLTYPE_FRONT : CULLTYPE_BACK;
	m_aRenderStates[RENDER_PASS_MAIN].m_apTextures[TEXTURE_REGISTER_TERRAIN_SPLAT] = m_sRenderInfo.m_strSplatMap.c_str( );
	m_aRenderStates[RENDER_PASS_MAIN].m_apTextures[TEXTURE_REGISTER_TERRAIN_NORMAL] = m_sRenderInfo.m_strNormalMap.c_str( );
	m_aRenderStates[RENDER_PASS_MAIN].m_apTextures[TEXTURE_REGISTER_TERRAIN_SPLAT_LAYER_0] = m_sRenderInfo.m_astrSplatLayers[0].c_str( );
	m_aRenderStates[RENDER_PASS_MAIN].m_apTextures[TEXTURE_REGISTER_TERRAIN_SPLAT_LAYER_1] = m_sRenderInfo.m_astrSplatLayers[1].c_str( );
	m_aRenderStates[RENDER_PASS_MAIN].m_apTextures[TEXTURE_REGISTER_TERRAIN_SPLAT_LAYER_2] = m_sRenderInfo.m_astrSplatLayers[2].c_str( );
	m_aRenderStates[RENDER_PASS_MAIN].m_bDiffuseAlphaMaskIsOpaque = true;
	m_aRenderStates[RENDER_PASS_MAIN].m_bFadeToBillboard = false;
	assert(m_pVertexDecl);
	m_aRenderStates[RENDER_PASS_MAIN].m_sVertexDecl = *m_pVertexDecl;

	bSuccess = m_aRenderStates[RENDER_PASS_MAIN].InitGfx(m_sRenderInfo.m_sAppState, aSearchPaths, m_nMaxAnisotropy, 1.0f, "Terrain", "Terrain", NULL);

	// shadow-casting render state
	if (m_sRenderInfo.m_bCastShadows)
	{
		m_aRenderStates[RENDER_PASS_SHADOW_CAST] = m_aRenderStates[RENDER_PASS_MAIN];
		m_aRenderStates[RENDER_PASS_SHADOW_CAST].MakeDepthOnly( );
		m_aRenderStates[RENDER_PASS_SHADOW_CAST].m_eRenderPass = RENDER_PASS_SHADOW_CAST;
		m_aRenderStates[RENDER_PASS_SHADOW_CAST].m_eFaceCulling = CULLTYPE_NONE;

		bSuccess &= m_aRenderStates[RENDER_PASS_SHADOW_CAST].InitGfx(m_sRenderInfo.m_sAppState, aSearchPaths, m_nMaxAnisotropy, 1.0f, "Terrain_do", "Terrain_do", NULL);
	}

	// depth-only render state
	if (m_sRenderInfo.m_bDepthOnlyPrepass)
	{
		m_aRenderStates[RENDER_PASS_DEPTH_PREPASS] = m_aRenderStates[RENDER_PASS_MAIN];
		m_aRenderStates[RENDER_PASS_DEPTH_PREPASS].MakeDepthOnly( );

		bSuccess &= m_aRenderStates[RENDER_PASS_DEPTH_PREPASS].InitGfx(m_sRenderInfo.m_sAppState, aSearchPaths, m_nMaxAnisotropy, 1.0f, "Terrain_do", "Terrain_do", NULL);
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CTerrainRI::StartRender

CTerrainRI_TemplateList
inline st_bool CTerrainRI_t::StartRender(ERenderPass ePass, const SSimpleMaterial& sLighting)
{
	st_bool bSuccess = true;

	ST_UNREF_PARAM(sLighting); // todo

	bSuccess &= m_cConstantBuffer.Bind( );

	bSuccess &= m_aRenderStates[ePass].BindMaterialWhole(ePass, m_sRenderInfo.m_bTexturingEnabled ? TEXTURE_BIND_ENABLED : TEXTURE_BIND_FALLBACK);

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CTerrainRI::EndRender

CTerrainRI_TemplateList
inline st_bool CTerrainRI_t::EndRender(void) const
{
	st_bool bSuccess = true;

	bSuccess &= CRenderStateRI_t::UnBind( );
	bSuccess &= TGeometryBufferClass::UnBindVertexBuffer( );
	bSuccess &= TGeometryBufferClass::UnBindIndexBuffer( );

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CTerrainRI::InitVboPool

CTerrainRI_TemplateList
inline st_bool CTerrainRI_t::InitVboPool(void)
{
	st_bool bSuccess = false;

	if (m_sHeapReserves.m_nMaxVisibleTerrainCells > 0)
	{
		m_aDynamicVboPool.resize(m_sHeapReserves.m_nMaxVisibleTerrainCells);
		for (st_int32 i = 0; i < m_sHeapReserves.m_nMaxVisibleTerrainCells; ++i)
			m_aDynamicVboPool[i] = NewVbo( );

		m_aOriginalVboPool = m_aDynamicVboPool;

		bSuccess = true;
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CTerrainRI::NextVbo

CTerrainRI_TemplateList
inline TGeometryBufferClass* CTerrainRI_t::NextVbo(void)
{
	TGeometryBufferClass* pVbo = NULL;

	if (m_aDynamicVboPool.empty( ))
		pVbo = NewVbo( );
	else
	{
		pVbo = m_aDynamicVboPool.back( );
		m_aDynamicVboPool.pop_back( );
	}

	return pVbo;
}


///////////////////////////////////////////////////////////////////////  
//  CTerrainRI::NewVbo

CTerrainRI_TemplateList
inline TGeometryBufferClass* CTerrainRI_t::NewVbo(void) const
{
	TGeometryBufferClass* pVbo = NULL;
	pVbo = st_new(TGeometryBufferClass, "CTerrainRI::TGeometryBufferClass")(true, false, "terrain_vbo_pool");

	assert(m_pVertexDecl);
	pVbo->SetVertexDecl(*m_pVertexDecl, &m_aRenderStates[RENDER_PASS_MAIN].GetTechnique( ));

	// create empty VB to fill in later
	st_uint32 uiNumVertices = st_uint32(GetMaxTileRes( ) * GetMaxTileRes( ));
	pVbo->CreateUninitializedVertexBuffer(uiNumVertices);

	return pVbo;
}


