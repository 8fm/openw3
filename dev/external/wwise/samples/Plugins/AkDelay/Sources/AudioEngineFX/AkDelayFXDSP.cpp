//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkDelayFXDSP.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/SoundEngine/Common/AkSimd.h>
#include <AK/Plugin/PluginServices/AkVectorValueRamp.h>
#include <math.h>

#ifndef __SPU__

CAkDelayFXDSP::CAkDelayFXDSP( ) 
: m_uNumProcessedChannels(0)
{
	
}

CAkDelayFXDSP::~CAkDelayFXDSP( )
{

}

void CAkDelayFXDSP::Setup( AkDelayFXParams * pInitialParams, bool in_bIsSendMode, AkUInt32 in_uSampleRate )
{	
	m_bSendMode = in_bIsSendMode;
	m_PreviousParams = *pInitialParams;
	m_uSampleRate = in_uSampleRate;

	// Algorithm only looks at feedback gain, this allows interpolation of enable/disable feedback parameter
	if ( !m_PreviousParams.RTPC.bFeedbackEnabled )
		m_PreviousParams.RTPC.fFeedback = 0.f;
	// When effect is sent, set the dry level to -96 dBFS and wet level to 0 dBFS regardless of parameter values
	if ( m_bSendMode )
		m_PreviousParams.RTPC.fWetDryMix = 1.f;
}

// Delay-line initialization or re-initialization
AKRESULT CAkDelayFXDSP::InitDelay( AK::IAkPluginMemAlloc * in_pAllocator, AkDelayFXParams * pParams, AkChannelMask in_uChannelMask )
{
	m_DelayMem.Term( in_pAllocator );

	m_uNumProcessedChannels = AK::GetNumChannels( in_uChannelMask );
	m_bProcessLFE = pParams->NonRTPC.bProcessLFE;
	if ( AK::HasLFE( in_uChannelMask ) && !m_bProcessLFE )
	{
		--m_uNumProcessedChannels;
	}

	if (m_uNumProcessedChannels == 0)
		return AK_Fail;

	return m_DelayMem.Init( in_pAllocator, (AkUInt32)( pParams->NonRTPC.fDelayTime * m_uSampleRate ), m_uNumProcessedChannels );
}

void CAkDelayFXDSP::ResetDelay( )
{
	m_DelayMem.Reset();
}

void CAkDelayFXDSP::TermDelay( AK::IAkPluginMemAlloc * in_pAllocator )
{
	m_DelayMem.Term( in_pAllocator );
}

void CAkDelayFXDSP::ComputeTailLength( bool in_bFeedbackEnabled, AkReal32 in_fFeedbackValue )
{
	static const AkReal32 DYNAMICRANGE = -60.f;			// Level (in dB) at the end of the effect tail (effect shut off after this)
	static const AkReal32 MAXFEEDBACKGAIN = -0.6f;		// Feedback higher than -0.6 dB will result in MAXTAILITERATIONS
	static const AkUInt32 MAXTAILITERATIONS = 100;		// Maximum number of delay iterations when feedback is used. 
	// The effect will be shut off after MAXTAILITERATIONS * delay time

	// Feedback not enabled equivalent to feedback of 0 for the algorithm
	AkReal32 fFeedbackGain = in_bFeedbackEnabled ? in_fFeedbackValue : 0.f;
	if ( fFeedbackGain != 0.f )
	{
		AkReal32 fIterationGain = 20.f * log10f( in_fFeedbackValue );
		// Figure out how many iteration are necessary in order to fall under threshold
		AkReal32 fNumIterations;
		if ( fIterationGain < MAXFEEDBACKGAIN )
			fNumIterations = DYNAMICRANGE / fIterationGain;
		else
			fNumIterations = MAXTAILITERATIONS;
		m_uTailLength = (AkUInt32)( fNumIterations * m_DelayMem.GetDelayLength() );
	}
	else
	{
		m_uTailLength = m_DelayMem.GetDelayLength();
	}
}

#endif // #ifndef __SPU__

