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

#ifndef _AK_MARKERS_H_
#define _AK_MARKERS_H_

#include "AkCommon.h"
#include "AkPBI.h"

class CAkMarkers
{
public:

    CAkMarkers();
	~CAkMarkers();
	
	AKRESULT Allocate( AkUInt32 in_uNumMarkers );
	AKRESULT SetLabel( AkUInt32 in_idx, char * in_psLabel, AkUInt32 in_uStrSize );
	void Free();
	
	inline AkUInt32 Count() { return m_hdrMarkers.uNumMarkers; }
	
	inline AkUInt32 NeedMarkerNotification( CAkPBI* in_pCtx ) // returns boolean as uint32 for performance
		{ return m_pMarkers && ( in_pCtx->GetRegisteredNotif() & AK_Marker ); }
	
	void CopyRelevantMarkers( 
		CAkPBI* in_pCtx,
		AkPipelineBuffer & io_buffer, 
		AkUInt32 in_ulBufferStartPos
		);
							
	// Sends markers to PlayingMgr that are located in region [in_uStartSample, in_uStopSample[  (PCM samples) .
	void NotifyRelevantMarkers( 
		CAkPBI * in_pCtx, 
		AkUInt32 in_uStartSample, 
		AkUInt32 in_uStopSample
		);
	
	AkForceInline void TimeSkipMarkers( 
		CAkPBI * in_pCtx, 
		AkUInt32 in_ulCurrSampleOffset, 
		AkUInt32 in_uSkippedSamples, // Region: [in_ulCurrSampleOffset, in_ulCurrSampleOffset+in_uSkippedSamples[
		AkUInt32 // File size.
		)
	{
		AkUInt32 uEffectiveFramesRequested = in_uSkippedSamples;// * GetPitchSRRatio();
		NotifyRelevantMarkers( in_pCtx, in_ulCurrSampleOffset, in_ulCurrSampleOffset + uEffectiveFramesRequested );
	}

	// Returns the closest marker to the given position (in samples).
	// Returns NULL if there is no marker.
	const AkAudioMarker * GetClosestMarker( AkUInt32 in_uPosition ) const;

	static inline AkReal32 GetRate( AkPitchValue in_pitch ) { return powf( 2.f, in_pitch / 1200.f ); }

public:
	AkMarkersHeader     m_hdrMarkers; // Markers header.
    AkAudioMarker *     m_pMarkers;   // Marker data.
};

#endif // _AK_MARKERS_H_
