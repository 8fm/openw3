///////////////////////////////////////////////////////////////////////  
//  MyMouseNavigation.cpp
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

#include "MyMouseNavigation.h"
#include "Core/CoordSys.h"
#include "Core/Random.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//	Constants

const st_float32 c_fSpeedUpScalar = 3.0f;
const st_float32 c_fSlowDownScalar = 0.15f;


///////////////////////////////////////////////////////////////////////  
//  CMyMouseNavigation::CMyMouseNavigation

CMyMouseNavigation::CMyMouseNavigation( ) :
    m_nBaseY(0),
    m_nDeltaY(0),
    m_nLastX(0),
    m_nLastY(0),
	m_bDemoMode(false),
	m_fDemoCameraTimer(0.0f),
	m_nLastCamera(0)
{
	m_aSavedCameras.resize(10);

	m_afDemoRandomTime[0] = 8.0f;
	m_afDemoRandomTime[1] = 15.0f;
	m_afDemoRandomSpeed[0] = 1.0f;
	m_afDemoRandomSpeed[1] = 7.0f;
}


///////////////////////////////////////////////////////////////////////  
//  CMyMouseNavigation::Advance

st_bool CMyMouseNavigation::Advance(st_float32 fFrameTime)
{
	if (m_bDemoMode)
	{
		static CRandom cDemoRandom(rand( ));

		m_fDemoCameraTimer -= fFrameTime;
		if (m_fDemoCameraTimer < 0.0f)
		{
			st_int32 nNewCamera = m_nLastCamera;
			while (nNewCamera == m_nLastCamera)
				nNewCamera = cDemoRandom.GetInteger(0, 9);
			LoadCamera(nNewCamera);
			m_nLastCamera = nNewCamera;

			m_vDemoVelocity.Set(cDemoRandom.GetFloat(-1.0f, 1.0f),
								cDemoRandom.GetFloat(-1.0f, 1.0f),
								cDemoRandom.GetFloat(-0.5f, 0.5f));
			m_vDemoVelocity.Normalize( );
			m_vDemoVelocity *= cDemoRandom.GetFloat(m_afDemoRandomSpeed[0], m_afDemoRandomSpeed[1]);
			m_fDemoCameraTimer = cDemoRandom.GetFloat(m_afDemoRandomTime[0], m_afDemoRandomTime[1]);
		}

		m_vCameraPos += CCoordSys::ConvertFromStd(m_vDemoVelocity * fFrameTime);
	}
	else
	{
		const st_float32 c_fForwardSpeed = 300.0f;
		const st_float32 c_fKeyScalar = (m_bShiftDown ? c_fSpeedUpScalar : 1.0f) * (m_bControlDown ? c_fSlowDownScalar : 1.0f);
		if (m_eMouseButton == BUTTON_LEFT || m_nDeltaY != 0)
		{
			MoveForward(c_fKeyScalar * m_fTranslateSpeed * fFrameTime * -m_nDeltaY);
		}

		if (m_bSustainForwardMotion || m_bSustainBackwardMotion)
		{
			st_int32 nDirection = static_cast<int>(m_bSustainForwardMotion) - static_cast<int>(m_bSustainBackwardMotion);
			MoveForward(c_fKeyScalar * m_fTranslateSpeed * fFrameTime * c_fForwardSpeed * nDirection);
		}

		if (m_bSustainRightMotion || m_bSustainLeftMotion)
		{
			st_int32 nDirection = static_cast<int>(m_bSustainLeftMotion) - static_cast<int>(m_bSustainRightMotion);
			Strafe(c_fKeyScalar * m_fTranslateSpeed * fFrameTime * c_fForwardSpeed * 0.5f * nDirection);
		}
	}

    return true;
}


///////////////////////////////////////////////////////////////////////  
//  CMyMouseNavigation::IsMoving

st_bool CMyMouseNavigation::IsMoving(void) const
{
	return m_bDemoMode || m_bSustainForwardMotion || m_bSustainBackwardMotion || 
			m_bSustainLeftMotion || m_bSustainRightMotion || m_abMousePressed[BUTTON_LEFT] || 
			m_abMousePressed[BUTTON_MIDDLE] || m_abMousePressed[BUTTON_RIGHT];
}


///////////////////////////////////////////////////////////////////////  
//  CMyMouseNavigation::MouseButton

void CMyMouseNavigation::MouseButton(EMouseButton eButton, st_bool bPressed, st_int32 nX, st_int32 nY)
{
	if (!m_bDemoMode)
	{
		m_eMouseButton = eButton;
		m_abMousePressed[eButton] = bPressed;

		if (bPressed && eButton == BUTTON_LEFT)
		{
			m_nDeltaY = 0;
			m_nBaseY = nY;
		}
	}
	else
	{
		if (bPressed && eButton == BUTTON_LEFT)
			m_fDemoCameraTimer = -1.0f;
	}

    m_nLastX = nX;
    m_nLastY = nY;
}


///////////////////////////////////////////////////////////////////////  
//  CMyMouseNavigation::MouseMotion

void CMyMouseNavigation::MouseMotion(st_int32 nX, st_int32 nY)
{
	if (!m_bDemoMode)
	{
		const st_float32 c_fKeyScalar = (m_bShiftDown ? c_fSpeedUpScalar : 1.0f) * (m_bControlDown ? c_fSlowDownScalar : 1.0f);

		if (m_abMousePressed[BUTTON_LEFT])
		{
			AdjustAzimuth(-(nX - m_nLastX) * m_fRotateSpeed);
			m_nDeltaY = nY - m_nBaseY;
		}
		else if (m_abMousePressed[BUTTON_MIDDLE])
		{
			const st_float32 c_fDeltaAdjust = 1.0f;
			AdjustAzimuth(-nX * c_fDeltaAdjust * m_fRotateSpeed);
			AdjustPitch(-nY * c_fDeltaAdjust * m_fRotateSpeed);
		}
		else if (m_abMousePressed[BUTTON_RIGHT])
		{
			Strafe(nX * -c_fKeyScalar * m_fTranslateSpeed * 0.5f);
			m_vCameraPos += CCoordSys::UpAxis( ) * st_float32(-nY) * c_fKeyScalar * m_fTranslateSpeed;
		}
	}

	m_nLastX = nX;
	m_nLastY = nY;
}


