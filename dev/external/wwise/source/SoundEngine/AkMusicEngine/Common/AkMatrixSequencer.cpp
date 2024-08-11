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
// AkMatrixSequencer.cpp
//
// Multi-chain, branchable segment sequencer.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkMatrixSequencer.h"
#include "AkMusicSegment.h"
#include "AkMusicRenderer.h"
#include "AkMatrixAwareCtx.h"
#include "AkScheduleWindow.h"
#include "AkMonitor.h"
#include "AkPlayingMgr.h"
#include "AkScheduleWindow.h"


///////////////////////////////////////////////////

CAkScheduledItem::CAkScheduledItem(
    AkInt64			in_iLocalTime,
	CAkSegmentCtx * in_pSegment
    )
:pNextItem( NULL )
,m_iLocalTime( in_iLocalTime )
,m_pSegment( in_pSegment )
,m_bCmdPlayPending( false )
,m_bCmdStopPending( false )
,m_bWasPlaybackSkipped( false )
{
    if ( m_pSegment )
        m_pSegment->SetOwner( this );
}

CAkScheduledItem::~CAkScheduledItem()
{
    AKASSERT( !m_pSegment || !"Low-level segment context should have been detached first" );
	AKASSERT( m_listAssociatedActions.IsEmpty() );
	m_listAssociatedActions.Term();
}

// Process actions. To be called from segment chain once per audio frame.
void CAkScheduledItem::Process(
	AkInt64		in_iCurrentTime,		// Current time in the time coordinates of the caller (chain).
	AkUInt32	in_uNumSamples,			// Number of samples to process.
	bool		in_bSkipPlay			// If true, skip play command.
    )
{
	in_iCurrentTime = ParentToLocalTime( in_iCurrentTime );
	AkInt64	iCurrentFrameEnd = in_iCurrentTime + in_uNumSamples;
	
	if ( m_pSegment )
	{
		// Note: If there is a low-level segment, current time must be within a reasonnable range from it 
		// (less than 3 segment-wide authored in the tool), so it is safe to convert it to 32 bits.
		AkInt32 iShortRangeCurTime = AkTimeConv::ToShortRange( in_iCurrentTime );
		
		// Perform scheduled actions.

		// Play command.
		if ( m_bCmdPlayPending )
		{
			// (we spend most of our time after the play cmd)
			if ( iShortRangeCurTime <= m_cmdPlay.iRelativeTime
				&& iCurrentFrameEnd > m_cmdPlay.iRelativeTime )
			{
				if ( !in_bSkipPlay )
					m_pSegment->_Play( m_cmdPlay.fadeParams );
				else
					m_bWasPlaybackSkipped = true;
				m_bCmdPlayPending = false;
			}
		}
		else if ( in_bSkipPlay && m_pSegment->IsPlaying() && !m_bWasPlaybackSkipped )
		{
			// If playback should be skipped but segment is already playing, fade it out.
			_CancelPlayback( iShortRangeCurTime);
		}

		// Stop command. When it is executed, segment may detach itself from this item.
		if ( m_bCmdStopPending )
		{
			// (we spend most of the time before the stop cmd)
			if ( iCurrentFrameEnd > m_cmdStop.iRelativeTime 
				&& iShortRangeCurTime <= m_cmdStop.iRelativeTime )
			{
				m_pSegment->_Stop( m_cmdStop.transParams, m_cmdStop.iRelativeTime - iShortRangeCurTime );
				m_bCmdStopPending = false;
			}
		}

		// Run low-level segment context.
		if ( m_pSegment && m_pSegment->RequiresProcessing() )
			m_pSegment->Process( iShortRangeCurTime, in_uNumSamples );
	}

    // Associated commands (stingers, states).
    AssociatedActionsList::IteratorEx itAction = m_listAssociatedActions.BeginEx();
    while ( itAction != m_listAssociatedActions.End() )
    {
		AkAssociatedAction * pAction = (*itAction);
		if ( iCurrentFrameEnd > pAction->iRelativeTime )
		{
            // Dequeue action for execution.
            AKASSERT( pAction->iRelativeTime >= in_iCurrentTime ||
                        !"This action should already have been executed" );

            switch ( pAction->eActionType )
            {
            case AssocActionTypeStinger:
				// Stinger. Nothing to do, it is processed by the sequencer. 
				// After it is removed, it will not be rescheduled if this item is unexpectedly stopped.
                break;
            case AssocActionTypeState:
				// Notify music renderer for state change.
                CAkMusicRenderer::Get()->PerformDelayedStateChange( pAction->pStateChangeCookie );
                break;
            }

			itAction = m_listAssociatedActions.Erase( itAction );
			AkDelete( g_DefaultPoolId, pAction );
        }
        else
            ++itAction;
    }
}

// Returns segment's active duration (0 if no segment).
AkInt32 CAkScheduledItem::SegmentDuration()
{
	if ( m_pSegment )
		return m_pSegment->SegmentNode()->ActiveDuration();
	return 0;
}

// Music queries.
AKRESULT CAkScheduledItem::GetInfo(
	AkInt32			in_iSegmentPosition,	// Segment position.
	AkSegmentInfo &	out_uSegmentInfo		// Returned info.
	)
{
	if ( m_pSegment )
	{
		CAkMusicSegment * pNode = m_pSegment->SegmentNode();
		AKASSERT( pNode );
		out_uSegmentInfo.iPreEntryDuration	= AkTimeConv::SamplesToMilliseconds( pNode->PreEntryDuration() );
		out_uSegmentInfo.iActiveDuration	= AkTimeConv::SamplesToMilliseconds( pNode->ActiveDuration() );
		out_uSegmentInfo.iPostExitDuration	= AkTimeConv::SamplesToMilliseconds( pNode->PostExitDuration() );
		out_uSegmentInfo.iCurrentPosition	= AkTimeConv::SamplesToMilliseconds( in_iSegmentPosition );
		AkInt32 iRemainingSilentTime = m_pSegment->GetRemainingSilentTime( in_iSegmentPosition );
		if ( iRemainingSilentTime > 0 )
			out_uSegmentInfo.iRemainingLookAheadTime = AkTimeConv::SamplesToMilliseconds( iRemainingSilentTime );
		else
			out_uSegmentInfo.iRemainingLookAheadTime = 0;
	}
	else
	{
		// Otherwise, this is an empty bucket. It's not considered as an error, it's just that
		// there's nothing to say about it.
		out_uSegmentInfo.iPreEntryDuration	= 0;
		out_uSegmentInfo.iActiveDuration	= 0;
		out_uSegmentInfo.iPostExitDuration	= 0;
		out_uSegmentInfo.iCurrentPosition	= 0;
		out_uSegmentInfo.iRemainingLookAheadTime = 0;
	}	
	return AK_Success;
}

