/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderProxy.h"
#include "renderProxyFadeable.h"

enum EDimmerType : CEnum::TValueType;

/// Dimmer proxy
class CRenderProxy_Dimmer : public IRenderProxyDrawable
{
protected:
	EDimmerType	m_dimmerType;
	Bool		m_isMarker;
	Float		m_ambientLevel;
	Float		m_marginFactor;

private:
	Uint32		m_budgetLastFrameIndex;
	Float		m_fadeAlphaBudget;

public:
	CRenderProxy_Dimmer( const RenderProxyInitInfo& initInfo );
	virtual ~CRenderProxy_Dimmer();

	virtual void OnNotVisibleFromAutoHide( CRenderCollector& collector ) override;

	//! Collect elements for rendering
	virtual void CollectElements( CRenderCollector& collector ) override;


public:
	//! Set budget fade alpha
	void SetBudgetFadeAlpha( Float alpha );

	//! Update budget fade alpha
	void UpdateBudgetFadeAlpha( Float frameTime, Bool isInMainRange );

public:
	RED_FORCE_INLINE Bool IsMarker() const { return m_isMarker; }
	RED_FORCE_INLINE EDimmerType GetDimmerType() const { return m_dimmerType; }
	RED_FORCE_INLINE Float GetAmbientLevel() const { return m_ambientLevel; }
	RED_FORCE_INLINE Float GetMarginFactor() const { return m_marginFactor; }
	RED_FORCE_INLINE Float GetFadeAlphaCombined() const { return Min( GetGenericFadeFraction(), m_fadeAlphaBudget ); }
	RED_FORCE_INLINE Float GetFadeAlphaBudget() const { return m_fadeAlphaBudget; }

protected:
	const EFrameUpdateState UpdateOncePerFrame( const CRenderCollector& collector );
	void UpdateFade( const CRenderCollector& colector, const Bool wasVisibleLastFrame );
};
