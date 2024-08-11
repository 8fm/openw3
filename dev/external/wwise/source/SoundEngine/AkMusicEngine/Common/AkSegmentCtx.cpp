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
// AkSegmentCtx.cpp
//
// Segment context.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkSegmentCtx.h"
#include "AkMusicSegment.h"
#include "AkMusicTrack.h"
#include "AkSound.h"
#include "AkMusicRenderer.h"
#include "AkMusicStructs.h"
#include "AkMonitor.h"
#include "AkMatrixSequencer.h"
#include "AkSegmentChain.h"

// Look-ahead time compensation for XMA.
// XMA sources have a non-negligible latency that makes them sound very bad when used with the precise, 
// sample accurate music engine. It compensates it with a little hack, by slightly boosting the look-ahead
// time of XMA sources (streamed as well as in-memory).
#ifdef AK_XBOX360
#include "AkProfile.h"
#define XMA_COMPENSATION		(384)
#endif

// Look-ahead time boost for AAC on iOS.
// AAC sources have a very long setup time on iOS, that has nothing to do with streaming.
// Let's increase it without the user's consent (AAC is not recommended with interactive music).
// Note that as opposed to the XMA compensation, this is just a look-ahead time boost and not a compensation;
// we want playback to remain synchronized the way it is right now.
#ifdef AK_IOS
#include "AkProfile.h"
#define AAC_LOOK_AHEAD_TIME_BOOST	((AkUInt32)((AkReal32)AkAudioLibSettings::g_pipelineCoreFrequency * 400.f / 1000.f))	// 400 ms
#endif

#ifdef AK_PS4
#include "AkProfile.h"
#endif


CAkSegmentCtx::CAkSegmentCtx(
    CAkMusicSegment *   in_pSegmentNode,
    CAkMusicCtx *       in_pParentCtx      
    )
    :CAkMusicCtx( in_pParentCtx )
    ,m_pSegmentNode( in_pSegmentNode )
    ,m_pOwner( NULL )
    ,m_iAudibleTime( 0 )
#ifndef AK_OPTIMIZED
	,m_PlaylistItemID( AK_INVALID_UNIQUE_ID )
#endif
{
	if( m_pSegmentNode )
	{
		// IMPORTANT. Lock once and add ref to protect the children map.
		AkAutoLock<CAkLock> IndexLock( g_pIndex->GetNodeLock( AkNodeType_Default ) ); 

		m_pSegmentNode->AddRef();

		AkUInt16 uNumTracks = m_pSegmentNode->NumTracks();

		if ( m_arTracks.Reserve( uNumTracks ) != AK_Success )
			return;

		for ( AkUInt16 iTrack=0; iTrack<uNumTracks; iTrack++ )
		{
			// Get info of source that is supposed to play on this track at this time.
			CAkMusicTrack * pTrack = m_pSegmentNode->Track( iTrack );
			pTrack->AddRef();
			AKVERIFY( m_arTracks.AddLast( pTrack ) != NULL );
		}
	}
}

CAkSegmentCtx::~CAkSegmentCtx()
{
    m_sequencer.Term();
	m_arTrackRS.Term();

	if( m_pSegmentNode )
	{
		// IMPORTANT. Lock once and add ref to protect the children map.
		AkAutoLock<CAkLock> IndexLock( g_pIndex->GetNodeLock( AkNodeType_Default ) );

		TrackArray::Iterator it = m_arTracks.Begin();
		while ( it != m_arTracks.End() )
		{
			(*it)->Release();
			++it;
		}

		m_pSegmentNode->Release();
	}

	m_arTracks.Term();
}

AKRESULT CAkSegmentCtx::Init(
    CAkRegisteredObj *  in_GameObject,
    UserParams &        in_rUserparams
    )
{
    CAkMusicCtx::Init( in_GameObject, in_rUserparams );

	AkUInt32 uNumTracks = m_arTracks.Length();

	if( m_arTrackRS.Reserve( uNumTracks ) != AK_Success )
		return AK_InsufficientMemory;

	TrackArray::Iterator it = m_arTracks.Begin();
	while ( it != m_arTracks.End() )
	{
		m_arTrackRS.AddLast( (*it)->GetNextRS() );// no error check since we did reserve
		++it;
	}

    return AK_Success;
}

