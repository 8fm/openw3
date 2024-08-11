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

#include "LinearResampler.h"
#include <AK/Tools/Common/AkAssert.h>

namespace DSP
{

	void CAkLinearResampler::Execute(	
		AkAudioBuffer * io_pInBuffer, 
		AkUInt32		in_uInOffset,
		AkAudioBuffer * io_pOutBuffer,
		AkReal32		in_fResamplingFactor)
	{
		const AkUInt16 uInBufferFrames = io_pInBuffer->uValidFrames;
		const AkUInt32 uOutBufferFrames = io_pOutBuffer->MaxFrames() - io_pOutBuffer->uValidFrames;
		const AkUInt32 uNumChannels = AkMin( io_pInBuffer->NumChannels(), io_pOutBuffer->NumChannels() );

		AkReal32 fInterpLoc;
		AkReal32 fFrameSkip = 100.f/in_fResamplingFactor;
		AkUInt16 uFramesConsumed;
		AkUInt16 uFramesProduced;

		for ( AkUInt32 i = 0; i < uNumChannels; i++ )
		{
			AkReal32 * AK_RESTRICT pInBuf = (AkReal32 * AK_RESTRICT) io_pInBuffer->GetChannel( i ) + in_uInOffset; 
			AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( i ) + io_pOutBuffer->uValidFrames;
			fInterpLoc = m_fInterpLoc;
			uFramesConsumed = 0;
			uFramesProduced = 0;

			while ( true )
			{
				AkReal32 fPrevSample;
				AkReal32 fNextSample;
				if ( fInterpLoc < 0.f )
				{
					fPrevSample = m_fPastVals[i];
					fNextSample = pInBuf[0];
				}
				else
				{
					AkUInt16 uIndex = (AkUInt16)fInterpLoc;		
					// Check input condition
					if ( uIndex >= (uInBufferFrames-1) )
					{
						if ( uIndex > (uInBufferFrames-1) )
						{
							// Both previous and next frames are in the next buffer
							// Keep only fractional part for next buffer
							// No need to cache previous sample
							fInterpLoc -= (AkReal32)uInBufferFrames;
						}
						else
						{
							// Previous frame in this buffer, but next frame only in next buffer
							// Cache and consume previous so that it can be interpolated with next on subsequent iteration
							m_fPastVals[i] = pInBuf[uIndex];
							fInterpLoc -= (AkReal32)(uIndex+1);
						}	
						uFramesConsumed = uInBufferFrames;
						break;
					}

					// In all other cases we have both samples
					fPrevSample = pInBuf[uIndex];
					fNextSample = pInBuf[uIndex+1];
				}
				// Check output condition
				if ( uFramesProduced == uOutBufferFrames )
				{
					// Cache and consume previous so that it can be interpolated with next on subsequent iteration
					m_fPastVals[i] = fPrevSample;
					if ( fInterpLoc > 0.f )
					{
						AkUInt16 uIndex = (AkUInt16)fInterpLoc;
						fInterpLoc -= (AkReal32)(uIndex+1);
						uFramesConsumed = uIndex+1;
					}
					break;
				}

				AkReal32 fSampleDiff = fNextSample - fPrevSample;
				AkReal32 fXDistPrevSample;
				if ( fInterpLoc >= 0.f )
					fXDistPrevSample = fInterpLoc - (AkUInt32)fInterpLoc;
				else
					fXDistPrevSample = 1.f + fInterpLoc;
				*pfOutBuf++ = fPrevSample + ( fXDistPrevSample * fSampleDiff );
				uFramesProduced++;
				fInterpLoc += fFrameSkip;	
			}
		}

		m_fInterpLoc = fInterpLoc;
		io_pInBuffer->uValidFrames -= uFramesConsumed;
		io_pOutBuffer->uValidFrames += uFramesProduced;
		// Note: Possible that a few frames are not output when upsampling and reaching output condition on very last input frame consumed
		if ( io_pInBuffer->eState == AK_NoMoreData && io_pInBuffer->uValidFrames == 0 )
			io_pOutBuffer->eState = AK_NoMoreData;
		else if ( io_pOutBuffer->uValidFrames == io_pOutBuffer->MaxFrames() )
			io_pOutBuffer->eState = AK_DataReady;
		else
			io_pOutBuffer->eState = AK_DataNeeded;
	}
	
} // namespace DSP
