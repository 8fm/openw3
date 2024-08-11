/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "flashRenderTarget.h"

class CFlashMovieScaleform;

// TBD: Register multiple movies, and share textureimages between them if same scaling
// TBD: Removin scene before flash, and update to null texture?
class CFlashRenderTargetScaleform : public CFlashRenderTarget
{
private:
	CFlashMovieScaleform*					m_flashMovie;
	String									m_targetName;

private:
	Uint32									m_imageWidth;
	Uint32									m_imageHeight;
	IRenderGameplayRenderTarget*			m_renderTarget;

public:
	CFlashRenderTargetScaleform( CFlashMovieScaleform* flashMovie, const String& targetName, Uint32 width, Uint32 height );
	virtual ~CFlashRenderTargetScaleform();
	
public:
	virtual Bool IsValid() const override { return m_renderTarget != nullptr; }
	virtual void RenderScene( IFlashRenderSceneProvider* renderSceneProvider ) override;
	virtual const String& GetTargetName() const override { return m_targetName; }

	virtual IRenderGameplayRenderTarget* GetRenderTarget() const { return m_renderTarget; }

private:
	Bool TryReplaceImage();
};