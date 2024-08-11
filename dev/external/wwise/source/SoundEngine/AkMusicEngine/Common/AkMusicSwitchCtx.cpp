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
// AkMusicSwitchCtx.cpp
//
// Music switch cntr context.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkMusicSwitchCtx.h"
#include "AkMusicSwitchCntr.h"
#include "AkSegmentCtx.h"
#include "AkMusicSegment.h"
#include "AkSequenceCtx.h"
#include "AkMusicStructs.h"
#include "AkMusicRenderer.h"
#include "AkMusicRanSeqCntr.h"
#include "AkScheduleWindow.h"
#include "AkMonitor.h"
#include "AkPlayingMgr.h"

//In the dynamic argument paths, the value of 0 is used as a "fallback" argument.
inline AkUniqueID ZeroToNoneHash(AkUniqueID in_switchID)
{
	#define AK_HASH_STATE_NONE 748895195 // This is the hash of "none" by GetIDFromString( "none" )
	AKASSERT( AK::SoundEngine::GetIDFromString( "none" ) == AK_HASH_STATE_NONE );
	
	if (in_switchID == 0)
		return AK_HASH_STATE_NONE;
	else
		return in_switchID;
}

//-----------------------------------------------------------------------------
// Name: CAkNothingCtx
// Desc: Chain-based context which represents an empty switch.
//-----------------------------------------------------------------------------
CAkNothingCtx::CAkNothingCtx(
    CAkMusicCtx *   in_parent
    )
:CAkChainCtx( in_parent )
,m_bWasReferenced( false )
{
}

CAkNothingCtx::~CAkNothingCtx()
{
}

AKRESULT CAkNothingCtx::Init(
    CAkRegisteredObj *  in_pGameObj,
    UserParams &    in_rUserparams,
    CAkMatrixSequencer *
    )
{
    if ( CAkChainCtx::Init( in_pGameObj, in_rUserparams ) == AK_Success )
    {
		// Create the one and only empty item.
		CAkScheduledItem * pItem = EnqueueItem( 0, NULL );
		if ( pItem )
		{
			AddRef();
			m_bWasReferenced = true;
			return AK_Success;
		}
    }
    return AK_Fail;
}

// MusicCtx override. Release.
void CAkNothingCtx::OnStopped()
{
	AddRef();

	if ( m_bWasReferenced )
	{
		m_bWasReferenced = false;
        Release();
    }

	CAkChainCtx::OnStopped();

	Release();
}

// For Music Renderer's music contexts look-up: concrete contexts must return their own node.
CAkMusicNode * CAkNothingCtx::Node()
{
    return NULL;
}


//-----------------------------------------------------------------------------
// Name: CAkMusicSwitchTransition
// Desc: Switch transition. The switch context keeps a list of all pending transitions. 
//       (either active or obsolete, but kept alive). 
//-----------------------------------------------------------------------------

CAkMusicSwitchTransition * CAkMusicSwitchTransition::Create(
	CAkMatrixAwareCtx *     in_pDestination
	)
{
	return AkNew( g_DefaultPoolId, CAkMusicSwitchTransition( in_pDestination ) );
}

CAkMusicSwitchTransition::CAkMusicSwitchTransition(
    CAkMatrixAwareCtx *     in_pDestination
    )
:pNextItem( NULL )
,m_pDestCtx( in_pDestination )
,m_bCmdPlayPending( false )
,m_bCmdStopPending( false )
,m_bWasScheduledToStop( false )
{
}

CAkMusicSwitchTransition::~CAkMusicSwitchTransition()
{
	// IMPORTANT: Clear destination to release it. This is a smart pointer.
	m_pDestCtx = NULL;
}

void CAkMusicSwitchTransition::Dispose()
{
	AkDelete( g_DefaultPoolId, this );
}

// Transition reversal.
bool CAkMusicSwitchTransition::CanBeCancelled()
{
	// Can be cancelled if its destination has not started playing yet.
	return !( Destination()->RequiresProcessing() );
}

bool CAkMusicSwitchTransition::CanBeRestored()
{
	// Can be restored if its destination can restart playing.
	return Destination()->CanRestartPlaying();
}

void CAkMusicSwitchTransition::Cancel()
{
	m_bCmdPlayPending = false;
	m_bCmdStopPending = false;

	// Explicitly stop the destination: reverted transition destination 
	// contexts have not started to play yet, but they might have created
	// and scheduled a whole bunch of children. Propagate STOP command to
	// ensure that the sub hierarchy is properly released.
	AKASSERT( Destination() );
	Destination()->_Cancel();
	
	Dispose();
}

void CAkMusicSwitchTransition::Restore(
	AkInt64		in_iCurrentTime,			// Current time relative to the sync time of this object.
	AkUInt32	in_uMaxFadeInDuration,		// Max fade-in duration. We want to be at full volume when new transition sync occurs.
    bool		in_bUseMaxFadeInDuration	// Ignore in_uMaxFadeInDuration if this parameter is false.
)
{
	// Undo stop command.

	AKASSERT( Destination()->CanRestartPlaying() );

	// ISSUE: Negative fade out offsets: Reference to Stop target is not held by transition.
	// Although we might protect our pointer from destruction, all its children would be 
	// stopped. 
	// Negative fade out offsets have been disabled for Wwise 2007.1.

	if ( m_bWasScheduledToStop && !m_bCmdStopPending )
	{
		// Stopping has already started. Need to fade back in.
		AKASSERT( Destination()->IsPlaying() );
			
		// Not possible that m_cmdStop.transParams.TransitionTime != 0 if !pending && playing.
		AKASSERT( m_cmdStop.transParams.TransitionTime > 0 );

		// Rule for fade-in time : 
		// Take the min value between new transition time and time elapsed since beginning of fade-out.

		AKASSERT( in_iCurrentTime >= m_cmdStop.iRelativeTime );
		
		AkMusicFade revFadeParams;
		revFadeParams.eFadeCurve = AkCurveInterpolation_Linear;
		AkUInt32 uFadeOutTimeElapsed = (AkUInt32)( in_iCurrentTime - m_cmdStop.iRelativeTime );
		if ( !in_bUseMaxFadeInDuration ||
			 uFadeOutTimeElapsed < in_uMaxFadeInDuration )
		{
			revFadeParams.transitionTime = AkTimeConv::SamplesToMilliseconds( uFadeOutTimeElapsed );
		}
		else
		{
			revFadeParams.transitionTime = AkTimeConv::SamplesToMilliseconds( in_uMaxFadeInDuration );
		}

		revFadeParams.eFadeCurve = m_cmdStop.transParams.eFadeCurve;
		revFadeParams.iFadeOffset = 0;
		Destination()->_Play( revFadeParams );
	}
	else
	{
		// Stopping has not started yet. Just undo it.
		m_bCmdStopPending = false;
	}

	m_bWasScheduledToStop = false;

	// Undo cutoff.
	m_cutoffInfo.Reset();
}

// Transition scheduling.
void CAkMusicSwitchTransition::ScheduleToPlay(
	AkInt64					in_iSyncTime,		// Sync time in the context of the owner context.
	const AkMusicFade &		in_fadeParams,		// Volume fade.
    AkInt32					in_iLookAheadTime	// Amount of time ahead sync point where playback should start.
	)
{
	// Schedule transition destination.
	AKASSERT( Destination() );
	Destination()->Schedule( in_iSyncTime );

	// Set up play command.
	m_cmdPlay.iRelativeTime = in_iSyncTime - in_iLookAheadTime;
	m_cmdPlay.fadeParams	= in_fadeParams;

	// Do not bother fading in when destination is <nothing>.
	if ( Destination()->Node() == NULL )
	{
		m_cmdPlay.fadeParams.transitionTime = 0;
		AKASSERT( in_iLookAheadTime == 0 );	// A null context should not require any look-ahead.
	}

    // Adjust fade offset relatively to sync time if there is a fade (transition time > 0).
	if ( in_fadeParams.transitionTime > 0 )
		m_cmdPlay.fadeParams.iFadeOffset += in_iLookAheadTime;

    m_bCmdPlayPending = true;
}

void CAkMusicSwitchTransition::ScheduleToStop(
	AkInt64					in_iSyncTime,		// Owner context's sync point, after which segment chains are cutoff (inclusive).
	AkInt64					in_iStopTime,		// Owner context's time at which stopping should occur/begin.
	AkTimeMs                in_iTransDuration,	// Fade out duration.
    AkCurveInterpolation	in_eFadeCurve,		// Fade curve.
	bool					in_bCutoff			// True in order to set cutoff at sync point. False to skip cutoff mechanism.
	)
{
	m_cmdStop.transParams.TransitionTime = in_iTransDuration;
    m_cmdStop.transParams.eFadeCurve = in_eFadeCurve;
    m_cmdStop.iRelativeTime = in_iStopTime;
    
	m_bCmdStopPending = true;
	m_bWasScheduledToStop = true;

	if ( in_bCutoff )
		m_cutoffInfo.Set( in_iSyncTime );
}

// Process switch transition: Notifies Sync, RelativeProcess()es switchee before sync,
// executes high-level transition commands (play and stop on higher-level contexts).
// Consumes absolute switch time.
// Returns true if transition reached its sync point during processing.
void CAkMusicSwitchTransition::Process(
	AkInt64			in_iCurrentTime,		// Current time relative to the sync time of the owner switch context.
	AkUInt32		in_uNumSamples,			// Number of samples to process.
	AkCutoffInfo &	in_cutoffInfo			// Downstream chain cutoff info.
	)
{
	AkInt64	iCurrentFrameEnd = in_iCurrentTime + in_uNumSamples;

	// Play command.
    if ( m_bCmdPlayPending 
		&& in_iCurrentTime <= m_cmdPlay.iRelativeTime
		&& iCurrentFrameEnd > m_cmdPlay.iRelativeTime )
    {
		Destination()->_Play( m_cmdPlay.fadeParams );
        m_bCmdPlayPending = false;
    }

	// Stop command.
    if ( m_bCmdStopPending 
		&& iCurrentFrameEnd > m_cmdStop.iRelativeTime 
		&& in_iCurrentTime <= m_cmdStop.iRelativeTime )
    {
		AKASSERT( m_bWasScheduledToStop );
		Destination()->_Stop( m_cmdStop.transParams, AkTimeConv::ToShortRange(m_cmdStop.iRelativeTime - in_iCurrentTime) );
        m_bCmdStopPending = false;
    }

	// Process destination.
	if ( Destination()->RequiresProcessing() )
	{
		in_cutoffInfo.Merge( m_cutoffInfo );
		Destination()->Process( in_iCurrentTime, in_uNumSamples, in_cutoffInfo );
	}
}


