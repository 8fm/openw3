/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "flashValue.h"
#include "flashFunction.h"
#include "inputKeys.h"
#include "inputBufferedInputEvent.h"

//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
class CSwfResource;
class CFlashMovie;
class CFlashPlayer;
class CRenderFrame;
class IViewport;

//////////////////////////////////////////////////////////////////////////
// IFlashPlayerStatusListener
//////////////////////////////////////////////////////////////////////////
class IFlashPlayerStatusListener
{
public:
	virtual void OnFlashPlayerShuttingDown() {}

protected:
	IFlashPlayerStatusListener() {}
	~IFlashPlayerStatusListener() {}
};

//////////////////////////////////////////////////////////////////////////
// IFlashExternalInterfaceHandler
//////////////////////////////////////////////////////////////////////////
class IFlashExternalInterfaceHandler
{
public:
	virtual void OnFlashExternalInterface( const String& methodName, CFlashMovie* flashMovie, const TDynArray< CFlashValue >& args, CFlashValue& outRetval )=0;

protected:	
	IFlashExternalInterfaceHandler() {}
	virtual ~IFlashExternalInterfaceHandler() {}
};

//////////////////////////////////////////////////////////////////////////
// IFlashMouseCursorHandler
//////////////////////////////////////////////////////////////////////////
class IFlashMouseCursorHandler
{
public:
	enum ECursorType
	{
		Arrow,
		Hand,
		IBeam,
		Button,
	};

public:
	virtual void OnFlashShowMouseCursor()=0;
	virtual void OnFlashHideMouseCursor()=0;
	virtual void OnFlashSetMouseCursor( ECursorType cursorType )=0;

protected:
	IFlashMouseCursorHandler() {}
	virtual ~IFlashMouseCursorHandler() {}
};

//////////////////////////////////////////////////////////////////////////
// SFlashMovieInitParams
//////////////////////////////////////////////////////////////////////////
struct SFlashMovieInitParams
{
	Int32									m_layer;
	EFlashMovieRenderGroup					m_renderGroup;
	Bool									m_attachOnStart;
	Bool									m_notifyPlayer;

	// Scaleform seems to not create a loading task if set. Careful: if waiting only for frame1 to finish, the job manager is already locked, could wait indefinitely if nothing else unlocks it from another thread
	Bool									m_waitForLoadFinish;

	SFlashMovieInitParams();
};

//////////////////////////////////////////////////////////////////////////
// CFlashPlayer
//////////////////////////////////////////////////////////////////////////
class CFlashPlayer : public Red::System::NonCopyable
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_GUI );

protected:
	CFlashPlayer() {}

public:
	static CFlashPlayer* CreateFlashPlayerInstance();

public:
	virtual ~CFlashPlayer() {}
	virtual void Tick( Float timeDelta, const Rect& viewport )=0;
	virtual void Capture( Bool force = false )=0;

public:
	virtual Bool OnViewportInput( EInputKey key, EInputAction action, Float data )=0;
	virtual void OnViewportGenerateFragments( CRenderFrame* frame )=0;

public:
	virtual void CenterMouse()=0;
	virtual void SetMousePosition(Float xPos, Float yPos)=0;

public:
	virtual Bool RegisterStatusListener( IFlashPlayerStatusListener* statusListener )=0;
	virtual Bool UnregisterStatusListener( IFlashPlayerStatusListener* statusListener )=0;

public:
	virtual Bool RegisterExternalInterface( const String& methodName, IFlashExternalInterfaceHandler* flashExternalInterfaceHandler )=0;
	virtual Bool UnregisterExternalInterface( const String& methodName )=0;

public:
	virtual Bool RegisterMouseCursorHandler( IFlashMouseCursorHandler* flashMouseCursorHandler )=0;
	virtual Bool UnregisterMouseCursorHandler( IFlashMouseCursorHandler* flashMouseCursorHandler )=0;

public:
	virtual CFlashMovie* CreateMovie( const TSoftHandle< CSwfResource >& swfHandle, const SFlashMovieInitParams& initParams )=0;
};
