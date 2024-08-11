///////////////////////////////////////////////////////////////////////
//  MySky.cpp
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

#include "MySky.h"
#ifdef MY_SKY_ACTIVE
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  CMySky::CMySky

CMySky::CMySky( ) :
	m_bActive(true)
{
}


///////////////////////////////////////////////////////////////////////  
//  CMySky::~CMySky

CMySky::~CMySky( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CMySky::SetActive

void CMySky::SetActive(st_bool bActive)
{
	m_bActive = bActive;
}


///////////////////////////////////////////////////////////////////////  
//  CMySky::IsActive

st_bool CMySky::IsActive(void) const
{
	return m_bActive;
}


///////////////////////////////////////////////////////////////////////  
//  CMySky::Init

st_bool CMySky::Init(const CMyConfigFile& cConfigFile)
{
	st_bool bSuccess = false;

	// set up render options from app's command-line options
	SSkyRenderInfo sInfo;

	// app states
	sInfo.m_sAppState.m_bMultisampling = true;
	sInfo.m_sAppState.m_bAlphaToCoverage = false;
	sInfo.m_sAppState.m_bDepthPrepass = false;
	sInfo.m_sAppState.m_bDeferred = cConfigFile.m_sDeferredRender.m_bEnabled;

	// shader file location
	sInfo.m_strShaderPath = cConfigFile.m_sSky.m_strShaderPath;

	// sun
	sInfo.m_vSunColor = cConfigFile.m_sSky.m_vSunColor;
	sInfo.m_fSunSize = cConfigFile.m_sSky.m_fSunSize;
	sInfo.m_fSunSpreadExponent = cConfigFile.m_sSky.m_fSunSpreadExponent;

	// sky/fog
	sInfo.m_vSkyColor = cConfigFile.m_sSky.m_vColor;
	sInfo.m_vFogColor = cConfigFile.m_sFog.m_vColor;
	sInfo.m_fFogStartDistance = cConfigFile.m_sFog.m_afLinear[0];
	sInfo.m_fFogEndDistance = cConfigFile.m_sFog.m_afLinear[1];
	sInfo.m_fFogDensity = cConfigFile.m_sFog.m_fDensity;
	sInfo.m_fSkyFogMin = cConfigFile.m_sSky.m_afFogRange[0];
	sInfo.m_fSkyFogMax = cConfigFile.m_sSky.m_afFogRange[1];
	sInfo.m_fNearClip = cConfigFile.m_sWorld.m_fNearClip;
	sInfo.m_fFarClip = cConfigFile.m_sWorld.m_fFarClip;
	sInfo.m_strCloudTextureFilename = cConfigFile.m_sSky.m_strTexture.c_str( );

	// set up the SDK's sky render class
	SetRenderInfo(sInfo);
	bSuccess = CSkyRender::Init( );

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMySky::Render

st_bool CMySky::Render(CRenderStats& cStats)
{
	st_bool bSuccess = false;

	if (m_bActive)
	{
		bSuccess &= CSkyRender::Render(cStats);
	}
	else
		bSuccess = true; // is inactive, so doing nothing is success

	return bSuccess;
}


#endif // MY_SKY_ACTIVE
