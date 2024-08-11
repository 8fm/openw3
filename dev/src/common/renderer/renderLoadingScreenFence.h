/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../engine/loadingScreen.h"

//////////////////////////////////////////////////////////////////////////
// ELoadingScreenFenceState
//////////////////////////////////////////////////////////////////////////
enum ELoadingScreenFenceState
{
	eLoadingScreenFenceState_Shown,
	eLoadingScreenFenceState_StartFade,
	eLoadingScreenFenceState_FadeInProgress,
	eLoadingScreenFenceState_Finished,
};

//////////////////////////////////////////////////////////////////////////
// CRenderLoadingScreenFence
//////////////////////////////////////////////////////////////////////////
class CRenderLoadingScreenFence : public ILoadingScreenFence
{
public:
	CRenderLoadingScreenFence( const SLoadingScreenFenceInitParams& initParams );

private:
	const SLoadingScreenFenceInitParams	m_initParams;
	ELoadingScreenFenceState			m_state;

public:
	virtual Bool						IsActive() const override { return const_cast< volatile ELoadingScreenFenceState& >( m_state ) != eLoadingScreenFenceState_Finished; }

public:
	CFlashMovie*						GetFlashMovie() const { return m_initParams.m_flashMovie; }
	IViewport*							GetViewport() const { return m_initParams.m_viewport; }
	const String&						GetCaption() const { return m_initParams.m_caption; }
	const String&						GetInitString() const { return m_initParams.m_initString; }

public:
	ELoadingScreenFenceState			GetState() const { return m_state; }
	void								SetState( ELoadingScreenFenceState state ) { m_state = state; }
};
