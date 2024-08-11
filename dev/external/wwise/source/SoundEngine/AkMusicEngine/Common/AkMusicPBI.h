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
// AkMusicPBI.h
//
// Enhances the PBI by keeping a pointer to a parent (music) context. 
// Removes itself from parent in destructor.
// Also, handles transitions of a multi-level context hierarchy:
// checks if it is registered to a transition of one of its ascendant
// before creating one (a extra step before PBI::_Play/_Stop).
//
//////////////////////////////////////////////////////////////////////
#ifndef _MUSIC_PBI_H_
#define _MUSIC_PBI_H_

#include "AkPBI.h"
#include "AkMusicCtx.h"

struct AkTrackSrc;

// IMPORTANT: CAkPBI must be the first base class specified, because the Upper Renderer AkDestroys
// them through a pointer to a CAkPBI.
class CAkMusicPBI : public CAkPBI,
                    public CAkChildCtx
{

    friend class CAkMusicRenderer;

public:

    virtual ~CAkMusicPBI();

	virtual AKRESULT Init( AkPathInfo* in_pPathInfo );

	virtual void Term( bool in_bFailedToInit );

	//Seeking
	// Disabled on Music PBI. Seeking is always performed by Segment Ctx.
	virtual void SeekTimeAbsolute( AkTimeMs in_iPosition, bool in_bSnapToMarker );
	virtual void SeekPercent( AkReal32 in_fPercent, bool in_bSnapToMarker );

	// Stop the PBI (the PBI is then destroyed)
	void _Stop( 
        AkUInt32 in_uStopOffset
        );

	// Apply an automation value of a given type.
	void SetAutomationValue( AkClipAutomationType in_eAutomationType, AkReal32 in_fValue );

	void FixStartTimeForFadeIn();
	inline void SetFadeOut() { m_bNeedsFadeOut = true; }

protected:

    // Child context implementation.
    // ------------------------------------------

	// Called at the end of each audio frame. Clean up is carried ob here, including the call to OnStopped() 
	// if this was this context's last frame.
	virtual void OnFrameEnd() {}

	// Notify context that this will be the last processing frame, propagated from a high-level context Stop().
	// Stop offset is set on the PBI. Lower engine will stop it in_uNumSamples after the beginning of the audio frame.
	virtual void OnLastFrame( 
		AkUInt32 in_uNumSamples			// Number of samples left to process before stopping. 
        );								// 0 if immediate, AK_NO_IN_BUFFER_STOP_REQUESTED if it should correspond to an audio frame, whatever its size (mininum transition time).
	
	// Pause context playback, propagated from a high-level context _Pause().
	virtual void OnPaused();

	// Resume context playback, propagated from a high-level context _Resume().
	virtual void OnResumed();

    // Fade management. Set fade level ratios.
    virtual void SetPBIFade( 
        void * in_pOwner,
        AkReal32 in_fFadeRatio
        );

	// Special refcounting methods for parent contexts. After VirtualAddRef(), a child context must not be destroyed
	// until VirtualRelease() is called.
	virtual void VirtualAddRef();
	virtual void VirtualRelease();

#ifndef AK_OPTIMIZED
	virtual void OnEditDirty();
#endif

    // Overriden methods from PBIs.
    // ---------------------------------------------

	// Override PBI _stop: skip command when posted because sound has not already started playing.
	// Interactive music sounds need to play even if they did not start playing (WG-19938).
	virtual void _Stop( AkPBIStopMode in_eStopMode = AkPBIStopMode_Normal, bool in_bIsFromTransition = false, bool in_bHasNotStarted = false );

    //Must be overriden to force interactive music to ignore pitch from busses
	virtual void RefreshParameters();

public:
	const AkTrackSrc * GetSrcInfo() const { return m_pSrcInfo; }

#ifndef AK_OPTIMIZED
	inline void MonitorPaused() { Monitor(AkMonitorData::NotificationReason_Paused); }
#endif

    // Only the Music Renderer can create/_Play music PBIs.
private:

	// Overridden from base PBI.
	virtual AkUInt32	GetStopOffset() const;	// Returns stop offset value.
	virtual AkUInt32	GetAndClearStopOffset();// Returns stop offset value effectively used for audio frame truncation.
	virtual AkCtxVirtualHandlingResult NotifyVirtualOff( AkVirtualQueueBehavior in_eBehavior );

	void SetStopOffset( AkUInt32 in_ulStopOffset ){ m_ulStopOffset = in_ulStopOffset; }
    
    CAkMusicPBI(
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
        );

protected:
	AkUInt32			m_ulStopOffset;
	const AkTrackSrc *	m_pSrcInfo;		// Pointer to track's source playlist item.
#ifdef _DEBUG
	AkUInt32			m_uRefCount;	// Debug variable, to ensure that this is not destroyed while VirtualAddRef() was called.
#endif
	AkUInt32			m_bWasStoppedByUEngine :1;
	AkUInt32			m_bNeedsFadeOut :1;	// True if intra-frame stopping should be bypassed with fade out instead.
};

#endif
