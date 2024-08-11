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
#include "AkSequencableSegmentCtx.h"
#include "AkMatrixSequencer.h"
#include "AkMusicSegment.h"
#include "AkScheduleWindow.h"

CAkSequencableSegmentCtx::CAkSequencableSegmentCtx(
    CAkMusicSegment *   in_pSegmentNode,
    CAkMusicCtx *       in_pParentCtx
    )
:CAkChainCtx( in_pParentCtx )
,m_pSegmentNode( in_pSegmentNode )
{
	if( m_pSegmentNode )
		m_pSegmentNode->AddRef();
}

CAkSequencableSegmentCtx::~CAkSequencableSegmentCtx()
{
    if( m_pSegmentNode )
		m_pSegmentNode->Release();
}

AKRESULT CAkSequencableSegmentCtx::Init( 
    CAkRegisteredObj *  in_GameObject,
    UserParams &        in_rUserparams
    )
{
    AKRESULT eResult = CAkChainCtx::Init( in_GameObject, in_rUserparams );
    if ( eResult != AK_Success )
		return eResult;

	// Default start position is at the beginning of the pre-entry.
	AkInt32 iStartPos = -m_pSegmentNode->PreEntryDuration();

#ifndef AK_OPTIMIZED
	// Override in_iStartPos with Wwise cursor position if specified (WWISE_RESERVED_BIT is set) 
	// AND this is a top-level context.
	if ( !m_pParentCtx
		&& in_rUserparams.CustomParam().ui32Reserved & AK_EVENTFROMWWISE_RESERVED_BIT )
		iStartPos = m_pSegmentNode->StartPos() - m_pSegmentNode->PreEntryDuration();
#endif

	// Enqueue segment buckets, prepare context to play at iStartPos.
	AkInt32 iLookAheadDuration;
	eResult = SetupSegmentChain( in_GameObject, in_rUserparams, iStartPos, iLookAheadDuration );

	// By default, schedule this context in iLookAheadDuration.
	// Parent may re-prepare and reschedule this context at will.
	Schedule( iLookAheadDuration );
	return eResult;
}

AKRESULT CAkSequencableSegmentCtx::SetupSegmentChain( 
    CAkRegisteredObj *  in_GameObject,
    UserParams &        in_rUserparams,
	AkInt32				in_iStartPos,
	AkInt32 &			out_iLookAheadDuration
    )
{
	out_iLookAheadDuration = 0;

	// Check start position against segment duration.
	if ( AK_EXPECT_FALSE( in_iStartPos >= (AkInt32)( m_pSegmentNode->ActiveDuration() + m_pSegmentNode->PostExitDuration() ) ) )
		return AK_Fail;

	// Create segment context.
    CAkSegmentCtx * pChildCtx = m_pSegmentNode->CreateLowLevelSegmentCtxAndAddRef( this, in_GameObject, in_rUserparams );
	if ( AK_EXPECT_FALSE( !pChildCtx ) )
		return AK_Fail;

	CAkScheduledItem * pItem = EnqueueItem( 0, pChildCtx );
		
	// Release child segment context (only) once it has been enqueued in the chain.
	pChildCtx->Release();

	if ( AK_EXPECT_FALSE( !pItem ) )
		return AK_Fail;

	// Reset local time: this helper is also used for seeking.
	Schedule( 0 );

	// Prepare, set start position.
	AkUniqueID uCueDest;	// ignored.
	out_iLookAheadDuration = Prepare( NULL, in_iStartPos, NULL, uCueDest );

	// Attach a stop action to this item. To occur at the end of post-exit.
	pItem->AttachStopCmd( 0, AkCurveInterpolation_Linear, // fading is ignored
		m_pSegmentNode->ActiveDuration() + m_pSegmentNode->PostExitDuration() );

	// Enqueue a trailing, empty item to the chain.
	pItem = EnqueueItem( m_pSegmentNode->ActiveDuration(), NULL );
	if ( AK_EXPECT_FALSE( !pItem ) )
		return AK_Fail;

	return AK_Success;
}

// Matrix Aware Context implementation.
// ----------------------------------------------------------

