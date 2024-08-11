/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
* Kamil Nowakowski
*/

#pragma once
#include "renderProxy.h"
#include "renderProxyDrawable.h"
#include "renderDissolve.h"


//**********************************************************************************************************//
//**********************************************************************************************************//
// Base class for any fadeable objects

class IRenderProxyFadeable : public IRenderProxyBase
{
	EFadeType								m_fadeType:8;				//!< Should this component be faded and how?

	CRenderDissolveValue					m_genericFade;				//!< Generic fade value (proxy fade in/fade out)
	CRenderDissolveValue					m_temporaryFade;			//!< Temporary fade value

public:

	IRenderProxyFadeable( ERenderProxyType type, const RenderProxyInitInfo& initInfo );

	virtual ~IRenderProxyFadeable();

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - 

	//! Get fade type
	RED_INLINE EFadeType					GetFadeType() const { return m_fadeType; }

	RED_INLINE const CRenderDissolveValue&	GetGenericDissolve() const { return m_genericFade; }

	RED_INLINE const CRenderDissolveValue&	GetTemporaryDissolve() const { return m_temporaryFade; }

	RED_INLINE Bool							IsFading( const CRenderDissolveSynchronizer& sc ) const { return m_genericFade.IsFading(sc) || m_temporaryFade.IsFading(sc); }

	RED_INLINE CRenderDissolveAlpha			CalcMergedDissolve( const CRenderDissolveSynchronizer& sc ) const { return m_genericFade.ComputeAlpha( sc ) * m_temporaryFade.ComputeAlpha( sc ); }

	//! Compute dissolve values for given LOD
	const Vector							CalcDissolveValues( ) const;

	//! Is the proxy visible with respect to fading (alpha > 0)?
	Bool									IsFadeVisible() const;

	/// Helper function for fade update
	void									UpdateDistanceFade( Float currentDistanceSq , Float autoHideDistanceSq, Bool doFade );

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// Generic fade interface

	Float									GetGenericFadeFraction() const;

	CRenderDissolveAlpha					GetGenericFadeAlpha() const;

	RED_INLINE CRenderDissolveAlpha			GetGenericFadeAlpha( const CRenderDissolveSynchronizer& sc ) const { return m_genericFade.ComputeAlpha( sc ); }
	
	RED_INLINE void							SetGenericFadeZero() { m_genericFade.SetAlphaZero(); }

	RED_INLINE void							SetGenericFadeOne() { m_genericFade.SetAlphaOne(); }

	void									FinishGenericFade();

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// Temporal fade interface

	Float									GetTemporalFadeFraction() const;

	CRenderDissolveAlpha					GetTemporalFadeAlpha() const;

	RED_INLINE CRenderDissolveAlpha			GetTemporalFadeAlpha( const CRenderDissolveSynchronizer& sc ) const { return m_temporaryFade.ComputeAlpha( sc ); }

	RED_INLINE void							SetTemporalFadeZero() { m_temporaryFade.SetAlphaZero(); }

	RED_INLINE void							SetTemporalFadeOne() { m_temporaryFade.SetAlphaOne(); }

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - 

	//! Set fade type
	void									SetFadeType( EFadeType type );

	//! Set temporary fade
	void									SetTemporaryFade();

	//! When in FT_FadeOutAndDestroy, is the fade complete and this proxy ready for destruction?
	bool									IsFadeAndDestroyFinished( Int32 currFrameIndex );

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - 

};

//**********************************************************************************************************//
//**********************************************************************************************************//
