///////////////////////////////////////////////////////////////////////  
//  My2dInterpolator.h
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
#include "Core/Core.h"
#include "Utilities/Utility.h"


///////////////////////////////////////////////////////////////////////  
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

	///////////////////////////////////////////////////////////////////////  
	//	class CMy2dInterpolator

	template<class T>
	class CMy2dInterpolator
	{
	public:
									CMy2dInterpolator( ) : 
										m_nWidth(0), 
										m_nHeight(0), 
										m_pData(NULL) 
									{ 
									}

		void						Clear(void);
		T							NearestNeighbor(st_float32 fU, st_float32 fV) const;
		T							InterpolateValue(st_float32 fU, st_float32 fV) const;
		T							InterpolateValueClamped(st_float32 fU, st_float32 fV) const;
		T							Smooth(st_float32 fU, st_float32 fV, st_float32 fDistance, st_float32 fSlope, st_bool bClamped) const;
		st_bool						IsPresent(void) const;

		st_int32					m_nWidth;
		st_int32					m_nHeight;
		T*							m_pData;
	};

	// include inline functions
	#include "My2dInterpolator_inl.h"

} // end namespace SpeedTree
