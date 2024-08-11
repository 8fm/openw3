/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
* Kamil Nowakowski
*/
#pragma  once

#include "renderLoadingScreenFence.h"

class CRenderTarget;
class CPostProcessDrawer;

class CLoadingScreenBlur
{

	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );

	Bool					m_enabled;
	Bool					m_fadingOut;
	Bool					m_useFallback;

	CRenderTarget*			m_temporalBuffer[2];	//!< Buffer used for temporal bluring
	Uint16					m_flipSide;				//!< Define current input/output 
	
	Float					m_blurProgress;		//!< Dunno what yet
	Float					m_fadeScale;		//!< Dunno what yet
	Float					m_fadeTime;
	Float					m_maxBluringTime;

	Float					m_blurScaleTempo;	//!<
	Float					m_blurSourceTempo;	//!<

	Uint8					m_sizeDiv;			//!<
	
	IViewport*				m_viewport;

	Uint32					m_refcount;

public:

	CLoadingScreenBlur (  );

	~CLoadingScreenBlur (  );

public:

	//----------------------------------------------------

	RED_INLINE Bool IsActive() const { return m_enabled; }

	RED_INLINE Bool	IsDrawable() const { return m_enabled || m_fadingOut; }

	RED_INLINE IViewport* GetViewport() const { return m_viewport; } 

	RED_INLINE void Tick( Float timeDelta ) { m_blurProgress += timeDelta; }

	//----------------------------------------------------

	// 
	void Init( IViewport* viewport , Float blurScale = 2.0f , Float timeScale = 4.0f, Bool useFallback = false );

	//
	void Deinit( Float fadeTime );

	// Render arleady blurred image over the screen
	void DrawFade( CPostProcessDrawer &drawer, const CRenderFrameInfo& frameInfo , Float timeDelta , ERenderTargetName renderSource );

	// Allows dynamic rescaling 
	Bool AllowsDynamicRescaling() const;

private:

	// reset the blur parameters to their default values
	void ResetBlurParametersToDefaults();

	//
	RED_INLINE Bool IsInitialized() const { return m_temporalBuffer[0] != NULL && m_temporalBuffer[1] != NULL; }

	// Create new buffer for bluring
	void CreateTemporalBuffer( const CRenderFrameInfo& frameInfo );

	// We don't need buffer anymore
	void ReleaseTemporalBuffer( );

	// DO blur shit
	void PerformBlur( 
		CPostProcessDrawer &drawer,			
		CRenderTarget* sourceColor, 
		const CRenderFrameInfo& frameInfo 
		);

};
