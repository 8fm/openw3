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
#ifndef _AK_FILE_PARSER_BASE_H_
#define _AK_FILE_PARSER_BASE_H_

#include "AkPrivateTypes.h"
#include <AK/Tools/Common/AkMonitorError.h>

//-----------------------------------------------------------------------------
// Constants.
//-----------------------------------------------------------------------------

// Note: Both RIFF and RIFX exist (they inform us on the endianness of the file),
// but this parser uses the platform defined AkPlatformRiffChunkId (which translates
// to one of them according to the endianness of the platform).
static const AkFourcc RIFXChunkId = AkmmioFOURCC('R', 'I', 'F', 'X');
static const AkFourcc RIFFChunkId = AkmmioFOURCC('R', 'I', 'F', 'F');
static const AkFourcc WAVEChunkId = AkmmioFOURCC('W', 'A', 'V', 'E');
/// TEMP
static const AkFourcc xWMAChunkID = AkmmioFOURCC('X', 'W', 'M', 'A');
///
static const AkFourcc fmtChunkId  = AkmmioFOURCC('f', 'm', 't', ' ');
// AK-specific chunks.
static const AkFourcc seekTableChunkId = AkmmioFOURCC('s', 'e', 'e', 'k');
static const AkFourcc analysisDataChunkId = AkmmioFOURCC('a', 'k', 'd', ' ');

// Other, standard chunks.
static const AkFourcc dataChunkId = AkmmioFOURCC('d', 'a', 't', 'a');
static const AkFourcc factChunkId = AkmmioFOURCC('f', 'a', 'c', 't');
static const AkFourcc wavlChunkId = AkmmioFOURCC('w', 'a', 'v', 'l');
static const AkFourcc slntChunkId = AkmmioFOURCC('s', 'l', 'n', 't');
static const AkFourcc cueChunkId  = AkmmioFOURCC('c', 'u', 'e', ' ');
static const AkFourcc plstChunkId = AkmmioFOURCC('p', 'l', 's', 't');
static const AkFourcc LISTChunkId = AkmmioFOURCC('L', 'I', 'S', 'T');
static const AkFourcc adtlChunkId = AkmmioFOURCC('a', 'd', 't', 'l');
static const AkFourcc lablChunkId = AkmmioFOURCC('l', 'a', 'b', 'l');
static const AkFourcc noteChunkId = AkmmioFOURCC('n', 'o', 't', 'e');
static const AkFourcc ltxtChunkId = AkmmioFOURCC('l', 't', 'x', 't');
static const AkFourcc smplChunkId = AkmmioFOURCC('s', 'm', 'p', 'l');
static const AkFourcc instChunkId = AkmmioFOURCC('i', 'n', 's', 't');
static const AkFourcc rgnChunkId  = AkmmioFOURCC('r', 'g', 'n', ' ');
static const AkFourcc JunkChunkId = AkmmioFOURCC('J', 'U', 'N', 'K');

static const AkUInt8 HAVE_FMT	= 0x01;
static const AkUInt8 HAVE_DATA	= 0x02;
static const AkUInt8 HAVE_CUES	= 0x04;
static const AkUInt8 HAVE_SMPL	= 0x08;
static const AkUInt8 HAVE_SEEK	= 0x10;

//-----------------------------------------------------------------------------
// Structs.
//-----------------------------------------------------------------------------
#pragma pack(push, 1)
struct AkChunkHeader
{
	AkFourcc	ChunkId;
	AkUInt32	dwChunkSize;
};



// This is a copy of WAVEFORMATEX
struct WaveFormatEx
{	
	AkUInt16  	wFormatTag;
	AkUInt16  	nChannels;
	AkUInt32  	nSamplesPerSec;
	AkUInt32  	nAvgBytesPerSec;
	AkUInt16  	nBlockAlign;
	AkUInt16  	wBitsPerSample;
	AkUInt16    cbSize;	// size of extra chunk of data, after end of this struct
};

