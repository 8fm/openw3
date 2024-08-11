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
// AkMatrixSequencer.h
//
// Multi-chain, branchable segment sequencer.
//
//////////////////////////////////////////////////////////////////////
#ifndef _MATRIX_SEQUENCER_H_
#define _MATRIX_SEQUENCER_H_

#include "AudiolibDefs.h"
#include "AkMusicStructs.h"
#include "PrivateStructures.h"
#include "AkSegmentCtx.h"
#include "AkStateMgr.h"
#include "AkMatrixAwareCtx.h"

#include <AK/MusicEngine/Common/AkMusicEngine.h>
#include <AK/Tools/Common/AkArray.h>

class CAkScheduleWindow;


struct AkSegmentPlayCmd
{
    AkInt32         iRelativeTime;
    AkMusicFade     fadeParams;
	
    AkSegmentPlayCmd()
    :iRelativeTime( 0 )
    {
		// Fade parameters are always ignored when transition time is 0.
		fadeParams.transitionTime = 0;
	}
};

struct AkSegmentStopCmd
{
    AkInt32         iRelativeTime;
    TransParams     transParams;
    
    AkSegmentStopCmd()
    :iRelativeTime( 0 )
    { }
};

// Additionnal actions associated to buckets: Used for stingers and states.
enum AkAssociatedActionType
{
    AssocActionTypeStinger,
    AssocActionTypeState
};

class AkStingerRecord;

struct AkAssociatedAction
{
    AkInt64                 iRelativeTime;
    AkAssociatedActionType  eActionType;
    union
    {
        AkStingerRecord *   pStingerRecord;
        void *              pStateChangeCookie;
    };

	AkAssociatedAction * pNextLightItem;
};

typedef AkListBareLight<AkAssociatedAction> AssociatedActionsList;


class CAkScheduledItem
{
public:
    CAkScheduledItem(
        AkInt64			in_LocalTime,
        CAkSegmentCtx * in_pSegment
        );
    ~CAkScheduledItem();

    // Getters.
    // Scheduled time of this item relative to the beginning of the chain.
    inline AkInt64 Time() { return m_iLocalTime; }

    // Process: Process actions and referenced segment if applicable. 
	// To be called from segment chain once per audio frame.
    void Process(
		AkInt64		in_iCurrentTime,		// Current time in the time coordinates of the caller (chain).
		AkUInt32	in_uFrameDuration,		// Number of samples to process. 
		bool		in_bSkipPlay			// If true, skip play command.
        );

    bool CanDestroy();
	void Destroy();	

    // List bare public link.
    CAkScheduledItem *  pNextItem;

	// Access low-level segment context.
	CAkSegmentCtx * SegmentCtx() const { return m_pSegment; }

	// Returns segment's active duration (0 if no segment).
	AkInt32 SegmentDuration();
    
	// Music queries.
	AKRESULT GetInfo(
		AkInt32			in_iSegmentPosition,	// Segment position.
		AkSegmentInfo &	out_uSegmentInfo		// Returned info.
		);
    
    // Prepares segment context (if applicable), modifies associated Play action time to look-ahead.
	// The segment will be ready to play at the specified start position (relative to EntryCue). They might however
	// be set to begin earlier, in case in_iFadeOffset is negative, but in this case, the returned preparation
	// time takes this duration into account.
	// Returns the required time needed before the synchronization point (can be negative! with a positive fade offset).
	AkInt32 Prepare( 
		AkUInt32	in_uSyncPosition,	// Position within the item (relative to segment Entry Cue) where sync should occur.
		AkInt32		in_iStartPosition	// Position where playback should start (relative to segment Entry Cue).
		);

	// Notifications from low-level segment ctx.
	//
	// Context stopped:
    // Cancel play/stop commands and executes all other associated actions.
	// Detach from low-level segment.
	void OnStopped();

	// Context paused:
	// Execute delayed state changes: they must not be held while music is paused (WG-20167).
	void OnPaused();

