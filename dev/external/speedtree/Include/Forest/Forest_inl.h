///////////////////////////////////////////////////////////////////////  
//  Forest.inl
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
//  SHeapReserves::SHeapReserves

ST_INLINE SHeapReserves::SHeapReserves( ) :
	m_nMaxBaseTrees(1),
	m_nMaxVisibleTreeCells(1),
	m_nMaxVisibleGrassCells(1),
	m_nMaxVisibleTerrainCells(1),
	m_nMaxTreeInstancesInAnyCell(1),
	m_nMaxPerBaseGrassInstancesInAnyCell(1),
	m_nNumShadowMaps(0)
{
}


///////////////////////////////////////////////////////////////////////  
//  CView::CView

ST_INLINE CView::CView( ) :
	m_fNearClip(1.0f),
	m_fFarClip(100.0f),
	m_fCameraAzimuth(0.0f),
	m_fCameraPitch(0.0f),
	m_fHorzFadeStartAngle(DegToRad(30.0f)),
	m_fHorzFadeEndAngle(DegToRad(60.0f))
{
}


///////////////////////////////////////////////////////////////////////  
//  CView::SetLodRefPoint

ST_INLINE void CView::SetLodRefPoint(const Vec3& vLodRefPoint)
{
	m_vLodRefPoint = vLodRefPoint;
}


///////////////////////////////////////////////////////////////////////  
//  CView::GetCameraPos

ST_INLINE const Vec3& CView::GetCameraPos(void) const
{
	return m_vCameraPos;
}


///////////////////////////////////////////////////////////////////////  
//  CView::GetLodRefPoint

ST_INLINE const Vec3& CView::GetLodRefPoint(void) const
{
	return m_vLodRefPoint;
}


///////////////////////////////////////////////////////////////////////  
//  CView::GetProjection

ST_INLINE const Mat4x4& CView::GetProjection(void) const
{
	return m_mProjection;
}


///////////////////////////////////////////////////////////////////////  
//  CView::GetModelview

ST_INLINE const Mat4x4& CView::GetModelview(void) const
{
	return m_mModelview;
}


///////////////////////////////////////////////////////////////////////  
//  CView::GetModelviewNoTranslate

ST_INLINE const Mat4x4& CView::GetModelviewNoTranslate(void) const
{
	return m_mModelviewNoTranslate;
}


///////////////////////////////////////////////////////////////////////  
//  CView::GetNearClip

ST_INLINE st_float32 CView::GetNearClip(void) const
{
	return m_fNearClip;
}


///////////////////////////////////////////////////////////////////////  
//  CView::GetFarClip

ST_INLINE st_float32 CView::GetFarClip(void) const
{
	return m_fFarClip;
}


///////////////////////////////////////////////////////////////////////  
//  CView::GetCameraDir

ST_INLINE const Vec3& CView::GetCameraDir(void) const
{
	return m_vCameraDir;
}


///////////////////////////////////////////////////////////////////////  
//  CView::GetComposite

ST_INLINE const Mat4x4& CView::GetComposite(void) const
{
	return m_mComposite;
}


///////////////////////////////////////////////////////////////////////  
//  CView::GetCompositeNoTranslate

ST_INLINE const Mat4x4& CView::GetCompositeNoTranslate(void) const
{
	return m_mCompositeNoTranslate;
}


///////////////////////////////////////////////////////////////////////  
//  CView::GetProjectionInverse

ST_INLINE const Mat4x4& CView::GetProjectionInverse(void) const
{
	return m_mProjectionInverse;
}


///////////////////////////////////////////////////////////////////////  
//  CView::GetCameraAzimuth

ST_INLINE st_float32 CView::GetCameraAzimuth(void) const
{
	return m_fCameraAzimuth;
}


///////////////////////////////////////////////////////////////////////  
//  CView::GetCameraPitch

ST_INLINE st_float32 CView::GetCameraPitch(void) const
{
	return m_fCameraPitch;
}


///////////////////////////////////////////////////////////////////////  
//  CView::GetFrustumPoints

