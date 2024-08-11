///////////////////////////////////////////////////////////////////////  
//  MySky.h
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
#define MY_SKY_ACTIVE // if not defined, will disable all initialization and rendering of all sky geometry
#include "Utilities/Utility.h"
#include "MySpeedTreeRenderer.h"
#include "MyCmdLineOptions.h"


///////////////////////////////////////////////////////////////////////  
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

	// if compiled w/o terrain active, use stubbed-out class definition to keep the
	// MyApplication.* files from getting too cluttered
	#ifdef MY_SKY_ACTIVE


	///////////////////////////////////////////////////////////////////////  
	//  Class CMySky

	class CMySky : public CSkyRender
	{
	public:
									CMySky( );
	virtual							~CMySky( );

			// flags
			void					SetActive(st_bool bActive);
			st_bool					IsActive(void) const;

			// initialization
			st_bool					Init(const CMyConfigFile& cConfigFile);

			// rendering
			st_bool					Render(CRenderStats& cStats); // will call SetShaderConstants internally

	private:
			st_bool					m_bActive;
			CTexture				m_texSkydome;
	};


	#else // MY_SKY_ACTIVE


	///////////////////////////////////////////////////////////////////////  
	//  Class CMySky

	class CMySky : public CSkyRender
	{
	public:
									CMySky( )							{ }
	virtual							~CMySky( )							{ }

			// flags
			void					SetActive(st_bool)					{ }
			st_bool					IsActive(void) const				{ return false; }

			// initialization
			st_bool					Init(const CMyConfigFile&)			{ return true; }

			// rendering
			st_bool					Render(CRenderStats&)				{ return true; }
	};

	#endif // MY_SKY_ACTIVE

} // end namespace SpeedTree
