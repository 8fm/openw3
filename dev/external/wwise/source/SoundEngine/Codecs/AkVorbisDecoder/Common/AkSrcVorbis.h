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
// AkSrcVorbis.h
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_SRC_VORBIS_H_
#define _AK_SRC_VORBIS_H_

#include "ogg.h"		// Ogg layer
#include "ivorbiscodec.h"
#include "codec_internal.h"

#include "AkVorbisInfo.h"	// Encoder information

enum AkVorbisDecoderState
{
	UNINITIALIZED			= 0,	// need to read header
	INITIALIZED				= 1,	// need to read seek table
	SEEKTABLEINTIALIZED		= 2,	// need to read codebook headers

	PACKET_STREAM  = 3,				// steady state
	END_OF_STREAM = 4				// end of packet stream (loop end or file end)
};

#endif // _AK_SRC_VORBIS_H_
