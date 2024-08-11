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
// AkMusicCtx.h
//
// Base context class for all parent contexts.
// Propagates commands to its children. Implements child removal.
//
// NOTE: Only music contexts are parent contexts, so this class is
// defined here. Move to AkAudioEngine if other standard nodes use them.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkMusicCtx.h"
#include "AkPlayingMgr.h"
#include "AkMusicSegment.h"

#include "AkTransitionManager.h"
#include "AkMusicRenderer.h"

#define AK_MIN_MUSIC_FADE_TRANSITION_TIME	(1)


CAkMusicCtx::CAkMusicCtx( 
    CAkMusicCtx *   in_parent
    )
    :CAkChildCtx( in_parent )
    ,m_uRefCount( 0 ) 
    ,m_fPlayStopFadeRatio( AK_MUTED_RATIO )
    ,m_fPauseResumeFadeRatio( AK_UNMUTED_RATIO )
	,m_uNumLastSamples( AK_NO_IN_BUFFER_STOP_REQUESTED )
	,m_uPauseCount( 0 )
	,m_eStatus( CtxStatusIdle )
    ,m_bIsPaused( false )	
{
	// Must never create a new child once leaving 'playing' state.
	AKASSERT( !in_parent || !in_parent->IsStopping() );
}

CAkMusicCtx::~CAkMusicCtx()
{
    // A music context cannot be destroyed until all its children are destroyed and removed.
    AKASSERT( m_listChildren.IsEmpty() || 
              !"A music context cannot be destroyed until all its children have been destroyed");

    // Remove transitions if applicable.
    if( m_PBTrans.pvPSTrans )
	{
        //MONITOR_OBJECTNOTIF(m_UserParams.PlayingID, m_pGameObj->ID(), m_UserParams.CustomParam, AkMonitorData::NotificationReason_Fade_Aborted, m_CntrHistArray, id, 0 );
        g_pTransitionManager->RemoveTransitionUser( m_PBTrans.pvPSTrans, this );
	}

	if( m_PBTrans.pvPRTrans )
	{
        //MONITOR_OBJECTNOTIF(m_UserParams.PlayingID, m_pGameObj->ID(), m_UserParams.CustomParam, AkMonitorData::NotificationReason_Fade_Aborted, m_CntrHistArray, id, 0 );
        g_pTransitionManager->RemoveTransitionUser( m_PBTrans.pvPRTrans, this );
	}

    m_listChildren.Term();
}

// Init: Connects to parent or to Music Renderer.
void CAkMusicCtx::Init(
    CAkRegisteredObj *,
    UserParams &
    )
{
    if ( m_pParentCtx )
        Connect();
}

// Parent-child management.
void CAkMusicCtx::AddChild(
    CAkChildCtx * in_pChildCtx
    )
{
    AKASSERT( in_pChildCtx );
    AKASSERT( !( m_listChildren.FindEx( in_pChildCtx ) != m_listChildren.End() ) );
    m_listChildren.AddFirst( in_pChildCtx );
    AddRef();
}

void CAkMusicCtx::RemoveChild( 
    CAkChildCtx * in_pChildCtx
    )
{
    AKASSERT( in_pChildCtx );
    AKRESULT eResult = m_listChildren.Remove( in_pChildCtx );
    AKASSERT( eResult == AK_Success );
    if ( eResult == AK_Success )
        Release();
}

void CAkMusicCtx::Release()
{
    AKASSERT( m_uRefCount > 0 );
    --m_uRefCount;
    if ( m_uRefCount == 0 )
    {
        if ( m_pParentCtx )
            m_pParentCtx->RemoveChild( this );
        else
            CAkMusicRenderer::Get()->RemoveChild( (CAkMatrixAwareCtx*)this ); /// TEMP. Templatize child/parent/music contexts. Avoid casts.
        AkDelete( g_DefaultPoolId, this );
    }
}