void CAkScheduledItem::OnStopped()
{
	m_bCmdPlayPending = false;
	m_bCmdStopPending = false;

	// Execute all associated actions now.
	// Associated commands (stingers, states).
    AssociatedActionsList::IteratorEx itAction = m_listAssociatedActions.BeginEx();
    while ( itAction != m_listAssociatedActions.End() )
    {
		AkAssociatedAction * pAction = (*itAction);

        switch ( pAction->eActionType )
        {
        case AssocActionTypeStinger:
			// Stinger. Nothing to do, it is processed by the sequencer. 
			// After it is removed, it will not be rescheduled if this item is unexpectedly stopped.
            break;
        case AssocActionTypeState:
			// Notify music renderer for state change.
            CAkMusicRenderer::Get()->PerformDelayedStateChange( pAction->pStateChangeCookie );
            break;
        }
		itAction = m_listAssociatedActions.Erase( itAction );
		AkDelete( g_DefaultPoolId, pAction );
    }

	// Release low-level segment if it is still attached.
	if ( m_pSegment )
	{
		AKASSERT( !m_pSegment->IsPlaying() );
		Detach();
	}
}

// Context paused:
// Execute delayed state changes: they must not be held while music is paused (WG-20167).
void CAkScheduledItem::OnPaused()
{
	AssociatedActionsList::IteratorEx itAction = m_listAssociatedActions.BeginEx();
    while ( itAction != m_listAssociatedActions.End() )
    {
		AkAssociatedAction * pAction = (*itAction);
		if ( pAction->eActionType == AssocActionTypeState )
		{
			// Notify music renderer for state change.
            CAkMusicRenderer::Get()->PerformDelayedStateChange( pAction->pStateChangeCookie );
			itAction = m_listAssociatedActions.Erase( itAction );
			AkDelete( g_DefaultPoolId, pAction );
        }
		else
			++itAction;		
    }
}

// Prepares segment context (if applicable), modifies associated Play action time to look-ahead.
// The segment will be ready to play at the specified start position (relative to EntryCue). They might however
// be set to begin earlier, in case in_iFadeOffset is negative, but in this case, the returned preparation
// time takes this duration into account.
// Returns the required time needed before the synchronization point (can be negative! with a positive fade offset).
AkInt32 CAkScheduledItem::Prepare( 
	AkUInt32	in_uSyncPosition,	// Position within the item (relative to segment Entry Cue) where sync should occur.
    AkInt32		in_iStartPosition	// Position where playback should start (relative to segment Entry Cue).
    )
{
	// An empty item always takes 0 look-ahead time
	AkInt32 iSchedulingLookAhead = 0;

	if ( m_pSegment )
	{
		AKASSERT( !m_pSegment->IsPlaying() );
		
		// Prepare segment. Take fade offset into account.
		AkInt32 iSegmentLookAhead = m_pSegment->Prepare( in_iStartPosition );

		// Fix look-ahead according to sync position.
		iSchedulingLookAhead = iSegmentLookAhead + ( in_uSyncPosition - in_iStartPosition );
	}
	
	// Set Play action time (relative to this item sync point).
	m_cmdPlay.iRelativeTime = in_uSyncPosition - iSchedulingLookAhead;
	
	// Clear fade on segment play command: a high-level context will take care of it,
	// either by setting it at this level (SetFadeIn) or at a higher level.
	m_cmdPlay.fadeParams.transitionTime = 0;
	
	m_bCmdPlayPending = true;

    return iSchedulingLookAhead;
}

// Commands scheduling.
void CAkScheduledItem::SetFadeIn(
    AkTimeMs                in_iTransDuration,
    AkCurveInterpolation    in_eFadeCurve,
    AkInt32                 in_iFadeOffset
    )
{
    AKASSERT( m_pSegment );
    m_cmdPlay.fadeParams.transitionTime = in_iTransDuration;
    m_cmdPlay.fadeParams.eFadeCurve     = in_eFadeCurve;
    m_cmdPlay.fadeParams.iFadeOffset    = in_iFadeOffset;
	m_bCmdPlayPending = true;
}

void CAkScheduledItem::AttachStopCmd(
    AkTimeMs                in_iTransDuration,
    AkCurveInterpolation    in_eFadeCurve,
    AkInt32                 in_iRelativeTime
    )
{
    AKASSERT( m_pSegment );
	m_cmdStop.transParams.TransitionTime = in_iTransDuration;

	// Deal with fades that are too long, stop times that are too early.
	if ( in_iRelativeTime < m_cmdPlay.iRelativeTime )
	{
		m_cmdStop.transParams.TransitionTime += AkTimeConv::SamplesToMilliseconds( in_iRelativeTime - m_cmdPlay.iRelativeTime );
		in_iRelativeTime = m_cmdPlay.iRelativeTime;
	}

    m_cmdStop.transParams.eFadeCurve = in_eFadeCurve;
    m_cmdStop.iRelativeTime = in_iRelativeTime;
	m_bCmdStopPending = true;
}

// Modify Stop Cmd time: perform a straight stop at either the Exit Cue or end of Post-Exit.
// Notes. 1) Stop command is left untouched if a fade out is already defined.
// 2) Stop command is irrelevant if the bucket is empty.
void CAkScheduledItem::ForcePostExit(
    bool                    in_bPlayPostExit    // Move stop at the end of Post-Exit if True, and Exit Cue if false.
    )
{
    if ( SegmentCtx() &&
		 m_cmdStop.transParams.TransitionTime == 0 )
    {
        CAkMusicSegment * pNode = SegmentCtx()->SegmentNode();
        if ( in_bPlayPostExit )
            m_cmdStop.iRelativeTime = pNode->ActiveDuration() + pNode->PostExitDuration();
        else
            m_cmdStop.iRelativeTime = pNode->ActiveDuration();
    }
}

// Stop a segment based on how much time it has played already. Stopped immediately
// if it has not been playing yet, or faded out by the amount of time it has been playing.
void CAkScheduledItem::CancelPlayback( 
	AkInt64		in_iCurrentTime		// Current time in the time coordinates of the caller (chain).
	)
{
	if ( m_pSegment 
		&& m_pSegment->IsPlaying()
		&& !m_bWasPlaybackSkipped )
	{
		in_iCurrentTime = ParentToLocalTime( in_iCurrentTime );
		_CancelPlayback( AkTimeConv::ToShortRange( in_iCurrentTime ) );
	}
	else
	{
		OnStopped();
	}
}

