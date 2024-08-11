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

#include "AkVorbisCodec.h"
#include <AK/SoundEngine/Common/AkCommonDefs.h>

//================================================================================
// decode a bunch of vorbis packets
//================================================================================
void DecodeVorbis( AkTremorInfo* in_pTremorInfo, AkUInt16 in_uMaxPacketSize, AkUInt8* in_pInputBuf, AkInt16* in_pOuputBuf )
{
	AkVorbisPacketReader PacketReader( in_pInputBuf, in_pTremorInfo->uInputDataSize );

	AkUInt32 uFramesToWrite = in_pTremorInfo->uRequestedFrames;

	do
	{
		AkUInt32 uFramesWritten = vorbis_dsp_pcmout( &in_pTremorInfo->VorbisDSPState, in_pOuputBuf, uFramesToWrite );
		if ( uFramesWritten == 0 )
		{
			// No decoded frames ready to output? Read and decode a new packet.

			ogg_packet Packet;
			AKRESULT eResult = PacketReader.ReadPacket( in_pTremorInfo->ReturnInfo.eDecoderState, in_uMaxPacketSize, in_pTremorInfo->bNoMoreInputPackets, Packet );
			if ( eResult == AK_Fail )
			{
				in_pTremorInfo->ReturnInfo.uFramesProduced = 0;
				in_pTremorInfo->ReturnInfo.eDecoderStatus = AK_Fail;
				return;
			}			
			else if ( eResult == AK_DataNeeded )
				break; // No more input packets in buffer. Will return the data we have decoded so far.

			vorbis_dsp_synthesis(&in_pTremorInfo->VorbisDSPState, &Packet);
		}
		else
		{
			in_pOuputBuf += uFramesWritten * in_pTremorInfo->VorbisDSPState.channels;
			uFramesToWrite -= uFramesWritten;
		}
	}
	while ( uFramesToWrite );
	
	in_pTremorInfo->ReturnInfo.uFramesProduced = in_pTremorInfo->uRequestedFrames - uFramesToWrite;
	in_pTremorInfo->ReturnInfo.uInputBytesConsumed = PacketReader.GetSizeConsumed();

	// is it the end of the stream ?
	if( in_pTremorInfo->ReturnInfo.eDecoderState == END_OF_STREAM && !vorbis_dsp_pcmout( &in_pTremorInfo->VorbisDSPState, NULL, 0 ) )
	{
		in_pTremorInfo->ReturnInfo.eDecoderStatus = AK_NoMoreData;
	}
	else
	{
		in_pTremorInfo->ReturnInfo.eDecoderStatus = in_pTremorInfo->ReturnInfo.uFramesProduced ? AK_DataReady : AK_NoDataReady;
	}
}
