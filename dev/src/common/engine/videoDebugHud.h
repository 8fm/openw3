/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "flashPlayer.h"
#include "flashValue.h"

#ifndef NO_DEBUG_PAGES

class CVideoDebugHud : public IFlashExternalInterfaceHandler
{
public:
	CVideoDebugHud();
	virtual ~CVideoDebugHud();
	void Tick( Float timeDelta );
	virtual void OnFlashExternalInterface( const String& methodName, CFlashMovie* flashMovie, const TDynArray< CFlashValue >& args, CFlashValue& outRetval ) override;

public:
	Bool PlayVideo( const String& safeDepotPath, Uint8 extraVideoParamFlags = 0 );
	void StopVideo();

private:
	Bool RegisterVideoDebugHud( CFlashValue& flashSprite );
	void CancelVideo();
	void CloseVideoDebugHud();
	void OpenVideoDebugHud();
	
private:
	void ToggleGui( Bool onOff );

private:
	void ClearFlashText();

private:
	struct FlashObjects
	{
		CFlashValue		m_debugHud;
		CFlashValue		m_tfTitle;
		CFlashValue		m_tfMetadata;
		CFlashValue		m_tfCuePoint;
		CFlashValue		m_tfSubtitles;
	};

private:
	IRenderVideo*	m_currentVideo;
	CFlashMovie*	m_flashVideoDebugHud;
	FlashObjects	m_flashObjects;
	String			m_currentVideoTitle;
	Bool			m_isFlashRegistered;
	Bool			m_isVideoTitleSet;
};

extern CVideoDebugHud* GVideoDebugHud;

#endif // #ifndef NO_DEBUG_PAGES