// Helper: Stop a segment based on how much time it has played already. Must be playing.
void CAkScheduledItem::_CancelPlayback(
	AkInt32		in_iCurrentTime		// Current local time.
	)
{
	AKASSERT( m_pSegment && m_pSegment->IsPlaying() && !m_bWasPlaybackSkipped );
	
	// Fade out segment that already started playing (pre-entry or streaming look-ahead - has not reached 
	// sync point yet).
	TransParams revTransParams;
	revTransParams.eFadeCurve = AkCurveInterpolation_Linear;
	AkInt32 iTimeElapsed = in_iCurrentTime - m_cmdPlay.iRelativeTime;
	AKASSERT( iTimeElapsed >= 0 );
	
	// Take the time while segment was silent into account.
	AkInt32 iSilentTime = m_pSegment->GetAudibleTime() - m_cmdPlay.iRelativeTime;
	AKASSERT( iSilentTime >= 0 );
	iTimeElapsed -= iSilentTime;
	iTimeElapsed = AkMax( iTimeElapsed, 0 );
	
	// Note: Behavior change. Item is faded out by time elapsed only, not sync time,
	// but it takes the silent time into account (related to WG-18010).
	revTransParams.TransitionTime = AkTimeConv::SamplesToMilliseconds( iTimeElapsed );
	m_pSegment->_Stop( revTransParams );

	// Tag this item as 'bWasPlaybackSkipped': it cannot be restored.
	m_bWasPlaybackSkipped = true;
}

void CAkScheduledItem::AttachAssociatedAction(
    AkAssociatedAction * in_pAction
    )
{
    m_listAssociatedActions.AddFirst( in_pAction );
}

void CAkScheduledItem::PopAssociatedActionsToRescheduleAfterTransitionSyncPoint(
    AssociatedActionsList & io_listCancelledActions,
    AkInt64 in_iMinActionTime
    )
{
    AssociatedActionsList::IteratorEx it = m_listAssociatedActions.BeginEx();
    while ( it != m_listAssociatedActions.End() )
    {
        // Note: Stinger actions attached exactly at in_iMinActionTime are removed; state change actions
		// attached exactly at in_iMinActionTime are not.
        if ( (*it)->iRelativeTime > in_iMinActionTime 
			|| ( (*it)->eActionType == AssocActionTypeStinger
				&& (*it)->iRelativeTime == in_iMinActionTime ) )
		{
			// Remove and Push in output array.
			AkAssociatedAction * pAction = (*it);
			it = m_listAssociatedActions.Erase( it );
			io_listCancelledActions.AddFirst( pAction );
		}
        else
            ++it;
    }
}

// Condition for removal from the segment chain.
bool CAkScheduledItem::CanDestroy()
{
    return !m_pSegment && !m_bCmdStopPending && m_listAssociatedActions.IsEmpty();
}

// Dispose after having removed from chain.
void CAkScheduledItem::Destroy()
{
    AkDelete( g_DefaultPoolId, this );
}

void CAkScheduledItem::NotifyMusicCallbacks( 
	AkInt32  in_iSegmentPosition,	// Segment position relative to the entry cue.
	AkUInt32 in_uFrameDuration,		// Range within which music events need to be notified
	AkUInt32 in_uNotifFlags,		// Flags,
	AkPlayingID in_playingID		// Playing ID.
	) const
{
	AKASSERT( in_uFrameDuration > 0 ); // 
	AKASSERT( SegmentCtx() );

	if ( !in_uNotifFlags )
		return;	// Usual case: no one registered to music notifications.

	const AkMusicGrid& rGrid = SegmentCtx()->SegmentNode()->GetMusicGrid();

	// Do UserCues notifications
	if( in_uNotifFlags & AK_MusicSyncUserCue )
	{
		SegmentCtx()->SegmentNode()->NotifyUserCuesInRange( 
			in_playingID, 
			rGrid,
			in_iSegmentPosition, 
			in_uFrameDuration );
	}

	if( in_uNotifFlags & AK_MusicSyncEntry )
	{
		if ( 0 >= in_iSegmentPosition 
			&& 0 < in_iSegmentPosition + (AkInt32)in_uFrameDuration )
		{
			g_pPlayingMgr->NotifyMusic( in_playingID, AK_MusicSyncEntry, rGrid );
		}
	}

	AkUInt32 uNumBars;
	AkUInt32 uNumBeats;
	AkUInt32 uNumGrids;
	SegmentCtx()->SegmentNode()->GetNumMusicGridInRange( 
		in_iSegmentPosition, 
		in_uFrameDuration,
		uNumBars,
		uNumBeats,		
		uNumGrids );

	// Do Bar notifications
	if( in_uNotifFlags & AK_MusicSyncBar )
	{
		for ( AkUInt32 uBar = 0; uBar<uNumBars; uBar++ )
			g_pPlayingMgr->NotifyMusic( in_playingID, AK_MusicSyncBar, rGrid );
	}

	// Do Beat notifications
	if( in_uNotifFlags & AK_MusicSyncBeat )
	{
		for ( AkUInt32 uBeat = 0; uBeat<uNumBeats; uBeat++ )
			g_pPlayingMgr->NotifyMusic( in_playingID, AK_MusicSyncBeat, rGrid );
	}

	// Do Grid notifications
	if( in_uNotifFlags & AK_MusicSyncGrid )
	{
		for ( AkUInt32 uGrid = 0; uGrid<uNumGrids; uGrid++ )
			g_pPlayingMgr->NotifyMusic( in_playingID, AK_MusicSyncGrid, rGrid );
	}

	if( in_uNotifFlags & AK_MusicSyncExit )
	{
		AkInt32 iActiveDuration = SegmentCtx()->SegmentNode()->ActiveDuration();
		if ( iActiveDuration >= in_iSegmentPosition 
			&& iActiveDuration < in_iSegmentPosition + (AkInt32)in_uFrameDuration )
		{
			g_pPlayingMgr->NotifyMusic( in_playingID, AK_MusicSyncExit, rGrid );
		}
	}
}


#ifndef AK_OPTIMIZED
AkUniqueID CAkScheduledItem::SegmentID()
{ 
	if ( m_pSegment 
		&& m_pSegment->SegmentNode() )
		return m_pSegment->SegmentNode()->ID();
	return AK_INVALID_UNIQUE_ID;
}
#endif

