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
// AkMusicPBI.cpp
//
// Enhances the PBI by keeping a pointer to a parent (music) context. 
// Removes itself from parent in destructor.
// Also, handles transitions of a multi-level context hierarchy:
// checks if it is registered to a transition of one of its ascendant
// before creating one (a extra step before PBI::_Play/_Stop).
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkMusicPBI.h"
#include "AkTransitionManager.h"
#include "AkSegmentCtx.h"
#include "Ak3DParams.h"

CAkMusicPBI::CAkMusicPBI(
    CAkMusicCtx *		in_parent,
    CAkSoundBase*		in_pSound,			// Pointer to the sound.
	CAkSource*			in_pSource,			// Pointer to the source.
	CAkRegisteredObj *	in_pGameObj,		// Game object and channel association.
	UserParams&			in_UserParams,		// User Parameters.
	const AkTrackSrc *	in_pSrcInfo,		// Pointer to track's source playlist item.
	PlayHistory&		in_rPlayHistory,	// History stuff.
	AkUniqueID			in_SeqID,			// Sample accurate seq id.
	PriorityInfoCurrent	in_Priority,
	AkUInt32			in_uSourceOffset,
	CAkLimiter*			in_pAMLimiter,
	CAkLimiter*			in_pBusLimiter
    )
:CAkPBI( in_pSound,
		 in_pSource,
         in_pGameObj,
         in_UserParams,
         in_rPlayHistory,
         in_SeqID,
         in_Priority,
#ifdef AK_MOTION
		 false /*Currently no support for feedback in interactive music*/,
#endif // AK_MOTION
		 in_uSourceOffset,
		 in_pAMLimiter,
		 in_pBusLimiter
		 )
,CAkChildCtx( in_parent )
,m_ulStopOffset( AK_NO_IN_BUFFER_STOP_REQUESTED )
,m_pSrcInfo( in_pSrcInfo )
#ifdef _DEBUG
,m_uRefCount( 0 )
#endif
,m_bWasStoppedByUEngine ( false )
,m_bNeedsFadeOut( false )
{
	// Override pre-buffering flag set by base class.
	m_bRequiresPreBuffering = false;
}

CAkMusicPBI::~CAkMusicPBI()
{
	AKASSERT( m_uRefCount == 0 );	// Debug variable.
}

void CAkMusicPBI::Term( bool in_bFailedToInit )
{
    AKASSERT( m_pParentCtx );
    if ( !m_bWasStoppedByUEngine )
    {
        // Destruction occurred without the higher-level hierarchy knowing it. 
        // Notify Segment (we know our parent is a Segment).
        static_cast<CAkSegmentCtx*>(m_pParentCtx)->RemoveAllReferences( this );
    }
    m_pParentCtx->RemoveChild( this );

	CAkPBI::Term( in_bFailedToInit );
}

AKRESULT CAkMusicPBI::Init( AkPathInfo* in_pPathInfo )
{
    AKASSERT( m_pParentCtx || !"A Music PBI HAS to have a parent" );
    
	Connect();

    return CAkPBI::Init( in_pPathInfo );
}

//Seeking
// Disabled on Music PBI. Seeking is always performed by Segment Ctx.
void CAkMusicPBI::SeekTimeAbsolute( AkTimeMs, bool )
{
	AKASSERT( !"Cannot seek Music PBI directly" );
}
void CAkMusicPBI::SeekPercent( AkReal32, bool )
{
	AKASSERT( !"Cannot seek Music PBI directly" );
}

void CAkMusicPBI::_Stop( AkPBIStopMode in_eStopMode, bool in_bIsFromTransition, bool in_bHasNotStarted )
{
	// Skip _Stop if a stop offset was set on a sound that started playing in the same frame.
	if ( !in_bHasNotStarted || m_ulStopOffset == AK_NO_IN_BUFFER_STOP_REQUESTED )
		CAkPBI::_Stop( in_eStopMode, in_bIsFromTransition );
}

