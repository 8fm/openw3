///////////////////////////////////////////////////////////////////////  
//  NavigationBase.h
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
#include "Core/Matrix.h"
#include "Utilities/Utility.h"


///////////////////////////////////////////////////////////////////////  
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

	///////////////////////////////////////////////////////////////////////  
	//  CMyNavigationBase
	//
	//  Geared mostly for terrain-style navigation, not object inspection

	class CMyNavigationBase  
	{
	public: 
			enum EMouseButton
			{
				BUTTON_NONE, BUTTON_LEFT, BUTTON_MIDDLE, BUTTON_RIGHT, BUTTON_COUNT
			};

        					CMyNavigationBase( );
	virtual 				~CMyNavigationBase( );

	virtual st_bool			Advance(st_float32 fFrameTime);
	virtual const Vec3&     GetCameraPos(void) const				{ return m_vCameraPos; }
	virtual st_float32		GetCameraAzimuth(void) const			{ return m_fAzimuth; }
	virtual st_float32		GetCameraPitch(void) const				{ return m_fPitch; }
	virtual	const Mat4x4&   GetCameraTransform(void);
	virtual	void            SetCameraPos(const Vec3& vPos)			{ m_vCameraPos = vPos; }
	virtual	void            MouseButton(EMouseButton eButton, st_bool bPressed, st_int32 nX, st_int32 nY);
	virtual	void            MouseMotion(st_int32 nX, st_int32 nY);
	virtual void            SetSpeedScalars(st_float32 fTranslate = -1.0f, st_float32 fRotate = -1.0f);
	virtual	void			SetShiftKeyState(st_bool bDown)			{ m_bShiftDown = bDown; }
	virtual	void			SetControlKeyState(st_bool bDown)		{ m_bControlDown = bDown; }

			// adjust camera
			void			AdjustAzimuth(st_float32 fAmount);
			void			AdjustPitch(st_float32 fAmount);
			void			MoveForward(st_float32 fAmount);
			void			Strafe(st_float32 fAmount);
	virtual	void			SustainForwardMotion(st_bool bSustain)	{ m_bSustainForwardMotion = bSustain; }
	virtual	void			SustainBackwardMotion(st_bool bSustain)	{ m_bSustainBackwardMotion = bSustain; }
	virtual	void			SustainLeftMotion(st_bool bSustain)		{ m_bSustainLeftMotion = bSustain; }
	virtual	void			SustainRightMotion(st_bool bSustain)	{ m_bSustainRightMotion = bSustain; }

	virtual st_bool			IsButtonDown(EMouseButton eButton)		{ return m_abMousePressed[static_cast<int>(eButton)]; }	
	virtual	st_bool			IsMoving(void) const					{ return true; }

			// queries
			st_bool			IsShiftDown(void) const					{ return m_bShiftDown; }
			st_bool			IsControlDown(void) const				{ return m_bControlDown; }

	protected:
			// orientation
			Vec3            m_vCameraPos;
			Mat4x4			m_mTransform;

			// orientation
			st_float32      m_fAzimuth;		// in radians
			st_float32      m_fPitch;		// in radians
			
			// modifiers
			st_bool			m_bShiftDown;
			st_bool			m_bControlDown;
			st_float32		m_fControllerSpeed;
			st_bool			m_bSustainForwardMotion;
			st_bool			m_bSustainBackwardMotion;
			st_bool			m_bSustainLeftMotion;
			st_bool			m_bSustainRightMotion;

			// speeds
			st_float32      m_fTranslateSpeed;
			st_float32      m_fRotateSpeed;

			// mouse buttons
			EMouseButton    m_eMouseButton;
			st_bool         m_abMousePressed[BUTTON_COUNT];
	};

} // end namespace SpeedTree
