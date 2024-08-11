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
// AkMusicRenderer.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkMusicRenderer.h"
#include "AkSoundBase.h"
#include "AkSource.h"
#include "AkTransitionManager.h"
#include "AkURenderer.h"
#include "AkMonitor.h"
#include "AkPlayingMgr.h"
#include "AkAudioLib.h"
#include "AkMusicNode.h"
#include "AkSegmentCtx.h"
#include "AkMusicSegment.h"
#include "AkMatrixSequencer.h"
#include "AkMatrixAwareCtx.h"
#include "AkMusicBank.h"
#include "AkMonitorData.h"
#include "ProxyMusic.h"
#include "AkMusicTransAware.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>


//-----------------------------------------------------------------------------
// Default values defines.
//-----------------------------------------------------------------------------
#define DEFAULT_STREAMING_LOOK_AHEAD_RATIO	(1.f);	// Multiplication factor for all streaming look-ahead heuristic values.


// Define interface
namespace AK
{
	namespace MusicEngine
	{
		AKRESULT Init(
			AkMusicSettings *	in_pSettings	///< Initialization settings (can be NULL, to use the default values)
			)
		{
			if( CAkMusicRenderer::Create( in_pSettings ) != NULL )
			{
				SoundEngine::RegisterGlobalCallback( CAkMusicRenderer::PerformNextFrameBehavior );
				SoundEngine::AddExternalStateHandler( CAkMusicRenderer::SetState );
				SoundEngine::AddExternalBankHandler( AkMusicBank::LoadBankItem );
#ifndef AK_OPTIMIZED
				SoundEngine::AddExternalProfileHandler( CAkMusicRenderer::HandleProfiling );

				AK::ProxyMusic::Init();
#endif
				return AK_Success;
			}
			return AK_Fail;
		}

		/// Get the default values of the initialization settings of the music engine.
		/// \sa
		/// - \ref soundengine_integration_init_advanced
		/// - AK::MusicEngine::Init()
		void GetDefaultInitSettings(
            AkMusicSettings &	out_settings	///< Returned default platform-independent sound engine settings
		    )
		{
			out_settings.fStreamingLookAheadRatio = DEFAULT_STREAMING_LOOK_AHEAD_RATIO;
		}

		void Term()
		{
			CAkMusicRenderer * pMusicRenderer = CAkMusicRenderer::Get();
			if ( pMusicRenderer )
				pMusicRenderer->Destroy( );
		}

		AKRESULT GetPlayingSegmentInfo(
			AkPlayingID		in_PlayingID,		// Playing ID returned by AK::SoundEngine::PostEvent()
			AkSegmentInfo &	out_segmentInfo,	// Position of the music segment (in ms) associated with that playing ID. The position is relative to the Entry Cue.
			bool			in_bExtrapolate		// Position is extrapolated based on time elapsed since last sound engine update.
			)
		{
			// Music position repository is owned by the music renderer.
			return CAkMusicRenderer::Get()->GetPlayingSegmentInfo( in_PlayingID, out_segmentInfo, in_bExtrapolate );
		}

	} // namespace MusicEngine
} // namespace AK

//------------------------------------------------------------------
// Defines.
//------------------------------------------------------------------
#define PENDING_STATE_CHANGES_MIN_ITEMS     (sizeof(CAkMusicRenderer::AkStateChangeRecord)/DEFAULT_POOL_BLOCK_SIZE)
#define PENDING_STATE_CHANGES_MAX_ITEMS     (AK_NO_MAX_LIST_SIZE)

//------------------------------------------------------------------
// Global variables.
//------------------------------------------------------------------
CAkMusicRenderer * CAkMusicRenderer::m_pMusicRenderer = NULL;
CAkMusicRenderer::MatrixAwareCtxList CAkMusicRenderer::m_listCtx;
CAkSegmentInfoRepository CAkMusicRenderer::m_segmentInfoRepository;
CAkMusicRenderer::PendingStateChanges CAkMusicRenderer::m_queuePendingStateChanges;
// Global music settings.
AkMusicSettings CAkMusicRenderer::m_musicSettings;
AkEvent CAkMusicRenderer::m_hTermEvent;
#ifndef AK_OPTIMIZED
bool CAkMusicRenderer::m_bEditDirty = false;
#endif