// WAVEFORMATEXTENSIBLE without the format GUID
// Codecs that require format-specific chunks should extend this structure.
struct WaveFormatExtensible : public WaveFormatEx
{
	AkUInt16    wSamplesPerBlock;
	AkUInt32    dwChannelMask;
};

#pragma pack(pop)

struct CuePoint
{
	AkUInt32   dwIdentifier;
	AkUInt32   dwPosition;
	AkFourcc  fccChunk;       // Unused. Wav lists are not supported.
	AkUInt32   dwChunkStart;   // Unused. Wav lists are not supported.
	AkUInt32   dwBlockStart;   // Unused. Wav lists are not supported.
	AkUInt32   dwSampleOffset;
};

struct LabelCuePoint
{
	AkUInt32   dwCuePointID;
	char	  strLabel[1]; // variable-size string
};

struct Segment 
{
	AkUInt32    dwIdentifier;
	AkUInt32    dwLength;
	AkUInt32    dwRepeats;
};

struct SampleChunk
{
	AkUInt32     dwManufacturer;
	AkUInt32     dwProduct;
	AkUInt32     dwSamplePeriod;
	AkUInt32     dwMIDIUnityNote;
	AkUInt32     dwMIDIPitchFraction;
	AkUInt32     dwSMPTEFormat;
	AkUInt32     dwSMPTEOffset;
	AkUInt32     dwSampleLoops;
	AkUInt32     cbSamplerData;
};

struct SampleLoop
{
	AkUInt32     dwIdentifier;
	AkUInt32     dwType;
	AkUInt32     dwStart;
	AkUInt32     dwEnd;
	AkUInt32     dwFraction;
	AkUInt32     dwPlayCount;
};

class CAkMarkers;

namespace AkFileParser
{
	struct FormatInfo
	{
		AkUInt32				uFormatSize;	// WaveFormatExtensible size.
		WaveFormatExtensible *	pFormat;		// Pointer to format data (WaveFormatExtensible or extended struct).
	};

	struct AnalysisData
	{
		// Max loudness.
		AkReal32 		fLoudnessNormalizationGain;	// Normalization gain due to loudness analysis. Used iif "Enable Normalization" is on.
		AkReal32 		fDownmixNormalizationGain;	// Normalization gain due to downmix. Used all the time.
		
		// Envelope.
		AkUInt32		uNumEnvelopePoints;	// Number of entries in envelope table.
		AkReal32 		fEnvelopePeak;		// Largest envelope point (dB).
		EnvelopePoint	arEnvelope[1];		// Variable array of envelope points.
	};

	struct AnalysisDataChunk
	{
		AnalysisDataChunk() : uDataSize( 0 ), pData( NULL ) {}

		AkUInt32		uDataSize;	// Size of the data pointed by pData.
		AnalysisData *	pData;		// Data. 
	};

    struct SeekInfo
	{
		AkUInt32	uSeekChunkSize;	// (Format-specific) seek table size.
		void *		pSeekTable;		// (Format-specific) seek table content.
	};

    extern AKRESULT Parse( 
		const void *	in_pvBuffer,				// Buffer to be parsed.
		AkUInt32		in_ulBufferSize,			// Buffer size.
		FormatInfo &	out_pFormatInfo,			// Returned audio format info.
		CAkMarkers *	out_pMarkers,				// Markers. NULL if not wanted. (Mandatory for markers).
		AkUInt32 *		out_pulLoopStart,			// Loop start position (in sample frames).
		AkUInt32 *		out_pulLoopEnd,				// Loop end position (in sample frames).
		AkUInt32 *		out_pulDataSize,			// Size of data portion of the file.
		AkUInt32 *		out_pulDataOffset,			// Offset in file to the data.
		AnalysisDataChunk * out_pAnalysisData,		// Returned analysis data chunk (pass NULL if not wanted).
		SeekInfo *		out_pSeekTableInfo			// (Format-specific) seek table info (pass NULL if not wanted).
		);

	// choose appropriate monitoring message given result returned by above function 
	extern AK::Monitor::ErrorCode ParseResultToMonitorMessage( AKRESULT );
}

#endif //_AK_FILE_PARSER_BASE_H_
