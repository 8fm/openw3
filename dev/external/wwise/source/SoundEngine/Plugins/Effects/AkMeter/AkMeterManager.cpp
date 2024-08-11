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

#include "AkMeterManager.h"
#include "AkAudioLib.h"
#include "AkRegistryMgr.h"
#include "AkRTPCMgr.h"

CAkMeterManager * CAkMeterManager::pInstance = NULL;

CAkMeterManager::CAkMeterManager( AK::IAkPluginMemAlloc * in_pAllocator ) 
	: m_pAllocator( in_pAllocator )
{
	pInstance = this;
	AK::SoundEngine::AddBehavioralExtension( CAkMeterManager::BehavioralExtension );
}

CAkMeterManager::~CAkMeterManager() 
{
	AK::SoundEngine::RemoveBehavioralExtension( CAkMeterManager::BehavioralExtension );
	m_meters.Term();
	pInstance = NULL;
}


void CAkMeterManager::Execute( bool in_bLastCall )
{
	struct ValueEntry
	{
		AkUniqueID key;
		AkReal32 fValue;
	};

	ValueEntry * pValues = (ValueEntry * ) AkAlloca( m_meters.Length() * sizeof( ValueEntry ) ); 
	AkInt32 cValues = 0;

	for ( Meters::IteratorEx it = m_meters.BeginEx(); it != m_meters.End(); )
	{
		CAkMeterFX * pMeter = *it;

		AkReal32 fValue = pMeter->m_state.fLastValue;
		AkUniqueID uGameParamID = pMeter->m_uGameParamID;

		if ( pMeter->m_bTerminated )
		{
			fValue = pMeter->m_fMin; // WG-18165: force send out min value when terminated

			it = m_meters.Erase( it );
			AK_PLUGIN_DELETE( m_pAllocator, pMeter );
		}
		else
		{
			++it;
		}

		// Do not access pMeter members from this point on; object may have been deleted.

		if ( uGameParamID != AK_INVALID_UNIQUE_ID )
		{
			AkInt32 iPrevValue = 0;
			for ( ; iPrevValue < cValues; ++iPrevValue )
			{
				if ( pValues[ iPrevValue ].key == uGameParamID )
				{
					 pValues[ iPrevValue ].fValue = AK_FPMax( pValues[ iPrevValue ].fValue, fValue );
					 break;
				}
			}

			if ( iPrevValue == cValues ) // not found in pValues -- add it
			{
				 pValues[ cValues ].key = uGameParamID;
				 pValues[ cValues ].fValue = fValue;
				 ++cValues;
			}
		}
	}
	
	for ( AkInt32 iValue = 0; iValue < cValues; ++iValue )
	{
		// We're in audio thread, so we can skip the message queue
		TransParams transParams;
		transParams.TransitionTime = 0;
		transParams.eFadeCurve = AkCurveInterpolation_Linear;
		g_pRTPCMgr->SetRTPCInternal( pValues[ iValue ].key, pValues[ iValue ].fValue, NULL, transParams );
#ifdef WWISE_AUTHORING
		CAkRegisteredObj * pTransportGameObj = g_pRegistryMgr->GetObjAndAddref( 0 ); // GameObjects::GO_Transport
		if ( pTransportGameObj )
		{
			g_pRTPCMgr->SetRTPCInternal( pValues[ iValue ].key, pValues[ iValue ].fValue, pTransportGameObj, transParams );
			pTransportGameObj->Release();
		}
#endif
	}

	if ( m_meters.IsEmpty() )
	{
		AkDelete( g_LEngineDefaultPoolId, this );
	}
}

CAkMeterManager * CAkMeterManager::Instance( AK::IAkPluginMemAlloc * in_pAllocator )
{
	if ( !pInstance )
	{
		CAkMeterManager * pMeterManager;
		AkNew2( pMeterManager, g_LEngineDefaultPoolId, CAkMeterManager, CAkMeterManager( in_pAllocator ) );
		if ( !pMeterManager )
			return NULL;
	}

	return pInstance;
}