//-----------------------------------------------------------------------------
// Name: CAkMusicSwitchMonitor
//-----------------------------------------------------------------------------

void CAkMusicSwitchMonitor::SetSwitch( 
					   AkUInt32 in_switchID, 
					   CAkRegisteredObj * in_pGameObj
					   )
{
	// Compute switch change only if notification applies to us.
	if ( in_pGameObj == m_pMusicCtx->Sequencer()->GameObjectPtr() || 
		AkGroupType_State == m_pMusicCtx->m_pSwitchCntrNode->GetSwitchGroupType(m_uIdx) )  
	{
		in_switchID = ZeroToNoneHash(in_switchID);

		if( m_targetSwitchID != in_switchID )
		{
			m_targetSwitchID = in_switchID;
			if ( m_pMusicCtx->m_pParentCtx &&
				static_cast<CAkMusicSwitchCtx*>(m_pMusicCtx->m_pParentCtx)->HasOrAscendentHasPendingTransition() )
			{
				m_pMusicCtx->SetDelayedSwitchTargetFlag();
			}
			else
			{
				m_pMusicCtx->ChangeSwitch( CAkMusicSwitchCtx::TransitionInfo() );

				// Notify children that the transition was changed. Some of them could be waiting for this.
				m_pMusicCtx->TryPropagateDelayedSwitchChange();
			}
		}
	}
}

AKRESULT CAkMusicSwitchMonitor::Init( AkUInt32 in_uIdx, CAkMusicSwitchCtx& in_pMusicCtx )
{
	m_targetSwitchID = AK_INVALID_UNIQUE_ID;
	m_uIdx = in_uIdx;
	m_pMusicCtx = &in_pMusicCtx;
	CAkMusicSwitchCntr* pMusicSwCtr = m_pMusicCtx->m_pSwitchCntrNode;
	AkUInt32 uGroupID = pMusicSwCtr->GetSwitchGroup(in_uIdx);
	AkGroupType uGroupType = pMusicSwCtr->GetSwitchGroupType(in_uIdx);
	AKRESULT res = SubscribeSwitch( uGroupID, uGroupType );

	if (res == AK_Success)
	{
		m_targetSwitchID = ZeroToNoneHash( GetSwitchToUse( in_pMusicCtx.Sequencer()->GameObjectPtr(), 
			uGroupID,
			uGroupType ));
	}

	return res;
}


//-----------------------------------------------------------------------------
// Name: CAkMusicSwitchCtx
// Desc: Music Switch Container Context.
//-----------------------------------------------------------------------------

CAkMusicSwitchCtx::CAkMusicSwitchCtx(
    CAkMusicSwitchCntr *in_pSwitchNode,
    CAkMusicCtx *       in_pParentCtx
    )
:CAkMatrixAwareCtx( in_pParentCtx )
,m_pSwitchCntrNode( in_pSwitchNode )
,m_pLastNonCancelledTransitionInFrame( NULL )
,m_bHasDelayedSwitchID( false )
,m_bWasReferenced( false )
{
	m_itActiveSwitch = m_queueTransitions.End();

	if( m_pSwitchCntrNode )
		m_pSwitchCntrNode->AddRef();
}

CAkMusicSwitchCtx::~CAkMusicSwitchCtx()
{
	m_switchMonitors.Term();

	if( m_pSwitchCntrNode )
		m_pSwitchCntrNode->Release();
}

AKRESULT CAkMusicSwitchCtx::Init(
    CAkRegisteredObj *  in_GameObject,
    UserParams &        in_rUserparams
    )
{
    AKRESULT eResult = CAkMatrixAwareCtx::Init( in_GameObject, in_rUserparams );
	if ( eResult == AK_Success )
	{
		m_switchMonitors.Term();

		const AkUInt32 uNumSwGroups = m_pSwitchCntrNode->GetTreeDepth();
		if (uNumSwGroups > 0)
		{
			eResult = m_switchMonitors.Reserve(uNumSwGroups);
			if (eResult == AK_Success)
			{
				// Register to RTPC/State Mgr.
				for (AkUInt32 grp = 0, nGroups = uNumSwGroups;  
						eResult == AK_Success && grp < nGroups;
						++grp)
				{
					m_switchMonitors.AddLast(CAkMusicSwitchMonitor());
					eResult = m_switchMonitors[grp].Init(grp, *this);
				}
			}
		}


		if ( eResult == AK_Success )
		{
			// Add ref ourselves. A switch context lives until explicitly stopped (released in OnStopped).
			AddRef();
			m_bWasReferenced = true;

			if ( IsTopLevel() )
			{
				// Top-level context.
				// Set initial switch to <nothing>.
				eResult = SetInitialSwitch( true );
				if ( eResult == AK_Success )
				{
					// Schedule transition to current switch.
					ChangeSwitch( TransitionInfo() );
				}
			}
			else
			{
				// Child switch context: create our first active child context that corresponds to the switch desired,
				// and return it. Our parent will take care of scheduling us.
				eResult = SetInitialSwitch();
			}

		}
	}
    return eResult;
}

// Called by parent (switches): completes the initialization.
// Propagate to children.
void CAkMusicSwitchCtx::EndInit()
{
	AkTransitionsQueue::Iterator it = m_queueTransitions.Begin();
    while ( it != m_queueTransitions.End() )
    {
		if ( (*it)->Destination() )
			(*it)->Destination()->EndInit();
		++it;
	}
}

// Interface for parent switch context: trigger switch change that was delayed because of parent transition.
void CAkMusicSwitchCtx::PerformDelayedSwitchChange()
{
	if ( IsPlaying() || IsIdle() )
	{
		if ( HasDelayedSwitchTarget() )
		{
			// If we're idle or playing, schedule our switch transition now.
			// Note: Need to perform a transition even if idle because we might need some time before 
			// the sync point, which we don't have.
			ChangeSwitch( TransitionInfo() );
		}

		// Propagate to children if this or any ascendent has no pending transition.
		TryPropagateDelayedSwitchChange();
	}
}

// Instantiate and schedule child according to current switch the first time, 
// while this context is idle.
// Returns AK_Success or AK_Fail. When failing, the children/transitions previously scheduled (if any) are restored.
AKRESULT CAkMusicSwitchCtx::SetInitialSwitch(
	bool				in_bNothing			// If true, initial child is nothing (in_switchID is ignored).
    )
{
	AkTransitionsQueue transitionsToCancel;
	AkTransitionsQueue::IteratorEx it = m_queueTransitions.BeginEx();
	while ( it != m_queueTransitions.End() )
	{
		CAkMusicSwitchTransition * pCancelledTransition = (*it);
		it = m_queueTransitions.Erase( it );
		transitionsToCancel.AddLast( pCancelledTransition );
	}

	// Clear delayed switch ID.
	ClearDelayedSwitchTargetFlag();
	
	// Create destination context and prepare it.
	AkUniqueID firstNodeID = in_bNothing ? AK_MUSIC_TRANSITION_RULE_ID_NONE : ResolveAudioNode();
	CAkMatrixAwareCtx * pInitialCtx = CreateDestinationContext( firstNodeID );

	AKRESULT eResult = AK_Success;
	if ( !pInitialCtx ||
		 PrepareFirstContext( pInitialCtx ) != AK_Success )
	{
		eResult = AK_Fail;
	}

	// Cancel dequeued transitions (or restore in case of error).
	if ( eResult == AK_Success )
	{
		// Cancel dequeued transitions. 
		AkTransitionsQueue::IteratorEx it = transitionsToCancel.BeginEx();
		while ( it != transitionsToCancel.End() )
		{
			CAkMusicSwitchTransition * pCancelledTransition = (*it);
			it = transitionsToCancel.Erase( it );
			pCancelledTransition->Cancel();
		}
	}
	else
	{
		// There was an error. Restore all dequeued transitions to avoid leaving this context in an inconsistent state.
		AkTransitionsQueue::IteratorEx it = transitionsToCancel.BeginEx();
		while ( it != transitionsToCancel.End() )
		{
			CAkMusicSwitchTransition * pCancelledTransition = (*it);
			it = transitionsToCancel.Erase( it );
			m_queueTransitions.AddLast( pCancelledTransition );
		}
	}

	transitionsToCancel.Term();

	return eResult;
}

// Switch transition: Instantiate and schedule child according to new switch value, 
// while this context is playing.
// No return value: when failing, the context is already playing and a new transition will simply not happen.
void CAkMusicSwitchCtx::ChangeSwitch(
	const TransitionInfo & in_transitionInfo // Transition info. 
	)
{
	// Clear delayed switch ID.
	ClearDelayedSwitchTargetFlag();

	// This operation is potentially dangerous if this context is stopping.
	if ( IsStopping() )
		return;

    // Query our node for the associated child node ID.
	AkUniqueID nextNodeID = ResolveAudioNode();

	// Create next node context.
	CAkMatrixAwareCtx * pNewContext;
	pNewContext = CreateDestinationContext( nextNodeID );
	// Schedule.
	if ( pNewContext )
	{
		ScheduleSwitchTransition( nextNodeID, in_transitionInfo, pNewContext );

		// NOTE: ScheduleSwitchTransition() could decide not to enqueue a new transition. In such a case (io_)pNewContext
		// would be destroyed and set to NULL therein.
		if ( pNewContext )
			pNewContext->EndInit();
	}
}

AkUniqueID CAkMusicSwitchCtx::ResolveAudioNode() const
{
	AkUniqueID audioNode = AK_INVALID_UNIQUE_ID;
	AkUInt32 uNumGroups = m_switchMonitors.Length();
	
	AkUniqueID* pSwitchIDs = (AkUniqueID*)AkAlloca( sizeof(AkUniqueID)*uNumGroups );
	for (AkUInt32 uGrp=0; uGrp < uNumGroups; ++uGrp )
	{
		pSwitchIDs[uGrp] = m_switchMonitors[uGrp].m_targetSwitchID;
	}
	audioNode = m_pSwitchCntrNode->ResolvePath( pSwitchIDs, uNumGroups, 0);

	return audioNode;
}