void CAkMusicPBI::SetAutomationValue( AkClipAutomationType in_eAutomationType, AkReal32 in_fValue )
{
	switch ( in_eAutomationType )
	{
	case AutomationType_Volume:
		// NOTE: Volume automation uses a slot in the "mute map" in order to save memory. 
		// It is handled as a dB scale in the UI, and therefore transformed using AkMath::Prescaling_ToLin_dB().
		// Here we need to use it directly as a fade ratio, so instead of relying on the automatic curve
		// conversion, m_tableAutomation.Set() uses "AkCurveScaling_None" and we simplify the math:
		// scaled_val = 20*log(prescaled_val+1)
		// fade_val = 10^(scaled_val/20) = 10^log(prescaled_val+1) = prescaled_val+1
		// IMPORTANT: Keep in sync with AkMath::Prescaling_ToLin_dB().
		in_fValue += 1.f;

	case AutomationType_FadeIn:
	case AutomationType_FadeOut:
		SetPBIFade( (AkUInt8*)this + (AkUIntPtr)in_eAutomationType, in_fValue );
		break;
	case AutomationType_LPF:
		m_LPFAutomationOffset = in_fValue;
		CalculateEffectiveLPF();
		break;
	default:
		AKASSERT( !"Invalid automation type" );
	}
}

// Child context implementation.
// ------------------------------------------

// Notify context that this will be the last processing frame, propagated from a high-level context Stop().
// Stop offset is set on the PBI. Lower engine will stop it in_uNumSamples after the beginning of the audio frame.
void CAkMusicPBI::OnLastFrame( 
	AkUInt32 in_uNumSamples			// Number of samples left to process before stopping. 
    )
{
    // Set stop offset only if smaller than previous value.
	if ( in_uNumSamples < m_ulStopOffset && in_uNumSamples != AK_NO_IN_BUFFER_STOP_REQUESTED )
		SetStopOffset( in_uNumSamples );

	// Stop abruptly (no transition). Do not use minimum transition time if m_ulStopOffset is not 
	// AK_NO_IN_BUFFER_STOP_REQUESTED; the lower engine will stop this PBI using the stop offset.
	TransParams transParams;
	transParams.TransitionTime = 0;
	CAkPBI::_Stop( transParams, m_ulStopOffset == AK_NO_IN_BUFFER_STOP_REQUESTED || m_bNeedsFadeOut );
}

// Pause context playback, propagated from a high-level context _Pause().
void CAkMusicPBI::OnPaused()
{
    // Enqueue pause command to lower engine, through upper renderer.
    _Pause( true );
}

// Resume context playback, propagated from a high-level context _Resume().
void CAkMusicPBI::OnResumed()
{
    // Enqueue pause command to lower engine, through upper renderer.
    _Resume();

    // Note. PBIs mute their m_cPauseResumeFade in _Pause(), but do not unmute it in _Resume().
    // Force it.
    m_fPauseResumeFadeRatio = AK_UNMUTED_RATIO;
    CalculateMutedEffectiveVolume();
}

// Special refcounting methods for parent contexts. After VirtualAddRef(), a child context must not be destroyed
// until VirtualRelease() is called.
void CAkMusicPBI::VirtualAddRef()
{
	// Does nothing: We take for granted that the URenderer does not destroy us if this was called.
	// TODO: Implement true refcounting if this changes.
#ifdef _DEBUG
	// Debug variable
	++m_uRefCount;
#endif
}
void CAkMusicPBI::VirtualRelease()
{
	// Does nothing.
#ifdef _DEBUG
	// Debug variable
	AKASSERT( m_uRefCount > 0 );
	--m_uRefCount;
#endif
}

#ifndef AK_OPTIMIZED
void CAkMusicPBI::OnEditDirty()
{
	// This notification is never propagated at this level.
	AKASSERT( !"Unhandled notification" );
}
#endif


// OVERRIDEN METHODS FROM PBIs.
// ---------------------------------------------

