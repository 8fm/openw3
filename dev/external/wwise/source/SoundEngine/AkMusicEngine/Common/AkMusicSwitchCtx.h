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
// AkMusicSwitchCtx.h
//
// Music Switch Container Context.
//
//////////////////////////////////////////////////////////////////////
#ifndef _MUSIC_SWITCH_CTX_H_
#define _MUSIC_SWITCH_CTX_H_

#include "AkMatrixAwareCtx.h"
#include "AkRegisteredObj.h"
#include "PrivateStructures.h"
#include "AkMusicTransAware.h"
#include "AkSwitchAware.h"
#include "AkMatrixSequencer.h"
#include "AkSegmentChain.h"
#include "AkDecisionTree.h"

#include <AK/Tools/Common/AkSmartPtr.h>
#include <AK/Tools/Common/AkListBare.h>
#include <AK/Tools/Common/AkArray.h>

class CAkMusicSwitchCntr;

//-----------------------------------------------------------------------------
// Name: CAkNothingCtx
// Desc: Chain-based context which represents an empty switch.
//-----------------------------------------------------------------------------
class CAkNothingCtx : public CAkChainCtx
{
public:
    CAkNothingCtx(
        CAkMusicCtx *   in_parent = NULL
        );
    virtual ~CAkNothingCtx();

    AKRESULT Init(
        CAkRegisteredObj *  in_pGameObj,
        UserParams &    in_rUserparams,
        CAkMatrixSequencer * in_pSequencer
        );

	// CAkChainCtx implementation
    // ----------------------------------------------------------
	// Once a "nothing" context is created successfully, it is valid.
	virtual bool IsValid() { return true; }

protected:

	// MusicCtx override. Release.
    virtual void OnStopped();

    // For Music Renderer's music contexts look-up: concrete contexts must return their own node.
    virtual CAkMusicNode * Node();

private:
	bool m_bWasReferenced;
};


//-----------------------------------------------------------------------------
// Name: CAkMusicSwitchTransition
// Desc: Switch transition. The switch context keeps a list of all pending transitions. 
//       (either active or obsolete, but kept alive). 
//-----------------------------------------------------------------------------
struct AkSwitchPlayCmd
{
    AkInt64         iRelativeTime;	// Playback start time relative to switch sync. Occurs within a reasonnable time near sync (defined by transition rules).
    AkMusicFade     fadeParams;
};

struct AkSwitchStopCmd
{
    AkInt64         iRelativeTime;	// Stop time relative to switch sync. Can occur a long time after switch has synched.
    TransParams     transParams;
};

class CAkMusicSwitchTransition
{
public:
    static CAkMusicSwitchTransition * Create(
        CAkMatrixAwareCtx *     in_pDestination
        );

	void Dispose();

	// Destination context.
	inline CAkMatrixAwareCtx * Destination() { return m_pDestCtx; }

	// Get the time at which the sync point of this transition is reached.
	// A transition reaches its sync point when its destination reaches its sync point.
	// The time returned is relative to the context that owns this transition.
	inline AkInt64 SyncTime()
	{
		return Destination()->SyncTime();
	}

	// Transition scheduling.
    void ScheduleToPlay(
		AkInt64					in_iSyncTime,		// Sync time in the context of the owner context.
		const AkMusicFade &		in_fadeParams,		// Volume fade.
        AkInt32					in_iLookAheadTime	// Amount of time ahead sync point where playback should start.
		);

	void ScheduleToStop(
		AkInt64					in_iSyncTime,		// Owner context's sync point, after which segment chains are cutoff (inclusive).
		AkInt64					in_iStopTime,		// Time at which stopping should occur/begin in the context of the owner context.
		AkTimeMs                in_iTransDuration,	// Fade out duration.
        AkCurveInterpolation	in_eFadeCurve,		// Fade curve.
		bool					in_bCutoff			// True in order to set cutoff at sync point. False to skip cutoff mechanism.
		);

	inline bool RequiresProcessing()
	{
		// Requires processing if it has a command pending or if destination requires to be processed.
		return m_bCmdStopPending || Destination()->RequiresProcessing() || m_bCmdPlayPending;
	}

