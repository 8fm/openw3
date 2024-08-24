//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKMP3TAGREADER_H
#define _AKMP3TAGREADER_H

#include <AK/SoundEngine/Common/AkTypes.h>
#include <windows.h>

#define AK_MP3_TAG_STRING_SIZE	256

struct AkMP3BaseInfo
{
	AkUInt16 uNumChannels;
	AkUInt16 uSampleRate;		
	AkTimeMs msDuration;		// duration of file, in milliseconds
	AkUInt32 uBitRate;			// kilobits / sec
	AkUInt32 uFirstFrameOffset;	// offset of first mpeg frame in file
	AkUInt32 uMPEGStreamSize;	// size of mpeg stream in file, not including tags
	AkUInt8 aToc[ 100 ];		// Xing table-of-contents
	bool bToc;					// Is Xing TOC valid?
};

struct AkMP3Tag
{
	WCHAR wszAlbum[ AK_MP3_TAG_STRING_SIZE ];
	WCHAR wszArtist[ AK_MP3_TAG_STRING_SIZE ];
	WCHAR wszTrack[ AK_MP3_TAG_STRING_SIZE ];
};

AK_CALLBACK( void, AkMP3Callback )( 
	void* in_pCookie,	///< In: cookie, as given to AkMP3PrepareForPlayback.
	AkTimeMs in_msPos	///< In: current position in MP3 file.
	);

// Returns true if MP3 file is considered valid for playback.
AK_FUNC( bool, AkMP3InfoRead )( 
	LPCTSTR in_tszFilename,			///< In: Full path of MP3 file to read.
	AkMP3BaseInfo & out_info,		///< Out: structure describing the MP3 file. Needs to be passed to AkMP3PrepareForPlayback.
	AkMP3Tag * out_pTag = NULL		///< Out: optional structure containing tags. Not parsed from file if NULL.
	);

AK_FUNC( void, AkMP3PrepareForPlayback )( 
	LPCTSTR in_tszFilename,					///< In: Full path of MP3 file to read.
	AkMP3BaseInfo & in_info,				///< In: structure describing the MP3 file, as given by AkMP3InfoRead.
	AkTimeMs in_msStartAt = 0,				///< In: optional playback start position, in milliseconds (should be in range [0, in_info.msDuration])
	AkMP3Callback in_funcCallback = NULL,	///< In: optional callback function. will be called every audio frame.
	void * in_pCallbackCookie = NULL		///< In: cookie which will be passed to callback function.
	);

#endif // _AKMP3TAGREADER_H