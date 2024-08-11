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

#include "AkExpanderFXParams.h"
#include <math.h>
#include <AK/Tools/Common/AkBankReadHelpers.h>

// Creation function
AK::IAkPluginParam * CreateExpanderFXParams( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AKASSERT( in_pAllocator != NULL );
	return AK_PLUGIN_NEW( in_pAllocator, CAkExpanderFXParams( ) );
}

// Constructor/destructor.
CAkExpanderFXParams::CAkExpanderFXParams( )
{
}

CAkExpanderFXParams::~CAkExpanderFXParams( )
{
}

// Copy constructor.
CAkExpanderFXParams::CAkExpanderFXParams( const CAkExpanderFXParams & in_rCopy )
{
	m_Params = in_rCopy.m_Params;
}

// Create duplicate.
AK::IAkPluginParam * CAkExpanderFXParams::Clone( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AKASSERT( in_pAllocator != NULL );
	return AK_PLUGIN_NEW( in_pAllocator, CAkExpanderFXParams( *this ) );
}

// Init.
AKRESULT CAkExpanderFXParams::Init(	AK::IAkPluginMemAlloc *	in_pAllocator,									   
										const void *		in_pParamsBlock, 
										AkUInt32			in_ulBlockSize )
{
	if ( in_ulBlockSize == 0)
	{
		// Init default parameters.
		m_Params.fThreshold = AK_EXPANDER_THRESHOLD_DEF;
		m_Params.fRatio = AK_EXPANDER_RATIO_DEF;
		m_Params.fAttack = AK_EXPANDER_ATTACK_DEF;
		m_Params.fRelease = AK_EXPANDER_RELEASE_DEF;
		m_Params.fOutputLevel = AK_EXPANDER_GAIN_DEF;
		m_Params.bProcessLFE = AK_EXPANDER_PROCESSLFE_DEF;
		m_Params.bChannelLink = AK_EXPANDER_CHANNELLINK_DEF;

		return AK_Success;
	}
	return SetParamsBlock( in_pParamsBlock, in_ulBlockSize );
}

// Term.
AKRESULT CAkExpanderFXParams::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AKASSERT( in_pAllocator != NULL );
	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

// Blob set.
AKRESULT CAkExpanderFXParams::SetParamsBlock(	const void * in_pParamsBlock, 
												AkUInt32 in_ulBlockSize
												)
{  
	AKRESULT eResult = AK_Success;
	AkUInt8 * pParamsBlock = (AkUInt8 *)in_pParamsBlock;
	m_Params.fThreshold = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	m_Params.fRatio = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	m_Params.fAttack = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	m_Params.fRelease = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	m_Params.fOutputLevel  = AK_DBTOLIN( READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize ) );
	m_Params.bProcessLFE = READBANKDATA( bool, pParamsBlock, in_ulBlockSize );
	m_Params.bChannelLink = READBANKDATA( bool, pParamsBlock, in_ulBlockSize );
	CHECKBANKDATASIZE( in_ulBlockSize, eResult );
    return eResult;
}

// Update one parameter.
AKRESULT CAkExpanderFXParams::SetParam(	AkPluginParamID in_ParamID,
											const void * in_pValue, 
											AkUInt32 in_ulParamSize )
{
	AKASSERT( in_pValue != NULL );
	if ( in_pValue == NULL )
	{
		return AK_InvalidParameter;
	}
	AKRESULT eResult = AK_Success;

	switch ( in_ParamID )
	{
	case AK_EXPANDERFXPARAM_THRESHOLD_ID:
		m_Params.fThreshold = *reinterpret_cast<const AkReal32*>(in_pValue);
		break;
	case AK_EXPANDERFXPARAM_RATIO_ID:
		m_Params.fRatio = *reinterpret_cast<const AkReal32*>(in_pValue);
		break;
	case AK_EXPANDERFXPARAM_ATTACK_ID:
		m_Params.fAttack = *reinterpret_cast<const AkReal32*>(in_pValue);
		break;
	case AK_EXPANDERFXPARAM_RELEASE_ID:
		m_Params.fRelease = *reinterpret_cast<const AkReal32*>(in_pValue);
		break;
	case AK_EXPANDERFXPARAM_GAIN_ID:
		{
			AkReal32 fDbVal = *reinterpret_cast<const AkReal32*>(in_pValue);
			m_Params.fOutputLevel  = powf( 10.f, fDbVal * 0.05f );
		}
		break;
	case AK_EXPANDERFXPARAM_PROCESSLFE_ID:
		m_Params.bProcessLFE = *reinterpret_cast<const bool*>(in_pValue);
		break;
	case AK_EXPANDERFXPARAM_CHANNELLINK_ID:
		m_Params.bChannelLink = *reinterpret_cast<const bool*>(in_pValue);
		break;
	default:
		AKASSERT(!"Invalid parameter.");
		eResult = AK_InvalidParameter;
	}

	return eResult;
}