    // Process switch transition: Process destination, execute transition commands.
    void Process(
		AkInt64			in_iCurrentTime,		// Current time relative to the sync time of the owner switch context.
		AkUInt32		in_uNumSamples,			// Number of samples to process.
		AkCutoffInfo &	in_cutoffInfo			// Downstream chain cutoff info.
		);
    
    // Transition reversal.
	bool CanBeCancelled();
    void Cancel();
	bool CanBeRestored();
	void Restore( 
		AkInt64		in_iCurrentTime,			// Current time relative to the sync time of this object.
		AkUInt32	in_uMaxFadeInDuration,		// Max fade-in duration. We want to be at full volume when new transition sync occurs.
		bool		in_bUseMaxFadeInDuration	// Ignore in_uMaxFadeInDuration if this parameter is false.
		);

public:
    
	// List bare sibling.
	CAkMusicSwitchTransition * pNextItem;

	~CAkMusicSwitchTransition();
private:
	
	CAkMusicSwitchTransition(
        CAkMatrixAwareCtx *     in_pDestination
        );

	CAkSmartPtr<CAkMatrixAwareCtx> m_pDestCtx;	// Destination context.

	AkCutoffInfo		m_cutoffInfo;			// Cutoff info for this transition (time relative to owner switch context local time).

	// Play/stop commands.
	AkSwitchPlayCmd     m_cmdPlay;
    AkSwitchStopCmd     m_cmdStop;

	// Flags.
	AkUInt8				m_bCmdPlayPending		:1;
	AkUInt8				m_bCmdStopPending		:1;	// Set when stop command is scheduled, reset when it is executed (beginning of fade if applicable).
	AkUInt8				m_bWasScheduledToStop	:1;	// Set when stop command is scheduled.
};

typedef AkListBare<CAkMusicSwitchTransition> AkTransitionsQueue;
typedef AkTransitionsQueue::Iterator AkTransQueueIter;

class CAkMusicSwitchCtx;

class CAkMusicSwitchMonitor: public CAkSwitchAware
{
public:
	CAkMusicSwitchMonitor(): m_uIdx((AkUInt32)-1), m_pMusicCtx(NULL) {}
	virtual ~CAkMusicSwitchMonitor(){Term();}

	//CAkSwitchAware interface
	virtual void SetSwitch( 
		AkUInt32 in_Switch, 
		CAkRegisteredObj * in_pGameObj = NULL 
		);

	AKRESULT Init( AkUInt32 in_uIdx, CAkMusicSwitchCtx& in_pMusicCtx );
	void Term() {UnsubscribeSwitches();}


	AkUniqueID                  m_targetSwitchID;

	AkUInt32 m_uIdx;
	CAkMusicSwitchCtx* m_pMusicCtx;
};

//-----------------------------------------------------------------------------
// Name: CAkMusicSwitchCtx
// Desc: Music Switch Container Context.
//-----------------------------------------------------------------------------
class CAkMusicSwitchCtx : public CAkMatrixAwareCtx
                         
{
public:
	
    CAkMusicSwitchCtx(
        CAkMusicSwitchCntr *in_pSwitchNode,
        CAkMusicCtx *       in_pParentCtx
        );
    virtual ~CAkMusicSwitchCtx();

    AKRESULT Init(
        CAkRegisteredObj *  in_GameObject,
        UserParams &        in_rUserparams
        );

	// Matrix Aware Context implementation.
    // ----------------------------------------------------------

	// Change playback position.
	// The chain is destroyed and re-prepared at the correct position.
	virtual AKRESULT SeekTimeAbsolute( 
		AkTimeMs & io_position,	// Seek position, in ms, relative to Entry Cue (if applicable). Returned as effective position.
		bool in_bSnapToCue		// True if final position needs to be on a cue (Exit cue excluded).
		);
	virtual AKRESULT SeekPercent( 
		AkReal32 & io_fPercent,	// Seek position, in percentage, between Entry and Exit Cues ([0,1]). Returned as effective position.
		bool in_bSnapToCue		// True if final position needs to be on a cue (Exit cue excluded).
		);

	// MatrixAware::EndInit override. Called by parent (switches): completes the initialization.
	virtual void EndInit();


    // Matrix Aware Context interface
    // ----------------------
    
