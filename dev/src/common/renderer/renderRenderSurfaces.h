/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderHelpers.h"

/// A render target 
class CRenderTarget
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );
	friend class CRenderSurfaces;

protected:
	GpuApi::TextureRef			m_texture;				//!< Texture
	Uint32 						m_width;				//!< Width of the rendering texture
	Uint32 						m_height;				//!< Height of the rendering surface
	ERenderTargetFormat			m_format;				//!< Internal format
	Uint32						m_msaaLevel;			//!> Multisampling level
	Bool						m_samplable;			//!> Can we sample this rendertarget?
	Uint32						m_numMipLevels;			//!> Number of mip levels
	Uint32						m_extraUsageFlags;		//!> Extra usage flags (will be set for the rendertarget along with implicit ones)

	Int32 						m_esramOffset;			//!< XboxOne only, -1 means that the rendertarget is not in the ESRAM
	Uint32 						m_esramSize;			//!< XboxOne only
public:

#if MICROSOFT_ATG_DYNAMIC_SCALING	
	//! Get the render target width
	RED_INLINE Uint32 GetWidth() const 
	{ 
		return DynamicScalingGetTargetWidth(m_texture); 
	}

	//! Get the render target width
	RED_INLINE Uint32 GetHeight() const 
	{ 
		return DynamicScalingGetTargetHeight(m_texture); 
	}
#else
	//! Get the render target width
	RED_INLINE Uint32 GetWidth() const { return m_width; }

	//! Get the render target height
	RED_INLINE Uint32 GetHeight() const { return m_height; }
#endif

	//! Get format of the render target
	RED_INLINE ERenderTargetFormat GetFormat() const { return m_format;  }

	//! Get number of mip levels
	RED_INLINE Uint32 GetNumMipLevels() const { return m_numMipLevels; }

	//! Get msaa level
	RED_INLINE Uint32 GetMSAALevel() const { return m_msaaLevel;  }

	//! Is multisampled
	RED_INLINE Bool IsMultisampled() const { return GetMSAALevel() > 1;  }

	//! Get texture
	RED_INLINE const GpuApi::TextureRef& GetTexture() const { return m_texture; }

public:
	CRenderTarget( Uint32 width, Uint32 height, ERenderTargetFormat format, Bool samplable, Uint32 msaaLevel, Int32 esramOffset = -1, Uint32 esramSize = 0, Uint32 numMipLevels = 1, Uint32 extraUsageFlags = 0 );
	virtual ~CRenderTarget();

	void CreateResources( Bool dynamicScaling = false, Uint32 alignment = 4 );

	void ReleaseResources();

protected:
	virtual void OnDeviceLost();
	virtual void OnDeviceReset();
};

/// Group of rendering surfaces
class CRenderSurfaces : public IDynamicRenderResource
{
public:
	enum ePersistentSurface
	{
		PS_Luminance,
		PS_RLRHistory,
		PS_TemporalAntialias,
	};

public:
	static Vector GetGBufferDefaultClearColor( Uint32 gbufferIndex );
	
protected:
	GpuApi::TextureRef			m_depthBuffer;
	GpuApi::TextureRef			m_depthBufferMSAA;

#ifndef NO_EDITOR
	// This texture is used only for overlays and debug surfaces (editor only)
#endif

	CRenderTarget*				m_renderTargets[ RTN_Max ];

	Uint32						m_width;
	Uint32						m_height;
	Uint32						m_msaaLevel;
	Uint32						m_msaaLevelX;
	Uint32						m_msaaLevelY;
	Bool						m_temporalAASupported;
	Bool						m_highPrecisionEnabled;
	Uint32						m_dirtyPersistentSurfacesFlags;

public:

#if MICROSOFT_ATG_DYNAMIC_SCALING	
	//! Get the width of the rendering surfaces main buffer
	RED_INLINE Uint32 GetWidth( Bool ignoreScaling = false ) const 
	{ 
		if(!ignoreScaling)
		{
			if(m_renderTargets[RTN_Color])
				return m_renderTargets[RTN_Color]->GetWidth();
		
			if(!m_depthBuffer.isNull())
				return DynamicScalingGetTargetWidth(m_depthBuffer);
		}
		return m_width; 
	}

	//! Get the height of the rendering surfaces main buffer
	RED_INLINE Uint32 GetHeight( Bool ignoreScaling = false ) const 
	{ 
		if(!ignoreScaling)
		{
			if(m_renderTargets[RTN_Color])
				return m_renderTargets[RTN_Color]->GetHeight();

			if(!m_depthBuffer.isNull())
				return DynamicScalingGetTargetHeight(m_depthBuffer);
		}
		return m_height; 
	}
#else
	//! Get the width of the rendering surfaces main buffer
	RED_INLINE Uint32 GetWidth(bool ignoreScaling = false) const { return m_width; }

	//! Get the height of the rendering surfaces main buffer
	RED_INLINE Uint32 GetHeight(bool ignoreScaling = false) const { return m_height; }
#endif

	//! Get the surfaces msaa level
	RED_INLINE Uint32 GetMSAALevel()  const { return m_msaaLevel; }
	RED_INLINE Uint32 GetMSAALevelX() const { return m_msaaLevelX; }
	RED_INLINE Uint32 GetMSAALevelY() const { return m_msaaLevelY; }

	//! Get whether msaa is supported
	RED_INLINE Bool IsMSAASupported() const { return GetMSAALevel() > 1; }

	//! Get whether temporal AA is supported
	Bool IsTemporalAASupported() const { return m_temporalAASupported; }

	//! Get the depth buffer
	GpuApi::TextureRef GetDepthBufferTex() const;
	GpuApi::TextureRef GetDepthBufferTexMSAA() const;

	//! Get local reflections fullres mask texture
	GpuApi::TextureRef GetLocalReflectionsMaskTex() const;
	
public:
	CRenderSurfaces( Uint32 width, Uint32 height, Uint32 msaaLevelX, Uint32 msaaLevelY );
	~CRenderSurfaces();
	
	//! Get the render target
	CRenderTarget* GetRenderTarget( ERenderTargetName renderTargetName ) const;

	//! Get rendertarget texture
	GpuApi::TextureRef GetRenderTargetTex( ERenderTargetName renderTargetName ) const;

	//! Describe resource
	virtual CName GetCategory() const;

	// Get the used video memory
	virtual Uint32 GetUsedVideoMemory() const;

	//! Is hi precision enabled
	Bool IsHighPrecisionEnabled() const;

	//! Set high precision
	Bool SetHighPrecision( Bool enableHighPrecision );

	// HACK
	Bool IsRenderTargetsSwappable( ERenderTargetName RT1, ERenderTargetName RT2 ) const;
	void SwapRenderTargetPointers( ERenderTargetName RT1, ERenderTargetName RT2 );

	//! Set persistent surface dirty
	void SetPersistentSurfaceDirty( ePersistentSurface surface, Bool newValue );

	//! Set all persistent surfaces dirty
	void SetAllPersistentSurfacesDirty( Bool newValue );

	//! Is persistent surface dirty
	Bool IsPersistentSurfaceDirty( ePersistentSurface surface ) const;

	void CreateResources();

	void ReleaseResources();

protected:
	//! Create rendering surfaces
	void CreateSurfaces();

private:
	//! Called when device is lost
	virtual void OnDeviceLost();

	//! Called when device is being reset
	virtual void OnDeviceReset();
};
