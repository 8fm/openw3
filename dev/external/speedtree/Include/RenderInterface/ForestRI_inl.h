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
// LAVA++
// GetLightMaterial

inline const SSimpleMaterial& GetLightMaterial( const SSimpleMaterial &currentMaterial, Bool allowGlobalMaterialSettings, Bool isBillboard )
{
	struct Local
	{
		static SSimpleMaterial BuildBillboard()
		{
			SSimpleMaterial mat;
			mat.m_vDiffuse.Set( 1.f, 1.f, 1.f );
			mat.m_vSpecular.Set( 1.f, 1.f, 1.f );
			mat.m_vTransmission.Set( 0.f, 0.f, 0.f );
			return mat;
		}

		static SSimpleMaterial BuildNonBillboard()
		{
			SSimpleMaterial mat;
			mat.m_vDiffuse.Set( 1.f, 1.f, 1.f );
			mat.m_vSpecular.Set( 1.f, 1.f, 1.f );
			mat.m_vTransmission.Set( 1.f, 1.f, 1.f );
			return mat;
		}
	};

	static SSimpleMaterial s_billboard = Local::BuildBillboard();
	static SSimpleMaterial s_nonBillboard = Local::BuildNonBillboard();
	return allowGlobalMaterialSettings ? currentMaterial : (isBillboard ? s_billboard : s_nonBillboard);
}
// LAVA--

///////////////////////////////////////////////////////////////////////
//  CForestRI::CForestRI

CForestRI_TemplateList
inline CForestRI_t::CForestRI( )
{
}


///////////////////////////////////////////////////////////////////////
//  CForestRI::~CForestRI

CForestRI_TemplateList
inline CForestRI_t::~CForestRI( )
{
}


///////////////////////////////////////////////////////////////////////
//  CForestRI::ReleaseGfxResources

CForestRI_TemplateList
inline void CForestRI_t::ReleaseGfxResources(void)
{
	m_tFizzleNoise.ReleaseGfxResources( );
	m_tPerlinNoiseKernel.ReleaseGfxResources( );
	m_tAmbientImageLighting.ReleaseGfxResources( );
	m_cFrameConstantBuffer.ReleaseGfxResources( );
	m_cFogAndSkyConstantBuffer.ReleaseGfxResources( );
}


///////////////////////////////////////////////////////////////////////
//  CForestRI::SetRenderInfo

CForestRI_TemplateList
inline void CForestRI_t::SetRenderInfo(const SForestRenderInfo& sInfo)
{
	m_sRenderInfo = sInfo;
}


///////////////////////////////////////////////////////////////////////
//  CForestRI::GetRenderInfo

CForestRI_TemplateList
inline const SForestRenderInfo& CForestRI_t::GetRenderInfo(void) const
{
	return m_sRenderInfo;
}


///////////////////////////////////////////////////////////////////////
//  CForestRI::InitGfx