CAkMusicRenderer * CAkMusicRenderer::Create(
	AkMusicSettings *	in_pSettings
	)
{
    if ( m_pMusicRenderer )
    {
        AKASSERT( !"Should be called only once" );
        return m_pMusicRenderer;
    }

    AKASSERT( g_DefaultPoolId != AK_INVALID_POOL_ID );
    m_pMusicRenderer = AkNew( g_DefaultPoolId, CAkMusicRenderer() );
    if ( m_pMusicRenderer )
    {
        if ( m_pMusicRenderer->Init( in_pSettings ) != AK_Success )
        {
            m_pMusicRenderer->Destroy();
            m_pMusicRenderer = NULL;
        }
    }

    return m_pMusicRenderer;
}

void CAkMusicRenderer::Destroy()
{
	AK::SoundEngine::StopAll();
	// StopAll() needs to be processed by the audio thread in order for playing music objects to stop gracefully.
	AK::SoundEngine::RenderAudio();	
	
	// Note: technically we should unregister our hooks in the sound
	// engine by pushing a dedicated action in the audio mgr queue.
	// We are only safe because m_listCtx is static, and resists to
	// calls to Begin() even though it has been term'ed.
	
	while ( !m_listCtx.IsEmpty() )
	{
		AKPLATFORM::AkWaitForEvent( m_hTermEvent );
	}
	AkDestroyEvent( m_hTermEvent );
	
	CAkMusicTransAware::TermPanicTransitionRule();

    m_listCtx.Term();
	m_segmentInfoRepository.Term();
    AKVERIFY( m_queuePendingStateChanges.Term() == AK_Success );
    AkDelete( g_DefaultPoolId, this );
}

AKRESULT CAkMusicRenderer::Init(
	AkMusicSettings *	in_pSettings
	)
{
	// Store user settings.
	if ( in_pSettings )
	{
		m_musicSettings = *in_pSettings;
	}
	else
	{
		// Use defaults.
		AK::MusicEngine::GetDefaultInitSettings( m_musicSettings );
	}

	m_segmentInfoRepository.Init();

#ifndef AK_OPTIMIZED
	m_bEditDirty = false;
#endif
	
	AKRESULT eResult = AkCreateEvent( m_hTermEvent );
	if ( eResult == AK_Success )
		return m_queuePendingStateChanges.Init( PENDING_STATE_CHANGES_MIN_ITEMS, PENDING_STATE_CHANGES_MAX_ITEMS );
	return eResult;
}

CAkMusicRenderer::CAkMusicRenderer()
{
    m_pMusicRenderer = NULL;
}

CAkMusicRenderer::~CAkMusicRenderer()
{
    m_pMusicRenderer = NULL;
}