	// Interface for low-level segment contexts: detach when stopped.
	inline void Detach()
	{
		m_pSegment->SetOwner( NULL );
		m_pSegment = NULL;
	}

	// Get play time relative to this item.
	inline AkInt32 GetPlayTime() { return m_cmdPlay.iRelativeTime; }

	// Commands scheduling.
    // Play:
	void SetFadeIn(
        AkTimeMs                in_iTransDuration,
        AkCurveInterpolation    in_eFadeCurve,
        AkInt32                 in_iFadeOffset
        );

	// Stop:
    void AttachStopCmd(
        AkTimeMs                in_iTransDuration,
        AkCurveInterpolation    in_eFadeCurve,
        AkInt32                 in_iRelativeTime
        );

    // Modify Stop Cmd time: perform a straight stop at either the Exit Cue or end of Post-Exit.
    // Note. Stop command is left untouched if a fade out is already defined.
    void ForcePostExit(
        bool                    in_bPlayPostExit    // Move stop at the end of Post-Exit if True, and Exit Cue if false.
        );

	inline bool WasPlaybackSkipped() 		{ return m_bWasPlaybackSkipped; }

	// Stop a segment based on how much time it has played already. Stopped immediately
	// if it has not been playing yet, or faded out by the amount of time it has been playing.
	void CancelPlayback( 
		AkInt64		in_iCurrentTime		// Current time in the time coordinates of the caller (chain).
		);

	// Attach an associated action to this item (stinger, state). 
    void AttachAssociatedAction(
        AkAssociatedAction * in_pAction
        );

	// Removes all associated actions that are scheduled to occur after in_iMinActionTime
	// and pushes them into io_arCancelledActions.
	// Note: Stinger actions attached exactly at in_iMinActionTime are removed; state change actions
	// attached exactly at in_iMinActionTime are not.
	void PopAssociatedActionsToRescheduleAfterTransitionSyncPoint(
        AssociatedActionsList & io_listCancelledActions,
        AkInt64 in_iMinActionTime		// Time (relative to this item's time).
        );

	// Notify music callbacks related to segment node pointed by this item.
	void NotifyMusicCallbacks( 
		AkInt32  in_iSegmentPosition,	// Segment position relative to the entry cue.
		AkUInt32 in_uFrameDuration,		// Range within which music events need to be notified
		AkUInt32 in_uNotifFlags,		// Flags,
		AkPlayingID in_playingID		// Playing ID.
		) const;

#ifndef AK_OPTIMIZED
	// Returns the segment node ID (0 if <nothing>).
	AkUniqueID		SegmentID();
#endif

protected:

	inline AkInt64 ParentToLocalTime( AkInt64 in_iTime ) { return in_iTime - m_iLocalTime; }

	// Helper: Stop a segment based on how much time it has played already. Must be playing.
	void _CancelPlayback( 
		AkInt32		in_iCurrentTime		// Current local time.
		);

private:
	// Scheduled time of this item relative to the beginning of the chain.
	AkUInt64				m_iLocalTime;

	// For stingers and state changes.
    AssociatedActionsList	m_listAssociatedActions;

	CAkSmartPtr<CAkSegmentCtx> m_pSegment;

    AkSegmentPlayCmd        m_cmdPlay;
    AkSegmentStopCmd        m_cmdStop;

	AkUInt16				m_bCmdPlayPending		:1;
	AkUInt16				m_bCmdStopPending		:1;
	AkUInt16				m_bWasPlaybackSkipped	:1;
};

// Stinger record keeping.
class AkStingerRecord
{
public:
	AkStingerRecord() {}
	~AkStingerRecord() { pStingerCtx = NULL; }

	inline AkInt64 AbsoluteDontRepeatTime() { return iSyncTime + uDontRepeatTime; }
	inline AkInt64 AbsoluteStartTime() { return iSyncTime - iLookAheadDuration; }
	inline AkInt64 AbsoluteStopTime() { return iSyncTime + iRelativeStopTime; }

