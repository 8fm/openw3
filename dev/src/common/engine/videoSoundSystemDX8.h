/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.

* Modified from Scaleform sources to reduce video microstuttering. Temporarily here
* to avoid having to rebuild the video library while tweaking.
*/

/**************************************************************************

Filename    :   Video_VideoSoundSystemDX8.h
Content     :   Video sound system implementation based on DirectSound
Created     :   March 2009
Authors     :   Maxim Didenko, Vladislav Merker

Copyright   :   Copyright 2011 Autodesk, Inc. All Rights reserved.

Use of this software is subject to the terms of the Autodesk license
agreement provided at the time of installation or download, or which
otherwise accompanies this software in either electronic or hard copy form.

**************************************************************************/

#ifndef INC_GFX_VIDEO_SYSTEMSOUND_DX8_H
#define INC_GFX_VIDEO_SYSTEMSOUND_DX8_H

#ifdef USE_SCALEFORM
#if defined( RED_PLATFORM_WINPC )

#include "GFxConfig.h"
#ifdef GFX_ENABLE_VIDEO

#include "Video/Video_Video.h"
#include "Kernel/SF_MemoryHeap.h"
#include <mmreg.h>
#include <mmsystem.h>
#include <dsound.h>

namespace Scaleform { namespace GFx { namespace Video {

//////////////////////////////////////////////////////////////////////////
//

// VideoSoundSystemDX8 is a sample video sound system implementation based on DirectSound
// used on the Windows platform. An instance of this class is usually created and passed
// to the Video::SetSoundSystem method as a part of GFx video initialization.

// Typically the DirectSound object would already be created by the game; when that is
// the case, proper IDirectSound8 pointers should to be passed in the constructor.
// Alternatively, developers can pass a null pointer to have the VideoSoundSystemDX8
// create a new DirectSound instance.

class VideoSoundSystemDX8Tmp : public VideoSoundSystem
{
public:
	// Creates a DirectSound video sound creator. If null is passed for pds8,
	// new DirectSound object will be created and used
	VideoSoundSystemDX8Tmp(IDirectSound8 *pds8, MemoryHeap* pheap = Memory::GetGlobalHeap());
	~VideoSoundSystemDX8Tmp();

	// VideoSoundSystem implementation
	virtual VideoSound* Create();

private:
	class VideoSoundSystemDX8ImplTmp* pImpl;
}; 

}}} // Scaleform::GFx::Video

#endif // GFX_ENABLE_VIDEO

#endif // defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )
#endif // USE_SCALEFORM


#endif // INC_GFX_VIDEO_SYSTEMSOUND_DX8_H
