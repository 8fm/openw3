/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../../common/engine/baseEngine.h"
#include "../../common/engine/viewportWindowMode.h"
#include "../../common/core/commandLineParams.h"

class CGameDebugMenu;
class CServerInterface;

/// Platform-independent Game engine
class CR4GameEngine : public CBaseEngine
{
private:
	CGameDebugMenu*		m_debugMenu;
	ViewportHandle		m_viewport;
	String				m_travelUrl;			//!< World travel URL

public:

	// Parameters struct passed on startup. Use this rather than parsing the command line
	class Parameters
	{
	public:
		Parameters();

		// Render viewport interface
		IPlatformViewport* m_renderViewport;

		// Screen resolution
		Int32	m_desktopWidth;
		Int32	m_desktopHeight;
		Int32	m_desiredWidth;						
		Int32	m_desiredHeight;
		Bool	m_resolutionOverride;
		Bool	m_forceWindowed;
		Bool	m_borderlessMode;

		// Render stuff
		Int32	m_textureResolutionClamp;			// 0 - 4, default = 2

		// Engine features
		Bool	m_enableLogging;
		Bool	m_enablePhysicsDebugger;
		Bool	m_enableKinect;
		Bool	m_enableFPSDisplay;

		// Startup features
		String	m_scriptToExecuteOnStartup;
		String	m_worldToRun;
		String	m_videoToPlay;
	};

	CR4GameEngine( const Parameters& params, const Core::CommandLineArguments& coreParams );

	// Initialize engine, create platform dependent systems
	virtual Bool Initialize();

	// Shutdown engine, called when main loop exits
	virtual void Shutdown();

	// Engine tick method, called from within main loop
	virtual void Tick( float timeDelta );

	// Hosted game has ended
	virtual void OnGameEnded();

	virtual Bool IsFPSDisplayEnabled() const override
	{
		return m_initialParameters.m_enableFPSDisplay;
	}

private:

	void ProcessViewportFilterConfigs();
	void GetViewportSettings( Int32& width, Int32& height, EViewportWindowMode& windowMode );
	void PostInitialize( float timeDelta = 0.0f );
	Bool IsPostInitialized() { return m_postInitialized; }
	IPlatformViewport* GetPlatformViewport()	{ return m_initialParameters.m_renderViewport; }

protected:
	Parameters m_initialParameters;
	Core::CommandLineArguments m_initialCoreParameters;
};