	AkStingerRecord *			pNextLightItem;		// List bare light sibling.
	
	CAkSmartPtr<CAkMatrixAwareCtx>  pStingerCtx;    // Stinger context.

	AkTriggerID                 triggerID;
	AkUniqueID	                segmentID;
	AkInt64                     iSyncTime;			// Sync time of stinger (owner context time reference).
	AkUInt32					uDontRepeatTime;    // Absolute time before which this trigger cannot be re-played.

	// Ignored when no stinger context:
	AkInt32						iLookAheadDuration;	// Look-ahead duration of stinger context. Start time is thus iSyncTime - iLookAheadDuration.
	AkInt32						iRelativeStopTime;  // Stop command offset relative to sync time.

	bool						bCanBeRescheduled;	// True when it can be rescheduled on path change: 
		// applies to stingers which have look-ahead property and were scheduled in active segment.
};

class CAkMatrixSequencer : public IAkTriggerAware
{
public:
    CAkMatrixSequencer(
        CAkMatrixAwareCtx * in_pOwner,
		UserParams &    in_rUserparams,
		CAkRegisteredObj *  in_pGameObj
        );
    ~CAkMatrixSequencer();

    // Used by Renderer:
    //
    void Execute( AkUInt32 in_uNumFrames );
	inline void Advance() { m_uTime += m_uCurTimeWindowSize; }
	
	AkInt32 GetCurSegmentPosition();

	inline AkUInt64 Now() const { return m_uTime; }
	inline AkUInt32 CurTimeWindowSize() const { return m_uCurTimeWindowSize; }

	// Time coordinate conversions.
	AkInt64 GlobalToOwnerTime( AkInt64 in_iTime );
	AkInt64 OwnerToGlobalTime( AkInt64 in_iTime );
	

    // Shared object accross a MatrixAware context hierarchy.
    inline CAkRegisteredObj * GameObjectPtr() { return m_pGameObj; }
    inline AkPlayingID		  PlayingID() { return m_UserParams.PlayingID(); }
    inline UserParams &		  GetUserParams() { return m_UserParams; }

	// Reschedule provided actions in active chain.
	// Upon returning, all the actions contained in the list are freed and the list is empty.
	void RescheduleCancelledActions( 
		AssociatedActionsList & in_listActions 
		);

	// Stopped notification.
	void OnStopped();

	// Global processing (stingers, notifications, ...). Called from owner context.
	void Process( 
		AkInt64		in_iCurrentTime,	// Current owner time.
		AkUInt32	in_uNumSamples		// Number of samples to process.
		);

	// IAkTriggerAware implementation:
    // ----------------------------------------
    virtual void Trigger( 
        AkTriggerID in_triggerID 
        );


    // Delayed states management:
    // ----------------------------------------
    // See if someone in the hierarchy of the current segment (or the next segment if it playing) 
    // is registered to in_stateGroupID, and if it is not immediate.
    // Returns the absolute number of samples in which state change should be effective.
    // Also, returns 
    // the segment look-ahead index which responded to the state group 
    // (0=current segment, 1=next segment, etc.);
    // the sync time relative to this segment.
    AkInt64 QueryStateChangeDelay( 
        AkStateGroupID  in_stateGroupID,
		AkUInt32 &      out_uSegmentLookAhead,  // returned segment look-ahead index which responded to the state group.
		AkInt64 &		out_iSyncTime			// sync time relative to owner
        );
    // Handle delayed state change. Compute and schedule state change in sequencer.
    AKRESULT ProcessDelayedStateChange(
        void *          in_pCookie,
		AkUInt32        in_uSegmentLookAhead,	// handling segment index (>=0).
		AkInt64 		in_iSyncTime			// Sync time relative to owner context.
        );

	AkUInt32 GetMusicSyncFlags();

private:
	CAkMatrixSequencer(CAkMatrixSequencer&);