// Perform(): 
// Instantiate and play MusicPBIs in current time window [in_iNow, in_iNow+in_uNumSamples[, 
// schedule and executes associated stop commands.
void CAkSegmentCtx::Process(
	AkInt32		in_iTime,			// Current time (in samples) relative to the Entry Cue.
	AkUInt32	in_uNumSamples		// Time window size.
	)
{
	ProcessPrologue( in_uNumSamples	);

	// Execute all pending actions that should occur in the next audio frame.
	ExecuteScheduledCmds( in_iTime, in_uNumSamples );
    
	// Process automation.
	// Note: Automation values are computed for the _end_ of the processing window.
	AkInt32 iCurrentClipTime = SegmentTimeToClipData( in_iTime + in_uNumSamples );
	AutomationList::Iterator it = m_listAutomation.Begin();
	while ( it != m_listAutomation.End() )
	{
		(*it)->Apply( iCurrentClipTime );
		++it;
	}

	ProcessEpilogue();
}

void CAkSegmentCtx::ScheduleAudioClips()
{
	// Compute the effective segment position when playback will really start.
	// Make it relative to the pre-entry since track clip data is expressed this way.
	AkInt32 iEffectiveSegmentPosition = SegmentTimeToClipData( m_iAudibleTime );
	
	AkUInt32 iTrack = 0;
	TrackArray::Iterator it = m_arTracks.Begin();
	while ( it != m_arTracks.End() )
    {
        // Get info of source that is supposed to play on this track at this time.
        CAkMusicTrack * pTrack = (*it);
        AKASSERT( pTrack );
		const CAkMusicTrack::TrackPlaylist & playList = pTrack->GetTrackPlaylist();
		AkUInt32 uNumClips = playList.Length();
		AkUInt32 uClip = 0;
		while( uClip < uNumClips )
		{
			if( playList[uClip].uSubTrackIndex == m_arTrackRS[iTrack] )
			{
                const AkTrackSrc & srcInfo = playList[uClip];
                CAkMusicSource * pSrc = pTrack->GetSourcePtr( srcInfo.srcID );
				if( pSrc )
				{
					AkSrcTypeInfo * pSrcTypeInfo = pSrc->GetSrcTypeInfo();
					AKASSERT( pSrcTypeInfo );

					// Compute source's look ahead.
					// Look ahead is the source's look-ahead, if it is streaming, and has no prefetched data or
					// play position will not be 0.
					AkUInt32 uSrcLookAhead = 0;
					if ( ( pSrcTypeInfo->mediaInfo.Type == SrcTypeFile ) &&
						( !pSrc->IsZeroLatency() ||
						iEffectiveSegmentPosition > (AkInt32)srcInfo.uClipStartPosition ||
						srcInfo.iSourceTrimOffset != 0 ) )
					{
						uSrcLookAhead = pSrc->StreamingLookAhead();
					}

#ifdef AK_IOS
					// Look-ahead time boost for iOS AAC. This is NOT a codec-specific latency compensation.
					if ( CODECID_FROM_PLUGINID( pSrcTypeInfo->dwID ) == AKCODECID_AAC 
						&& !AK_PERF_OFFLINE_RENDERING )
					{
						uSrcLookAhead += AAC_LOOK_AHEAD_TIME_BOOST;
					}
#endif
#ifdef AK_XBOX360
					if ( CODECID_FROM_PLUGINID( pSrcTypeInfo->dwID ) == AKCODECID_XMA 
						&& !AK_PERF_OFFLINE_RENDERING )
					{
						uSrcLookAhead += AK_NUM_VOICE_REFILL_FRAMES;
					}
#endif
#ifdef AK_XBOXONE
					if ( CODECID_FROM_PLUGINID( pSrcTypeInfo->dwID ) == AKCODECID_XMA )
					{
						uSrcLookAhead += AK_NUM_VOICE_REFILL_FRAMES;
					}
#endif

#ifdef AK_PS4
					if ( CODECID_FROM_PLUGINID( pSrcTypeInfo->dwID ) == AKCODECID_ATRAC9 )
					{
						uSrcLookAhead += AK_NUM_VOICE_REFILL_FRAMES;
					}
#endif


					// Start everything that exist after current position (taking segment's look-ahead into account).
					AkInt32 iStopAt = (AkInt32)( srcInfo.uClipStartPosition + srcInfo.uClipDuration );
					if ( iEffectiveSegmentPosition < iStopAt )
					{
						// Should be played.

						// Compute action time and source offset (start position).
						// If effective position is before clip start, then the source offset is the 
						// trim offset and the action time is the effective play position. 
						// Otherwise, the source offset is the trim offset plus the offset between the segment
						// position and the clip start position (wrapped over the source's duration),
						// and the action time is the segment position (now).
						
						AkInt32 iActionTime;
						AkInt32 iSourceOffset;
						if ( iEffectiveSegmentPosition > (AkInt32)srcInfo.uClipStartPosition )
						{
							iActionTime = iEffectiveSegmentPosition - (AkInt32)uSrcLookAhead;
							iSourceOffset = ( ( iEffectiveSegmentPosition - (AkInt32)srcInfo.uClipStartPosition ) + srcInfo.iSourceTrimOffset ) % srcInfo.uSrcDuration;
						}
						else
						{
							iActionTime = (AkInt32)srcInfo.uClipStartPosition - (AkInt32)uSrcLookAhead;
							iSourceOffset = srcInfo.iSourceTrimOffset;
						
							// Special case to compensate for XMA latency: shift action start time if and only
							// if source offset is 0 (seeking XMA is sample-accurate).
#ifdef AK_XBOX360
							if ( iSourceOffset == 0 
								&& CODECID_FROM_PLUGINID( pSrcTypeInfo->dwID ) == AKCODECID_XMA )
							{
								iActionTime -= XMA_COMPENSATION;
							}
#endif
						}
						AKASSERT( iSourceOffset >= 0 );

						AkMusicActionPlay * pAction = AkNew( g_DefaultPoolId, AkMusicActionPlay(
							ClipDataToSegmentTime( iActionTime ),
							pTrack,
							srcInfo,
							iSourceOffset,
							uSrcLookAhead ) );

						if ( pAction )
						{
							// Query track for clip automation.
							pAction->AttachClipAutomation( uClip, AutomationType_Volume, srcInfo.uClipStartPosition );
							pAction->AttachClipAutomation( uClip, AutomationType_LPF, srcInfo.uClipStartPosition );
							pAction->AttachClipAutomation( uClip, AutomationType_FadeIn, srcInfo.uClipStartPosition );
							pAction->AttachClipAutomation( uClip, AutomationType_FadeOut, srcInfo.uClipStartPosition );

							m_sequencer.ScheduleAction( pAction );
						}
					}
				}
			}
			++uClip;
		}
		++it;
		++iTrack;
    }
}