ST_INLINE const Vec3* CView::GetFrustumPoints(void) const
{
	return m_avFrustumPoints;
}


///////////////////////////////////////////////////////////////////////  
//  CView::GetFrustumPlanes

ST_INLINE const Vec4* CView::GetFrustumPlanes(void) const
{
	return m_avFrustumPlanes;
}


///////////////////////////////////////////////////////////////////////  
//  CView::GetFrustumExtents

ST_INLINE const CExtents& CView::GetFrustumExtents(void) const
{
	return m_cFrustumExtents;
}


///////////////////////////////////////////////////////////////////////  
//  CView::GetCameraFacingMatrix

ST_INLINE const Mat4x4& CView::GetCameraFacingMatrix(void) const
{
	return m_mCameraFacingMatrix;
}


///////////////////////////////////////////////////////////////////////  
//  CView::SetHorzBillboardFadeAngles

ST_INLINE void CView::SetHorzBillboardFadeAngles(st_float32 fStart, st_float32 fEnd)
{
	m_fHorzFadeStartAngle = fStart;
	m_fHorzFadeEndAngle = fEnd;
}


///////////////////////////////////////////////////////////////////////  
//  CView::GetHorzBillboardFadeAngles

ST_INLINE void CView::GetHorzBillboardFadeAngles(st_float32& fStart, st_float32& fEnd) const
{
	fStart = m_fHorzFadeStartAngle;
	fEnd = m_fHorzFadeEndAngle;
}


///////////////////////////////////////////////////////////////////////  
//  CView::GetHorzBillboardFadeValue

ST_INLINE st_float32 CView::GetHorzBillboardFadeValue(void) const
{
	return m_fHorzFadeValue;
}


///////////////////////////////////////////////////////////////////////
//  CForest::WindEnable

ST_INLINE void CForest::WindEnable(st_bool bEnabled)
{
	m_bWindEnabled = bEnabled;
}


///////////////////////////////////////////////////////////////////////
//  CForest::WindIsEnabled

ST_INLINE st_bool CForest::WindIsEnabled(void) const
{
	return m_bWindEnabled;
}


///////////////////////////////////////////////////////////////////////
//  CForest::WindEnableGusting

ST_INLINE void CForest::WindEnableGusting(const TTreePtrArray& aBaseTrees, st_bool bEnabled)
{
	for (size_t i = 0; i < aBaseTrees.size( ); ++i)
	{
		CTree* pBaseTree = aBaseTrees[i];
		assert(pBaseTree);

		pBaseTree->GetWind( ).EnableGusting(bEnabled);
	}

	m_bWindGustingEnabled = bEnabled;
}


///////////////////////////////////////////////////////////////////////
//  CForest::WindIsGustingEnabled

ST_INLINE st_bool CForest::WindIsGustingEnabled(void) const
{
	return m_bWindGustingEnabled;
}


///////////////////////////////////////////////////////////////////////
//  CForest::WindAdvance

ST_INLINE void CForest::WindAdvance(const TTreePtrArray& aBaseTrees, st_float32 fWallTimeInSecs)
{
	for (size_t i = 0; i < aBaseTrees.size( ); ++i)
	{
		CTree* pBaseTree = aBaseTrees[i];
		assert(pBaseTree);

		pBaseTree->GetWind( ).Advance(m_bWindEnabled, fWallTimeInSecs);
	}

	m_fWindTime = fWallTimeInSecs;
}


///////////////////////////////////////////////////////////////////////
//  CForest::WindPreroll

ST_INLINE void CForest::WindPreroll(const TTreePtrArray& aBaseTrees, st_float32 fWallTimeInSecs)
{
	const st_float32 c_fTickLength = 1.0f / 30.0f; // 30 ticks/sec
	const st_int32 c_nNumSteps = st_int32((fWallTimeInSecs / c_fTickLength) + 0.5f);

	st_float32 fTime = 0.0f;
	for (st_int32 nStep = 0; nStep < c_nNumSteps; ++nStep)
	{
		for (size_t i = 0; i < aBaseTrees.size( ); ++i)
			aBaseTrees[i]->GetWind( ).Advance(true, fTime);

		fTime += c_fTickLength;
	}
}