///////////////////////////////////////////////////

CAkMatrixSequencer::CAkMatrixSequencer(
    CAkMatrixAwareCtx * in_pOwner,
    UserParams &    in_rUserparams,
    CAkRegisteredObj *  in_pGameObj
    )
:m_uTime( 0 )
,m_pOwner( in_pOwner )
,m_pGameObj( NULL )
,m_uCurTimeWindowSize( 0 )
{
	m_UserParams    = in_rUserparams;
    m_pGameObj      = in_pGameObj;
	m_pGameObj->AddRef();

    // Query triggers and register to state mgr.
    RegisterTriggers();
}

CAkMatrixSequencer::~CAkMatrixSequencer()
{
	// Unregister triggers if applicable.
    UnregisterTriggers();

    m_listPendingStingers.Term();

	if ( m_pGameObj )
	{
		m_pGameObj->Release();
		m_pGameObj = NULL;
	}
}

// Time coordinate conversions.
AkInt64 CAkMatrixSequencer::GlobalToOwnerTime( AkInt64 in_iTime )
{ 
	return in_iTime - m_pOwner->SyncTime(); 
}

AkInt64 CAkMatrixSequencer::OwnerToGlobalTime( AkInt64 in_iTime )
{ 
	return in_iTime + m_pOwner->SyncTime(); 
}


// Used by Renderer:
//
void CAkMatrixSequencer::Execute( AkUInt32 in_uNumFrames )
{
	m_pOwner->AddRef();

    if ( m_pOwner->RequiresProcessing() )
    {
		m_uCurTimeWindowSize = in_uNumFrames;

		// Process concurrent segment chains for next audio frame.
		AkCutoffInfo cutoffInfo;
		m_pOwner->Process( Now(), in_uNumFrames, cutoffInfo );

		// Advance time. Ideally this should be executed at the end of an audio frame.
		Advance();

		m_pOwner->_EndFrame();
    }

	m_pOwner->Release();
}

// Stopping:
void CAkMatrixSequencer::OnStopped()
{
	RemoveAllPendingStingers();
}

// Global processing (stingers, notifications, ...). Called from owner context.
void CAkMatrixSequencer::Process( 
	AkInt64		in_iCurrentTime,	// Current owner time.
	AkUInt32	in_uNumSamples		// Number of samples to process.
	)
{
	ProcessStingers( in_iCurrentTime, in_uNumSamples );
	
	ProcessMusicNotifications( in_iCurrentTime, in_uNumSamples );
}

AkInt32 CAkMatrixSequencer::GetCurSegmentPosition()
{
	// Window should point at the first item, so that even if we are (or started)
	// in the post exit, the returned position is relative to the first item (which 
	// contains the low-level segment context).

	CAkScheduleWindow window( m_pOwner, true );
	if ( !window.IsValid() )
		return 0;
	
	// Convert to absolute segment time.
	CAkMusicNode * pParentNode;
	CAkMusicSegment * pSegmentNode = window.GetNode( &pParentNode );
	if ( pSegmentNode )
	{
		AkInt32 iSegmentPos = window.ToSegmentPosition( GlobalToOwnerTime( Now() ) );
		return iSegmentPos + pSegmentNode->PreEntryDuration();
	}
	else 
	{
		// This can happen if a stinger is being played over this segment and it has finished playing.
		// We know that the parent of this empty item is the segment. 
		AKASSERT( pParentNode->NodeCategory() == AkNodeCategory_MusicSegment );
		pSegmentNode = static_cast<CAkMusicSegment*>(pParentNode);
		return pSegmentNode->Duration(); // end of post-exit
	}
}

// Reschedule actions in active chain.
// Upon returning, all the actions contained in the list are freed and the list is empty.
void CAkMatrixSequencer::RescheduleCancelledActions( 
	AssociatedActionsList & in_listActions 
	)
{
	// Process cancelled actions list.
    AssociatedActionsList::IteratorEx itAction = in_listActions.BeginEx();
    while ( itAction != in_listActions.End() )
    {
        AkAssociatedAction * pAction = (*itAction);
        switch ( pAction->eActionType )
        {
        case AssocActionTypeStinger:
            
			// reschedule if they were scheduled in first item and they have look-ahead property 
            // (record::bCanBeRescheduled is true).
            // destroy all the others and log.

            {
				AkStingerRecord * pRecord = pAction->pStingerRecord;
                AkTriggerID triggerID = pRecord->triggerID;
				bool bDoReschedule = pRecord->bCanBeRescheduled;
				if ( pRecord->pStingerCtx )
					pRecord->pStingerCtx->CancelPlayback( GlobalToOwnerTime( Now() ) );
				
				// Clear (but keep) stinger record until garbage collection.
                ClearStingerRecord( pRecord );

				if ( bDoReschedule )
                {   
                    HandleTrigger( triggerID, true );
                }
                else
                {
                    MONITOR_ERRORMESSAGE( AK::Monitor::ErrorCode_StingerCouldNotBeScheduled );
                }
            }
            break;
        case AssocActionTypeState:
            // Reschedule State change through Music Renderer ("last chance only").
            CAkMusicRenderer::Get()->RescheduleDelayedStateChange( pAction->pStateChangeCookie );
            break;

        default:
            AKASSERT( !"Unhandled associated action type" );
        }

		itAction = in_listActions.Erase( itAction );
		AkDelete( g_DefaultPoolId, pAction );
    }
}

AkUInt32 CAkMatrixSequencer::GetMusicSyncFlags()
{
	AKASSERT( m_pOwner );
	return m_pOwner->GetRegisteredNotif() & AK_MusicSyncAll;
}


// IAkTriggerAware implementation:
// ----------------------------------------
void CAkMatrixSequencer::Trigger( 
    AkTriggerID in_triggerID 
    )
{
    HandleTrigger( in_triggerID, false );
}


// Stingers management.
// ---------------------------------------

