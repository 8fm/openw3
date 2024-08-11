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
#include "AkMatrixAwareCtx.h"
#include "AkMusicRenderer.h"
#include "AkMatrixSequencer.h"
#include "AkMusicSegment.h"
#include "AkScheduleWindow.h"
#include "AkMonitor.h"

CAkMatrixAwareCtx::CAkMatrixAwareCtx(
    CAkMusicCtx *   in_parent // Parent context. NULL if this is a top-level context.
    )
:CAkMusicCtx( in_parent )
,m_pSequencer( NULL )
,m_iLocalTime( 0 )
{
}

CAkMatrixAwareCtx::~CAkMatrixAwareCtx()
{
}

AKRESULT CAkMatrixAwareCtx::Init(
    CAkRegisteredObj *  in_pGameObj,
    UserParams &        in_rUserparams
    )
{
    CAkMusicCtx::Init( in_pGameObj, in_rUserparams );
    if ( m_pParentCtx )
    {
        // Inherit our parent's Sequencer.
        SetSequencer( static_cast<CAkMatrixAwareCtx*>(m_pParentCtx)->Sequencer() );
		return AK_Success;
    }
    else
    {
        // We are a top-level context. Add ourselves to the Music Renderer.
        return CAkMusicRenderer::Get()->AddChild( this, in_rUserparams, in_pGameObj );
    }
}

// Called by parent (switches): completes the initialization.
// Default implementation does nothing.
void CAkMatrixAwareCtx::EndInit()
{
}

// Get the look-ahead duration of this context that is strictly due to streaming.
AkUInt32 CAkMatrixAwareCtx::GetSilentDuration()
{
	AkInt64 iPlayTime;
	AkInt64 iPlayTimeAudible;
	QueryLookAheadInfo( iPlayTime, iPlayTimeAudible );
	// "Real" play time happens earlier than play time without look-ahead due to streaming.
	AKASSERT( iPlayTime <= iPlayTimeAudible );
	// Note: This sequence has not started playing yet. We can assume its sync offsets are well within 32 bits.
	return AkTimeConv::ToShortRange( iPlayTimeAudible - iPlayTime );
}

// Change playback position if this is supported by the context.
// Default implementation does not support this.
AKRESULT CAkMatrixAwareCtx::SeekTimeAbsolute( 
	AkInt32 & /*io_iPosition */,
	bool /*in_bSnapToCue */
	)
{
	AKASSERT( !"Cannot seek within this type of music object" );
	return AK_Fail;
}

AKRESULT CAkMatrixAwareCtx::SeekPercent( 
	AkReal32 & /*io_fPercent*/,
	bool /*in_bSnapToCue */
	)
{
	AKASSERT( !"Cannot seek within this type of music object" );
	return AK_Fail;
}

// Music object queries.
AKRESULT CAkMatrixAwareCtx::GetPlayingSegmentInfo(
	AkSegmentInfo &	out_segmentInfo		
	)
{
	CAkScheduleWindow window( this );
	if ( window.IsValid() )
	{
		CAkScheduledItem * pItem = window.GetScheduledItem();
		if ( pItem )
			return pItem->GetInfo( window.ToSegmentPosition( GlobalToLocalTime( Sequencer()->Now() ) ), out_segmentInfo );
	}	
	return AK_Fail;
}

void CAkMatrixAwareCtx::OnStopped()
{
	CAkMusicCtx::OnStopped();

	// Notify sequencer if this context is top-level: need to clean up stingers.
	AKASSERT( !RequiresProcessing() );
	if ( IsTopLevel() && Sequencer() )	// WG-20190: At this point, the sequencer might not have been created if we were out of memory at ctx init.
		Sequencer()->OnStopped();
}

#ifndef AK_OPTIMIZED
void CAkMatrixAwareCtx::OnPaused()
{
	// NOTE: OnPaused() is called without consideration to pause count.
	if ( IsTopLevel() && !IsPaused() )
	{
		MONITOR_MUSICOBJECTNOTIF( m_pSequencer->PlayingID(), m_pSequencer->GameObjectPtr()->ID(), m_pSequencer->GetUserParams().CustomParam(), AkMonitorData::NotificationReason_Paused, Node()->ID(), AK_INVALID_UNIQUE_ID );
	}

	CAkMusicCtx::OnPaused();
}

void CAkMatrixAwareCtx::OnResumed()
{
	// NOTE: OnResumed() is called only when pause count reaches 0.
	if ( IsTopLevel() && IsPaused() )
	{
		MONITOR_MUSICOBJECTNOTIF( m_pSequencer->PlayingID(), m_pSequencer->GameObjectPtr()->ID(), m_pSequencer->GetUserParams().CustomParam(), AkMonitorData::NotificationReason_Resumed, Node()->ID(), AK_INVALID_UNIQUE_ID );
	}

	CAkMusicCtx::OnResumed();
}

AkUniqueID CAkMatrixAwareCtx::NodeID()
{
	CAkMusicNode * pNode = Node();
	return ( pNode ) ? pNode->ID() : AK_INVALID_UNIQUE_ID;
}
#endif

CAkMusicSegment * CAkMatrixAwareCtx::GetFirstSegmentNode(
	CAkMusicNode ** out_ppParentNode // Returned parent node. Pass NULL to ignore.
	)
{
	CAkScheduleWindow wndDest( this, true );
	if ( wndDest.IsValid() )
		return wndDest.GetNode( out_ppParentNode );
	if ( out_ppParentNode )
		*out_ppParentNode = NULL;
	return NULL;
}

// Shared Segment Sequencer.
void CAkMatrixAwareCtx::SetSequencer( 
    CAkMatrixSequencer * in_pSequencer
    ) 
{ 
    AKASSERT( !m_pSequencer ); 
    m_pSequencer = in_pSequencer; 
}

void CAkMatrixAwareCtx::SetRegisteredNotif( 
    AkUInt32 in_uCallbackFlags
    ) 
{ 
    m_uRegisteredNotif = in_uCallbackFlags;
}

// Handles global processing (stingers) if applicable, and stopping after last frame. 
void CAkMatrixAwareCtx::ProcessEpilogue(
	AkInt64			in_iCurrentTime,		// Current local time.
	AkUInt32		in_uNumSamples			// Number of samples to process.
	)
{
	if ( IsTopLevel() )
		Sequencer()->Process( in_iCurrentTime, in_uNumSamples );
	CAkMusicCtx::ProcessEpilogue();
}

