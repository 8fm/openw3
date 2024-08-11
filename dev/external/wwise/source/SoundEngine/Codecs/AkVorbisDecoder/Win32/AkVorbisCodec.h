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
// AkVorbisCodec.h
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_VORBIS_CODEC_H_
#define _AK_VORBIS_CODEC_H_

#include "AkSrcVorbis.h"

inline AkUInt16 ReadU16(AkUInt16* pAddress)
{
	return *pAddress;
}

//================================================================================
// Decoding audio data packets
//================================================================================
inline void UnpackPacket( AkUInt8 * in_pHeaderPtr, AkUInt8* in_pOggPacketPtr, bool in_bEndOfStream, ogg_packet & out_Packet )
{
	AkUInt16 uPacketSize = ReadU16((AkUInt16*)in_pHeaderPtr);
	
	out_Packet.buffer.data = in_pOggPacketPtr;
	out_Packet.buffer.size = uPacketSize;
	out_Packet.e_o_s = in_bEndOfStream;
}

struct AkVorbisPacketReader
{
	inline AkVorbisPacketReader( AkUInt8* in_pFirstPacketStart, AkUInt32 in_uTotalSize )
	{
		m_pFirstPacketStart = in_pFirstPacketStart;
		m_uTotalSize = in_uTotalSize;
		m_uCurrentOffset = 0;
	}

	inline AKRESULT ReadPacket( AkVorbisDecoderState & io_eState, AkUInt16 in_uMaxPacketSize, bool in_bNoMoreInputPackets, ogg_packet & out_Packet )
	{
		if ( m_uCurrentOffset + sizeof(OggPacketHeader) <= m_uTotalSize )
		{
			AkUInt8* pCurPtr = (AkUInt8*)m_pFirstPacketStart + m_uCurrentOffset;
			AkUInt16 uPacketSize = ReadU16((AkUInt16*)pCurPtr);
			if ( AK_EXPECT_FALSE( uPacketSize > in_uMaxPacketSize ) )
				return AK_Fail;
			if ( AK_EXPECT_FALSE( io_eState == END_OF_STREAM ) ) // happens with sample-accurate looping
				return AK_DataNeeded;
			if ( m_uCurrentOffset + sizeof(OggPacketHeader) + uPacketSize <= m_uTotalSize )
			{
				m_uCurrentOffset += sizeof(OggPacketHeader) + uPacketSize;

				out_Packet.buffer.data = pCurPtr + sizeof(OggPacketHeader);
				out_Packet.buffer.size = uPacketSize;
				out_Packet.e_o_s = ( DidConsumeAll() && in_bNoMoreInputPackets );
				if ( out_Packet.e_o_s )
					io_eState = END_OF_STREAM;

				return AK_DataReady;
			}
			else
			{
				return AK_DataNeeded;
			}
			
		}
		else
		{
			return AK_DataNeeded;
		}
	}

	inline AkUInt32 GetSizeConsumed( )
	{
		return m_uCurrentOffset;
	}

	inline bool DidConsumeAll( )
	{
		return (m_uCurrentOffset == m_uTotalSize);
	}

private:
	AkUInt8*	m_pFirstPacketStart;
	AkUInt32 	m_uCurrentOffset;
	AkUInt32	m_uTotalSize;
};

struct AkTremorInfoReturn
{
	AkUInt32			uFramesProduced;		// Frames produced by decoder
	AKRESULT			eDecoderStatus;			// Decoder status
	AkVorbisDecoderState eDecoderState;			// Current decoder state
	AkUInt32			uInputBytesConsumed;	// Size of all packets consumed
};

struct AkTremorInfo
{
	AkTremorInfoReturn	ReturnInfo;				// the codec will update and return this part
	vorbis_dsp_state	VorbisDSPState;			// central working state for the packet->PCM decoder
	AkUInt8*			pucData;				// Pointer to output location
	AkUInt32			uChannelMask;			// Channels to decode	
	AkUInt32			uRequestedFrames;		// Requested frames to decode
	AkUInt32			uInputDataSize;			// Data size of all input (not necessarly a number of packets)
	bool				bNoMoreInputPackets;	// Signal to the codec that there will not be anymore incoming packets after this
};

// Information common to Vorbis file and Vorbis bank (members)
struct AkVorbisSourceState
{
	AkTremorInfo			TremorInfo;				// Information used by the codec for decoding
	AkVorbisInfo			VorbisInfo;				// Additional information from encoder
	AkUInt32				uSampleRate;			// Sample rate
	AkVorbisSeekTableItem*	pSeekTable;				// Seek table storage (only allocated if necessary)
	AkUInt32				uSeekTableSizeRead;		// Size read from seek table
};

void DecodeVorbis( AkTremorInfo* in_pTremorInfo, AkUInt16 in_uMaxPacketSize, AkUInt8* in_pInputBuf, AkInt16* in_pOuputBuf );

static inline AKRESULT VorbisSeek( 
	const AkVorbisSourceState & in_vorbisState,
	AkUInt32 in_uDesiredSample,		// Desired sample position in file.
	AkUInt32 & out_uSeekedSample,	// Returned sample where file position was set.
	AkUInt32 & out_uFileOffset		// Returned file offset where file position was set.
	)
{
	if ( in_uDesiredSample == 0 )
	{
		out_uSeekedSample = 0;
		out_uFileOffset = in_vorbisState.VorbisInfo.dwVorbisDataOffset;
		return AK_Success;
	}

	AkUInt32 uNumSeekTableItems = in_vorbisState.VorbisInfo.dwSeekTableSize / sizeof(AkVorbisSeekTableItem);
	if ( !in_vorbisState.pSeekTable || !uNumSeekTableItems )
		return AK_Fail;

	// Sequentially run in seek table to determine seek position
	// TODO: Bisection algorithm would yield better performance
	
	AkUInt32 uCurFileOffset = 0;
	AkUInt32 uCurFrameOffset = 0;
	AkUInt32 index = 0;

	while ( index < uNumSeekTableItems )
	{
		AkUInt32 uNextFrameOffset = uCurFrameOffset + in_vorbisState.pSeekTable[index].uPacketFrameOffset;
		if ( uNextFrameOffset > in_uDesiredSample )
			break;

		AkUInt32 uNextFileOffset = uCurFileOffset + in_vorbisState.pSeekTable[index].uPacketFileOffset;
		uCurFileOffset = uNextFileOffset;
		uCurFrameOffset = uNextFrameOffset;
		++index;
	}

	if ( index > 0 )
	{
		out_uFileOffset = uCurFileOffset + in_vorbisState.VorbisInfo.dwSeekTableSize;
		out_uSeekedSample = uCurFrameOffset;
	}
	else
	{
		out_uFileOffset = in_vorbisState.VorbisInfo.dwVorbisDataOffset;
		out_uSeekedSample = 0;
	}

	return AK_Success;
}


#endif // _AK_VORBIS_CODEC_H_
