/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiToolTipManager.h"
#include "redGuiManager.h"

namespace RedGui
{
	CRedGuiToolTipManager::CRedGuiToolTipManager( CRedGuiManager* redGuiManager )
		: m_delayVisible( 0.5f )
		, m_oldFocusControl( nullptr )
		, m_toolTipVisible( false )
		, m_currentTime( 0 )
		, m_needToolTip( false )
		, m_currentFocusControl( nullptr )
	{
		redGuiManager->EventTick.Bind( this, &CRedGuiToolTipManager::NotifyEventFrameTick );
		redGuiManager->GetInputManager()->EventChangeMouseFocus.Bind( this, &CRedGuiToolTipManager::NotifyEventChangeMouseFocus );
	}

	CRedGuiToolTipManager::~CRedGuiToolTipManager()
	{
		GRedGui::GetInstance().EventTick.Unbind( this, &CRedGuiToolTipManager::NotifyEventFrameTick );
		GRedGui::GetInstance().GetInputManager()->EventChangeMouseFocus.Unbind( this, &CRedGuiToolTipManager::NotifyEventChangeMouseFocus );
	}

	Float CRedGuiToolTipManager::GetDelayVisible() const
	{
		return m_delayVisible;
	}

	void CRedGuiToolTipManager::SetDelayVisible( Float val )
	{
		m_delayVisible = val;
	}

	void CRedGuiToolTipManager::NotifyEventFrameTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime )
	{
		if( m_oldFocusControl != m_currentFocusControl )
		{
			if( m_toolTipVisible == true )
			{
				HideToolTip( m_oldFocusControl );
			}
			m_oldFocusControl = m_currentFocusControl;
			m_needToolTip = false;

			if( m_oldFocusControl != nullptr )
			{
				m_currentTime = 0;
				m_oldMousePosition = GRedGui::GetInstance().GetInputManager()->GetMousePosition();
				m_needToolTip = m_oldFocusControl->GetNeedToolTip();
			}
		}
		
		if( m_needToolTip == true )
		{
			Vector2 position = GRedGui::GetInstance().GetInputManager()->GetMousePosition();
			if( m_toolTipVisible == false && position != m_oldMousePosition )
			{
				if( m_toolTipVisible == true )
				{
					HideToolTip( m_oldFocusControl );
				}
				m_currentTime = 0;
				m_oldMousePosition = position;
			}
			else
			{
				if( m_toolTipVisible == false )
				{
					m_currentTime += deltaTime;
					if( m_currentTime >= m_delayVisible )
					{
						Vector2 tooltipPosition( position );
						tooltipPosition.Y += 20;	// fix position under the cursor (20px x 20px)
						ShowToolTip( m_oldFocusControl, tooltipPosition );
					}
				}
			}
		}
	}

	void CRedGuiToolTipManager::HideToolTip( CRedGuiControl* control )
	{
		m_toolTipVisible = false;
		if( control != nullptr )
		{
			control->PropagateToolTip( false, Vector2( 0.0f,0.0f ) );
		}
	}

	void CRedGuiToolTipManager::ShowToolTip( CRedGuiControl* control, const Vector2& position )
	{
		m_toolTipVisible = true;
		if( control != nullptr )
		{
			control->PropagateToolTip( true, position );
		}
	}

	void CRedGuiToolTipManager::NotifyEventChangeMouseFocus( RedGui::CRedGuiEventPackage& eventPackage )
	{
		m_currentFocusControl = eventPackage.GetEventSender();
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
