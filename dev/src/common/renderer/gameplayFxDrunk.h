
#pragma once

#include "gameplayFx.h"

class CDrunkEffectPostFX : public IGameplayEffect
{
private:
	Float						m_timeSinceStart;
	Float						m_fadeInTime;
	Float						m_fadeOutTime;
	Float						m_scale;

	Float						m_timeScale;
	Float						m_angleScale;
	Float						m_baseAngle;

	Bool						m_isFadingOut;

public:
	CDrunkEffectPostFX( CGameplayEffects* parent );

	~CDrunkEffectPostFX();

	//-------------------------------------------
	// Common FX methods

	virtual void Init();

	virtual void Shutdown();

	virtual Bool Apply( CRenderCollector &collector, const CRenderFrameInfo& frameInfo, ERenderTargetName rtnColorSource , ERenderTargetName rtnColorTarget );

	virtual void Tick( Float time );

	//-------------------------------------------

	void Enable( Float fadeInTime );

	void Disable( Float fadeOutTime );

	void Scale( Float scale );

};