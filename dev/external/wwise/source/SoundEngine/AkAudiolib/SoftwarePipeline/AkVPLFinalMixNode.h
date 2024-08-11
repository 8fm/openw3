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
// AkVPLFinalMixNode.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_VPL_FINAL_MIX_NODE_H_
#define _AK_VPL_FINAL_MIX_NODE_H_

#include "AkSrcBase.h"

#include "AkMixer.h"
#include "Ak3DParams.h"
#include "AkVPLNode.h"
#include "AkVPLMixBusNode.h"

// Effect
#include <AK/SoundEngine/Common/IAkPlugin.h>
#include "AkFXContext.h"
#include <AK/SoundEngine/Common/IAkRTPCSubscriber.h>
#include "AkBusCtx.h"

class CAkVPLFinalMixNode : public CAkBusFX
{
public:

	AKRESULT			Init( AkUInt32 in_uChannelMask );
	AKRESULT			Term();
	void				Stop() { m_eState = NodeStateStop; }
	AKRESULT			ReleaseBuffer();
	void				Connect( CAkVPLMixBusNode * in_pInput );
#ifdef AK_VITA_HW
	void				ConnectMasterBusFx();
#endif

	//New execution model
	AKRESULT ConsumeBuffer( 
			AkAudioBufferBus* 		io_rpBuffer,
			bool					in_bPan,
			AkAudioMix				in_PanMix[]
			);

	void			GetResultingBuffer(
		AkPipelineBufferBase& io_rBuffer
#ifdef AK_PS4
		, bool in_bForce71
#endif
		);

	AKRESULT			SetAllInsertFx();
	
	VPLNodeState		GetState(){ return m_eState; }

	void ResetNextVolume( AkReal32 in_dBVolume )
	{
		SetNextVolume( in_dBVolume );
		Update2DParams( NULL );
	}

private:
	VPLNodeState		m_eState;
#ifdef AK_VITA_HW
	bool				m_bInitFx;
#endif
};

#endif // _AK_VPL_FINAL_MIX_NODE_H_
