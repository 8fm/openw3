/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../../common/engine/platformViewport.h"
#include "../../common/engine/baseEngine.h"

// This is a temporary interface and will be removed when the new platform projects come online
// We will need to (eventually) make interfaces for the viewport / window / input stuff too
class CEditorPlatformViewport : public IPlatformViewport
{
public:
	Bool PumpMessages()
	{
		return SPumpMessages();
	}
};

/// Editor version of the engine
class CEditorEngine : public CBaseEngine, public IScriptCompilationFeedback
{
public:
	CEditorEngine();

	// Initialize engine, create platform systems
	virtual Bool Initialize();

	// Shutdown engine, called when main loop exits
	virtual void Shutdown();

	// Engine tick method, called from within main loop
	virtual void Tick( float timeDelta );

	IPlatformViewport* GetPlatformViewport()	{ return &m_platformViewport; }

	virtual Bool IsFPSDisplayEnabled() const override { return false; }
public:

	virtual Bool IsPostInitialized() { return m_postInitialized; }
	
	virtual IScriptCompilationFeedback* QueryScriptCompilationFeedback() override { return this; }
	virtual ECompileScriptsReturnValue OnCompilationFailed( const CScriptCompilationMessages& ) override;

private:
	CEditorPlatformViewport m_platformViewport;
};
