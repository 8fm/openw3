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

#include "stdafx.h"
#include "AkScheduleWindow.h"
#include "AkMusicSegment.h" 
#include "AkSegmentChain.h" 

#define AK_LLONG_MAX	((AkInt64)(9223372036854775807ULL))

CAkScheduleWindow::CAkScheduleWindow( 
	bool				in_bFirstItemOnly
	)
: m_uLevel( 0 )
, m_uWindowDuration( 0 )
, m_bIsDurationInfinite( true )
, m_bInvalidChain( false )
, m_bIsActiveSegment( false )
, m_bFirstItemOnly( in_bFirstItemOnly )
{
}

CAkScheduleWindow::CAkScheduleWindow( 
	CAkMatrixAwareCtx * in_pCreatorCtx,
	bool				in_bFirstItemOnly
	)
: m_uLevel( 0 )
, m_uWindowDuration( 0 )
, m_bIsDurationInfinite( true )
, m_bInvalidChain( false )
, m_bIsActiveSegment( false )
, m_bFirstItemOnly( in_bFirstItemOnly )
{
	// Call GetNextScheduleWindow() on creator context to place the window over the first valid item.
	// Leaves (chain contexts) return the item that corresponds to the current time (default), OR 
	// just the first item according to the mode (in_bFirstItemOnly). 
	in_pCreatorCtx->GetNextScheduleWindow( *this );
	m_bIsActiveSegment = !in_bFirstItemOnly;	// At this point, the window is looking at the active segment LX
}

CAkScheduleWindow::~CAkScheduleWindow()
{
}

// Get music segment node associated with this scheduling window, and its parent.
// The segment may be NULL, if the window points on <nothing>, or on the trailing part
// of a finite playlist or segment. Since a <nothing> context may only exist as a child
// of a switch container, at least the segment node or the parent node is defined.
CAkMusicSegment * CAkScheduleWindow::GetNode( 
	CAkMusicNode ** out_ppParentNode // Returned parent node. Pass NULL to ignore.
	) const
{
	AKASSERT( IsValid() );

	CAkMusicSegment * pSegment;
    if ( GetScheduledItem()->SegmentCtx() )
	{
        pSegment = GetScheduledItem()->SegmentCtx()->SegmentNode();
		if ( out_ppParentNode )
			*out_ppParentNode = static_cast<CAkMusicNode*>( pSegment->Parent() );
	}
    else
    {
		pSegment = NULL;
		if ( out_ppParentNode )
		{
			*out_ppParentNode = GetChainCtx()->Node();	// Can be a segment, a sequence, or "nothing" (used by switch containers).
			if ( !(*out_ppParentNode) )
			{
				// If the chain is a null chain, get the node of its parent (must be a SwitchCtx, must succeed).
				CAkMatrixAwareCtx * pParentCtx = static_cast<CAkMatrixAwareCtx*>( GetChainCtx()->Parent() );
				AKASSERT( pParentCtx );
				*out_ppParentNode = pParentCtx->Node();
			}
		}
    }
    AKASSERT( !out_ppParentNode || (*out_ppParentNode) || pSegment );
    return pSegment;
}

// Return window's position relative to the creator context's sync time.
AkInt64 CAkScheduleWindow::StartTime() const
{
	return StartTimeRelativeToLevel( 0 );
}