// Similar to URenderer::Play().
// Creates a Music PBI (a PBI that can be a child of a high-level context) and assigns a parent.
// Returns it
// Uses the parent's transition, the parent's game object
AKRESULT CAkMusicRenderer::Play( 
    CAkMusicCtx *		io_pParentCtx,
	CAkSoundBase*		in_pSound,
	CAkSource *			in_pSource,
    CAkRegisteredObj *	in_pGameObj,
    TransParams &		in_transParams,
    UserParams&			in_rUserparams,
	const AkTrackSrc *	in_pSrcInfo,		// Pointer to track's source playlist item.
    //AkPlaybackState	in_ePlaybackState,
	AkUInt32			in_uSourceOffset,	// Start position of source (in samples, at the native sample rate).
    AkUInt32			in_uFrameOffset,    // Frame offset for look-ahead and LEngine sample accuracy.
    CAkMusicPBI *&		out_pPBI            // TEMP: Created PBI is needed to set the transition from outside.
    )
{
	AKRESULT eResult = AK_Fail;
	AkMonitorData::NotificationReason eReason;
    // Check parameters.
    AKASSERT( in_pSound != NULL );
    if( in_pSound == NULL )
        return AK_InvalidParameter;

	AkReal32 fMaxRadius;
	PriorityInfoCurrent priority = CAkURenderer::_CalcInitialPriority( in_pSound, in_pGameObj, fMaxRadius );

	AKRESULT eValidateLimitsResult = CAkURenderer::ValidateLimits( priority.GetCurrent(), eReason );
	bool bAllowedToPlay = eValidateLimitsResult != AK_Fail;
	if( bAllowedToPlay )
	{
		CounterParameters counterParams;
		counterParams.fPriority = priority.GetCurrent();
		counterParams.pGameObj = in_pGameObj;
		AKRESULT eIncrementPlayCountResult = in_pSound->IncrementPlayCount( counterParams );

		bAllowedToPlay = eIncrementPlayCountResult != AK_Fail;
		if( eIncrementPlayCountResult == AK_MustBeVirtualized || eValidateLimitsResult == AK_MustBeVirtualized )
		{
			//Update bAllowedToPlay based on the behavior of the sound
			AkVirtualQueueBehavior _unused;
			bAllowedToPlay = CAkURenderer::GetVirtualBehaviorAction( 
				in_pSound->GetVirtualBehavior( _unused ) 
				);
		}

		if ( bAllowedToPlay )
		{
			AkPathInfo PathInfo = { NULL, AK_INVALID_UNIQUE_ID };
			// We don't care about the play history. TODO Get rid of it.
			PlayHistory history;
    		history.Init();
			out_pPBI = AkNew( RENDERER_DEFAULT_POOL_ID, 
				CAkMusicPBI( io_pParentCtx,
							 in_pSound,
							 in_pSource,
							 in_pGameObj,
							 in_rUserparams,
							 in_pSrcInfo,
							 history,     
							 AK_INVALID_SEQUENCE_ID,
							 priority,
							 in_uSourceOffset,
							 counterParams.pAMLimiter,
							 counterParams.pBusLimiter
							 ) );

			if( out_pPBI != NULL )
			{
				if( out_pPBI->Init( &PathInfo ) == AK_Success )
				{
					out_pPBI->SetMaxDistance( fMaxRadius );
					out_pPBI->SetFrameOffset( in_uFrameOffset );
					Play( out_pPBI, in_transParams/*, in_ePlaybackState*/ );
					return AK_Success;
				}
				else
				{
					out_pPBI->Term( true ); //does call DecrementPlayCount()
					AkDelete( RENDERER_DEFAULT_POOL_ID, out_pPBI );
					out_pPBI = NULL;
					return eResult;
				}
			}
		}
		else
		{
			eResult = AK_PartialSuccess;
			eReason = AkMonitorData::NotificationReason_PlayFailedLimit;
		}

		{
			// We need a new CounterParameters param here, with initial defaults values.
			CounterParameters counterParamsError;
			counterParamsError.pGameObj = in_pGameObj;
			// Either ran out of memory, or insufficient priority to play
			in_pSound->DecrementPlayCount( counterParamsError );
		}
	}
	else
	{
		eResult = AK_PartialSuccess;
	}

	if( eResult == AK_PartialSuccess )
	{
		PlayHistory playHistory;
		playHistory.Init();

		in_pSound->MonitorNotif(
			eReason,
			in_pGameObj->ID(),
			in_rUserparams,
			playHistory );
	}

	return eResult;
} // Play


void CAkMusicRenderer::Play(	
    CAkMusicPBI *   in_pContext, 
    TransParams &   in_transParams
    /*,
	AkPlaybackState	in_ePlaybackState*/
	)
{
	// Add PBI context to Upper Renderer's list.
    CAkURenderer::EnqueueContext( in_pContext );
	
    in_pContext->_InitPlay();
    
	bool l_bPaused = false;
    /* TODO
	// Check if the play command is actually a play-pause.
	if( in_ePlaybackState == PB_Paused )
	{
		l_bPaused = true;
	}
    */

	in_pContext->_Play( in_transParams, l_bPaused, true );
}

// Stops all top-level contexts.
bool CAkMusicRenderer::StopAll()
{
	bool bWasSomethingStopped = false;
    // Look among our top-level children.
	// Note. Contexts may dequeue themselves while being stopped
	MatrixAwareCtxList::Iterator it = m_listCtx.Begin();
	while ( it != m_listCtx.End() )
	{
		// Cache our pointer on the stack, as it could self-destruct inside OnStopped().
		CAkMatrixAwareCtx * pCtx = (*it);
		++it;
		TransParams transParams;
		transParams.TransitionTime = 0;
		transParams.eFadeCurve = AkCurveInterpolation_Linear;
		pCtx->_Stop( transParams, 0 );	// Stop immediately.
		bWasSomethingStopped = true;
	}

	return bWasSomethingStopped;
}


// Game triggered actions (stop/pause/resume).
void CAkMusicRenderer::Stop(	
    CAkMusicNode *      in_pNode,
    CAkRegisteredObj *  in_pGameObj,
    TransParams &       in_transParams,
	AkPlayingID			in_playingID
    )
{
    // Look among our top-level children.
    // Note. Contexts may dequeue themselves while being stopped.
    MatrixAwareCtxList::Iterator it = m_listCtx.Begin();
	while ( it != m_listCtx.End() )
    {
		// Cache our pointer on the stack, as it could self-destruct inside OnStopped().
        CAkMatrixAwareCtx * pCtx = (*it);
		++it;

        if( pCtx->Node() == in_pNode )
		{
			if( CheckObjAndPlayingID( in_pGameObj, pCtx->Sequencer()->GameObjectPtr(), in_playingID, pCtx->Sequencer()->PlayingID() ) )
			{
				pCtx->_Stop( in_transParams );
			}
		}
    }
}