// Context commands
//

// Initialize context for playback.
// Prepare the context for playback: set initial context position.
// Audible playback will start at position in_iSourceOffset (relative to Entry Cue).
// Returns the exact amount of time (samples) that will elapse between call to _Play() and 
// beginning of playback at position in_iSourceOffset.
AkInt32 CAkSegmentCtx::Prepare(
    AkInt32 in_iSourceOffset	// Position in samples, at the native sample rate, relative to the Entry Cue.
    )
{
	// The time when this context should be audible is in_iSourceOffset -> m_iAudibleTime.
	// ScheduleAudioClips() uses m_iAudibleTime to compute appropriate play actions based on clip data.
    m_iAudibleTime = in_iSourceOffset;

	// Compute the look-ahead time required to begin playback exactly at in_iSourceOffset.
	return ComputeMinSrcLookAhead( in_iSourceOffset );	
}

void CAkSegmentCtx::OnPlayed()
{
	CAkMusicCtx::OnPlayed();

	// Schedule audio clips playback in this segment.
	ScheduleAudioClips();

	NotifyAction( AkMonitorData::NotificationReason_MusicSegmentStarted );
}

// Override MusicCtx OnStopped: Need to flush actions to release children.
void CAkSegmentCtx::OnStopped()
{
	AddRef();

	// Flush sequencer and automation.
	Flush();

	if ( m_pOwner )
	{
		// Notify music segment ended (once - IsPlaying() is checked therein).
		NotifyAction( AkMonitorData::NotificationReason_MusicSegmentEnded );

		// Notify owner.
		m_pOwner->Detach();
	}

	CAkMusicCtx::OnStopped();

	Release();
}

