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
// AkLEngine.cpp
//
// Implementation of the IAkLEngine interface. Contains code that is
// common to multiple platforms.
//
//   - Software pipeline-specific code can be found in
//     SoftwarePipeline/AkLEngine_SoftwarePipeline.cpp
//   - Platform-specific code can be found in <Platform>/AkLEngine.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>

#include "AkLEngine.h"
#include "Ak3DListener.h"
#include "AkSink.h"
#include "AkFXMemAlloc.h"
#include "AkOutputMgr.h"

//-----------------------------------------------------------------------------
//LEngine memory pools.
//-----------------------------------------------------------------------------
AKSOUNDENGINE_API AkMemPoolId g_LEngineDefaultPoolId = AK_INVALID_POOL_ID;

//-----------------------------------------------------------------------------
//Lower engine global singletons.
//-----------------------------------------------------------------------------
CAkSink *				g_pAkSink		 = NULL;
AkPlatformInitSettings  g_PDSettings	 = { 0, };
extern AkInitSettings					g_settings; 

//-----------------------------------------------------------------------------
//Static variables.
//-----------------------------------------------------------------------------
AkUniqueID					CAkLEngine::m_VPLPipelineID		= 1;	// Profiling vpl src id. start at 1 since 0 means bus.
AkUInt32					CAkLEngine::m_uLastStarvationTime = 0;

#ifdef AK_MOTION
CAkFeedbackDeviceMgr*		CAkLEngine::m_pDeviceMgr = NULL;
#endif // AK_MOTION

#define LENGINE_DEFAULT_POOL_BLOCK_SIZE	(64)

AKRESULT CAkLEngine::CreateLEnginePools()
{
	// create the default pool
	if ( g_LEngineDefaultPoolId == AK_INVALID_POOL_ID )
	{
		g_LEngineDefaultPoolId = AK::MemoryMgr::CreatePool(NULL,
			g_PDSettings.uLEngineDefaultPoolSize > LENGINE_DEFAULT_POOL_BLOCK_SIZE ? g_PDSettings.uLEngineDefaultPoolSize : LENGINE_DEFAULT_POOL_SIZE,
			LENGINE_DEFAULT_POOL_BLOCK_SIZE,
			LENGINE_DEFAULT_ALLOCATION_TYPE,
			LENGINE_DEFAULT_POOL_ALIGN );

		if ( g_LEngineDefaultPoolId != AK_INVALID_POOL_ID )
		{
			AkFXMemAlloc::GetLower()->SetPoolId(g_LEngineDefaultPoolId);
			AK_SETPOOLNAME(g_LEngineDefaultPoolId,AKTEXT("Lower Engine Default"));
		}
	}

    if ( g_LEngineDefaultPoolId == AK_INVALID_POOL_ID )
	{
		return AK_InsufficientMemory;
	}

	return AK_Success;
}

void CAkLEngine::DestroyLEnginePools()
{
	AKASSERT( AK::MemoryMgr::IsInitialized() );

	if ( AK::MemoryMgr::IsInitialized() )
	{
		// destroy the default pool
		if ( g_LEngineDefaultPoolId != AK_INVALID_POOL_ID )
		{
			AKVERIFY( AK::MemoryMgr::DestroyPool( g_LEngineDefaultPoolId ) == AK_Success );
			g_LEngineDefaultPoolId = AK_INVALID_POOL_ID;
		}
	}
}

//-----------------------------------------------------------------------------
// Name: ApplyGlobalSettings
// Desc: Stores global settings in global variable g_PDSettings.
//
// Parameters: AkPlatformInitSettings * io_pPDSettings 
//-----------------------------------------------------------------------------
void CAkLEngine::ApplyGlobalSettings( AkPlatformInitSettings *   io_pPDSettings )
{
	// Settings.
	if ( io_pPDSettings == NULL )
	{
		GetDefaultPlatformInitSettings( g_PDSettings );
	}
	else
	{
		g_PDSettings = *io_pPDSettings;

#if defined( AK_MIN_NUM_REFILLS_IN_VOICE_BUFFER )
		if( g_PDSettings.uNumRefillsInVoice < AK_MIN_NUM_REFILLS_IN_VOICE_BUFFER )
			g_PDSettings.uNumRefillsInVoice = AK_DEFAULT_NUM_REFILLS_IN_VOICE_BUFFER;
#endif

		// Update client settings with actual values (might have changed due to hardware constraints).
		*io_pPDSettings = g_PDSettings;
	}
}

