/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class IViewport;
class CRenderFrame;
class CRenderCamera;

enum EInputKey : Int32;
enum EInputAction : Int32;

// Mouse move packet
struct CMousePacket
{
	IViewport*	m_viewport;
	Int32			m_x;
	Int32			m_y;
	Int32			m_dx;
	Int32			m_dy;
	Float		m_pressure;
	Vector		m_rayOrigin;
	Vector		m_rayDirection;

	CMousePacket( IViewport* viewport, Int32 x, Int32 y, Int32 dx, Int32 dy, Float pressure );
};

/// Rendering viewport hook
class IViewportHook
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );
public:
	//! Destructor
	virtual ~IViewportHook() {};

	//! Called when a hook is attached to a viewport
	virtual void OnHookAttached( IViewport* view ) {}

	//! Mouse movement
	virtual Bool OnViewportTrack( const CMousePacket& packet ) { return false; };

	//! Mouse movement when in captured state
	virtual Bool OnViewportMouseMove( const CMousePacket& packet ) { return false; };

	//! Generalized input event
	virtual Bool OnViewportInput( IViewport* view, EInputKey key, EInputAction action, Float data ) { return false; };

	//! Generalized mouse click, windows only
	virtual Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y ) { return false; };

	//! Create rendering frame for viewport
	virtual CRenderFrame *OnViewportCreateFrame( IViewport *view );

	//! Render viewport, should issue a render command
	virtual void OnViewportRenderFrame( IViewport *view, CRenderFrame *frame );

	//! Generate debug viewport fragments
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame ) {};

	//! Override camera
	virtual void OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera ) {};

	//! External viewport tick
	virtual void OnViewportTick( IViewport* view, Float timeDelta ) {};

	//! Kill focus
	virtual void OnViewportKillFocus( IViewport* view ) {}

	//! Set focus
	virtual void OnViewportSetFocus( IViewport* view ) {}

	//! Viewport window activated
	virtual void OnViewportActivated( IViewport* view ) {}

	//! Viewport window deactivated
	virtual void OnViewportDeactivated( IViewport* view ) {}

	//! Set new dimensions of a viewport
	virtual void OnViewportSetDimensions ( IViewport* view ) {}
};

