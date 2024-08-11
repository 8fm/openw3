///////////////////////////////////////////////////////////////////////  
//  MouseNavigation.h
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

#pragma once
#include "MyNavigationBase.h"
#include "Core/FixedArray.h"


///////////////////////////////////////////////////////////////////////  
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

	///////////////////////////////////////////////////////////////////////  
	//  Class CMyMouseNavigation
	//
	//  Geared mostly for terrain-style navigation, not object inspection

	class CMyMouseNavigation : public CMyNavigationBase  
	{
	public: 
							CMyMouseNavigation( );

			st_bool			Advance(st_float32 fFrameTime);

			void			DemoModeToggle(st_float32 afTime[2], st_float32 afSpeed[2]);
			st_bool			IsDemoMode(void) { return m_bDemoMode; }

			void            MouseButton(EMouseButton eButton, st_bool bPressed, st_int32 nX, st_int32 nY);
			void            MouseMotion(st_int32 nX, st_int32 nY);

	virtual	void			SustainForwardMotion(st_bool bSustain);
	virtual	void			SustainBackwardMotion(st_bool bSustain);
	virtual	void			SustainLeftMotion(st_bool bSustain);
	virtual	void			SustainRightMotion(st_bool bSustain);
	virtual	st_bool			IsMoving(void) const;

			void			SaveCamera(st_int32 nWhich, const Vec3& vLight);
			void			LoadCamera(st_int32 nWhich);
			void			SaveCameraFile(const CFixedString& strFilename);
			void			LoadCameraFile(const CFixedString& strFilename);
			Vec3			GetCurrentCameraLightDir(void) { return m_aSavedCameras[m_nLastCamera].m_vLightDir; }

	private:

			// mouse mechanics
			st_int32        m_nBaseY;
			st_int32        m_nDeltaY;
			st_int32        m_nLastX;
			st_int32        m_nLastY;

			// demo mode
			st_bool			m_bDemoMode;
			st_float32		m_afDemoRandomTime[2];	// demo camera will stay on in location between [0] and [1] seconds
			st_float32		m_afDemoRandomSpeed[2];
			st_float32		m_fDemoCameraTimer;
			Vec3			m_vDemoVelocity;
			st_int32		m_nLastCamera;

			// saved camera states
			struct SCameraSave
			{
				SCameraSave(void) :
					m_fAzimuth(0.0f),
					m_fPitch(0.0f),
					m_vLightDir(0.2f, -0.7f, -0.7f)
				{
				}
				Vec3		m_vPos;
				st_float32	m_fAzimuth;
				st_float32	m_fPitch;
				Vec3		m_vLightDir;
			};
			CFixedArray<SCameraSave, 10>	m_aSavedCameras;
	};

} // end namespace SpeedTree
