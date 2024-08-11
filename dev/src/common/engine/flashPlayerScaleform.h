/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#ifdef USE_SCALEFORM

#include "flashPlayer.h"
#include "flashPlayer.h"
#include "flashPlayerScaleformInput.h"
#include "flashFontConfig.h"
#include "inputKeys.h"
#include "inputBufferedInputEvent.h"

#include "scaleformPlayer.h"
#include "scaleformExternalInterface.h"
#include "scaleformUserEvent.h"

class CFlashMovieScaleform;
class CScaleformLoader;
class CScaleformSystem;

class CFlashPlayerScaleform
	: public CFlashPlayer
	, public IScaleformPlayer
	, public IScaleformInputEventListener
	, public IScaleformExternalInterfaceHandler
	, public IScaleformUserEventHandler
{
private:
	struct SFlashMovieLayerReverseSortFunc
	{
		static RED_INLINE Bool Less( const CFlashMovieScaleform* key1, const CFlashMovieScaleform* key2 ) { return key1->GetLayerInfo().m_layerDepth > key2->GetLayerInfo().m_layerDepth; }	
	};
private:
	Rect						m_viewport;
	CScaleformInputManager		m_inputManager;
	SF::Ptr< CScaleformLoader > m_loader;
	Bool m_isInitialized;
	CScaleformSystem*			m_system;

private:
	TDynArray< IFlashPlayerStatusListener* >								m_statusListeners;
	THashMap< String, IFlashExternalInterfaceHandler* >						m_externalInterfaceHandlers;
	TDynArray< IFlashMouseCursorHandler* >									m_mouseCursorHandlers;

	//! Movies are ordered from highest to lowest layer for event processing. Currently shouldn't actually matter.
	TSortedArray< CFlashMovieScaleform*, SFlashMovieLayerReverseSortFunc >	m_movieWeakRefs;

	Bool																	m_isInTick;
	Bool																	m_isCenterMouseRequested;
	Bool																	m_isMousePosChangeRequested;
	Float																	m_desiredMouseX;
	Float																	m_desiredMouseY;

private:
	virtual void UpdateViewportLayout( const Rect& viewport );

public:
	//! IScaleformPlayer functions
	virtual void OnScaleformInit() override;
	virtual void OnScaleformShutdown() override;
	virtual Bool RegisterScaleformMovie( CFlashMovieScaleform* movie ) override;
	virtual Bool UnregisterScaleformMovie( CFlashMovieScaleform* movie ) override;

public:
	//! IScaleformInputEventListener functions
	virtual void OnGFxKeyEvent( const GFx::KeyEvent& event );
	virtual void OnGFxMouseEvent( const GFx::MouseEvent& event );
	virtual void OnGFxGamePadAnalogEvent( const GFx::GamePadAnalogEvent& event );

public:
	//! IScaleformExternalInterfaceHandler functions
	virtual void OnExternalInterface( GFx::Movie* movie, const SFChar* methodName, const GFx::Value* args, SFUInt argCount ) override;

public:
	//! IScaleformUserEventHandler
	virtual void OnUserEvent( GFx::Movie* movie, const GFx::Event& event ) override;

public:
	CFlashPlayerScaleform();
	virtual ~CFlashPlayerScaleform();
	void Init();
	void Shutdown();

public:
	virtual Bool OnViewportInput( EInputKey key, EInputAction action, Float data ) override;
	virtual void OnViewportGenerateFragments( CRenderFrame* frame ) override;
	virtual void CenterMouse() override;
	virtual void SetMousePosition(Float xPos, Float yPos) override;

	virtual void Tick( Float timeDelta, const Rect& viewport ) override;
	virtual void Capture( Bool force ) override;

public:
	virtual Bool RegisterStatusListener( IFlashPlayerStatusListener* statusListener ) override;
	virtual Bool UnregisterStatusListener( IFlashPlayerStatusListener* statusListener ) override;

public:
	virtual Bool RegisterExternalInterface( const String& methodName, IFlashExternalInterfaceHandler* flashExternalInterfaceHandler ) override;
	virtual Bool UnregisterExternalInterface( const String& methodName ) override;

public:
	virtual Bool RegisterMouseCursorHandler( IFlashMouseCursorHandler* flashMouseCursorHandler ) override;
	virtual Bool UnregisterMouseCursorHandler( IFlashMouseCursorHandler* flashMouseCursorHandler ) override;

public:
	virtual CFlashMovie* CreateMovie( const TSoftHandle< CSwfResource >& swfHandle, const SFlashMovieInitParams& initParams ) override;

private:
	void NotifyShuttingDown();
};

#endif // USE_SCALEFORM
