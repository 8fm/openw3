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
// AkVPL.h
//
// Implementation of the Lower Audio Engine.
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_VPL_H_
#define _AK_VPL_H_

#include "AkVPLMixBusNode.h"

class AkVPL
{
public:
	AkVPL() 
		: m_fDownstreamGain( 1.f )
		, m_uDevice (0)
		, m_bReferenced( false )
		, m_bIsHDR( false )
#ifdef AK_MOTION
		, m_bFeedback( false )
#endif
	{}

	~AkVPL();

	inline AkVPL * GetParent(){ return m_MixBus.m_pParent; }
	inline void SetParent(AkVPL * in_parent){ m_MixBus.m_pParent = in_parent; }

	inline bool IsHDR() { return m_bIsHDR; }
	inline AkVPL * GetHdrBus()
	{
		if ( IsHDR() )
			return this;
		// Not this. Check parent.
		AkVPL * pParent = GetParent();
		if ( pParent )
			return pParent->GetHdrBus();
		return NULL;
	}
	
	inline bool CanDestroy()
	{ 
		bool bCanDestroy = ( m_MixBus.GetState() != NodeStatePlay 
							&& m_MixBus.ConnectionCount() == 0 
							&& !m_bReferenced );
#ifdef AK_VITA_HW
		const float AK_VITA_FRAME_ELAPSED_TIME_MS = 1000.0f * AK_NUM_VOICE_REFILL_FRAMES / 48000.0f;
		return bCanDestroy && m_MixBus.IsExpired(AK_VITA_FRAME_ELAPSED_TIME_MS);
#else
		return bCanDestroy;
#endif
	}
	
	inline void OnFrameEnd()
	{ 
#ifdef AK_VITA_HW
		m_MixBus.UpdateVolume(); 
#endif
		m_bReferenced = false;
	}

	CAkVPLMixBusNode		m_MixBus;			// Mix bus node.

	AkReal32				m_fDownstreamGain;	// Total gain including this bus's gain down to the output.
	AkOutputDeviceID		m_uDevice;

	AkUInt8					m_bReferenced	:1;	// Voices referencing aux busses during a frame set this flag so that the bus is not destroyed at the end of the frame. 
	AkUInt8					m_bIsHDR		:1;	// True when bus is HDR (in such a case it is an AkHdrBus). 
	AkUInt8					m_bExpMode		:1;	// True when using exponential mode. HDR specific setting - garbage in standard VPLs. 
#ifdef AK_MOTION
	AkUInt8					m_bFeedback		:1;	// This goes in the feedback pipeline
#endif

} AK_ALIGN_DMA;


#endif // _AK_VPL_H_