CForestRI_TemplateList
inline st_bool CForestRI_t::InitGfx(void)
{
	st_bool bSuccess = false;

	// init shader constants system
	TShaderConstantClass::Init( );

	// alpha noise texture (or white if off)
	if (m_sRenderInfo.m_sAppState.m_bAlphaToCoverage)
		bSuccess = m_tFizzleNoise.LoadColor(0xffff33ff);
	else
		bSuccess = m_tFizzleNoise.LoadNoise(c_nNoiseTexWidth, c_nNoiseTexWidth);

	// noise
	bSuccess &= m_tPerlinNoiseKernel.LoadPerlinNoiseKernel(256, 256, 4);

	// image-based ambient lighting
	if (!m_sRenderInfo.m_strImageBasedAmbientLightingFilename.empty( ))
	{
		if (!m_tAmbientImageLighting.Load(m_sRenderInfo.m_strImageBasedAmbientLightingFilename.c_str( )))
		{
			CCore::SetError("Failed to load image based ambient lighting texture [%s]", m_sRenderInfo.m_strImageBasedAmbientLightingFilename.c_str( ));
			bSuccess = false;
		}
	}

	// shadow smoothing table
	const st_float32 c_fOffset = 1.0f / m_sRenderInfo.m_nShadowsResolution;
	m_avShadowSmoothingTable[0].Set(0.0f, c_fOffset, 0.0f, 0.0f);
	m_avShadowSmoothingTable[1].Set(-c_fOffset, -c_fOffset, 0.0f, 0.0f);
	m_avShadowSmoothingTable[2].Set(-c_fOffset, 0.0f, 0.0f, 0.0f); // todo: delete m_avShadowSmoothingTable

	// constant buffer
	bSuccess &= m_cFrameConstantBuffer.Init(&m_sFrameConstantBufferLayout, sizeof(m_sFrameConstantBufferLayout), CONST_BUF_REGISTER_FRAME);
	bSuccess &= m_cFogAndSkyConstantBuffer.Init(&m_sFogAndSkyConstantBufferLayout, sizeof(m_sFogAndSkyConstantBufferLayout), CONST_BUF_REGISTER_FOG_AND_SKY);

	// set a reserve capacity for the draw calls array to avoid allocations during render loop
	m_aSortedDrawCalls.reserve(300);
	
	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CForestRI::StartRender

CForestRI_TemplateList
inline st_bool CForestRI_t::StartRender(void)
{
	st_bool bSuccess = true;

	// set some of the shader constants that are shared by all shaders, regardless of configuration; start
	// by grabbing any one of the trees registered to the forest and use
	{
		// set up samplers
		TTextureClass::SetSamplerStates( );

		// reset shader constant optimizer
		TShaderConstantClass::Reset( );

		// fog/sky parameters (fog color can blend into sky color)
		bSuccess &= UpdateFogAndSkyConstantBuffer( );

		CRenderStateRI_t::ClearLastBoundTextures( );

		// shader constant buffers
		bSuccess &= m_cFrameConstantBuffer.Bind( );
		bSuccess &= m_cFogAndSkyConstantBuffer.Bind( );

		// bind noise textures
		bSuccess &= TShaderConstantClass::SetTexture(TEXTURE_REGISTER_FIZZLE_NOISE, m_tFizzleNoise, false);
		bSuccess &= TShaderConstantClass::SetTexture(TEXTURE_REGISTER_PERLIN_NOISE_KERNEL, m_tPerlinNoiseKernel, false);
		// LAVA++ ctremblay ++ : Should we handle that ?
		// bSuccess &= TShaderConstantClass::SetTexture(TEXTURE_REGISTER_IMAGE_BASED_AMBIENT_LIGHTING, m_tAmbientImageLighting, false);
		// LAVA-- ctremblay --
	}

	if (!bSuccess)
		CCore::SetError("CForestRI::StartRender() failed to set one or more shader constants");

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CForestRI::EndRender

CForestRI_TemplateList
inline st_bool CForestRI_t::EndRender(void)
{
	st_bool bSuccess = true;

	bSuccess &= CRenderStateRI_t::UnBind( );
	bSuccess &= TGeometryBufferClass::UnBindVertexBuffer( );
	bSuccess &= TGeometryBufferClass::UnBindIndexBuffer( );
	bSuccess &= TShaderTechniqueClass::UnBind( );

	CForest::FrameEnd( );

	return bSuccess;
}

// todo
#define UNIFIED_APPROACH

#ifdef UNIFIED_APPROACH

	///////////////////////////////////////////////////////////////////////
	//  CForestRI::Render3dTrees

	CForestRI_TemplateList
	inline st_bool CForestRI_t::Render3dTrees(ERenderPass ePass, const CVisibleInstancesRI_t& cVisibleInstances, st_bool bRenderOpaqueMaterials) const
	{
		ScopeTrace(ePass == RENDER_PASS_MAIN ? "CForestRI_t::Render3dTrees(Lit)" : "CForestRI_t::Render3dTrees(Depth-only)");

		st_bool bSuccess = true;

		// clear sorted draw call array
		m_aSortedDrawCalls.resize(0);

		// queue draw calls, but don't execute them yet
		{
			const TDetailedCullDataArray& aPerBase3dInstances = cVisibleInstances.Get3dInstanceLods( );
			for (st_int32 nBase = 0; nBase < st_int32(aPerBase3dInstances.size( )); ++nBase)
			{
				// get this base tree's instance list
				const SDetailedCullData& detailedCullData = aPerBase3dInstances[nBase];
				const T3dTreeInstanceLodArray* pInstanceLods = &detailedCullData.m_a3dInstanceLods;
				if (pInstanceLods->empty( ))
					continue;

				// access base tree as render interface type
				CTreeRI_t* pBaseTree = (CTreeRI_t*) aPerBase3dInstances[nBase].m_pBaseTree;
				assert(pBaseTree);

				// grab the instancing data for this base tree
				const typename CVisibleInstancesRI_t::SForestInstancingData* pInstancingData = cVisibleInstances.FindInstancingDataByBaseTree(pBaseTree);
				if (pInstancingData)
				{
					// access geometry
					const SGeometry* pGeometry = pBaseTree->GetGeometry( );
					assert(pGeometry);
					assert(static_cast<const SLod*>(pGeometry->m_pLods));

					// run through each possible LOD level, looking for instances that should be rendered at that LOD
					for (st_int32 nLod = 0; nLod < pGeometry->m_nNumLods; ++nLod)
					{
						// if there's nothing available at this LOD, move to the next
						if (pInstancingData->m_t3dTreeInstancingMgr.NumInstances(nLod) < 1)
							continue;

						const SLod* pLod = pGeometry->m_pLods + nLod;
						assert(pLod);

						// each instance may require several draw calls to render
						assert(static_cast<const SDrawCall*>(pLod->m_pDrawCalls));
						for (st_int32 nDrawCall = 0; nDrawCall < pLod->m_nNumDrawCalls; ++nDrawCall)
						{
							// geometry buffer
							const TGeometryBufferClass* pGeometryBuffer = pBaseTree->GetGeometryBuffer(nLod, nDrawCall);
							assert(pGeometryBuffer);

							// draw call
							const SDrawCall* pDrawCall = pLod->m_pDrawCalls + nDrawCall;
							if (pDrawCall->m_nNumVertices > 0 && pDrawCall->m_nNumIndices > 0)
							{
								// render state
								const CRenderStateRI_t* pRenderState = &pBaseTree->Get3dRenderStates(ePass)[pDrawCall->m_nRenderStateIndex];

								// skip this object if rendering only transparent textures and this one is opaque
								if (!bRenderOpaqueMaterials && pRenderState->m_bDiffuseAlphaMaskIsOpaque)
									continue;

								// skip this object if we're in a shadow-casting pass and this object is flagged as non-caster
								if (ePass == RENDER_PASS_SHADOW_CAST && !pRenderState->m_bCastsShadows)
									continue;

								// passed all conditions for skipping the render, so add it to the draw call queue
								SDrawCallData sDraw;
								sDraw.m_pBaseTree = pBaseTree;
								sDraw.m_nLod = nLod;
								sDraw.m_pRenderState = pRenderState;
								sDraw.m_pDrawCall = pDrawCall;
								sDraw.m_pGeometryBuffer = pGeometryBuffer;
								sDraw.m_nBufferOffset = pBaseTree->GetGeometryBufferOffset(nLod, nDrawCall);
								sDraw.m_pInstancingData = pInstancingData;
								//sDraw.m_closestInstance = Min( detailedCullData.m_fClosest3dTreeDistanceSquared, 256.0f);
								sDraw.m_closestInstance = detailedCullData.m_fClosest3dTreeDistanceSquared;
								m_aSortedDrawCalls.insert_sorted(sDraw);
							}
						}
					}
				}
			}
		}

		// execute draw calls
		st_int64 lCurrentSortKey = -1;
		for (int nDrawCall = 0; nDrawCall < st_int32(m_aSortedDrawCalls.size( )); ++nDrawCall)
		{
			const SDrawCallData& sDraw = m_aSortedDrawCalls[nDrawCall];

			// todo: one some platforms, this won't be so cheap
			(void) sDraw.m_pBaseTree->BindConstantBuffers( );
			(void) sDraw.m_pRenderState->BindConstantBuffer( );
			(void) sDraw.m_pRenderState->BindTextures(ePass, m_sRenderInfo.m_bTexturingEnabled ? TEXTURE_BIND_ENABLED : TEXTURE_BIND_FALLBACK, sDraw.m_closestInstance);

			if (lCurrentSortKey != sDraw.GetHashKey( ))
			{
				sDraw.m_pRenderState->BindShader( );
				sDraw.m_pRenderState->BindStateBlock( );

				lCurrentSortKey = sDraw.GetHashKey( );
			}

			SInstancedDrawStats sInstanceStats;
			bSuccess &= sDraw.m_pInstancingData->m_t3dTreeInstancingMgr.Render3dTrees(sDraw.m_nBufferOffset, sDraw.m_nLod, sInstanceStats, cVisibleInstances.GetInstanceRingBuffer() );
#ifndef RED_PLATFORM_FINAL
			SSpeedTreeRenderStats::s_treeDrawcalls[ SSpeedTreeRenderStats::s_bufferIndex ] += sInstanceStats.m_nNumDrawCalls;
			SSpeedTreeRenderStats::s_treesRendered[ SSpeedTreeRenderStats::s_bufferIndex ] += sInstanceStats.m_nNumInstancesDrawn;
#endif
		}

		return bSuccess;
	}

#else

	///////////////////////////////////////////////////////////////////////
	//  CForestRI::Render3dTrees

	CForestRI_TemplateList
	inline st_bool CForestRI_t::Render3dTrees(ERenderPass ePass, const CVisibleInstancesRI_t& cVisibleInstances, st_bool bRenderOpaqueMaterials) const
	{
		ScopeTrace(ePass == RENDER_PASS_MAIN ? "CForestRI_t::Render3dTrees(Lit)" : "CForestRI_t::Render3dTrees(Depth-only)");

		st_bool bSuccess = true;

		const TDetailedCullDataArray& aPerBase3dInstances = cVisibleInstances.Get3dInstanceLods( );
		for (st_int32 nBase = 0; nBase < st_int32(aPerBase3dInstances.size( )); ++nBase)
		{
			// get this base tree's instance list
			const T3dTreeInstanceLodArray* pInstanceLods = &aPerBase3dInstances[nBase].m_a3dInstanceLods;
			if (pInstanceLods->empty( ))
				continue;

			// access base tree as render interface type
			CTreeRI_t* pBaseTree = (CTreeRI_t*) aPerBase3dInstances[nBase].m_pBaseTree;

			// grab the instancing data for this base tree
			const typename CVisibleInstancesRI_t::SForestInstancingData* pInstancingData = cVisibleInstances.FindInstancingDataByBaseTree(pBaseTree);
			if (pInstancingData)
			{
				// stats tracking
				CRenderStats::SObjectStats& sObjStats = m_cRenderStats.GetObjectStats(pBaseTree->GetFilename( ));
				sObjStats.m_nNumInstances += st_int32(pInstanceLods->size( ));

				// access geometry
				const SGeometry* pGeometry = pBaseTree->GetGeometry( );
				assert(pGeometry);
				assert(static_cast<const SLod*>(pGeometry->m_pLods));

				// run through each possible LOD level, looking for instances that should be rendered at that LOD
				for (st_int32 nLod = 0; nLod < pGeometry->m_nNumLods; ++nLod)
				{
					// if there's nothing available at this LOD, move to the next
					if (pInstancingData->m_t3dTreeInstancingMgr.NumInstances(nLod) == 0)
						continue;

					const SLod* pLod = pGeometry->m_pLods + nLod;
					assert(pLod);

					// each instance may require several draw calls to render
					assert(static_cast<const SDrawCall*>(pLod->m_pDrawCalls));
					for (st_int32 nDrawCall = 0; nDrawCall < pLod->m_nNumDrawCalls; ++nDrawCall)
					{
						// geometry buffer
						const TGeometryBufferClass* pGeometryBuffer = pBaseTree->GetGeometryBuffer(nLod, nDrawCall);
						assert(pGeometryBuffer);

						// draw call
						const SDrawCall* pDrawCall = pLod->m_pDrawCalls + nDrawCall;
						if (pDrawCall->m_nNumVertices > 0 && pDrawCall->m_nNumIndices > 0)
						{
							// render state
							const CRenderStateRI_t* pRenderState = &pBaseTree->Get3dRenderStates(ePass)[pDrawCall->m_nRenderStateIndex];

							// skip this object if rendering only transparent textures and this one is opaque
							if (!bRenderOpaqueMaterials && pRenderState->m_bDiffuseAlphaMaskIsOpaque)
								continue;

							// skip this object if we're in a shadow-casting pass and this object is flagged as non-caster
							if (ePass == RENDER_PASS_SHADOW_CAST && !pRenderState->m_bCastsShadows)
								continue;

							assert(pInstancingData);
							if (pRenderState->BindMaterialWhole(ePass, m_sRenderInfo.m_bTexturingEnabled ? TEXTURE_BIND_ENABLED : TEXTURE_BIND_FALLBACK))
							{
								st_int32 nBufferOffset = pBaseTree->GetGeometryBufferOffset(nLod, nDrawCall);
								SInstancedDrawStats sInstanceStats;
								bSuccess &= pInstancingData->m_t3dTreeInstancingMgr.Render3dTrees(nBufferOffset, nLod, sInstanceStats);

								// update render statistics
								sObjStats.m_nNumDrawCalls += sInstanceStats.m_nNumDrawCalls;
								st_int32 nNumTriangles = st_int32(pGeometryBuffer->NumIndices( ) / 3) * sInstanceStats.m_nNumInstancesDrawn;
								sObjStats.m_nNumTrianglesRendered += nNumTriangles;
							}
						}
					}
				}
			}
		}

		return bSuccess;
	}

#endif


///////////////////////////////////////////////////////////////////////
//  CForestRI::RenderGrass

CForestRI_TemplateList
inline st_bool CForestRI_t::RenderGrass(ERenderPass ePass, const CTreeRI_t* pBaseGrass, const CVisibleInstancesRI_t& cVisibleInstances/*LAVA++*/, CSpeedTreeInstanceRingBuffer& instancingRingBuffer ) const
{
	ScopeTrace(ePass == RENDER_PASS_MAIN ? "CForestRI_t::RenderGrass(Lit)" : "CForestRI_t::RenderGrass(Depth-only)");

	st_bool bSuccess = true;

	// grab the instancing data for this base tree
	const typename CVisibleInstancesRI_t::SForestInstancingData* pInstancingData = cVisibleInstances.FindInstancingDataByBaseTree(pBaseGrass);
	if (pInstancingData && pInstancingData->m_t3dTreeInstancingMgr.NumInstances( ) > 0)
	{
		// access geometry
		const SGeometry* pGeometry = pBaseGrass->GetGeometry( );
		assert(pGeometry);
		assert(static_cast<const SLod*>(pGeometry->m_pLods));

		// always render the highest LOD (should really only be one for grass models anyway)
		const SLod* pLod = pGeometry->m_pLods + 0;
		assert(pLod);

		// for grass, we expect at most one draw call
		assert(static_cast<const SDrawCall*>(pLod->m_pDrawCalls));
		if (pLod->m_nNumDrawCalls > 0)
		{
			const SDrawCall* pDrawCall = pLod->m_pDrawCalls + 0;
			if (pDrawCall->m_nNumVertices > 0 && pDrawCall->m_nNumIndices > 0)
			{
				// geometry buffer
				const TGeometryBufferClass* pGeometryBuffer = pBaseGrass->GetGeometryBuffer(0, 0);
				assert(pGeometryBuffer);

				// render state
				const CRenderStateRI_t* pRenderState = &pBaseGrass->Get3dRenderStates(ePass)[pDrawCall->m_nRenderStateIndex];

				if (pBaseGrass->BindConstantBuffers( ) &&
					pRenderState->BindMaterialWhole(ePass, m_sRenderInfo.m_bTexturingEnabled ? TEXTURE_BIND_ENABLED : TEXTURE_BIND_FALLBACK, 1.0f))
				{
					st_int32 nBufferOffset = pBaseGrass->GetGeometryBufferOffset(0, 0);

					SInstancedDrawStats sInstanceStats;
					bSuccess &= pInstancingData->m_t3dTreeInstancingMgr.RenderGrass(nBufferOffset, sInstanceStats, instancingRingBuffer);
#ifndef RED_PLATFORM_FINAL
					SSpeedTreeRenderStats::s_grassDrawcalls[ SSpeedTreeRenderStats::s_bufferIndex ] += sInstanceStats.m_nNumDrawCalls;
					SSpeedTreeRenderStats::s_grassRendered[ SSpeedTreeRenderStats::s_bufferIndex ] += sInstanceStats.m_nNumInstancesDrawn;
#endif
				}
			}
		}
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CForestRI::RenderBillboards

CForestRI_TemplateList
inline st_bool CForestRI_t::RenderBillboards(ERenderPass ePass, const CVisibleInstancesRI_t& cVisibleInstances) const
{
	ScopeTrace(ePass == RENDER_PASS_MAIN ? "CForestRI_t::RenderBillboards(Lit)" : "CForestRI_t::RenderBillboards(Depth-only)");

	st_bool bSuccess = true;

	// get data structure that contains the base trees with their associated billboard instancing structs
	//const typename CVisibleInstancesRI_t::TInstanceDataPtrArray& aPerBaseBillboardInstancingData = cVisibleInstances.GetPerBaseInstancingData( );
	const typename CVisibleInstancesRI_t::TInstanceDataPtrMap& mPerBaseBillboardInstancingDataMap = cVisibleInstances.GetPerBaseInstancingDataMap( );

	// run through each base tree, rendering its billboards
	for (typename CVisibleInstancesRI_t::TInstanceDataPtrMap::const_iterator i = mPerBaseBillboardInstancingDataMap.begin( ); i != mPerBaseBillboardInstancingDataMap.end( ); ++i)
		//for (st_int32 nBaseTree = 0; nBaseTree < st_int32(aPerBaseBillboardInstancingData.size( )); ++nBaseTree)
	{
		// get base tree
		const CTreeRI_t* pBaseTree = (const CTreeRI_t*) i->first;
		assert(pBaseTree);

		const SDetailedCullData* detailedCullData = (const_cast<CVisibleInstancesRI_t&>(cVisibleInstances)).GetInstaceLodArrayByBase(pBaseTree);
		
		// get instance manager
		const TInstancingMgrClass& tBillboardIM = i->second->m_tBillboardInstancingMgr;

		// if there's no billboards for this base true, just move to the next
		if (tBillboardIM.NumInstances( ) == 0)
			continue;
		assert(tBillboardIM.IsInitialized( ));

		const CRenderStateRI_t& c_sRenderState = pBaseTree->GetBillboardRenderState(ePass);
		//const Float bindDistance = Min( detailedCullData->m_fClosestBillboardCellDistanceSquared, 256.0f);
		//const Float bindDistance = Min( Min( detailedCullData->m_fClosestBillboardCellDistanceSquared, detailedCullData->m_fClosest3dTreeDistanceSquared ), 10000.0f );
		// Billboards always bind at a close distance. Otherwise they will be considered low priority pretty much always, because
		// they're always rendered at a distance.
		const Float bindDistance = 25.0f;
		if (c_sRenderState.BindMaterialWhole(ePass, m_sRenderInfo.m_bTexturingEnabled ? TEXTURE_BIND_ENABLED : TEXTURE_BIND_FALLBACK, bindDistance ) &&
			pBaseTree->BindConstantBuffers( ))
		{
			// render billboard geometry for this base tree and retrieve stats
			SInstancedDrawStats sDrawStats;
			bSuccess &= tBillboardIM.RenderBillboards(sDrawStats, cVisibleInstances.GetInstanceRingBuffer());
#ifndef RED_PLATFORM_FINAL
			SSpeedTreeRenderStats::s_billboardDrawcalls[ SSpeedTreeRenderStats::s_bufferIndex ] += sDrawStats.m_nNumDrawCalls;
			SSpeedTreeRenderStats::s_billboardsRendered[ SSpeedTreeRenderStats::s_bufferIndex ] += sDrawStats.m_nNumInstancesDrawn;
#endif
		}
		else
			bSuccess = false;
	}

	return bSuccess;
}


////////////////////////////////////////////////////////////
//	CForestRI::GetFrameConstantBuffer

CForestRI_TemplateList
inline TShaderConstantBufferClass& CForestRI_t::GetFrameConstantBuffer(void)
{
	return m_cFrameConstantBuffer;
}


////////////////////////////////////////////////////////////
//	CForestRI::GetFrameConstantBufferContents

CForestRI_TemplateList
inline SFrameCBLayout& CForestRI_t::GetFrameConstantBufferContents(void)
{
	return m_sFrameConstantBufferLayout;
}


////////////////////////////////////////////////////////////
//	CForestRI::GetFogAndSkyBufferContents

CForestRI_TemplateList
inline SFogAndSkyCBLayout& CForestRI_t::GetFogAndSkyBufferContents(void) const
{
	return m_sFogAndSkyConstantBufferLayout;
}


////////////////////////////////////////////////////////////
//	CForestRI::UpdateFrameConstantBuffer
//
//	These shader parameters are all dependent on the current view, so expect
//	this function to be called multiple times in a multipass render.

CForestRI_TemplateList
inline st_bool CForestRI_t::UpdateFrameConstantBuffer(const CView& cView, st_int32 nWindowWidth, st_int32 nWindowHeight)
{
	bool bSuccess = true;

	// projections
	m_sFrameConstantBufferLayout.m_mModelViewProj3d = cView.GetCompositeNoTranslate( ).Transpose( ); // todo: can we do away with this transpose?
	m_sFrameConstantBufferLayout.m_mProjectionInverse3d = cView.GetProjectionInverse( ).Transpose( );
	m_sFrameConstantBufferLayout.m_mCameraFacingMatrix = cView.GetCameraFacingMatrix( ).Transpose( ); // todo: test

	// other camera
	m_sFrameConstantBufferLayout.m_vCameraPosition = cView.GetCameraPos( );
	
	// LAVA++
	// Drey: The camera direction is reversed in the sdk, we don't want that (this is likely due to axes switch in our view transform)
	//m_sFrameConstantBufferLayout.m_vCameraDirection = cView.GetCameraDir( );
	m_sFrameConstantBufferLayout.m_vCameraDirection = -cView.GetCameraDir( );
	// LAVA--

	m_sFrameConstantBufferLayout.m_vLodRefPosition = cView.GetLodRefPoint( );

	m_sFrameConstantBufferLayout.m_vViewport = Vec2(st_float32(nWindowWidth), st_float32(nWindowHeight));
	m_sFrameConstantBufferLayout.m_vViewportInverse = Vec2(1.0f / m_sFrameConstantBufferLayout.m_vViewport.x, 1.0f / m_sFrameConstantBufferLayout.m_vViewport.y);
	m_sFrameConstantBufferLayout.m_fFarClip = cView.GetFarClip( );

	// LAVA++ Why would they not do that ... that's beyond me
	// Dir light
	m_sFrameConstantBufferLayout.m_sDirLight.m_vDiffuse = m_sRenderInfo.m_sLightMaterial.m_vDiffuse;
	m_sFrameConstantBufferLayout.m_sDirLight.m_vSpecular = m_sRenderInfo.m_sLightMaterial.m_vSpecular;
	m_sFrameConstantBufferLayout.m_sDirLight.m_vTransmission = m_sRenderInfo.m_sLightMaterial.m_vTransmission;
	// LAVA--

	// misc
	m_sFrameConstantBufferLayout.m_fWallTime = CForest::WindGetGlobalTime( );

	// shadows
	if (m_sRenderInfo.m_bShadowsEnabled)
	{
		// convert ranges in world units to ranges in [0,1] range by dividing my far clip value;
		// unused maps must default to 1.0f
		for (st_int32 nMap = 0; nMap < m_sRenderInfo.m_nShadowsNumMaps; ++nMap)
			m_sFrameConstantBufferLayout.m_sShadows.m_vMapRanges[nMap] = (nMap < m_sRenderInfo.m_nShadowsNumMaps) ? m_sRenderInfo.m_afShadowMapRanges[nMap] / m_sRenderInfo.m_fFarClip : 1.0f;

		assert(m_sRenderInfo.m_nShadowsNumMaps > 0);
		const st_float32 c_fFurthestShadowPercent = m_sFrameConstantBufferLayout.m_sShadows.m_vMapRanges[m_sRenderInfo.m_nShadowsNumMaps - 1]; // shadow fully faded by this distance
		m_sFrameConstantBufferLayout.m_sShadows.m_fFadeStartPercent = m_sRenderInfo.m_fShadowFadePercent * c_fFurthestShadowPercent;
		m_sFrameConstantBufferLayout.m_sShadows.m_fFadeInverseDistance = 1.0f / (c_fFurthestShadowPercent - m_sFrameConstantBufferLayout.m_sShadows.m_fFadeStartPercent);
		m_sFrameConstantBufferLayout.m_sShadows.m_fTerrainAmbientOcclusion = 1.0f / (m_sFrameConstantBufferLayout.m_sShadows.m_fFadeStartPercent - c_fFurthestShadowPercent);

		const st_float32 c_fOffset = 1.0f / m_sRenderInfo.m_nShadowsResolution;
		m_sFrameConstantBufferLayout.m_sShadows.m_avSmoothingTable[0].Set(0.0f, c_fOffset, 0.0f, 0.0f);
		m_sFrameConstantBufferLayout.m_sShadows.m_avSmoothingTable[1].Set(-c_fOffset, -c_fOffset, 0.0f, 0.0f);
		m_sFrameConstantBufferLayout.m_sShadows.m_avSmoothingTable[2].Set(-c_fOffset, 0.0f, 0.0f, 0.0f);

		m_sFrameConstantBufferLayout.m_sShadows.m_fShadowMapWritingActive = 0.0f; // todo
		m_sFrameConstantBufferLayout.m_sShadows.m_vTexelOffset.Set(0.5f / m_sRenderInfo.m_nShadowsResolution, 0.5f / m_sRenderInfo.m_nShadowsResolution);
	}

	bSuccess &= m_cFrameConstantBuffer.Update( );

	return bSuccess;
}


////////////////////////////////////////////////////////////
//	CForestRI::UpdateFogAndSkyConstantBuffer

CForestRI_TemplateList
inline st_bool CForestRI_t::UpdateFogAndSkyConstantBuffer(void)
{
	bool bSuccess = true;

	m_sFogAndSkyConstantBufferLayout.m_vFogColor = m_sRenderInfo.m_vFogColor;
	m_sFogAndSkyConstantBufferLayout.m_fFogDensity = m_sRenderInfo.m_fFogDensity;
	m_sFogAndSkyConstantBufferLayout.m_fFogEndDist = m_sRenderInfo.m_fFogEndDistance;
	m_sFogAndSkyConstantBufferLayout.m_fFogSpan = (m_sRenderInfo.m_fFogEndDistance - m_sRenderInfo.m_fFogStartDistance);
	m_sFogAndSkyConstantBufferLayout.m_vSkyColor = m_sRenderInfo.m_vSkyColor;
	m_sFogAndSkyConstantBufferLayout.m_vSunColor = m_sRenderInfo.m_vSunColor;
	m_sFogAndSkyConstantBufferLayout.m_fSunSize = m_sRenderInfo.m_fSunSize;
	m_sFogAndSkyConstantBufferLayout.m_fSunSpreadExponent = m_sRenderInfo.m_fSunSpreadExponent;

	bSuccess &= m_cFogAndSkyConstantBuffer.Update( );

	return bSuccess;
}

///////////////////////////////////////////////////////////////////////
//  CForestRI::SDrawCallData::operator<

CForestRI_TemplateList
inline st_bool CForestRI_t::SDrawCallData::operator<(const SDrawCallData& sRight) const
{
	// sort for branch type, then base tree, then lowest LOD
	if (GetHashKey( ) == sRight.GetHashKey( ))
		return (m_nLod < sRight.m_nLod);
	else
		return (GetHashKey( ) < sRight.GetHashKey( ));
}


///////////////////////////////////////////////////////////////////////
//  CForestRI::SDrawCallData::GetHashKey

CForestRI_TemplateList
inline st_int64 CForestRI_t::SDrawCallData::GetHashKey(void) const
{
	assert(m_pRenderState);

	return m_pRenderState->GetHashKey( );
}