// Find a valid sync point within a window given a time constraint and a synchronization rule.
// Times are expressed in the referential of the calling context (the creator of this object).
// Returns AK_Success if a sync point could be found within this window, false otherwise.
AKRESULT CAkScheduleWindow::FindSyncPoint(
	AkInt64			in_iMinTime,			// Minimum time before finding a valid sync point (creator context local time).
	AkSyncType      in_eSyncType,			// Sync rule.
	AkUniqueID &	io_uCueFilter,			// Source cue filter, returned as the selected cue.
	bool            in_bDoSkipEntryCue,		// If true, will not consider Entry marker (returned position will be exclusively greater than 0).
	bool			in_bSucceedOnNothing,	// If true, will return a valid position if window points on <nothing>. Otherwise, will succeed only if SyncType is Immediate.
	AkInt64 &		out_iSyncTime			// Returned sync time (creator context local time).
	) const
{
	AKASSERT( IsValid() );

	// Compute time offset of window relative to context.
	AkInt64 iWindowTimeOffset = StartTime();

	// Clamp the minimal time to this window's start time. If it is smaller, it means that there 
	// is no constraint.
	if ( in_iMinTime < iWindowTimeOffset )
		in_iMinTime = iWindowTimeOffset;

	// Express it in terms of segment position. 
	AkInt64 iMinSegmentPosition = CtxTimeToSegmentPosition( in_iMinTime );
	
	AKRESULT eResult;
	AkInt64 iSegmentSyncPos;
	// Find sync point based on segment properties if this item points to a segment and the 
	// chain is valid. If it is invalid (due to memory failure), the sync point will be 
	// computed as if this item was <nothing>.
	if ( GetScheduledItem()->SegmentCtx() 
		&& !m_bInvalidChain )
	{
		CAkMusicSegment * pSegmentNode = GetScheduledItem()->SegmentCtx()->SegmentNode();
		AKASSERT( pSegmentNode );

		// Note: iMinSegmentPosition is positive and relative to this window, so unless the segment is 
		// more than 12 hours long, it fits into an int32.	
		AKASSERT( iMinSegmentPosition >= 0 );
		AkUInt32 uMinSegmentPosition = AkTimeConv::ToShortRange( iMinSegmentPosition );
		AkUInt32 uSegmentSyncPos;
		eResult = pSegmentNode->GetExitSyncPos(
			uMinSegmentPosition,
			in_eSyncType,
			io_uCueFilter,
			in_bDoSkipEntryCue,
			uSegmentSyncPos );
		iSegmentSyncPos = uSegmentSyncPos;
	}
	else
	{
		// Only Immediate sync can be scheduled over empty items.
		io_uCueFilter = AK_INVALID_UNIQUE_ID;
		if ( in_bSucceedOnNothing ||
			 in_eSyncType == SyncTypeImmediate )
		{
			iSegmentSyncPos = iMinSegmentPosition;
			eResult = AK_Success;
		}
		else
			eResult = AK_Fail;
	}

	if ( eResult == AK_Success )
	{
		// Convert segment position back into context time.
		out_iSyncTime = SegmentPositionToCtxTime( iSegmentSyncPos );

		// Check against window length.
		if ( !IsDurationInfinite() && out_iSyncTime > ( iWindowTimeOffset + (AkInt64)Duration() ) )
		{
			// Sync does not occur within this window.
			eResult = AK_PartialSuccess;
		}
	}

	return eResult;
}

// True if given time falls flush on current item's exit cue.
bool CAkScheduleWindow::IsAtExitCue( 
	AkUInt64		in_uTime		// Time in the referential of the creator context.
	) const
{
	AKASSERT( IsValid() );
	if ( !GetScheduledItem()->SegmentCtx() )
		return false;	// <nothing>: no exit cue.
	AKASSERT( GetNode() );

	return ( ToSegmentPosition( in_uTime ) == GetNode()->ActiveDuration() );
}

// Cancel actions associated to the item pointed by the current window after in_iSyncTime. 
// These actions are pushed into io_arCancelledActions for rescheduling.
// Note: Stinger actions attached exactly at in_iSyncTime are cancelled; state change actions
// attached exactly at in_iSyncTime are not.
void CAkScheduleWindow::CancelActionsAfterTransitionSyncPoint(
	AssociatedActionsList & io_listCancelledActions,	// List in which cancelled actions are pushed.
	AkInt64			in_iSyncTime			// Creator context local time after which actions are cancelled.
	)
{
	AKASSERT( IsValid() );

	// Convert context time into item time.
	AkInt64 iItemTime = in_iSyncTime - (AkInt64)StartTime();
	GetScheduledItem()->PopAssociatedActionsToRescheduleAfterTransitionSyncPoint( io_listCancelledActions, iItemTime );
}

