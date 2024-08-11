/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/

//////////////////////////////////////////////////////////////////////
//
// AkCritical.h
//
// AudioKinetic Critical section system for Win32
//
//////////////////////////////////////////////////////////////////////
#ifndef _CRITICAL_H_
#define _CRITICAL_H_

#include <AK/Tools/Common/AkLock.h>

///////////////////////////////////////////////////////////////////////////////////////////
//AkFunctionCritical is intended to be use when an entire function must be set as critical
//
// To use this utility simply add:
//		CAkFunctionCritical SpaceSetAsCritical;
//
// At the beginning of a function
///////////////////////////////////////////////////////////////////////////////////////////
extern CAkLock g_csMain;

class CAkFunctionCritical
{
public:
	AkForceInline CAkFunctionCritical()
	{
		g_csMain.Lock();
	};

	AkForceInline ~CAkFunctionCritical()
	{
		g_csMain.Unlock();
	};
};
#endif