// ITransitionable implementation:
// ----------------------------------------
// Set our own virtual fade ratio.
void CAkMusicCtx::TransUpdateValue(
    AkIntPtr in_eTarget, 
    AkReal32 in_fValue, 
    bool in_bIsTerminated )
{
    bool bIsFadeOut = false;

	TransitionTargets eTarget = (TransitionTargets)in_eTarget;
    switch ( eTarget )
    {
    case TransTarget_Stop:
        bIsFadeOut = true;
    case TransTarget_Play:
        m_fPlayStopFadeRatio = in_fValue;
        SetPBIFade( &m_fPlayStopFadeRatio, m_fPlayStopFadeRatio );
        if ( in_bIsTerminated )
        {
            m_PBTrans.pvPSTrans = NULL;
            
            // Complete stop.
            // TODO (perhaps) sample accurate transitions.
            if ( bIsFadeOut )
                OnLastFrame( AK_NO_IN_BUFFER_STOP_REQUESTED );
        }
        break;
    case TransTarget_Pause:
        bIsFadeOut = true;
    case TransTarget_Resume:
        m_fPauseResumeFadeRatio = in_fValue;
        SetPBIFade( &m_fPauseResumeFadeRatio, m_fPauseResumeFadeRatio );
        if ( in_bIsTerminated )
        {
            m_PBTrans.pvPRTrans = NULL;
            
            // Complete stop if not active.
            if ( bIsFadeOut )
                OnPaused();
        }
        break;
    }
}

// Overridable methods.
// ----------------------------------------

// Notify context that it should be start playing.
void CAkMusicCtx::OnPlayed()
{
	// Update state.
    m_eStatus = CtxStatusPlaying;
}

// Child context handling implementation.
// ----------------------------------------

// Notify context that this will be the last processing frame, propagated from a high-level context Stop().
// Stop() calls OnLastFrame() when fade out is finished or context should stop within the audio frame.
// Calls OnStopped() if in_uNumSamples is 0 or in any case when stopping should occur immediately. 
// Otherwise, contexts should call OnStopped() at the end of their processing routine if IsLastFrame() is true.
void CAkMusicCtx::OnLastFrame( 
	AkUInt32 in_uNumSamples			// Number of samples left to process before stopping. 
	)								// 0 if immediate, AK_NO_IN_BUFFER_STOP_REQUESTED if it should correspond to an audio frame, whatever its size (mininum transition time).
{
	AddRef();

	// OnLastFrame can be self-destructive. See comment in OnStopped().

	ChildrenCtxList::Iterator it = m_listChildren.Begin();
	while ( it != m_listChildren.End() )
	{
		(*it)->VirtualAddRef();
		(*it)->OnLastFrame( in_uNumSamples );	// (*it) cannot be destroyed.
		++it;
	}

	it = m_listChildren.Begin();
	while ( it != m_listChildren.End() )
	{
		CAkChildCtx * pChild = (*it);	// Increment iterator before releasing the item.
		++it;
		pChild->VirtualRelease();		// pChild may be destroyed.
	}

	// Store how much samples remain to be processed by this context.
	if ( in_uNumSamples < m_uNumLastSamples && in_uNumSamples != AK_NO_IN_BUFFER_STOP_REQUESTED )
		m_uNumLastSamples = in_uNumSamples;

	// Update state. Notify OnStopped() if stopping is immediate.
	if ( m_uNumLastSamples == 0		// Stopping should occur immediately
		|| IsIdle()					// Had not started playing
		|| IsPaused() )				// Paused
	{
		OnStopped();
	}
	else if ( m_eStatus <= CtxStatusLastFrame )
	{
		m_eStatus = CtxStatusLastFrame;
	}

	Release();
}
	
// Notify context that it should be completely stopped, propagated from a high-level context Stop().
// Stop() calls either OnLastFrame() or OnStopped(). In the latter case, contexts should call OnStopped()
// at the end of their processing frame.
void CAkMusicCtx::OnStopped()
{
	// Clear playing flag.
    m_eStatus = CtxStatusStopped;
}

// Pause context playback, propagated from a high-level context _Pause().
void CAkMusicCtx::OnPaused()
{
    // Just propagate the command down to children (will ultimately affect leaf PBIs).
    ChildrenCtxList::Iterator it = m_listChildren.Begin( );
    while ( it != m_listChildren.End( ) )
    {
        (*it)->OnPaused();
        ++it;
    }

	// Set paused flag and pause play-stop transition.
	m_bIsPaused = true;
	if( m_PBTrans.pvPSTrans )
	{
		g_pTransitionManager->Pause( m_PBTrans.pvPSTrans );
	}
}