// Stop the PBI.
void CAkMusicPBI::_Stop( 
    AkUInt32 in_uStopOffset )
{
	// This function is the entry point of segment context's scheduled stops. 
	// Set "stopped by upper engine" flag. When it is not set, it forces us to notify the segment context to
	// search and remove all references to this PBI (when it was stopped because of an error, for example).
	m_bWasStoppedByUEngine = true;

	OnLastFrame( in_uStopOffset );
}


// Fade management. Propagate fades down to PBIs muted map
void CAkMusicPBI::SetPBIFade( 
    void * in_pOwner,
    AkReal32 in_fFadeRatio
    )
{
    AkMutedMapItem mutedMapItem;
    mutedMapItem.m_bIsPersistent = true;
    mutedMapItem.m_bIsGlobal = false;
    mutedMapItem.m_Identifier = in_pOwner;
    
    SetMuteMapEntry( mutedMapItem, in_fFadeRatio );
}

void CAkMusicPBI::FixStartTimeForFadeIn() 
{ 
	AkInt32 iOldFrameOffset = GetFrameOffset();
	AKASSERT( iOldFrameOffset >= 0 );
	AkInt32 iSubFrameOffset = iOldFrameOffset % AK_NUM_VOICE_REFILL_FRAMES;

	if ( iSubFrameOffset > AK_NUM_VOICE_REFILL_FRAMES / 2 
		|| iSubFrameOffset > (AkInt32)m_uSeekPosition )
	{
		AkInt32 iFrameOffsetIncrement = AK_NUM_VOICE_REFILL_FRAMES - iSubFrameOffset;			
		SetFrameOffset( GetFrameOffset() + iFrameOffsetIncrement );
		SetNewSeekPosition( m_uSeekPosition + iFrameOffsetIncrement, false );
	}
	else if ( iSubFrameOffset > 0 )
	{
		SetFrameOffset( GetFrameOffset() - iSubFrameOffset );
		AKASSERT( (AkInt32)m_uSeekPosition >= iSubFrameOffset );
		SetNewSeekPosition( m_uSeekPosition - iSubFrameOffset, false );
	}
	m_bNeedsFadeIn = true;
}

AkUInt32 CAkMusicPBI::GetStopOffset() const
{
	return m_ulStopOffset;
}

AkUInt32 CAkMusicPBI::GetAndClearStopOffset()
{
	AkUInt32 ulStopOffset = m_ulStopOffset;
	m_ulStopOffset = AK_NO_IN_BUFFER_STOP_REQUESTED;
	return ( !m_bNeedsFadeOut ) ? ulStopOffset : AK_NO_IN_BUFFER_STOP_REQUESTED;
}

AkCtxVirtualHandlingResult CAkMusicPBI::NotifyVirtualOff( AkVirtualQueueBehavior in_eBehavior )
{
	// Interactive music only has "From Elapsed Time option". Replace this assert with an if() if it changes.
	AKASSERT( in_eBehavior == AkVirtualQueueBehavior_FromElapsedTime );

	AkInt32 iLookAheadTime;
	AkInt32 iSourceOffset;
	if ( static_cast<CAkSegmentCtx*>(m_pParentCtx)->GetSourceInfoForPlaybackRestart(
		this,				// Context which became physical.
		iLookAheadTime,		// Returned required frame offset (sample-accurate look-ahead time).
		iSourceOffset ) )	// Returned required source offset ("core" sample-accurate offset in source).
	{
		// Look-ahead time should be a multiple of the frame size so that volume ramps sound smooth.
		SetFrameOffset( iLookAheadTime );
		SetNewSeekPosition( iSourceOffset, false );
		return VirtualHandling_RequiresSeeking;
	}
	else
	{
		// Source cannot restart before scheduled stop time.
		return VirtualHandling_ShouldStop;
	}
}

void CAkMusicPBI::RefreshParameters()
{
	CAkPBI::RefreshParameters();
	m_EffectiveParams.Pitch = 0;
}
