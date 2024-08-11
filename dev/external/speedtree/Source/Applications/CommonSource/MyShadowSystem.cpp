///////////////////////////////////////////////////////////////////////
//  MyShadowSystem.cpp
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

#include "MyShadowSystem.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  CMyShadowSystem::CMyShadowSystem

CMyShadowSystem::CMyShadowSystem( ) :
	m_bShadowMapsReady(false)
{
}


///////////////////////////////////////////////////////////////////////  
//  CMyShadowSystem::~CMyShadowSystem

CMyShadowSystem::~CMyShadowSystem( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CMyShadowSystem::SetHeapReserves

void CMyShadowSystem::SetHeapReserves(const SHeapReserves& sHeapReserves)
{
	for (st_int32 nMap = 0; nMap < sHeapReserves.m_nNumShadowMaps; ++nMap)
	{
		m_acVisibleTreesFromLight[nMap].SetHeapReserves(sHeapReserves);
		m_asTerrainCullResults[nMap].SetHeapReserves(sHeapReserves);
	}
}


///////////////////////////////////////////////////////////////////////  
//  CMyShadowSystem::SetCullCellSize

void CMyShadowSystem::SetCullCellSize(st_float32 f3dTreeCellSize)
{
	for (st_int32 nMap = 0; nMap < c_nMaxNumShadowMaps; ++nMap)
		m_acVisibleTreesFromLight[nMap].SetCellSize(f3dTreeCellSize);
}


///////////////////////////////////////////////////////////////////////  
//  CMyShadowSystem::Init

st_bool CMyShadowSystem::Init(const CMyConfigFile& cConfigFile)
{
	st_bool bSuccess = true;

	const CMyConfigFile::SShadows& sShadows = cConfigFile.m_sShadows;

	if (sShadows.m_bEnabled)
	{
		st_assert(sShadows.m_nNumMaps > 0 && sShadows.m_nNumMaps <= c_nMaxNumShadowMaps, "If sShadows.m_bEnabled is true, then sShadows.m_nNumMaps must be in the right range");
		st_assert(sShadows.m_nResolution > 0, "If sShadows.m_bEnabled is true, then sShadows.m_nNumMaps must be greater than zero");

		// create shadow maps
		for (st_int32 nMap = 0; nMap < sShadows.m_nNumMaps; ++nMap)
		{
			const st_int32 c_nNumMultisampling = 1;
			if (!m_acShadowMaps[nMap].InitGfx(RENDER_TARGET_TYPE_SHADOW_MAP, sShadows.m_nResolution, sShadows.m_nResolution, c_nNumMultisampling))
			{
				CCore::SetError("Failed to create shadow map %d of size %d X %d", nMap, sShadows.m_nResolution, sShadows.m_nResolution);
				bSuccess = false;
			}
		}

		m_bShadowMapsReady = bSuccess;
	}
	else
	{
		// load a white texture to use for shadows since they're off
		bSuccess = m_cWhiteTexture.LoadColor(0xffffffff);
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyShadowSystem::InitGfx

st_bool CMyShadowSystem::InitGfx(const CMyConfigFile& cConfigFile, const CArray<CTreeRender*>& aBaseTrees)
{
	st_bool bSuccess = true;

	const CMyConfigFile::SShadows& sShadows = cConfigFile.m_sShadows;

	for (st_int32 nMap = 0; nMap < sShadows.m_nNumMaps; ++nMap)
		bSuccess &= m_acVisibleTreesFromLight[nMap].InitGfx(aBaseTrees);

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyShadowSystem::ReleaseGfxResources

void CMyShadowSystem::ReleaseGfxResources(void)
{
	for (st_int32 i = 0; i < c_nMaxNumShadowMaps; ++i)
	{
		m_acShadowMaps[i].ReleaseGfxResources( );
		m_acVisibleTreesFromLight[i].ReleaseGfxResources( );
	}

	m_cWhiteTexture.ReleaseGfxResources( );
}


///////////////////////////////////////////////////////////////////////  
//  CMyShadowSystem::UpdateView

st_bool CMyShadowSystem::UpdateView(const CMyConfigFile& cConfigFile,
									const CView& cMainCameraView, 
									const Vec3& vLightDir,
									CMyTerrain& cTerrain,
									CArray<CMyGrassLayer>& aGrassLayers,
									TGrassInstArray& aTmpBuffer,
									st_int32 nFrameIndex,
                                    st_bool bForceUpdate,
									CMyInstancesContainer& cAllTreeInstances)
{
    ScopeTrace("MyShadowSystem::UpdateView");

    ST_UNREF_PARAM(bForceUpdate);

	st_bool bSuccess = true;

	// aliases
	const CMyConfigFile::SShadows& sShadows = cConfigFile.m_sShadows;
	SFrameCBLayout& sFrameConstantBuffer = CForestRender::GetFrameConstantBufferContents( );

	const st_float32 c_afMapDistances[5] = 
	{
		0.0f,
		cConfigFile.m_sShadows.m_afMapRanges[0],
		cConfigFile.m_sShadows.m_afMapRanges[1],
		cConfigFile.m_sShadows.m_afMapRanges[2],
		cConfigFile.m_sShadows.m_afMapRanges[3]
	};

	for (st_int32 nMap = 0; nMap < sShadows.m_nNumMaps; ++nMap)
	{
	    const st_float32 c_fMapStartPercent = c_afMapDistances[nMap] / cMainCameraView.GetFarClip( );
	    const st_float32 c_fMapEndPercent = c_afMapDistances[nMap + 1] / cMainCameraView.GetFarClip( );

        {
            ScopeTrace("ComputeLightView");
	        bSuccess &= CForest::ComputeLightView(vLightDir, 
										          cMainCameraView.GetFrustumPoints( ),
										          c_fMapStartPercent,
										          c_fMapEndPercent,
										          m_acLightViews[nMap],
										          cConfigFile.m_sShadows.m_fBehindCameraDistance);

			sFrameConstantBuffer.m_sShadows.m_amLightModelViewProjs[nMap] = m_acLightViews[nMap].GetComposite( ).Transpose( );
        }

	    m_acLightViews[nMap].SetLodRefPoint(cMainCameraView.GetCameraPos( ));
		CMyPopulate::StreamTrees(cAllTreeInstances,
								 m_acLightViews[nMap],
								 m_acVisibleTreesFromLight[nMap],
								 nFrameIndex,
								 true,  // render trees
								 false, // update billboards
								 true); // true for shadow pass

		// terrain
		if (cConfigFile.m_sTerrain.m_bCastShadows)
			cTerrain.CullAndPopulate(m_acLightViews[nMap], nFrameIndex, m_asTerrainCullResults[nMap]);

		// grass
		CMyPopulate::StreamGrass(aGrassLayers,
								 m_acLightViews[nMap],
								 cTerrain,
								 nFrameIndex,
								 aTmpBuffer,
								 nMap); // true for shadow pass

	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyShadowSystem::NotifyOfPopulationChange

void CMyShadowSystem::NotifyOfPopulationChange(void)
{
	for (st_int32 i = 0; i < c_nMaxNumShadowMaps; ++i)
		m_acVisibleTreesFromLight[i].NotifyOfPopulationChange( );
}


///////////////////////////////////////////////////////////////////////  
//  CMyShadowSystem::RenderIntoMaps

st_bool CMyShadowSystem::RenderIntoMaps(const CMyConfigFile& cConfigFile, 
										CForestRender& cForest,
										CArray<CMyGrassLayer>& aGrassLayers,
										CMyTerrain& cTerrain,
										st_int32 nFrameIndex,
										st_bool bCastersActive,
										st_bool bForceRender)
{
	st_bool bSuccess = false;

	const CMyConfigFile::SShadows& sShadows = cConfigFile.m_sShadows;

	if (m_bShadowMapsReady)
	{
		bSuccess = true;

		for (st_int32 nMap = 0; nMap < sShadows.m_nNumMaps; ++nMap)
		{
			// check interval timing to see if this map needs to update
			if (bForceRender || (nFrameIndex % cConfigFile.m_sShadows.m_anMapUpdateIntervals[nMap] == 0))
			{
				CRenderTarget& cShadowMap = m_acShadowMaps[nMap];

				if (cShadowMap.SetAsTarget( ))
				{
					// clear just the depth buffer
					cShadowMap.Clear(Vec4( ));

					// actual rendering calls
					if (bCastersActive)
					{
						// set shadow map rendering specific states
						bSuccess &= cForest.UpdateFrameConstantBuffer(m_acLightViews[nMap], -1, -1); // window width/height not used by shadow shaders
						cForest.GetFrameConstantBufferContents( ).m_sShadows.m_fShadowMapWritingActive = 1.0f;
						cForest.GetFrameConstantBuffer( ).Bind( );

						bSuccess &= cForest.Render3dTrees(RENDER_PASS_SHADOW_CAST, m_acVisibleTreesFromLight[nMap], true);

						for (st_int32 nLayer = 0; nLayer < st_int32(aGrassLayers.size( )); ++nLayer)
						{
							if (aGrassLayers[nLayer].GetPopulationParams( ).m_bCastShadows)
							{
								bSuccess &= cForest.RenderGrass(RENDER_PASS_SHADOW_CAST, aGrassLayers[nLayer].GetBaseGrass( ), aGrassLayers[nLayer].GetVisibleFromShadowMap(nMap));
							}
						}

						// restore render state
						cForest.GetFrameConstantBufferContents( ).m_sShadows.m_fShadowMapWritingActive = 0.0f;
					}

					// render terrain into shadow maps
					if (cConfigFile.m_sTerrain.m_bCastShadows)
					{
						bSuccess &= cTerrain.Render(RENDER_PASS_SHADOW_CAST,
													m_asTerrainCullResults[nMap],
													SSimpleMaterial( ),
													cForest.GetRenderStats( ));
					}

					// with rendering complete into this map, release it and bind it as a texture
					cShadowMap.ReleaseAsTarget( );
				}
			}
		}

		#ifdef _XBOX
			DX9::Device( )->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0xff000000, 1.0f, 0L);
		#endif
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyShadowSystem::BindAsSourceTextures

st_bool CMyShadowSystem::BindAsSourceTextures(const CMyConfigFile& cConfigFile, st_int32 nRegisterOffset)
{
	st_bool bSuccess = false;

	const CMyConfigFile::SShadows& sShadows = cConfigFile.m_sShadows;

	if (m_bShadowMapsReady)
	{
		bSuccess = true;

		for (st_int32 nMap = 0; nMap < sShadows.m_nNumMaps; ++nMap)
			bSuccess &= m_acShadowMaps[nMap].BindAsTexture(nRegisterOffset + nMap);
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyShadowSystem::BindBlankShadowsAsSourceTextures

void CMyShadowSystem::BindBlankShadowsAsSourceTextures(void)
{
	for (st_int32 i = TEXTURE_REGISTER_SHADOW_MAP_0; i <= TEXTURE_REGISTER_SHADOW_MAP_3; ++i)
		CShaderConstant::SetTexture(i, m_cWhiteTexture, false);

	CShaderConstant::SubmitSetTexturesInBatch( );
}


///////////////////////////////////////////////////////////////////////  
//  CMyShadowSystem::UnBindAsSourceTextures

void CMyShadowSystem::UnBindAsSourceTextures(const CMyConfigFile& cConfigFile, st_int32 nRegisterOffset)
{
	if (m_bShadowMapsReady)
		for (st_int32 nMap = 0; nMap < cConfigFile.m_sShadows.m_nNumMaps; ++nMap)
			m_acShadowMaps[nMap].UnBindAsTexture(nRegisterOffset + nMap);
}


///////////////////////////////////////////////////////////////////////  
//  CMyShadowSystem::GetVisibleInstances

const CVisibleInstancesRender& CMyShadowSystem::GetVisibleInstances(st_int32 nShadowMapIndex) const
{
	return m_acVisibleTreesFromLight[nShadowMapIndex];
}


///////////////////////////////////////////////////////////////////////  
//  CMyShadowSystem::OnResetDevice

void CMyShadowSystem::OnResetDevice(const CMyConfigFile& cConfigFile)
{
	for (st_int32 i = 0; i < cConfigFile.m_sShadows.m_nNumMaps; ++i)
		m_acShadowMaps[i].OnResetDevice( );
}


///////////////////////////////////////////////////////////////////////  
//  CMyShadowSystem::OnLostDevice

void CMyShadowSystem::OnLostDevice(const CMyConfigFile& cConfigFile)
{
	for (st_int32 i = 0; i < cConfigFile.m_sShadows.m_nNumMaps; ++i)
		m_acShadowMaps[i].OnLostDevice( );
}
