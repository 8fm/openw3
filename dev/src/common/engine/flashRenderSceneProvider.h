/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#pragma once

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class CRenderFrame;
class CRenderFrameInfo;
class IRenderScene;

//////////////////////////////////////////////////////////////////////////
// IFlashRenderSceneProvider
//////////////////////////////////////////////////////////////////////////
class IFlashRenderSceneProvider
{
protected:
	virtual ~IFlashRenderSceneProvider() {}

public:
	virtual void								Tick( Float timeDelta )=0;
	virtual const SFlashRenderTargetCamera&		GetDefaultCamera() const=0;

	//!< Render world of the current scene onto provided render target
	virtual void								RenderWorld( IRenderGameplayRenderTarget* renderTarget ) = 0;
};