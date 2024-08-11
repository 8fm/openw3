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

////////////////////////////////////////////////////////////////////////
// RumbleBus.h
//
// Final mix bus for the rumble function of the game controllers.
//
// Copyright 2007 Audiokinetic Inc.
//
// Author:  mjean
// Version: 1.0
//
///////////////////////////////////////////////////////////////////////
#pragma once

#if defined AK_WIN || defined AK_XBOX360
#include "RumbleBusWindowsXBox.h"
#elif defined AK_XBOXONE
#include "RumbleBusXboxOne.h"
#elif defined AK_PS3
#include "RumbleBusPS3.h"
#elif defined AK_WII_FAMILY
#include "RumbleBusWiiWiiU.h"
#elif defined AK_PS4
#include "RumbleBusPS4.h"
#endif
