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
// AkLEngine_SoftWarePipeline.h
//
// Implementation of the Lower Audio Engine.
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_LENGINE_SOFTWARE_PIPELINE_H_
#define _AK_LENGINE_SOFTWARE_PIPELINE_H_

#include "AkLEngine.h"

//-----------------------------------------------------------------------------
// Structs.
//-----------------------------------------------------------------------------

class CAkVPLSrcCbxNodeBase;

class AkHdrBus : public AkVPL
{
public:
	AkHdrBus( CAkBusCtx in_BusCtx );
	
	void ComputeHdrAttenuation();

	// Get computed global window top, in absolute (from the point of view of the top level node) dB SPL. 
	inline AkReal32 GetHdrWindowTop() { return m_fHdrWinTop; }

	// Voices present their effective SPL value to the HDR bus via this function. Once all the voices have done so,
	// the bus may compute the window top. The volume value is abolute (contains bus gains following this bus all the way down to the master out).
	inline void PushEffectiveVoiceVolume( AkReal32 in_fEffectiveVoiceVolume )
	{
		// Transform volume to scope of this bus.
		if ( in_fEffectiveVoiceVolume > m_fHdrMaxVoiceVolume )
			m_fHdrMaxVoiceVolume = in_fEffectiveVoiceVolume;
	}

	inline AkReal32 GetMaxVoiceWindowTop( AkReal32 in_fPeakVolume )
	{
		AkReal32 fOffset = m_fDownstreamGainDB + m_fThreshold;

		AkReal32 fPeakVolumeAboveThreshold = in_fPeakVolume - fOffset;
		AkReal32 fMaxVoiceWindowTop = fOffset;
		if ( fPeakVolumeAboveThreshold > 0 )
			fMaxVoiceWindowTop += ( m_fGainFactor * fPeakVolumeAboveThreshold );

		return fMaxVoiceWindowTop;
	}

protected:
	AkReal32				m_fHdrMaxVoiceVolume;	// Max voice volume. Target window top is based on this.
	AkReal32				m_fHdrWinTopState;		// Filtered HDR window top (linear value): used for game param only.
	AkReal32				m_fHdrWinTop;			// Actual window top level, in dB.
	AkReal32				m_fReleaseCoef;			// Release one-pole filter coefficient.
	AkReal32				m_fDownstreamGainDB;	// Downstream gain in dB.
	AkReal32				m_fThreshold;			// Cached threshold value, in dB.
	AkReal32				m_fGainFactor;			// Gain factor above threshold (derived from ratio).
};

#endif // _AK_LENGINE_SOFTWARE_PIPELINE_H_