// Determines whether a new transition must be enqueued (handles "Continue to play" behavior).
bool CAkMusicSwitchCtx::IsSwitchTransitionNeeded(
	AkUniqueID					in_nextNodeID,			// Destination node ID.
	AkSeekingInfo	*			in_pSeekingInfo,		// Seeking info. NULL if seeking not required.
	CAkMusicSwitchTransition *	in_pLastValidTransition	// Transition object whose destination should be checked against in_nextNodeID.
	)
{
	// Transition needed anyway if not "continue playback on switch change" or if seeking.
	if ( !m_pSwitchCntrNode->ContinuePlayback() 
		|| in_pSeekingInfo )
		return true;

	AKASSERT( m_queueTransitions.Last()->Destination() );
	CAkMusicNode * pCurTargetNode = in_pLastValidTransition->Destination()->Node();
	if ( ( pCurTargetNode && pCurTargetNode->ID() == in_nextNodeID ) ||
		 ( !pCurTargetNode && in_nextNodeID == AK_MUSIC_TRANSITION_RULE_ID_NONE ) )
	{
		return false;
	}
	return true;
}


// CAkMatrixAwareCtx
// -----------------------------------------------------

// Sequencer access.
void CAkMusicSwitchCtx::Process(
	AkInt64			in_iCurrentTime,		// Current time in the time coordinates of the caller (parent).
	AkUInt32		in_uNumSamples,			// Number of samples to process.
	AkCutoffInfo &	in_cutoffInfo			// Downstream chain cutoff info.
	)
{
	ProcessPrologue( in_iCurrentTime, in_uNumSamples, in_cutoffInfo );

	AkInt64	iCurrentFrameEnd = in_iCurrentTime + in_uNumSamples;

	// Reset m_pLastNonCancelledTransitionInFrame for next frame.
	m_pLastNonCancelledTransitionInFrame = NULL;

	AkTransitionsQueue::IteratorEx it = m_queueTransitions.BeginEx();
    while ( it != m_queueTransitions.End() )
    {
		CAkMusicSwitchTransition * pTrans = (*it);

		if ( in_iCurrentTime <= pTrans->SyncTime()
			&& iCurrentFrameEnd > pTrans->SyncTime() )
		{
			// Sync frame!
			AKASSERT( pTrans->RequiresProcessing() );
			Sync( it );
		}

        // Process transition.
		if ( pTrans->RequiresProcessing() )
		{
			// Copy cutoff info on the stack for each individual transition.
			AkCutoffInfo cutoffInfo = in_cutoffInfo;
			pTrans->Process( in_iCurrentTime, in_uNumSamples, cutoffInfo );
			++it;
		}
		else if ( pTrans != (*m_itActiveSwitch) )
		{
	        // Release pending transition.
			it = m_queueTransitions.Erase( it );
            pTrans->Dispose();
        }
        else
            ++it;
    }

	ProcessEpilogue( in_iCurrentTime, in_uNumSamples );
}

// Prepares the context according to the destination transition rule, if specified. 
// Otherwise, prepares it at position in_iMinStartPosition. The returned look-ahead time 
// corresponds to the time needed before reaching the synchronization point.
// Returns the required look-ahead time. 
AkInt32 CAkMusicSwitchCtx::Prepare(
	const AkMusicTransDestRule * in_pRule,	// Transition destination (arrival) rule. NULL means "anywhere", in any segment.
	AkInt32		in_iMinStartPosition,		// Minimum play start position in segment/sequence (relative to first segment entry cue). Actual position may occur later depending on rule.
	AkSeekingInfo * in_pSeekingInfo,		// Seeking info (supersedes in_iMinStartPosition). NULL if not seeking.
	AkUniqueID & out_uSelectedCue,			// Returned cue this context was prepared to. AK_INVALID_UNIQUE_ID if not on a cue.
	AkUniqueID	in_uCueHashForMatchSrc		// Cue which was used in the source segment. Used only if rule is bDestMatchCueName.
	)
{
	// At this point, a switch container was just created and has one and only one transition in its list.
	// Prepare its destination.
	AKASSERT( !m_queueTransitions.IsEmpty() && m_itActiveSwitch != m_queueTransitions.End() );
	return m_queueTransitions.First()->Destination()->Prepare( in_pRule, in_iMinStartPosition, in_pSeekingInfo, out_uSelectedCue, in_uCueHashForMatchSrc );
}

// Inspect context that has not started playing yet and determine the time when it will start playing,
// relative to its sync time (out_iPlayTime, typically <= 0). Additionnally, compute the time when it 
// will start playing and will be audible (out_iPlayTimeAudible, always >= out_iPlayTime).
void CAkMusicSwitchCtx::QueryLookAheadInfo(
	AkInt64 &	out_iPlayTime,			// Returned time of earliest play command (relative to context).
	AkInt64 &	out_iPlayTimeAudible	// Returned time of earliest play command that becomes audible. Always >= out_iPlayTime. Note that it may not refer to the same play action as above.
	)
{
	AkTransitionsQueue::Iterator it = m_queueTransitions.Begin();
	if ( it == m_queueTransitions.End() )
	{
		out_iPlayTime = 0;
		out_iPlayTimeAudible = 0;
		return;
	}

	// Compute look-ahead info for first destination and convert to local time.
	AkInt64 iPlayTime;
	AkInt64 iPlayTimeAudible;

	// Skip first (<nothing>) context if this is a top-level context.
	if ( IsTopLevel() )
	{
		// Ensure there is another transition, just in case we were out of memory at the time of
		// instantiation.
		AkTransitionsQueue::Iterator itNext = it;
		++itNext;
		if ( itNext != m_queueTransitions.End() )
			++it;
	}

	(*it)->Destination()->QueryLookAheadInfo( iPlayTime, iPlayTimeAudible );
	
	out_iPlayTime = iPlayTime + (*it)->SyncTime();
	out_iPlayTimeAudible = iPlayTimeAudible + (*it)->SyncTime();

	++it;

    while ( it != m_queueTransitions.End() )
    {
		(*it)->Destination()->QueryLookAheadInfo( iPlayTime, iPlayTimeAudible );
		iPlayTime += (*it)->SyncTime();	// Convert to local time
		if ( iPlayTime < out_iPlayTime )
			out_iPlayTime = iPlayTime;
		iPlayTimeAudible += (*it)->SyncTime();	// Convert to local time
		if ( iPlayTimeAudible < out_iPlayTimeAudible )
			out_iPlayTimeAudible = iPlayTimeAudible;

		++it;
	}
}

// A switch context can restart playing if it hasn't already stopped and its ultimate transition 
// destination can restart.
bool CAkMusicSwitchCtx::CanRestartPlaying()
{
	if ( IsPlaying() || IsIdle() )
		return m_queueTransitions.Last()->CanBeRestored();
	else
		return false;
}

// Stop a context based on how much time it has played already. Should be stopped immediately
// if it has not been playing yet, or faded out by the amount of time it has been playing.
void CAkMusicSwitchCtx::CancelPlayback(
	AkInt64 in_iCurrentTime		// Current time in the time coordinates of the caller (parent).
	)
{
	AddRef();
	if ( IsIdle() )
		OnStopped();
	else if ( IsPlaying() )
	{
		// Convert into local time values.
		in_iCurrentTime = ParentToLocalTime( in_iCurrentTime );
		AKASSERT( in_iCurrentTime <= 0 );

		TransParams transParams;
		transParams.TransitionTime = AkTimeConv::SamplesToMilliseconds( AkTimeConv::ToShortRange( -in_iCurrentTime ) );
		transParams.eFadeCurve = AkCurveInterpolation_Linear;
		_Stop( transParams );
	}
	Release();
}

// Context-specific delegate for CAkScheduleWindow. 
// Switch contexts push their current transition objects (branch) into the window, and ask their destination
// context to complete the job. They also need to compare with the next transition object in order to determine
// which branch to use, and possibly shorten the window length if a transition is scheduled during the segment
// pointed by this window.
void CAkMusicSwitchCtx::GetNextScheduleWindow( 
	CAkScheduleWindow & io_window,		// Current schedule window, returned as next schedule window. If io_window had just been instantiated, the active window would be returned.
	bool in_bDoNotGrow					// If true, chains are not asked to grow. "False" is typically used for scheduling, whereas "True" is used to check the content of a chain.
	)
{
	// Push level.
	CAkScheduleWindow::AutoBranchLevel branchLevel( io_window );

	AkTransQueueIter itCurBranch = io_window.GetBranch();
	if ( itCurBranch == m_queueTransitions.End() )
	{
		// First time. Use our active branch.
		if ( m_itActiveSwitch != m_queueTransitions.End() )
		{
			itCurBranch = m_itActiveSwitch;
			io_window.SetBranch( itCurBranch );
		}
		else
		{
			if ( !io_window.IsValid() )
				return;
		}
	}
	else
	{
		// Must not have been called again if length was already infinite.
		AKASSERT( !io_window.IsDurationInfinite() );
	}

	// Note: schedule window can be invalid after this call. In such a case,
	// StartTimeRelativeToCurrentLevel() below would return infinity and we would change branch.
	(*itCurBranch)->Destination()->GetNextScheduleWindow( io_window, in_bDoNotGrow );
	
	// If there is another branch following the current one, get its next schedule window 
	// in a separate object, and compare sync times. We choose the branch that comes first.

	AkTransQueueIter itNextBranch = ++itCurBranch;

	if ( itNextBranch != m_queueTransitions.End() )
	{
		AkInt64 iNextBranchSync = (*itNextBranch)->SyncTime();

		// Compare the start time of the window (relative to this context) with the 
		// sync time of the next branch. Skip/change branch as long as next branch
		// occurs before (or at the same time - we want to skip branches with length 0).
		while ( io_window.StartTimeRelativeToCurrentLevel() >= iNextBranchSync )
		{
			// Change branch! Set this branch and get new schedule window from child.
			io_window.SetBranch( itNextBranch );
			(*itNextBranch)->Destination()->GetNextScheduleWindow( io_window, in_bDoNotGrow );	

			// Check next branch in order to truncate window length if applicable.
			++itNextBranch;
			if ( itNextBranch != m_queueTransitions.End() )
				iNextBranchSync = (*itNextBranch)->SyncTime();
			else
			{
				// No more valid branch. Break.
				break;
			}
		}
		// else Remain in current branch.

		// Truncate window length with next branch if applicable.
		if ( itNextBranch != m_queueTransitions.End() )
		{
			AKASSERT( io_window.StartTimeRelativeToCurrentLevel() < iNextBranchSync );
			AkUInt64 uMaxDuration = ( iNextBranchSync - io_window.StartTimeRelativeToCurrentLevel() );
			if ( io_window.IsDurationInfinite() || io_window.Duration() > uMaxDuration )
				io_window.SetDuration( uMaxDuration, false );
		}
	}
}