// Handle trigger event.
void CAkMatrixSequencer::HandleTrigger( 
    AkTriggerID in_triggerID, 
    bool in_bReschedule
    )
{
	// Get a window to the active segment.
	AkInt64 iCurrentTime = GlobalToOwnerTime( Now() );
	CAkScheduleWindow window( m_pOwner );
	if ( !window.IsValid() )
		return;

	AkUInt32 uLookAheadIdx = 0; // look ahead index in segment chain.

	// Get next segment if this trigger must be rescheduled.
	if ( in_bReschedule )
	{
		// The current window cannot be infinite since we're being re-scheduled.
		AKASSERT( !window.IsDurationInfinite() );

		// Get next segment.
		m_pOwner->GetNextScheduleWindow( window );
		++uLookAheadIdx;
	}

    // Inspect buckets.
    CAkStinger * pStingerData;
    AKRESULT eSyncFoundResult = AK_Fail;

    do
    {
        pStingerData = NULL;

		CAkMusicNode * pParentNode;
        CAkMusicSegment * pSegment = window.GetNode( &pParentNode );
		CAkMusicNode * pNode = ( pSegment ) ? pSegment : pParentNode;
		AKASSERT( pNode );

        // Find THE stinger for that trigger.
        CAkMusicNode::CAkStingers stingers;
        pNode->GetStingers( &stingers );
        CAkMusicNode::StingerArray & arStingers = stingers.GetStingerArray();
        CAkMusicNode::StingerArray::Iterator itStinger = arStingers.Begin();
        while ( itStinger != arStingers.End() )
        {
            // Check match, and inhibation.
            if ( (*itStinger).TriggerID() == in_triggerID )
            {
            	if ( !CanPlayStinger( in_triggerID ) )
            	{
            		// LOG if cannot play stinger (because of DontRepeatTime).
            		MONITOR_ERRORMESSAGE( AK::Monitor::ErrorCode_StingerCouldNotBeScheduled );
					stingers.Term();
            		return;
            	}
                pStingerData = &(*itStinger);
                break;
            }
            
            ++itStinger;
        }
        
        
        // If we found stinger data that matches the input trigger, and that this trigger can be played.
        if ( pStingerData )
        {
            // Found a stinger that could be played. Try to schedule it.
                    
            // Create stinger-segment context and prepare.
            AkInt32 iRequiredDestLookAhead;
            CAkMatrixAwareCtx * pStingerCtx = CreateStingerCtx( pStingerData->SegmentID(), iRequiredDestLookAhead );

			if ( !pStingerCtx && 
                 pStingerData->SegmentID() != AK_INVALID_UNIQUE_ID )
            {
                // Failed creating non-<nothing> stinger segment context.
                // Almost certain it will fail on next attempt so let's leave now.
                stingers.Term();
                return;
            }

            // Try to schedule it in current window. 
            AkInt64 iSyncTime;
			AkUniqueID uCueFilter = pStingerData->CueFilter();
			eSyncFoundResult = window.FindSyncPoint(
				iCurrentTime + iRequiredDestLookAhead,	// Minimum time before finding a valid sync point.
				pStingerData->SyncPlayAt(),	// Sync rule.
				uCueFilter,					// Source cue filter, returned as the selected cue.
				false,						// false = allow sync at entry cue.
				false,						// false = fail if !Immediate rule on <nothing>.
				iSyncTime );				// Returned Sync time in the referential of the creator context.
		
            if ( eSyncFoundResult == AK_Success )
            {
                // Stinger can start playing within this segment.
				AKRESULT eResult = ScheduleStingerForPlayback(
                    window,						// Window over which stinger is scheduled
                    pStingerCtx,				// Stinger
                    pStingerData,				// Stinger data
                    iSyncTime,					// Sync time relative to owner context
                    iRequiredDestLookAhead,		// Look-ahead
                    ( uLookAheadIdx == 0 ) );	// Schedule in next segment (used by rescheduling policy)

                if ( eResult == AK_Success )
                {
					// Note: These 2 ids will be the same as long as only segments may be used as triggers.
					AkUniqueID stingerNodeID = AK_INVALID_UNIQUE_ID;
					AkUniqueID stingerSegmentNodeID = AK_INVALID_UNIQUE_ID;
					if ( pStingerCtx )
					{
						CAkMusicNode * pStingerParent;
						CAkMusicSegment * pStingerSegment = pStingerCtx->GetFirstSegmentNode( &pStingerParent );
						if ( pStingerSegment )
							stingerNodeID = stingerSegmentNodeID = pStingerSegment->ID();
						else 
						{
							AKASSERT( pStingerParent );	// Either the segment node is set, or the parent node exists.
							stingerNodeID = pParentNode->ID();
						}
					}

					MONITOR_MUSICTRANS( 
						PlayingID(), 
						GameObjectPtr()->ID(), 
						(in_bReschedule) ? AkMonitorData::NotificationReason_StingerRescheduled : AkMonitorData::NotificationReason_StingerScheduled, 
						0,
						m_pOwner->NodeID(), 
						( pSegment ) ? pSegment->ID() : pParentNode->ID(),
						stingerNodeID, 
						( pSegment ) ? pSegment->ID() : AK_INVALID_UNIQUE_ID,
						stingerSegmentNodeID, 
						uCueFilter, 
						AK_INVALID_UNIQUE_ID, 
						AkTimeConv::SamplesToMilliseconds( AkTimeConv::ToShortRange(iSyncTime - iCurrentTime) ) );
				}
				else
				{
                    // Failed scheduling stinger. Almost certain it will fail on next attempt so let's leave now.
                    // Destroy created context if applicable.
                    stingers.Term();
                    return;
                }
            }
            else
            {
                // Stinger cannot/shouldn't start playing within this segment.
                // Either prepare to inspect next segment (if it exists), or leave and log dropped stinger.

				if ( pStingerCtx )
					pStingerCtx->_Cancel();

				if ( !window.IsDurationInfinite() 
					&& pStingerData->m_numSegmentLookAhead == 1 
					&& uLookAheadIdx == 0 )
				{
					// Stinger cannot start playing in this segment, but it has LookAhead property:
					// Get next segment.
					m_pOwner->GetNextScheduleWindow( window );
                    ++uLookAheadIdx;
                }
                else
                {
                    // Cannot be scheduled.
					// Log
                    MONITOR_ERRORMESSAGE( AK::Monitor::ErrorCode_StingerCouldNotBeScheduled );
                    
                    // Set sync result to AK_Success to break out.
                    eSyncFoundResult = AK_Success;
                }
            }
        }
        stingers.Term();
    }
    while ( eSyncFoundResult != AK_Success && pStingerData );
}

// Trigger registration.
void CAkMatrixSequencer::RegisterTriggers()
{
    if ( g_pStateMgr->RegisterTrigger( this, m_pGameObj ) != AK_Success )
    {
        // TODO Log Cannot register to triggers.
    }
}

void CAkMatrixSequencer::UnregisterTriggers()
{
    RemoveAllPendingStingers();

    // Unregister all triggers associated with 'this' TriggerAware.
    g_pStateMgr->UnregisterTrigger( this );
}