//-----------------------------------------------------------------------------
// Name: VPLDestroySource
// Desc: Destroy a specified source.
//
// Parameters:
//	CAkVPLSrcCbxNode * in_pCbx : Pointer to source to stop.
//-----------------------------------------------------------------------------
void CAkLEngine::VPLDestroySource( AKPBI_CBXCLASS * in_pCbx, bool in_bNotify )
{
	if (in_bNotify && in_pCbx->GetContext())
	{
		MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_CannotPlaySource_Create, in_pCbx->GetContext() );
	}

	in_pCbx->Term();

	// Free the context.
	AkDelete( g_LEngineDefaultPoolId, in_pCbx );
} // VPLDestroySource

#if defined(AK_WII_FAMILY_HW) || defined(AK_3DS)

void CAkLEngine::DestroyEnvMixBus( CAkVPLMixBusNode * in_pMixBus )
{
	AKASSERT( in_pMixBus != NULL );

	// Destroy the mixer.
	in_pMixBus->Term();

	// Destroy the mix bus.
	AkDelete( g_LEngineDefaultPoolId, in_pMixBus );
} // DestroyEnvMixBus

#endif

//-----------------------------------------------------------------------------
// Name: MergeLastAndCurrentValues
// Desc: Merge two sets of environmental values.
//-----------------------------------------------------------------------------
void MergeLastAndCurrentValues(const AkAuxSendValueEx * AK_RESTRICT in_pNewValues,
							   AkMergedEnvironmentValue * AK_RESTRICT io_paMergedValues,
							   bool in_bFirstBufferConsumed,
							   AkUInt8 & out_uNumSends
#if !defined AK_WII && !defined AK_3DS
							   ,CAkVPLSrcCbxNodeBase * in_pCbx
#endif
							   )
{
	AKASSERT( in_pNewValues );
	AKASSERT( io_paMergedValues );

	// save old values
	AkUInt32 cLastValues = 0;
	AkAuxSendValueEx arLastValues[AK_MAX_AUX_SUPPORTED];
	for ( AkUInt32 iLastValue = 0; iLastValue < out_uNumSends; ++iLastValue )
	{
		AkReal32 fControlValue = io_paMergedValues[ iLastValue ].fControlValue;
		if ( fControlValue > 0.0f )
		{
			arLastValues[ cLastValues ].auxBusID = io_paMergedValues[ iLastValue ].auxBusID;
			arLastValues[ cLastValues ].fControlValue = fControlValue;
			arLastValues[ cLastValues ].eAuxType = io_paMergedValues[ iLastValue ].eAuxType;
			arLastValues[ cLastValues ].PerDeviceAuxBusses.Transfer(io_paMergedValues[ iLastValue ].PerDeviceAuxBusses);	
	
			++cLastValues;
		}
	}
	
	// copy new values into merged
	AkUInt32 cMergedValues = 0;
	bool bMergedArray[AK_MAX_AUX_SUPPORTED];// Not initialized, but will be initialized in the loop only when required.
											// Used to support the situation where an environmental ID would be twice in the list, avoiding confusion between them.
	while( cMergedValues < AK_MAX_AUX_SUPPORTED && in_pNewValues[cMergedValues].auxBusID != AK_INVALID_AUX_ID )
	{		
		io_paMergedValues[cMergedValues].auxBusID = in_pNewValues[cMergedValues].auxBusID;
		io_paMergedValues[cMergedValues].fControlValue = in_pNewValues[cMergedValues].fControlValue;
		io_paMergedValues[cMergedValues].fLastControlValue = in_bFirstBufferConsumed ? 0.0f : in_pNewValues[cMergedValues].fControlValue;
		io_paMergedValues[cMergedValues].eAuxType = in_pNewValues[cMergedValues].eAuxType;		
		io_paMergedValues[cMergedValues].PerDeviceAuxBusses.RemoveAll();
		bMergedArray[cMergedValues] = false;
		++cMergedValues;
	}

	// merge old values with new values
	for ( AkUInt32 uIterLastValues = 0; uIterLastValues < cLastValues; ++uIterLastValues )
	{
		AkUInt32 auxBusID = arLastValues[uIterLastValues].auxBusID;

		bool bFound = false;

		for( AkUInt32 uIterMergedValues = 0; uIterMergedValues < cMergedValues; uIterMergedValues++ )
		{
			if(io_paMergedValues[uIterMergedValues].auxBusID == auxBusID && !bMergedArray[uIterMergedValues])
			{
				AkMergedEnvironmentValue * const pValue = io_paMergedValues + uIterMergedValues;
				pValue->fLastControlValue = arLastValues[uIterLastValues].fControlValue;
				pValue->PerDeviceAuxBusses.Transfer(arLastValues[uIterLastValues].PerDeviceAuxBusses);			
				bMergedArray[uIterMergedValues] = true;
				bFound = true;				
				break;
			}
		}

		// put 'dying' value into merged values, but only if we have space -- new values have priority.
		if ( !bFound && cMergedValues < AK_MAX_AUX_SUPPORTED )
		{
			AkMergedEnvironmentValue * const pValue = io_paMergedValues + cMergedValues;
			pValue->auxBusID = auxBusID;
			pValue->fControlValue = 0.0f;
			pValue->fLastControlValue = arLastValues[uIterLastValues].fControlValue;
			pValue->eAuxType = arLastValues[uIterLastValues].eAuxType;
			pValue->PerDeviceAuxBusses.Transfer(arLastValues[uIterLastValues].PerDeviceAuxBusses);
			++cMergedValues;
		}
	}

	// terminate values array
	out_uNumSends = (AkUInt8)cMergedValues;

#if !defined AK_WII && !defined AK_3DS
	//Anything that is set as false in bMergedArray is a new aux bus that wasn't there in the last frame.
	//Make sure the VPL mixing bus is created.
	for( AkUInt32 uIterMergedValues = 0; uIterMergedValues < cMergedValues; uIterMergedValues++ )
	{
 		AkMergedEnvironmentValue * const pValue = io_paMergedValues+uIterMergedValues;
		CAkLEngine::EnsureAuxBusExist(in_pCbx, pValue);
	}
#endif	
}

