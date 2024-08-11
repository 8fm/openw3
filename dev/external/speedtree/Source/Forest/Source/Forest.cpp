///////////////////////////////////////////////////////////////////////
//  Forest.cpp
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

#include "Forest/Forest.h"
#include "Utilities/Utility.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  CView::Set

st_bool CView::Set(const Vec3& vCameraPos,
				   const Mat4x4& mProjection,
				   const Mat4x4& mModelview,
				   st_float32 fNearClip,
				   st_float32 fFarClip,
				   st_bool bPreventLeafCardFlip)
{
	st_bool bChanged = false;

	if (vCameraPos != m_vCameraPos ||
		mProjection != m_mProjection ||
		mModelview != m_mModelview ||
		fNearClip != m_fNearClip ||
		fFarClip != m_fFarClip)
	{
		if (m_vLodRefPoint == m_vCameraPos)
			m_vLodRefPoint = vCameraPos;

		m_vCameraPos = vCameraPos;
		m_mProjection = mProjection;
		m_mModelview = mModelview;
		m_fNearClip = fNearClip;
		m_fFarClip = fFarClip;

		// modelview w/o translate (translate the camera pos back out)
		m_mModelviewNoTranslate = m_mModelview;
		m_mModelviewNoTranslate.Translate(m_vCameraPos);

		// composite view
		m_mComposite = m_mModelview * m_mProjection;

		// composite view w/o translate
		m_mCompositeNoTranslate = m_mModelviewNoTranslate * m_mProjection;

		// compute view inverse matrix
		m_mCompositeNoTranslate.Invert(m_mProjectionInverse);
		m_mProjectionInverse = m_mProjectionInverse.Transpose( );

		// camera direction
		m_vCameraDir.Set(-m_mModelview.m_afSingle[2], -m_mModelview.m_afSingle[6], -m_mModelview.m_afSingle[10]);

		// adjust camera angles based on camera direction
		m_fCameraAzimuth = atan2f(CCoordSys::OutComponent(m_vCameraDir), CCoordSys::RightComponent(m_vCameraDir));
		m_fCameraPitch = asinf(CCoordSys::UpComponent(m_vCameraDir));

		// possibly adjust camera and azimuth if an alternate coordinate system is used
		if (CCoordSys::IsLeftHanded( ))
		{
			m_fCameraAzimuth = -m_fCameraAzimuth;
			if (CCoordSys::IsYAxisUp( ))
				m_fCameraPitch = -m_fCameraPitch;
		}

		// compute horizontal fade angle
		m_fHorzFadeValue = (fabs(m_fCameraPitch) - m_fHorzFadeStartAngle) / (m_fHorzFadeEndAngle - m_fHorzFadeStartAngle);
		m_fHorzFadeValue = st_min(m_fHorzFadeValue, 1.0f);
		m_fHorzFadeValue = st_max(m_fHorzFadeValue, 0.0f);
		
		ComputeCameraFacingMatrix(bPreventLeafCardFlip);
		ComputeFrustumValues( );

		bChanged = true;
	}

	return bChanged;
}


///////////////////////////////////////////////////////////////////////  
//  CView::ComputeCameraFacingMatrix

void CView::ComputeCameraFacingMatrix(st_bool bPreventLeafCardFlip)
{
	if (bPreventLeafCardFlip)
	{
		m_mCameraFacingMatrix = m_mModelviewNoTranslate.Transpose( );
		if (CCoordSys::IsYAxisUp( ))
		{
			CCoordSys::RotateOutAxis(m_mCameraFacingMatrix, c_fHalfPi);
			CCoordSys::RotateUpAxis(m_mCameraFacingMatrix, c_fHalfPi);
		}
		else
		{
			CCoordSys::RotateUpAxis(m_mCameraFacingMatrix, c_fHalfPi);
			CCoordSys::RotateOutAxis(m_mCameraFacingMatrix, c_fHalfPi);
		}
	}
	else
	{
		m_mCameraFacingMatrix.SetIdentity( );
		CCoordSys::RotateUpAxis(m_mCameraFacingMatrix, m_fCameraAzimuth);
		if (!CCoordSys::IsLeftHanded( ) && CCoordSys::IsYAxisUp( ))
			CCoordSys::RotateOutAxis(m_mCameraFacingMatrix, m_fCameraPitch);
		else
			CCoordSys::RotateOutAxis(m_mCameraFacingMatrix, -m_fCameraPitch);
	}
}