// Schedule stinger playback.
AKRESULT CAkMatrixSequencer::ScheduleStingerForPlayback(
    const CAkScheduleWindow & in_window,			// Window over which stinger is scheduled.
    CAkMatrixAwareCtx *	in_pStingerCtx,				// Stinger context. Can be NULL (<NOTHING>).
    const CAkStinger *	in_pStingerData,			// Stinger data from node.
	AkInt64				in_iSyncTime,				// Stinger sync time relative to owner context.
    AkInt32				in_iLookAheadDuration,		// Required look-ahead time for scheduling.
    bool				in_bScheduledInCurrentSegment	// True if stinger was scheduled above current segment, false above next segment.
    )
{
    AKASSERT( in_iLookAheadDuration >= 0 );

	// Create a stinger record and an associated action for the scheduled item.
	AkStingerRecord * pStingerRecord = AkNew( g_DefaultPoolId, AkStingerRecord );
	if ( !pStingerRecord )
		return AK_Fail;

	AkAssociatedAction * pAction = AkNew( g_DefaultPoolId, AkAssociatedAction );
	if ( !pAction )
	{
		AkDelete( g_DefaultPoolId, pStingerRecord );
		return AK_Fail;
	}

	pStingerRecord->triggerID           = in_pStingerData->TriggerID();
    pStingerRecord->segmentID           = in_pStingerData->SegmentID();
	pStingerRecord->iSyncTime			= in_iSyncTime;
    pStingerRecord->uDontRepeatTime     = in_pStingerData->DontRepeatTime();
	pStingerRecord->iLookAheadDuration	= in_iLookAheadDuration;

    if ( in_pStingerCtx )
    {
		in_pStingerCtx->Schedule( in_iSyncTime );

		pStingerRecord->pStingerCtx	= in_pStingerCtx;

		// Setup relative stop time.
		CAkMusicSegment * pSegmentNode = in_pStingerCtx->GetFirstSegmentNode();
		if ( pSegmentNode )
			pStingerRecord->iRelativeStopTime = pSegmentNode->ActiveDuration() + pSegmentNode->PostExitDuration();
		else
			pStingerRecord->iRelativeStopTime = 0;
    }
    
    // Can be rescheduled if stinger has look-ahead property, and was scheduled in current segment.
    pStingerRecord->bCanBeRescheduled   = in_bScheduledInCurrentSegment && ( in_pStingerData->m_numSegmentLookAhead > 0 );

	m_listPendingStingers.AddFirst( pStingerRecord );

	// Setup stinger action to active segment.
	
    // Play.
    pAction->eActionType = AssocActionTypeStinger;
	// Note: Behavior change (attach at sync point instead of play time). Stinger can be rescheduled 
	// even if it started playing, in which case it is faded out when cancelled. Related to WG-18010.
	AkInt64 iItemSyncTime = in_window.ToItemTime( in_iSyncTime );
	pAction->iRelativeTime = iItemSyncTime;
    pAction->pStingerRecord = pStingerRecord;

    in_window.GetScheduledItem()->AttachAssociatedAction( pAction );

	return AK_Success;
}

// Create a stinger-segment context.
CAkMatrixAwareCtx * CAkMatrixSequencer::CreateStingerCtx(
    AkUniqueID in_segmentID,
    AkInt32 & out_iLookAheadDuration
    )
{
    CAkMusicNode * pStingerNode = static_cast<CAkMusicNode*>(g_pIndex->GetNodePtrAndAddRef( in_segmentID, AkNodeType_Default ));
    if ( pStingerNode )
    {
		// Create a context, child of top-level context (owner).
		CAkMatrixAwareCtx * pStingerCtx = pStingerNode->CreateContext( m_pOwner, m_pGameObj, m_UserParams );
        if ( pStingerCtx )
        {
			// Stingers always start at pre-entry.
			AkMusicTransDestRule ruleEntry;
			ruleEntry.fadeParams.iFadeOffset = 0;
			ruleEntry.fadeParams.transitionTime = 0;
			ruleEntry.uCueFilterHash = AK_INVALID_UNIQUE_ID;
			ruleEntry.uJumpToID = 0;
			ruleEntry.eEntryType = EntryTypeEntryMarker;
			ruleEntry.bPlayPreEntry = true;
			ruleEntry.bDestMatchSourceCueName = false;
			AkUniqueID uSelectedCue;	// ignored.
			out_iLookAheadDuration = pStingerCtx->Prepare( &ruleEntry, 0, NULL, uSelectedCue );
        }
		pStingerNode->Release();
		return pStingerCtx;
    }
    else
        out_iLookAheadDuration = 0;
	return NULL;    
}

// Can play stinger. False if another stinger is there, with positive don't repeat time.
bool CAkMatrixSequencer::CanPlayStinger(
    AkTriggerID			in_triggerID
    )
{
	// Stingers are processed in the time reference of the owner context. 
	// Get current local time of owner context.
	AkInt64	iCurrentTime = GlobalToOwnerTime( Now() );
	
    PendingStingersList::Iterator it = m_listPendingStingers.Begin();
    while ( it != m_listPendingStingers.End() )
    {
		// NOTE: Don't repeat time is inclusive in order to always avoid playing same stinger at the exact same time, even if it is 0.
        if ( (*it)->triggerID == in_triggerID &&
             (*it)->AbsoluteDontRepeatTime() >= iCurrentTime )	 
        {
            // Cannot play this stinger.
            return false;
        }
        ++it;
    }
    return true;
}

