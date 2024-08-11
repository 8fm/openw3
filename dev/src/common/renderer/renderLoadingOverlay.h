/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../engine/videoPlayer.h"
#include "../engine/flashValue.h"
#include "../engine/flashMovie.h"
#include "../engine/flashPlayer.h"

//////////////////////////////////////////////////////////////////////////
// CRenderLoadingOverlay
//////////////////////////////////////////////////////////////////////////
class CRenderLoadingOverlay : public IFlashExternalInterfaceHandler
{
private:
	enum EVisiblityLatch : Uint8
	{
		eVisiblityLatch_None,
		eVisiblityLatch_SetVisible,
		eVisiblityLatch_SetInvisible,
	};

private:
	String						m_caption;

private:
	CFlashMovie*				m_flashMovie;
	CFlashValue					m_flashSprite;

private:
	Double						m_noninteractiveDuration;
	Double						m_noninteractiveCooldown;
	Double						m_fadeOutCooldown;

private:
	Float						m_origY;

private:
	EVisiblityLatch				m_visiblityLatch;
	Bool						m_forceVisible:1;
	Bool						m_isVisible:1;
	Bool						m_latchedVisible:1;
	Bool						m_wasNoninteractive:1;
	Bool						m_hackPendingResetPosition:1;

private:
	struct SFlashHooks
	{
		CFlashValue				m_fadeIn;
		CFlashValue				m_fadeOut;
	};

	SFlashHooks m_flashHooks;

public:
	//! IFlashExternalInterfaceHandler functions
	virtual void OnFlashExternalInterface( const String& methodName, CFlashMovie* flashMovie, const TDynArray< CFlashValue >& args, CFlashValue& outRetval ) override;

public:
	CRenderLoadingOverlay();
	~CRenderLoadingOverlay();

public:
	void Tick( Float timeDelta, Float unclampedTimeDelta );

public:
	void InitWithFlash( CFlashMovie* flashMovie );
	void FadeIn( const String& caption );
	void FadeOut( const String& caption );

public:
	String GetCaption() const { return m_forceVisible ? TXT("<forced>") : m_caption; }

private:
	void UpdateForceVisible( Float timeDelta );
	void HACK_ChangePosition();
	void HACK_ResetPosition();

private:
	Bool RegisterLoadingOverlay( CFlashValue& flashSprite );

private:
	void DoFadeOutFlash( Float fadeOutTime );
	void DoFadeInFlash( Float fadeInTime );
};
