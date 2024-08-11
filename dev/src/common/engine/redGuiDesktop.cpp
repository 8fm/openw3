/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiManager.h"
#include "redGuiPanel.h"
#include "redGuiMenuBar.h"
#include "redGuiDesktop.h"

namespace RedGui
{
	namespace
	{
		const Uint32 GMenuBarHeight = 20;
	}

	CRedGuiDesktop::CRedGuiDesktop()
		: CRedGuiControl( 0, 0, 800, 600 ),
			m_menuBar( nullptr )
	{
		SetBorderVisible( false );
		SetNeedKeyFocus( true );

		GRedGui::GetInstance().RegisterDesktop( this );

		// create panel for menu bar
		m_menuBarPanel = new CRedGuiPanel( 0, 0, GetWidth(), GMenuBarHeight );
		m_menuBarPanel->SetVisible(false);
		m_menuBarPanel->SetPadding(Box2::IDENTITY);
		this->AddChild(m_menuBarPanel);
		m_menuBarPanel->SetDock(DOCK_Top);

		// create client area
		m_client = new CRedGuiPanel(0, 0, GetWidth(), GetHeight() );
		SetControlClient(m_client);
		m_client->SetDock(DOCK_Fill);
		m_client->SetBackgroundColor( Color::CLEAR );
		m_client->SetBorderVisible( false );	
	}

	CRedGuiDesktop::~CRedGuiDesktop()
	{
		GRedGui::GetInstance().UnregisterDesktop( this );
	}

	CRedGuiWindow* CRedGuiDesktop::AddWindow( CRedGuiWindow* window )
	{
		window->EventRootKeyChangeFocus.Bind( this, &CRedGuiDesktop::NotifyOnRootKeyChangeFocus );
		window->EventWindowOpened.Bind( this, &CRedGuiDesktop::NotifyOnWindowOpened );
		AddChild( window );
		m_keyFocusedControls.PushBackUnique( window );
		return window;
	}

	void CRedGuiDesktop::RemoveWindow( CRedGuiWindow* window )
	{
		window->EventRootKeyChangeFocus.Unbind( this, &CRedGuiDesktop::NotifyOnRootKeyChangeFocus );
		window->EventWindowOpened.Unbind( this, &CRedGuiDesktop::NotifyOnWindowOpened );
		m_keyFocusedControls.Remove( window );
		RemoveChild( window );
	}

	void CRedGuiDesktop::AddChild( CRedGuiControl* child )
	{
		CRedGuiControl::AddChild( child );
	}

	void CRedGuiDesktop::RemoveChild( CRedGuiControl* child )
	{
		CRedGuiControl::RemoveChild( child );
	}

	void CRedGuiDesktop::Draw()
	{
		/* intentionally empty */
	}

	void CRedGuiDesktop::SetMenuBar(CRedGuiMenuBar* menuBar)
	{
		if( m_menuBar != nullptr )
		{
			m_menuBarPanel->SetVisible(false);
			m_menuBarPanel->RemoveChild( m_menuBar );
			m_keyFocusedControls.Remove( m_menuBar );

			// move client area
			m_client->SetSize(Vector2(m_client->GetSize().X, GetSize().Y ));
		}

		if( menuBar != nullptr )
		{
			m_menuBar = menuBar;
			m_menuBar->SetMargin( Box2::ZERO );
			m_menuBar->SetDock(DOCK_Fill);
			m_keyFocusedControls.PushBackUnique( m_menuBar );

			m_menuBarPanel->SetVisible(true);
			m_menuBarPanel->SetPadding( Box2::ZERO );
			m_menuBarPanel->AddChild(menuBar);

			// move client area
			m_client->SetSize( Vector2( m_client->GetSize().X, GetSize().Y - m_menuBarPanel->GetSize().Y ) );
		}
	}

	CRedGuiMenuBar* CRedGuiDesktop::GetMenuBar() const
	{
		return m_menuBar;
	}

	void CRedGuiDesktop::ResizeLayerItemView( const Vector2& oldView, const Vector2& newView )
	{
		SetMaxSize( newView );
		SetSize( newView );
		CRedGuiControl::ResizeLayerItemView( oldView, newView );
	}

