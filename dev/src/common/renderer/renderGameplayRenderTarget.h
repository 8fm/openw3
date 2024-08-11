/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
	Kamil Nowakowski
*/
#pragma once

#include "../engine/renderGameplayRenderTargetInterface.h"

class CRenderGameplayRenderTarget : public IRenderGameplayRenderTarget
{
	
	const AnsiChar*			m_tag;

	GpuApi::TextureRef		m_renderTexture;

public:

	CRenderGameplayRenderTarget( const AnsiChar* tag );

	~CRenderGameplayRenderTarget();

	//!< Returns reference to the render targets texture
	virtual GpuApi::TextureRef GetGpuTexture() const override;

	//!< Rescales the texture to be able to contains given dimension
	virtual Bool RequestResizeRenderSurfaces( Uint32 width, Uint32 height ) override;

	virtual void CopyFromRenderTarget( ERenderTargetName sourceRenderTarget, Rect sourceRect ) override;

};
