/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../core/handleMap.h"

#include "renderObject.h"
#include "scaleformConfig.h"

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
namespace Scaleform { namespace Render {
	class Image;
	class TextureManager;
	class ThreadCommand;
} }
#endif

class IRenderResource;
class IRenderVideo;
class IRenderGameplayRenderTarget;

#if WITH_SCALEFORM_VIDEO
class CVideoControllerScaleform;
#endif

//////////////////////////////////////////////////////////////////////////
// IRenderScaleform
//////////////////////////////////////////////////////////////////////////
class IRenderScaleform : public IRenderObject
{
public:
	enum ERenderState
	{ 
		RS_Error,
		RS_Uninitialized,
		RS_ShuttingDown,
		RS_DeviceLost,
		RS_Ready,
	};

public:
	IRenderScaleform() {}
	virtual ~IRenderScaleform() {}

public:
	virtual ERenderState GetRenderState() const=0;

public:
#ifdef USE_SCALEFORM
	virtual SF::Render::Image*				CreateImage( IRenderResource* renderResource, Uint32 imageWidth, Uint32 imageHeight, Uint32 imageUseFlags )=0;
	virtual SF::Render::Image*				CreateRenderTargetImage( IRenderGameplayRenderTarget* renderTarget, Uint32 imageWidth, Uint32 imageHeight )=0;
	virtual SF::Render::TextureManager*		GetTextureManager() const=0;
	virtual void							SendThreadCommand( SF::Render::ThreadCommand* command )=0;
#endif // USE_SCALEFORM
};