void CAkLEngine::SetPanningRule( 
	AkUInt32 			in_iOutputID,
	AkSinkType 			in_eDeviceType,
	AkPanningRule		in_panningRule 
	)
{
	// Set panning rule on default device.
	AkDevice * pDevice = CAkOutputMgr::GetDevice( AK_MAKE_DEVICE_KEY( in_eDeviceType, in_iOutputID ) );
	if ( pDevice )
	{
		pDevice->ePanningRule = in_panningRule;
		CAkListener::ResetListenerData();
	}
}

void CAkLEngine::GetDefaultOutputSettingsCommon( AkOutputSettings & out_settings )
{
	// For backward compatibility: by default, platforms that don't support rear speakers use 
	// "headphones" panning while others use "speaker" panning. 
#ifdef AK_REARCHANNELS 
	out_settings.ePanningRule = AkPanningRule_Speakers;
#else
	out_settings.ePanningRule = AkPanningRule_Headphones;
#endif

	out_settings.uChannelMask = AK_CHANNEL_MASK_DETECT;
}
//Panic mode support.  Called when the audio HW stops calling.  We switch to the Dummy sink.
void CAkLEngine::ReplaceMainSinkWithDummy()
{	
	//Replace the sink with the dummy sink.
	CAkSink* pSink = CAkSink::Create( g_settings.settingsMainOutput, AkSink_Dummy, 0 );
	if (pSink && CAkOutputMgr::ReplaceSink(AK_MAIN_OUTPUT_DEVICE, pSink) == AK_Success)
	{
		pSink->Play();
	}
}