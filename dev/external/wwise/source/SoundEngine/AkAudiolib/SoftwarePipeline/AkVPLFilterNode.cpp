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

#include "AkVPLFilterNode.h"
#include "AudiolibDefs.h"
#include "AkEnvironmentsMgr.h"
#include "AkMonitor.h"
#include "AkFXMemAlloc.h"
#include "AkAudioLibTimer.h"
#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <AK/SoundEngine/Common/AkSimd.h>

#ifdef AK_PS3
#include "AkLEngine.h"
#endif

void CAkVPLFilterNode::GetBuffer( AkVPLState & io_state )
{
	if( m_bLast )
	{
		io_state.result = AK_NoMoreData;
		ConsumeBuffer( io_state );
	}

	// else just continue up the chain to fetch input buffer
}

void CAkVPLFilterNode::ConsumeBuffer( AkVPLState & io_state )
{
	// Bypass FX if necessary
	if( m_bBypassed || m_pCtx->GetBypassAllFX() )
	{
		if ( !m_LastBypassed )	// Reset FX if not bypassed last buffer
			m_pEffect->Reset( );

		m_LastBypassed = true;

		return;
	}
	else
	{
		m_LastBypassed = false;
	}

	if ( io_state.result == AK_NoMoreData )
		m_bLast = true;
        
	if( !io_state.HasData() )
	{
		AKASSERT( io_state.MaxFrames() > 0 );
		AKASSERT( m_pAllocatedBuffer == NULL );

		m_pAllocatedBuffer = (AkUInt8*)AkMalign( g_LEngineDefaultPoolId, io_state.MaxFrames()*sizeof(AkReal32)*io_state.NumChannels(), AK_BUFFER_ALIGNMENT );
		if ( m_pAllocatedBuffer )
		{
			io_state.AttachInterleavedData( m_pAllocatedBuffer, (AkUInt16) io_state.MaxFrames(), 0, io_state.GetChannelMask() );
		}
		else
		{
			io_state.result = AK_Fail;
			return;
		}
	}

	io_state.eState = io_state.result;
	AKASSERT( io_state.MaxFrames() % 4 == 0 ); // Allocate size for vectorization
	
#ifdef AK_PS3
	AK::MultiCoreServices::DspProcess* pDspProcess = NULL;
	m_pEffect->Execute( (AkAudioBuffer*) &io_state, pDspProcess );
	if ( pDspProcess )
	{
		io_state.result = AK_ProcessNeeded;
		CAkLEngine::QueueDspProcess(pDspProcess);
	}
	else
	{
		io_state.result = io_state.eState;
	}

#else
	AK_START_PLUGIN_TIMER( m_FXID );
 	m_pEffect->Execute( &io_state );
	AK_STOP_PLUGIN_TIMER( m_FXID );
	io_state.result = io_state.eState;
	AKASSERT( io_state.uValidFrames <= io_state.MaxFrames() );	// Produce <= than requested
#endif
	
	AKSIMD_ASSERTFLUSHZEROMODE;
}

#ifdef AK_PS3
void CAkVPLFilterNode::ProcessDone( AkVPLState & io_state )
{
	AKASSERT( io_state.uValidFrames <= io_state.MaxFrames() );	// Produce <= than requested
	io_state.result = io_state.eState;
}
#endif

//-----------------------------------------------------------------------------
// Name: ReleaseBuffer
// Desc: Releases a processed buffer.
//-----------------------------------------------------------------------------
void CAkVPLFilterNode::ReleaseBuffer()
{
	// Assume output buffer was entirely consumed by client.
	if( m_pAllocatedBuffer )
	{
		AkFalign( g_LEngineDefaultPoolId, m_pAllocatedBuffer );
		m_pAllocatedBuffer = NULL;
	}
	else if ( m_pInput )
	{
		m_pInput->ReleaseBuffer();
	}
} // ReleaseBuffer

AKRESULT CAkVPLFilterNode::Seek()
{
	m_pEffect->Reset();

	// Seek anywhere: reset m_bLast!
	m_bLast = false;

	return m_pInput->Seek();
}

void CAkVPLFilterNode::VirtualOn( AkVirtualQueueBehavior eBehavior )
{
	if ( eBehavior != AkVirtualQueueBehavior_Resume )
		m_pEffect->Reset();

	CAkVPLFilterNodeBase::VirtualOn( eBehavior );
}

AKRESULT CAkVPLFilterNode::Init( 
		IAkPlugin * in_pPlugin,
		const AkFXDesc & in_fxDesc,
		AkUInt32 in_uFXIndex,
		CAkPBI * in_pCtx,
		AkAudioFormat &	in_format )
{
	m_uChannelMask		= in_format.uChannelMask;
	m_pEffect			= (IAkInPlaceEffectPlugin *) in_pPlugin;
	m_pAllocatedBuffer	= NULL;

	AKRESULT eResult = CAkVPLFilterNodeBase::Init( 
		in_pPlugin,
		in_fxDesc,
		in_uFXIndex,
		in_pCtx,
		in_format );
	if ( eResult == AK_Success )
	{
		eResult = m_pEffect->Init(
			AkFXMemAlloc::GetLower(),
			m_pInsertFXContext,	
			m_pParam,
			in_format
			);

		if ( eResult == AK_Success )
		{
			eResult = m_pEffect->Reset( );
		}
		else
		{
			switch ( eResult )
			{ 
			case AK_UnsupportedChannelConfig:
				MONITOR_PLUGIN_ERROR( AK::Monitor::ErrorCode_PluginUnsupportedChannelConfiguration, in_pCtx, m_FXID );	
				break;
			case AK_PluginMediaNotAvailable:
				MONITOR_PLUGIN_ERROR( AK::Monitor::ErrorCode_PluginMediaUnavailable, in_pCtx, m_FXID );	
				break;
			default:
				MONITOR_PLUGIN_ERROR( AK::Monitor::ErrorCode_PluginInitialisationFailed, in_pCtx, m_FXID );	
				break;
			}
		}
	}
	return eResult;
} // Init

void CAkVPLFilterNode::Term()
{
	ReleaseMemory();
	CAkVPLFilterNodeBase::Term();
} // Term

void CAkVPLFilterNode::ReleaseMemory()
{
	if( m_pEffect != NULL )
	{
		m_pEffect->Term( AkFXMemAlloc::GetLower() );
		AK_DECREMENT_PLUGIN_COUNT( m_FXID );
		m_pEffect = NULL;
	}
	if( m_pAllocatedBuffer )
	{
		AkFree( g_LEngineDefaultPoolId, m_pAllocatedBuffer );
		m_pAllocatedBuffer = NULL;
	}
}
AKRESULT CAkVPLFilterNode::TimeSkip( AkUInt32 & io_uFrames )
{
	if ( m_bLast )
		return AK_NoMoreData;

	//No need to check the result.  The non-time based effects cannot alter the flow of time so 
	//whether they return AK_DataReady or Ak_NoMoreData is irrelevant.  Any error occurring is not relevant
	//to the rest of the pipeline either.
	if (m_pEffect)
		m_pEffect->TimeSkip(io_uFrames);

	return m_pInput->TimeSkip( io_uFrames );
}

AkChannelMask CAkVPLFilterNode::GetOutputChannelMask()
{
	return m_uChannelMask;
}