// Resume context playback, propagated from a high-level context _Resume().
void CAkMusicCtx::OnResumed()
{
    // Just propagate the command down to children.
    ChildrenCtxList::Iterator it = m_listChildren.Begin( );
    while ( it != m_listChildren.End( ) )
    {
        (*it)->OnResumed();
        ++it;
    }

	// Clear paused flag and resume play-stop transition.
    m_bIsPaused = false;
	if( m_PBTrans.pvPSTrans )
	{
		g_pTransitionManager->Resume( m_PBTrans.pvPSTrans );
	}
}

void CAkMusicCtx::OnFrameEnd() 
{
	_EndFrame();
}

void CAkMusicCtx::_EndFrame() 
{
	// OnFrameEnd can be self-destructive. See comment in OnLastFrame().
	AddRef();
	
	ChildrenCtxList::Iterator it = m_listChildren.Begin();
	while ( it != m_listChildren.End() )
	{
		CAkChildCtx * pChild = (*it);	// Increment iterator before releasing the item.
		++it;
		pChild->OnFrameEnd();	// (*it) cannot be destroyed.
	}

	if ( IsLastFrame() ) 
		OnStopped(); 

	Release();
}

// Special refcounting methods for parent contexts. After VirtualAddRef(), a child context must not be destroyed
// until VirtualRelease() is called.
void CAkMusicCtx::VirtualAddRef()
{
	AddRef();
}
void CAkMusicCtx::VirtualRelease()
{
	Release();
}

#ifndef AK_OPTIMIZED
void CAkMusicCtx::OnEditDirty()
{
	// Just propagate the command down to children.
    ChildrenCtxList::Iterator it = m_listChildren.Begin( );
    while ( it != m_listChildren.End( ) )
    {
        (*it)->OnEditDirty();
        ++it;
    }
}
#endif


// Context commands
//

// Start context playback.
// Return - AKRESULT - AK_Success if succeeded
void CAkMusicCtx::_Play( 
    TransParams & in_transParams
    )
{
    AkMusicFade fadeParams;
    fadeParams.transitionTime   = in_transParams.TransitionTime;
    fadeParams.eFadeCurve       = in_transParams.eFadeCurve;
    fadeParams.iFadeOffset      = 0;

    _Play( fadeParams );
}

// Start context playback.
// Return - AKRESULT - AK_Success if succeeded
void CAkMusicCtx::_Play( 
    AkMusicFade & in_fadeParams
    )
{
	if ( IsStopping() )
		return;

    // Play command that is not propagated. 
	// Create/update PS transition if transitionTime > 0, BUT ALSO if there is already a PS transition
	// (because it needs to be updated to avoid having a transition fighting with this new valus).
	if ( m_PBTrans.pvPSTrans )
    {
        // This context has a transition. Revert it.
	    g_pTransitionManager->ChangeParameter(
            m_PBTrans.pvPSTrans,
            TransTarget_Play,
            AK_UNMUTED_RATIO,
            in_fadeParams.transitionTime,
			in_fadeParams.eFadeCurve,
            AkValueMeaning_Default );
    }
	else if ( in_fadeParams.transitionTime > 0 )
    {
        // Otherwise create our own if duration is not null.
        TransitionParameters Params(
			this,
			TransTarget_Play,
			AK_MUTED_RATIO,
			AK_UNMUTED_RATIO,
			in_fadeParams.transitionTime,
			in_fadeParams.eFadeCurve,
			false,
			true );
	    m_PBTrans.pvPSTrans = g_pTransitionManager->AddTransitionToList(Params);
        m_PBTrans.bIsPSTransFading = true;

        if( !m_PBTrans.pvPSTrans )
		{
			// TODO : Should send a warning to tell the user that the transition is being skipped because 
			// the max num of transition was reached, or that the transition manager refused to do 
			// it for any other reason.

			// Forcing the end of transition right now.
			TransUpdateValue( Params.eTarget, Params.fTargetValue, true );
		}
        else if ( in_fadeParams.iFadeOffset != 0 )
        {
            // Use Transition Mgr's transition time-offset service.
            m_PBTrans.pvPSTrans->Offset( in_fadeParams.iFadeOffset / AK_NUM_VOICE_REFILL_FRAMES );
            /** Note. For now we count on trans mgr to update us at every frame, even when we are
            TimeRatio < 0, because new created PBIs will not inherit this property otherwise. But we suffer
            from the cost of interpolation. 
            m_uPlayStopFade = 0;
            SetPBIFade( &m_uPlayStopFade, m_uPlayStopFade );
            */
        }
    }

    /**
    // Notify Music Renderer that we just began playing, so that it can actualize its pending state changes queue.
    CAkMusicRenderer::Get()->NotifyStateChangeStartPlay( this );
    **/
    
	OnPlayed();
}

