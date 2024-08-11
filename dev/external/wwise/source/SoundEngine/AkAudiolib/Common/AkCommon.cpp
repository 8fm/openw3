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
// AkCommon.cpp
//
// Implementation of public AkAudioBuffer structure.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkCommon.h"
#include "AudiolibDefs.h"
#include <AK/Tools/Common/AkObject.h>

// AkPipelineBuffer data setters.
// ---------------------------------------

void AkPipelineBuffer::FreeMarkers()
{
	if ( pMarkers )
	{
		AkFree( AK_MARKERS_POOL_ID, pMarkers );
	}
	ResetMarkerPointer();
}

#if !defined (AK_WII_FAMILY_HW) && !defined(AK_3DS)

// Deinterleaved data. 
// ---------------------------------------

// This implementation could be platform specific.
#include "AkLEngine.h"

AKRESULT AkPipelineBufferBase::GetCachedBuffer( 
	AkUInt16		in_uMaxFrames, 
	AkChannelMask	in_uChannelMask )	
{
	// Note. The current implementation uses one contiguous buffer.
	AkUInt32 uNumChannels = GetNumChannels( in_uChannelMask );
	AKASSERT( uNumChannels || !"Channel mask must be set before allocating audio buffer" );
	AkUInt32 uAllocSize = in_uMaxFrames * sizeof(AkReal32) * uNumChannels;
	void * pBuffer = CAkLEngine::GetCachedAudioBuffer( uAllocSize );
	if ( pBuffer )
	{
		pData = pBuffer;
		uMaxFrames = in_uMaxFrames;
		uChannelMask = in_uChannelMask;
		uValidFrames = 0;
		return AK_Success;
	}
	return AK_InsufficientMemory;
}
void AkPipelineBufferBase::ReleaseCachedBuffer()
{
	AKASSERT( pData && uMaxFrames > 0 && NumChannels() > 0 );
	AkUInt32 uAllocSize = uMaxFrames * sizeof(AkReal32) * NumChannels();
	CAkLEngine::ReleaseCachedAudioBuffer( uAllocSize, pData );
	pData = NULL;
	uMaxFrames = 0;
}

AkDeviceInfo::AkDeviceInfo( AkVPL * in_pOutputBus, AkOutputDeviceID in_uDeviceID, bool in_bCrossDeviceSend ) 
	: fLPF( 0 )
	, fObsLPF( 0 )
	, pMixBus( in_pOutputBus )
	, uDeviceID( in_uDeviceID )
	, fMaxVolume( 0.f )
	, bCrossDeviceSend(in_bCrossDeviceSend)
{
	ZeroAll();
#ifndef AK_VITA_HW
	AKASSERT( in_pOutputBus );
#else
	if ( in_pOutputBus )
#endif
	{
		pMixBus->m_MixBus.Connect();
	}
}

AkDeviceInfo::~AkDeviceInfo()
{
	if ( pMixBus )
		pMixBus->m_MixBus.Disconnect();
}

AkChannelMask AkDeviceInfo::GetOutputConfig()
{
#ifndef AK_VITA_HW
	return pMixBus->m_MixBus.GetChannelMask();
#else
	return AK_SPEAKER_SETUP_STEREO;
#endif
}

#else

// This implementation could be platform specific.
#include "AkLEngine.h"

AkDeviceInfo::AkDeviceInfo( AkVPL * in_pOutputBus, AkOutputDeviceID in_uDeviceID, bool in_bCrossDeviceSend ) 
	: fLPF( 0 )
	, fObsLPF( 0 )
	, pMixBus( in_pOutputBus )
	, uDeviceID( in_uDeviceID )
	, bCrossDeviceSend( in_bCrossDeviceSend )
{
	ZeroAll();
}

AkDeviceInfo::~AkDeviceInfo()
{
}

AkChannelMask AkDeviceInfo::GetOutputConfig()
{
	return CAkLEngine::GetChannelMask();
}

#endif // !defined (AK_WII_FAMILY) && !defined(AK_3DS)
