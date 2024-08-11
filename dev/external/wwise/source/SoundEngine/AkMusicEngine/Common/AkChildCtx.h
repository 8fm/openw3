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
// AkChildCtx.h
//
// Base class for all child contexts (playback instances).
// Child contexts are contexts that can be enqueued in a high-level
// context. 
// Defines interface for child playback commands, commands that are
// propagated through the context hierarchy.
// NOTE Currently this is just an interface.
//
//////////////////////////////////////////////////////////////////////
#ifndef _CHILD_CTX_H_
#define _CHILD_CTX_H_

#include "PrivateStructures.h"

class CAkMusicCtx;

class CAkChildCtx
{
public:

	// Context commands
	//

    CAkChildCtx( CAkMusicCtx * in_pParentCtx );

	// Called at the end of each audio frame. Clean up is carried ob here, including the call to OnStopped() 
	// if this was this context's last frame.
	virtual void OnFrameEnd() = 0;

	// Notify context that this will be the last processing frame, propagated from a high-level context Stop().
	virtual void OnLastFrame( 
		AkUInt32 in_uNumSamples			// Number of samples left to process before stopping. 
        ) = 0;							// 0 if immediate, AK_NO_IN_BUFFER_STOP_REQUESTED if it should correspond to an audio frame, whatever its size (mininum transition time).
	
	// Pause context playback, propagated from a high-level context _Pause().
	virtual void OnPaused() = 0;

	// Resume context playback, propagated from a high-level context _Resume().
	virtual void OnResumed() = 0;

    // Fade management. Set fade level ratios.
    virtual void SetPBIFade( 
        void * in_pOwner,
        AkReal32 in_fFadeRatio
        ) = 0;

	// Special refcounting methods for parent contexts. After VirtualAddRef(), a child context must not be destroyed
	// until VirtualRelease() is called.
	virtual void VirtualAddRef() = 0;
	virtual void VirtualRelease() = 0;

#ifndef AK_OPTIMIZED
	virtual void OnEditDirty() = 0;
#endif

    CAkMusicCtx * Parent() { return m_pParentCtx; }

	CAkChildCtx *		pNextLightItem;	// Sibling. Must be public for ListBareLight.

protected:
    virtual ~CAkChildCtx();
    
    void Connect();

protected:
    CAkMusicCtx *       m_pParentCtx;
};

#endif // _CHILD_CTX_H_
