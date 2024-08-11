///////////////////////////////////////////////////////////////////////  
//  MyText.h
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
#include "Core/String.h"

#ifdef SPEEDTREE_DIRECTX9
	#include "SDKmisc.h"
#endif

#ifdef _XBOX
	#include <xtl.h>
	#include "AtgFont.h"
#endif


///////////////////////////////////////////////////////////////////////  
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

	///////////////////////////////////////////////////////////////////////  
	//	Class CMyText

	class CMyText
	{
	public:
								CMyText( );

			bool				Init(void);
			void				ReleaseGfxResources(void);

			bool				BeginText(void);
			bool				DrawLine(const st_char* pText);
			void				EndText(void);

#ifdef SPEEDTREE_DIRECTX9
			void				OnLostDevice(void);
			void				OnResetDevice(void);
#endif

#ifdef _XBOX
	static	void				SetFont(ATG::Font* pFont);
#endif

	private:	
			st_int32			m_nOffset;
#ifdef SPEEDTREE_DIRECTX9
			CDXUTTextHelper*	m_pHelper;
			ID3DXFont*			m_pFont9;
			ID3DXSprite*		m_pSprite9;
#endif
#ifdef _WIN32
			st_uint32			m_dlLists;
			st_int32			m_anSizes[95][2];
#endif
#ifdef _XBOX
	static	ATG::Font*			m_pFont;
#endif
	};

} // end namespace SpeedTree