void CAkMusicRenderer::Pause(	
    CAkMusicNode *      in_pNode,
    CAkRegisteredObj *  in_pGameObj,
    TransParams &       in_transParams,
	AkPlayingID			in_playingID
    )
{
    // Look among our top-level children.
    MatrixAwareCtxList::Iterator it = m_listCtx.Begin();
	while ( it != m_listCtx.End() )
    {
        if( (*it)->Node() == in_pNode )
		{
			if( CheckObjAndPlayingID( in_pGameObj, (*it)->Sequencer()->GameObjectPtr(), in_playingID, (*it)->Sequencer()->PlayingID() ) )
			{
                (*it)->_Pause( in_transParams );
			}
		}
        ++it;
	} 
}

void CAkMusicRenderer::Resume(	
    CAkMusicNode *      in_pNode,
    CAkRegisteredObj *  in_pGameObj,
    TransParams &       in_transParams,
    bool                in_bMasterResume,    // REVIEW
	AkPlayingID			in_playingID
    )
{
    // Look among our top-level children.
    MatrixAwareCtxList::Iterator it = m_listCtx.Begin();
	while ( it != m_listCtx.End() )
    {
        if( (*it)->Node() == in_pNode )
		{
			if( CheckObjAndPlayingID( in_pGameObj, (*it)->Sequencer()->GameObjectPtr(), in_playingID, (*it)->Sequencer()->PlayingID() ) )
			{
                (*it)->_Resume( in_transParams, in_bMasterResume );
			}
		}
        ++it;
	}
}

void CAkMusicRenderer::SeekTimeAbsolute(	
    CAkMusicNode *		in_pNode,
    CAkRegisteredObj *  in_pGameObj,
	AkTimeMs			in_iSeekTime,
	bool				in_bSnapToCue
	)
{
	// Search segment contexts among our top-level children.
    MatrixAwareCtxList::Iterator it = m_listCtx.Begin();
	while ( it != m_listCtx.End() )
    {
		// Cache pointer in case destruction occurs while iterating in the list.
		CAkMatrixAwareCtx * pCtx = (*it);
		++it;
        if( pCtx->Node() == in_pNode )
		{
			if( !in_pGameObj || pCtx->Sequencer()->GameObjectPtr() == in_pGameObj )
			{
				if ( pCtx->SeekTimeAbsolute( in_iSeekTime, in_bSnapToCue ) == AK_Success )
				{
					UserParams & userParams = pCtx->Sequencer()->GetUserParams();
					g_pPlayingMgr->NotifyMusicPlayStarted( userParams.PlayingID() );
					MONITOR_OBJECTNOTIF(userParams.PlayingID(), pCtx->Sequencer()->GameObjectPtr()->ID(), userParams.CustomParam(), AkMonitorData::NotificationReason_Seek, CAkCntrHist(), pCtx->Node()->ID(), false, in_iSeekTime );
				}
				else
				{
					MONITOR_ERRORMSG( "Music Renderer: Seeking failed" );
				}
			}
		}
	} 
}

void CAkMusicRenderer::SeekPercent(	
    CAkMusicNode *		in_pNode,
    CAkRegisteredObj *  in_pGameObj,
	AkReal32			in_fSeekPercent,
	bool				in_bSnapToCue
	)
{
	// Search segment contexts among our top-level children.
    MatrixAwareCtxList::Iterator it = m_listCtx.Begin();
	while ( it != m_listCtx.End() )
    {
		// Cache pointer in case destruction occurs while iterating in the list.
		CAkMatrixAwareCtx * pCtx = (*it);
		++it;
        if( pCtx->Node() == in_pNode )
		{
			if( !in_pGameObj || pCtx->Sequencer()->GameObjectPtr() == in_pGameObj )
			{
				if ( pCtx->SeekPercent( in_fSeekPercent, in_bSnapToCue ) == AK_Success )
				{
					UserParams & userParams = pCtx->Sequencer()->GetUserParams();
					g_pPlayingMgr->NotifyMusicPlayStarted( userParams.PlayingID() );
					AkReal32 fPercent = in_fSeekPercent*100;
					MONITOR_OBJECTNOTIF(userParams.PlayingID(), pCtx->Sequencer()->GameObjectPtr()->ID(), userParams.CustomParam(), AkMonitorData::NotificationReason_SeekPercent, CAkCntrHist(), pCtx->Node()->ID(), false, *(AkTimeMs*)&fPercent );
				}
				else
				{
					MONITOR_ERRORMSG( "Music Renderer: Seeking failed" );
				}
			}
		}
	} 
}