    // Sequencer access.
    virtual void Process(
		AkInt64			in_iCurrentTime,		// Current time in the time coordinates of the caller (parent).
		AkUInt32		in_uNumSamples,			// Number of samples to process.
		AkCutoffInfo &	in_cutoffInfo			// Downstream chain cutoff info.
		);

	// Prepares the context according to the destination transition rule, if specified. 
	// Otherwise, prepares it at position in_iMinStartPosition. The returned look-ahead time 
	// corresponds to the time needed before reaching the synchronization point.
	// Returns the required look-ahead time. 
	virtual AkInt32 Prepare(
		const AkMusicTransDestRule * in_pRule,	// Transition destination (arrival) rule. NULL means "anywhere", in any segment.
		AkInt32		in_iMinStartPosition,		// Minimum play start position in segment/sequence (relative to first segment entry cue). Actual position may occur later depending on rule.
		AkSeekingInfo * in_pSeekingInfo,		// Seeking info (supersedes in_iMinStartPosition). NULL if not seeking.
		AkUniqueID & out_uSelectedCue,			// Returned cue this context was prepared to. AK_INVALID_UNIQUE_ID if not on a cue.
		AkUniqueID	in_uCueHashForMatchSrc = 0	// Cue which was used in the source segment. Used only if rule is bDestMatchCueName.
		);

	// Context-specific delegate for CAkScheduleWindow. 
	// Switch contexts push their current transition objects (branch) into the window, and ask their destination
	// context to complete the job. They also need to compare with the next transition object in order to determine
	// which branch to use, and possibly shorten the window length if a transition is scheduled during the segment
	// pointed by this window.
	virtual void GetNextScheduleWindow( 
		CAkScheduleWindow & io_window,		// Current schedule window, returned as next schedule window. If io_window had just been instantiated, the active window would be returned.
		bool in_bDoNotGrow = false			// If true, chains are not asked to grow. "False" is typically used for scheduling, whereas "True" is used to check the content of a chain.
		);

	// Call this to refresh the current window's length.
	virtual void RefreshWindow(
		CAkScheduleWindow & io_window
		);

	// Inspect context that has not started playing yet and determine the time when it will start playing,
	// relative to its sync time (out_iPlayTime, typically <= 0). Additionnally, compute the time when it 
	// will start playing and will be audible (out_iPlayTimeAudible, always >= out_iPlayTime).
	virtual void QueryLookAheadInfo(
		AkInt64 &	out_iPlayTime,			// Returned time of earliest play command (relative to context).
		AkInt64 &	out_iPlayTimeAudible	// Returned time of earliest play command that becomes audible. Always >= out_iPlayTime. Note that it may not refer to the same play action as above.
		);
	
	// Interface for parent switch context: trigger switch change that was delayed because of parent transition.
	virtual void PerformDelayedSwitchChange();

    // For Music Renderer's music contexts look-up.
    virtual CAkMusicNode * Node();

	// Query for transition reversal. Asks active chain if it is not too late to start playing again.
	virtual bool CanRestartPlaying();

	// Stop a context based on how much time it has played already. Should be stopped immediately
	// if it has not been playing yet, or faded out by the amount of time it has been playing.
	virtual void CancelPlayback(
		AkInt64 in_iCurrentTime		// Current time in the time coordinates of the caller (parent).
		);

protected:
    // Music Context interface
    // ----------------------

	// Override MusicCtx OnPlayed: Need to play first transition destination.
	virtual void OnPlayed();

    // Override MusicCtx OnStop: Need to release transitions' destination.
    virtual void OnStopped();

    // Helpers.
private:

	AKRESULT Seek( 
		AkSeekingInfo & in_seekInfo,	// Seek position in destination.
		bool in_bSnapToCue				// True if final position needs to be on a cue (Exit cue excluded).
		);

	// Instantiate and schedule child according to current switch the first time, 
	// while this context is idle.
	// Returns AK_Success or AK_Fail. When failing, the children/transitions previously scheduled (if any) are restored.
    AKRESULT SetInitialSwitch(
		bool				in_bNothing = false	// If true, initial child is <nothing> (in_switchID is ignored).
        );

	// Structure for passing custom/special transition info.
	class TransitionInfo
	{
	public:
		TransitionInfo()
			: pSeekingInfo( NULL )
			, bOverrideEntryType( false )
		{}

