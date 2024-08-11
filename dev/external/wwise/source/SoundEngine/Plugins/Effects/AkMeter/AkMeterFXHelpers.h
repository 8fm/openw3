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

#pragma once

#include "AkMeterFXParams.h"
#include "AkMath.h"
#include "AkPrivateTypes.h"
#include <limits.h>

static AkReal32 AkMeterGetValue(
	AkAudioBuffer * in_pBuffer,
	AkMeterFXParams * in_pParams,
	AkReal32 * in_pfInChannel )
{
	AkUInt32 uNumChannels = in_pBuffer->NumChannels();
	AkReal32 fValue = 0;

	if ( in_pBuffer->uValidFrames )
	{
		if ( in_pParams->NonRTPC.eMode == AkMeterMode_Peak )
		{
			AkReal32 fMin = (AkReal32) INT_MAX;
			AkReal32 fMax = (AkReal32) INT_MIN;

			for ( unsigned int uChan = 0; uChan < uNumChannels; ++uChan )
			{
				register AkReal32 * AK_RESTRICT pfBuf = in_pBuffer->GetChannel(uChan);	
				register AkReal32 * AK_RESTRICT pfEnd = pfBuf + in_pBuffer->uValidFrames;

				AkReal32 fChannelMin = (AkReal32) INT_MAX;
				AkReal32 fChannelMax = (AkReal32) INT_MIN;

				while ( pfBuf < pfEnd )
				{
					fChannelMin = AK_FPMin( fChannelMin, *pfBuf );
					fChannelMax = AK_FPMax( fChannelMax, *pfBuf );

					++pfBuf;
				}

				if ( in_pfInChannel )
					in_pfInChannel[ uChan ] = 20.f*log10( AK_FPMax( fabs( fChannelMin ), fChannelMax ) );

				fMin = AK_FPMin( fMin, fChannelMin );
				fMax = AK_FPMax( fMax, fChannelMax );
			}

			fValue = AK_FPMax( fabs( fMin ), fMax );
		}
		else // RMS
		{
			AkReal32 fMax = (AkReal32) INT_MIN;
			for ( unsigned int uChan = 0; uChan < uNumChannels; ++uChan )
			{
				register AkReal32 * AK_RESTRICT pfBuf = in_pBuffer->GetChannel(uChan);	
				register AkReal32 * AK_RESTRICT pfEnd = pfBuf + in_pBuffer->uValidFrames;

				AkReal32 fSum = 0.0f;
				while ( pfBuf < pfEnd )
				{
					fSum += *pfBuf * *pfBuf;
					++pfBuf;
				}

				fSum /= in_pBuffer->uValidFrames;

				if ( in_pfInChannel )
				{
					in_pfInChannel[ uChan ] = 10.f*log10( fSum );
				}

				fMax = AK_FPMax( fMax, fSum );
			}

			fValue = sqrtf( fMax );
		}
	}
	else if ( in_pfInChannel )
	{
		for ( unsigned int uChan = 0; uChan < uNumChannels; ++uChan )
			in_pfInChannel[ uChan ] = AK_MINIMUM_VOLUME_DBFS;
	}

	fValue = 20.f*log10( fValue ); // convert to dB
	fValue = AK_FPMax( in_pParams->RTPC.fMin, fValue );
	fValue = AK_FPMin( in_pParams->RTPC.fMax, fValue );

	return fValue;
}

static void AkMeterApplyBallistics(
	AkReal32 in_fValue,
	AkUInt32 in_uElapsed, // in audio frames
	AkMeterFXParams * in_pParams,
	AkMeterState * in_pState )
{
	if ( in_fValue > in_pState->fLastValue )
	{
		in_pState->fHoldTime = 0.0f;
		for ( int i = 0; i < HOLD_MEMORY_SIZE; ++i )
			in_pState->fHoldMemory[ i ] = in_pParams->RTPC.fMin;

		if ( in_pParams->RTPC.fAttack == 0.0f )
			in_pState->fLastValue = in_fValue;
		else
		{
			AkReal32 fTimeDelta = (AkReal32) in_uElapsed / in_pState->uSampleRate;
			AkReal32 fMaxRamp = ( fTimeDelta / in_pParams->RTPC.fAttack ) * 10.0f;
			in_pState->fLastValue = AK_FPMin( in_fValue, in_pState->fLastValue + fMaxRamp );
		}

		in_pState->fReleaseTarget = in_pState->fLastValue;
	}
	else
	{
		AkReal32 fTimeDelta = (AkReal32) in_uElapsed / in_pState->uSampleRate;
		in_pState->fHoldTime += fTimeDelta;

		if ( in_pState->fHoldTime >= in_pParams->RTPC.fHold )
		{
			int iMaxMemorySlot = HOLD_MEMORY_SIZE;
			AkReal32 fMemoryValue = in_fValue;
			for ( int i = 0; i < HOLD_MEMORY_SIZE; ++i )
			{
				if ( in_pState->fHoldMemory[ i ] >= fMemoryValue )
				{
					fMemoryValue = in_pState->fHoldMemory[ i ];
					iMaxMemorySlot = i;
				}
			}

			in_pState->fHoldTime = in_pParams->RTPC.fHold / ( HOLD_MEMORY_SIZE + 1 ) * (HOLD_MEMORY_SIZE - iMaxMemorySlot );

			int j = 0;
			for ( int i = iMaxMemorySlot + 1; i < HOLD_MEMORY_SIZE ; ++i )
				in_pState->fHoldMemory[ j++ ] = in_pState->fHoldMemory[ i ];
			for ( ; j < HOLD_MEMORY_SIZE; ++j )
				in_pState->fHoldMemory[ j ] = in_pParams->RTPC.fMin;

			in_pState->fReleaseTarget = fMemoryValue;
		}
		else
		{
			int iHoldSlot = (int) ( in_pState->fHoldTime / in_pParams->RTPC.fHold * HOLD_MEMORY_SIZE + 0.5 ) - 1;
			if ( iHoldSlot >= 0 )
			{
				if ( in_fValue > in_pState->fHoldMemory[ iHoldSlot ] )
					in_pState->fHoldMemory[ iHoldSlot ] = in_fValue;
			}

			if ( in_fValue > in_pState->fReleaseTarget )
				in_pState->fReleaseTarget = in_fValue;
		}

		if ( in_pParams->RTPC.fRelease == 0.0f )
			in_pState->fLastValue = in_pState->fReleaseTarget;
		else
		{
			AkReal32 fMaxRamp = ( fTimeDelta / in_pParams->RTPC.fRelease ) * 10.0f;
			in_pState->fLastValue = AK_FPMax( in_pState->fReleaseTarget, in_pState->fLastValue - fMaxRamp );
		}
	}
}