// Call this to refresh the current window's length.
void CAkMusicSwitchCtx::RefreshWindow(
	CAkScheduleWindow & io_window
	)
{
	// Push level.
	CAkScheduleWindow::AutoBranchLevel branchLevel( io_window );

	AkTransQueueIter itCurBranch = io_window.GetBranch();
	AKASSERT( itCurBranch != m_queueTransitions.End() );

	(*itCurBranch)->Destination()->RefreshWindow( io_window );
	
	// Truncate window length with next branch if applicable.
	AkTransQueueIter itNextBranch = ++itCurBranch;
	if ( itNextBranch != m_queueTransitions.End() )
	{
		AkInt64 iNextBranchSync = (*itNextBranch)->SyncTime();
		AKASSERT( io_window.StartTimeRelativeToCurrentLevel() < iNextBranchSync );

		AkUInt64 uMaxDuration = ( iNextBranchSync - io_window.StartTimeRelativeToCurrentLevel() );
		if ( io_window.IsDurationInfinite() || io_window.Duration() > uMaxDuration )
			io_window.SetDuration( uMaxDuration, false );
	}
}

// Get scheduling window pointed by the destination of the last (latest) scheduled transition.
void CAkMusicSwitchCtx::MoveWindowToLastNonCancellableTransition( 
	CAkMusicSwitchTransition *	in_pLastNonCancellableTransition,
	CAkScheduleWindow &			io_window 
	)
{
	// Push level.
	CAkScheduleWindow::AutoBranchLevel branchLevel( io_window );

	AkTransQueueIter itLastBranch = m_queueTransitions.FindEx( in_pLastNonCancellableTransition );
	if ( io_window.GetBranch() != itLastBranch )
	{
		io_window.SetBranch( itLastBranch );
		(*itLastBranch)->Destination()->GetNextScheduleWindow( io_window );
		AKASSERT( io_window.IsValid() );
	}
}

// Create a scheduling window pointing to the transition segment. It is meant to be used to schedule 
// a transition from the transition segment. The transition object's destination should be the 
// transition segment context.
void CAkMusicSwitchCtx::SetupWindowForTransContext( 
	CAkMusicSwitchTransition *	in_pTransitionToTransCtx,	// Transition object pointing to transition segment context.
	AkInt64						in_uSyncTime,				// Sync time of transition context.
	CAkScheduleWindow &			out_wndTrans				// Returned scheduling window pointing on transition segment.
	)
{
	// Set scheduled time for transition context now so that window time makes sense.
	in_pTransitionToTransCtx->Destination()->Schedule( in_uSyncTime );

	// Push level.
	CAkScheduleWindow::AutoBranchLevel branchLevel( out_wndTrans );

	AkTransQueueIter itTrans;
	itTrans.pItem = in_pTransitionToTransCtx;

	out_wndTrans.SetBranch( itTrans );

	(*itTrans)->Destination()->GetNextScheduleWindow( out_wndTrans );
	AKASSERT( out_wndTrans.IsValid() );
}

// Context commands
//
// Override MusicCtx OnPlayed: Need to play first transition destination.
void CAkMusicSwitchCtx::OnPlayed()
{
	if ( IsIdle() )
	{
		// Context was just created. Schedule and play first destination now. 
		// Following destinations are played based on switch scheduling.
		m_queueTransitions.First()->Destination()->Schedule( 0 );
		AkMusicFade fadeParams;
		fadeParams.transitionTime = 0;
		fadeParams.iFadeOffset = 0;
		fadeParams.eFadeCurve = AkCurveInterpolation_Linear;
		m_queueTransitions.First()->Destination()->_Play( fadeParams );
	}

	CAkMusicCtx::OnPlayed();
}

// Override MusicCtx OnStopped: Need to release transitions destination.
void CAkMusicSwitchCtx::OnStopped()
{
	AddRef();

	m_switchMonitors.Term();

	// Destroy all pending transitions.
	AkTransitionsQueue::IteratorEx it = m_queueTransitions.BeginEx();
	while ( it != m_queueTransitions.End() )
	{
		CAkMusicSwitchTransition * pTrans = (*it);
		it = m_queueTransitions.Erase( it );
		pTrans->Dispose();
	}
	m_itActiveSwitch = m_queueTransitions.Begin();
	m_queueTransitions.Term();

	if ( m_bWasReferenced )
	{
		m_bWasReferenced = false;
		Release();
	}

	CAkMatrixAwareCtx::OnStopped();

	Release();
}

// For Music Renderer's music contexts look-up.
CAkMusicNode * CAkMusicSwitchCtx::Node()
{
    return m_pSwitchCntrNode;
}

// Callback from pending switch transitions.
void CAkMusicSwitchCtx::Sync( 
	AkTransQueueIter &	in_itNewTransition	// Iterator to transition which just reached its sync point.
	)
{
	// Sync: Set the active switch iterator to the new transition object. 
	// Normally it is ++m_itActiveSwitch, unless it is the first transition, in such case it has already
	// been set.
	AKASSERT( m_itActiveSwitch == in_itNewTransition || ++m_itActiveSwitch == in_itNewTransition );
	m_itActiveSwitch = in_itNewTransition;

	if ( Sequencer()->GetMusicSyncFlags() & AK_MusicSyncPoint )
	{
		CAkScheduleWindow curWindow( this );
		if ( curWindow.IsValid() )
		{
			// WG-19892: sync point is notified when synching to nothing or to trailing sequence.
			CAkMusicNode * pNode, *pParentNode;
			CAkMusicSegment * pSegmentNode = curWindow.GetNode( &pParentNode );
			pNode = ( pSegmentNode ) ? pSegmentNode : pParentNode;

			const AkMusicGrid& rGrid = pNode->GetMusicGrid();
			g_pPlayingMgr->NotifyMusic( Sequencer()->PlayingID(), AK_MusicSyncPoint, rGrid );
		}
	}

	MONITOR_MUSICTRANS( 
		Sequencer()->PlayingID(), 
		Sequencer()->GameObjectPtr()->ID(), 
		AkMonitorData::NotificationReason_MusicTransitionResolved, 
		AK_UINT_MAX, 
		m_pSwitchCntrNode->ID(), 
		AK_INVALID_UNIQUE_ID, 
		(*m_itActiveSwitch)->Destination()->NodeID(), 
		AK_INVALID_UNIQUE_ID, 
		AK_INVALID_UNIQUE_ID, 
		AK_INVALID_UNIQUE_ID, 
		AK_INVALID_UNIQUE_ID, 
		0 );

	TryPropagateDelayedSwitchChange();
}

void CAkMusicSwitchCtx::TryPropagateDelayedSwitchChange()
{
	// If there is no more pending transition in this context and in any ascendent, trigger possibly delayed switch changes on child.
	if ( !HasOrAscendentHasPendingTransition() )
	{
		AkTransQueueIter itTrans = m_queueTransitions.Begin();
		while ( itTrans != m_queueTransitions.End() )
		{
			CAkMatrixAwareCtx * pChild = (*itTrans)->Destination();
			if ( pChild )
				pChild->PerformDelayedSwitchChange();
			++itTrans;
		}
	}
}

// Creates child context.
CAkMatrixAwareCtx * CAkMusicSwitchCtx::CreateDestinationContext(
    AkUniqueID in_ID	// Node ID
    )
{
	CAkMatrixAwareCtx * pNewContext = NULL;
    if ( in_ID != AK_MUSIC_TRANSITION_RULE_ID_NONE )
    {
		pNewContext = CreateMusicContext( in_ID );
    }
    
	// Create a Nothing context if ID is <nothing>, or if there was an error.
	if ( in_ID == AK_MUSIC_TRANSITION_RULE_ID_NONE ||
		 !pNewContext )
    {
        CAkNothingCtx * pNothingCtx = AkNew( g_DefaultPoolId, CAkNothingCtx( this ) );
        if ( pNothingCtx )
        {
			pNothingCtx->AddRef();
			if ( pNothingCtx->Init( Sequencer()->GameObjectPtr(),
                                  Sequencer()->GetUserParams(),
                                  Sequencer() ) == AK_Success )
			{
				pNothingCtx->Release();
			}
			else
            {
				pNothingCtx->_Cancel();
				pNothingCtx->Release();
                pNothingCtx = NULL;
            }
			
        }
        pNewContext = pNothingCtx;
    }

    return pNewContext;
}

CAkMatrixAwareCtx * CAkMusicSwitchCtx::CreateMusicContext(
	AkUniqueID in_ID
	)
{
	CAkMatrixAwareCtx * pNewContext = NULL;

	CAkMusicNode * pNewNode = static_cast<CAkMusicNode*>(g_pIndex->GetNodePtrAndAddRef( in_ID, AkNodeType_Default ));
    if ( pNewNode )
	{
		// TODO Optim. Avoid passing GameObj and Params to child context.
		pNewContext = pNewNode->CreateContext(
			this,
			Sequencer()->GameObjectPtr(),
			Sequencer()->GetUserParams() );
		pNewNode->Release();
	}

	return pNewContext;
}

