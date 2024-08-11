///////////////////////////////////////////////////////////////////////  
//	MyParamAdjuster.cpp
//
//	All source files prefixed with "My" indicate an example implementation, 
//	meant to detail what an application might (and, in most cases, should)
//	do when interfacing with the SpeedTree SDK.  These files constitute 
//	the bulk of the SpeedTree Reference Application and are not part
//	of the official SDK, but merely example or "reference" implementations.
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization and may
//	not be copied or disclosed except in accordance with the terms of
//	that agreement
//
//      Copyright (c) 2003-2014 IDV, Inc.
//      All Rights Reserved.
//
//		IDV, Inc.
//		http://www.idvinc.com


/////////////////////////////////////////////////////////////////////
// Preprocessor

#include "MyParamAdjuster.h"
#include "Core/CoordSys.h"
#include "Utilities/Utility.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  CMyParamAdjuster::CMyParamAdjuster

CMyParamAdjuster::CMyParamAdjuster(EParamType eType) :
	m_vDir(1.0f, 0.0f, 0.0f),
	m_fHorzAngle(c_fHalfPi),
	m_fVertAngle(c_fQuarterPi),
	m_fFloatValue(1.0f),
	m_fFloatMin(0.1f),
	m_fFloatMax(2.0f),
	m_bActive(false),
	m_eType(eType),
	m_fSpeed(0.0025f)
{
}


///////////////////////////////////////////////////////////////////////  
//  CMyParamAdjuster::SetInitDir

void CMyParamAdjuster::SetInitDir(const Vec3& vDir)
{
    m_vDir = CCoordSys::ConvertFromStd(vDir);
	m_fHorzAngle = atan2(vDir.y, vDir.x) + c_fPi;
	m_fVertAngle = -asin(vDir.z);
}


///////////////////////////////////////////////////////////////////////  
//  CMyParamAdjuster::SetInitFloat

void CMyParamAdjuster::SetInitFloat(st_float32 fValue, st_float32 fMin, st_float32 fMax, st_float32 fSpeed)
{
	m_fFloatValue = fValue;
	m_fFloatMin = fMin;
	m_fFloatMax = fMax;
	m_fSpeed = fSpeed;
}


///////////////////////////////////////////////////////////////////////  
//  CMyParamAdjuster::Start

void CMyParamAdjuster::Start(st_int32 nMouseX, st_int32 nMouseY)
{
	m_bActive = true;
	m_nBaseMouseX = nMouseX;
	m_nBaseMouseY = nMouseY;
}


///////////////////////////////////////////////////////////////////////  
//  CMyParamAdjuster::Move

void CMyParamAdjuster::Move(st_int32 nMouseX, st_int32 nMouseY)
{
	if (m_bActive)
	{
		st_int32 nDeltaX = nMouseX - m_nBaseMouseX;
		st_int32 nDeltaY = nMouseY - m_nBaseMouseY;

		if (m_eType == DIRECTION)
		{
			ComputeDirection(nDeltaX, nDeltaY);
		}
		else if (m_eType == FLOAT)
		{
			ComputeFloat(nDeltaY);
		}

		m_nBaseMouseX = nMouseX;
		m_nBaseMouseY = nMouseY;
	}
}


///////////////////////////////////////////////////////////////////////  
//  CMyParamAdjuster::End

void CMyParamAdjuster::End(void)
{
	m_bActive = false;
}


///////////////////////////////////////////////////////////////////////  
//  CMyParamAdjuster::IsActive

st_bool CMyParamAdjuster::IsActive(void) const
{
	return m_bActive;
}


///////////////////////////////////////////////////////////////////////  
//  CMyParamAdjuster::ComputeDirection

void CMyParamAdjuster::ComputeDirection(st_int32 nDeltaX, st_int32 nDeltaY)
{
	const st_float32 c_fSpeed = 0.006f;
	if (abs(nDeltaX) >= abs(nDeltaY))
		m_fHorzAngle += (CCoordSys::IsLeftHanded( ) ? 1.0f : -1.0f) * nDeltaX * c_fSpeed;
	else
		m_fVertAngle += nDeltaY * -c_fSpeed;

	// keep the light above the horizon
	m_fVertAngle = Clamp(m_fVertAngle, 0.0f, c_fPi);

	// light direction is specified as it is in DirectX and OpenGL: the vector is the direction the light's
	// pointing; however, the vertex shaders expect a negated value during the actual constant upload
	m_vDir = CCoordSys::ConvertFromStd(-cosf(m_fHorzAngle) * cosf(m_fVertAngle), -sinf(m_fHorzAngle) * cosf(m_fVertAngle), -sinf(m_fVertAngle));
	m_vDir.Normalize( );
}


///////////////////////////////////////////////////////////////////////  
//  CMyParamAdjuster::ComputeFloat

void CMyParamAdjuster::ComputeFloat(st_int32 nDelta)
{
	m_fFloatValue -= nDelta * m_fSpeed;
	m_fFloatValue = Clamp<st_float32>(m_fFloatValue, m_fFloatMin, m_fFloatMax);
}


