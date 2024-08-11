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
// AudiolibDefs.h
//
// AkAudioLib Internal defines
//
//////////////////////////////////////////////////////////////////////
#ifndef _AUDIOLIB_DEFS_H_
#define _AUDIOLIB_DEFS_H_

#include "PlatformAudiolibDefs.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>

//----------------------------------------------------------------------------------------------------
// (Upper) Renderer.
//----------------------------------------------------------------------------------------------------
#define RENDERER_DEFAULT_POOL_ID				(g_DefaultPoolId)

//----------------------------------------------------------------------------------------------------
// Behavioral engine.
//----------------------------------------------------------------------------------------------------
#define DEFAULT_CONTINUOUS_PLAYBACK_LOOK_AHEAD	(1)	// 1 audio frame sync refill

//----------------------------------------------------------------------------------------------------
// Sources.
//----------------------------------------------------------------------------------------------------
// Invalid ID (AkPluginID for plugins).
#define AK_INVALID_SOURCE_ID                    (AK_UINT_MAX)

//----------------------------------------------------------------------------------------------------
// SrcFile.
//----------------------------------------------------------------------------------------------------

#define AK_MARKERS_POOL_ID	                    (g_LEngineDefaultPoolId)

//----------------------------------------------------------------------------------------------------
// Paths
//----------------------------------------------------------------------------------------------------

#define DEFAULT_MAX_NUM_PATHS                   (255)

// max number of users a path can have (<=255)
#define AK_PATH_USERS_LIST_SIZE					(8)

//----------------------------------------------------------------------------------------------------
// Transitions
//----------------------------------------------------------------------------------------------------

#define DEFAULT_MAX_NUM_TRANSITIONS             (255)

// max number of users a transition can have (<=255)
#define	AK_TRANSITION_USERS_LIST_SIZE			(255)

//----------------------------------------------------------------------------------------------------
//  Voice Mgr
//----------------------------------------------------------------------------------------------------

#define AK_DEFAULT_LISTENER				(0)
#define AK_DEFAULT_LISTENER_MASK		(0x01)
#define AK_ALL_LISTENERS_MASK			(0x000000ff)
#define AK_ALL_REMOTE_LISTENER_MASK		(0x0000000f)


//----------------------------------------------------------------------------------------------------
// SwitchCntr
//----------------------------------------------------------------------------------------------------
#define AK_MAX_SWITCH_LIST_SIZE			AK_NO_MAX_LIST_SIZE// should be infinite, or max possible
#define AK_MAX_CONTINUOUS_ITEM_IN_SWLIST	AK_NO_MAX_LIST_SIZE// should be infinite, or max possible

//----------------------------------------------------------------------------------------------------
// Pipeline sample types
//----------------------------------------------------------------------------------------------------
#define AUDIOSAMPLE_FLOAT_MIN -1.f
#define AUDIOSAMPLE_FLOAT_MAX 1.f
#define AUDIOSAMPLE_SHORT_MIN -32768.f
#define AUDIOSAMPLE_SHORT_MAX 32767.f
#define AUDIOSAMPLE_UCHAR_MAX 255.f
#define AUDIOSAMPLE_UCHAR_MIN 0.f

// Below this volume, a node won't bother sending out data to the mix bus
#define AK_OUTPUT_THRESHOLD						(0.5f/32768.0f)

#endif //_AUDIOLIB_DEFS_H_

