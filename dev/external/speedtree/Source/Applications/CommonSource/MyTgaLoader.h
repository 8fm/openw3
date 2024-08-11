///////////////////////////////////////////////////////////////////////  
//  MyTgaLoader.h
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
#include "Core/Types.h"


///////////////////////////////////////////////////////////////////////  
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{


	///////////////////////////////////////////////////////////////////////  
	//	Class CMyTgaLoader

	class CMyTgaLoader
	{
	public:
							CMyTgaLoader(void);
							CMyTgaLoader(st_uchar* pImageData, st_int32 nWidth, st_int32 nHeight, st_int32 nDepth, st_bool bCopyData = true);
							~CMyTgaLoader(void);

			bool			Read(const char* pFilename);
							
			st_int32		GetWidth(void) const			{ return m_nWidth; }
			st_int32		GetHeight(void) const			{ return m_nHeight; }
			st_int32		GetDepth(void) const			{ return m_nDepth; } // in bytes (3 = RGB, 4 = RGBA)
			st_uchar*		GetRawData(void) const			{ return m_pImageData; }

			st_uchar*		GetPixel(st_int32 nWidth, st_int32 nHeight) const;
			void			SetPixel(const st_uchar* pValue, st_int32 nWidth, st_int32 nHeight);

	private:
			st_int32		m_nHeight;			// in pixels
			st_int32		m_nWidth;			// in pixels
			st_int32		m_nDepth;			// in bytes (3 = RGB, 4 = RGBA)
			st_uchar*		m_pImageData;
			st_bool			m_bDeleteData;
	};

} // end namespace SpeedTree