// Add/Remove Top-Level Music Contexts (happens when created from MusicNode::Play()).
AKRESULT CAkMusicRenderer::AddChild( 
    CAkMatrixAwareCtx * in_pMusicCtx,
    UserParams &        in_rUserparams,
    CAkRegisteredObj *  in_pGameObj        
    )
{
	AKRESULT eResult = AK_Fail;
    // Create and enqueue a top-level sequencer.
    CAkMatrixSequencer * pSequencer = AkNew( g_DefaultPoolId, CAkMatrixSequencer( in_pMusicCtx, in_rUserparams, in_pGameObj ) );
    if ( pSequencer )
    {
        CAkMusicNode * pNode = in_pMusicCtx->Node();
		if ( pNode && pNode->IncrementActivityCount() )
		{
			m_listCtx.AddFirst( in_pMusicCtx );

            // TODO Enforce do not set sequencer elsewhere than here.
            in_pMusicCtx->SetSequencer( pSequencer );

            // We generated a Top-Level context and sequencer:
            // Register/Add ref to the Playing Mgr, so that it keeps the playing ID alive.
            if ( in_rUserparams.PlayingID() )
            {
				// IMPORTANT: Initialize callback flags because PlayingMgr does not set them if the entry
				// does not exist.
				AkUInt32 uCallbackFlags = 0;

                AKASSERT( g_pPlayingMgr );
                eResult = g_pPlayingMgr->SetPBI( in_rUserparams.PlayingID(), in_pMusicCtx, &uCallbackFlags );

				in_pMusicCtx->SetRegisteredNotif( uCallbackFlags );

				if ( uCallbackFlags & AK_EnableGetMusicPlayPosition )
				{
					if ( m_segmentInfoRepository.CreateEntry( in_rUserparams.PlayingID() ) != AK_Success )
					{
						// Failed creating an entry in the repository. Go on but ignore segment info queries.
						in_pMusicCtx->SetRegisteredNotif( uCallbackFlags & ~AK_EnableGetMusicPlayPosition );
					}
				}
				g_pPlayingMgr->NotifyMusicPlayStarted( in_rUserparams.PlayingID() );
            }
   		}
        else
        {
        	if ( pNode )
        		pNode->DecrementActivityCount();

			// Destroy sequencer now if it wasn't assigned to the context yet. Otherwise, let contexts
			// remove themselves normally from the renderer.
            AkDelete( g_DefaultPoolId, pSequencer );
        }
    }

    return eResult;
}
void CAkMusicRenderer::RemoveChild( 
    CAkMatrixAwareCtx * in_pMusicCtx
    )
{
    // Note: This call may fail if the context was never added to the renderer's children, because 
	// CAkMusicRenderer::AddChild() failed (because no memory).
    m_listCtx.Remove( in_pMusicCtx );
    
	// Note: The context may not have a sequencer if it was not created because of an out-of-memory condition.
    CAkMatrixSequencer * pSequencer = in_pMusicCtx->Sequencer();
	if( pSequencer )
	{
		// Notify Playing Mgr.
		AKASSERT(g_pPlayingMgr);
		if ( pSequencer->PlayingID() )
		{
			if ( in_pMusicCtx->GetRegisteredNotif() & AK_EnableGetMusicPlayPosition )
				m_segmentInfoRepository.RemoveEntry( pSequencer->PlayingID() );

			g_pPlayingMgr->Remove( pSequencer->PlayingID(), in_pMusicCtx );
			if( in_pMusicCtx->Node() )
				in_pMusicCtx->Node()->DecrementActivityCount();
		}
	
		AkDelete( g_DefaultPoolId, pSequencer );
	}

	AKPLATFORM::AkSignalEvent( m_hTermEvent );
}

