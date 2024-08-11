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
// AkSrcBase.h
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_SRC_BASE_H_
#define _AK_SRC_BASE_H_

#include "AkCommon.h"
#include <AK/Tools/Common/AkObject.h>
#include "AkPBI.h"
#include "AkVPLSrcNode.h"
#include "AkMarkers.h"

class CAkSrcBaseEx : public CAkVPLSrcNode
{
public:

    CAkSrcBaseEx( CAkPBI * in_pCtx );
	virtual ~CAkSrcBaseEx();

	// Base implementation.
	// 

	// StopStream: Free markers.
	virtual void StopStream();

	// StopLooping: Set loop count to 1.
	virtual AKRESULT StopLooping();

	// Sets current sample and updates loop count.
	virtual AKRESULT TimeSkip( AkUInt32 & io_uFrames );

	virtual AkReal32 GetDuration() const;	// Returns duration of file in ms, taking looping into account.

	virtual AKRESULT Seek();

	// Returns estimate of relative loudness at current position, compared to the loudest point of the sound, in dBs (always negative).
	// in_uBufferedFrames is the number of samples that are currently buffered in the pitch node (used for interpolation).
	// Sources hold the position of the next buffer to be acquired, so they substract in_uBufferedFrames
	// from it to get a position that is more accurate than simply "the next block".
	virtual AkReal32 GetAnalyzedEnvelope( AkUInt32 in_uBufferedFrames );
	
	inline AkUInt16 GetLoopCnt() const { return m_uLoopCnt; }
	inline AkUInt32 GetPCMLoopStart() const { return m_uPCMLoopStart; }
	inline AkUInt32 GetPCMLoopEnd() const { return m_uPCMLoopEnd; }
	
	// Used on VitaHW. Only poll the playback position from low level NGS when necessary.
	inline bool NeedPositionUpdates() { return ( m_markers.NeedMarkerNotification( m_pCtx  ) || (m_pCtx->GetRegisteredNotif() & AK_EnableGetSourcePlayPosition) ); }

protected:
//#ifdef AK_VITA_HW
	AKRESULT HardwareVoiceUpdate( AkUInt32 in_NextSample, AkReal32 in_fPitch );
//#endif
	// Handler invoked when looping or sound completes while updating source (SubmitBufferAndUpdate()).
	// Base implementation: source-side loop count is updated.
	// Return code is passed to the pipeline. Return either AK_Fail, AK_DataReady or AK_NoMoreData.
	// Note: When implementing layers that need to loop before the source (e.g. streaming, codecs with 
	// complex input management, ...), implement your own derived handler and call the base class.
	virtual AKRESULT OnLoopComplete(
		bool in_bEndOfFile		// True if this was the end of file, false otherwise.
		) 
	{
		// Note: LOOPING_INFINITE == 0, so we can do this.
		if ( m_uLoopCnt > 1 ) // && m_uLoopCnt != LOOPING_INFINITE
			--m_uLoopCnt;
		return ( in_bEndOfFile ) ? AK_NoMoreData : AK_DataReady;
	}

	// Services.
	//

	// Prepare output buffer and update source. Derived implementation should call this at the end of GetBuffer(),
	// after having produced data for the pipeline.
	// - Attaches a buffer to the pipeline's io_state.
	// - Posts markers and position information.
	// - Updates the output (PCM) sample position. Call this after having produced output data
	// in GetBuffer(). 
	// - Handles source-side looping, invokes OnLoopComplete() loop if applicable.
	// - Sets the status to pass to the pipeline: AK_NoDataReady, AK_DataReady, AK_NoMoreData, AK_Fail.
	// IMPORTANT: Do not call this if there had been an error. If in_uNumSamplesProduced == 0, AK_NoDataReady
	// is returned.
	void SubmitBufferAndUpdate( 
		void * in_pData, 
		AkUInt16 in_uNumSamplesProduced, 
		AkUInt32 in_uSampleRate, 
		AkChannelMask in_channelMask, 
		AkVPLState & io_state
		);

