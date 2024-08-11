/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/engine/inputDeviceManager.h"
#include "../../common/engine/inputEditorInterface.h"
#include "../../common/engine/viewport.h"

CViewportWidgetManager::CViewportWidgetManager( CEdRenderingPanel *viewport )
	: m_viewport( viewport )
	, m_dragged( nullptr )
	, m_highlighted( nullptr )
	, m_sensitivity( 1.0f )
	, m_widgetMode( RPWM_Move )
	, m_widgetSpace( RPWS_Global )
	, m_enabled( false )
	, m_first( true )
{
}

CViewportWidgetManager::~CViewportWidgetManager()
{
	m_widgets.ClearPtr();
}

CEdRenderingPanel* CViewportWidgetManager::GetRenderingPanel()
{
	return m_viewport;
}

void CViewportWidgetManager::AddWidget( const String& groupName, CViewportWidgetBase *widget )
{
	ASSERT( widget );
	widget->m_groupName = groupName;
	widget->m_manager = this;
	m_widgets.PushBack( widget );
}

void CViewportWidgetManager::RemoveWidget( CViewportWidgetBase* widget )
{
	ASSERT( widget != nullptr );
	ASSERT( m_dragged == nullptr );
	m_widgets.Remove( widget );
}

void CViewportWidgetManager::SetMouseSensivitity( Float scale )
{
	m_sensitivity = scale;
}

void CViewportWidgetManager::SetWidgetMode( ERPWidgetMode mode )
{
	if ( mode != m_widgetMode )
	{
		m_widgetMode = mode;
		UpdateWidgetGroups();
	}
}

void CViewportWidgetManager::EnableWidgets( Bool enable )
{
	if ( enable != m_enabled )
	{
		m_enabled = enable;
		UpdateWidgetGroups();
	}
}

void CViewportWidgetManager::UpdateWidgetGroups()
{
	EnableGroup( TXT("Move"),   m_widgetMode == RPWM_Move   && m_enabled );
	EnableGroup( TXT("Rotate"), m_widgetMode == RPWM_Rotate && m_enabled );
	EnableGroup( TXT("Scale"),  m_widgetMode == RPWM_Scale  && m_enabled );
}

void CViewportWidgetManager::EnableGroup( const String& group, Bool state )
{
	for ( CViewportWidgetBase* widget : m_widgets )
	{
		if ( widget->GetGroupName().EqualsNC( group ) )
		{
			widget->EnableWidget( state );
			if ( !state && m_dragged == widget )
			{
				m_dragged->Deactivate();
				m_dragged = nullptr;
			}
		}
	}
}

Bool CViewportWidgetManager::OnViewportMouseMove( const CMousePacket& originalPacket )
{
	if ( m_dragged != nullptr )
	{	
		// get current mouse position
		POINT mousePos;
		::GetCursorPos( &mousePos );
		::ScreenToClient( (HWND)m_viewport->GetHandle(), &mousePos );
		if( m_first == true )
		{
			m_first = false;
			m_initMousePos.Set( mousePos.x, mousePos.y );
			m_newMousePos = m_initMousePos;
		}

		// calculate correct delta
 		Int32 dx = mousePos.x - m_initMousePos.X;
 		Int32 dy = mousePos.y - m_initMousePos.Y;
		m_newMousePos += Vector2( dx, dy );

		//////////////////////////////////////////////////////////////////////////
		// recreate mouse pocket object and get raw information about mouse position 
		// is needed because input pipeline in engine give wrong information about
		// delta when mouse cursor will be set to initial position
		CMousePacket packet( originalPacket.m_viewport, m_newMousePos.X, m_newMousePos.Y, dx, dy, originalPacket.m_pressure );
		if ( m_dragged->OnViewportMouseMove( packet ) )
		{
			// Restore mouse position
			POINT mouse = { m_initMousePos.X, m_initMousePos.Y };
			::ClientToScreen( (HWND)m_viewport->GetHandle(), &mouse );
			::SetCursorPos( mouse.x, mouse.y );

			return true;
		}
		else
		{
			// since acion on widget failed, then stop dragging
			m_dragged->Deactivate();
			m_dragged = nullptr;
			return false;
		}
	}

	return false;
}

Bool CViewportWidgetManager::OnViewportTrack( const CMousePacket& packet )
{
	// Update active widget
	return UpdateHighlighedWidget();
}

Bool CViewportWidgetManager::OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y )
{
	CMousePacket packet( view, x, y, 0, 0, 0.f );
	// Clicked
	if ( button==0 && state )
	{
		// Make sure about active widget
		UpdateHighlighedWidget();

		// Valid widget under mouse cursor, start dragging
		if ( m_highlighted )
		{
			m_dragged = m_highlighted;
			if ( m_dragged->Activate() )
			{
				if ( m_dragged ) // still active? yes, unfortunately Activate may show some UI, thus cause de-activation (how ironic)
				{
					view->SetMouseMode( MM_Capture );
					m_first = true;
				}
			}
			return true;
		}
	}

	// Widget drag deactivated
	if ( m_dragged && button==0 && !state )
	{
		view->SetMouseMode( MM_Normal );
		m_dragged->Deactivate();
		m_dragged = nullptr;
		return true;
	}

	// If dragging filter all events
	if ( m_dragged != nullptr )
	{
		return true;
	}

	// Not handled
	return false;
}

void CViewportWidgetManager::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	// Draw widgets
	for ( Uint32 i=0; i<m_widgets.Size(); i++ )
	{
		CViewportWidgetBase* widget = m_widgets[i];
		if ( widget != m_highlighted && widget->IsEnabled() )
		{
			widget->GenerateFragments( frame, false );
		}
	}

	// Generate fragments for active widget
	if ( m_highlighted )
	{
		m_highlighted->GenerateFragments( frame, true );
	}
}

CViewportWidgetBase *CViewportWidgetManager::GetWidgetAtPoint( const wxPoint &point ) const
{
	// Build fake render frame info
	CRenderFrameInfo info( m_viewport->GetViewport() );

	// Check last active widget first
	if ( m_dragged && m_dragged->IsEnabled() )
	{
		if ( m_dragged->CheckCollision( info, point ) )
		{
			return m_dragged;
		}
	}

	// Check collision with other widgets
	for ( Uint32 i=0; i<m_widgets.Size(); i++ )
	{
		// Test only enabled widgets
		CViewportWidgetBase *widget = m_widgets[i];
		if ( widget->IsEnabled() )
		{
			// Detect collision
			if ( widget->CheckCollision( info, point ) )
			{
				return widget;
			}
		}
	}

	// No collision
	return NULL;
}

Bool CViewportWidgetManager::UpdateHighlighedWidget()
{
	// Get mouse position
	wxPoint mouseScreenPos;
	if ( m_viewport->GetCursorPos( mouseScreenPos.x, mouseScreenPos.y ) )
	{
		m_highlighted = GetWidgetAtPoint( mouseScreenPos );
	}

	return m_highlighted != nullptr;
}
