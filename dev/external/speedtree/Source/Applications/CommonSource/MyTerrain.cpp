///////////////////////////////////////////////////////////////////////
//  MyTerrain.cpp
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

#include "MyTerrain.h"
#ifdef MY_TERRAIN_ACTIVE
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  CMyTerrain::CMyTerrain

CMyTerrain::CMyTerrain( ) :
	m_bActive(true)
{
	const SVertexDecl::SAttribDesc c_asTerrainVertexDecl[ ] =
	{
		{ 0, VERTEX_ATTRIB_0, VERTEX_FORMAT_FULL_FLOAT, 3,
			{ { VERTEX_PROPERTY_POSITION, VERTEX_COMPONENT_X }, 
		      { VERTEX_PROPERTY_POSITION, VERTEX_COMPONENT_Y }, 
			  { VERTEX_PROPERTY_POSITION, VERTEX_COMPONENT_Z }, 
			  { VERTEX_PROPERTY_UNASSIGNED, 0 } } },
		{ 0, VERTEX_ATTRIB_1, VERTEX_FORMAT_FULL_FLOAT, 3,
			{ { VERTEX_PROPERTY_DIFFUSE_TEXCOORDS, VERTEX_COMPONENT_X }, 
			  { VERTEX_PROPERTY_DIFFUSE_TEXCOORDS, VERTEX_COMPONENT_Y }, 
			  { VERTEX_PROPERTY_AMBIENT_OCCLUSION, VERTEX_COMPONENT_X }, 
			  { VERTEX_PROPERTY_UNASSIGNED, 0 } } },
		VERTEX_DECL_END( )
	};

	m_sVertexDecl.Set(c_asTerrainVertexDecl);
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrain::~CMyTerrain

CMyTerrain::~CMyTerrain( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrain::Init

st_bool CMyTerrain::Init(const CMyConfigFile& cConfigFile, const SMyCmdLineOptions& sCmdLine)
{
	st_bool bSuccess = false;

	// load terrain data; don't continue if failure
	const CFixedString& c_strTerrainPath = cConfigFile.m_sTerrain.m_strShaderPath;
	if (m_cData.Init(cConfigFile))
	{
		STerrainRenderInfo sInfo;

		// app states
		sInfo.m_sAppState.m_bMultisampling = (sCmdLine.m_nNumSamples > 1);
		sInfo.m_sAppState.m_bAlphaToCoverage = false;
		sInfo.m_sAppState.m_bDepthPrepass = cConfigFile.m_sForwardRender.m_bDepthOnlyPrepass;
		sInfo.m_sAppState.m_bDeferred = cConfigFile.m_sDeferredRender.m_bEnabled;

		// terrain shader
		sInfo.m_strShaderPath = c_strTerrainPath;

		// texture filenames
		sInfo.m_strNormalMap = cConfigFile.m_sTerrain.m_strNormalMapFilename;
		sInfo.m_strSplatMap = cConfigFile.m_sTerrain.m_strSplatMapFilename;
		sInfo.m_astrSplatLayers[0] = cConfigFile.m_sTerrain.m_asSplatLayers[0].m_strFilename;
		sInfo.m_astrSplatLayers[1] = cConfigFile.m_sTerrain.m_asSplatLayers[1].m_strFilename;
		sInfo.m_astrSplatLayers[2] = cConfigFile.m_sTerrain.m_asSplatLayers[2].m_strFilename;

		// render parameters
		sInfo.m_afSplatTileValues[0] = cConfigFile.m_sTerrain.m_asSplatLayers[0].m_fRepeat;
		sInfo.m_afSplatTileValues[1] = cConfigFile.m_sTerrain.m_asSplatLayers[1].m_fRepeat;
		sInfo.m_afSplatTileValues[2] = cConfigFile.m_sTerrain.m_asSplatLayers[2].m_fRepeat;

		// lighting / material
		sInfo.m_bShadowsEnabled = (cConfigFile.m_sShadows.m_nResolution > 0);
		sInfo.m_vMaterialAmbient = cConfigFile.m_sTerrain.m_vAmbient;
		sInfo.m_vMaterialDiffuse = cConfigFile.m_sTerrain.m_vDiffuse;

		// shadows
		sInfo.m_bCastShadows = cConfigFile.m_sTerrain.m_bCastShadows;

		// fog
		sInfo.m_fFogStartDistance = cConfigFile.m_sFog.m_afLinear[0];
		sInfo.m_fFogEndDistance = cConfigFile.m_sFog.m_afLinear[1];
		sInfo.m_vFogColor = cConfigFile.m_sFog.m_vColor;

		// LOD
		sInfo.m_nNumLodLevels = cConfigFile.m_sTerrain.m_nNumLodLevels;
		sInfo.m_nMaxTerrainRes = cConfigFile.m_sTerrain.m_nMaxResolution;
		sInfo.m_fCellSize = cConfigFile.m_sTerrain.m_fCellSize;
		
		// misc
		sInfo.m_bDepthOnlyPrepass = cConfigFile.m_sForwardRender.m_bDepthOnlyPrepass;
		sInfo.m_nMaxAnisotropy = sCmdLine.m_nMaxAnisotropy;

		// render parameters
		SetRenderInfo(sInfo);

		// graphics
		SetMaxAnisotropy(sInfo.m_nMaxAnisotropy);
		bSuccess = CTerrainRender::InitGfx(sInfo.m_nNumLodLevels, sInfo.m_nMaxTerrainRes, sInfo.m_fCellSize, m_sVertexDecl);
		CTerrainRender::SetLodRange(0.0f, cConfigFile.m_sWorld.m_fFarClip);

		// height hints
		st_float32 fLowestPointOnTerrain = 0.0f, fHighestPointOnTerrain = 0.0f;
		m_cData.GetHeightRange(fLowestPointOnTerrain, fHighestPointOnTerrain);
		SetHeightHints(fLowestPointOnTerrain, fHighestPointOnTerrain);
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrain::SetHeapReserves

void CMyTerrain::SetHeapReserves(const SHeapReserves& sHeapReserves)
{
	// base class reserves
	CTerrain::SetHeapReserves(sHeapReserves);
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrain::ComputeAmbientOcclusion

void CMyTerrain::ComputeAmbientOcclusion(const CMyConfigFile& cConfigFile, const CMyInstancesContainer& cInstances)
{
	m_cData.ComputeAmbientOcclusion(cConfigFile, cInstances);
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrain::CullAndPopulate

void CMyTerrain::CullAndPopulate(const CView& cView, st_int32 nFrameIndex, STerrainCullResults& sResults)
{
    ScopeTrace("CMyTerrain::CullAndPopulate");

	if (m_bActive)
	{
		CullAndComputeLOD(cView, nFrameIndex, sResults);

		if (!sResults.m_aCellsToUpdate.empty( ))
			Populate(sResults);
	}
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrain::Populate

void CMyTerrain::Populate(STerrainCullResults& sResults)
{
	if (!m_cData.IsLoaded( ))
		return;

	TTerrainCellArray& aCells = sResults.m_aCellsToUpdate;

	// gather some working parameters
	const st_int32 c_nNumTileRes = GetMaxTileRes( );
	const st_int32 c_nNumCells = st_int32(aCells.size( ));
	const st_int32 c_nTiles = m_cData.GetNumTiles( );
	const st_float32 c_fCellSize = GetCellSize( );
	const st_float32 c_fVertexSpacing = c_fCellSize / (c_nNumTileRes - 1);
	const st_float32 c_afTexcoordScalars[2] = { 1.0f / m_cData.GetSize( ).x, 1.0f / m_cData.GetSize( ).y };
	const st_float32 c_afTexcoordSpacings[2] = { c_fVertexSpacing * c_afTexcoordScalars[0], c_fVertexSpacing * c_afTexcoordScalars[1] };

	// if populating the cells in parallel, we'll need multiple copies of the random
	// generator and the intermediate buffer that's filled per cell
    CStaticArray<STerrainVertex> aVertices(c_nNumTileRes * c_nNumTileRes, "PopulateTerrainCells::STerrainVertex");
	for (st_int32 nCell = 0; nCell < c_nNumCells; ++nCell)
	{
		// cell the cell and associated VBO
		CTerrainCell* pCell = aCells[nCell];
		st_assert(pCell, "m_sCullResults.m_aCellsToUpdate should never contain a NULL cell pointer");

		// determine visibility due to possible tiling or truncation
		st_bool bCellVisible = true;
		if (c_nTiles != 0)
		{
			if ((pCell->Col( ) < 0) ||
				(pCell->Row( ) < 0) ||
				(pCell->Col( ) * c_fCellSize >= m_cData.GetSize( ).x * c_nTiles) ||
				(pCell->Row( ) * c_fCellSize >= m_cData.GetSize( ).y * c_nTiles))
			{
				bCellVisible = false;
			}
		}
		pCell->SetIsPopulated(bCellVisible);

		if (pCell->IsPopulated( ))
		{
			CGeometryBuffer* pVbo = (CGeometryBuffer*) pCell->GetVbo( );
			if (pVbo)
			{
				// get cell attributes
				const st_int32 c_nCellRow = pCell->Row( );
				const st_int32 c_nCellCol = pCell->Col( );

				// loop through, assigning the vertex data
				const st_float32 c_fStartPosX = c_nCellCol * c_fCellSize;
				const st_float32 c_fTexCoordOffsetX = Frac(c_fStartPosX * c_afTexcoordScalars[0]);
				st_float32 s = c_fTexCoordOffsetX;

				if (CCoordSys::IsDefaultCoordSys( ))
				{
					for (st_int32 nCol = 0; nCol < c_nNumTileRes; ++nCol)
					{
						// coords
						st_float32 x = c_fStartPosX + nCol * c_fVertexSpacing;
						st_float32 y = c_nCellRow * c_fCellSize;

						// texcoords
						const st_float32 c_fTexCoordOffsetY = Frac(y * c_afTexcoordScalars[1]);
						st_float32 t = c_fTexCoordOffsetY;

						STerrainVertex* pVertex = &aVertices[nCol * c_nNumTileRes];
						for (st_int32 nRow = 0; nRow < c_nNumTileRes; ++nRow)
						{
							pVertex->m_vCoords.Set(x, y, m_cData.GetHeight(x, y));
							pVertex->m_afTexCoords[0] = s;
							pVertex->m_afTexCoords[1] = -t;
							pVertex->m_afTexCoords[2] = m_cData.GetAmbientOcclusion(x, y);

							++pVertex;
							y += c_fVertexSpacing;
							t += c_afTexcoordSpacings[1];
						}

						s += c_afTexcoordSpacings[0];
					}
				}
				else
				{
					for (st_int32 nCol = 0; nCol < c_nNumTileRes; ++nCol)
					{
						// coords
						st_float32 x = c_fStartPosX + nCol * c_fVertexSpacing;
						st_float32 y = c_nCellRow * c_fCellSize;

						// texcoords
						const st_float32 c_fTexCoordOffsetY = Frac(y * c_afTexcoordScalars[1]);
						st_float32 t = c_fTexCoordOffsetY;

						STerrainVertex* pVertex = &aVertices[nCol * c_nNumTileRes];
						for (st_int32 nRow = 0; nRow < c_nNumTileRes; ++nRow)
						{
							pVertex->m_vCoords.Set(CCoordSys::ConvertFromStd(x, y, m_cData.GetHeight(x, y)));
							pVertex->m_afTexCoords[0] = s;
							pVertex->m_afTexCoords[1] = -t;
							pVertex->m_afTexCoords[2] = m_cData.GetAmbientOcclusion(x, y);

							++pVertex;
							y += c_fVertexSpacing;
							t += c_afTexcoordSpacings[1];
						}

						s += c_afTexcoordSpacings[0];
					}
				}

				#ifdef SPEEDTREE_OPENMP
					#pragma omp critical
				#endif
				pVbo->OverwriteVertices(&aVertices[0], st_uint32(aVertices.size( )), 0);
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrain::Render

st_bool CMyTerrain::Render(ERenderPass ePass, 
						   const STerrainCullResults& sResults,
						   const SSimpleMaterial& sLightMaterial, 
						   CRenderStats& cStats)
{
	st_bool bSuccess = false;

	if (m_bActive)
		bSuccess = CTerrainRender::Render(ePass, 
										  sResults, 
										  sLightMaterial, 
										  cStats);
	else
		bSuccess = true; // is inactive, so not rendering is success

	return bSuccess;
}


#endif // MY_TERRAIN_ACTIVE
