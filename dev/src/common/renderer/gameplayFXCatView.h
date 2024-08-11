/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "gameplayFx.h"

#include "../../common/game/gameSystem.h"

class CCatViewEffectPostFX : public IGameplayEffect
{

private:
	Float						m_timeSinceStart;
	Float						m_fadeInTime;
	Float						m_fadeOutTime;
	Bool						m_isFadingOut;

	Vector						m_tintColorNear;
	Vector						m_tintColorFar;
	Float						m_desaturation;

	Float						m_viewRange;
	Float						m_blurStrength;
	Float						m_brightStrength;

	Float						m_pulseBase;
	Float						m_pulseScale;
	Float						m_pulseSpeed;
	Float						m_blurSize;
	Float						m_hightlightInterior;	//!< Scale of the color inside the objects
	
	Float						m_fogStartOffset;
	Float						m_fogDensity;

	Vector						m_hightlightColor;		//!< Color of the highlighted objects. Alpha is the multiplier

	Vector						m_playerPosition;
	Bool						m_autoPositioning;

	Uint8						m_maskStencilBits;

public:
	CCatViewEffectPostFX( CGameplayEffects* parent );

	~CCatViewEffectPostFX();

	//-------------------------------------------
	// Common FX methods

	virtual void Init();

	virtual void Shutdown();

	virtual Bool Apply( CRenderCollector &collector, const CRenderFrameInfo &frameInfo, ERenderTargetName rtnColor, ERenderTargetName rtnTarget );

	virtual void Tick( Float time );

	virtual void Reset();

	//-------------------------------------------

	void Enable( Float fadeInTime );

	void Disable( Float fadeOutTime );
	
	RED_INLINE void SetFog( Float fogDensity, Float fogStartOffset )
	{
		m_fogDensity = fogDensity;
		m_fogStartOffset = fogStartOffset;
	}

	RED_INLINE void SetPlayerPosition( const Vector& playerPosition , Bool autoPositioning = false )
	{ 
		m_playerPosition = playerPosition; 
		m_autoPositioning = autoPositioning;
	}

	RED_INLINE void SetDesaturation( Float desaturation ) { m_desaturation = desaturation; }

	RED_INLINE void SetTintColors( const Vector& tintNear , const Vector& tintFar ) 
	{  
		m_tintColorNear = tintNear;
		m_tintColorFar = tintFar;
	}

	RED_INLINE void SetHightlight( Vector color , Float hightlightInterior , Float blurSize )  
	{ 
		m_hightlightColor = color;
		m_hightlightInterior = hightlightInterior;
		m_blurSize = blurSize;
	}

	RED_INLINE void SetBrightness( Float brightStrength ) { m_brightStrength = brightStrength; }

	RED_INLINE void SetViewRange( Float viewRange ) { m_viewRange = viewRange; }

	RED_INLINE void SetPulseParams( Float base , Float scale , Float speed ) { m_pulseBase = base; m_pulseScale = scale; m_pulseSpeed = speed; }

};