// Stop context playback.
void CAkMusicCtx::_Stop( 
    TransParams & in_transParams,
	AkUInt32 in_uNumLastSamples	// Number of samples left to process before stopping. 
	)							// 0 is "immediate", AK_NO_IN_BUFFER_STOP_REQUESTED is "one audio frame with minimal transition time". 
{
	// Create/update PS transition if transitionTime > 0, BUT ALSO if there is already a PS transition
	// (because it needs to be updated to avoid having a transition fighting with this new value).
	if ( IsPaused() )
	{
		// Stop with no transition, either immediately or within the next audio frame.
		OnLastFrame( in_uNumLastSamples );
	}
	else if ( m_PBTrans.pvPSTrans )
    {
        // This context has a transition. Revert it.
        g_pTransitionManager->ChangeParameter(
            m_PBTrans.pvPSTrans,
            TransTarget_Stop,
            AK_MUTED_RATIO,
            in_transParams.TransitionTime,
			in_transParams.eFadeCurve,
            AkValueMeaning_Default );
    }
    else if ( in_transParams.TransitionTime > 0 
			&& RequiresProcessing() )
    {
		// Requires a fade out, and this context is not already stopped or paused.
        TransitionParameters Params(
			this,
			TransTarget_Stop,
			AK_UNMUTED_RATIO,
			AK_MUTED_RATIO,
			in_transParams.TransitionTime,
			in_transParams.eFadeCurve,
			false,
			true );
	    m_PBTrans.pvPSTrans = g_pTransitionManager->AddTransitionToList(Params);
        m_PBTrans.bIsPSTransFading = true;

		if( !m_PBTrans.pvPSTrans )
		{
			// TODO : Should send a warning to tell the user that the transition is being skipped because 
			// the max num of transition was reached, or that the transition manager refused to do 
			// it for any other reason.

			// Forcing the end of transition right now.
			TransUpdateValue( Params.eTarget, Params.fTargetValue, true );
		}
    }
    else
    {
        // Stop with no transition, either immediately or within the next audio frame.
		OnLastFrame( in_uNumLastSamples );
    }
}

// Cancel context that has been created but not played yet.
void CAkMusicCtx::_Cancel()
{
	// Stop with no transition, immediately.
	OnLastFrame( 0 );
}

// Pause context playback.
void CAkMusicCtx::_Pause( 
    TransParams & in_transParams
    )
{
	/// IMPORTANT: Music contexts pausing is ALWAYS done through a transition. This resolves bug WG-8218.
	/// Thus, when OnPaused() is called, the lower engine will not pull any data, not even for this frame.
	++m_uPauseCount;

    // Create/update PR transition if transitionTime > 0, BUT ALSO if there is already a PS transition
	// (because it needs to be updated to avoid having a transition fighting with this new valus).
    if ( m_PBTrans.pvPRTrans )
    {
        // This context has a transition. Revert it.
        g_pTransitionManager->ChangeParameter(
            m_PBTrans.pvPRTrans,
            TransTarget_Pause,
            AK_MUTED_RATIO,
            in_transParams.TransitionTime,
			in_transParams.eFadeCurve,
            AkValueMeaning_Default );
    }
    else
    {
        TransitionParameters Params(
			this,
			TransTarget_Pause,
			AK_UNMUTED_RATIO,
			AK_MUTED_RATIO,
			in_transParams.TransitionTime,
			in_transParams.eFadeCurve,
			false,
			true );
	    m_PBTrans.pvPRTrans = g_pTransitionManager->AddTransitionToList(Params);
        m_PBTrans.bIsPRTransFading = true;

		if( !m_PBTrans.pvPRTrans )
		{
			// TODO : Should send a warning to tell the user that the transition is being skipped because 
			// the max num of transition was reached, or that the transition manager refused to do 
			// it for any other reason.

			// Forcing the end of transition right now.
			TransUpdateValue( Params.eTarget, Params.fTargetValue, true );
		}
    }
}

