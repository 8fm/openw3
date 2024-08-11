/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "nodeTransformManager.h"

class CEdRenderingPanel;

/// Widget manager
class CViewportWidgetManager
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Editor );
protected:
	CEdRenderingPanel*					m_viewport;
	TDynArray< CViewportWidgetBase* >	m_widgets;
	CViewportWidgetBase*				m_highlighted;
	CViewportWidgetBase*				m_dragged;
	Float								m_sensitivity;
	ERPWidgetMode						m_widgetMode;			//!< Current widget mode
	ERPWidgetSpace						m_widgetSpace;			//!< Current widget space
	Bool								m_enabled;
	Bool								m_first;
	Vector2								m_initMousePos;
	Vector2								m_newMousePos;

public:
	CViewportWidgetManager( CEdRenderingPanel *viewport );
	~CViewportWidgetManager();

	// Get widget mode
	RED_INLINE ERPWidgetMode GetWidgetMode() const { return m_widgetMode; }

	// Get widget space
	RED_INLINE ERPWidgetSpace GetWidgetSpace() const { return m_widgetSpace; }

	// Get widget space
	RED_INLINE CEdRenderingPanel *GetViewport() const { return m_viewport; }

	// Set widget mode
	void SetWidgetMode( ERPWidgetMode mode );

	// Set widget space
	void SetWidgetSpace( ERPWidgetSpace space ) { m_widgetSpace = space; }

	// Get rendering panel we are operating on
	CEdRenderingPanel* GetRenderingPanel();

	// Add viewport widget
	void AddWidget( const String& groupName, CViewportWidgetBase *widget );

	// Remove viewport widget
	void RemoveWidget( CViewportWidgetBase* widget );

	// Set mouse sensitivity
	void SetMouseSensivitity( Float scale );

	// Toggle widgets
	void EnableWidgets( bool state );

public:
	// Viewport events
	virtual Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y );
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );
	virtual Bool OnViewportMouseMove( const CMousePacket& packet );
	virtual Bool OnViewportTrack( const CMousePacket& packet );

protected:
	// Get widget at given mouse coordinates
	CViewportWidgetBase *GetWidgetAtPoint( const wxPoint &point ) const;
	void UpdateWidgetGroups();

	// Toggle widget group visibility
	void EnableGroup( const String& group, Bool state );

private:
	// Update active widgets
	Bool UpdateHighlighedWidget();

};
