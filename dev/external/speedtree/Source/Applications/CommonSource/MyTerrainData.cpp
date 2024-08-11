///////////////////////////////////////////////////////////////////////  
//  MyTerrainData.cpp
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

#include <cstdio>
#include "MyTerrainData.h"
#include "MyTgaLoader.h"
#include "Utilities/Utility.h"
#include "MyApplication.h"
#ifdef SPEEDTREE_OPENMP
	#include <omp.h>
#endif
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  CMyTerrainData::Init

st_bool CMyTerrainData::Init(const CMyConfigFile& cConfigFile)
{
	st_bool bSuccess = false;

	// alias
	const CMyConfigFile::STerrain& cTerrain = cConfigFile.m_sTerrain;

	// remove old data if called more than once
	Clear( );

	// copy some parameters from config file
	m_vSize = cTerrain.m_vHeightMapSize;
	m_fHeightScalar = 0.15f;

	// load TGA height map
	if (LoadHeight(cTerrain.m_strHeightMapFilename.c_str( ), CHANNEL_ALL))
	{
		m_nTiles = cConfigFile.m_sTerrain.m_nTiles;

		bSuccess = ResampleHeight(cConfigFile);
	
		if (bSuccess)
		{
			ComputeSlope(1.0f);
			GenerateNormals( );

			//SaveOBJ("G:/__WORK__/test.obj");
		}
	}
	else
		CCore::SetError("CMyTerrainData::Init, failed to load height map [%s]\n", cTerrain.m_strHeightMapFilename.c_str( ));

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrainData::LoadHeight

st_bool CMyTerrainData::LoadHeight(const char* pFilename, EChannels eChannels, st_int32 nTiles)
{
	st_bool bReturn = false;

	m_cHeightData.Clear( );
	m_fMinHeight = FLT_MAX;
	m_fMaxHeight = -FLT_MAX;
	m_nTiles = 0;

	CMyTgaLoader cLoader;
	if (cLoader.Read(pFilename))
	{
		m_cHeightData.m_nWidth = cLoader.GetWidth( );
		m_cHeightData.m_nHeight = cLoader.GetHeight( );
		st_uint32 uiDepth = cLoader.GetDepth( );
		st_uchar* pImageData = cLoader.GetRawData( );

		if (uiDepth < 4)
			eChannels = (EChannels)(eChannels & ~CHANNEL_ALPHA);
		if (eChannels == 0)
			Error("CMyTerrainData::LoadHeight requires a 4 channel image (RGBA) when using CHANNEL_ALPHA");
		else
		{
			st_uint32 uiSize = m_cHeightData.m_nWidth * m_cHeightData.m_nHeight;
			m_cHeightData.m_pData = st_new_array<st_float32>(uiSize, "CMyTerrainData::CMy2dInterpolator");
			st_float32* pData = m_cHeightData.m_pData;

			st_float32 fScalar = ((eChannels & CHANNEL_BLUE) ? 1.0f : 0.0f) + 
								 ((eChannels & CHANNEL_GREEN) ? 1.0f : 0.0f) +  
								 ((eChannels & CHANNEL_RED) ? 1.0f : 0.0f) + 
								 ((eChannels & CHANNEL_ALPHA) ? 1.0f : 0.0f);
			fScalar = m_vSize.z / (255.0f * fScalar);

			for (st_uint32 i = 0; i < uiSize; ++i)
			{
				*pData = 0.0f;

				// channels (BGRA)
				if (eChannels & CHANNEL_BLUE)
					*pData += pImageData[0];
				if (eChannels & CHANNEL_GREEN)
					*pData += pImageData[1];
				if (eChannels & CHANNEL_RED)
					*pData += pImageData[2];
				if (eChannels & CHANNEL_ALPHA)
					*pData += pImageData[3];

				*pData *= fScalar;

				m_fMinHeight = st_min(m_fMinHeight, *pData);
				m_fMaxHeight = st_max(m_fMaxHeight, *pData);

				++pData;
				pImageData += uiDepth;
			}

			bReturn = true;
			m_nTiles = nTiles;
		}
	}

	return bReturn;
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrainData::ResampleHeight

st_bool CMyTerrainData::ResampleHeight(const CMyConfigFile& cConfigFile)
{
	st_bool bSuccess = false;

	if (IsLoaded( ))
	{
		// determine mapping from old sampling rate to new one, based on terrain's sampling
		const st_float32 c_afTerrainPointSpacings[2] =
		{
			cConfigFile.m_sTerrain.m_fCellSize / (cConfigFile.m_sTerrain.m_nMaxResolution - 1),
			cConfigFile.m_sTerrain.m_fCellSize / (cConfigFile.m_sTerrain.m_nMaxResolution - 1)
		};

		// create a new height sample data set
		CMy2dInterpolator<st_float32> cResampledHeight;
		cResampledHeight.m_nWidth = st_int32(m_vSize.x / c_afTerrainPointSpacings[0]);
		cResampledHeight.m_nHeight = st_int32(m_vSize.y / c_afTerrainPointSpacings[1]);
		if (cConfigFile.m_sTerrain.m_nTiles == 1)
		{
			cResampledHeight.m_nWidth += 1;
			cResampledHeight.m_nHeight += 1;
		}
		cResampledHeight.m_pData = st_new_array<st_float32>(cResampledHeight.m_nWidth * cResampledHeight.m_nHeight, "CMyTerrainData::cResampledHeight");
		st_float32* pData = cResampledHeight.m_pData;

		// convert from old samples to new
		st_float32 afIntervals[2] = { 1.0f / cResampledHeight.m_nWidth, 1.0f / cResampledHeight.m_nHeight };
		st_float32 v = afIntervals[1];
		for (st_int32 h = 0; h < cResampledHeight.m_nHeight; ++h)
		{
			st_float32 u = afIntervals[0];
			for (st_int32 w = 0; w < cResampledHeight.m_nWidth; ++w)
			{
				if (m_nTiles == 1)
					*pData++ = m_cHeightData.InterpolateValueClamped(u, v);
				else
					*pData++ = m_cHeightData.InterpolateValue(u, v);
				u += afIntervals[0];
			}

			v += afIntervals[1];
		}

		// copy new samples over old
		st_delete_array<st_float32>(m_cHeightData.m_pData);
		m_cHeightData = cResampledHeight;

		bSuccess = true;
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrainData::GenerateNormals

void CMyTerrainData::GenerateNormals(void)
{
	m_cNormalData.Clear( );

	if (m_cHeightData.m_nWidth > 0 && m_cHeightData.m_nHeight > 0 && m_cHeightData.m_pData != NULL)
	{
		const st_int32 c_nWidth = m_cNormalData.m_nWidth = m_cHeightData.m_nWidth;
		const st_int32 c_nHeight = m_cNormalData.m_nHeight = m_cHeightData.m_nHeight;
		m_cNormalData.m_pData = st_new_array<SNormalData>(c_nWidth * c_nHeight, "CMyTerrainData::CMy2dInterpolator");

		const st_float32 c_fUSpacing = m_vSize.x / st_float32(c_nWidth);
		const st_float32 c_fVSpacing = m_vSize.y / st_float32(c_nHeight);

		for (st_int32 i = 0; i < c_nWidth; ++i)
		{
			const st_int32 c_nLeft = i;
			const st_int32 c_nRight = (i == c_nWidth - 1 ? 0 : i + 1);

			for (st_int32 j = 0; j < c_nHeight; ++j)
			{
				const st_int32 c_nTop = j;
				const st_int32 c_nBottom = (j == c_nHeight - 1 ? 0 : j + 1);

				// compute upper triangle normal
				{
					Vec3 vAcross(c_fUSpacing, 0.0f, m_cHeightData.m_pData[c_nRight + c_nBottom * c_nWidth] - m_cHeightData.m_pData[c_nLeft + c_nBottom * c_nWidth]);
					Vec3 vDown(0.0f, c_fVSpacing, m_cHeightData.m_pData[c_nRight + c_nBottom * c_nWidth] - m_cHeightData.m_pData[c_nRight + c_nTop * c_nWidth]);

					m_cNormalData.m_pData[i + j * c_nWidth].m_vUpper = vAcross.Cross(vDown);
					m_cNormalData.m_pData[i + j * c_nWidth].m_vUpper.Normalize( );
				}

				// compute lower triangle normal
				{
					Vec3 vAcross(c_fUSpacing, 0.0f, m_cHeightData.m_pData[c_nRight + c_nTop * c_nWidth] - m_cHeightData.m_pData[c_nLeft + c_nTop * c_nWidth]);
					Vec3 vDown(0.0f, c_fVSpacing, m_cHeightData.m_pData[c_nLeft + c_nBottom * c_nWidth] - m_cHeightData.m_pData[c_nLeft + c_nTop * c_nWidth]);

					m_cNormalData.m_pData[i + j * c_nWidth].m_vLower = vAcross.Cross(vDown);
					m_cNormalData.m_pData[i + j * c_nWidth].m_vLower.Normalize( );
				}
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrainData::ComputeSlope

void CMyTerrainData::ComputeSlope(st_float32 fSmoothing)
{
	m_cSlopeData.Clear( );

	if (m_cHeightData.m_nWidth > 0 && m_cHeightData.m_nHeight > 0 && m_cHeightData.m_pData != NULL)
	{
		st_int32 nWidth = m_cSlopeData.m_nWidth = m_cHeightData.m_nWidth;
		st_int32 nHeight = m_cSlopeData.m_nHeight = m_cHeightData.m_nHeight;
		m_cSlopeData.m_pData = st_new_array<st_float32>(nWidth * nHeight, "CMyTerrainData::CMy2dInterpolator");

		st_float32 fUSpacing = m_vSize.x / (st_float32)nWidth;
		st_float32 fVSpacing = m_vSize.y / (st_float32)nHeight;

		#ifdef SPEEDTREE_OPENMP
			// restrict the # of threads to no more than SPEEDTREE_OPENMP_MAX_THREADS
			st_int32 nAvailableThreads = omp_get_max_threads( );
			omp_set_num_threads(st_min(SPEEDTREE_OPENMP_MAX_THREADS, nAvailableThreads));
			#pragma omp parallel for
		#endif
		for (st_int32 i = 0; i < nWidth; ++i)
		{
			st_int32 nLeft = (i == 0 ? nWidth - 1 : i - 1);
			st_int32 nRight = (i == nWidth - 1 ? 0 : i + 1);

			for (st_int32 j = 0; j < nHeight; ++j)
			{
				st_int32 nTop = (j == 0 ? nHeight - 1 : j - 1);
				st_int32 nBottom = (j == nHeight - 1 ? 0 : j + 1);

				st_float32 fSlope = 0.0f;
				st_float32 fThisHeight = m_cHeightData.m_pData[i + j * nWidth];
				fSlope += atan2(fabs(fThisHeight - m_cHeightData.m_pData[nRight + j * nWidth]), fUSpacing);
				fSlope += atan2(fabs(fThisHeight - m_cHeightData.m_pData[nLeft + j * nWidth]), fUSpacing);
				fSlope += atan2(fabs(fThisHeight - m_cHeightData.m_pData[i + nTop * nWidth]), fVSpacing);
				fSlope += atan2(fabs(fThisHeight - m_cHeightData.m_pData[i + nBottom * nWidth]), fVSpacing);

				fSlope *= (0.25f / c_fHalfPi);

				if (fSlope > 1.0f)
					fSlope = 1.0f;
				m_cSlopeData.m_pData[i + j * nWidth] = fSlope;
			}
		}

		if (fSmoothing > 0.0f)
		{
			st_float32* pTempBuffer = st_new_array<st_float32>(nWidth * nHeight, "CMyTerrainData::st_float32");

			st_float32 fBlurScalar = 1.0f / (fSmoothing * 2.5066f);
			st_float32 fBlurExpDenominator = -1.0f / (2.0f * fSmoothing * fSmoothing);

			// precompute the gaussian weights
			const size_t c_nMaxExpectedWeights = 100; // is typically around 5
			CStaticArray<st_float32> aWeights(c_nMaxExpectedWeights, "CMyTerrainData::ComputeSlope", false);
			st_float32 fWeight = 0.0f;
			st_float32 fWeightSum = 0.0f;
			st_int32 nDistance = 0;
			st_int32 nMaxDistance = st_min(nWidth, nHeight);
			do 
			{
				st_float32 fExponent = fBlurExpDenominator * st_float32(nDistance) * st_float32(nDistance);
				if (fExponent < -10.0f)
					break;
				fWeight = fBlurScalar * exp(fExponent);
				fWeightSum += fWeight;
				if (nDistance > 0)
					fWeightSum += fWeight;
				aWeights.push_back(fWeight);

				st_assert(aWeights.size( ) < c_nMaxExpectedWeights, "aWeights is unreasonably high");

				++nDistance;
			} 
			while (nDistance < nMaxDistance);

			st_int32 nNumWeights = st_int32(aWeights.size( ));
			for (st_int32 i = 0; i < nNumWeights; ++i)
				aWeights[i] /= fWeightSum;

			// horizontal blur
			#ifdef SPEEDTREE_OPENMP
				#pragma omp parallel for
			#endif
			for (st_int32 y = 0; y < st_int32(nHeight); ++y)
			{
				st_float32* pNewData = &pTempBuffer[y * nWidth];
				st_float32* pOriginalData = &m_cSlopeData.m_pData[y * nWidth];

				for (st_int32 x = 0; x < st_int32(nWidth); ++x)
				{
					pNewData[x] = 0.0f;
					for (st_int32 nOffset = -nNumWeights + 1; nOffset < nNumWeights; ++nOffset)
					{
						st_int32 x2 = x + nOffset;
						st_float32 fWeight2 = aWeights[abs(x - x2)];

						if (x2 < 0)
							x2 += st_int32(nWidth);
						if (x2 >= st_int32(nWidth))
							x2 -= st_int32(nWidth);

						pNewData[x] += pOriginalData[x2] * fWeight2;
					}
				}
			}

			// vertical blur
			#ifdef SPEEDTREE_OPENMP
				#pragma omp parallel for
			#endif
			for (st_int32 x = 0; x < st_int32(nWidth); ++x)
			{
				st_float32* pOriginalData = &pTempBuffer[x];
				st_float32* pNewData = &m_cSlopeData.m_pData[x];

				for (st_int32 y = 0; y < st_int32(nHeight); ++y)
				{
					pNewData[y * nWidth] = 0.0f;
					for (st_int32 nOffset = -nNumWeights + 1; nOffset < nNumWeights; ++nOffset)
					{
						st_int32 y2 = y + nOffset;
						st_float32 fWeight2 = aWeights[abs(y - y2)];

						if (y2 < 0)
							y2 += st_int32(nHeight);
						if (y2 >= st_int32(nHeight))
							y2 -= st_int32(nHeight);

						pNewData[y * nWidth] += pOriginalData[y2 * nWidth] * fWeight2;
					}
				}
			}

			st_delete_array<st_float32>(pTempBuffer);
		}
	}
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrainData::ComputeAmbientOcclusion

void CMyTerrainData::ComputeAmbientOcclusion(const CMyConfigFile& cConfigFile, const CMyInstancesContainer& sInstances)
{
	const CMyConfigFile::STerrain* pTerrainConfig = &cConfigFile.m_sTerrain;

	m_cAOData.Clear( );
	if (pTerrainConfig->m_fAmbientOcclusionBrightness > 0.9f)
		return;

	// compute tree extents
	CExtents cTreeExtents;
	sInstances.ComputeExtents(cTreeExtents);
	if (!cTreeExtents.Valid( ))
		return;

	// modify extents so it lines up with terrain cells
	*(const_cast<Vec3*>(&cTreeExtents.Min( ))) = CCoordSys::ConvertToStd(cTreeExtents.Min( ));
	*(const_cast<Vec3*>(&cTreeExtents.Max( ))) = CCoordSys::ConvertToStd(cTreeExtents.Max( ));
	cTreeExtents.Order( );	
	cTreeExtents[0] -= pTerrainConfig->m_fAmbientOcclusionDistance;
	cTreeExtents[1] -= pTerrainConfig->m_fAmbientOcclusionDistance;
	cTreeExtents[3] += pTerrainConfig->m_fAmbientOcclusionDistance;
	cTreeExtents[4] += pTerrainConfig->m_fAmbientOcclusionDistance;
	cTreeExtents[0] /= pTerrainConfig->m_fCellSize;
	cTreeExtents[1] /= pTerrainConfig->m_fCellSize;
	cTreeExtents[3] /= pTerrainConfig->m_fCellSize;
	cTreeExtents[4] /= pTerrainConfig->m_fCellSize;
	cTreeExtents[0] = floor(cTreeExtents[0]);
	cTreeExtents[1] = floor(cTreeExtents[1]);
	cTreeExtents[3] = ceil(cTreeExtents[3]);
	cTreeExtents[4] = ceil(cTreeExtents[4]);
	cTreeExtents[0] *= pTerrainConfig->m_fCellSize;
	cTreeExtents[1] *= pTerrainConfig->m_fCellSize;
	cTreeExtents[3] *= pTerrainConfig->m_fCellSize;
	cTreeExtents[4] *= pTerrainConfig->m_fCellSize;

	// set up dimming data
	m_afAOBounds[0] = cTreeExtents[0];
	m_afAOBounds[1] = cTreeExtents[1];
	m_afAOBounds[2] = cTreeExtents[3] - cTreeExtents[0];
	m_afAOBounds[3] = cTreeExtents[4] - cTreeExtents[1];
	m_cAOData.m_nWidth = st_int32(st_float32(pTerrainConfig->m_nMaxResolution) * m_afAOBounds[2] / pTerrainConfig->m_fCellSize);
	m_cAOData.m_nHeight = st_int32(st_float32(pTerrainConfig->m_nMaxResolution) * m_afAOBounds[3] / pTerrainConfig->m_fCellSize);
	m_cAOData.m_pData = st_new_array<st_uchar>(m_cAOData.m_nWidth * m_cAOData.m_nHeight, "CMyTerrainData::CMy2dInterpolator");
	memset(m_cAOData.m_pData, 0xff, m_cAOData.m_nWidth * m_cAOData.m_nHeight);

	// set up precomputed constants
	const st_float32 fWidth = st_float32(m_cAOData.m_nWidth);
	const st_float32 fHeight = st_float32(m_cAOData.m_nHeight);
	const st_float32 fOneOverWidth = 1.0f / fWidth;
	const st_float32 fOneOverHeight = 1.0f / fHeight;
	const st_float32 fOneOverBoundsWidth = 1.0f / m_afAOBounds[2];
	const st_float32 fOneOverBoundsHeight = 1.0f / m_afAOBounds[3];

	// iterate through trees
	const TTreePtrArray& aBaseTrees = sInstances.GetBaseTrees( );
	const TTreeInstArray2D& aaInstancesPerBaseTree = sInstances.GetInstancesPerBaseTree( );
	st_uint32 uiNumBaseTrees = st_uint32(aBaseTrees.size( ));
	for (st_uint32 uiBaseTree = 0; uiBaseTree < uiNumBaseTrees; ++uiBaseTree)
	{
		CTree* pBaseTree = aBaseTrees[uiBaseTree];

		Vec3 vDiagonal = pBaseTree->GetExtents( ).GetDiagonal( );
		vDiagonal.z = 0.0f;
		st_float32 fTreeGroundRadius = vDiagonal.Magnitude( ) * 0.5f;

		const TTreeInstArray* pInstances = &aaInstancesPerBaseTree[uiBaseTree];
		for (TTreeInstArray::const_iterator iInstance = pInstances->begin( ); iInstance != pInstances->end( ); ++iInstance)
		{
			// splat underneath this tree
			const Vec3 vPos = CCoordSys::ConvertToStd(iInstance->GetPos( ));
			const st_int32 nU = st_int32(fWidth * (vPos.x - m_afAOBounds[0]) * fOneOverBoundsWidth + 0.5f);
			const st_int32 nV = st_int32(fHeight * (vPos.y - m_afAOBounds[1]) * fOneOverBoundsHeight + 0.5f);

			const st_float32 fTreeRadius = fTreeGroundRadius * iInstance->GetScalar( );
			st_float32 fDistanceScalar = 1.0f / (fTreeRadius + pTerrainConfig->m_fAmbientOcclusionDistance);
			fDistanceScalar *= fDistanceScalar;
			const st_int32 nXSteps = st_int32(fWidth * (pTerrainConfig->m_fAmbientOcclusionDistance + fTreeRadius) * fOneOverBoundsWidth + 0.5f);
			const st_int32 nYSteps = st_int32(fHeight * (pTerrainConfig->m_fAmbientOcclusionDistance + fTreeRadius) * fOneOverBoundsHeight + 0.5f);

			st_float32 fHeightInfluence = st_min(1.0f, (iInstance->GetScalar( ) * pBaseTree->GetExtents( ).Max( )[2]) / (pTerrainConfig->m_fAmbientOcclusionDistance + fTreeRadius));
			for (st_int32 nX = nU - nXSteps; nX <= nU + nXSteps; ++nX)
			{
				for (st_int32 nY = nV - nYSteps; nY <= nV + nYSteps; ++nY)
				{
					const st_float32 fXDiff = m_afAOBounds[2] * st_float32(nX) * fOneOverWidth + m_afAOBounds[0] - vPos.x;
					const st_float32 fYDiff = m_afAOBounds[3] * st_float32(nY) * fOneOverHeight + m_afAOBounds[1] - vPos.y;
					st_float32 fDist = fXDiff * fXDiff + fYDiff * fYDiff;
					fDist *= fDistanceScalar;
					if (fDist < 1.0f)
					{
						fDist = Interpolate(1.0f, fDist, fHeightInfluence);
						st_uchar* pValue = &m_cAOData.m_pData[nX + nY * m_cAOData.m_nWidth];
						*pValue = st_min(*pValue, st_uchar(fDist * 255.0f + 0.5f));
					}
				}
			}
		}
	}

	st_int32 nSize = m_cAOData.m_nWidth * m_cAOData.m_nHeight;
	#ifdef SPEEDTREE_OPENMP
		// restrict the # of threads to no more than SPEEDTREE_OPENMP_MAX_THREADS
		st_int32 nAvailableThreads = omp_get_max_threads( );
		omp_set_num_threads(st_min(SPEEDTREE_OPENMP_MAX_THREADS, nAvailableThreads));
		#pragma omp parallel for
	#endif
	for (st_int32 i = 0; i < nSize; ++i)
	{
		m_cAOData.m_pData[i] = Interpolate<st_uchar>(m_cAOData.m_pData[i], 255, pTerrainConfig->m_fAmbientOcclusionBrightness);
	}

	// flip these for easy use in GetAmbientOcclusion()
	if (m_afAOBounds[2] != 0.0f)
		m_afAOBounds[2] = 1.0f / m_afAOBounds[2];
	if (m_afAOBounds[3] != 0.0f)
		m_afAOBounds[3] = 1.0f / m_afAOBounds[3];
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrainData::SaveOBJ

st_bool	CMyTerrainData::SaveOBJ(const char* pFilename)
{
	st_bool bReturn = false;

	FILE* pFile = fopen(pFilename, "w");
	if (pFile != NULL)
	{
		for (int x = 0; x < m_cHeightData.m_nWidth; ++x)
		{
			for (int y = 0; y < m_cHeightData.m_nHeight; ++y)
			{
				fprintf(pFile, "v %g %g %g\n", x * m_vSize.x, y * m_vSize.y, m_cHeightData.m_pData[y * m_cHeightData.m_nWidth + x] * m_vSize.z);
			}
		}

		for (int x = 1; x < m_cHeightData.m_nWidth; ++x)
		{
			for (int y = 1; y < m_cHeightData.m_nHeight; ++y)
			{
				int iBase = x + y * m_cHeightData.m_nWidth + 1;

				fprintf(pFile, "f %d %d %d %d\n", iBase, iBase - m_cHeightData.m_nWidth, iBase - 1 - m_cHeightData.m_nWidth, iBase - 1);
			}
		}

		fclose(pFile);
		bReturn = true;
	}

	return bReturn;
}

