/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version:  Build: 
  Copyright (c) 2006-2020 Audiokinetic Inc.
 ***********************************************************************/

//////////////////////////////////////////////////////////////////////
//
// PlatformAudiolibDefs.h
//
// AkAudioLib Internal defines
//
//////////////////////////////////////////////////////////////////////
#ifndef _PLATFORM_AUDIOLIB_DEFS_H_
#define _PLATFORM_AUDIOLIB_DEFS_H_

//----------------------------------------------------------------------------------------------------
// Audio file format
//----------------------------------------------------------------------------------------------------
// Little-Endian platforms RIFF ID
#define AkPlatformRiffChunkId RIFFChunkId

//----------------------------------------------------------------------------------------------------
// Voice manager
//----------------------------------------------------------------------------------------------------
#define AK_MIN_NUM_REFILLS_IN_VOICE_BUFFER		(1)
#define AK_DEFAULT_NUM_REFILLS_IN_VOICE_BUFFER	(4)
#define AK_DEFAULT_NUM_SAMPLES_PER_FRAME		(1024)
#define AK_DEFAULT_SAMPLE_TYPE					(AkDataTypeID)(AK_FLOAT)

#endif //_PLATFORM_AUDIOLIB_DEFS_H_
