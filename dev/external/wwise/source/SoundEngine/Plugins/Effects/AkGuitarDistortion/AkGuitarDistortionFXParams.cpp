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

#include "AkGuitarDistortionFXParams.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>

// Creation function
AK::IAkPluginParam * CreateGuitarDistortionFXParams( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkGuitarDistortionFXParams( ) );
}

// Constructor/destructor.
CAkGuitarDistortionFXParams::CAkGuitarDistortionFXParams( )
{
	SetDirty( true ); // Initial instance parameters must trigger computations on client side
}

CAkGuitarDistortionFXParams::~CAkGuitarDistortionFXParams( )
{
}

// Copy constructor.
CAkGuitarDistortionFXParams::CAkGuitarDistortionFXParams( const CAkGuitarDistortionFXParams & in_rCopy )
{
	m_Params = in_rCopy.m_Params;
	SetDirty( true ); // Initial instance parameters must trigger computations on client side
}

// Create duplicate.
AK::IAkPluginParam * CAkGuitarDistortionFXParams::Clone( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkGuitarDistortionFXParams( *this ) );
}

// Init/Term.
AKRESULT CAkGuitarDistortionFXParams::Init(	AK::IAkPluginMemAlloc *	in_pAllocator,									   
											const void *			in_pParamsBlock, 
											AkUInt32				in_ulBlockSize )
{
	if ( in_ulBlockSize != 0)
	{
		return SetParamsBlock( in_pParamsBlock, in_ulBlockSize );
	}
	// Wwise does serialized SetParam()
	return AK_Success;
}

AKRESULT CAkGuitarDistortionFXParams::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

// Blob set.
AKRESULT CAkGuitarDistortionFXParams::SetParamsBlock(	const void * in_pParamsBlock, 
														AkUInt32 in_ulBlockSize
												)
{  
	AKRESULT eResult = AK_Success;
	AkUInt8 * pParamsBlock = (AkUInt8 *)in_pParamsBlock;

#define READFILTERBAND( FilterBand )													\
	FilterBand.eFilterType = READBANKDATA( AkFilterType, pParamsBlock, in_ulBlockSize );\
	FilterBand.fGain = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );			\
	FilterBand.fFrequency = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );		\
	FilterBand.fQFactor = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );		\
	FilterBand.bOnOff = READBANKDATA( bool, pParamsBlock, in_ulBlockSize );				\

	// Pre-EQ params
	for ( AkUInt32 i = 0; i < NUMPREDISTORTIONEQBANDS; i++ )
	{
		READFILTERBAND( m_Params.PreEQ[i] );
	}
	// Post-EQ params
	for ( AkUInt32 i = 0; i < NUMPOSTDISTORTIONEQBANDS; i++ )
	{
		READFILTERBAND( m_Params.PostEQ[i] );
	}
	// Distortion params
	m_Params.Distortion.eDistortionType = READBANKDATA( AkDistortionType, pParamsBlock, in_ulBlockSize );
	m_Params.Distortion.fDrive = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	m_Params.Distortion.fTone = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	m_Params.Distortion.fRectification = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	// Global params	
	m_Params.fOutputLevel = AK_DBTOLIN( READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize ) );
	m_Params.fWetDryMix = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );

	CHECKBANKDATASIZE( in_ulBlockSize, eResult );
 
	SetDirty( true ); // Initial instance parameters must trigger computations on client side

	return eResult;
}