// Override MusicCtx OnPaused on some platforms: Stop all sounds and prepare to restart them on resume (WG-19814).
void CAkSegmentCtx::OnPaused()
{
	// Execute pause, even if we are going to stop all our children, in order to keep the Transport happy.
	CAkMusicCtx::OnPaused();

	if ( m_pOwner )
		m_pOwner->OnPaused();

#ifdef AK_STOP_MUSIC_ON_PAUSE

	// Stop all children (with min time transition).
	// Note: Children of segment contexts are MusicPBIs. They cannot be destroyed from within OnStopped().
	ChildrenCtxList::Iterator it = m_listChildren.Begin( );
    while ( it != m_listChildren.End( ) )
    {
#ifndef AK_OPTIMIZED
		static_cast<CAkMusicPBI*>(*it)->MonitorPaused();
#endif
		(*it)->OnLastFrame( AK_NO_IN_BUFFER_STOP_REQUESTED );
		++it;
    }

	Flush();

#endif	// AK_STOP_MUSIC_ON_PAUSE
}

#ifdef AK_STOP_MUSIC_ON_PAUSE
void CAkSegmentCtx::OnResumed()
{
	// Restart all clips at the current position + look-ahead time.
	// Avoid doing this if not playing: OnResumed() is broadcasted to everyone, even if they haven't 
	// started playing yet.
	if ( IsPlaying()
		&& IsPaused() )	// Also, OnResumed() is called even if we weren't paused.
	{
		RescheduleAudioClipsNow();
	}

	CAkMusicCtx::OnResumed();
}
#endif	// AK_STOP_MUSIC_ON_PAUSE

#ifndef AK_OPTIMIZED
// Catch MusicCtx OnEditDirty(): Stop and reschedule all audio clips.
void CAkSegmentCtx::OnEditDirty()
{
	ChildrenCtxList::Iterator it = m_listChildren.Begin( );
    while ( it != m_listChildren.End( ) )
    {
		(*it)->OnLastFrame( 0 );
		++it;
    } 

	Flush();

	if ( IsPlaying() )
	{
		MONITOR_ERRORMESSAGE( AK::Monitor::ErrorCode_MusicClipsRescheduledAfterTrackEdit );
		RescheduleAudioClipsNow();
	}
}
#endif

// Re-schedule all audio clips to play now + look-ahead time corresponding to now.
// Note: Sounds should have been stopped and dcheduler and automation should have been flushed already.
void CAkSegmentCtx::RescheduleAudioClipsNow()
{
	// Compute the global (worst case) look-ahead time at the current position, then prepare
	// the context at this at this new position. Re-preparing until it converges.
	AkInt32 iSegmentPosition = ((CAkChainCtx*)Parent())->GetSegmentPosition( m_pOwner );
	AkInt32 iLookAheadTime = Prepare( iSegmentPosition );
	if ( iLookAheadTime > 0 )
	{
		AkInt32 iNewLookAheadTime = Prepare( iSegmentPosition + iLookAheadTime );
		while ( iNewLookAheadTime > iLookAheadTime )
		{
			iLookAheadTime = iNewLookAheadTime;
			iNewLookAheadTime = Prepare( iSegmentPosition + iLookAheadTime );
		}
	}

	// Re-schedule audio clips at the new prepared position.
	ScheduleAudioClips();
}

// Flush sequencer commands and automation.
void CAkSegmentCtx::Flush()
{
	while ( !m_listAutomation.IsEmpty() )
	{
		AkMusicAutomation * pAutomation = m_listAutomation.First();
		m_listAutomation.RemoveFirst();
		AkDelete( g_DefaultPoolId, pAutomation );
	}
	m_sequencer.Flush();
}