// Schedule transition actions for switch change.
void CAkMusicSwitchCtx::ScheduleSwitchTransition(
	AkUniqueID			in_destinationID,	// Node ID of destination.
	const TransitionInfo & in_transitionInfo,	// Transition info. 
    CAkMatrixAwareCtx *& io_pNewContext 	// New context. Can be destroyed internally if we decide not to schedule it.
    )
{
    AKASSERT( io_pNewContext );
    
	AKASSERT( !m_pParentCtx ||
			  !static_cast<CAkMusicSwitchCtx*>(m_pParentCtx)->HasOrAscendentHasPendingTransition() );

	AkInt64 iCurrentTime = GlobalToLocalTime( Sequencer()->Now() );

	CAkMusicSwitchTransition * pTransition1 = NULL;
	CAkMusicSwitchTransition * pTransition2 = NULL;

	// Get a window to the current schedulable item within this context.
	CAkScheduleWindow wndSource( this );
	if ( !wndSource.IsValid() )
	{
		io_pNewContext->_Cancel();
		io_pNewContext = NULL;
		return;
	}

	// Handle current transition(s) reversal.
	AkTransitionsQueue listCancelledTransitions;
	// Behavior change: Always dequeue cancellable transitions _before_ finding sync point if they have been scheduled in 
	// the same frame.
	if ( m_pLastNonCancelledTransitionInFrame )
		DequeueCancellableTransitions( m_pLastNonCancelledTransitionInFrame, listCancelledTransitions, wndSource );

	CAkMusicSwitchTransition * pLastNonCancellableTransition = FindLastNonCancellableTransition();	
	// pLastNonCancellableTransition points at the latest transition object that cannot be removed from the queue.
	// Perhaps it already points to the same destination as the new destination: Reconsider scheduling new transition.
	// Also, pLastNonCancellableTransition may not be restorable. This is only possible if it is the last 
	// transition in the queue. In such a case it is useless to try to schedule a transition.
	if ( IsSwitchTransitionNeeded( in_destinationID, in_transitionInfo.pSeekingInfo, pLastNonCancellableTransition )
		&& pLastNonCancellableTransition->CanBeRestored() )
	{    
		// Yes. Proceed.

		bool bFailedEnqueueingTransition = true;

		// Move the scheduling window to the destination context of the last transition that is not 
		// cancellable. If it is the current active transition, the winodw will point at the current 
		// active item. This is where we can start trying to schedule a new transition.
		MoveWindowToLastNonCancellableTransition( pLastNonCancellableTransition, wndSource );
		
		//
		// Find transition sync point.
		//

		AssociatedActionsList listCancelledActions;	// List used to gather future actions (triggers, state changes) that will be cancelled.

		AKRESULT eReturnVal;
		AkUInt32 uSrcSegmentLookAhead = 0;	// Used for transition length limit.
		do
		{
			// Avoid computing a transition uselessly if playback was already skipped.
			if ( wndSource.GetScheduledItem()->WasPlaybackSkipped() )
			{
				// Playback of this segment was skipped: transition would be enqueued for nothing (see WG-8114).
				// Just bail out.
				// Note: Could be irrelevant since WG-20980. Review.
				break;		
			}
			
			// Query transition rule.
			// Note: io_pNewContext may come out NULL if there is a fatal error with the destination chain.
			const AkMusicTransitionRule & rule = GetTransitionRule( 
				wndSource, 
				io_pNewContext,
				uSrcSegmentLookAhead );
			if ( !io_pNewContext )
			{
				// Context re-creation because of transition rule failed.
				break;
			}

			// Has a transition segment? 
			CAkMatrixAwareCtx * pTransCtx = NULL;
			if ( rule.pTransObj )
			{
				// Create transition segment context.				
				pTransCtx = CreateMusicContext( rule.pTransObj->segmentID );
				if ( pTransCtx )
				{
					// Create transition objects.
					pTransition1 = CAkMusicSwitchTransition::Create( pTransCtx );
					pTransition2 = CAkMusicSwitchTransition::Create( io_pNewContext );
					if ( !pTransition1 || !pTransition2 )
					{
						// Failed creating one of the transitions. Destroy transition context and transition 1 and 2, 
						// and attempt recreating transition 1 towards final destination (skip transition segment).
						if ( pTransition1 )
							pTransition1->Cancel();		// destroys destination (transition context)
						else
							pTransCtx->_Cancel();
						pTransCtx = NULL;
						
						if ( pTransition2 )
						{
							pTransition2->Dispose();	// preserves destination
							pTransition2 = NULL;
						}

						pTransition1 = CAkMusicSwitchTransition::Create( io_pNewContext );
					}
				}
				else
				{
					// Failed to create transition context. Try perfoming a transition without it.
					pTransition1 = CAkMusicSwitchTransition::Create( io_pNewContext );
				}
			}
			else
			{
				// Create transition objects.
				pTransition1 = CAkMusicSwitchTransition::Create( io_pNewContext );
			}
			if ( !pTransition1 )
			{
				// No "main" transition object: fatal.
				break;
			}

			
			// Split rule in 2: one for source to transition segment,
			// and another for transition segment to destination.
			AkMusicTransSrcRule		ruleFrom;
			AkMusicTransDestRule	ruleTo;
			AkMusicTransDestRule	ruleToTrans;
			AkMusicTransSrcRule		ruleFromTrans;
			SplitRule( rule, in_transitionInfo, ruleFrom, ruleTo, ruleToTrans, ruleFromTrans );

			// Prepare destination contexts (and get required look-ahead).
			// Note: if rule is To_SameTime or To_CueMatch, this (initial) call to Prepare() will do the same as To_PreEntry.
			// The context will have to be re-prepared after we find a sync point.
			AkUniqueID uDestCueHash;
			AkInt32 iDestinationLookAhead = io_pNewContext->Prepare( &ruleTo, 0, in_transitionInfo.pSeekingInfo, uDestCueHash );
				
			AkInt64 iMinSyncTime1, iMinSyncTime2;
			AkInt64 iSyncPoint1;

			AkInt32 iTransitionCtxLookAhead;
			AkUniqueID uTransCtxDestCueHash;

			// Used with transition context only.
			AkInt64 iSyncPoint2 = 0;
			CAkScheduleWindow wndTrans( true );

			if ( pTransition2 )
			{
				// We have 2 transitions. Prepare transition context.
				AKASSERT( pTransCtx );
				iTransitionCtxLookAhead = pTransCtx->Prepare( &ruleToTrans, 0, NULL, uTransCtxDestCueHash );

				// Compute minimum times after which we can find a sync point.
				// Note: iMinSyncTime1 must take the constraints of the second transition into account, to ensure
				// that we do not fail finding a sync point on the transition context just because the first 
				// transition was scheduled too early.
				iMinSyncTime1 = ComputeWorstMinSyncTime( iCurrentTime, ruleFrom, iTransitionCtxLookAhead, pTransCtx, ruleFromTrans, iDestinationLookAhead );
				iMinSyncTime2 = ComputeMinSyncTime( ruleFromTrans, iCurrentTime, iDestinationLookAhead );
			}
			else
			{
				iTransitionCtxLookAhead = iDestinationLookAhead;

				// Compute minimum time after which we can find a sync point.
				iMinSyncTime1 = ComputeMinSyncTime( ruleFrom, iCurrentTime, iDestinationLookAhead );
				iMinSyncTime2 = 0;	// not used.
			}
		
			AkUniqueID uSrcCueFilterHash = ruleFrom.uCueFilterHash;
			AkUniqueID uTransCtxCueFilterHash = 0;

find_sync_pos:

			// Compute transition data for first transition, try to find a point to schedule the musical transition.
			uSrcCueFilterHash = ruleFrom.uCueFilterHash;
			eReturnVal = wndSource.FindSyncPoint( 
				iMinSyncTime1,
				(AkSyncType)ruleFrom.eSyncType,
				uSrcCueFilterHash,	// Source cue filter, returned as the selected cue.
				ruleTo.RequiresIterativePreparing(), // Avoid finding a sync point on pre-entry, unless we are dealing with a "same time" destination rule.
				true,	// force succeed if source is <nothing>.
				iSyncPoint1 );

			// Behavior change: Reversals need to be more aggressive now that we have multiple switch groups. If the scheduling window is looking at 
			// the active segment and it would have been able to find a sync point within that segment if a transition wasn't already scheduled,
			// dequeue transitions right away.
			if ( eReturnVal == AK_PartialSuccess 
				&& wndSource.IsActiveSegment() )
			{
				// It would have worked within current segment. Dequeue all transitions now and proceed
				// as if it was successful.
				DequeueCancellableTransitions( pLastNonCancellableTransition, listCancelledTransitions, wndSource );
				
				// Retry finding sync position.
				eReturnVal = wndSource.FindSyncPoint( 
					iMinSyncTime1,
					(AkSyncType)ruleFrom.eSyncType,
					uSrcCueFilterHash,	// Source cue filter, returned as the selected cue.
					ruleTo.RequiresIterativePreparing(), // Avoid finding a sync point on pre-entry, unless we are dealing with a "same time" destination rule.
					true,	// force succeed if source is <nothing>.
					iSyncPoint1 );
				
				// FindSyncPoint() would have succeeded if no branching occurred within the current segment.
				// Now that we removed it, we know that it must have succeeded.
				// WG-23281: We cannot be sure that dequeuing actually worked (branching may be owned by another switch container).
				// AKASSERT( eReturnVal == AK_Success );
			}

			if ( pTransition2 
				&& eReturnVal == AK_Success )
			{
				// First transition can be scheduled, but there is another transition to try to schedule.

				// Create another scheduling window that points to the transition segment.
				SetupWindowForTransContext( pTransition1, iSyncPoint1, wndTrans );

				// Compute transition data for second transition, try to find a point to schedule the musical transition.
				eReturnVal = wndTrans.FindSyncPoint( 
					iMinSyncTime2,
					(AkSyncType)ruleFromTrans.eSyncType,
					uTransCtxCueFilterHash,
					true,	// Avoid finding a sync point on pre-entry, unless we are dealing with a "same time" destination rule.
					true,	// force succeed if source is <nothing>.
					iSyncPoint2 );
			}

			// Special handling for iterative transitions (if scheduling was successful until now).
			// Avoid trying to perform a SameTime transition when going to or coming from <nothing>, it is useless.
			// No need to check destination: preparing a <nothing> context has no effect and returns 0.
			if ( ruleTo.RequiresIterativePreparing()
				&& eReturnVal == AK_Success 
				&& !in_transitionInfo.pSeekingInfo		// Skip iterative preparing if seeking.
				&& wndSource.GetScheduledItem()->SegmentCtx() )
			{
				// Prepare destination with position corresponding to sync time in source segment (and get required look-ahead).
				if ( ruleTo.eEntryType == EntryTypeSameTime )
				{
					iDestinationLookAhead = io_pNewContext->Prepare( 
						&ruleTo, 
						wndSource.ToSegmentPosition( iSyncPoint1 ),
						NULL,
						uDestCueHash );
				}
				else
				{
					// Match Cue in source.
					AKASSERT( ruleTo.bDestMatchSourceCueName 
							&& ( ruleTo.eEntryType == EntryTypeRandomMarker || ruleTo.eEntryType == EntryTypeRandomUserMarker ) );

					iDestinationLookAhead = io_pNewContext->Prepare( 
						&ruleTo, 
						0,
						NULL,
						uDestCueHash,
						uSrcCueFilterHash );
				}

				// Check if the new required look-ahead time allows us to perform this transition.
				AkInt32 iWorstLookAhead;
				if ( pTransition2 )
					iWorstLookAhead = ComputeWorstLookAheadTime( pTransCtx, iTransitionCtxLookAhead, iDestinationLookAhead );
				else
				{
					iWorstLookAhead = iDestinationLookAhead;
					iTransitionCtxLookAhead = iDestinationLookAhead;
				}
				

				if ( iWorstLookAhead > ( iSyncPoint1 - iCurrentTime ) )
				{
					// It does not. Fix the minimal time to sync in by incrementing it with the difference 
					// between the sync time constraint and the required look-ahead, and retry.
					// This is mandatory to ensure convergence.

					// Compute new minimum position to find a sync point on source segment.
					//iMinSyncTime1 = iSyncPoint1 + ( iWorstLookAhead - ( iSyncPoint1 - iCurrentTime ) );
					iMinSyncTime1 = ( iWorstLookAhead + iCurrentTime );
					
					goto find_sync_pos;
				}
			}

			if ( eReturnVal == AK_Success )
			{
				// Transition(s) can be executed. 

				// In the process, the schedule window may have been moved along the chain of scheduled
				// items. Identify the transition that corresponds to the current branch of the 
				// schedule window. This is our new non-cancellable transition. All subsequent ones
				// will be dequeued and cancelled.
				// IMPORTANT: Cancelled transitions must be dequeued and window refreshed before calling 
				// ScheduleTransition().
				{
					CAkScheduleWindow::AutoBranchLevel branchLevel( wndSource );
					pLastNonCancellableTransition = (*wndSource.GetBranch());
					AKASSERT( pLastNonCancellableTransition 
							&& ( m_queueTransitions.FindEx( pLastNonCancellableTransition ) != m_queueTransitions.End() ) );
				}
				// Dequeue transitions.
				DequeueCancellableTransitions( pLastNonCancellableTransition, listCancelledTransitions, wndSource );

				// Now that transitions have been removed, reconsider if we really should perform a transition.
				// If this switch container is "Continue To Play" and pLastNonCancellableTransition points to
				// the same destination, we don't need to perform a transition. In such a case, bail out and
				// clean up.
				if ( !IsSwitchTransitionNeeded( in_destinationID, in_transitionInfo.pSeekingInfo, pLastNonCancellableTransition ) )
					break;

				m_pLastNonCancelledTransitionInFrame = pLastNonCancellableTransition;
				bFailedEnqueueingTransition = false;

#ifndef AK_OPTIMIZED
				AKASSERT( io_pNewContext );
				AkUniqueID idDestSegment = AK_INVALID_UNIQUE_ID;
				AkUniqueID idDestNode = AK_INVALID_UNIQUE_ID;
				CAkMusicNode * pParentNode;
				CAkMusicSegment * pSegmentNode = io_pNewContext->GetFirstSegmentNode( &pParentNode );
				if ( pSegmentNode )
					idDestNode = idDestSegment = pSegmentNode->ID();
				else 
				{
					AKASSERT( pParentNode );	// Either the segment node is set, or the parent node exists.
					idDestNode = pParentNode->ID();
				}
				
				
				// Warning: wndSource changes within ScheduleTransition(). 
				// Store ID of source segment now for monitoring.
				AkUniqueID idSrcSegment = AK_INVALID_UNIQUE_ID;
				AkUniqueID idSrcNode = AK_INVALID_UNIQUE_ID;
				pSegmentNode = wndSource.GetNode( &pParentNode );
				if ( pSegmentNode )
					idSrcNode = idSrcSegment = pSegmentNode->ID();
				else 
				{
					AKASSERT( pParentNode );	// Either the segment node is set, or the parent node exists.
					idSrcNode = pParentNode->ID();
				}
#endif

				// Schedule first transition.
				ScheduleTransition( 
					iCurrentTime, 
					pTransition1,
					ruleFrom,
					ruleToTrans,
					wndSource, 
					iTransitionCtxLookAhead,
					iSyncPoint1,
					false,
					listCancelledActions );

				if ( pTransition2 )
				{
					// Schedule second.
					ScheduleTransition( 
						iCurrentTime, 
						pTransition2,
						ruleFromTrans, 
						ruleTo,
						wndTrans, 
						iDestinationLookAhead, 
						iSyncPoint2,
						true,
						listCancelledActions );

					MONITOR_MUSICTRANS( 
						Sequencer()->PlayingID(), 
						Sequencer()->GameObjectPtr()->ID(), 
						AkMonitorData::NotificationReason_MusicTransitionScheduled, 
						rule.index, 
						m_pSwitchCntrNode->ID(), 
						idSrcNode,
						rule.pTransObj->segmentID,
						idSrcSegment, 
						rule.pTransObj->segmentID, 
						uSrcCueFilterHash, 
						uTransCtxDestCueHash, 
						AkTimeConv::SamplesToMilliseconds( AkTimeConv::ToShortRange(iSyncPoint1 - iCurrentTime) ) );
					MONITOR_MUSICTRANS( 
						Sequencer()->PlayingID(), 
						Sequencer()->GameObjectPtr()->ID(), 
						AkMonitorData::NotificationReason_MusicTransitionScheduled, 
						rule.index, 
						m_pSwitchCntrNode->ID(), 
						rule.pTransObj->segmentID,
						idDestNode, 
						rule.pTransObj->segmentID, 
						idDestSegment, 
						uTransCtxCueFilterHash, 
						uDestCueHash, 
						AkTimeConv::SamplesToMilliseconds( AkTimeConv::ToShortRange(iSyncPoint2 - iCurrentTime) ) );
				}
				else
				{
					MONITOR_MUSICTRANS( 
						Sequencer()->PlayingID(), 
						Sequencer()->GameObjectPtr()->ID(), 
						AkMonitorData::NotificationReason_MusicTransitionScheduled, 
						rule.index, 
						m_pSwitchCntrNode->ID(), 
						idSrcNode, 
						idDestNode, 
						idSrcSegment, 
						idDestSegment, 
						uSrcCueFilterHash, 
						uDestCueHash, 
						AkTimeConv::SamplesToMilliseconds( AkTimeConv::ToShortRange(iSyncPoint1 - iCurrentTime) ) );
				}
			}
			else 
			{
				// Could not schedule transition within this window. Try next one.

				// Cancel (destroy) transitions.
				if ( !pTransition2 )
					pTransition1->Dispose();	// preserves destination
				else
				{
					pTransition1->Cancel();		// destroys destination (transition context)
					pTransition2->Dispose();	// preserves destination
					pTransition2 = NULL;
				}

				// Move schedule window over next segment.
				AKASSERT( !wndSource.IsDurationInfinite() || !"Should have been able to schedule a transition already" );
				GetNextScheduleWindow( wndSource );
			}
		}
		while ( eReturnVal != AK_Success );

		// If no new transition was enqueued, destroy destination context.
		if ( bFailedEnqueueingTransition )
		{
			// Ensure transition objects are cleaned out.
			if ( pTransition1 )
			{
				// Notes: 
				// - pTransition2 cannot exist if pTransition1 does not exist.
				// - destroy final destination separately by "disposing" of the transition instead of cancelling it.
				if ( !pTransition2 )
					pTransition1->Dispose();	// preserves destination
				else
				{
					pTransition1->Cancel();		// destroys destination (transition context)
					pTransition2->Dispose();	// preserves destination
				}
			}

			// Destroy destination context.
			if ( io_pNewContext )
			{
				io_pNewContext->_Cancel();
				io_pNewContext = NULL;
			}

			if ( m_queueTransitions.Last()->CanBeRestored() )
				m_queueTransitions.Last()->Restore( iCurrentTime, 0, false );
		}

		// Reschedule cancelled actions if applicable.
		Sequencer()->RescheduleCancelledActions( listCancelledActions );
	}
	else
	{
		AKASSERT( pLastNonCancellableTransition->CanBeRestored() || pLastNonCancellableTransition == m_queueTransitions.Last() );

		// We finally decided not to enqueue a new transition. Dequeue all cancellable transitions
		// if applicable, destroy the new context and restore previous transition.
		DequeueCancellableTransitions( pLastNonCancellableTransition, listCancelledTransitions, wndSource );

		io_pNewContext->_Cancel();
		io_pNewContext = NULL;

		if ( m_queueTransitions.Last()->CanBeRestored() )
			m_queueTransitions.Last()->Restore( iCurrentTime, 0, false );
	}
    

	// Cancel transitions if applicable.
	AkTransitionsQueue::IteratorEx it = listCancelledTransitions.BeginEx();
	while ( it != listCancelledTransitions.End() )
	{
		CAkMusicSwitchTransition * pCancelledTransition = (*it);
		it = listCancelledTransitions.Erase( it );
		MONITOR_MUSICTRANS( Sequencer()->PlayingID(), Sequencer()->GameObjectPtr()->ID(), AkMonitorData::NotificationReason_MusicTransitionReverted, AK_UINT_MAX, m_pSwitchCntrNode->ID(), AK_INVALID_UNIQUE_ID, ( pCancelledTransition->Destination() ) ? pCancelledTransition->Destination()->NodeID() : AK_INVALID_UNIQUE_ID, AK_INVALID_UNIQUE_ID, AK_INVALID_UNIQUE_ID, AK_INVALID_UNIQUE_ID, AK_INVALID_UNIQUE_ID, 0 );
		pCancelledTransition->Cancel();
	}
	
	listCancelledTransitions.Term();

#ifndef AK_OPTIMIZED
	// Post "transition not scheduled" notification if applicable.
	if ( !io_pNewContext )
	{
		MONITOR_MUSICTRANS( Sequencer()->PlayingID(), Sequencer()->GameObjectPtr()->ID(), AkMonitorData::NotificationReason_MusicTransitionNoTransNeeded, AK_UINT_MAX, m_pSwitchCntrNode->ID(), AK_INVALID_UNIQUE_ID, in_destinationID, AK_INVALID_UNIQUE_ID, AK_INVALID_UNIQUE_ID, AK_INVALID_UNIQUE_ID, AK_INVALID_UNIQUE_ID, 0 );
	}
#endif
}

