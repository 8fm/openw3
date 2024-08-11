/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../../common/engine/platformViewport.h"

class CGameDebugMenu;
class CServerInterface;

// This is a temporary interface and will be removed when the new platform projects come online
// We will need to (eventually) make interfaces for the viewport / window / input stuff too
class CR6PlatformViewport : public IPlatformViewport
{
public:
	Bool PumpMessages()
	{
		return SPumpMessages();
	}
};

/// Game engine
class CGameEngine : public CBaseEngine
{
private:
	CGameDebugMenu*		m_debugMenu;
	IViewport*			m_viewport;
	String				m_travelUrl;			//!< World travel URL
	CR6PlatformViewport m_platformViewport;

public:
	CGameEngine();

	// Initialize engine, create platform dependent systems
	virtual Bool Initialize();

	// Shutdown engine, called when main loop exits
	virtual void Shutdown();

	// Engine tick method, called from within main loop
	virtual void Tick( float timeDelta );

	// Hosted game has ended
	virtual void OnGameEnded();

	// Renderer platform interface
	IPlatformViewport* GetPlatformViewport() { return &m_platformViewport; }

	virtual Bool IsFPSDisplayEnabled() const override { return false; }

private:
	// Load all "auto load resources"
	void PostInitialize( float timeDelta = 0.0f );

	Bool IsPostInitialized() { return m_postInitialized; }
};
