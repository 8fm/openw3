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
#ifndef _MUSIC_CTX_H_
#define _MUSIC_CTX_H_

#include "AkChildCtx.h"
#include "AkPoolSizes.h"
#include <AK/Tools/Common/AkListBareLight.h>
#include "AkMusicStructs.h"
#include "AkRegisteredObj.h"
#include "AkTransportAware.h"
#include "ITransitionable.h"

class CAkMusicNode;
class CAkMusicSegment;
class AkMusicTransSrcRule;

class CAkMusicCtx : public CAkChildCtx,
					public CAkTransportAware,
                    public ITransitionable
{
public:

	enum CtxStatus
	{
		CtxStatusIdle		= 0,
		CtxStatusPlaying	= 1,
		CtxStatusLastFrame	= 1<<1,
		CtxStatusStopped	= 1<<2
	};

    CAkMusicCtx( 
        CAkMusicCtx *   in_parent = NULL        // Parent context. NULL if this is a top-level context.
        );

    virtual ~CAkMusicCtx();

    // Init: Connects to parent.
    void Init(
        CAkRegisteredObj *  in_pGameObj,
        UserParams &    in_rUserparams        
        );

    // Parent-child management.
    void AddChild(
        CAkChildCtx * in_pChildCtx
        );
    void RemoveChild( 
        CAkChildCtx * in_pChildCtx
        );

    // Ref counting interface.
    void AddRef() { ++m_uRefCount; }
    void Release();

    // ITransitionable implementation:
    // ----------------------------------------
    // Fades: Music context keep their own fade level, and register it to the leaf (PBI)'s mute map,
    // by propagating it through the CAkChildCtx::SetPBIFade() interface.
    virtual void TransUpdateValue(
        AkIntPtr in_eTarget, 
        AkReal32 in_fValue, 
        bool in_bIsTerminated 
        );

protected:

	// Overridable methods.
	// ----------------------------------------

	// Notify context that it should be start playing.
	virtual void OnPlayed();

    // Child context handling implementation.
    // ----------------------------------------

	// Called at the end of each audio frame. Clean up is carried ob here, including the call to OnStopped() 
	// if this was this context's last frame.
	virtual void OnFrameEnd();

	// Notify context that this will be the last processing frame, propagated from a high-level context Stop().
	// Stop() calls OnLastFrame() when fade out is finished or context should stop within the audio frame.
	// Calls OnStopped() if in_uNumSamples is 0 or in any case when stopping should occur immediately. 
	// Otherwise, contexts should call OnStopped() at the end of their processing routine if IsLastFrame() is true.
	virtual void OnLastFrame( 
		AkUInt32 in_uNumSamples // Number of samples left to process before stopping. 
        );						// 0 if immediate, AK_NO_IN_BUFFER_STOP_REQUESTED if it should correspond to an audio frame, whatever its size (mininum transition time).
	
	// Notify context that it should be completely stopped.
	virtual void OnStopped();

	// Pause context playback, propagated from a high-level context _Pause().
	virtual void OnPaused();

	// Resume context playback, propagated from a high-level context _Resume().
	virtual void OnResumed();

    // Fade management. Propagate fades down to PBIs muted map
    virtual void SetPBIFade( 
        void * in_pOwner,
        AkReal32 in_fFadeRatio
        );
    
	// Special refcounting methods for parent contexts. After VirtualAddRef(), a child context must not be destroyed
	// until VirtualRelease() is called.
	virtual void VirtualAddRef();
	virtual void VirtualRelease();

public:
#ifndef AK_OPTIMIZED
	virtual void OnEditDirty();
#endif

public:
	// Context commands:
	// Propagate _Cmd() messages to children.
    // ----------------------------------------

	// Called at the end of each audio frame. Clean up is carried ob here, including the call to OnStopped() 
	// if this was this context's last frame.
	void _EndFrame();
    
	// Start context playback.
	void _Play( 
        AkMusicFade & in_fadeParams
        );

    // Start context playback. No fade offset overload.
	void _Play( 
        TransParams & in_transParams
        );

	// Stop context playback.
	void _Stop( 
        TransParams & in_transParams,
		AkUInt32 in_uNumLastSamples	= AK_NO_IN_BUFFER_STOP_REQUESTED// Number of samples left to process before stopping. 
		);															// 0 is "immediate", AK_NO_IN_BUFFER_STOP_REQUESTED is "one audio frame with minimal transition time". 

	// Cancel context that has been created but not played yet.
	void _Cancel();

	// Pause context playback.
	void _Pause( 
        TransParams & in_transParams
        );

	// Resume context playback.
	void _Resume( 
        TransParams & in_transParams,
        bool in_bIsMasterResume // REVIEW
        );


    // IAkTransportAware interface implementation.
    // Needed for Playing Mgr. NOT IMPLEMENTED.

    // Stop the PBI (the PBI is then destroyed)
	//
	//Return - AKRESULT - AK_Success if succeeded
	virtual void _Stop( AkPBIStopMode in_eStopMode = AkPBIStopMode_Normal, bool in_bIsFromTransition = false, bool in_bHasNotStarted = false );

#ifndef AK_OPTIMIZED
	virtual void _StopAndContinue(
		AkUniqueID			in_ItemToPlay,
		AkUInt16				in_PlaylistPosition,
		CAkContinueListItem* in_pContinueListItem
		);
#endif

	// Status management.

	// Returns True if Process() should be called.
	inline bool RequiresProcessing() { return ( m_eStatus & (CtxStatusPlaying | CtxStatusLastFrame) ) && !IsPaused(); }

	// Returns true if context is playing and is not within its last processing frame.
    inline bool IsPlaying() { return ( m_eStatus & CtxStatusPlaying ) > 0; }

	// Returns true if context has not started playing.
	inline bool IsIdle() { return m_eStatus == CtxStatusIdle; }	

	// Returns false if context is playing or idle, true if in its last frame or already stopped.
    inline bool IsStopping() { return ( m_eStatus > CtxStatusPlaying ); }

	// Returns true if context is (completely) paused.
	inline bool	IsPaused() { return m_bIsPaused; }

protected:

	//
	// Services: Processing prologue/epilogue.
	//

	// Truncates the number of frames if context is within its last processing frame.
	inline void ProcessPrologue(
		AkUInt32 &	io_uNumSamples	// Number of samples to process.
		) 
	{
		AddRef();

		AKASSERT( RequiresProcessing() );
		if ( IsLastFrame() )
		{
			if ( m_uNumLastSamples != AK_NO_IN_BUFFER_STOP_REQUESTED )
				io_uNumSamples = m_uNumLastSamples;;
		}
		AKASSERT( io_uNumSamples > 0 );
	}

	inline void ProcessEpilogue() 
	{ 
		Release();
	}
	

	// Returns true if context is within its last processing frame.
	inline bool IsLastFrame() { return ( m_eStatus & CtxStatusLastFrame ) > 0; }	
	

	// Returns number of samples in last frame, AK_NO_IN_BUFFER_STOP_REQUESTED if not specified.
	// Meant for controlling MusicPBIs.
	inline AkUInt32 GetNumSamplesInLastFrame() { AKASSERT( IsLastFrame() ); return m_uNumLastSamples; }

    // Parent/child management.
    typedef AkListBareLight<CAkChildCtx> ChildrenCtxList;
    ChildrenCtxList     m_listChildren;

#ifdef _DEBUG
	AkUInt32 NumChildren();
	AkUInt32 GetRefCount() { return m_uRefCount; }
#endif

private:

    // Common members.
	PlaybackTransition	m_PBTrans;      // Context's current playback transition.

    // Ref counting.
    AkUInt32            m_uRefCount;

	// Fade levels.
    // Note. Even though fade levels are sent directly to leaves through CAkChildCtx::SetPBIFade(), their values are
    // kept instead of just a flag (bAudible) for 2 reasons:
    // 1) we use uStopFade and uPauseFade addresses as the mute map key (distinguish pause and stop);
    // 2) possible optimization for contexts: avoid performing actions (like creating PBIs) if they will not be
    // audible anyway.
    AkReal32			m_fPlayStopFadeRatio;
    AkReal32			m_fPauseResumeFadeRatio;

	// Number of samples left to process after IsLastFrame() has been called. If it is 
	// AK_NO_IN_BUFFER_STOP_REQUESTED, the number of samples to process should be equal to the number of 
	// samples in the audio frame, whatever it is (typically for a smooth fade out). Otherwise it 
	// represents a sharp stop m_uNumLastSamples after the beginning of the audio frame.
	// When 0, the context is stopped.
	AkUInt32			m_uNumLastSamples;
    
    // Pause count.
    AkUInt16            m_uPauseCount;

private:
	AkUInt8	/*CtxStatus*/	m_eStatus	:4;	// Context playback status.
	AkUInt8             m_bIsPaused     :1; // True when this context is completely paused.
};

#endif
