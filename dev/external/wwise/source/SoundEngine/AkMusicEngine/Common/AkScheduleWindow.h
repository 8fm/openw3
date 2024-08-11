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

#ifndef _SCHEDULE_WINDOW_H_
#define _SCHEDULE_WINDOW_H_

#include "AkSegmentChain.h"
#include "AkMusicSwitchCtx.h"
#include <AK/Tools/Common/AkArray.h>
#include "PrivateStructures.h"

class CAkMusicNode;
class CAkMusicSegment;

//-----------------------------------------------------------------------------
// Name: CAkScheduleWindow
// Desc: Time-spanned view of a scheduled item (segment). Used to browse
// through a tree of scheduled items.
//-----------------------------------------------------------------------------
class CAkScheduleWindow
{
public:

	CAkScheduleWindow( 
		CAkMatrixAwareCtx * in_pCreatorCtx,	// Creator context. When window is created, it will point on this context's active scheduled item.
		bool				in_bFirstItemOnly=false	// If true, places the window on the first item of the active chain. Otherwise, the item corresponds to the current time (default).
		);
	// Use this contructor if you don't want to place the window anywhere. (Do not use it until you do).
	CAkScheduleWindow( 
		bool				in_bFirstItemOnly=false	// If true, the window will be placed on the first item of the active chain the first time it is set. Otherwise, the item corresponds to the current time (default).
		);
	~CAkScheduleWindow();

	inline bool IsValid() const { return m_itScheduledItem.IsSet(); }
	inline bool IsActiveSegment() const { return m_bIsActiveSegment; }	// Returns true if window is pointing at active segment.
	
	// Return window's start time relative to the creator context's sync time (always >= 0).
	AkInt64 StartTime() const;

	// Convert a given time local to creator context in terms of position of the segment that is 
	// pointed by this window.
	// Returns 0 if window points to <nothing>.
	inline AkInt32 ToSegmentPosition(
		AkInt64		in_iTime			// Creator context local time.
		) const
	{
		if ( GetScheduledItem()->SegmentCtx() ) 
			return AkTimeConv::ToShortRange( CtxTimeToSegmentPosition( in_iTime ) );
		return 0;
	}

	// Identical to ToSegmentPosition(), except that it works for nothing also. Returned value
	// is long range (int64).
	inline AkInt64 ToItemTime(
		AkInt64		in_iTime			// Creator context local time.
		) const
	{
		return CtxTimeToSegmentPosition( in_iTime );
	}

	// Returns duration left in window at a given time. Call only if window duration is not infinite.
	inline AkUInt64 DurationLeft(
		AkInt64		in_iTime			// Creator context local time.
		)
	{
		AKASSERT( !IsDurationInfinite() );
		AkInt64 iItemTime = ToItemTime( in_iTime );
		if ( iItemTime < (AkInt64)Duration() )
			return (AkUInt64)( (AkInt64)Duration() - iItemTime );
		return 0;
	}

	// Notify all desired music callbacks for the item pointed by this window, for the interval
	// [in_iCurrentTime,in_iCurrentTime+in_uFrameDuration[.
	void NotifyMusicCallbacks( 
		AkInt64		in_iCurrentTime,	// Current time (creator context local time).
		AkUInt32	in_uFrameDuration,	// Range within which music events need to be notified.
		AkUInt32	in_uMusicSyncFlags,	// Flags,
		AkPlayingID in_playingID		// Playing ID.
		) const;

	// Return window's duration.
	inline AkUInt64 Duration() const { return m_uWindowDuration; }
	inline bool IsDurationInfinite() const { return m_bIsDurationInfinite; }

	// Get music segment node associated with this scheduling window, and its parent.
	// The segment may be NULL, if the window points at <nothing>, or to the trailing part
	// of a finite playlist or segment. Since a <nothing> context may only exist as a child
	// of a switch container, at least the segment node or the parent node is defined.
	CAkMusicSegment * GetNode( 
		CAkMusicNode ** out_ppParentNode = NULL	// Returned parent node. Can pass NULL.
		) const;

	// Get scheduled item pointed by the window.
	inline CAkScheduledItem * GetScheduledItem() const { return (*m_itScheduledItem); }
	inline CAkChainCtx * GetChainCtx() const { return m_itScheduledItem.Owner(); }

	// Find a valid sync point within a window given a time constraint and a synchronization rule.
	// Times are expressed in the referential of the creator context (the creator of this object).
	// Returns AK_Success if a sync point could be found within this window, false otherwise.
	AKRESULT FindSyncPoint(
		AkInt64			in_iMinTime,			// Minimum time before finding a valid sync point (creator context local time).
		AkSyncType      in_eSyncType,			// Sync rule.
		AkUniqueID &	io_uCueFilter,			// Source cue filter, returned as the selected cue.
		bool            in_bDoSkipEntryCue,		// If true, will not consider Entry marker (returned position will be exclusively greater than 0).
		bool			in_bSucceedOnNothing,	// If true, will return a valid position if window points on <nothing>. Otherwise, will succeed only if SyncType is Immediate.
		AkInt64 &		out_iSyncTime			// Returned sync time (creator context local time).
		) const;

	// True if sync point is at current item's exit cue.
	bool IsAtExitCue( 
		AkUInt64		in_uSyncTime			// Sync time in the referential of the creator context.
		) const;

