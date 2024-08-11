/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
	Kamil Nowakowski
*/
#pragma once

#include "../renderer/renderHelpers.h"

class IRenderGameplayRenderTarget : public IRenderObject
{
public:
	enum class CopyMode
	{
		Normal, TextureMask, BackgroundColor
	};

protected:

	CopyMode				m_copyMode;

	Uint32					m_imageWidth;			//!<	Size of the active area (render size)
	Uint32					m_imageHeight;			//!<	Size of the active area (render size)

	Uint32					m_textureWidth;			//!<	Size of the texture (physical area)
	Uint32					m_textureHeight;		//!<	Size of the texture (physical area)

	IRenderResource*		m_textureMask;
	Vector					m_backgroundColor;

public:

	IRenderGameplayRenderTarget();

	virtual ~IRenderGameplayRenderTarget();

public:

	//!< Returns reference to the render targets texture
	virtual GpuApi::TextureRef GetGpuTexture() const = 0;

	//!< Rescales the texture to be able to contains given dimension
	virtual Bool RequestResizeRenderSurfaces( Uint32 width, Uint32 height ) = 0;

	// Copy from another render target
	virtual void CopyFromRenderTarget( ERenderTargetName renderTarget , Rect sourceRect ) = 0;

public:

	//!< Gets copy mode
	RED_INLINE CopyMode GetCopyMode() const { return m_copyMode; }

	//!< Gets image width size (active area)
	RED_INLINE Uint32	GetImageWidth() const { return m_imageWidth; }

	//!< Gets image height size (active area)
	RED_INLINE Uint32	GetImageHeight() const { return m_imageHeight; }

	//!< Gets texture width size (physical area)
	RED_INLINE Uint32	GetTextureWidth() const { return m_textureWidth; }

	//!< Gets texture height size (physical area)
	RED_INLINE Uint32	GetTextureHeight() const { return m_textureHeight; }

	//!< Get aspect ratio of view
	RED_INLINE Float	GetAspectRatio() const { return m_imageWidth/Float(m_imageHeight); }

	//!< Gets custom texture mask (alpha and render background)
	RED_INLINE IRenderResource* GetTextureMask() const { return m_textureMask; }

	//!< Gets custom background color
	RED_INLINE const Vector& GetBackgroundColor() const { return m_backgroundColor; }

	//!< Sets normal (copy mode)
	void	SetNormal();

	//!< Sets custom texture mask (alpha and render background)
	void	SetTextureMask( IRenderResource* textureMask );

	//!< Sets custom background color
	void	SetBackgroundColor( const Vector& backgroundColor );
};