///////////////////////////////////////////////////////////////////////
//  CForest::WindSetStrength

ST_INLINE void CForest::WindSetStrength(const TTreePtrArray& aBaseTrees, st_float32 fTreeStrength)
{
	for (size_t i = 0; i < aBaseTrees.size( ); ++i)
	{
		CTree* pBaseTree = aBaseTrees[i];
		assert(pBaseTree);

		pBaseTree->GetWind( ).SetStrength(fTreeStrength);
	}
}


///////////////////////////////////////////////////////////////////////
//  CForest::WindSetInitDirection

ST_INLINE void CForest::WindSetInitDirection(const TTreePtrArray& aBaseTrees, const Vec3& vWindDir)
{
	for (size_t i = 0; i < aBaseTrees.size( ); ++i)
	{
		CTree* pBaseTree = aBaseTrees[i];
		assert(pBaseTree);

		pBaseTree->GetWind( ).SetInitDirection(vWindDir);
	}
}


///////////////////////////////////////////////////////////////////////
//  CForest::WindSetDirection

ST_INLINE void CForest::WindSetDirection(const TTreePtrArray& aBaseTrees, const Vec3& vWindDir)
{
	for (size_t i = 0; i < aBaseTrees.size( ); ++i)
	{
		CTree* pBaseTree = aBaseTrees[i];
		assert(pBaseTree);

		pBaseTree->GetWind( ).SetDirection(vWindDir);
	}

	m_vWindDir = vWindDir;
}


///////////////////////////////////////////////////////////////////////
//  CForest::WindGetGlobalTime

ST_INLINE st_float32 CForest::WindGetGlobalTime(void) const
{
	return m_fWindTime;
}


///////////////////////////////////////////////////////////////////////
//  CForest::FrameEnd

ST_INLINE void CForest::FrameEnd(void)
{
	++m_nFrameIndex;
	m_bLightDirChanged = false;
}


///////////////////////////////////////////////////////////////////////
//  CForest::GetFrameIndex

ST_INLINE st_int32 CForest::GetFrameIndex(void) const
{
	return m_nFrameIndex;
}


///////////////////////////////////////////////////////////////////////  
//  CForest::SetLightDir

ST_INLINE void CForest::SetLightDir(const Vec3& vLightDir)
{
	if (m_vLightDir != vLightDir)
	{
		m_vLightDir = vLightDir;
		m_bLightDirChanged = true;
	}
}


///////////////////////////////////////////////////////////////////////  
//  CForest::GetLightDir

ST_INLINE const Vec3& CForest::GetLightDir(void) const
{
	return m_vLightDir;
}


///////////////////////////////////////////////////////////////////////  
//  CForest::LightDirChanged

ST_INLINE st_bool CForest::LightDirChanged(void) const
{
	return m_bLightDirChanged;
}


///////////////////////////////////////////////////////////////////////  
//  CForest::SetCascadedShadowMapDistances

ST_INLINE void CForest::SetCascadedShadowMapDistances(const st_float32 afSplits[c_nMaxNumShadowMaps], st_float32 fFarClip)
{
	for (st_int32 i = 0; i < c_nMaxNumShadowMaps; ++i)
		m_afCascadedShadowMapSplits[i + 1] = afSplits[i] / fFarClip;

	m_bLightDirChanged = true;
}


///////////////////////////////////////////////////////////////////////  
//  CForest::GetCascadedShadowMapDistances

ST_INLINE const float* CForest::GetCascadedShadowMapDistances(void) const
{
	return m_afCascadedShadowMapSplits;
}


///////////////////////////////////////////////////////////////////////  
//  CForest::SetShadowFadePercentage

ST_INLINE void CForest::SetShadowFadePercentage(st_float32 fFade)
{
	m_fShadowFadePercentage = fFade;
}


///////////////////////////////////////////////////////////////////////  
//  CForest::GetShadowFadePercentage

ST_INLINE st_float32 CForest::GetShadowFadePercentage(void) const
{
	return m_fShadowFadePercentage;
}




