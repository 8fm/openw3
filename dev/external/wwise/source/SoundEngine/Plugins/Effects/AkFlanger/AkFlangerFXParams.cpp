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

#include "AkFlangerFXParams.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>

// Creation function
AK::IAkPluginParam * CreateFlangerFXParams( AK::IAkPluginMemAlloc * in_pAllocator )
{
    return AK_PLUGIN_NEW( in_pAllocator, CAkFlangerFXParams( ) );
}

// Constructor/destructor.
CAkFlangerFXParams::CAkFlangerFXParams( )
{
}

CAkFlangerFXParams::~CAkFlangerFXParams( )
{
}

// Copy constructor.
CAkFlangerFXParams::CAkFlangerFXParams( const CAkFlangerFXParams & in_rCopy )
{
    m_Params = in_rCopy.m_Params;
	SetDirty(true);
}

void CAkFlangerFXParams::SetDirty( bool in_bDirty )
{
	m_Params.NonRTPC.SetDirty(in_bDirty);
	m_Params.RTPC.SetDirty(in_bDirty);
}

// Create duplicate.
AK::IAkPluginParam * CAkFlangerFXParams::Clone( AK::IAkPluginMemAlloc * in_pAllocator )
{
    return AK_PLUGIN_NEW( in_pAllocator, CAkFlangerFXParams( *this ) );
}

// Init/Term.
AKRESULT CAkFlangerFXParams::Init(	AK::IAkPluginMemAlloc *	in_pAllocator,									   
									const void *			in_pParamsBlock, 
									AkUInt32				in_ulBlockSize 
                                     )
{
    if ( in_ulBlockSize == 0)
    {
        // Init default parameters.
		m_Params.NonRTPC.fDelayTime		= AK_FLANGERFXPARAM_DELAYTIME_DEF;

		m_Params.RTPC.fDryLevel			= AK_FLANGERFXPARAM_DRYLEVEL_DEF;
		m_Params.RTPC.fFfwdLevel		= AK_FLANGERFXPARAM_FFWDLEVEL_DEF;
		m_Params.RTPC.fFbackLevel		= AK_FLANGERFXPARAM_FBACKLEVEL_DEF;	
			
		m_Params.RTPC.modParams.lfoParams.eWaveform		= AK_FLANGERFXPARAM_MODWAVEFORM_DEF;
		m_Params.RTPC.modParams.lfoParams.fFrequency	= AK_FLANGERFXPARAM_MODFREQUENCY_DEF;
		
		
		m_Params.RTPC.modParams.phaseParams.fPhaseOffset	= AK_FLANGERFXPARAM_MODPHASEOFFSET_DEF;
		m_Params.RTPC.modParams.phaseParams.ePhaseMode		= AK_FLANGERFXPARAM_MODPHASEMODE_DEF;
		m_Params.RTPC.modParams.phaseParams.fPhaseSpread	= AK_FLANGERFXPARAM_MODPHASESPREAD_DEF;

		m_Params.RTPC.fModDepth		= AK_FLANGERFXPARAM_MODDEPTH_DEF;
		m_Params.NonRTPC.bEnableLFO	= AK_FLANGERFXPARAM_ENABLELFO_DEF;

		m_Params.RTPC.fWetDryMix	= AK_FLANGERFXPARAM_WETDRYMIX_DEF;
		m_Params.RTPC.fOutputLevel	= AK_FLANGERFXPARAM_OUTPUTLEVEL_DEF;
		m_Params.NonRTPC.bProcessCenter	= AK_FLANGERFXPARAM_PROCESSCENTER_DEF;
		m_Params.NonRTPC.bProcessLFE	= AK_FLANGERFXPARAM_PROCESSLFE_DEF;

		SetDirty(true);
		
        return AK_Success;
    }
    return SetParamsBlock( in_pParamsBlock, in_ulBlockSize );
}

AKRESULT CAkFlangerFXParams::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
    AK_PLUGIN_DELETE( in_pAllocator, this );
    return AK_Success;
}

// Blob set.
AKRESULT CAkFlangerFXParams::SetParamsBlock(	const void * in_pParamsBlock, 
                                                AkUInt32 in_ulBlockSize
                                               )
{
	AKRESULT eResult = AK_Success;
	AkUInt8 * pParamsBlock = (AkUInt8 *)in_pParamsBlock;

	m_Params.NonRTPC.fDelayTime = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	

	m_Params.RTPC.fDryLevel = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	m_Params.RTPC.fFfwdLevel = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	m_Params.RTPC.fFbackLevel = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	
	m_Params.RTPC.fModDepth = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	m_Params.RTPC.modParams.lfoParams.fFrequency = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	m_Params.RTPC.modParams.lfoParams.eWaveform = READBANKDATA( DSP::LFO::Waveform , pParamsBlock, in_ulBlockSize );	
	m_Params.RTPC.modParams.lfoParams.fSmooth = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	m_Params.RTPC.modParams.lfoParams.fPWM = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	m_Params.RTPC.modParams.phaseParams.fPhaseOffset = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	m_Params.RTPC.modParams.phaseParams.ePhaseMode = READBANKDATA( DSP::LFO::MultiChannel::PhaseMode, pParamsBlock, in_ulBlockSize );	
	m_Params.RTPC.modParams.phaseParams.fPhaseSpread = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	

	// Normalize modulation parameters
	m_Params.RTPC.fModDepth *= 0.01f;
	m_Params.RTPC.modParams.lfoParams.fSmooth *= 0.01f;
	m_Params.RTPC.modParams.lfoParams.fPWM *= 0.01f;

	m_Params.RTPC.fOutputLevel = AK_DBTOLIN( READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize ) );
	m_Params.RTPC.fWetDryMix = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );	
	m_Params.NonRTPC.bEnableLFO = READBANKDATA( bool, pParamsBlock, in_ulBlockSize );	
	m_Params.NonRTPC.bProcessCenter = READBANKDATA( bool, pParamsBlock, in_ulBlockSize );	
	m_Params.NonRTPC.bProcessLFE = READBANKDATA( bool, pParamsBlock, in_ulBlockSize );	

	CHECKBANKDATASIZE( in_ulBlockSize, eResult );

	SetDirty(true);

    return eResult;
}