// Music Audio Loop interface.
void CAkMusicRenderer::PerformNextFrameBehavior( bool /*in_bLastCall*/ )
{
	// Refresh context hierarchies if sensitive live editing was performed.
#ifndef AK_OPTIMIZED
	if ( IsEditDirty() )
	{
		MatrixAwareCtxList::Iterator it = m_listCtx.Begin();
		while ( it != m_listCtx.End() )
		{
			(*it)->OnEditDirty();
			++it;
		}
		ClearEditDirty();
	}
#endif 

    // Perform top-level segment sequencers.
	MatrixAwareCtxList::Iterator it = m_listCtx.Begin();
	while ( it != m_listCtx.End() )
    {
		// Cache our pointer on the stack in case dequeueing occurs from within Execute().
        CAkMatrixAwareCtx * pCtx = (*it);
		++it;

		CAkMatrixSequencer * pSequencer = pCtx->Sequencer();

		// Store current segment info before preparing context for next audio frame.
		if ( pCtx->GetRegisteredNotif() & AK_EnableGetMusicPlayPosition )
		{
			AkSegmentInfo segmentInfo;
			if ( pCtx->GetPlayingSegmentInfo( segmentInfo ) == AK_Success )
				m_segmentInfoRepository.UpdateSegmentInfo( pSequencer->PlayingID(), segmentInfo );
		}
		
		// Execute logic for next audio frame.
		pSequencer->Execute( AK_NUM_VOICE_REFILL_FRAMES );
	}
}

// Music objects queries.
AKRESULT CAkMusicRenderer::GetPlayingSegmentInfo(
	AkPlayingID		in_PlayingID,		// Playing ID returned by AK::SoundEngine::PostEvent()
	AkSegmentInfo &	out_segmentInfo,	// Position of the music segment (in ms) associated with that playing ID. The position is relative to the Entry Cue.
	bool			in_bExtrapolate		// Position is extrapolated based on time elapsed since last sound engine update.
	)
{
	return m_segmentInfoRepository.GetSegmentInfo( in_PlayingID, out_segmentInfo, in_bExtrapolate ); 
}

//
// States management.
//

// Returns true if state change was handled (delayed) by the Music Renderer. False otherwise.
bool CAkMusicRenderer::SetState( 
    AkStateGroupID     in_stateGroupID, 
    AkStateID          in_stateID
    )
{
    // Query top-level context sequencers that need to handle state change by delaying it.

    CAkMatrixAwareCtx * pChosenCtx = NULL;

    // Values for chosen context.
    AkInt64 iChosenRelativeSyncTime;  
    AkUInt32 uChosenSegmentLookAhead;

    if ( GetDelayedStateChangeData(
            in_stateGroupID,
            pChosenCtx,
            iChosenRelativeSyncTime,
            uChosenSegmentLookAhead ) <= 0 )
    {
        // Either a context requires an immediate change, or no one is registered to this state group.
        // Return now as "not handled".        
        return false;
    }


    //
    // Process delayed state change.
    // 
    AKASSERT( pChosenCtx );

    // Reserve a spot for pending state change.
    AkStateChangeRecord * pNewStateChange = m_queuePendingStateChanges.AddFirst();
    if ( !pNewStateChange )
    {
        // No memory. Return without handling delayed state change.
        // Invalidate all pending state changes that relate to this state group.
        PendingStateChangeIter it = m_queuePendingStateChanges.Begin();
    	InvalidateOlderPendingStateChanges( it, in_stateGroupID );
        return false;
    }

    
    // Delegate handling of delayed state change sequencing to the appropriate context sequencer.
    if ( pChosenCtx->Sequencer()->ProcessDelayedStateChange( pNewStateChange, 
                                                             uChosenSegmentLookAhead, 
                                                             iChosenRelativeSyncTime ) == AK_Success )
    {
        // Setup the rest of the state change record.
        pNewStateChange->stateGroupID   = in_stateGroupID;
        pNewStateChange->stateID        = in_stateID;
        pNewStateChange->bWasPosted     = false;
		pNewStateChange->bIsReferenced	= true;

        // Return True ("handled") unless a context required an immediate change.
        return true;
    }
    else
    {
        // Failed handling delayed state change.
        // Remove the record, return False ("not handled").
        AKVERIFY( m_queuePendingStateChanges.RemoveFirst() == AK_Success );
        return false;
    }
}

// Execute a StateChange music action.
void CAkMusicRenderer::PerformDelayedStateChange(
    void *             in_pCookie
    )
{
    // Find pending state change in queue. 
    PendingStateChangeIterEx it;
	// Post state change if required (if was not already posted).
	FindPendingStateChange( in_pCookie, it );
	(*it).bIsReferenced = false;
    if ( !(*it).bWasPosted )
    {
        (*it).bWasPosted = true;
		
        AkStateGroupID stateGroupID = (*it).stateGroupID;
        //
        // Set state on sound engine, with flag "skip call to state handler extension".
        // 
        AKVERIFY( AK::SoundEngine::SetState( 
            stateGroupID, 
            (*it).stateID, 
            false, 
            true ) == AK_Success ); 

		// Invalidate all older pending state changes (for this StateGroup ID).
        InvalidateOlderPendingStateChanges( ++it, stateGroupID );
    }
	// else State change is obsolete.
	
    
    // Clean up queue.
    CleanPendingStateChanges();
}

