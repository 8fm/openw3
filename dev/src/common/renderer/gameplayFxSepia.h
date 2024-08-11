
#pragma once

#include "gameplayFx.h"

class CSepiaEffectPostFX : public IGameplayEffect
{
private:
	Float						m_timeSinceStart;
	Float						m_fadeInTime;
	Float						m_fadeOutTime;
	Bool						m_isFadingOut;

public:
	CSepiaEffectPostFX( CGameplayEffects* parent );

	~CSepiaEffectPostFX();

	//-------------------------------------------
	// Common FX methods

	virtual void Init();

	virtual void Shutdown();

	virtual void Tick( Float time );

	virtual Bool Apply( CRenderCollector &collector, const CRenderFrameInfo& frameInfo, ERenderTargetName rtnColorSource , ERenderTargetName rtnColorTarget );

	//-------------------------------------------

	void Enable( Float fadeInTime );

	void Disable( Float fadeOutTime );

	Float GetParamStrength() const { return m_factor; }

};