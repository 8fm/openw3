///////////////////////////////////////////////////////////////////////  
//  MyOverlays.h
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
#include "Forest/Forest.h"
#include "MyCmdLineOptions.h"
#include "MyConfigFile.h"
#include "MySpeedTreeRenderer.h"


///////////////////////////////////////////////////////////////////////  
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

	///////////////////////////////////////////////////////////////////////  
	//  Class CMyOverlays

	class CMyOverlays
	{
	public:
								CMyOverlays( );
								~CMyOverlays( );

			st_bool				InitGfx(const SMyCmdLineOptions& ssCmdLine, const CMyConfigFile& cConfigFile);
			void				ReleaseGfxResources(void);

			st_bool				Render(void) const;

	private:
			st_bool				InitGeometry(st_int32 nScreenWidth, st_int32 nScreenHeight, const CShaderTechnique& cTechnique);

			enum EAnchorType
			{
				ANCHOR_TOP_LEFT, ANCHOR_TOP_CENTER, ANCHOR_TOP_RIGHT,
				ANCHOR_CENTER_LEFT, ANCHOR_CENTER_CENTER,ANCHOR_CENTER_RIGHT,
				ANCHOR_BOTTOM_LEFT, ANCHOR_BOTTOM_CENTER, ANCHOR_BOTTOM_RIGHT
			};

			struct SOverlay
			{
								SOverlay( ) :
									m_fSize(0.25f),
									m_eAnchor(ANCHOR_CENTER_CENTER)
								{
									m_afPos[0] = m_afPos[1] = 0.5f;
								}

				CTexture		m_cImage;
				st_float32		m_afPos[2];
				st_float32		m_fSize;
				EAnchorType		m_eAnchor;
			};

			CArray<SOverlay>	m_aOverlays;
			CRenderState		m_cRenderState;		
			Mat4x4				m_mCompositeView; // modelview * projection
			CGeometryBuffer		m_cQuads; // two indexed triangles for each overlay
	};

} // end namespace SpeedTree
