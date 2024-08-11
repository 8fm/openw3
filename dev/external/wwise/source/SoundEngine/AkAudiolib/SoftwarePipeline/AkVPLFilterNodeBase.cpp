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

/////////////////////////////////////////////////////////////////////
//
// AkVPLFilterNode.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "AkVPLFilterNodeBase.h"
#include "AkFxBase.h"
#include "AkLEngine.h"
#include "AudiolibDefs.h"
#include "AkEffectsMgr.h"
#include "AkMonitor.h"
#include "AkFXMemAlloc.h"
#include "AkAudioLibTimer.h"
#include <AK/SoundEngine/Common/IAkPlugin.h>
#include "AkRTPCMgr.h"

void CAkVPLFilterNodeBase::VirtualOn( AkVirtualQueueBehavior eBehavior )
{
	if( eBehavior == AkVirtualQueueBehavior_FromBeginning )
	{
		// WG-5935 
		// If the last buffer was drained and we restart from beginning, the flag must be resetted.
		m_bLast = false;
	}	
	
    if ( !m_bLast )
    {
		m_pInput->VirtualOn( eBehavior );
	}
}

AKRESULT CAkVPLFilterNodeBase::VirtualOff( AkVirtualQueueBehavior eBehavior, bool in_bUseSourceOffset )
{
	if ( !m_bLast )
	    return m_pInput->VirtualOff( eBehavior, in_bUseSourceOffset );

	return AK_Success;
}

AKRESULT CAkVPLFilterNodeBase::Init( 
		IAkPlugin * in_pPlugin,
		const AkFXDesc & in_fxDesc,
		AkUInt32 in_uFXIndex,
		CAkPBI * in_pCtx,
		AkAudioFormat & )
{
	AKASSERT( in_pPlugin != NULL );
	AKASSERT( in_pCtx != NULL );

	m_pCtx					= in_pCtx;
	m_pInsertFXContext		= NULL;
	m_bLast					= false;
	m_bBypassed				= false;
	m_LastBypassed			= false;
	m_uFXIndex				= in_uFXIndex;

	m_pParam = in_fxDesc.pFx->GetFXParam()->Clone( AkFXMemAlloc::GetLower() );
	if ( !m_pParam )
		return AK_Fail;

	m_FXID = in_fxDesc.pFx->GetFXID(); // Cached copy of fx id for profiling.

	AK_INCREMENT_PLUGIN_COUNT( m_FXID );
	m_pInsertFXContext = AkNew( g_LEngineDefaultPoolId, CAkInsertFXContext( in_pCtx, in_uFXIndex ) );
	if ( m_pInsertFXContext != NULL )
	{
		in_fxDesc.pFx->SubscribeRTPC( m_pParam, in_pCtx->GetGameObjectPtr() );
	}
	else
	{
		MONITOR_PLUGIN_ERROR( AK::Monitor::ErrorCode_PluginAllocationFailed, in_pCtx, m_FXID );	
		return AK_Fail;
	}

	return AK_Success;
} // Init

void CAkVPLFilterNodeBase::Term()
{
	if ( m_pParam )
	{
		g_pRTPCMgr->UnSubscribeRTPC( m_pParam );

		m_pParam->Term( AkFXMemAlloc::GetLower() );
		m_pParam = NULL;
	}

	if ( m_pInsertFXContext )
	{
		AkDelete( g_LEngineDefaultPoolId, m_pInsertFXContext );
		m_pInsertFXContext = NULL;
	}
} // Term