void CAkSegmentCtx::ExecuteScheduledCmds(
	AkInt32		in_iTime,			// Current time (relative to entry cue).
	AkUInt32	in_uNumSamples		// Time window size.
	)
{
    AkMusicAction * pAction;
    // Get next action.
    while ( m_sequencer.PopImminentAction( in_iTime, in_uNumSamples, pAction ) == AK_DataReady )
    {
        // Execute action and destroy it.
		AKASSERT( ( pAction->Time() - in_iTime ) >= 0 
				&& ( pAction->Time() - in_iTime ) < (AkInt32)in_uNumSamples );
		AkUInt32 uAudioFrameOffset = pAction->Time() - in_iTime;
		switch ( pAction->Type() )
		{
		case MusicActionTypeStop:
			{
				AkMusicActionStop * pActionStop = static_cast<AkMusicActionStop*>(pAction);
				pActionStop->pTargetPBI->_Stop( uAudioFrameOffset );

				// Apply and destroy all automation related to this PBI if applicable.
				if ( pActionStop->bHasAutomation )
				{
					AutomationList::IteratorEx it = m_listAutomation.BeginEx();
					while ( it != m_listAutomation.End() )
					{
						if ( (*it)->pPBI == pActionStop->pTargetPBI )
						{
							AkMusicAutomation * pAutomation = (*it);
							// Note: Automation values are computed for the _end_ of the processing window.
							pAutomation->Apply( SegmentTimeToClipData( in_iTime + in_uNumSamples ) );
							it = m_listAutomation.Erase( it );
							AkDelete( g_DefaultPoolId, pAutomation );
						}
						else
							++it;
					}
				}
			}
			break;

		case MusicActionTypePlay:
			{
				AkMusicActionPlay * pActionPlay = static_cast<AkMusicActionPlay*>(pAction);

				// Trans params: No transition for sources that start playing in NextFrame processing.
				TransParams transParams;
				transParams.TransitionTime = 0;
				CAkMusicPBI * pSrcCtx = NULL;
				const AkTrackSrc & srcInfo = pActionPlay->TrackSrc();
				AKRESULT eResult = CAkMusicRenderer::Play( this,
					pActionPlay->Track(),
					pActionPlay->Source(),
					GameObjectPtr(),
					transParams,
					GetUserParams(),
					&srcInfo,
					pActionPlay->SourceOffset(),
					pActionPlay->PlayOffset() + uAudioFrameOffset,
					pSrcCtx );
				if ( eResult == AK_Success )
				{
					AKASSERT( pSrcCtx );
					
					if ( !IsLastFrame() )
					{
						// Enqueue an explicit Stop action.
						// Action must occur in 
						// source duration + begin_trim_offset + end_trim_offset
						AkMusicActionStop * pActionStop = AkNew( g_DefaultPoolId, AkMusicActionStop(
							ClipDataToSegmentTime( srcInfo.uClipStartPosition + srcInfo.uClipDuration ),// Time
							pSrcCtx ) );										// Target
						if ( pActionStop )
						{
							m_sequencer.ScheduleAction( pActionStop );

							// Register automation to segment context.
							while ( AkMusicAutomation * pAutomation = pActionPlay->PopAutomation() )
							{
								pAutomation->pPBI = pSrcCtx;
								pActionStop->bHasAutomation = true;
								m_listAutomation.AddFirst( pAutomation );
								if ( pAutomation->pAutomationData->Type() == AutomationType_FadeIn )
								{
									// Fade in.
									// Adjust audio frame and source offsets to the closest frame boundary. 
									pSrcCtx->FixStartTimeForFadeIn();
								}
								else if ( pAutomation->pAutomationData->Type() == AutomationType_FadeOut )
								{
									// Fade out. Set flag on music PBI so that it bypasses its intra-frame stopping mechanism.
									pSrcCtx->SetFadeOut();
								}
							}
						}
						else
						{
							// Cannot schedule a stop action for this PBI: do not play it.
							pSrcCtx->_Stop( 0 );
						}
					}
					else
					{
						// We played a PBI after OnLastFrame() was processed. Execute _Stop now.
						// NOTE: Get number of samples in last frame directly from CAkMusicCtx:
						// if it is set to AK_NO_IN_BUFFER_STOP_REQUESTED, we want to pass it as is
						// to our music PBI in order to have a smooth stop.
						pSrcCtx->_Stop( GetNumSamplesInLastFrame() );

						// Apply automation now and destroy.
						while ( AkMusicAutomation * pAutomation = pActionPlay->PopAutomation() )
						{
							pAutomation->pPBI = pSrcCtx;
							// Note: Automation values are computed for the _end_ of the processing window.
							pAutomation->Apply( SegmentTimeToClipData( in_iTime + in_uNumSamples ) );
							AkDelete( g_DefaultPoolId, pAutomation );
						}
					}
				}
			}
			break;

		default:
			AKASSERT( !"Invalid action" );
		}
		AkDelete( g_DefaultPoolId, pAction );
    }
}