	void CRedGuiDesktop::AddToMenuBar( RedGui::CRedGuiControl* control )
	{
		m_menuBarPanel->AddChild( control );
		control->SetDock( RedGui::DOCK_Right );
		m_keyFocusedControls.PushBackUnique( control );
	}

	Bool CRedGuiDesktop::OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data )
	{
		Bool result = false;
		Int32 previousIndex = m_activeKeyFocusedControl;

		if( event == RGIE_PreviousWindow )
		{
			do
			{
				--m_activeKeyFocusedControl;
				if( m_activeKeyFocusedControl < 0 )
				{
					m_activeKeyFocusedControl = (Int32)( m_keyFocusedControls.Size() - 1 );
				}

				if( m_keyFocusedControls[m_activeKeyFocusedControl]->GetVisible() == true && m_keyFocusedControls[m_activeKeyFocusedControl]->GetEnabled() == true )
				{
					m_focusedDesktopControl = m_keyFocusedControls[m_activeKeyFocusedControl];
					m_focusedDesktopControl->MoveToTop();
					GRedGui::GetInstance().GetInputManager()->SetKeyFocusControl( m_focusedDesktopControl );
					break;
				}
			}while( m_activeKeyFocusedControl != previousIndex );
			result = true;
		}
		else if( event == RGIE_NextWindow )
		{
			do
			{
				++m_activeKeyFocusedControl;
				if( m_activeKeyFocusedControl == (Int32)m_keyFocusedControls.Size() )
				{
					m_activeKeyFocusedControl = 0;
				}

				if( m_keyFocusedControls[m_activeKeyFocusedControl]->GetVisible() == true && m_keyFocusedControls[m_activeKeyFocusedControl]->GetEnabled() == true )
				{
					m_focusedDesktopControl = m_keyFocusedControls[m_activeKeyFocusedControl];
					m_focusedDesktopControl->MoveToTop();
					GRedGui::GetInstance().GetInputManager()->SetKeyFocusControl( m_focusedDesktopControl );
					break;
				}
			}while( m_activeKeyFocusedControl != previousIndex );

			result = true;
		}

		if( m_focusedDesktopControl != nullptr )
		{
			if( m_focusedDesktopControl->PropagateInternalInputEvent( event, data ) == true )
			{
				result = true;
			}
		}

		return result;
	}

	void CRedGuiDesktop::NotifyOnRootKeyChangeFocus( RedGui::CRedGuiEventPackage& eventPackage, Bool value )
	{
		if( value == false )
		{
			m_focusedDesktopControl = nullptr;
		}
		else
		{
			if( eventPackage.GetEventSender()->IsAWindow() == true )
			{
				m_focusedDesktopControl = eventPackage.GetEventSender();

				for( Uint32 i=0; i<m_keyFocusedControls.Size(); ++i )
				{
					if( m_focusedDesktopControl == m_keyFocusedControls[i] )
					{
						m_activeKeyFocusedControl = i;
						break;
					}
				}
			}
		}
	}

	void CRedGuiDesktop::NotifyOnWindowOpened( RedGui::CRedGuiEventPackage& eventPackage )
	{
		if( eventPackage.GetEventSender()->IsAWindow() == true )
		{
			m_focusedDesktopControl = eventPackage.GetEventSender();
			for( Uint32 i=0; i<m_keyFocusedControls.Size(); ++i )
			{
				if( m_focusedDesktopControl == m_keyFocusedControls[i] )
				{
					m_activeKeyFocusedControl = i;
					break;
				}
			}
		}
	}

	void CRedGuiDesktop::SetActiveModalWindow( CRedGuiControl* modalWindow )
	{
		m_focusedDesktopControl = modalWindow;
		m_focusedDesktopControl->MoveToTop();
		GRedGui::GetInstance().GetInputManager()->SetKeyFocusControl( m_focusedDesktopControl );
	}

} // namespace RedGui

#endif	// NO_RED_GUI