// Compute required look-ahead time for transition source-to-trans_segment. It is the most constraining
// between the look-ahead required for the transition segment and the look-ahead required for the destination.
AkInt32 CAkMusicSwitchCtx::ComputeWorstLookAheadTime( 
	CAkMatrixAwareCtx * in_pTransCtx,			// Context of transition segment.
	AkInt32				in_iTransCtxLookAhead,	// Required look-ahead for transition context.
	AkInt32				in_iDestCtxLookAhead	// Required look-ahead for destination context.
	)
{
	AKASSERT( in_pTransCtx );
	CAkMusicSegment * pTransSegmentNode = in_pTransCtx->GetFirstSegmentNode();
	AkUInt32 uTransSegmentActiveLength = ( pTransSegmentNode ) ? pTransSegmentNode->ActiveDuration() : 0;
	AkInt32 iWorstLookAhead = in_iDestCtxLookAhead - uTransSegmentActiveLength;
	if ( iWorstLookAhead < in_iTransCtxLookAhead )
		iWorstLookAhead = in_iTransCtxLookAhead;
	return iWorstLookAhead;
}

// Compute minimum time for a sync point given a transition rule and the required look-ahead time of the destination context.
AkInt64 CAkMusicSwitchCtx::ComputeMinSyncTime(
	const AkMusicTransSrcRule & in_rule,			// Transition rule for source.
	AkInt64				in_iCurrentTime,			// Current time.
	AkInt32				in_iDestinationLookAhead	// Required look-ahead time for destination.
	)
{
	// Compute source minimal time constraint (depends on fade time).
    AkInt32 iSrcMinTimeConstraint = 
        AkTimeConv::MillisecondsToSamples( in_rule.fadeParams.transitionTime ) -
        in_rule.fadeParams.iFadeOffset;

    // The effective time constraint is the maximum time between the source and the destination constraints.
	return in_iCurrentTime + AkMax( iSrcMinTimeConstraint, in_iDestinationLookAhead );
}