// Resume context playback.
void CAkMusicCtx::_Resume( 
    TransParams & in_transParams, 
    bool in_bIsMasterResume
    )
{
    if ( in_bIsMasterResume || m_uPauseCount <= 1 )
    {       
    	m_uPauseCount = 0;
    	
        // Resume command that is not propagated. 
        // Create/update PR transition if transitionTime > 0, BUT ALSO if there is already a PS transition
		// (because it needs to be updated to avoid having a transition fighting with this new valus).
		if ( m_PBTrans.pvPRTrans )
        {
            // This context has a transition. Revert it.
	        g_pTransitionManager->ChangeParameter(
                m_PBTrans.pvPRTrans,
                TransTarget_Resume,
                AK_UNMUTED_RATIO,
                in_transParams.TransitionTime,
				in_transParams.eFadeCurve,
                AkValueMeaning_Default );
        }
        else if ( in_transParams.TransitionTime > 0 )
        {
            // Otherwise create our own if duration is not null.
            TransitionParameters Params(
				this,
				TransTarget_Resume,
				m_fPauseResumeFadeRatio,
				AK_UNMUTED_RATIO,
				in_transParams.TransitionTime,
				in_transParams.eFadeCurve,
				false,
				true );
	        m_PBTrans.pvPRTrans = g_pTransitionManager->AddTransitionToList(Params);
            m_PBTrans.bIsPRTransFading = true;

            if( !m_PBTrans.pvPRTrans )
		    {
			    // TODO : Should send a warning to tell the user that the transition is being skipped because 
			    // the max num of transition was reached, or that the transition manager refused to do 
			    // it for any other reason.
	
			    // Forcing the end of transition right now.
			    TransUpdateValue( Params.eTarget, Params.fTargetValue, true );
		    }
        }
        else
		{
			// Reset pause resume fade in case we were paused fading. Notify PBIs below.
			m_fPauseResumeFadeRatio = AK_UNMUTED_RATIO;
			SetPBIFade( &m_fPauseResumeFadeRatio, m_fPauseResumeFadeRatio );
		}

        // Now, propagate the command down to children.
        OnResumed();
    }
    else
        --m_uPauseCount;
}


// Stop the PBI (the PBI is then destroyed)
void CAkMusicCtx::_Stop( AkPBIStopMode, bool, bool )
{
    AKASSERT( !"Not implemented" );
}

#ifndef AK_OPTIMIZED
void CAkMusicCtx::_StopAndContinue(
		AkUniqueID,
		AkUInt16,
		CAkContinueListItem*
		)
{
    AKASSERT( !"Not implemented" );
}
#endif


// Fade management. Propagate fades down to PBIs muted map
void CAkMusicCtx::SetPBIFade( 
    void * in_pOwner,
    AkReal32 in_fFadeRatio
    )
{
    ChildrenCtxList::Iterator it = m_listChildren.Begin( );
    while ( it != m_listChildren.End( ) )
    {
        (*it)->SetPBIFade( in_pOwner, in_fFadeRatio );
        ++it;
    }
}

#ifdef _DEBUG
AkUInt32 CAkMusicCtx::NumChildren()
{
	AkUInt32 uNumChildren = 0;
	ChildrenCtxList::Iterator it = m_listChildren.Begin( );
    while ( it != m_listChildren.End( ) )
    {
        ++uNumChildren;
        ++it;
    }
	return uNumChildren;
}
#endif
