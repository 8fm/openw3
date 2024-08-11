///////////////////////////////////////////////////////////////////////  
//  MyGrass.cpp
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

#include "MyGrass.h"
#include "MyTerrain.h"
#include "MyTgaLoader.h"
#include "Core/Random.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  CMyGrassLayer::CMyGrassLayer

CMyGrassLayer::CMyGrassLayer( ) :
	m_cVisibleGrassMainCamera(POPULATION_GRASS),
	m_pBaseGrass(NULL),
	m_fCullRadius(0.0f)
{
}
	

///////////////////////////////////////////////////////////////////////  
//  CMyGrassLayer::~CMyGrassLayer

CMyGrassLayer::~CMyGrassLayer( )
{
	st_delete<CTreeRender>(m_pBaseGrass);
	m_cMask.Clear( );
}


///////////////////////////////////////////////////////////////////////  
//  CMyGrassLayer::ReleaseGfxResources

void CMyGrassLayer::ReleaseGfxResources(void)
{
	m_cVisibleGrassMainCamera.ReleaseGfxResources( );
	for (st_int32 i = 0; i < c_nMaxNumShadowMaps; ++i)
		m_acVisibleGrassShadowMaps[i].ReleaseGfxResources( );
}


///////////////////////////////////////////////////////////////////////  
//  CMyGrassLayer::SetPopulationParams

void CMyGrassLayer::SetPopulationParams(const CMyConfigFile::SGrassPopulationParams& sPopulation)
{
	m_sPopulationParams = sPopulation;

	// load mask if necessary
	if (!m_sPopulationParams.m_strMaskFilename.empty( ) && m_sPopulationParams.m_eMaskChannel != CHANNEL_NONE)
	{
		m_cMask.Clear( );

		CMyTgaLoader cLoader;
		if (!cLoader.Read(m_sPopulationParams.m_strMaskFilename.c_str( )))
			Error("Could not load grass mask [%s]", m_sPopulationParams.m_strMaskFilename.c_str( ));
		else
		{
			m_cMask.m_nWidth = cLoader.GetWidth( );
			m_cMask.m_nHeight = cLoader.GetHeight( );
			st_uint32 uiDepth = cLoader.GetDepth( );
			st_uchar* pImageData = cLoader.GetRawData( );

			if (uiDepth < 4)
				m_sPopulationParams.m_eMaskChannel = (EChannels)(m_sPopulationParams.m_eMaskChannel & ~CHANNEL_ALPHA);
			if (m_sPopulationParams.m_eMaskChannel == 0)
				Error("Grass mask requires a 4 channel image (RGBA) when using CHANNEL_ALPHA");
			else
			{
				st_uint32 uiSize = m_cMask.m_nWidth * m_cMask.m_nHeight;
				m_cMask.m_pData = st_new_array<st_uchar>(uiSize, "CMyGrassLayer::CMy2dInterpolator");
				st_uchar* pData = m_cMask.m_pData;

				st_float32 fScalar = ((m_sPopulationParams.m_eMaskChannel & CHANNEL_BLUE) ? 1.0f : 0.0f) + 
									 ((m_sPopulationParams.m_eMaskChannel & CHANNEL_GREEN) ? 1.0f : 0.0f) +  
									 ((m_sPopulationParams.m_eMaskChannel & CHANNEL_RED) ? 1.0f : 0.0f) + 
									 ((m_sPopulationParams.m_eMaskChannel & CHANNEL_ALPHA) ? 1.0f : 0.0f);
				fScalar = 1.0f / fScalar;

				for (st_uint32 i = 0; i < uiSize; ++i)
				{
					st_float32 fTemp = 0.0f;

					// channels (BGRA)
					if (m_sPopulationParams.m_eMaskChannel & CHANNEL_BLUE)
						fTemp += pImageData[0];
					if (m_sPopulationParams.m_eMaskChannel & CHANNEL_GREEN)
						fTemp += pImageData[1];
					if (m_sPopulationParams.m_eMaskChannel & CHANNEL_RED)
						fTemp += pImageData[2];
					if (m_sPopulationParams.m_eMaskChannel & CHANNEL_ALPHA)
						fTemp += pImageData[3];

					*pData = (st_uchar)(fTemp * fScalar + 0.5f);

					++pData;
					pImageData += uiDepth;
				}
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////  
//  CMyGrassLayer::NotifyOfPopulationChanged

void CMyGrassLayer::NotifyOfPopulationChanged(void)
{
	m_cVisibleGrassMainCamera.NotifyOfPopulationChange( );
	for (st_int32 i = 0; i < c_nMaxNumShadowMaps; ++i)
		m_acVisibleGrassShadowMaps[i].NotifyOfPopulationChange( );
}


///////////////////////////////////////////////////////////////////////  
//  CMyGrassLayer::IsPositionMasked

st_bool CMyGrassLayer::IsPositionMasked(st_float32 fX, st_float32 fY, CRandom& cDice, const Vec3& vMaskSize) const
{
	st_bool bReturn = false;

	// our app-specific masking scheme is tied to the terrain's size and makes no sense
	// if the terrain system is disabled
	#ifdef MY_TERRAIN_ACTIVE
		if (m_sPopulationParams.m_vArea != Vec4(0.0f, 0.0f, 0.0f, 0.0f))
		{
			if (fX < m_sPopulationParams.m_vArea.x || fX > m_sPopulationParams.m_vArea.y ||
				fY < m_sPopulationParams.m_vArea.z || fY > m_sPopulationParams.m_vArea.w)
			{
				bReturn = true;
			}
		}

		if (!bReturn && m_cMask.IsPresent( ))
		{
			bReturn = true;
			if (cDice.GetInteger(0, 255) < m_cMask.NearestNeighbor(fX / vMaskSize.x, fY / vMaskSize.y))
				bReturn = false;
		}
	#else
		ST_UNREF_PARAM(fX);
		ST_UNREF_PARAM(fY);
		ST_UNREF_PARAM(vMaskSize);
		ST_UNREF_PARAM(cDice);
	#endif

	return bReturn;
}


///////////////////////////////////////////////////////////////////////  
//  CMyGrassLayer::IsCellMasked

st_bool CMyGrassLayer::IsCellMasked(st_float32 fCellMinX, st_float32 fCellMaxX, st_float32 fCellMinY, st_float32 fCellMaxY) const
{
	st_bool bReturn = false;

	if (m_sPopulationParams.m_vArea != Vec4(0.0f, 0.0f, 0.0f, 0.0f))
	{
		if (fCellMaxX < m_sPopulationParams.m_vArea.x || fCellMinX > m_sPopulationParams.m_vArea.y ||
			fCellMaxY < m_sPopulationParams.m_vArea.z || fCellMinY > m_sPopulationParams.m_vArea.w)
		{
			bReturn = true;
		}
	}

	return bReturn;
}