// Process stingers. Dequeues them if necessary.
void CAkMatrixSequencer::ProcessStingers(
	AkInt64		in_iCurrentTime,	// Current local time.
	AkUInt32	in_uFrameDuration	// Number of frames to process.
	)
{
	// Stingers are processed in the time reference of the owner context. 
	AkInt64	iCurrentFrameEnd = in_iCurrentTime + in_uFrameDuration;

	PendingStingersList::IteratorEx it = m_listPendingStingers.BeginEx();
	while ( it != m_listPendingStingers.End() )
	{
		AkStingerRecord * pRecord = (*it);

		// Process context if required. Execute "Play" and "Stop" at the appropriate time.
		if ( pRecord->pStingerCtx )
		{
			AkInt64 iStartTime = pRecord->AbsoluteStartTime();
			if ( iCurrentFrameEnd > iStartTime )
			{
				// Play:
				if ( in_iCurrentTime <= iStartTime )
				{
					// Within the frame where Play should be executed.
					AkMusicFade fadein;
					fadein.iFadeOffset = 0;
					fadein.transitionTime = 0;
					pRecord->pStingerCtx->_Play( fadein /*,AkTimeConv::ToShortRange( pRecord->iStartTime - iCurrentTime )*/ );
				}
				
				// Stop:
				AkInt64 iStopTime = pRecord->AbsoluteStopTime();
				if ( in_iCurrentTime <= iStopTime 
					&& iCurrentFrameEnd > iStopTime )
				{
					TransParams fadeOut;
					fadeOut.TransitionTime = 0;
					pRecord->pStingerCtx->_Stop( fadeOut, AkTimeConv::ToShortRange( iStopTime - in_iCurrentTime ) );
				}

				// Process.
				if ( pRecord->pStingerCtx->RequiresProcessing() )
				{
					// Use cutoff with stinger context: we just want to hear the first segment.
					// Cutoff time is inclusive, hence the +1. This means that contexts that start with
					// one or many segments with length 0, they will all be heard, including the first
					// one that has a length > 0...
					AkCutoffInfo cutoffInfo;
					cutoffInfo.Set( pRecord->iSyncTime + 1 );
					pRecord->pStingerCtx->Process( in_iCurrentTime, in_uFrameDuration, cutoffInfo );

					if ( pRecord->pStingerCtx->IsStopping() )
					{
						// Stopped. Release stinger context.
						pRecord->pStingerCtx = NULL;
					}
				}
			}
			++it;
		}
		else
		{
			// Destroy stinger record after its stinger has been released and its "don't repeat time" occurred in the 
			// past (even <nothing> stingers have a "don't repeat time" that correspond to their sync point).
			if ( in_iCurrentTime > pRecord->AbsoluteDontRepeatTime() )
			{
				it = m_listPendingStingers.Erase( it );
				AkDelete( g_DefaultPoolId, pRecord );
			}
			else
			{
				// Keep record until "don't repeat time" is exhausted.
				++it;
			}		
		}
	}
}

void CAkMatrixSequencer::RemoveAllPendingStingers()
{
    PendingStingersList::IteratorEx it = m_listPendingStingers.BeginEx();
    while ( !m_listPendingStingers.IsEmpty() )
    {
		AkStingerRecord * pRecord = (*it);
		AKASSERT( !pRecord->pStingerCtx || pRecord->pStingerCtx->IsStopping() );

		it = m_listPendingStingers.Erase( it );
		AkDelete( g_DefaultPoolId, pRecord );
    }
}

void CAkMatrixSequencer::ClearStingerRecord( 
    AkStingerRecord *	in_pStingerRecord	// Stinger record to clear.
	)
{
	PendingStingersList::Iterator it = m_listPendingStingers.FindEx( in_pStingerRecord );
    if ( it != m_listPendingStingers.End() )
    {
        AkStingerRecord * pRecord = (*it);

		// Clear its trigger ID so that it does not prevent this trigger to be rescheduled. 
		// It will be garbage-collected at next or until reverted stinger stops playing.
		pRecord->triggerID = AK_INVALID_UNIQUE_ID;
    }
}

// Delayed states management:
// ----------------------------------------

// See if someone in the hierarchy of the current segment (or the next segment if it playing) 
// is registered to in_stateGroupID, and if it is not immediate.
// Returns the absolute number of samples in which state change should be effective.
// Also, returns 
// the segment look-ahead index which responded to the state group 
// (0=current segment, 1=next segment, etc.);
// Sync time relative to the owner context.
AkInt64 CAkMatrixSequencer::QueryStateChangeDelay( 
    AkStateGroupID  in_stateGroupID,
    AkUInt32 &      out_uSegmentLookAhead,  // returned segment look-ahead index which responded to the state group.
	AkInt64 &		out_iSyncTime			// sync time relative to owner
    )
{
	AkInt64 iCurrentTime = GlobalToOwnerTime( Now() );

	CAkScheduleWindow window( m_pOwner );
	if ( !window.IsValid() )
		return 0;
	
    // Ask node of current segment if it exists.
    CAkSegmentCtx * pCtx = window.GetScheduledItem()->SegmentCtx();

    AkDelayedStateHandlingResult eFirstSegmentResult = AK_DelayedStateNotHandled;
	out_uSegmentLookAhead = 0;

    if ( pCtx &&
		 pCtx->IsPlaying() )
    {
        // There is a segment context. Get the earliest state change position, given the context's
        // current position. 
        eFirstSegmentResult = GetEarliestStateSyncTime( window, 
                                                        in_stateGroupID, 
                                                        out_iSyncTime );

        if ( eFirstSegmentResult == AK_DelayedStateImmediate )
        {
            return 0;   // Return 0. Will be considered as immediate by Renderer.
        }
        else if ( eFirstSegmentResult == AK_DelayedStateSyncFound )
        {
            // Return absolute delay.
            return out_iSyncTime - iCurrentTime;
        }
        // else Sync not found or StateGroup not handled: Check next segment.
    }   

	// Still here. Ask next segment if it exists, AND 
    // if it is playing OR first segment was registered to this state group.
	if ( !window.IsDurationInfinite() )
	{
		m_pOwner->GetNextScheduleWindow( window );
	
		CAkSegmentCtx * pNextCtx = window.GetScheduledItem()->SegmentCtx();
		if ( !pNextCtx )
		{
			// Next segment is <nothing>. 
			// If previous context was registered to this state and could not find a sync position, 
			// state change will sound best at sync point.
			// Note: attach state change action on current - previous - bucket (leave out_uSegmentLookAhead=0),
			// if it is playing. (State change action cannot be attached to empty buckets).
			if ( AK_DelayedStateCannotSync == eFirstSegmentResult 
				&& pCtx 
				&& pCtx->IsPlaying() )
			{
				out_iSyncTime = window.StartTime();
				return out_iSyncTime - iCurrentTime;
			}
			return 0;
		}
	    
		if ( eFirstSegmentResult != AK_DelayedStateNotHandled || // 1st segment registered to state group.
			 pNextCtx->IsPlaying() )
		{
			// Inspecting next segment. Last chance to handle it: we do not look-ahead further.
			out_uSegmentLookAhead = 1;

			AkInt64 iAbsoluteDelay;
			AkDelayedStateHandlingResult eHandlingResult = GetEarliestStateSyncTime( window, 
																					 in_stateGroupID, 
																					 out_iSyncTime );
			switch ( eHandlingResult )
			{
			case AK_DelayedStateImmediate:
			case AK_DelayedStateNotHandled:
				if ( eFirstSegmentResult == AK_DelayedStateNotHandled )
				{
					// If first segment does not handle this state group, change immediately.
					iAbsoluteDelay = 0;
				}
				else
				{
					// Otherwise, it will sound best at sync.
					AKASSERT( eFirstSegmentResult == AK_DelayedStateCannotSync );
					out_iSyncTime = window.StartTime();
					iAbsoluteDelay = out_iSyncTime - iCurrentTime;
				}
				break;

			case AK_DelayedStateSyncFound:
				// Return absolute delay.
				// Convert sync time relative to iterator.
				iAbsoluteDelay = out_iSyncTime - iCurrentTime;
				break;

			case AK_DelayedStateCannotSync:
				//if ( eFirstSegmentTriggerResult == AK_TriggerNotHandled )
					// If first segment does not handle this state group, change immediately.

					// Otherwise, too bad, also change immediately...
				iAbsoluteDelay = 0;
				break;

			default:
				AKASSERT( !"Invalid delayed state change handling return value" );
				iAbsoluteDelay = 0;
			}

			return iAbsoluteDelay;
		}
	}
    
    // First segment did not answer, and next segment was not asked. Change immmediately.
    return 0;
}