// Computes context's look-ahead value (samples) according to the position specified.
AkInt32 CAkSegmentCtx::ComputeMinSrcLookAhead(
    AkInt32		in_iPosition	// Segment position, relative to entry cue.
    )
{
	/// TODO This code should be merged with ScheduleClips().

	// Clip data is relative to beginning of pre-entry.
	in_iPosition = SegmentTimeToClipData( in_iPosition );

    AkInt32 iContextLookAhead = 0;

	AkUInt32 iTrack = 0;
	TrackArray::Iterator it = m_arTracks.Begin();
	while ( it != m_arTracks.End() )
    {
        // Get info from all sources on the required subtrack.
        CAkMusicTrack * pTrack = (*it);
        AKASSERT( pTrack );
		const CAkMusicTrack::TrackPlaylist & playList = pTrack->GetTrackPlaylist();
		CAkMusicTrack::TrackPlaylist::Iterator itPl = playList.Begin();
		while( itPl != playList.End() )
		{
			if( (*itPl).uSubTrackIndex == m_arTrackRS[iTrack] )
			{
                const AkTrackSrc & srcInfo = (*itPl);
				
				CAkMusicSource * pSrc = pTrack->GetSourcePtr( srcInfo.srcID );

				if( pSrc )
				{
					AkSrcTypeInfo * pSrcTypeInfo = pSrc->GetSrcTypeInfo();
					AKASSERT( pSrcTypeInfo );

					// Get source's relative start position. If < 0, make 0.
					AkInt32 iRelativeStartPos = srcInfo.uClipStartPosition - in_iPosition;


					// Get required look-ahead for this source at position in_iPosition.
					// Look ahead is the source's look-ahead, if it is streaming, and has no prefetched data or
					// play position will not be 0.
					AkInt32 iSrcRelLookAhead = 0;
					if ( ( pSrcTypeInfo->mediaInfo.Type == SrcTypeFile ) &&
						( !pSrc->IsZeroLatency() || iRelativeStartPos < 0 
						|| srcInfo.iSourceTrimOffset != 0 ) )
					{
						iSrcRelLookAhead = pSrc->StreamingLookAhead();
					}

					// Special case to compensate for XMA and AAC latency.
#ifdef AK_XBOX360
					if ( CODECID_FROM_PLUGINID( pSrcTypeInfo->dwID ) == AKCODECID_XMA )
					{
						iSrcRelLookAhead += AK_NUM_VOICE_REFILL_FRAMES;
						// In some cases XMA may require an additional look-ahead for compensation.
						iSrcRelLookAhead += XMA_COMPENSATION;
					}
#endif
#ifdef AK_XBOXONE
					if ( CODECID_FROM_PLUGINID( pSrcTypeInfo->dwID ) == AKCODECID_XMA )
					{
						iSrcRelLookAhead += AK_NUM_VOICE_REFILL_FRAMES;
					}
#endif
#ifdef AK_IOS
					if ( CODECID_FROM_PLUGINID( pSrcTypeInfo->dwID ) == AKCODECID_AAC 
						&& !AK_PERF_OFFLINE_RENDERING )
					{
						iSrcRelLookAhead += AAC_LOOK_AHEAD_TIME_BOOST;
					}
#endif
#ifdef AK_PS4
					if ( CODECID_FROM_PLUGINID( pSrcTypeInfo->dwID ) == AKCODECID_ATRAC9 )
					{
						iSrcRelLookAhead += AK_NUM_VOICE_REFILL_FRAMES;
					}
#endif

					if ( iRelativeStartPos < 0 )
						iRelativeStartPos = 0;

					iSrcRelLookAhead -= iRelativeStartPos;

					if ( iContextLookAhead < iSrcRelLookAhead )
						iContextLookAhead = iSrcRelLookAhead;
				}
				else
				{
					MONITOR_ERROR( AK::Monitor::ErrorCode_SelectedChildNotAvailable );
				}
			}
			++itPl;
		}
		++it;
		++iTrack;
    }

    return iContextLookAhead;
}

