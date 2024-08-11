///////////////////////////////////////////////////////////////////////  
//	MyParamAdjuster.h
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

#pragma once
#include "Core/Types.h"
#include "Core/Vector.h"


///////////////////////////////////////////////////////////////////////  
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

	///////////////////////////////////////////////////////////////////////  
	//  Class CMyParamAdjuster

	class CMyParamAdjuster
	{
	public:
			enum EParamType
			{
				DIRECTION,
				FLOAT
			};

							CMyParamAdjuster(EParamType eType = FLOAT);

			void			SetInitDir(const Vec3& vDir);
			void			SetInitFloat(st_float32 fValue, st_float32 fMin, st_float32 fMax, st_float32 fSpeed = 0.0025f);

			void			Start(st_int32 nMouseX, st_int32 nMouseY);
			void			Move(st_int32 nMouseX, st_int32 nMouseY);
			void			End(void);

			st_bool			IsActive(void) const;

			void			ComputeDirection(st_int32 nDeltaX = 0, st_int32 nDeltaY = 0);
			void			ComputeFloat(st_int32 nDelta = 0);

			// direction values
			Vec3			m_vDir;
			st_float32		m_fHorzAngle;
			st_float32		m_fVertAngle;

			// float values
			st_float32		m_fFloatValue;
			st_float32		m_fFloatMin;
			st_float32		m_fFloatMax;

	private:
			st_bool			m_bActive;
			EParamType		m_eType;
			st_float32		m_fSpeed;

			st_int32		m_nBaseMouseX;
			st_int32		m_nBaseMouseY;
	};

} // end namespace SpeedTree