// Notify Renderer whenever a StateChange music action needs to be rescheduled.
void CAkMusicRenderer::RescheduleDelayedStateChange(
    void *              in_pCookie
    )
{
    // Find pending state change in queue. 
    PendingStateChangeIterEx it;    
    FindPendingStateChange( in_pCookie, it );
	if ( !(*it).bWasPosted )
	{
		AkStateGroupID stateGroupID = (*it).stateGroupID;

		// Values for chosen context.
		AkInt64 iChosenRelativeSyncTime;  
		AkUInt32 uChosenSegmentLookAhead;
		CAkMatrixAwareCtx * pChosenCtx = NULL;

		if ( GetDelayedStateChangeData(
				stateGroupID, 
				pChosenCtx,
				iChosenRelativeSyncTime,
				uChosenSegmentLookAhead ) <= 0 )
		{
			// Either a context requires an immediate change, or no one is registered to this state group.
			CancelDelayedStateChange( stateGroupID, it );
			return;
		}

		AKASSERT( pChosenCtx );

		//
		// Process delayed state change.
		// 
	    
		// Delegate handling of delayed state change sequencing to the appropriate context sequencer.
		if ( pChosenCtx->Sequencer()->ProcessDelayedStateChange( in_pCookie, 
																 uChosenSegmentLookAhead, 
																 iChosenRelativeSyncTime ) != AK_Success )
		{
			// Failed handling delayed state change.
			// Set state on sound engine now, remove the record.
			CancelDelayedStateChange( stateGroupID, it );
		}
	}
	else 
	{
		// State change was obsolete anyway, so mark it as "not referenced" and clean now.
		(*it).bIsReferenced = false;
		CleanPendingStateChanges();
	}
}


// Helpers.

// Set state on sound engine now, with flag "skip call to state handler extension".
// Clean pending delayed state change list.
void CAkMusicRenderer::CancelDelayedStateChange( 
    AkStateGroupID     in_stateGroupID, 
    PendingStateChangeIterEx & in_itPendingStateChg
    )
{
    //
    // Set state on sound engine now, with flag "skip call to state handler extension".
    // 
    AKVERIFY( AK::SoundEngine::SetState( 
        in_stateGroupID, 
        (*in_itPendingStateChg).stateID, 
        false, 
        true ) == AK_Success ); 

    // Invalidate record, clean.
    (*in_itPendingStateChg).bWasPosted = true;
	(*in_itPendingStateChg).bIsReferenced = false;
    InvalidateOlderPendingStateChanges( in_itPendingStateChg, in_stateGroupID );
    CleanPendingStateChanges();
}

// Query top-level context sequencers that need to handle state change by delaying it.
// Returns the minimal absolute delay for state change. Value <= 0 means "immediate".
AkInt64 CAkMusicRenderer::GetDelayedStateChangeData(
    AkStateGroupID          in_stateGroupID, 
    CAkMatrixAwareCtx *&    out_pChosenCtx,
    AkInt64 &               out_iChosenRelativeSyncTime,
    AkUInt32 &              out_uChosenSegmentLookAhead
    )
{
    AkInt64 iEarliestAbsoluteDelay = 0;
    out_pChosenCtx = NULL;

    MatrixAwareCtxList::Iterator itCtx = m_listCtx.Begin();
	while ( itCtx != m_listCtx.End() )
    {
		if ( (*itCtx)->IsPlaying()
			&& !(*itCtx)->IsPaused() )
		{
			AkInt64 iRelativeSyncTime;  // state change time relative to segment
			AkUInt32 uSegmentLookAhead;

			AkInt64 iAbsoluteDelay = (*itCtx)->Sequencer()->QueryStateChangeDelay( in_stateGroupID, 
																				   uSegmentLookAhead,
																				   iRelativeSyncTime );
			if ( !out_pChosenCtx || 
				 iAbsoluteDelay < iEarliestAbsoluteDelay )
			{
				// This context requires a state change that should occur the earliest.
				iEarliestAbsoluteDelay = iAbsoluteDelay;

				out_iChosenRelativeSyncTime = iRelativeSyncTime;
				out_uChosenSegmentLookAhead = uSegmentLookAhead;
				out_pChosenCtx = (*itCtx);
			}
		}
        ++itCtx;
    }

    // NOTE. Since delayed processing always occurs one frame later, we substract one frame size out of the returned
    // delay. (Delays smaller than one frame will be considered as immediate).
    iEarliestAbsoluteDelay -= AK_NUM_VOICE_REFILL_FRAMES;
    return iEarliestAbsoluteDelay;
}