// Convert clip data time (relative to beginning of pre-entry) to segment time (relative to entry cue).
AkInt32 CAkSegmentCtx::ClipDataToSegmentTime(
	AkInt32		in_iClipDataTime		// Time, relative to beginning of pre-entry.
	)
{
	return in_iClipDataTime - m_pSegmentNode->PreEntryDuration();
}

// Convert segment time (relative to entry cue) to clip data time (relative to beginning of pre-entry).
AkInt32 CAkSegmentCtx::SegmentTimeToClipData(
	AkInt32		in_iClipDataTime		// Time, relative to entry cue.
	)
{
	return in_iClipDataTime + m_pSegmentNode->PreEntryDuration();
}


// PBI notifications.
//

// Called when PBI destruction occurred from Lower engine without the higher-level hierarchy knowing it.
// Remove all references to this target from this segment's sequencer.
void CAkSegmentCtx::RemoveAllReferences( 
    CAkPBI * in_pCtx   // Context whose reference must be removed.
    )
{
	// Remove all references from the action scheduler.
    m_sequencer.ClearActionsByTarget( (CAkMusicPBI*)in_pCtx );

	// Remove all references from the automation list.
	AutomationList::IteratorEx it = m_listAutomation.BeginEx();
	while ( it != m_listAutomation.End() )
	{
		if ( (*it)->pPBI == in_pCtx )
		{
			AkMusicAutomation * pAutomation = (*it);
			it = m_listAutomation.Erase( it );
			AkDelete( g_DefaultPoolId, pAutomation );
		}
		else
			++it;
	}
}