///////////////////////////////////////////////////////////////////////  
//  CMyMouseNavigation::SustainForwardMotion

void CMyMouseNavigation::SustainForwardMotion(st_bool bSustain)
{
	CMyNavigationBase::SustainForwardMotion(bSustain);
	m_nDeltaY = 0;
}


///////////////////////////////////////////////////////////////////////  
//  CMyMouseNavigation::SustainBackwardMotion

void CMyMouseNavigation::SustainBackwardMotion(st_bool bSustain)
{
	CMyNavigationBase::SustainBackwardMotion(bSustain);
	m_nDeltaY = 0;
}


///////////////////////////////////////////////////////////////////////  
//  CMyMouseNavigation::SustainLeftMotion

void CMyMouseNavigation::SustainLeftMotion(st_bool bSustain)
{
	CMyNavigationBase::SustainLeftMotion(bSustain);
	m_nDeltaY = 0;
}


///////////////////////////////////////////////////////////////////////  
//  CMyMouseNavigation::SustainRightMotion

void CMyMouseNavigation::SustainRightMotion(st_bool bSustain)
{
	CMyNavigationBase::SustainRightMotion(bSustain);
	m_nDeltaY = 0;
}


///////////////////////////////////////////////////////////////////////  
//  CMyMouseNavigation::SaveCamera

void CMyMouseNavigation::SaveCamera(st_int32 nWhich, const Vec3& vLight)
{
	if (nWhich > -1 && nWhich < st_int32(m_aSavedCameras.size( )))
	{
		m_aSavedCameras[nWhich].m_vPos = m_vCameraPos;
		m_aSavedCameras[nWhich].m_fAzimuth = m_fAzimuth;
		m_aSavedCameras[nWhich].m_fPitch = m_fPitch;
		m_aSavedCameras[nWhich].m_vLightDir = vLight;
		m_nLastCamera = nWhich;
		Report("Saved camera %d\n", nWhich);
	}
}


///////////////////////////////////////////////////////////////////////  
//  CMyMouseNavigation::LoadCamera

void CMyMouseNavigation::LoadCamera(st_int32 nWhich)
{
	if (nWhich > -1 && nWhich < st_int32(m_aSavedCameras.size( )))
	{
		m_vCameraPos = m_aSavedCameras[nWhich].m_vPos;
		m_fAzimuth = m_aSavedCameras[nWhich].m_fAzimuth;
		m_fPitch = m_aSavedCameras[nWhich].m_fPitch;
		m_nLastCamera = nWhich;
	}
}


///////////////////////////////////////////////////////////////////////  
//  CMyMouseNavigation::SaveCameraFile

void CMyMouseNavigation::SaveCameraFile(const CFixedString& strFilename)
{
	FILE* pFile = fopen(strFilename.c_str( ), "w");
	if (pFile != NULL)
	{
		for (CArray<SCameraSave>::const_iterator iter = m_aSavedCameras.begin( ); iter != m_aSavedCameras.end( ); ++iter)
		{
			Vec3 vPos = CCoordSys::ConvertFromStd(iter->m_vPos);
			fprintf(pFile, "%g %g %g %g %g %g %g %g\n", vPos[0], vPos[1], vPos[2], iter->m_fAzimuth, iter->m_fPitch, iter->m_vLightDir.x, iter->m_vLightDir.y, iter->m_vLightDir.z);
		}
		fclose(pFile);
	}
}


///////////////////////////////////////////////////////////////////////  
//  CMyMouseNavigation::LoadCameraFile

void CMyMouseNavigation::LoadCameraFile(const CFixedString& strFilename)
{
	if (strFilename.empty( ))
		return;

	FILE* pFile = fopen(strFilename.c_str( ), "r");
	if (pFile != NULL)
	{
		m_aSavedCameras.clear( );
		while (!feof(pFile))
		{
			SCameraSave sNew;
			if (fscanf(pFile, "%g %g %g %g %g %g %g %g", &sNew.m_vPos[0], &sNew.m_vPos[1], &sNew.m_vPos[2], &sNew.m_fAzimuth, &sNew.m_fPitch, &sNew.m_vLightDir.x, &sNew.m_vLightDir.y, &sNew.m_vLightDir.z) == 8)
			{
				sNew.m_vPos = CCoordSys::ConvertFromStd(sNew.m_vPos);
				m_aSavedCameras.push_back(sNew);
			}
		}
		fclose(pFile);
	}

	// make sure we still have 10
	m_aSavedCameras.resize(10);	
}


///////////////////////////////////////////////////////////////////////  
//  CMyMouseNavigation::DemoModeToggle

void CMyMouseNavigation::DemoModeToggle(st_float32 afTime[2], st_float32 afSpeed[2])
{
	m_bDemoMode = !m_bDemoMode;
	m_fDemoCameraTimer = 0.0f;
	m_afDemoRandomTime[0] = afTime[0];
	m_afDemoRandomTime[1] = afTime[1];
	m_afDemoRandomSpeed[0] = afSpeed[0];
	m_afDemoRandomSpeed[1] = afSpeed[1];
}