void CAkMusicRenderer::FindPendingStateChange( 
    void * in_pCookie,
    PendingStateChangeIterEx & out_iterator
    )
{
    out_iterator = m_queuePendingStateChanges.BeginEx();
    while ( out_iterator != m_queuePendingStateChanges.End() )
    {
        if ( &(*out_iterator) == in_pCookie )
        {
            // Found.
            break;
        }
        ++out_iterator;
    }

    // Must have been found.
    AKASSERT( out_iterator != m_queuePendingStateChanges.End() );
}

void CAkMusicRenderer::CleanPendingStateChanges()
{
    PendingStateChangeIterEx it = m_queuePendingStateChanges.BeginEx();
    while ( it != m_queuePendingStateChanges.End() )
    {
        // Dequeue if required (if ref count is 0).
        if ( !(*it).bIsReferenced )
        {
        	AKASSERT( (*it).bWasPosted );
            it = m_queuePendingStateChanges.Erase( it );
        }
        else
            ++it;
    }
}

void CAkMusicRenderer::InvalidateOlderPendingStateChanges( 
    PendingStateChangeIter & in_iterator,
    AkStateGroupID           in_stateGroupID
    )
{
    while ( in_iterator != m_queuePendingStateChanges.End() )
    {
        // Find next (older) pending state change with that same StateGroup ID.
        if ( (*in_iterator).stateGroupID == in_stateGroupID )
        {
            // Invalidate.
            (*in_iterator).bWasPosted = true;
        }
        ++in_iterator;
    }
}

#ifndef AK_OPTIMIZED
void CAkMusicRenderer::HandleProfiling()
{
	/*
	Get and post :
		SegmentID, 
		PlayingID 
		and position (in double 64 in ms)
	*/

	// We must first count them to make the initial allocation (only if required)
	AkUInt16 uNumPlayingIM = 0;
	MatrixAwareCtxList::Iterator it = m_listCtx.Begin();
	while ( it != m_listCtx.End() )
	{
		CAkMatrixAwareCtx * pCtx = (*it);
		++it;
		if( pCtx->Node()->NodeCategory() == AkNodeCategory_MusicSegment 
			&& pCtx->Sequencer()->GetUserParams().CustomParam().ui32Reserved & AK_EVENTFROMWWISE_RESERVED_BIT )
		{
			++uNumPlayingIM;
		}
	}
	if( uNumPlayingIM )
	{
		// We do have something to monitor, so let's gather the info.
		AkInt32 sizeofItem = SIZEOF_MONITORDATA_TO( segmentPositionData.positions )
						+ uNumPlayingIM * sizeof( AkMonitorData::SegmentPositionData );

		AkProfileDataCreator creator( sizeofItem );
		if ( !creator.m_pData )
			return;

		creator.m_pData->eDataType = AkMonitorData::MonitorDataSegmentPosition;

		creator.m_pData->segmentPositionData.numPositions = uNumPlayingIM;

		uNumPlayingIM = 0;
		it = m_listCtx.Begin();
		while ( it != m_listCtx.End() )
		{
			CAkMatrixAwareCtx * pCtx = (*it);
			++it;
			CAkMatrixSequencer* pSequencer = pCtx->Sequencer();
			if( pCtx->Node()->NodeCategory() == AkNodeCategory_MusicSegment 
			&& ( pSequencer->GetUserParams().CustomParam().ui32Reserved & AK_EVENTFROMWWISE_RESERVED_BIT )
			&& pCtx->IsPlaying() )
			{	
				AkMonitorData::SegmentPositionData& l_rdata = creator.m_pData->segmentPositionData.positions[ uNumPlayingIM ];
				AkInt32 iCurSegmentPosition = pSequencer->GetCurSegmentPosition();//in samples
				if( iCurSegmentPosition <= 0 )
				{
					l_rdata.f64Position = 0;//negative stands for not started yet, so we pass 0.
				}
				else
				{
					l_rdata.f64Position =	AkTimeConv::SamplesToSeconds( iCurSegmentPosition )*1000;
				}
				l_rdata.playingID =		pSequencer->PlayingID();
				l_rdata.segmentID =		pCtx->Node()->ID();
				l_rdata.customParam =	pSequencer->GetUserParams().CustomParam();

				++uNumPlayingIM;
			}
		}
	}
}
#endif