		inline void OverrideEntryType( AkEntryType in_eEntryType ) { eEntryType = in_eEntryType; bOverrideEntryType = true; }

		AkSeekingInfo *	pSeekingInfo;	// Seeking info. NULL if not seeking.
		bool			bOverrideEntryType;	// True if entry type should be overriden by eEntryType.
		AkEntryType		eEntryType;		// Overriden entry type. Undefined if bOverrideEntryType.		
	};

	// Switch transition: Instantiate and schedule child according to new switch value, 
	// while this context is playing.
	// No return value: when failing, the context is already playing and a new transition will simply not happen.
	void ChangeSwitch(
		const TransitionInfo & in_transitionInfo // Transition info. 
		);
	
	AkUniqueID ResolveAudioNode() const;

	// Schedule all transition actions required for switch change.
    void ScheduleSwitchTransition(
		AkUniqueID			in_destinationID,	// Node ID of destination.
		const TransitionInfo & in_transitionInfo,	// Transition info. 
        CAkMatrixAwareCtx *& io_pNewContext		// New context. Can be destroyed internally if we decide not to schedule it.
        );

    // Schedule a valid music transition: Create a pending switch transition
    // and attach Play and Stop commands.
    void ScheduleTransition(
		AkInt64				in_iCurrentTime,			// Current time.
		CAkMusicSwitchTransition * in_pNewTransition,	// New transition object to schedule.
		const AkMusicTransSrcRule &	in_ruleFrom,		// Source rule (details stopping previous context).
		const AkMusicTransDestRule & in_ruleTo,			// Destination rule (details starting playback of new context).
		CAkScheduleWindow & in_source,					// Scheduling window pointing on the source segment. Warning: changes within this call.
		AkInt32				in_iDestinationLookAhead,	// Required look-ahead time for transition destination.
		AkInt64				in_iSyncPoint,				// Sync point.
		bool				in_bIsFromTransitionCtx,	// True if this transition is scheduled right after a transition context.
		AssociatedActionsList & io_listCancelledActions	// Cancelled scheduled actions to reschedule.
        );

	// Sets the current active switch when a transition destination reaches its sync point.
	// It should be the next transition in the queue.
	// Posts sync notifications.
    void Sync( 
		AkTransQueueIter &	in_itNewTransition	// Iterator to transition which just reached its sync point.
		);

    // Creates next node context.
    CAkMatrixAwareCtx * CreateDestinationContext(
		AkUniqueID in_ID
		);
	CAkMatrixAwareCtx * CreateMusicContext(
		AkUniqueID in_ID
		);

    // Get the appropriate transition rule.
    const AkMusicTransitionRule & GetTransitionRule( 
        const CAkScheduleWindow & in_source,			// Scheduling window pointing on the source segment.
		CAkMatrixAwareCtx *&io_pDestContext,			// Destination context. Can be destroyed within this function (and set to NULL).
		AkUInt32 &			io_uSrcSegmentLookAhead 	// Src look-ahead limit. Passed this limit, use panic rule.
        );

	// Splits rule based on whether it uses a transition segment.
	void SplitRule( 
		const AkMusicTransitionRule &	in_rule,		// Original rule. 
		const TransitionInfo &			in_transitionInfo,	// Transition info. 
		AkMusicTransSrcRule	&			out_ruleFrom,	// Returned "source" rule from original source.
		AkMusicTransDestRule &			out_ruleTo,		// Returned "destination" rule to original destination.
		AkMusicTransDestRule &			out_ruleToTrans,	// Returned "destination" rule to transition context (or original destination if none).
		AkMusicTransSrcRule	&			out_ruleFromTrans	// Returned "source" rule from transition context (or original destination if none).
		);

	// Compute required look-ahead time for transition source-to-trans_segment. It is the most constraining
	// between the look-ahead required for the transition segment and the look-ahead required for the destination.
	AkInt32 ComputeWorstLookAheadTime( 
		CAkMatrixAwareCtx * in_pTransCtx,			// Context of transition segment.
		AkInt32				in_iTransCtxLookAhead,	// Required look-ahead for transition context.
		AkInt32				in_iDestCtxLookAhead	// Required look-ahead for destination context.
		);

