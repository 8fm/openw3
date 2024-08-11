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
// Implementation of the IAkLEngine interface. Win32 version.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkLEngine.h"

#include "Ak3DListener.h"
#include "AkAudioLibTimer.h"
#include "AkSink.h"
#include "AkProfile.h"
#include "AkSpeakerPan.h"
#include "AkEffectsMgr.h"
#include "xmmintrin.h"

#include "AkPositionRepository.h"
#ifdef AK_MOTION
#include "AkFeedbackMgr.h"
#endif

#include "AkSrcBankAt9.h"
#include "AkSrcFileAt9.h"
#include "AkOutputMgr.h"

extern CAkLock g_csMain;

//-----------------------------------------------------------------------------
//Static variables.
//-----------------------------------------------------------------------------
AkEvent						CAkLEngine::m_EventStop;
extern						AkPlatformInitSettings g_PDSettings;
CAkACPManager				CAkLEngine::m_ACPManager;
void CAkLEngine::GetDefaultPlatformInitSettings( 
	AkPlatformInitSettings &      out_pPlatformSettings      // Platform specific settings. Can be changed depending on hardware.
	)
{
	memset( &out_pPlatformSettings, 0, sizeof( AkPlatformInitSettings ) );
	out_pPlatformSettings.threadLEngine.nPriority = AK_THREAD_PRIORITY_ABOVE_NORMAL;
	out_pPlatformSettings.threadLEngine.dwAffinityMask = AK_THREAD_AFFINITY_DEFAULT;
	out_pPlatformSettings.threadLEngine.uStackSize = AK_DEFAULT_STACK_SIZE;
	out_pPlatformSettings.threadLEngine.uSchedPolicy = AK_THREAD_DEFAULT_SCHED_POLICY;
	out_pPlatformSettings.threadBankManager.nPriority = AK_THREAD_BANK_MANAGER_PRIORITY;
	out_pPlatformSettings.threadBankManager.dwAffinityMask = AK_THREAD_AFFINITY_DEFAULT;
	out_pPlatformSettings.threadBankManager.uStackSize = AK_BANK_MGR_THREAD_STACK_BYTES;
	out_pPlatformSettings.threadBankManager.uSchedPolicy = AK_THREAD_DEFAULT_SCHED_POLICY;
	out_pPlatformSettings.uLEngineDefaultPoolSize = LENGINE_DEFAULT_POOL_SIZE;
	out_pPlatformSettings.uLEngineAcpBatchBufferSize = LENGINE_ACP_BATCH_BUFFER_SIZE;
	out_pPlatformSettings.fLEngineDefaultPoolRatioThreshold = 1.0f; // 1.0f == means 100% == disabled by default
	out_pPlatformSettings.uNumRefillsInVoice = AK_DEFAULT_NUM_REFILLS_IN_VOICE_BUFFER;
	//out_pPlatformSettings.uSampleRate = 48000; //DEFAULT_NATIVE_FREQUENCY;
	out_pPlatformSettings.threadMonitor.nPriority = AK_THREAD_PRIORITY_ABOVE_NORMAL;
	out_pPlatformSettings.threadMonitor.dwAffinityMask = AK_THREAD_AFFINITY_DEFAULT;
	out_pPlatformSettings.threadMonitor.uStackSize = AK_DEFAULT_STACK_SIZE;
	out_pPlatformSettings.threadMonitor.uSchedPolicy = AK_THREAD_DEFAULT_SCHED_POLICY;
}

//-----------------------------------------------------------------------------
// Name: Init
// Desc: Initialise the object.
//
// Parameters:
//
// Return: 
//	Ak_Success:          Object was initialised correctly.
//  AK_InvalidParameter: Invalid parameters.
//  AK_Fail:             Failed to initialise the object correctly.
//-----------------------------------------------------------------------------
AKRESULT CAkLEngine::Init()
{
	AK::SoundEngine::RegisterCodec(
		AKCOMPANYID_AUDIOKINETIC,
		AKCODECID_ATRAC9,
		CreateATRAC9FilePlugin,
		CreateATRAC9BankPlugin );

	AKRESULT eResult = SoftwareInit();
	if( eResult != AK_Fail )
	{
		eResult = m_ACPManager.Init();
	}
	return eResult;
} // Init

//-----------------------------------------------------------------------------
// Name: Term
// Desc: Terminate the object.
//
// Parameters:
//	None.
//
// Return:
//	Ak_Success: Object was terminated correctly.
//  AK_Fail:    Failed to terminate correctly.
//-----------------------------------------------------------------------------
void CAkLEngine::Term()
{
	m_ACPManager.Term();
	SoftwareTerm();
} // Term

AkUInt32 CAkLEngine::GetNumBufferNeededAndSubmit()
{
	AkUInt32 uBuffersNeeded = 0;

	AKRESULT eResult = g_pAkSink->IsDataNeeded( uBuffersNeeded );
	if ( eResult != AK_Success )
	{
		AKASSERT( false );
	}

	//nothing to submit, simply return num buffers
	return uBuffersNeeded;
}

//-----------------------------------------------------------------------------
// Name: Perform
// Desc: Perform all VPLs.
//-----------------------------------------------------------------------------
void CAkLEngine::Perform()
{
	AKAutoRazorMarker marker( "CAkLEngine::Perform()" );

	AkUInt32 uFlushZeroMode = _MM_GET_FLUSH_ZERO_MODE();
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);

	m_ACPManager.Update();

	SoftwarePerform();

	_MM_SET_FLUSH_ZERO_MODE(uFlushZeroMode);
} // Perform

void CAkLEngine::StartVoice()
{
//	return g_pAkSink->Play();
}

void CAkLEngine::GetDefaultOutputSettings( AkSinkType in_eSinkType, AkOutputSettings & out_settings )
{
	//Default to 5.1
	GetDefaultOutputSettingsCommon(out_settings);
}

bool CAkLEngine::GetSinkTypeText( AkSinkType in_sinkType, AkUInt32 in_uBufSize, char* out_pszBuf )
{
	// NOTE: if this function is changed, be sure to update GetNumSinkTypes and GetMaxSinkTypeTextLen !!
	if ( in_uBufSize > 10 )
	{
		switch ( in_sinkType )
		{
		case AkSink_Main:
			strcpy( out_pszBuf, "Main" );
			return true;
		case AkSink_Voice:
			strcpy( out_pszBuf, "Chat" );
			return true;
		case AkSink_Personal:
			strcpy( out_pszBuf, "Headset" );
			return true;
		case AkSink_PAD:
			strcpy( out_pszBuf, "Controller" );
			return true;
		case AkSink_Dummy:
			strcpy( out_pszBuf, "" );
			return true;
		default:
			strcpy( out_pszBuf, "" );
		}
	}
	else if ( in_uBufSize != 0 )
	{
		strcpy( out_pszBuf, "" );
	}
	return false;
}
