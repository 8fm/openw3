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
// AudiolibLimitations.h
//
// AkAudioLib Internal Maximas defines
//
//////////////////////////////////////////////////////////////////////
#ifndef _AUDIOLIB_LIMITATIONS_H_
#define _AUDIOLIB_LIMITATIONS_H_

// Important note!
// AK_MAX_NUM_CHILD cannot go over 65535 without modifying the core format of the PlayHistory object, actually storing position on a WORD
#define AK_MAX_NUM_CHILD						65535
#define AK_MAX_NUM_PLAYLIST_ITEM				AK_MAX_NUM_CHILD

#define AK_MAX_HIERARCHY_DEEP					32

#define AK_MAX_NUM_CHANNELS						8 // absolute number of channels, application-wide

#define AK_NUM_EFFECTS_PROFILER					(4)

#define AK_NUM_EFFECTS_BYPASS_ALL_FLAG			(AK_NUM_EFFECTS_PROFILER)

#define AK_MAX_GAME_DEFINED_AUX_PER_OBJ_PROFILER	(4)

#if defined(AK_WII) || defined(AK_3DS)
#define AK_NUM_EFFECTS_PER_OBJ					(1) 
#else
#define AK_NUM_EFFECTS_PER_OBJ					(AK_NUM_EFFECTS_PROFILER) 
#endif

#define AK_NUM_AUX_SEND_PER_OBJ					(4)
#define AK_NUM_AUX_SEND_PER_OBJ_PROFILER		(4)

#define AK_MAX_STRING_SIZE						260
#define AK_MAX_DEVICE_NUM						16

// max number of vertices in the path buffer (<=255)
#define AK_PATH_VERTEX_LIST_SIZE				(64)
#define AK_MAX_NUM_PATH							(64)

// Maximum number of effects per object

#endif //_AUDIOLIB_LIMITATIONS_H_