///////////////////////////////////////////////////////////////////////  
//  CView::ComputeFrustumValues

void CView::ComputeFrustumValues(void)
{
	ComputeFrustumPoints(*this, m_avFrustumPoints);
	ExtractPlanes(m_mComposite, m_avFrustumPlanes);
	ComputeFrustumAABB(m_avFrustumPoints, m_cFrustumExtents);
}


///////////////////////////////////////////////////////////////////////
//  CForest::CForest

CForest::CForest( ) :
	// instance storage & culling
	  m_nFrameIndex(1)
	// wind
	, m_bWindEnabled(false)
	, m_bWindGustingEnabled(false)
	, m_fWindTime(0.0f)
	, m_vWindDir(1.0f, 0.0f, 0.0f)
	// shadows & lighting support
	, m_vLightDir(0.577f, 0.577f, 0.577f)
	, m_bLightDirChanged(true)
	, m_fShadowFadePercentage(0.85f)
{
	m_afCascadedShadowMapSplits[4] = 1.0f;
	m_afCascadedShadowMapSplits[3] = 0.5f * m_afCascadedShadowMapSplits[4];
	m_afCascadedShadowMapSplits[2] = 0.5f * m_afCascadedShadowMapSplits[3];
	m_afCascadedShadowMapSplits[1] = 0.5f * m_afCascadedShadowMapSplits[2];
	m_afCascadedShadowMapSplits[0] = 0.0f;
}


///////////////////////////////////////////////////////////////////////
//  CForest::~CForest

CForest::~CForest( )
{
}


///////////////////////////////////////////////////////////////////////
//  CForest::CollisionAdjust

struct SCollisionSort
{
	st_bool					operator<(const SCollisionSort& sRight) const { return (m_fDistance < sRight.m_fDistance); }

	st_float32				m_fDistance;
	const CTree*			m_pBaseTree;
	const CTreeInstance*	m_pInstance;
};

