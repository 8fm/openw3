/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "gameplayFx.h"
#include "renderSimDiffusion2D.h"

struct SFocusModeGroup
{
	CName m_name;
	Vector m_color;
};


class CFocusModeEffectPostFX : public IGameplayEffect
#ifndef NO_EDITOR_EVENT_SYSTEM
	, public IEdEventListener
#endif
{
private:
	CRenderSimDiffusion2D			m_diffusion;
	Float							m_timeSinceStart;		
	Float							m_fadeInTime;
	Bool							m_isFadingOut;

	Float							m_desaturation;
	Float							m_highlightBoost;
	Float							m_effectDistanceShift;		

	Float							m_effectZoom;			// Amount of focus zoom & edge blur
	Float							m_fadeRange;			// Distance range where effect fades out completely
	Vector2							m_fadeCenterExponent;	// Exponent applied to X position, to fade out away from center
	Vector							m_fadeCenterStartRange;

	Float							m_dimmingFactor;
	Float							m_dimmingTime;
	Float							m_dimmingSpeed;
	Float							m_currentDimmingRate;
	Bool							m_isFadingOutInteractive;
	Bool							m_isFadingInInteractive;
	Bool							m_isEnabledExtended;

	Vector							m_playerPosition;

	TDynArray< SFocusModeGroup >	m_fmGroups;

#ifndef NO_EDITOR_EVENT_SYSTEM
	mutable Red::Threads::CMutex	m_groupsMutex;
#endif

public:
	static Uint8 GetUsedStencilBits();

public:
	CFocusModeEffectPostFX( CGameplayEffects* parent );

	~CFocusModeEffectPostFX();

	virtual void Init();

	virtual void Shutdown();

	void Enable( Float desaturation, Float highlightBoost );
	void Disable(  Bool forceDisable  );

	void EnableExtended( Float fadeInTime );
	void DisableExtended( Float fadeOutTime );

	void SetDimming( Bool dimming );

	void SetFadeParameters( Float fadeNear, Float fadeFar, Float dimmingTime, Float dimmingSpeed );

	void SetPlayerPosition( const Vector& playerPosition );

	virtual Bool Apply( CRenderCollector &collector, const CRenderFrameInfo &frameInfo, ERenderTargetName rtnColor, ERenderTargetName rtnTarget );

	virtual void Tick( Float time );

#ifndef NO_EDITOR_EVENT_SYSTEM
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );
#endif

protected:
	void ReloadFocusModeSettings();

	// In editor, must lock m_groupsMutex before calling, and keep locked while reference is held!
	const SFocusModeGroup& FindFocusModeGroup( const CName& name ) const;
};