// Computes source offset (in file) and frame offset (in pipeline time, relative to now) required to make a given source
// restart (become physical) sample-accurately.
// Returns false if restarting is not possible because of the timing constraints.
bool CAkSegmentCtx::GetSourceInfoForPlaybackRestart(
	const CAkMusicPBI * in_pCtx,	// Context which became physical.
	AkInt32 & out_iLookAhead,		// Returned required look-ahead time (frame offset).
	AkInt32 & out_iSourceOffset		// Returned required source offset ("core" sample-accurate offset in source).
    )
{
	// This context can be playing, about to stop, or even already stopped in the case of platforms which
	// may take several lower engine frames before stopping after being told so by behavioral engines.
	// Music PBIs should not bother becoming physical if the segment context is stopping.
	AKASSERT( !IsIdle() );
	if ( IsStopping() )
		return false;
	AKASSERT( m_pOwner );

	// Compute and set new source offset and look-ahead time.
	CAkMusicSource * pSrc = static_cast<CAkMusicSource*>( in_pCtx->GetSource() );
	AKASSERT( pSrc || !"PBI has no source" );

	AkSrcTypeInfo * pSrcTypeInfo = pSrc->GetSrcTypeInfo();
	AKASSERT( pSrcTypeInfo );


	// Get required look-ahead for this source at position iSourceOffset.
	// Look ahead is the source's look-ahead, if it is streaming (can't use prefetch data here... TODO Enable in Lower Engine).
	if ( pSrcTypeInfo->mediaInfo.Type == SrcTypeFile )
		out_iLookAhead = pSrc->StreamingLookAhead();
	else
		out_iLookAhead = 0;

	// XMA and AAC latency compensation: use bigger look-ahead to compute source offset.
#ifdef AK_XBOX360
	if ( CODECID_FROM_PLUGINID( pSrcTypeInfo->dwID ) == AKCODECID_XMA )
	{
		out_iLookAhead += XMA_COMPENSATION;
	}
#endif
#ifdef AK_XBOXONE
	if ( CODECID_FROM_PLUGINID( pSrcTypeInfo->dwID ) == AKCODECID_XMA )
	{
		out_iLookAhead += AK_NUM_VOICE_REFILL_FRAMES;
	}
#endif
#ifdef AK_IOS
	if ( CODECID_FROM_PLUGINID( pSrcTypeInfo->dwID ) == AKCODECID_AAC
		&& !AK_PERF_OFFLINE_RENDERING )
	{
		out_iLookAhead += AAC_LOOK_AHEAD_TIME_BOOST;
	}
#endif
#ifdef AK_PS4
	if ( CODECID_FROM_PLUGINID( pSrcTypeInfo->dwID ) == AKCODECID_ATRAC9 )
	{
		out_iLookAhead += AK_NUM_VOICE_REFILL_FRAMES;
	}
#endif

	// Round up look ahead time to AK_NUM_VOICE_REFILL_FRAMES, so that voice restart coincides with 
	// an audio frame (fade ins can only be smooth when applied to a full audio frame).
	out_iLookAhead = ( ( out_iLookAhead + AK_NUM_VOICE_REFILL_FRAMES - 1 ) / (AkUInt32)AK_NUM_VOICE_REFILL_FRAMES ) * AK_NUM_VOICE_REFILL_FRAMES;
	
	// Get source's track info.
	const AkTrackSrc * pSrcInfo = ((CAkMusicPBI*)in_pCtx)->GetSrcInfo();
	
	// Compute absolute segment position.
	// Note: Handling of virtual voices occur _after_ processing for next frame, so the engine's time has been
	// ticked already. Need to go back in time (-AK_NUM_VOICE_REFILL_FRAMES).
	// TODO: Centralize all ticks to one generic call at the end of an audio frame.
	AkUInt32 uNumFramesElapsed = (static_cast<CAkChainCtx*>(Parent()))->Sequencer()->CurTimeWindowSize();
	AkInt32 iAbsoluteSegmentPosition = SegmentTimeToClipData((static_cast<CAkChainCtx*>(Parent()))->GetSegmentPosition( m_pOwner )) - uNumFramesElapsed;

	// Compute source offset according to current segment position, and 
	AkInt32 iClipStartOffset = iAbsoluteSegmentPosition - ( (AkInt32)pSrcInfo->uClipStartPosition );

	// Check restart position (taking the required look-ahead time into account) against scheduled stop position.
	AkInt32 iStopAt = (AkInt32)( pSrcInfo->uClipStartPosition + pSrcInfo->uClipDuration );
	if ( iAbsoluteSegmentPosition + out_iLookAhead >= iStopAt )
		return false;	// Restarting will occur after scheduled stop.

	// Compute source offset: translate clip start offset into source offset, taking its required look-ahead time into account.
	out_iSourceOffset = ( iClipStartOffset + pSrcInfo->iSourceTrimOffset + out_iLookAhead ) % pSrcInfo->uSrcDuration;
	if ( out_iSourceOffset < 0 ) 
	{
		AKASSERT( !"The look-ahead time should have compensated for the negative segment position" );
		out_iSourceOffset = 0;
	}

	// XMA latency compensation: bigger look-ahead was used, but we need the lower engine to start earlier.
	// Correct it now.
#ifdef AK_XBOX360
	if ( CODECID_FROM_PLUGINID( pSrcTypeInfo->dwID ) == AKCODECID_XMA )
	{
		AKASSERT( out_iLookAhead >= XMA_COMPENSATION );
		out_iLookAhead -= XMA_COMPENSATION;
	}
#endif

	return true;
}

CAkRegisteredObj * CAkSegmentCtx::GameObjectPtr()
{ 
    AKASSERT( Parent() );
    return static_cast<CAkMatrixAwareCtx*>(Parent())->Sequencer()->GameObjectPtr(); 
}

AkPlayingID CAkSegmentCtx::PlayingID()
{ 
    AKASSERT( Parent() );
    return static_cast<CAkMatrixAwareCtx*>(Parent())->Sequencer()->PlayingID(); 
}

UserParams & CAkSegmentCtx::GetUserParams()
{ 
    AKASSERT( Parent() );
    return static_cast<CAkMatrixAwareCtx*>(Parent())->Sequencer()->GetUserParams(); 
}

void CAkSegmentCtx::NotifyAction( AkMonitorData::NotificationReason in_eReason )
{
#ifndef AK_OPTIMIZED
	if( !IsIdle() && GetPlayListItemID() != AK_INVALID_UNIQUE_ID )
	{
		AKASSERT( SegmentNode() && SegmentNode()->Parent() && SegmentNode()->Parent()->NodeCategory() == AkNodeCategory_MusicRanSeqCntr );
		// At this point, SegmentNode()->Parent() is a MusicRanSeqCntr
		MONITOR_MUSICOBJECTNOTIF( PlayingID(), GameObjectPtr()->ID(), GetUserParams().CustomParam(), in_eReason, SegmentNode()->Parent()->ID(), GetPlayListItemID() );
	}
#endif
}
