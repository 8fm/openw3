/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
* Kamil Nowakowski
*/

#pragma once
#include "renderProxy.h"
#include "renderProxyDrawable.h"

class IRenderProxyDissolvable;

//**********************************************************************************************************//
//**********************************************************************************************************//
/// Common group for render proxies
/// Each group contains low and high detail proxies
class CRenderProxyObjectGroup : public IRenderObject
{
public:
	CRenderProxyObjectGroup( const Uint64 groupTag );
	~CRenderProxyObjectGroup();

	RED_INLINE const Uint64 GetGroupTag() const { return m_groupTag; }

	// register/unregister high res proxy
	void RegisterProxy( IRenderProxyDissolvable* proxy );
	void UnregisterProxy( IRenderProxyDissolvable* proxy );

	// are we ready to switch to high res ?
	// condition: all high res proxy data is loaded
	Bool IsGroupReadyForRendering( const CRenderCollector& collector );

private:
	Uint64										m_groupTag;

	// ready flag, latches false->true when all highres proxies are ready
	// changes to false every time a high res proxy is registered
	Bool										m_ready; 

	// high-res proxies
	TDynArray< IRenderProxyDissolvable* >		m_proxies; 
};

//**********************************************************************************************************//
//**********************************************************************************************************//
// Extension for objects with lod and shadow casting ability

class IRenderProxyDissolvable : public IRenderProxyDrawable
{
	// LOD & Dissolve
	CRenderLODSelector						m_lodSelector;							//!< LOD selection & visibility logic
	CRenderDissolveAlpha					m_shadowFadeAlpha;						//!< Shadow fade alpha

public:

	IRenderProxyDissolvable( ERenderProxyType type, const RenderProxyInitInfo& initInfo );

	~IRenderProxyDissolvable();

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - 

	Bool									IsDissolved() const;

	Bool									IsShadowDissolved() const;

	//! Compute dissolve values for given LOD
	const Vector							CalcDissolveValues( const Int32 lodIndex = 0 ) const;

	//! Calculate the shadow dissolve values for given cascade
	const Vector							CalcShadowDissolveValues( const Int32 lodIndex = 0 ) const;

	const Vector							CalcShadowDissolveValuesNoLod( ) const;

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - 

	RED_INLINE CRenderLODSelector&			GetLodSelector() { return m_lodSelector; }

	RED_INLINE const CRenderLODSelector&	GetLodSelector() const { return m_lodSelector; }

	RED_INLINE Float						GetShadowFadeFraction() const { return m_shadowFadeAlpha.ToFraction(); }

	RED_INLINE const CRenderDissolveAlpha&	GetShadowFadeAlpha() const { return m_shadowFadeAlpha; }

	RED_INLINE void							SetShadowFadeZero() { m_shadowFadeAlpha.SetZero(); }

	RED_INLINE void							SetShadowFadeOne() { m_shadowFadeAlpha.SetOne(); }

	RED_INLINE void							SetShadowfadeAlpha( const CRenderDissolveAlpha& shadowFadeAlpha ) { m_shadowFadeAlpha = shadowFadeAlpha; }

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - 

	virtual Bool							IsProxyReadyForRendering() const;

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - 

protected:

	void									GetLOD( const CRenderCollector& collector, Int32& outBaseLOD, Int32& outNextLOD, Bool* outLODChange ) const;

	/// Update internal LOD calculations
	void									UpdateLOD( const CRenderCollector& collector, const Bool wasVisibleLastFrame, Bool forceLod0 = false );
	void									UpdateLOD( const Float altLodDistance, const CRenderCollector& collector, const Bool wasVisibleLastFrame, const Bool forceLod0 = false, const Int8 minLOD=-1, const Int8 maxLOD=100 );

	/// Update internal fade stuff
	void									UpdateFades( const CRenderCollector& collector, const Bool wasVisibleLastFrame );

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - 

};


//**********************************************************************************************************//
//**********************************************************************************************************//