st_bool CForest::CollisionAdjust(Vec3& vPoint, const CVisibleInstances& cVisibleInstances, st_int32 nMaxNumTestTrees)
{
	st_bool bModified = false;

	// find the closest instances
	CStaticArray<SCollisionSort> aCollisionTrees(nMaxNumTestTrees + 1, "CForest::CollisionAdjust", false);
	const TDetailedCullDataArray& aPerBase3dInstances = cVisibleInstances.Get3dInstanceLods( );
	for (st_int32 nBase = 0; nBase < st_int32(aPerBase3dInstances.size( )); ++nBase)
	{
		if (!aPerBase3dInstances[nBase].m_a3dInstanceLods.empty( ))
		{
			const CTree* pBaseTree = aPerBase3dInstances[nBase].m_pBaseTree;
			assert(pBaseTree);

			st_int32 nNumCollision = 0;
			(void) pBaseTree->GetCollisionObjects(nNumCollision);
			if (nNumCollision == 0)
				continue;

			// look through instances of this base tree, finding the closest
			SCollisionSort sSortTest;
			for (st_int32 nInstance = 0; nInstance < st_int32(aPerBase3dInstances[nBase].m_a3dInstanceLods.size( )); ++nInstance)
			{
				sSortTest.m_pBaseTree = pBaseTree;
				sSortTest.m_pInstance = aPerBase3dInstances[nBase].m_a3dInstanceLods[nInstance].m_pInstance;
				sSortTest.m_fDistance = vPoint.DistanceSquared(sSortTest.m_pInstance->GetPos( ));
				
				aCollisionTrees.insert_sorted(sSortTest);
				if (st_int32(aCollisionTrees.size( )) > nMaxNumTestTrees)
					aCollisionTrees.resize(nMaxNumTestTrees);
			}
		}
	}

	// test the collision objects in the closest trees
	for (CArray<SCollisionSort>::const_iterator iCollision = aCollisionTrees.begin( ); iCollision != aCollisionTrees.end( ); ++iCollision)
	{
		Vec3 vPos = iCollision->m_pInstance->GetPos( );
		st_float32 fScalar = iCollision->m_pInstance->GetScalar( );
		Vec3 vecRight = iCollision->m_pInstance->GetRightVector( );
		Vec3 vecUp = iCollision->m_pInstance->GetUpVector( );
		Vec3 vecOut = vecUp.Cross(vecRight).Normalize( );
		if (!CCoordSys::IsLeftHanded( ) && CCoordSys::IsYAxisUp( ))
			vecOut *= -1.0f;

		Mat3x3 mRotation(vecRight, vecOut, vecUp);

		// query collision objects for this base tree
		st_int32 nNumCollisionObjects = 0;
		const SCollisionObject* pCollisionObjects = iCollision->m_pBaseTree->GetCollisionObjects(nNumCollisionObjects);
		if (pCollisionObjects)
		{
			for (st_int32 i = 0; i < nNumCollisionObjects; ++i)
			{
				const SCollisionObject* pOrigObject = pCollisionObjects + i;
				Vec3 vSphereCenter = (mRotation * pOrigObject->m_vCenter1) * fScalar + vPos;
				st_float32 fRadius = pOrigObject->m_fRadius * fScalar;

				if (pOrigObject->m_vCenter1 != pOrigObject->m_vCenter2)
				{
					Vec3 vSphereCenter2 = (mRotation * pOrigObject->m_vCenter2) * fScalar + vPos;

					Vec3 vStartToEnd(vSphereCenter2 - vSphereCenter);
					st_float32 fAlong = ((vPoint - vSphereCenter).Dot(vStartToEnd)) / vStartToEnd.MagnitudeSquared( );
					fAlong = st_max(0.0f, st_min(1.0f, fAlong));

					vSphereCenter = Interpolate(vSphereCenter, vSphereCenter2, fAlong);				
				}

				if (vPoint.DistanceSquared(vSphereCenter) < fRadius * fRadius)
				{
					Vec3 vDir = vPoint - vSphereCenter;
					vDir.Normalize( );
					vPoint = vSphereCenter + vDir * fRadius;
					bModified = true;
				}
			}
		}
	}

	return bModified;
}


///////////////////////////////////////////////////////////////////////
//  Round

inline st_float32 Round(st_float32 fValue)
{
	return st_float32(st_int32(fValue > 0.0 ? fValue + 0.5 : fValue - 0.5));
}


///////////////////////////////////////////////////////////////////////
//  CForest::ComputeLightView