AKRESULT CAkSequencableSegmentCtx::SeekTimeAbsolute( 
	AkTimeMs & io_position,	// Seek position, in ms, relative to Entry Cue (if applicable). Returned as effective position.
	bool in_bSnapToCue		// True if final position needs to be on a cue (Exit cue excluded).
	)
{
	AkInt32 iPosition = AkTimeConv::MillisecondsToSamples( io_position );

	// Clamp position to beginning of pre-entry.
	if ( iPosition < -m_pSegmentNode->PreEntryDuration() )
		iPosition = -m_pSegmentNode->PreEntryDuration();

	if ( in_bSnapToCue ) 
	{
		iPosition = m_pSegmentNode->GetClosestCuePosition( iPosition );
		io_position = AkTimeConv::SamplesToMilliseconds( iPosition );
	}

	return Seek( iPosition );
}

AKRESULT CAkSequencableSegmentCtx::SeekPercent( 
	AkReal32 & io_fPercent,	// Seek position, in percentage, between Entry and Exit Cues ([0,1]). Returned as effective position.
	bool in_bSnapToCue		// True if final position needs to be on a cue (Exit cue excluded).
	)
{
	AKASSERT( io_fPercent >= 0 && io_fPercent <= 1.f );

	// Convert to position in samples.
	AkInt32 iPosition = (AkInt32)( io_fPercent * m_pSegmentNode->ActiveDuration() );
	
	if ( in_bSnapToCue ) 
	{
		iPosition = m_pSegmentNode->GetClosestCuePosition( iPosition );
		io_fPercent = (AkReal32)iPosition / m_pSegmentNode->ActiveDuration();
	}

	return Seek( iPosition );
}

// Change playback position.
// The chain is destroyed and re-prepared at the correct position.
AKRESULT CAkSequencableSegmentCtx::Seek( 
	AkInt32 in_iSeekPosition				// Seek position, relative to Entry Cue.
	)
{
	// Do not seek once this context is stopping.
	if ( IsStopping() )
		return AK_Fail;

	// Add ref while we clear our children and recreate the chain.
	AddRef();

	ChildrenCtxList::Iterator it = m_listChildren.Begin();
	while ( it != m_listChildren.End() )
	{
		CAkMusicCtx * pChild = static_cast<CAkMusicCtx*>(*it);
		++it;
		pChild->_Cancel();
	}

	Flush();

	// Re-setup chain and prepare context at desired location.
	AkInt32 iLookAheadDuration;
	AKRESULT eResult = SetupSegmentChain( 
		Sequencer()->GameObjectPtr(), 
		Sequencer()->GetUserParams(),
		in_iSeekPosition,
		iLookAheadDuration );

	// Re-schedule this context at Now + iLookAheadDuration
	if ( eResult == AK_Success )
		Schedule( GlobalToLocalTime( Sequencer()->Now() ) + iLookAheadDuration );
	else
	{
		TransParams transParams;
		transParams.eFadeCurve = AkCurveInterpolation_Linear;
		transParams.TransitionTime = 0;
		_Stop( transParams, 0 );
	}

	Release();

	return eResult;
}

// Overriden from CAkMatrixAwareCtx: Since there is only one segment in the segment chain of a 
// SequencableSegment, its info is returned regardless of the fact that it is active or not.
// Doing so, we get values during pre-entry and post-exit (negative values during the pre-entry).
AKRESULT CAkSequencableSegmentCtx::GetPlayingSegmentInfo(
	AkSegmentInfo &	out_segmentInfo		
	)
{
	CAkScheduleWindow window( this, true );	// Window should point at the first item (segment), even if current time corresponds to post-exit.
	if ( !window.IsValid() )
		return AK_Fail;
	CAkScheduledItem * pItem = window.GetScheduledItem();
	if ( pItem )
		return pItem->GetInfo( window.ToSegmentPosition( GlobalToLocalTime( Sequencer()->Now() ) ), out_segmentInfo );
	return AK_Fail;
}

// For Music Renderer's music contexts look-up: concrete contexts must return their own node.
CAkMusicNode * CAkSequencableSegmentCtx::Node()
{
    return m_pSegmentNode;
}
