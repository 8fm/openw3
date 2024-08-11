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
// AkVorbisInfo.h
//
// Structures common to sound engine and Wwise encoder
//
///////////////////////////////////////////////////////////////////////

#ifndef _AK_VORBISINFO_H_
#define _AK_VORBISINFO_H_

#include <AK/SoundEngine/Common/AkTypes.h>

#ifdef __SPU__

#define SIZEOF_OGGPACKETHEADER 2 // Need to update if structure below changes

#else

#include "AkFileParserBase.h"

#pragma pack(push, 1)
struct AkVorbisLoopInfo
{
	AkUInt32 dwLoopStartPacketOffset;	// File offset of packet containing loop start sample
	AkUInt32 dwLoopEndPacketOffset;		// File offset of packet following the one containing loop end sample
	AkUInt16 uLoopBeginExtra;			// Number of extra audio frames in loop begin
	AkUInt16 uLoopEndExtra;				// Number of extra audio frames in loop end
};

struct AkVorbisSeekTableItem
{
	AkUInt16 uPacketFrameOffset;		// Granule position (first PCM frame) of Ogg packet
	AkUInt16 uPacketFileOffset;			// File offset of packet in question
};

struct AkVorbisHeaderBase
{
	AkUInt32 dwTotalPCMFrames;			// Total number of encoded PCM frames
};

struct AkVorbisInfo
{
	AkVorbisLoopInfo LoopInfo;			// Looping related information
	AkUInt32 dwSeekTableSize;			// Size of seek table items (0 == not present)
	AkUInt32 dwVorbisDataOffset;		// Offset in data chunk to first audio packet
	AkUInt16 uMaxPacketSize;			// Maximum packet size
	AkUInt16 uLastGranuleExtra;			// Number of extra audio frames in last granule
	AkUInt32 dwDecodeAllocSize;			// Decoder expected allocation size
	AkUInt32 dwDecodeX64AllocSize;		// Decoder expected allocation size on platform with 64bits pointers.
	AkUInt32 uHashCodebook;
	AkUInt8  uBlockSizes[2];
};

struct AkVorbisHeader
	: public AkVorbisHeaderBase
	, public AkVorbisInfo
{
};

struct WaveFormatVorbis : public WaveFormatExtensible
{
	AkVorbisHeader	vorbisHeader;
};

struct OggPacketHeader
{
	AkUInt16 uPacketSize;
};
#define SIZEOF_OGGPACKETHEADER sizeof(OggPacketHeader)

#pragma pack(pop)

#endif

#endif // _AK_VORBISINFO_H_