// Update one parameter.
AKRESULT CAkFlangerFXParams::SetParam(	AkPluginParamID in_ParamID,
										const void * in_pValue, 
										AkUInt32 in_ulParamSize
                                         )
{
	AKASSERT( in_pValue != NULL );
	if ( in_pValue == NULL )
	{
		return AK_InvalidParameter;
	}

	switch ( in_ParamID )
	{
		// RTPC params
		case AK_FLANGERFXPARAM_DRYLEVEL_ID:
			m_Params.RTPC.fDryLevel = *((AkReal32*)in_pValue);
			m_Params.RTPC.SetDirty( true );
			break;
		case AK_FLANGERFXPARAM_FFWDLEVEL_ID:
			m_Params.RTPC.fFfwdLevel = *((AkReal32*)in_pValue);
			m_Params.RTPC.SetDirty( true );
			break;
		case AK_FLANGERFXPARAM_FBACKLEVEL_ID:	
			m_Params.RTPC.fFbackLevel = *((AkReal32*)in_pValue);
			m_Params.RTPC.SetDirty( true );
			break;
		case AK_FLANGERFXPARAM_MODDEPTH_ID:
			m_Params.RTPC.fModDepth = *((AkReal32*)in_pValue) * 0.01f;	// Normalize
			m_Params.RTPC.SetDirty( true );
			break;
		case AK_FLANGERFXPARAM_MODFREQUENCY_ID:
			m_Params.RTPC.modParams.lfoParams.fFrequency = *((AkReal32*)in_pValue);
			m_Params.RTPC.SetDirty( true );
			break;
		case AK_FLANGERFXPARAM_MODWAVEFORM_ID:
			m_Params.RTPC.modParams.lfoParams.eWaveform = (DSP::LFO::Waveform)(AkUInt32)*((AkReal32*)in_pValue);
			m_Params.RTPC.SetDirty( true );
			break;
		case AK_FLANGERFXPARAM_MODSMOOTHING_ID:
			m_Params.RTPC.modParams.lfoParams.fSmooth = *((AkReal32*)in_pValue) * 0.01f;	// Normalize
			m_Params.RTPC.SetDirty( true );
			break;
		case AK_FLANGERFXPARAM_MODPWM_ID:
			m_Params.RTPC.modParams.lfoParams.fPWM = *((AkReal32*)in_pValue) * 0.01f;	// Normalize
			m_Params.RTPC.SetDirty( true );
			break;
		case AK_FLANGERFXPARAM_MODPHASEOFFSET_ID:
			m_Params.RTPC.modParams.phaseParams.fPhaseOffset = *((AkReal32*)in_pValue);
			break;
		case AK_FLANGERFXPARAM_MODPHASEMODE_ID:
			m_Params.RTPC.modParams.phaseParams.ePhaseMode = (DSP::LFO::MultiChannel::PhaseMode)(AkUInt32)*((AkReal32*)in_pValue);
			break;
		case AK_FLANGERFXPARAM_MODPHASESPREAD_ID:
			m_Params.RTPC.modParams.phaseParams.fPhaseSpread = *((AkReal32*)in_pValue);
			break;
		case AK_FLANGERFXPARAM_OUTPUTLEVEL_ID:
			m_Params.RTPC.fOutputLevel = AK_DBTOLIN( *((AkReal32*)in_pValue) );
			m_Params.RTPC.SetDirty( true );
			break;	
		case AK_FLANGERFXPARAM_WETDRYMIX_ID:
			m_Params.RTPC.fWetDryMix = *((AkReal32*)in_pValue);
			m_Params.RTPC.SetDirty( true );
			break;
		// Non-RTPC params
		case AK_FLANGERFXPARAM_ENABLELFO_ID:
			m_Params.NonRTPC.bEnableLFO =  *((AkUInt8*)in_pValue) != 0;
			m_Params.NonRTPC.SetDirty( true );
			break;
		case AK_FLANGERFXPARAM_PROCESSCENTER_ID:
			m_Params.NonRTPC.bProcessCenter =  *((AkUInt8*)in_pValue) != 0;
			m_Params.NonRTPC.SetDirty( true );
			break;
		case AK_FLANGERFXPARAM_PROCESSLFE_ID:
			m_Params.NonRTPC.bProcessLFE =  *((AkUInt8*)in_pValue) != 0;
			m_Params.NonRTPC.SetDirty( true );
			break;
		case AK_FLANGERFXPARAM_DELAYTIME_ID:
			m_Params.NonRTPC.fDelayTime = *((AkReal32*)in_pValue);
			m_Params.NonRTPC.SetDirty( true );
			break;	
	}
	return AK_Success;

}
