///////////////////////////////////////////////////////////////////////
//  MyCmdLineOptions.cpp
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

#include "MyCmdLineOptions.h"
#include "Core/String.h"
#include "Utilities/Utility.h"

#ifdef __CELLOS_LV2__
	#include <sys/paths.h>
#endif

using namespace std;
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////
//  PrintUsage

void PrintUsage(const char* pExeName)
{
	Report("\tSpeedTree 6.3 Application (c) 2002-2013");
	Report("\nUsage:\n\n%s {<options>}\n", CFixedString(pExeName).NoPath( ).c_str( ));
	Report("where options can be (all units are same as the SRT & terrain files):\n");

	Report("\nFile options");
	Report("\t-sfc <text file>				loads SFC (SpeedTree Forest Config) file");

	Report("\nWindow options");
	Report("\t-res <width> <height>         window resolution");
	Report("\t-windowed                     makes the application run in a window");
	Report("\t-fullscreen                   makes the application run in fullscreen mode");
	Report("\t-samples <count>              multisampling configuration");
	Report("\t-anisotropy <value>           set the max texture anisotropy");
	Report("\t-gamma <value>                sets the screen gamma");
}


///////////////////////////////////////////////////////////////////////
//  SMyCmdLineOptions::SMyCmdLineOptions

SMyCmdLineOptions::SMyCmdLineOptions( ) :
	// display
	m_nWindowWidth(1280),
	m_nWindowHeight(720),
	m_bFullscreen(false),
	m_bFullscreenResOverride(false),
	m_bConsole(false),
	m_nNumSamples(1),
	m_nMaxAnisotropy(1),
	m_fGamma(1.0f)
{
}


///////////////////////////////////////////////////////////////////////
//  CMyCmdLineParser::Parse

st_bool CMyCmdLineParser::Parse(st_int32 argc, char* argv[], SMyCmdLineOptions& sConfig)
{
	st_bool bSuccess = false;
	
	st_bool bNeedsPrintUsage = false;
	if (argc == 1)
	{
		#ifdef _XBOX
			sConfig.m_strSfcFilename = "game:\\Forests\\Meadow\\Xbox360.sfc";
			bSuccess = true;
		#elif defined(_DURANGO)
			// Golden Mountain forests, forward & deferred (choose one)
			sConfig.m_nWindowWidth = 1920;
			sConfig.m_nWindowHeight = 1080;
			sConfig.m_nMaxAnisotropy = 1; // (>1) enabled anisotropic filtering
			
			// forward
			//sConfig.m_strSfcFilename = ".\\Meadow\\durango_deferred.sfc";
			//sConfig.m_nNumSamples = 4; // (>1) enables multisampling

			// deferred
			//
			// note: if vegetation is compiled with A2C enabled for deferred rendering, make sure
			//       the non-vegetation shaders are compiled (via the Compile Extra shaders dialog)
			//       in the Compiler app, with #define DEFERRED_A2C_ENABLED and	
			//		 #define NUM_MSAA_SAMPLES 2 where the value matches the app's.

			sConfig.m_strSfcFilename = ".\\Meadow\\durango_deferred.sfc";
			sConfig.m_nNumSamples = 1; // (>1) enables multisampling (currently unsupported for deferred; this sample looks pretty terrible w/o it)
			bSuccess = true;
		#elif defined(__CELLOS_LV2__) // PS3
			sConfig.m_strSfcFilename = "/app_home/Forests/Meadow/PS3.sfc";
			sConfig.m_nNumSamples = 2;
			bSuccess = true;
		#elif defined(__ORBIS__)
			sConfig.m_strSfcFilename = "/hostapp/Forests/Meadow/ps4_deferred.sfc";
			sConfig.m_nWindowWidth = 1920;
			sConfig.m_nWindowHeight = 1080;
			sConfig.m_nNumSamples = 1;
			sConfig.m_nMaxAnisotropy = 0;
			bSuccess = true;
		#elif defined(WIN32) || defined(__APPLE__)
			sConfig.m_strSfcFilename = "../../Forests/Meadow/Desktop.sfc";
			sConfig.m_nWindowWidth = 1280;
			sConfig.m_nWindowHeight = 720;
			sConfig.m_nNumSamples = 4;
			bSuccess = true;
		#elif defined(__GNUC__)
			sConfig.m_strSfcFilename = "../../Forests/Meadow/Desktop.sfc";
			sConfig.m_nWindowWidth = 1280;
			sConfig.m_nWindowHeight = 720;
			sConfig.m_nNumSamples = 4;
			bSuccess = true;
		#else
			bNeedsPrintUsage = true;
		#endif
	}
	else
	{
		// scan the options
		for (st_int32 i = 1; i < argc; ++i)
		{
			CFixedString strCommand = argv[i];
			if (strCommand == "-sfc")
			{
				if (i + 1 < argc)
				{
					sConfig.m_strSfcFilename = CFileSystemInterface::Get( )->CleanPlatformFilename(argv[++i]);
				}
				else
					bNeedsPrintUsage = true;
			}
			else if (strCommand == "-res")
			{
				if (i + 2 < argc)
				{
					sConfig.m_nWindowWidth = atoi(argv[++i]);
					sConfig.m_nWindowHeight = atoi(argv[++i]);
					sConfig.m_bFullscreenResOverride = true;
				}
				else
					bNeedsPrintUsage = true;
			}
			else if (strCommand == "-samples")
			{
				if (i + 1 < argc)
					sConfig.m_nNumSamples = st_max(1, atoi(argv[++i]));
				else
					bNeedsPrintUsage = true;
			}
			else if (strCommand == "-windowed" ||
					 strCommand == "-window")
			{
				sConfig.m_bFullscreen = false;
			}
			else if (strCommand == "-fullscreen")
			{
				sConfig.m_bFullscreen = true;
			}
			else if (strCommand == "-anisotropy")
			{
				if (i + 1 < argc)
				{
					sConfig.m_nMaxAnisotropy = atoi(argv[++i]);
				}
				else
					bNeedsPrintUsage = true;
			}
			else if (strCommand == "-gamma")
			{
				if (i + 1 < argc)
					sConfig.m_fGamma = st_float32(atof(argv[++i]));
				else
					bNeedsPrintUsage = true;
			}
			else if (strCommand == "-console")
			{
				sConfig.m_bConsole = true;
			}
			else
			{
				Warning("Unknown command [%s] (cmd-line argument #%d)\n", strCommand.c_str( ), i);
				bNeedsPrintUsage = true;
			}
		}
	}

	if (bNeedsPrintUsage)
		PrintUsage(argv[0]);
	else
		bSuccess = true;

	return bSuccess;
}