// Handle delayed state change. Compute and schedule state change in sequencer.
AKRESULT CAkMatrixSequencer::ProcessDelayedStateChange(
    void *          in_pCookie,
    AkUInt32        in_uSegmentLookAhead,	// handling segment index (>=0).
	AkInt64 		in_iSyncTime			// Sync time relative to owner context.
    )
{
	// Get proper window.
	CAkScheduleWindow window( m_pOwner );
	if ( !window.IsValid() )
		return AK_Fail;
	
	// Advance window over the required number of segments. This must succeed because this was 
	// just determined a moment ago in QueryStateChangeDelay().
    while ( in_uSegmentLookAhead > 0 )
    {
		AKASSERT( !window.IsDurationInfinite() );
		m_pOwner->GetNextScheduleWindow( window );
        --in_uSegmentLookAhead;
    }

    // There must be a segment context attached to this bucket, otherwise we wouldn't have required a delayed state change.
    AKASSERT( window.GetScheduledItem()->SegmentCtx() );

    // Create action.
    AkAssociatedAction * pStateChangeAction = AkNew( g_DefaultPoolId, AkAssociatedAction );
	if ( !pStateChangeAction )
		return AK_Fail;

    pStateChangeAction->eActionType = AssocActionTypeState;
    pStateChangeAction->iRelativeTime = window.ToItemTime( in_iSyncTime );
    pStateChangeAction->pStateChangeCookie = in_pCookie;
    
    window.GetScheduledItem()->AttachAssociatedAction( pStateChangeAction );

	return AK_Success;
}

// Helpers for state management.

// If at least one node of the segment's hierarchy is registered to the state group,
// - returns AK_TriggerImmediate if it requires an immediate state change,
//  if it also requires a delayed state processing, 
//  - returns AK_TriggerSyncFound if it was able to schedule it,
//  - returns AK_TriggerCannotSync if it was not.
// Otherwise, 
// - returns AK_TriggerNotHandled.
// Returned delay is relative to now (segment's position).
CAkMatrixSequencer::AkDelayedStateHandlingResult CAkMatrixSequencer::GetEarliestStateSyncTime( 
    const CAkScheduleWindow & in_window, 
    AkStateGroupID  in_stateGroupID, 
    AkInt64 &       out_iSyncTime	// Returned sync time relative to owner context.
    )
{
    out_iSyncTime = 0;

    // Ask this context's segment node for all its hierarchy's state change syncs for that state group.
    AkDelayedStateHandlingResult eHandlingResult;
    CAkParameterNodeBase::CAkStateSyncArray stateSyncs;
	AKASSERT( in_window.GetNode() );
    in_window.GetNode()->GetStateSyncTypes( in_stateGroupID, &stateSyncs );
    
    CAkParameterNodeBase::StateSyncArray & arStateSyncs = stateSyncs.GetStateSyncArray();
    
    if ( arStateSyncs.Length() > 0 )
    {
        // Array has syncs. At least someone is registered to this state group.

        // Now that we gathered the state syncs, we need to compute the earliest time to sync.
        CAkMusicNode::StateSyncArray::Iterator it = arStateSyncs.Begin();

        // Hierarchy is registered to that state group. 
        if ( (*it) == SyncTypeImmediate ) 
        {
            AKASSERT( arStateSyncs.Length() == 1 );
            // Immediate: leave now.
            eHandlingResult = AK_DelayedStateImmediate;
        }
        else
        {
             // Delayed processing is required.
            eHandlingResult = AK_DelayedStateCannotSync;

            // Find earliest sync position.
            while ( it != arStateSyncs.End() )
            {
                AKASSERT( (*it) != SyncTypeImmediate );                

                AkInt64 iSyncTime;
				AkUniqueID uCueFilter = 0;// Cue filter hash not supported on state changes.
                if ( in_window.FindSyncPoint( 
						GlobalToOwnerTime( Now() ),	// no time constraint.
						(*it),	// sync type.
						uCueFilter,		
						false,	// false = allow sync at entry marker.
						false,	// false = fail if !Immediate rule on <nothing>.
						iSyncTime ) == AK_Success && 
                    ( eHandlingResult == AK_DelayedStateCannotSync ||
                    iSyncTime < out_iSyncTime ) )
                {
                    out_iSyncTime = iSyncTime;
                    eHandlingResult = AK_DelayedStateSyncFound;
                }

                ++it;
            }
        }
    }  
    else
    {
        // Hierarchy is not registered to that state group. 
        eHandlingResult = AK_DelayedStateNotHandled;
    }

    stateSyncs.Term();

    return eHandlingResult;
}

void CAkMatrixSequencer::ProcessMusicNotifications(
	AkInt64		in_iCurrentTime,	// Current owner time.
	AkUInt32	in_uFrameDuration	// Number of frames to process.
	)
{

	CAkScheduleWindow window( m_pOwner );
	if ( !window.IsValid() )
		return;

	AkUInt32 uMusicSyncFlags = GetMusicSyncFlags();

	do
	{
		window.NotifyMusicCallbacks( in_iCurrentTime, in_uFrameDuration, uMusicSyncFlags, PlayingID() );
		if ( window.IsDurationInfinite() 
			|| window.DurationLeft( in_iCurrentTime ) >= in_uFrameDuration )
		{
			break;
		}
		else
		{
			m_pOwner->GetNextScheduleWindow( window );
		}
	}
	while ( true );
}