	// Process notifications.
	void ProcessMusicNotifications(
		AkInt64		in_iCurrentTime,	// Current owner time.
		AkUInt32	in_uFrameDuration	// Number of frames to process.
		);

    // Stingers management.
    // ---------------------------------------

	// Handle trigger event.
    void HandleTrigger( 
        AkTriggerID in_triggerID, 
        bool in_bCurrentSegmentOnly 
        );

    // Trigger registration.
    void RegisterTriggers();
    void UnregisterTriggers();

	// Process stingers. Dequeues them if necessary.
    void ProcessStingers(
		AkInt64		in_iCurrentTime,	// Current owner time.
		AkUInt32	in_uFrameDuration	// Number of frames to process.
		);

	// Creates a stinger-segment context and addrefs it. Caller needs to release it. 
	// Returns NULL and cleans up if failed.
    CAkMatrixAwareCtx * CreateStingerCtx(
        AkUniqueID in_segmentID,
        AkInt32 & out_iLookAheadDuration
        );
    // Schedule stinger playback.
    AKRESULT ScheduleStingerForPlayback(
        const CAkScheduleWindow & in_window,		// Window over which stinger is scheduled.
		CAkMatrixAwareCtx * in_pStingerCtx,			// Can be NULL (<NOTHING>).
		const CAkStinger *  in_pStingerData,
		AkInt64				in_uSyncTime,			// Stinger sync time relative to owner context.
		AkInt32             in_iLookAheadDuration,
		bool                in_bScheduledInCurrentSegment
        );
	// Can play stinger. False if another stinger instance exists, with same trigger ID and positive don't repeat time.
    bool CanPlayStinger(
		AkTriggerID			in_triggerID
		);
	
    // Remove all pending stingers when top context stopped playing.
    void RemoveAllPendingStingers();

	// Clear a stinger record when unscheduled. The trigger ID and dont repeat time
	// are cleared so that they get garbage-collected at next processing pass,
	// unless they still hold a playing (stopping) stinger.
    void ClearStingerRecord( 
        AkStingerRecord *	in_pStingerRecord	// Stinger record to clear.
        );

	// Helpers for state management.
    // -----
    
    enum AkDelayedStateHandlingResult
    {
        AK_DelayedStateSyncFound, 
        AK_DelayedStateCannotSync, 
        AK_DelayedStateImmediate,
        AK_DelayedStateNotHandled
    };

	// Get sync time for state change in specified scheduling window.
    // If at least one node of the segment's hierarchy is registered to the state group,
    // - returns AK_TriggerImmediate if it requires an immediate state change,
    //  otherwise if it requires a delayed state processing, 
    //  - returns AK_TriggerSyncFound if it was able to schedule it,
    //  - returns AK_TriggerCannotSync if it was not.
    // Otherwise, 
    // - returns AK_TriggerNotHandled.
    // Returned delay is relative to now (segment's position).
    AkDelayedStateHandlingResult GetEarliestStateSyncTime( 
        const CAkScheduleWindow & in_window, 
		AkStateGroupID  in_stateGroupID, 
		AkInt64 &       out_iSyncTime	// Returned sync time relative to owner context.
        );

private:

	AkUInt64			m_uTime;		// Master time of this sequencer.
	
    CAkMatrixAwareCtx * m_pOwner;

    // Shared object accross a MatrixAware context hierarchy.
    UserParams			m_UserParams;   // User Parameters.
    CAkRegisteredObj*	m_pGameObj;		// CAkRegisteredObj to use to Desactivate itself once the associated game object were unregistered.

	// Stingers.
	// List of active stinger records.
    typedef AkListBareLight<AkStingerRecord> PendingStingersList;
    PendingStingersList m_listPendingStingers;

	// Frame size to be processed. 
	// Needed because there is no "tick" at the end of audio frame processing. 
	// Some logic needs to go back in time during low-level processing.
	AkUInt32			m_uCurTimeWindowSize;	// This value is valid between calls to Execute() and Advance().
};

#endif //_MATRIX_SEQUENCER_H_