st_bool CForest::ComputeLightView(const Vec3& vLightDir, 
								  const Vec3 avMainFrustum[8], 
								  st_float32 fMapStartPercent,	// percent [0.0,1.0] of main frustum's far distance
								  st_float32 fMapEndPercent,	// percent [0.0,1.0] of main frustum's far distance
								  CView& cLightView, 
								  st_float32 fRearExtension)
{
	// derive the points of the sub-frustum based on the start & end percentages
	const st_int32 c_nNumSubFrustumPoints = 16;
	Vec3 avSubFrustum[c_nNumSubFrustumPoints];

	// near
	avSubFrustum[0] = Interpolate<Vec3>(avMainFrustum[0], avMainFrustum[4], fMapStartPercent);
	avSubFrustum[1] = Interpolate<Vec3>(avMainFrustum[1], avMainFrustum[5], fMapStartPercent);
	avSubFrustum[2] = Interpolate<Vec3>(avMainFrustum[2], avMainFrustum[6], fMapStartPercent);
	avSubFrustum[3] = Interpolate<Vec3>(avMainFrustum[3], avMainFrustum[7], fMapStartPercent);

	// far
	avSubFrustum[4] = Interpolate<Vec3>(avMainFrustum[0], avMainFrustum[4], fMapEndPercent);
	avSubFrustum[5] = Interpolate<Vec3>(avMainFrustum[1], avMainFrustum[5], fMapEndPercent);
	avSubFrustum[6] = Interpolate<Vec3>(avMainFrustum[2], avMainFrustum[6], fMapEndPercent);
	avSubFrustum[7] = Interpolate<Vec3>(avMainFrustum[3], avMainFrustum[7], fMapEndPercent);

	// expand subfrustum toward the light direction by fRearExtension
	const Vec3 c_vLightExtension = vLightDir * fRearExtension;
	avSubFrustum[8] = avSubFrustum[0] - c_vLightExtension;
	avSubFrustum[9] = avSubFrustum[1] - c_vLightExtension;
	avSubFrustum[10] = avSubFrustum[2] - c_vLightExtension;
	avSubFrustum[11] = avSubFrustum[3] - c_vLightExtension;
	avSubFrustum[12] = avSubFrustum[4] - c_vLightExtension;
	avSubFrustum[13] = avSubFrustum[5] - c_vLightExtension;
	avSubFrustum[14] = avSubFrustum[6] - c_vLightExtension;
	avSubFrustum[15] = avSubFrustum[7] - c_vLightExtension;

	// compute geometric center of extended sub-frustum
	Vec3 vSubFrustumCenter(0.0f, 0.0f, 0.0f);
	for (st_int32 i = 0; i < c_nNumSubFrustumPoints; ++i)
		vSubFrustumCenter += avSubFrustum[i];
	vSubFrustumCenter /= st_float32(c_nNumSubFrustumPoints);

	// pick furthest point from center
	st_float32 fMaxDist = avSubFrustum[0].Distance(vSubFrustumCenter);
	for (st_int32 i = 1; i < c_nNumSubFrustumPoints; ++i)
		fMaxDist = st_max(fMaxDist, avSubFrustum[i].Distance(vSubFrustumCenter));

	// build modelview matrix
	Mat4x4 mModelview;
	const Vec3 c_vLookFrom = vSubFrustumCenter - vLightDir * fMaxDist;
	const Vec3 c_vLookTo = vSubFrustumCenter;
	mModelview.LookAt(c_vLookFrom, c_vLookTo, CCoordSys::UpAxis( ));

	// build projection matrix
	Mat4x4 mProjection;
	st_float32 fNearClip, fFarClip;
	{
		// assemble the test points
		CExtents cLightViewExtents;
		for (st_int32 i = 0; i < c_nNumSubFrustumPoints; ++i)
			cLightViewExtents.ExpandAround(mModelview * avSubFrustum[i]);

		// the further we bring in the near clip value, the further back the virtual light point goes; this brings a 
		// trade-off of showing more objects casting between the light point and the receiving object.  Showing those 
		// things that actually cast looks better, but we have to render a lot more into the shadow buffer
		fFarClip = -cLightViewExtents.Min( )[2];
		fNearClip = fFarClip - 2.0f * fMaxDist;

		// opengl and directx use different projection matrices since their clip space z-depth ranges differ
		// (range is [-1, 1] for opengl and [0,1] for directx)
		mProjection.Ortho(cLightViewExtents.Min( )[0],
						  cLightViewExtents.Max( )[0],
						  cLightViewExtents.Min( )[1],
						  cLightViewExtents.Max( )[1],
						  fNearClip, 
						  fFarClip, 
						  false);
	}

	// set SpeedTree's CView object
	(void) cLightView.Set(c_vLookFrom, mProjection, mModelview, fNearClip, fFarClip, false);

	return true;
}