	// Sub-helper: Compares the output sample counter and returns 
	// AK_NoMoreData if the end of file was reached, or AK_DataReady otherwise.
	// Invokes OnLoopComplete() loop if applicable.
	AKRESULT HandleLoopingOrEndOfFile();

	// Clamps the number of requested frames to the end of loop or file, according to the current 
	// position and loop count.
	inline void ClampRequestedFrames( 
		AkUInt16 & io_uFrames	// In: Requested number of frames. Out: Number of frames clamped to loop end or eof.
		)
	{
		AkUInt32 uLimit = DoLoop() ? m_uPCMLoopEnd + 1 : m_uTotalSamples;
		if ( m_uCurSample + io_uFrames > uLimit )
		{
			AKASSERT( uLimit > m_uCurSample );
			io_uFrames = (AkUInt16)( uLimit - m_uCurSample );
		}
	}

	// Pre-buffering: returns AK_DataReady by default. This service is defined here so that codec implementations
	// may implement GetBuffer() independently of whether it is a bank or a streaming source.
	inline AKRESULT IsPrebufferingReady() { return AK_DataReady; }

	// Looping. 
	inline bool DoLoop() const { return m_uLoopCnt != LOOPING_ONE_SHOT; }	// Returns true if we are going to loop.
	inline void ResetLoopCnt() { m_uLoopCnt = m_pCtx->GetLooping(); }
	
	inline void SetLoopCnt( AkUInt16 in_uLoopCnt ) { m_uLoopCnt = in_uLoopCnt; }	// Don't call this unless you know what you're doing.

	// Get the seek value from PBI and returns it as the number 
	// of PCM samples (source's rate) from the beginning of the file.
	// Considers markers if applicable, and adjusts the loop count.
	AkUInt32 GetSourceOffset();

	// Position helpers.
	void UpdatePositionInfo( AkReal32 in_fLastRate, AkUInt32 in_uStartPos, AkUInt32 in_uFileEnd );
	void NotifyRelevantMarkers( AkUInt32 in_uStartSample, AkUInt32 in_uEndSample );	// Region: [in_uStartSample, in_uEndSample[

	// Marker helpers.
	//
	void CopyRelevantMarkers( 
		AkPipelineBuffer & io_buffer, 
		AkUInt32 in_ulBufferStartPos
		);

	// These 3 functions are used for virtual voices only
	void TimeSkipMarkersAndPosition( AkUInt32 in_ulCurrSampleOffset, AkUInt32 in_uSkippedSamples, AkUInt32 in_uFileEnd ); // Region: [in_ulCurrSampleOffset, in_ulCurrSampleOffset+in_uSkippedSamples[
	
	AkReal32 GetDurationNoLoop() const;		// Returns duration of file in ms, ignoring looping.
	
	// Seeking helper: Converts an absolute source position (which takes looping region into account) into 
	// a value that is relative to the beginning of the file, and the number of loops remaining.
	// Warning: May return an offset that is greater than the end of the source.
	void AbsoluteToRelativeSourceOffset( 
		AkUInt32 in_uAbsoluteSourcePosition, 
		AkUInt32 & out_uRelativeSourceOffset,
		AkUInt16 & out_uRemainingLoops 
		) const;

protected:

	AkUInt32			m_uTotalSamples;	// Total number of samples in PCM.
	AkUInt32			m_uCurSample;		// Current PCM sample position, used for markers, time skip, and so on.

	AkUInt32			m_uDataSize;		// Data size.
	AkUInt32			m_uDataOffset;		// Data offset.

	AkUInt32			m_uPCMLoopStart;
	AkUInt32			m_uPCMLoopEnd;

	CAkMarkers			m_markers;

	AkUInt32			m_uLastEnvelopePtIdx;	// Index of last envelope point (optimizes search).

private:
	AkUInt16            m_uLoopCnt;			// Number of remaining loops.
};

#endif