// Compute minimum time for a sync point given 2 transition rules and the required look-ahead time of both destination context.
AkInt64 CAkMusicSwitchCtx::ComputeWorstMinSyncTime( 
	AkInt64				in_iCurrentTime,			// Current time.
	const AkMusicTransSrcRule & in_rule1,			// Source transition rule of first transition.
	AkInt32				in_iLookAhead1,				// Required look-ahead of first destination.
	CAkMatrixAwareCtx * in_pTransCtx,				// Context of transition segment (first destination).
	const AkMusicTransSrcRule & in_rule2,			// Source transition rule of second transition.
	AkInt32				in_iLookAhead2 				// Required look-ahead of second destination.
	)
{
	// The minimum sync time for transition 1 is the maximum value between the time that was computed 
	// from the rules of transition 1, and the minimum time required for the sync point of transition 2,
	// minus the duration of the transition context.
	CAkMusicSegment * pTransSegmentNode = in_pTransCtx->GetFirstSegmentNode();
	AkInt32 iTransCtxDuration = ( pTransSegmentNode ) ? pTransSegmentNode->ActiveDuration() : 0;

	AkInt64 iMinSyncTime1 = ComputeMinSyncTime( in_rule1, in_iCurrentTime, in_iLookAhead1 );
	AkInt64 iMinSyncTime2 = ComputeMinSyncTime( in_rule2, in_iCurrentTime, in_iLookAhead2 ) - iTransCtxDuration;
	
	return AkMax( iMinSyncTime1, iMinSyncTime2 );
}

// Schedule a valid music transition: Create a pending switch transition
// and attach Play and Stop commands.
void CAkMusicSwitchCtx::ScheduleTransition(
	AkInt64				in_iCurrentTime,			// Current time.
	CAkMusicSwitchTransition * in_pNewTransition,	// New transition object to schedule.
	const AkMusicTransSrcRule &	in_ruleFrom,		// Source rule (details stopping previous context).
	const AkMusicTransDestRule & in_ruleTo,			// Destination rule (details starting playback of new context).
	CAkScheduleWindow & in_source,					// Scheduling window pointing on the source segment. Warning: changes within this call.
	AkInt32				in_iDestinationLookAhead,	// Required look-ahead time for transition destination.
	AkInt64				in_iSyncPoint,				// Sync point.
	bool				in_bIsFromTransitionCtx,	// True if this transition is scheduled right after a transition context.
	AssociatedActionsList & io_listCancelledActions	// Cancelled scheduled actions to reschedule.
    )
{
	CAkMusicSegment * pSrcSegmentNode = in_source.GetNode();

	CAkMusicSwitchTransition * pPreviousTrans = m_queueTransitions.Last();
	AKASSERT( pPreviousTrans );

	// Restore previous last transition.
	AKASSERT( in_iSyncPoint >= in_iCurrentTime );
	pPreviousTrans->Restore(
		in_iCurrentTime,
		AkTimeConv::ToShortRange(in_iSyncPoint - in_iCurrentTime),// Max fade-in duration. We want to be at full volume when new transition sync occurs.
		true );										// Use max fade-in duration.
		
    {
        // Create the Stop action.
        AkInt64 iStopTime = in_iSyncPoint;
        
        // If there is no fade, time to stop depends on whether or not we play
        // the post-exit. Otherwise, it depends on fade duration and offset.
		AkInt32 iFadeOutDuration = in_ruleFrom.fadeParams.transitionTime;
		AkInt32 iNumFadeOutSamplesAfterSyncPoint = 0;
        if ( iFadeOutDuration > 0 && pSrcSegmentNode != NULL )	// Do not fade out if source was <nothing>.
        {
			// Fade out: adjust time when stop should start.

			// Compute fade out time before and after sync point.
			AkInt32 iNumFadeOutSamples = AkTimeConv::MillisecondsToSamples( iFadeOutDuration );
			AkInt32 iEffectiveFadeOutDuration = iNumFadeOutSamples - in_ruleFrom.fadeParams.iFadeOffset;	// before sync point
			iNumFadeOutSamplesAfterSyncPoint = iNumFadeOutSamples - iEffectiveFadeOutDuration;	// after sync point (counterpart of iEffectiveFadeOutDuration)

			// Transition scheduling should have taken effective fade out into account.
			// Time to sync must occur later (unless source is <nothing>, in such a case we ignore fade outs).
			iStopTime -= iEffectiveFadeOutDuration;
        }
        else
        {
			// Move stop time to the end of post-exit if the rule requires it.
            if ( in_ruleFrom.bPlayPostExit &&
				 in_source.IsAtExitCue( in_iSyncPoint ) )
            {
                // Exit point is Exit Marker and rule specifies Play Post-Exit.
				AKASSERT( pSrcSegmentNode );
				iStopTime += pSrcSegmentNode->PostExitDuration();
            }
        }

		/// Behavior change (WG-18009): cut off all the time, not just with exit cue with post-exit transitions.
		// Skip cutoff if previous transition was a transition segment.
		/// Behavior change (WG-21838): avoid cutting off if there is a fade out with portion after sync point. Cut off fade outs sound bad.
		pPreviousTrans->ScheduleToStop( 
			in_iSyncPoint, 
			iStopTime, 
			iFadeOutDuration, 
			in_ruleFrom.fadeParams.eFadeCurve, 
			!in_bIsFromTransitionCtx && iNumFadeOutSamplesAfterSyncPoint <= 0 );

        // Change bucket behavior regarding Post-Exit.
		in_source.GetScheduledItem()->ForcePostExit( in_ruleFrom.bPlayPostExit );
    }

    // Schedule the Play command.
	in_pNewTransition->ScheduleToPlay( in_iSyncPoint, in_ruleTo.fadeParams, in_iDestinationLookAhead );

	// Starting with current window, iterate through the chain and pop any action that was attached
	// to a scheduled item.
	in_source.CancelActionsAfterTransitionSyncPoint( io_listCancelledActions, in_iSyncPoint );
	while ( !in_source.IsDurationInfinite() )
	{
		// Check next schedule window in "no grow" mode: last item will have infinite length.
		GetNextScheduleWindow( in_source, true );
		in_source.CancelActionsAfterTransitionSyncPoint( io_listCancelledActions, in_iSyncPoint );
	}

	// Enqueue transition.
	m_queueTransitions.AddLast( in_pNewTransition );
}

