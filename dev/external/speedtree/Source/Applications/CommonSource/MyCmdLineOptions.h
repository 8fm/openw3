///////////////////////////////////////////////////////////////////////  
//  MyCmdLineOptions.h
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
#include "MyPopulate.h"


///////////////////////////////////////////////////////////////////////  
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{


	///////////////////////////////////////////////////////////////////////  
	//  Structure SMyCmdLineOptions

	struct SMyCmdLineOptions
	{
											SMyCmdLineOptions( );

		// forest-specific settings
		CFixedString						m_strSfcFilename;

		// display
		st_int32							m_nWindowWidth;
		st_int32							m_nWindowHeight;
		st_bool								m_bFullscreen;
		st_bool								m_bFullscreenResOverride;
		st_bool								m_bConsole;
		st_int32							m_nNumSamples;
		st_int32							m_nMaxAnisotropy;
		st_float32							m_fGamma;
	};


	///////////////////////////////////////////////////////////////////////  
	//  class CMyCmdLineParser declaration

	class CMyCmdLineParser
	{
	public:
	static	st_bool		Parse(st_int32 argc, char* argv[], SMyCmdLineOptions& sCmdLine);
	};

} // end namespace SpeedTree