//PS3 processing is done in AkDelayFXDSPPS3.cpp
//WiiU processing is done in AkDelayFXDSPWiiU.cpp
#if !defined AK_PS3 && !defined AK_WIIU
// DSP computations for feedback delay effect
void CAkDelayFXDSP::Process( 
							AkAudioBuffer * io_pBuffer, 
							AkDelayFXParams * pCurrentParams 
							)
{
	m_FXTailHandler.HandleTail( io_pBuffer, m_uTailLength );
	const AkUInt32 uNumFrames = io_pBuffer->uValidFrames;

	// Algorithm only looks at feedback gain, this allows interpolation of enable/disable feedback parameter
	if ( !pCurrentParams->RTPC.bFeedbackEnabled )
		pCurrentParams->RTPC.fFeedback = 0.f;
	// When effect is sent, set the dry level to -96 dBFS and wet level to 0 dBFS regardless of parameter values
	if ( m_bSendMode )
		pCurrentParams->RTPC.fWetDryMix = 1.f;

	AkUInt32 uDelayPos = 0;
	for ( AkUInt32 i = 0; i < m_uNumProcessedChannels; ++i )
	{
		AkReal32 * AK_RESTRICT pfChan = (AkReal32 * AK_RESTRICT) io_pBuffer->GetChannel(i);

		CAkVectorValueRamp vGainRamp, vFeedbackRamp, vWetDryRamp;
		AKSIMD_V4F32 vGain = vGainRamp.Setup(m_PreviousParams.RTPC.fOutputLevel,pCurrentParams->RTPC.fOutputLevel,uNumFrames);	
		AKSIMD_V4F32 vFeedbackGain = vFeedbackRamp.Setup(m_PreviousParams.RTPC.fFeedback,pCurrentParams->RTPC.fFeedback,uNumFrames);
		AKSIMD_V4F32 vWetGain = vWetDryRamp.Setup(m_PreviousParams.RTPC.fWetDryMix,pCurrentParams->RTPC.fWetDryMix,uNumFrames);
		const AkReal32 fOne = 1.f;
		const AKSIMD_V4F32 vOne = AKSIMD_LOAD1_V4F32( fOne);
		AKSIMD_V4F32 vDryGain = AKSIMD_SUB_V4F32(vOne,vWetGain);
		uDelayPos = m_DelayMem.GetCurrentOffset();
		const AkUInt32 uDelayLength = m_DelayMem.GetDelayLength();

		// Process delay line with feedback
		AkUInt32 uFramesProduced = 0;
		while ( uFramesProduced < uNumFrames )
		{
			AkUInt32 uFramesThisLoop = AkMin( uDelayLength - uDelayPos, uNumFrames-uFramesProduced );
			AKASSERT( (uFramesThisLoop % 4) == 0 );
			AkReal32 * AK_RESTRICT pfDelayRW = (AkReal32 * AK_RESTRICT) m_DelayMem.GetCurrentPointer( uDelayPos, i );
			AkUInt32 uVectorIterations = uFramesThisLoop/4;
			while ( uVectorIterations-- )
			{
				// Compute channel delay
				AKSIMD_V4F32 vIn = AKSIMD_LOAD_V4F32( pfChan );				
				AKSIMD_V4F32 vDelay = AKSIMD_LOAD_V4F32( pfDelayRW );		
				AKSIMD_V4F32 vOut = AKSIMD_MUL_V4F32( vGain, AKSIMD_MADD_V4F32(vDryGain,vIn,AKSIMD_MUL_V4F32(vWetGain,vDelay)) );
				AKSIMD_STORE_V4F32( pfChan, vOut );
				AKSIMD_V4F32 vFeedback = AKSIMD_MADD_V4F32(vDelay,vFeedbackGain,vIn);
				AKSIMD_STORE_V4F32( pfDelayRW, vFeedback );
				pfChan += 4;
				pfDelayRW += 4;	
				// Interpolate ramp values
				vGain = vGainRamp.Tick();
				vFeedbackGain = vFeedbackRamp.Tick();
				vWetGain = vWetDryRamp.Tick();
				vDryGain = AKSIMD_SUB_V4F32(vOne,vWetGain);
			}

			uFramesProduced += uFramesThisLoop;
			uDelayPos += uFramesThisLoop;
			// Wrap delay line
			if ( uDelayPos == uDelayLength )
				uDelayPos = 0;
		}
	}
	m_DelayMem.SetCurrentOffset( uDelayPos ); // Internal state can be advanced once all channels are processed

	// Zero out the LFE channel in send effect configuration if it is not doubled, 
	// to avoid doubling of in-phase unprocessed LFE channel with the one from the dry path.
	// Note: LFE channel is always the last channel
	AkReal32 * pLFEChan = io_pBuffer->GetLFE( );
	if ( pLFEChan != NULL && m_bSendMode && !pCurrentParams->NonRTPC.bProcessLFE )
	{
		AkZeroMemLarge( pLFEChan, uNumFrames*sizeof(AkReal32) );
	}

	// Parameter ramps are reached
	m_PreviousParams = *pCurrentParams;
}
#endif