	// Compute minimum time for a sync point given a transition rule and the required look-ahead time of the destination context.
	AkInt64 ComputeMinSyncTime(
		const AkMusicTransSrcRule & in_rule,			// Transition rule for source.
		AkInt64				in_iCurrentTime,			// Current time.
		AkInt32				in_iDestinationLookAhead	// Required look-ahead time for destination.
		);

	// Compute minimum time for a sync point given a transition rule and the required look-ahead time of the destination context.
	AkInt64 ComputeWorstMinSyncTime( 
		AkInt64				in_iCurrentTime,			// Current time.
		const AkMusicTransSrcRule & in_rule1,			// Source transition rule of first transition.
		AkInt32				in_iLookAhead1,				// Required look-ahead of first destination.
		CAkMatrixAwareCtx * in_pTransCtx,				// Context of transition segment (first destination).
		const AkMusicTransSrcRule & in_rule2,			// Source transition rule of second transition.
		AkInt32				in_iLookAhead2 				// Required look-ahead of second destination.
		);

	// Child contexts / Transitions management.
	AKRESULT PrepareFirstContext( 
		CAkMatrixAwareCtx * in_pCtx
		);

	// Determines whether a new transition must be enqueued (handles "Continue to play" behavior).
	bool IsSwitchTransitionNeeded(
		AkUniqueID					in_nextNodeID,			// Destination node ID.
		AkSeekingInfo	*			in_pSeekingInfo,		// Seeking info. NULL if seeking not required.
		CAkMusicSwitchTransition *	in_pLastValidTransition	// Transition object whose destination should be checked against in_nextNodeID.
		);

	// Find last (latest) transition in the transition queue that is not cancellable. 
	CAkMusicSwitchTransition * FindLastNonCancellableTransition();

	// Dequeue and return all transitions starting from the one that follows in_pLastValidTransition.
	void DequeueCancellableTransitions( 
		CAkMusicSwitchTransition * in_pLastValidTransition,
		AkTransitionsQueue & out_listCancelledTransitions,
		CAkScheduleWindow & io_wndSource
		);
	
	// Returns true if this context or any of its ascendents has a pending transition.
	bool HasOrAscendentHasPendingTransition();

	// Calls CAkMatrixAwareCtx::PerformDelayedSwitchChange() for all its children.
	void TryPropagateDelayedSwitchChange();

	// Get scheduling window pointed by the destination of the last (latest) scheduled transition.
	void MoveWindowToLastNonCancellableTransition( 
		CAkMusicSwitchTransition *	in_pLastNonCancellableTransition,
		CAkScheduleWindow &			io_window 
		);

	// Create a scheduling window pointing to the transition segment. It is meant to be used to schedule 
	// a transition from the transition segment. The transition object's destination should be the 
	// transition segment context.
	void SetupWindowForTransContext( 
		CAkMusicSwitchTransition *	in_pTransitionToTransCtx,	// Transition object pointing to transition segment context.
		AkInt64						in_uSyncTime,				// Sync time of transition context.
		CAkScheduleWindow &			out_wndTrans				// Returned scheduling window pointing on transition segment.
		);

	inline void SetDelayedSwitchTargetFlag()	{ m_bHasDelayedSwitchID = true; }
	inline bool HasDelayedSwitchTarget()		{ return m_bHasDelayedSwitchID; }
	inline void ClearDelayedSwitchTargetFlag()	{ m_bHasDelayedSwitchID = false; }

	
private:

    CAkMusicSwitchCntr *        m_pSwitchCntrNode;
	AkArray<CAkMusicSwitchMonitor, const CAkMusicSwitchMonitor&, ArrayPoolDefault > m_switchMonitors;

	// Transitions queue.
	AkTransitionsQueue			m_queueTransitions;
    
    // Current active transition object.
	AkTransQueueIter			m_itActiveSwitch;

	// Last transition after which another transition has been enqueued in the frame.
	CAkMusicSwitchTransition *	m_pLastNonCancelledTransitionInFrame;

	AkUInt8						m_bHasDelayedSwitchID	:1;	// Indicates that a switch change (towards m_delayedSwitchID) is waiting for a parent switch to resolve.
	AkUInt8						m_bWasReferenced		:1;

	friend class CAkMusicSwitchMonitor;
};

#endif //_MUSIC_SWITCH_CTX_H_