	// Cancel actions associated to the item pointed by the current window after in_iSyncTime.
	// Note: Stinger actions attached exactly at in_iSyncTime are cancelled; state change actions
	// attached exactly at in_iSyncTime are not.
	void CancelActionsAfterTransitionSyncPoint(
		AssociatedActionsList & io_listCancelledActions,	// List in which cancelled actions are pushed.
		AkInt64			in_iSyncTime			// (Creator context) time after which actions are cancelled.
		);

	// 
	// Iterating through a chain of windows. Behavior is implemented by contexts.
	// Interface for all contexts.
	//
	// Set window duration (samples).
	void SetDuration( AkUInt64 in_uDuration, bool in_bInfinite );

	//
	// Specific interface for chain contexts.
	//
	// Get current scheduled item. End() if not set.
	AkScheduledChain::SelfContainedIter GetScheduledItemIterator() const { return m_itScheduledItem; }

	// When scheduled item has not been set already, use this to determine if the window should point
	// on the first item of the chain, or if it should correspond to the current time.
	bool FirstItemOnly() { return m_bFirstItemOnly; }

	// Set a new scheduled item.
	void SetScheduledItem( const AkScheduledChain::SelfContainedIter & in_item );
	
	// Clear current scheduled item: window will be invalid and its start time will be infinite.
	inline void Invalidate() { m_itScheduledItem.pItem = NULL; }

	//
	// Specific interface for switch contexts.
	//
	// Get branch item at current level.
	AkTransQueueIter GetBranch();

	// Get start time of the window relative to current switch context level.
	AkInt64 StartTimeRelativeToCurrentLevel() const;

	// Set a new branch object. The stack is popped and pushed consequently.
	void SetBranch( const AkTransQueueIter & in_itTrans );

	// Switch contexts need to increment level before setting branch in order to 
	// ensure branch stack consistency, and decrement it when they are done. 
	class AutoBranchLevel
	{
	public:
		AutoBranchLevel( CAkScheduleWindow & in_wnd )
		: m_wnd( in_wnd )
		{ 
			in_wnd.IncrementLevel();
		}
		~AutoBranchLevel()
		{ 
			m_wnd.DecrementLevel();
		}
	private:
		CAkScheduleWindow & m_wnd;
	};

protected:

	// Helpers.
	//
	inline void IncrementLevel() { ++m_uLevel; }
	inline void DecrementLevel() { AKASSERT( m_uLevel > 0 ); --m_uLevel; }

	// Convert a local time value of the creator context in terms of position relative to the entry cue of the segment
	// that is pointed by this window.
	inline AkInt64 CtxTimeToSegmentPosition( 
		AkInt64		in_iTime
		) const
	{
		AKASSERT( IsValid() );
		return m_itScheduledItem.CtxTimeToSegmentPosition( in_iTime - ChainCtxTimeRelativeToLevel( 0 ) );
	}

	// Convert position relative to the entry cue of the segment that is pointed by this window into
	// a local time of the creator context. 
	AkInt64 SegmentPositionToCtxTime( 
		AkInt64		in_iSegmentPosition
		) const
	{
		AKASSERT( IsValid() );
		return ChainCtxTimeRelativeToLevel( 0 ) + m_itScheduledItem.SegmentPositionToCtxTime( in_iSegmentPosition );
	}

	// Compute this window's start time relative to the context at the given level (level 0 is the creator context).
	AkInt64 StartTimeRelativeToLevel( AkUInt32 in_uLevel ) const;
	// Compute the time of the chain context that owns the item pointed by this window relative to 
	// the context at the given level (level 0 is the creator context). For example, if the creator was a chain context,
	// this function would return 0. If it was a switch context and the current chain context would be directly below,
	// ChainCtxTimeRelativeToLevel( 0 ) would return the chain context's sync time.
	AkInt64 ChainCtxTimeRelativeToLevel( AkUInt32 in_uLevel ) const;

	// Branch item (transition) stack.
	class BranchStack
	{
	public:
		BranchStack() :m_uSize( 0 ) {}
		~BranchStack() {}

		inline AkUInt32 Length() const { return m_uSize; }
		inline const AkTransQueueIter & operator[]( AkUInt32 index ) const
		{
			AKASSERT( index < Length() );
			return m_arBranches[index];
		}
		inline void AddLast( const AkTransQueueIter & in_branch )
		{
			m_arBranches[m_uSize] = in_branch;
			m_uSize++;
		}
		inline void RemoveLast()
		{
			AKASSERT( Length() > 0 );
			--m_uSize;
		}

	private:
		AkTransQueueIter	m_arBranches[AK_MAX_HIERARCHY_DEEP];
		AkUInt32			m_uSize;
	};
	BranchStack			m_arBranchStack;	// Transition stack.
	
	// Top of the stack: item within a chain with its context.
	AkScheduledChain::SelfContainedIter	m_itScheduledItem;
	
	AkUInt32			m_uLevel;			// Current stack level.

	AkUInt64			m_uWindowDuration;	// Length of the (schedulable) segment window in samples.
	bool				m_bIsDurationInfinite;// True if window length is infinite.
	bool				m_bInvalidChain;	// Set to true if duration is set to "infinite" because the chain could not grow due to memory failure.
	bool				m_bIsActiveSegment;	/// LX

	bool				m_bFirstItemOnly;	// If true, window is initially placed on the first item of the active chain. Otherwise, the item corresponds to the current time.
};

#endif // _SCHEDULE_WINDOW_H_