// Update one parameter.
AKRESULT CAkGuitarDistortionFXParams::SetParam(	AkPluginParamID in_ParamID,
												const void * in_pValue, 
												AkUInt32 in_ulParamSize )
{
	AKASSERT( in_pValue != NULL );
	if ( in_pValue == NULL )
	{
		return AK_InvalidParameter;
	}

// RTPC parameters are always received as type AkReal32, regardless of the type of the XML property
#define SETRTPCPARAMTYPE( __Param__, __Type__ )			\
	(__Param__) = (__Type__)( *((AkReal32*)in_pValue) );\

#define SETRTPCPARAMENUMTYPE( __Param__, __EnumType__ )					\
	(__Param__) = (__EnumType__)( (AkUInt32)*((AkReal32*)in_pValue) );	\

#define SETRTPCPARAMBOOLTYPE( __Param__ )					\
		(__Param__) = ( *((AkReal32*)in_pValue)  != 0.f );	\

	if ( in_ParamID < DISTORTIONPARAMETERSTARTID )
	{
		// EQ band parameter
		AkUInt32 uEQBandID;;
		AkUInt32 uEQParamTypeID = in_ParamID % 10;

		if ( (in_ParamID >= PREEQBANDPARAMETERIDSTART) && (in_ParamID < PREEQBANDPARAMETERIDEND) )
		{
			// Pre-EQ parameter
			uEQBandID = (in_ParamID - PREEQBANDPARAMETERIDSTART)/ 10;
			switch ( uEQParamTypeID )
			{
			case AK_GUITARDISTORTIONFXPARAM_FILTERTYPE_ID:
				SETRTPCPARAMENUMTYPE( m_Params.PreEQ[uEQBandID].eFilterType, AkFilterType );
				break;
			case AK_GUITARDISTORTIONFXPARAM_GAIN_ID:
				SETRTPCPARAMTYPE( m_Params.PreEQ[uEQBandID].fGain, AkReal32 );
				break;
			case AK_GUITARDISTORTIONFXPARAM_FREQUENCY_ID:
				SETRTPCPARAMTYPE( m_Params.PreEQ[uEQBandID].fFrequency, AkReal32 );
				break;
			case AK_GUITARDISTORTIONFXPARAM_QFACTOR_ID:
				SETRTPCPARAMTYPE( m_Params.PreEQ[uEQBandID].fQFactor, AkReal32 );
				break;
			case AK_GUITARDISTORTIONFXPARAM_ONOFF_ID:
				SETRTPCPARAMBOOLTYPE( m_Params.PreEQ[uEQBandID].bOnOff );
				break;
			default:
				goto InvalidParameter;
			}
			m_Params.PreEQ[uEQBandID].SetDirty( true );

		}
		else if ( (in_ParamID >= POSTEQBANDPARAMETERIDSTART) && (in_ParamID < POSTEQBANDPARAMETERIDEND) )
		{
			// Post-EQ parameter
			uEQBandID = (in_ParamID - POSTEQBANDPARAMETERIDSTART)/ 10;
			switch ( uEQParamTypeID )
			{
			case AK_GUITARDISTORTIONFXPARAM_FILTERTYPE_ID:
				SETRTPCPARAMENUMTYPE( m_Params.PostEQ[uEQBandID].eFilterType, AkFilterType );
				break;
			case AK_GUITARDISTORTIONFXPARAM_GAIN_ID:
				SETRTPCPARAMTYPE( m_Params.PostEQ[uEQBandID].fGain, AkReal32 );
				break;
			case AK_GUITARDISTORTIONFXPARAM_FREQUENCY_ID:
				SETRTPCPARAMTYPE( m_Params.PostEQ[uEQBandID].fFrequency, AkReal32 );
				break;
			case AK_GUITARDISTORTIONFXPARAM_QFACTOR_ID:
				SETRTPCPARAMTYPE( m_Params.PostEQ[uEQBandID].fQFactor, AkReal32 );
				break;
			case AK_GUITARDISTORTIONFXPARAM_ONOFF_ID:
				SETRTPCPARAMBOOLTYPE( m_Params.PostEQ[uEQBandID].bOnOff );
				break;
			default:
				goto InvalidParameter;
			}
			m_Params.PostEQ[uEQBandID].SetDirty( true );
		}
		else
		{
			goto InvalidParameter;
		}	
	}
	else
	{
		// Other parameters
		switch ( in_ParamID )
		{
		case AK_GUITARDISTORTIONFXPARAM_DISTORTIONTYPE_ID:
			SETRTPCPARAMENUMTYPE( m_Params.Distortion.eDistortionType, AkDistortionType );
			m_Params.Distortion.SetDirty( true );
			break;
		case AK_GUITARDISTORTIONFXPARAM_DRIVE_ID:
			SETRTPCPARAMTYPE( m_Params.Distortion.fDrive, AkReal32 );
			m_Params.Distortion.SetDirty( true );
			break;
		case AK_GUITARDISTORTIONFXPARAM_TONE_ID:
			SETRTPCPARAMTYPE( m_Params.Distortion.fTone, AkReal32 );
			m_Params.Distortion.SetDirty( true );
			break;
		case AK_GUITARDISTORTIONFXPARAM_RECTIFICATION_ID:
			SETRTPCPARAMTYPE( m_Params.Distortion.fRectification, AkReal32 );
			m_Params.Distortion.SetDirty( true );
			break;
		case AK_GUITARDISTORTIONFXPARAM_OUTPUTLEVEL_ID:
			SETRTPCPARAMTYPE( m_Params.fOutputLevel, AkReal32 );
			m_Params.fOutputLevel = AK_DBTOLIN( m_Params.fOutputLevel );
			break;
		case AK_GUITARDISTORTIONFXPARAM_WETDRYMIX_ID:
			SETRTPCPARAMTYPE( m_Params.fWetDryMix, AkReal32 );
			break;
		default:
			goto InvalidParameter;
		}
	}


	return AK_Success;

InvalidParameter:
	AKASSERT( !L"Invalid EQ parameter ID" );
	return AK_InvalidParameter;

}

// Retrieve all parameters at once
void CAkGuitarDistortionFXParams::GetParams( AkGuitarDistortionFXParams* out_pParams )
{
	*out_pParams = m_Params;
	SetDirty( false ); // Once parameter are read by the FX, the computations will be in sync with the parameters
}

// Parameters have been read by client
void CAkGuitarDistortionFXParams::SetDirty( bool in_bDirty )
{
	for ( AkUInt32 i = 0; i < NUMPREDISTORTIONEQBANDS; i++ )
		m_Params.PreEQ[i].SetDirty( in_bDirty );
	for ( AkUInt32 i = 0; i < NUMPOSTDISTORTIONEQBANDS; i++ )
		m_Params.PostEQ[i].SetDirty( in_bDirty );
	m_Params.Distortion.SetDirty( in_bDirty );
}