void CAkScheduleWindow::NotifyMusicCallbacks( 
	AkInt64		in_iCurrentTime,	// Current time (creator context local time).
	AkUInt32	in_uFrameDuration,	// Range within which music events need to be notified.
	AkUInt32	in_uMusicSyncFlags,	// Flags,
	AkPlayingID in_playingID		// Playing ID.
	) const
{
	AKASSERT( IsValid() );
	if ( GetScheduledItem()->SegmentCtx() )
	{
		GetScheduledItem()->NotifyMusicCallbacks( 
			ToSegmentPosition( in_iCurrentTime ),
			in_uFrameDuration,
			in_uMusicSyncFlags,
			in_playingID );
	}
}

// Interface for chain contexts: Set a new scheduled item.
void CAkScheduleWindow::SetScheduledItem( const AkScheduledChain::SelfContainedIter & in_item )
{
	m_bInvalidChain = !in_item.pCtx->IsValid();
	AKASSERT( in_item.IsSet() );
	m_itScheduledItem = in_item;
	m_bIsActiveSegment = false;
}

// Interface for switch contexts: Set a new branch object. The stack is popped and pushed consequently.
void CAkScheduleWindow::SetBranch( const AkTransQueueIter & in_itTrans )
{
	// A switch context calls us, and it should already have incremented the level.
	AKASSERT( m_uLevel > 0 );
	AkUInt32 uBranchLevel = m_uLevel - 1;

	AKASSERT( uBranchLevel <= m_arBranchStack.Length() );
	if ( uBranchLevel == m_arBranchStack.Length() )
	{
		// Adding a new branch on top of the stack.
		// Clear top of the stack.
		m_itScheduledItem.Reset();
		m_arBranchStack.AddLast( in_itTrans );
	}
	else if ( m_arBranchStack[uBranchLevel] != in_itTrans )
	{
		// Flush stack from current level to the end and insert the new item.
		m_itScheduledItem.Reset();
		AkInt32 iFlush = m_arBranchStack.Length() - 1;
		while ( iFlush >= (AkInt32)uBranchLevel )
		{
			m_arBranchStack.RemoveLast();
			--iFlush;
		}
		m_arBranchStack.AddLast( in_itTrans );
	}
	// else // Item set at this level is the same. Do nothing.
}

AkTransQueueIter CAkScheduleWindow::GetBranch()
{
	// A switch context calls us, and it should already have incremented the level.
	AKASSERT( m_uLevel > 0 );
	AkUInt32 uBranchLevel = m_uLevel - 1;

	AKASSERT( uBranchLevel <= m_arBranchStack.Length() );
	if ( uBranchLevel == m_arBranchStack.Length() )
	{
		// Not set at this level yet. 
		AkTransQueueIter itEnd;
		itEnd.pItem = NULL;
		return itEnd;
	}
	else
	{
		return m_arBranchStack[uBranchLevel];
	}
}

// Set window duration (samples).
void CAkScheduleWindow::SetDuration( AkUInt64 in_uDuration, bool in_bInfinite )
{
	m_uWindowDuration = in_uDuration;
	m_bIsDurationInfinite = in_bInfinite;
}

AkInt64 CAkScheduleWindow::StartTimeRelativeToCurrentLevel() const
{
	// A switch context calls us, and it should already have incremented the level.
	AKASSERT( m_uLevel > 0 );
	return StartTimeRelativeToLevel( m_uLevel - 1 );
}

AkInt64 CAkScheduleWindow::StartTimeRelativeToLevel( AkUInt32 in_uLevel ) const
{
	if ( IsValid() )
		return ChainCtxTimeRelativeToLevel( in_uLevel ) + m_itScheduledItem.GetScheduledItemTime();
	return AK_LLONG_MAX;	// Invalid window: return infinite start time.
}

AkInt64 CAkScheduleWindow::ChainCtxTimeRelativeToLevel( AkUInt32 in_uLevel ) const
{
	AKASSERT( IsValid() );

	AkUInt32 uNumLevels = m_arBranchStack.Length();
	AKASSERT( in_uLevel <= uNumLevels );

	AkInt64 iCtxSyncTime = 0;
	while ( in_uLevel < uNumLevels )
	{
		iCtxSyncTime += (*m_arBranchStack[in_uLevel])->SyncTime();
		++in_uLevel;
	}
	return iCtxSyncTime;
}
