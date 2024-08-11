///////////////////////////////////////////////////////////////////////  
//  NavigationBase.cpp
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

#include "MyNavigationBase.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  CMyNavigationBase::CMyNavigationBase

CMyNavigationBase::CMyNavigationBase( ) :
	m_fAzimuth(0.0f),
	m_fPitch(0.05f),
	m_bShiftDown(false),
	m_bControlDown(false),
	m_fControllerSpeed(1.0f),
	m_bSustainForwardMotion(false),
	m_bSustainBackwardMotion(false),
	m_bSustainLeftMotion(false),
	m_bSustainRightMotion(false),
	m_fTranslateSpeed(0.08f),
	m_fRotateSpeed(0.01f),
	m_eMouseButton(BUTTON_NONE)
{
	// mouse pressed state
	for (st_int32 nButton = BUTTON_NONE; nButton < BUTTON_COUNT; ++nButton)
		m_abMousePressed[nButton] = false;
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavigationBase::~CMyNavigationBase

CMyNavigationBase::~CMyNavigationBase( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavigationBase::Advance
//
//  Returns feet / sec speed

st_bool CMyNavigationBase::Advance(st_float32)
{
	return false;
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavigationBase::GetCameraTransform

const Mat4x4& CMyNavigationBase::GetCameraTransform(void)
{
	m_mTransform.SetIdentity( );

	// basically doing a polar to Cartesian conversion by getting an (x,y,z) target value from azimuth & pitch
	Vec3 vTarget = CCoordSys::ConvertFromStd(cosf(m_fAzimuth) * cosf(m_fPitch), sinf(m_fAzimuth) * cosf(m_fPitch), sinf(m_fPitch));

	// add 90.0 degrees to the pitch angle to define the up vector and recalculate
	Vec3 vUp = CCoordSys::ConvertFromStd(cosf(m_fAzimuth) * cosf(m_fPitch + c_fHalfPi), sinf(m_fAzimuth) * cosf(m_fPitch + c_fHalfPi), sinf(m_fPitch + c_fHalfPi));

	const Vec3 vOrigin;
	m_mTransform.LookAt(vOrigin, vTarget, vUp);

	m_mTransform.Translate(-m_vCameraPos.x, -m_vCameraPos.y, -m_vCameraPos.z);

	if (CCoordSys::IsLeftHanded( ))
	{
		m_mTransform.m_afRowCol[0][2] = -m_mTransform.m_afRowCol[0][2];
		m_mTransform.m_afRowCol[1][2] = -m_mTransform.m_afRowCol[1][2];
		m_mTransform.m_afRowCol[2][2] = -m_mTransform.m_afRowCol[2][2];
		m_mTransform.m_afRowCol[3][2] = -m_mTransform.m_afRowCol[3][2];
	}

	return m_mTransform;
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavigationBase::MouseButton

void CMyNavigationBase::MouseButton(EMouseButton, st_bool, st_int32, st_int32)
{
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavigationBase::MouseMotion

void CMyNavigationBase::MouseMotion(st_int32, st_int32)
{
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavigationBase::SetSpeedScalars

void CMyNavigationBase::SetSpeedScalars(st_float32 fTranslate, st_float32 fRotate)
{
    if (fTranslate != -1.0f)
        m_fTranslateSpeed = fTranslate;

    if (fRotate != -1.0f)
        m_fRotateSpeed = fRotate;
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavigationBase::AdjustAzimuth

void CMyNavigationBase::AdjustAzimuth(st_float32 fAmount)
{
	const st_float32 c_fAzimuthDir = CCoordSys::IsLeftHanded( ) ? -1.0f : 1.0f;
	m_fAzimuth += fAmount * c_fAzimuthDir;
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavigationBase::AdjustPitch

void CMyNavigationBase::AdjustPitch(st_float32 fAmount)
{
	m_fPitch += fAmount;
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavigationBase::MoveForward

void CMyNavigationBase::MoveForward(st_float32 fAmount)
{
	m_vCameraPos += CCoordSys::ConvertFromStd(fAmount * m_fControllerSpeed * cosf(m_fAzimuth) * cosf(m_fPitch),
											  fAmount * m_fControllerSpeed * sinf(m_fAzimuth) * cosf(m_fPitch),
											  fAmount * m_fControllerSpeed * sinf(m_fPitch));
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavigationBase::Strafe

void CMyNavigationBase::Strafe(st_float32 fAmount)
{
	Vec3 vDelta(CCoordSys::ConvertFromStd(fAmount * m_fControllerSpeed * cosf(m_fAzimuth + c_fHalfPi), 
										  fAmount * m_fControllerSpeed * sinf(m_fAzimuth + c_fHalfPi), 
										  0.0f));
	if (CCoordSys::IsLeftHanded( ))
		vDelta = -vDelta;

	m_vCameraPos += vDelta;
}
