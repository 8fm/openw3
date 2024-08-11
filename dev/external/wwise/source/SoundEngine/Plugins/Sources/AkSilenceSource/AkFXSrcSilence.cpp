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

#include "AkFXSrcSilence.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>

#if defined (AK_WII_FAMILY) || defined (AK_APPLE)
	#include <string.h>
#elif defined (AK_VITA) || defined(AK_3DS)
#endif

#include "../../../AkAudiolib/Common/AkRandom.h"

using namespace AK;

// Plugin mechanism. Implement Create function and register its address to the FX manager.
IAkPluginParam * CreateSilenceSourceParams( IAkPluginMemAlloc * in_pAllocator )
{
    return AK_PLUGIN_NEW( in_pAllocator, CAkFxSrcSilenceParams( ) );
}

// Constructor.
CAkFxSrcSilenceParams::CAkFxSrcSilenceParams()
{
}

// Copy constructor.
CAkFxSrcSilenceParams::CAkFxSrcSilenceParams( const CAkFxSrcSilenceParams &Copy )
{
    m_Params = Copy.m_Params;
}

// Destructor.
CAkFxSrcSilenceParams::~CAkFxSrcSilenceParams()
{
}

// Create shared parameters duplicate.
IAkPluginParam * CAkFxSrcSilenceParams::Clone( IAkPluginMemAlloc * in_pAllocator )
{
    return AK_PLUGIN_NEW( in_pAllocator, CAkFxSrcSilenceParams( *this ) );
}

// Shared parameters initialization.
AKRESULT CAkFxSrcSilenceParams::Init( IAkPluginMemAlloc *,									  
	                                  const void *			in_pvParamsBlock, 
	                                  AkUInt32				in_ulBlockSize 
                                 )
{
    if ( in_ulBlockSize == 0)
    {
        // Init with default if we got invalid parameter block.
        m_Params.fDuration = 1.f;				// Duration (secs).     
        m_Params.fRandomizedLengthMinus = 0.f;  // Maximum randomness to subtract to duration (secs)
		m_Params.fRandomizedLengthPlus = 0.f;   // Maximum randomness to add to duration (secs) 

        return AK_Success;
    }
	 
    return SetParamsBlock( in_pvParamsBlock, in_ulBlockSize );
}

// Shared parameters termination.
AKRESULT CAkFxSrcSilenceParams::Term( IAkPluginMemAlloc * in_pAllocator )
{
    AK_PLUGIN_DELETE( in_pAllocator, this );
    return AK_Success;
}

// Set all shared parameters at once.
AKRESULT CAkFxSrcSilenceParams::SetParamsBlock( const void * in_pvParamsBlock, 
                                                AkUInt32 in_ulBlockSize
                                              )
{
	AKRESULT eResult = AK_Success;
	AkUInt8 * pParamsBlock = (AkUInt8 *)in_pvParamsBlock;

	m_Params.fDuration = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	m_Params.fRandomizedLengthMinus = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	m_Params.fRandomizedLengthPlus = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );

	CHECKBANKDATASIZE( in_ulBlockSize, eResult );
	return eResult;
}

// Update one parameter.
AKRESULT CAkFxSrcSilenceParams::SetParam( AkPluginParamID in_ParamID,
                                          const void * in_pvValue, 
                                          AkUInt32
                                        )
{
    if ( in_pvValue == NULL )
	{
		return AK_InvalidParameter;
	}

	// Pointer should be aligned on 4 bytes
	AKASSERT(((AkIntPtr)in_pvValue & 3) == 0);

    // Set parameter value.
    switch ( in_ParamID )
    {
	case AK_SRCSILENCE_FXPARAM_DUR_ID:
		m_Params.fDuration = *reinterpret_cast<const AkReal32*>(in_pvValue);
		break;
	case AK_SRCSILENCE_FXPARAM_RANDMINUS_ID:
		m_Params.fRandomizedLengthMinus = *reinterpret_cast<const AkReal32*>(in_pvValue);
		break;
	case AK_SRCSILENCE_FXPARAM_RANDPLUS_ID:
		m_Params.fRandomizedLengthPlus = *reinterpret_cast<const AkReal32*>(in_pvValue);
		break;
	default:
		return AK_InvalidParameter;
    }

    return AK_Success;
}
 