// Helpers.
// Get the appropriate transition rule.
const AkMusicTransitionRule & CAkMusicSwitchCtx::GetTransitionRule( 
    const CAkScheduleWindow & in_source,			// Scheduling window pointing on the source segment.
	CAkMatrixAwareCtx *&io_pDestContext,			// Destination context. Can be destroyed within this function (and set to NULL).
	AkUInt32 &			io_uSrcSegmentLookAhead 	// Src look-ahead limit. Passed this limit, use panic rule.
	)
{
	if ( ++io_uSrcSegmentLookAhead > MAX_SRC_SEGMENT_LOOK_AHEAD )
	{
		// Post error message if transition used the panic rule.
		MONITOR_ERROR( AK::Monitor::ErrorCode_CannotScheduleMusicSwitch );
		return CAkMusicTransAware::GetPanicTransitionRule();
	}

	// Identify source.
	CAkMusicNode * pSrcParentNode;
	CAkMusicNode * pSrcNode = in_source.GetNode( &pSrcParentNode );
	AkUniqueID srcID = ( pSrcNode ) ? pSrcNode->ID() : AK_MUSIC_TRANSITION_RULE_ID_NONE;

	// Identify destination.
	CAkScheduleWindow destination( io_pDestContext, true );
	CAkMusicNode * pDestParentNode;
	CAkMusicNode * pDestNode = destination.GetNode( &pDestParentNode );
	AkUniqueID destID = ( pDestNode ) ? pDestNode->ID() : AK_MUSIC_TRANSITION_RULE_ID_NONE;

	// Get rule.
	bool bIsDestSequenceSpecific;
    const AkMusicTransitionRule & rule = m_pSwitchCntrNode->GetTransitionRule(
		m_pSwitchCntrNode, 
		srcID,
		pSrcParentNode,
		destID,
		pDestParentNode,
		bIsDestSequenceSpecific );

	// Jump to segment if rule found applies explicitly on sequencer container.
    if ( bIsDestSequenceSpecific )
    {
        // The selected rule was specifically defined for a playlist container:
        // Reset/Initialize it to the JumpTo index specified in the rule.

		AKASSERT( destination.GetChainCtx()->Node()->NodeCategory() == AkNodeCategory_MusicRanSeqCntr );

        // Leaf sequenced segment will be changed. Swap.
		CAkSequenceCtx * pSequenceCtx = static_cast<CAkSequenceCtx*>( destination.GetChainCtx() );

		pSequenceCtx->AddRef();
        if ( !pSequenceCtx->JumpToSegment( rule.destRule.uJumpToID ) )
		{
			// Failed jumping to segment. Stop the destination before releasing the reference to pSequenceCtx,
			// as they can be the same object.
			io_pDestContext->_Cancel();
			io_pDestContext = NULL;
		}
		pSequenceCtx->Release();
    }
    return rule;
}

// Splits rule based on whether it uses a transition segment.
void CAkMusicSwitchCtx::SplitRule( 
	const AkMusicTransitionRule &	in_rule,		// Original rule. 
	const TransitionInfo &			in_transitionInfo,	// Transition info. 
	AkMusicTransSrcRule	&			out_ruleFrom,	// Returned "source" rule from original source.
	AkMusicTransDestRule &			out_ruleTo,		// Returned "destination" rule to original destination.
	AkMusicTransDestRule &			out_ruleToTrans,	// Returned "destination" rule to transition context (or original destination if none).
	AkMusicTransSrcRule	&			out_ruleFromTrans	// Returned "source" rule from transition context (or original destination if none).
	)
{
	out_ruleFrom = in_rule.srcRule;
	out_ruleTo = in_rule.destRule;

	if ( in_rule.pTransObj )
	{
		out_ruleToTrans.fadeParams = in_rule.pTransObj->fadeInParams;
		out_ruleToTrans.uCueFilterHash = AK_INVALID_UNIQUE_ID;
		out_ruleToTrans.eEntryType = EntryTypeEntryMarker;
		out_ruleToTrans.bPlayPreEntry = in_rule.pTransObj->bPlayPreEntry;
		out_ruleToTrans.bDestMatchSourceCueName = false;

		out_ruleFromTrans.fadeParams = in_rule.pTransObj->fadeOutParams;
		out_ruleFromTrans.uCueFilterHash = AK_INVALID_UNIQUE_ID;
		out_ruleFromTrans.eSyncType = SyncTypeExitMarker;
		out_ruleFromTrans.bPlayPostExit = in_rule.pTransObj->bPlayPostExit;
	}
	else
	{
		out_ruleToTrans = out_ruleTo;
		out_ruleFromTrans = out_ruleFrom;
	}

	if ( in_transitionInfo.bOverrideEntryType )
		out_ruleTo.eEntryType = in_transitionInfo.eEntryType;
}

AKRESULT CAkMusicSwitchCtx::PrepareFirstContext( 
	CAkMatrixAwareCtx * in_pCtx
    )
{
	AKASSERT( m_queueTransitions.IsEmpty() );
	AKASSERT( in_pCtx );

	// Create pending switch item.
	CAkMusicSwitchTransition * pNewSwitch = CAkMusicSwitchTransition::Create( in_pCtx );
    if ( !pNewSwitch )
		return AK_Fail;

	m_queueTransitions.AddLast( pNewSwitch );

	// Setup active switch iterator.
	m_itActiveSwitch = m_queueTransitions.Begin();

	// Prepare. By default, prepare so that entry position corresponds to the sync position.
	// For example, a child playlist would be made to start at its entry cue, regardless of 
	// its own transition rules. A parent always has preceedence on how its child starts. 
	// If this switch container plays at top level, a <nothing> context is created first 
	// (Prepare(0) applies to it) and its own <nothing>-to-this-child rule decides if it
	// should play its pre-entry. If this container is instantiated as a child of another,
	// the parent container decides depending on its own transition rules.
	AkUniqueID uSelectedCue;	// ignored.
	Prepare( NULL, 0, NULL, uSelectedCue );
	return AK_Success;
}

// Returns true if this context or any of its ascendents has a pending transition.
bool CAkMusicSwitchCtx::HasOrAscendentHasPendingTransition()
{
	AKASSERT( m_itActiveSwitch != m_queueTransitions.End() );
	AkTransQueueIter it = m_itActiveSwitch;
	if ( ++it != m_queueTransitions.End() ) 
	{
		return true;
	}
	else if ( m_pParentCtx )
	{
		// Doesn't have a pending transition, but it has a parent: query it.
		return static_cast<CAkMusicSwitchCtx*>(m_pParentCtx)->HasOrAscendentHasPendingTransition();
	}
	return false;
}

CAkMusicSwitchTransition * CAkMusicSwitchCtx::FindLastNonCancellableTransition()
{
	AKASSERT( m_itActiveSwitch != m_queueTransitions.End() );

	// Transitions are cancellable if and only if they are willing to be cancelled 
	// AND the previous may be restored, AND no subsequent transitions is irreversible.
	AkTransitionsQueue::Iterator itTransRestore = m_itActiveSwitch;
	AkTransitionsQueue::Iterator itTransCancel = m_itActiveSwitch;
	AkTransitionsQueue::Iterator itLastNonCancellableTransition = m_itActiveSwitch;
	++itTransCancel;
	while ( itTransCancel != m_queueTransitions.End() )
	{
		if ( !(*itTransRestore)->CanBeRestored() 
			|| !(*itTransCancel)->CanBeCancelled() )
		{
			// The previous transition (itTransRestore) cannot be restored, or the current transition (itTransCancel)
			// cannot be cancelled. itTransCancel points at the new non-cancellable transition.
			itLastNonCancellableTransition = itTransCancel;
		}

		// Next pair.
		itTransRestore = itTransCancel;
		++itTransCancel;
	}

	return (*itLastNonCancellableTransition);
}

void CAkMusicSwitchCtx::DequeueCancellableTransitions( 
	CAkMusicSwitchTransition * in_pLastValidTransition,
	AkTransitionsQueue & io_listCancelledTransitions,
	CAkScheduleWindow & io_wndSource
	)
{
	AkTransitionsQueue::IteratorEx it = m_queueTransitions.FindEx( in_pLastValidTransition );
	AKASSERT( it != m_queueTransitions.End() ); // Must be found in the list.

	// Dequeue all the following ones and push them in the list of cancelled transitions.
	++it;
	while ( it != m_queueTransitions.End() )
	{
		CAkMusicSwitchTransition * pTrans = (*it);
		it = m_queueTransitions.Erase( it );
		io_listCancelledTransitions.AddLast( pTrans );
	}

	// Refresh scheduling window: after dequeuing transitions, the current window length
	// may be greater, or infinite.
	RefreshWindow( io_wndSource );
}


// Matrix Aware Context implementation.
// ----------------------------------------------------------

AKRESULT CAkMusicSwitchCtx::SeekTimeAbsolute( 
	AkTimeMs & io_position,	// Seek position, in ms, relative to Entry Cue (if applicable). Returned as effective position.
	bool in_bSnapToCue		// True if final position needs to be on a cue (Exit cue excluded).
	)
{
	AkInt32 iPosition = AkTimeConv::MillisecondsToSamples( io_position );

	// Clamp position to entry cue.
	if ( iPosition < 0 )
		iPosition = 0;

	AkSeekingInfo seekingInfo;
	seekingInfo.bRelative = false;
	seekingInfo.iSeekPosition = iPosition;

	return Seek( seekingInfo, in_bSnapToCue );
}

AKRESULT CAkMusicSwitchCtx::SeekPercent( 
	AkReal32 & io_fPercent,	// Seek position, in percentage, between Entry and Exit Cues ([0,1]). Returned as effective position.
	bool in_bSnapToCue		// True if final position needs to be on a cue (Exit cue excluded).
	)
{
	AkSeekingInfo seekingInfo;
	seekingInfo.bRelative = true;
	seekingInfo.fSeekPercent = io_fPercent;

	return Seek( seekingInfo, in_bSnapToCue );
}

// Change playback position.
// The chain is destroyed and re-prepared at the correct position.
AKRESULT CAkMusicSwitchCtx::Seek( 
	AkSeekingInfo & in_seekInfo,	// Seek position in destination.
	bool /*in_bSnapToCue*/			// True if final position needs to be on a cue (Exit cue excluded).
	)
{
	// Do not seek once this context is stopping.
	if ( IsStopping() )
		return AK_Fail;

	if ( !IsTopLevel() )
		return AK_Fail;

    // Top-level context.
	// Set initial switch to <nothing>.
	// Schedule transition to current switch.
	TransitionInfo transitionInfo;
	transitionInfo.pSeekingInfo = &in_seekInfo;

	/* Not supported (although we could force rule to be "random cue" - ideally, should be "first cue after")
	if ( in_bSnapToCue ) 
		transitionInfo.OverrideEntryType( EntryTypeRandomMarker );
		*/
	
	ChangeSwitch( transitionInfo );
	
	return AK_Success;
}