//-----------------------------------------------------------------------------
// Name: CreateEffect
// Desc: Plugin mechanism. Dynamic create function whose address must be 
//       registered to the FX manager.
//-----------------------------------------------------------------------------
IAkPlugin* CreateSilenceSource( IAkPluginMemAlloc * in_pAllocator )
{
    return AK_PLUGIN_NEW( in_pAllocator, CAkFXSrcSilence( ) );
}

//-----------------------------------------------------------------------------
// Name: CAkFXSrcSilence
// Desc: Constructor.
//-----------------------------------------------------------------------------
CAkFXSrcSilence::CAkFXSrcSilence()
{
	// Initialize members.
	m_uSampleRate = 0;
	m_uBytesPerSample = 0;
	m_sNumLoops = 1;
	m_sCurLoopCount = 0;
	m_fDurationModifier = 0.f;
	m_fInitDuration = 0.f;
	m_pSourceFXContext = NULL;
	m_pSharedParams = NULL;
}

//-----------------------------------------------------------------------------
// Name: ~CAkFXSrcSilence
// Desc: Destructor.
//-----------------------------------------------------------------------------
CAkFXSrcSilence::~CAkFXSrcSilence()
{

}

//-----------------------------------------------------------------------------
// Name: Init
// Desc: Init.
//-----------------------------------------------------------------------------
AKRESULT CAkFXSrcSilence::Init( IAkPluginMemAlloc *,		// Memory allocator interface.
								IAkSourcePluginContext *	in_pSourceFXContext,// Source FX context
								IAkPluginParam *			in_pParams,			// Effect parameters.
								AkAudioFormat &				io_rFormat			// Supported audio output format.
								)
{
	// Keep source FX context (looping etc.)
	m_pSourceFXContext = in_pSourceFXContext;

	// Save audio format internally
	m_uSampleRate = io_rFormat.uSampleRate;
	m_uBytesPerSample = io_rFormat.GetBitsPerSample() / 8;

	// Looping info.
	m_sNumLoops = m_pSourceFXContext->GetNumLoops( );
	AKASSERT( m_sNumLoops >= 0 );

    // Set parameters access.
    AKASSERT( NULL != in_pParams );
    m_pSharedParams = static_cast<CAkFxSrcSilenceParams*>(in_pParams);

	// Compute modifier (offset value)
	AkReal32 fRandomPlusMax = m_pSharedParams->GetRandomPlus( );
	AkReal32 fRandomMinusMax = m_pSharedParams->GetRandomMinus( );
	AKASSERT(fRandomPlusMax >= 0.f);
	AKASSERT(fRandomMinusMax <= 0.f);
	m_fDurationModifier = RandRange(fRandomMinusMax,fRandomPlusMax);

	m_fInitDuration = m_pSharedParams->GetDuration( ) + m_fDurationModifier;
	if ( m_fInitDuration < SILENCE_DURATION_MIN )
	{
		m_fInitDuration = SILENCE_DURATION_MIN;
	}

	AK_PERF_RECORDING_RESET();

    return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Term.
// Desc: Term. The effect must destroy itself herein
//-----------------------------------------------------------------------------
AKRESULT CAkFXSrcSilence::Term( IAkPluginMemAlloc * in_pAllocator )
{
    AK_PLUGIN_DELETE( in_pAllocator, this );
    return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Reset
// Desc: Reset or seek to start (looping).
//-----------------------------------------------------------------------------
AKRESULT CAkFXSrcSilence::Reset( void )
{
    m_ulOutByteCount = 0;
	m_sCurLoopCount = 0;
    return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: GetEffectType
// Desc: Effect type query.
//-----------------------------------------------------------------------------
// Info query:
// Effect type (source, monadic, mixer, ...)
// Buffer type: in-place, input(?), output (source), io.
AKRESULT CAkFXSrcSilence::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
{
    out_rPluginInfo.eType = AkPluginTypeSource;
	out_rPluginInfo.bIsInPlace = true;
	out_rPluginInfo.bIsAsynchronous = false;
    return AK_Success;
}

/// Skip processing of some frames when voice is virtual from elapsed time.
AKRESULT CAkFXSrcSilence::TimeSkip( AkUInt32 &io_uFrames )
{
	AkUInt32 uBufferSize = io_uFrames * m_uBytesPerSample;

	AkUInt32 ulProcessSize = 0;	
	AKRESULT eState = AK_DataReady;

	// Check if infinite
	if ( m_sNumLoops == 0 )
	{
		// Silence duration does not matter
		ulProcessSize = uBufferSize;
	}
	// Otherwise is finite
	else
	{
		// Apply random modifier
		AkReal32 fDuration = m_pSharedParams->GetDuration( ) + m_fDurationModifier;
		if ( fDuration < SILENCE_DURATION_MIN )
			fDuration = SILENCE_DURATION_MIN;

		// Multiplying this way ensure proper number of blocks output
		AkUInt32 ulRandomDur = static_cast<AkUInt32>(fDuration * m_uSampleRate) * m_sNumLoops * m_uBytesPerSample;
		// ulRandomDur may have changed and we may have already output to much data
		if ( m_ulOutByteCount >= ulRandomDur )
		{
			// Output no more and stop.
			eState = AK_NoMoreData;
		}
		else
		{
			// Compute size to be processed
			ulProcessSize = AkMin( uBufferSize, ulRandomDur - m_ulOutByteCount );

			// Update production counter
			m_ulOutByteCount += ulProcessSize;

			// Check for the end of this iteration.
			if ( ulProcessSize < uBufferSize )
				eState = AK_NoMoreData;	// This is the last buffer
		}
	}

	// Notify buffers of updated production
	io_uFrames = (AkUInt16)( ulProcessSize / m_uBytesPerSample );
	return eState;
}

//-----------------------------------------------------------------------------
// Name: Execute
// Desc: Effect processing.
//-----------------------------------------------------------------------------
void CAkFXSrcSilence::Execute(	AkAudioBuffer *							io_pBufferOut		// Output buffer interface.
#ifdef AK_PS3
								, AK::MultiCoreServices::DspProcess*&	out_pDspProcess 	// the process that needs to run
#endif
								)
{
	AkUInt32 uFrameAdvance = io_pBufferOut->MaxFrames();
	io_pBufferOut->eState = TimeSkip( uFrameAdvance ); 
	io_pBufferOut->uValidFrames = (AkUInt16)uFrameAdvance;

	AK_PERF_RECORDING_START( "Silence", 25, 30 );

	// Write silence
	memset( io_pBufferOut->GetChannel(0), 0, uFrameAdvance*m_uBytesPerSample );

	AK_PERF_RECORDING_STOP( "Silence", 25, 30 );
}


//-----------------------------------------------------------------------------
// Name: GetDuration()
// Desc: Get the duration of the source.
//
// Return: AkReal32 : duration of the source.
//
//-----------------------------------------------------------------------------
AkReal32 CAkFXSrcSilence::GetDuration( void ) const
{
	return ( m_fInitDuration * 1000.f ) * m_sNumLoops;
}

AKRESULT CAkFXSrcSilence::StopLooping()
{
	// Just make sure we finish within the timing of maximum one more loop.
	m_sNumLoops = 1;
	return AK_Success;
}

AKRESULT CAkFXSrcSilence::Seek( AkUInt32 in_uPosition )
{
	if ( m_sNumLoops != 0 )
	{
		// Apply random modifier
		AkReal32 fDuration = m_pSharedParams->GetDuration( ) + m_fDurationModifier;
		if ( fDuration < SILENCE_DURATION_MIN )
			fDuration = SILENCE_DURATION_MIN;
		
		// Multiplying this way ensure proper number of blocks output
		AkUInt32 ulRandomDur = static_cast<AkUInt32>(fDuration * m_uSampleRate) 
			* m_sNumLoops * m_uBytesPerSample;

		AkUInt32 uDesiredByteCount = m_uBytesPerSample * in_uPosition;

		// ulRandomDur may have changed and we may have already output to much data
		if ( uDesiredByteCount >= ulRandomDur )
			return AK_Fail;

		m_ulOutByteCount = uDesiredByteCount;
	}
	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: RandRange()
// Desc: RandRange returns a random float value between in_fMin and in_fMax
//-----------------------------------------------------------------------------
AkReal32 CAkFXSrcSilence::RandRange( AkReal32 in_fMin, AkReal32 in_fMax )
{
	// Get an integer in range (0,1.)
	AkReal32 fRandVal = AKRANDOM::AkRandom() / static_cast<AkReal32>(AKRANDOM::AK_RANDOM_MAX);
	return ( fRandVal * (in_fMax - in_fMin) + in_fMin );
}
