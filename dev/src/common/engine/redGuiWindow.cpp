/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiManager.h"
#include "redGuiButton.h"
#include "redGuiPanel.h"
#include "redGuiScrollPanel.h"
#include "redGuiLayer.h"
#include "redGuiImage.h"
#include "redGuiMenuBar.h"
#include "redGuiDesktop.h"
#include "redGuiWindow.h"
#include "inputKeys.h"

namespace RedGui
{
	namespace
	{
		const Uint32 GCaptionHeight = 20;
		const Uint32 GWindowBorderSize = 4;
		const Uint32 GCaptionButtonWidth = 16;
		const Uint32 GButtonOffset = 2;
	}

	CRedGuiWindow::CRedGuiWindow(Uint32 left, Uint32 top, Uint32 width, Uint32 height)
		: CRedGuiControl(left, top, width, height)
		, m_captionPanel(nullptr)
		, m_buttonWithText(nullptr)
		, m_exit(nullptr)
		, m_maximize(nullptr)
		, m_minimize(nullptr)
		, m_mouseRootFocus(false)
		, m_keyRootFocus(false)
		, m_client(nullptr)
		, m_movable(true)
		, m_resizable(true)
		, m_isMaximize(false)
		, m_isMinimize(false)
		, m_helpControl(nullptr)
		, m_canMoveByPad( false )
		, m_canResizeByPad( false )
	{
		SetNeedKeyFocus(true);
		SetVisible(false);
		SetBorderVisible(false);
		SetMinSize(Vector2(100, 60));

		//create background image
		m_backgroundImage = new CRedGuiImage(0,0,GetWidth(), GetHeight());
		m_backgroundImage->SetImage( Resources::GWindowBackground );
		AddChild(m_backgroundImage);
		m_backgroundImage->SetDock(DOCK_Fill);

		// create window borders
		{
			m_leftBorder = new CRedGuiPanel(0, 0, GWindowBorderSize, GetHeight());
			m_leftBorder->SetBorderVisible(false);
			m_leftBorder->SetBackgroundColor(Color::CLEAR);
			AddChild(m_leftBorder);
			m_leftBorder->SetDock(DOCK_Left);
			m_leftBorder->EventMouseDrag.Bind(this, &CRedGuiWindow::NotifyMouseBorderDrag);
			m_leftBorder->EventMouseButtonPressed.Bind(this, &CRedGuiWindow::NotifyMousePressed);
			m_leftBorder->SetPointer(MP_HResize);

			m_rightBorder = new CRedGuiPanel(0, 0, GWindowBorderSize, GetHeight());
			m_rightBorder->SetBorderVisible(false);
			m_rightBorder->SetBackgroundColor(Color::CLEAR);
			AddChild(m_rightBorder);
			m_rightBorder->SetDock(DOCK_Right);
			m_rightBorder->EventMouseDrag.Bind(this, &CRedGuiWindow::NotifyMouseBorderDrag);
			m_rightBorder->EventMouseButtonPressed.Bind(this, &CRedGuiWindow::NotifyMousePressed);
			m_rightBorder->SetPointer(MP_HResize);

			m_bottomBorder = new CRedGuiPanel(0, 0, GetWidth(), GWindowBorderSize);
			m_bottomBorder->SetBorderVisible(false);
			m_bottomBorder->SetBackgroundColor(Color::CLEAR);
			AddChild(m_bottomBorder);
			m_bottomBorder->SetDock(DOCK_Bottom);
			m_bottomBorder->EventMouseDrag.Bind(this, &CRedGuiWindow::NotifyMouseBorderDrag);
			m_bottomBorder->EventMouseButtonPressed.Bind(this, &CRedGuiWindow::NotifyMousePressed);
			m_bottomBorder->SetPointer(MP_VResize);

			// create bottom corner
			m_leftBottomCorner = new CRedGuiPanel(0, 0, GWindowBorderSize, GWindowBorderSize);
			m_leftBottomCorner->SetBorderVisible(false);
			m_leftBottomCorner->SetBackgroundColor(Color::CLEAR);
			m_leftBorder->AddChild(m_leftBottomCorner);
			m_leftBottomCorner->SetDock(DOCK_Bottom);
			m_leftBottomCorner->EventMouseDrag.Bind(this, &CRedGuiWindow::NotifyMouseBorderDrag);
			m_leftBottomCorner->EventMouseButtonPressed.Bind(this, &CRedGuiWindow::NotifyMousePressed);
			m_leftBottomCorner->SetPointer(MP_BackslashResize);

			m_rightBottomCorner = new CRedGuiPanel(0, 0, GWindowBorderSize, GWindowBorderSize);
			m_rightBottomCorner->SetBorderVisible(false);
			m_rightBottomCorner->SetBackgroundColor(Color::CLEAR);
			m_rightBorder->AddChild(m_rightBottomCorner);
			m_rightBottomCorner->SetDock(DOCK_Bottom);
			m_rightBottomCorner->EventMouseDrag.Bind(this, &CRedGuiWindow::NotifyMouseBorderDrag);
			m_rightBottomCorner->EventMouseButtonPressed.Bind(this, &CRedGuiWindow::NotifyMousePressed);
			m_rightBottomCorner->SetPointer(MP_SlashResize);
		}

		// create caption
		m_captionPanel = new CRedGuiPanel(0, 0, GetWidth(), GCaptionHeight);
		m_captionPanel->EventMouseButtonPressed.Bind(this, &CRedGuiWindow::NotifyMousePressed);
		m_captionPanel->EventMouseDrag.Bind(this, &CRedGuiWindow::NotifyMouseDrag);
		m_captionPanel->EventMouseButtonDoubleClick.Bind( this, &CRedGuiWindow::NotifyMouseButtonDoubleClick );
		m_captionPanel->SetBackgroundColor( Color::LIGHT_BLUE );
		m_captionPanel->SetTheme( RED_NAME( RedGuiGradientTheme ) );
		m_captionPanel->SetBorderVisible(false);
		m_captionPanel->SetPointer(MP_Move);
		this->AddChild(m_captionPanel);
		m_captionPanel->SetDock(DOCK_Top);
		m_captionPanel->SetPadding(Box2(10, 2, 2, 2));
		m_buttonWithText = new CRedGuiButton(0,0, GetWidth(), GCaptionHeight);
		m_buttonWithText->SetFitToText(true);
		m_buttonWithText->SetEnabled(false);
		m_buttonWithText->SetBackgroundColor(Color(0,0,0,0));
		m_buttonWithText->SetBorderVisible(false);
		m_buttonWithText->SetNeedKeyFocus( false );
		m_captionPanel->AddChild(m_buttonWithText);
		m_buttonWithText->SetDock(DOCK_Left);
		m_buttonWithText->SetPointer(MP_Move);

		// create buttons (minimize, maximize, exit)
		m_exit = new CRedGuiButton(2*GCaptionButtonWidth+GButtonOffset, GButtonOffset, GCaptionButtonWidth, GCaptionButtonWidth);
		m_exit->SetBackgroundColor(Color::LIGHT_RED);
		m_exit->SetImage( Resources::GExitIcon );
		m_exit->SetMargin(Box2(1,0,1,0));
		m_exit->SetPadding(Box2(2,2,2,2));
		m_exit->SetBackgroundColor(Color(175, 175, 175));
		m_exit->EventButtonClicked.Bind(this, &CRedGuiWindow::NotifyButtonClicked);
		m_captionPanel->AddChild(m_exit);
		m_exit->SetDock(DOCK_Right);

		m_maximize = new CRedGuiButton(GCaptionButtonWidth, GButtonOffset, GCaptionButtonWidth, GCaptionButtonWidth);
		m_maximize->EventButtonClicked.Bind(this, &CRedGuiWindow::NotifyButtonClicked);
		m_maximize->SetMargin(Box2(1,0,1,0));
		m_maximize->SetPadding(Box2(2,2,2,2));
		m_maximize->SetBackgroundColor(Color(175, 175, 175));
		m_maximize->SetImage( Resources::GMaximizeIcon );
		m_captionPanel->AddChild(m_maximize);
		m_maximize->SetDock(DOCK_Right);

		m_minimize = new CRedGuiButton(GButtonOffset, GButtonOffset, GCaptionButtonWidth, GCaptionButtonWidth);	 
		m_minimize->EventButtonClicked.Bind(this, &CRedGuiWindow::NotifyButtonClicked);
		m_minimize->SetMargin(Box2(1,0,1,0));
		m_minimize->SetPadding(Box2(2,2,2,2));
		m_minimize->SetBackgroundColor(Color(175, 175, 175));
		m_minimize->SetImage( Resources::GCollapseIcon );
		m_captionPanel->AddChild(m_minimize);
		m_minimize->SetDock(DOCK_Right);

		m_help = new CRedGuiButton(GButtonOffset, GButtonOffset, GCaptionButtonWidth, GCaptionButtonWidth);	 
		m_help->EventButtonClicked.Bind(this, &CRedGuiWindow::NotifyButtonClicked);
		m_help->SetMargin(Box2(1,0,1,0));
		m_help->SetPadding(Box2(2,2,2,2));
		m_help->SetBackgroundColor(Color(175, 175, 175));
		m_help->SetImage( Resources::GHelpIcon );
		m_captionPanel->AddChild(m_help);
		m_help->SetDock(DOCK_Right);
		m_help->SetVisible(false);

		// create panel for menu bar
		m_menuBarPanel = new CRedGuiPanel(0, GCaptionHeight, GetWidth(), GCaptionHeight-2);
		m_menuBarPanel->SetVisible(false);
		m_menuBarPanel->SetPadding(Box2::IDENTITY);
		//this->AddChild(m_menuBarPanel);
		m_menuBarPanel->SetDock(DOCK_Top);

		// create client area
		m_client = new CRedGuiScrollPanel(0, 0, GetWidth(), GetHeight() - GCaptionHeight);
		m_client->EventMouseButtonPressed.Bind(this, &CRedGuiWindow::NotifyMousePressed);
		m_client->EventMouseDrag.Bind(this, &CRedGuiWindow::NotifyMouseDrag);
		SetControlClient(m_client);
		m_client->SetDock(DOCK_Fill);
		m_client->SetBackgroundColor( Color::CLEAR );
		m_client->EventKeyButtonPressed.Bind(this, &CRedGuiWindow::NotifyKeyButtonPressed);
	}

	CRedGuiWindow::~CRedGuiWindow()
	{
		
	}

	void CRedGuiWindow::OnPendingDestruction() 
	{
		m_leftBorder->EventMouseDrag.Unbind(this, &CRedGuiWindow::NotifyMouseBorderDrag);
		m_leftBorder->EventMouseButtonPressed.Unbind(this, &CRedGuiWindow::NotifyMousePressed);

		m_rightBorder->EventMouseDrag.Unbind(this, &CRedGuiWindow::NotifyMouseBorderDrag);
		m_rightBorder->EventMouseButtonPressed.Unbind(this, &CRedGuiWindow::NotifyMousePressed);

		m_bottomBorder->EventMouseDrag.Unbind(this, &CRedGuiWindow::NotifyMouseBorderDrag);
		m_bottomBorder->EventMouseButtonPressed.Unbind(this, &CRedGuiWindow::NotifyMousePressed);

		m_leftBottomCorner->EventMouseDrag.Unbind(this, &CRedGuiWindow::NotifyMouseBorderDrag);
		m_leftBottomCorner->EventMouseButtonPressed.Unbind(this, &CRedGuiWindow::NotifyMousePressed);

		m_rightBottomCorner->EventMouseDrag.Unbind(this, &CRedGuiWindow::NotifyMouseBorderDrag);
		m_rightBottomCorner->EventMouseButtonPressed.Unbind(this, &CRedGuiWindow::NotifyMousePressed);

		m_captionPanel->EventMouseButtonPressed.Unbind(this, &CRedGuiWindow::NotifyMousePressed);
		m_captionPanel->EventMouseDrag.Unbind(this, &CRedGuiWindow::NotifyMouseDrag);
		m_captionPanel->EventMouseButtonDoubleClick.Unbind( this, &CRedGuiWindow::NotifyMouseButtonDoubleClick );

		m_exit->EventButtonClicked.Unbind(this, &CRedGuiWindow::NotifyButtonClicked);
		m_maximize->EventButtonClicked.Unbind(this, &CRedGuiWindow::NotifyButtonClicked);
		m_minimize->EventButtonClicked.Unbind(this, &CRedGuiWindow::NotifyButtonClicked);
		m_help->EventButtonClicked.Unbind(this, &CRedGuiWindow::NotifyButtonClicked);

		m_client->EventMouseButtonPressed.Unbind(this, &CRedGuiWindow::NotifyMousePressed);
		m_client->EventMouseDrag.Unbind(this, &CRedGuiWindow::NotifyMouseDrag);
		m_client->EventKeyButtonPressed.Unbind(this, &CRedGuiWindow::NotifyKeyButtonPressed);
	}

	void CRedGuiWindow::SetCaption(const String& value)
	{
		m_buttonWithText->SetText(value);
	}

	String CRedGuiWindow::GetCaption()
	{
		return m_buttonWithText->GetText();
	}

	void CRedGuiWindow::SetSize( const Vector2& value)
	{
		Vector2 newSize = value;
		CheckMinMaxSize( newSize );

		CRedGuiControl::SetSize(newSize);

		// change client area
		if(m_menuBar != nullptr)
		{
			GetClientControl()->SetPosition(Vector2(0.0f,m_captionPanel->GetSize().Y + m_menuBarPanel->GetSize().Y));
			GetClientControl()->SetSize(Vector2(GetSize().X, GetSize().Y - (m_captionPanel->GetSize().Y + m_menuBarPanel->GetSize().Y)));
		}
		else
		{
			GetClientControl()->SetPosition(Vector2(0.0f,m_captionPanel->GetSize().Y));
			GetClientControl()->SetSize(Vector2(GetSize().X, GetSize().Y-m_captionPanel->GetSize().Y));
		}
	}

	void CRedGuiWindow::SetMovable(Bool value)
	{
		m_movable = value;
	}

	Bool CRedGuiWindow::GetMovable() const
	{
		return m_movable;
	}

	void CRedGuiWindow::SetResizable(Bool value)
	{
		m_resizable = value;
	}

	Bool CRedGuiWindow::GetResizable() const
	{
		return m_resizable;
	}

	void CRedGuiWindow::SetEnabledCaptionButton(enum ECaptionButton button, Bool value)
	{
		switch(button)
		{
		case CB_Minimize:
			return m_minimize->SetEnabled(value);

		case CB_Maximize:
			return m_maximize->SetEnabled(value);

		case CB_Exit:
			return m_exit->SetEnabled(value);
		}
	}

	Bool CRedGuiWindow::GetEnabledCaptionButton(enum ECaptionButton button) const
	{
		switch(button)
		{
		case CB_Minimize:
			return m_minimize->GetEnabled();

		case CB_Maximize:
			return m_maximize->GetEnabled();

		case CB_Exit:
			return m_exit->GetEnabled();
		}

		return false;
	}

	void CRedGuiWindow::SetMenuBar(CRedGuiMenuBar* menuBar)
	{
		if(m_menuBar != nullptr)
		{
			m_menuBarPanel->SetVisible(false);
			m_menuBarPanel->RemoveChild(menuBar);

			// move client area
			m_client->SetSize(Vector2(m_client->GetSize().X, GetSize().Y - m_captionPanel->GetSize().Y ));
		}

		if(menuBar != nullptr)
		{
			m_menuBar = menuBar;
			m_menuBar->SetAnchor(ANCHOR_Stretch);
			m_menuBarPanel->SetVisible(true);
			m_menuBarPanel->AddChild(menuBar);

			// move client area
			m_client->SetSize(Vector2(m_client->GetSize().X, GetSize().Y - (m_captionPanel->GetSize().Y + m_menuBarPanel->GetSize().Y) ));
		}
	}

	CRedGuiMenuBar* CRedGuiWindow::GetMenuBar() const
	{
		return m_menuBar;
	}

	void CRedGuiWindow::Draw()
	{
		GetTheme()->DrawPanel( this );
	}

	void CRedGuiWindow::OnMouseLostFocus(CRedGuiControl* newControl)
	{
		m_internalKeyFocus = nullptr;
	}

	void CRedGuiWindow::NotifyMousePressed( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button)
	{
		RED_UNUSED( eventPackage );

		m_PreMovePosition = GetPosition();
		m_PreMoveSize = GetSize();
	}

	void CRedGuiWindow::NotifyButtonClicked( RedGui::CRedGuiEventPackage& eventPackage )
	{
		CRedGuiControl* sender = eventPackage.GetEventSender();

		if(sender == m_exit)
		{
			SetVisible(false);
		}
		else if(sender == m_maximize)
		{
			Maximize();

		}
		else if(sender == m_minimize)
		{
			Minimize();
		}
		else if(sender == m_help)
		{
			if(m_helpControl != nullptr)
			{
				m_helpControl->SetVisible(true);
			}
		}
	}

	void CRedGuiWindow::NotifyMouseDrag( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button)
	{
		CRedGuiControl* sender = eventPackage.GetEventSender();

		if(button != MB_Left)
		{
			return;
		}

		if(sender == m_captionPanel)
		{
			const Vector2& point = GRedGui::GetInstance().GetInputManager()->GetLastPressedPosition(MB_Left);

			if(m_isMaximize == true)
			{
				Maximize();
				SetPosition(Vector2(mousePosition.X - GetWidth()/2.0f, point.Y));
				m_PreMovePosition = GetPosition();
			}			

			if(m_movable == true)
			{
				Vector2 delta = mousePosition - point;
				SetPosition(m_PreMovePosition+delta);
			}
		}		

		 EventWindowChangeCoord(this);
	}

	void CRedGuiWindow::NotifyMouseBorderDrag( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button )
	{
		CRedGuiControl* sender = eventPackage.GetEventSender();

		if(button != MB_Left || m_resizable == false)
		{
			return;
		}

		const Vector2& point = GRedGui::GetInstance().GetInputManager()->GetLastPressedPosition( MB_Left );
		Vector2 delta = mousePosition - point;

		if( sender == m_leftBorder )
		{
			const Vector2 oldSize = GetSize();
			SetSize( Vector2( m_PreMoveSize.X - delta.X, m_PreMoveSize.Y ) );
			const Float moveDelta = oldSize.X - GetSize().X;
			SetPosition( Vector2( m_PreMovePosition.X + moveDelta, m_PreMovePosition.Y ) );	
		}		
		else if( sender == m_rightBorder )
		{
			SetSize( Vector2( m_PreMoveSize.X + delta.X, m_PreMoveSize.Y ) );
		}
		else if( sender == m_bottomBorder )
		{
			SetSize( Vector2( m_PreMoveSize.X, m_PreMoveSize.Y + delta.Y ) );
		}
		else if( sender == m_leftBottomCorner )
		{
			SetSize( Vector2( m_PreMoveSize.X - delta.X, m_PreMoveSize.Y + delta.Y ) );
			SetPosition( Vector2( m_PreMovePosition.X + delta.X, m_PreMovePosition.Y ) );
		}
		else if( sender == m_rightBottomCorner )
		{
			SetSize( Vector2( m_PreMoveSize.X + delta.X, m_PreMoveSize.Y + delta.Y ) );
		}
	}

	void CRedGuiWindow::NotifyMouseButtonDoubleClick( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button )
	{
		RED_UNUSED( eventPackage );

		Maximize();
	}

	void CRedGuiWindow::SetVisible( Bool value )
	{
		CRedGuiControl::SetVisible(value);

		if(GetVisible() == true)
		{
			UpLayerItem(false);
			MoveToTop();
			PropagateWindowOpened();
			SetGamepadLevel( true );
		}
		else
		{
			PropagateWindowClosed();
		}
	}

	void CRedGuiWindow::PropagateWindowClosed()
	{
		OnWindowClosed(this);
		EventWindowClosed(this);
	}

	void CRedGuiWindow::PropagateWindowOpened()
	{
		// centering window
		Vector2 viewSize = GRedGui::GetInstance().GetRenderManager()->GetViewSize();
		Vector2 mySize = GetSize();
		SetPosition(Vector2((viewSize.X - mySize.X)/2, (viewSize.Y - mySize.Y)/2));

		OnWindowOpened(this);
		EventWindowOpened(this);

		GRedGui::GetInstance().GetInputManager()->SetKeyFocusControl( this );
	}

	void CRedGuiWindow::Maximize()
	{
		if(m_resizable == true)
		{
			if(m_isMaximize == true)
			{
				if( m_isMinimize == true )
				{
					Minimize();
				}

				// show borders
				m_leftBorder->SetVisible(true);
				m_rightBorder->SetVisible(true);
				m_bottomBorder->SetVisible(true);

				m_maximize->SetImage( Resources::GMaximizeIcon );
				SetDock(DOCK_None);
				SetCoord(GetOriginalRect());
				m_isMaximize = !m_isMaximize;
			}
			else
			{
				if( m_isMinimize == true )
				{
					Minimize();
				}

				m_maximize->SetImage( Resources::GMinimizeIcon );
				SetOriginalRect(GetCoord());

				// hide borders
				m_leftBorder->SetVisible(false);
				m_rightBorder->SetVisible(false);
				m_bottomBorder->SetVisible(false);

				MoveToTop();
				SetDock(DOCK_Fill);	// !!! FIXME !!!
				Vector2 viewSize = GRedGui::GetInstance().GetRenderManager()->GetViewSize();
				SetPosition(Vector2(0.0f,18.0f));
				SetSize(Vector2( viewSize.X, viewSize.Y-18.0f));
				m_isMaximize = !m_isMaximize;
			}
		}
	}

	void CRedGuiWindow::Minimize()
	{
		if( m_isMinimize == true )
		{
			m_minimize->SetImage( Resources::GCollapseIcon );
			CRedGuiCroppedRect::SetCoord( Box2( GetPosition(), m_minPreviousCoord.Max ));
			CRedGuiCroppedRect::SetOriginalRect( GetCoord() );
			m_isMinimize = !m_isMinimize;

			// show borders
			m_leftBorder->SetVisible(true);
			m_rightBorder->SetVisible(true);
			m_bottomBorder->SetVisible(true);
		}
		else
		{
			m_minimize->SetImage( Resources::GExpandIcon );
			m_minPreviousCoord = GetCoord();
			CRedGuiCroppedRect::SetSize( Vector2( (Float)GetWidth(), (Float)GetTitleBarHeight() ) );
			CRedGuiCroppedRect::SetOriginalRect( GetCoord() );
			MoveToTop();
			m_isMinimize = !m_isMinimize;

			// hide borders
			m_leftBorder->SetVisible(false);
			m_rightBorder->SetVisible(false);
			m_bottomBorder->SetVisible(false);
		}
	}

	Bool CRedGuiWindow::IsMinimized() const
	{
		return m_isMinimize;
	}

	Bool CRedGuiWindow::IsMaximized() const
	{
		return m_isMaximize;
	}

	Uint32 CRedGuiWindow::GetTitleBarHeight() const
	{
		return (Uint32)m_captionPanel->GetHeight();
	}

	void CRedGuiWindow::SetPadding( const Box2& padding )
	{
		if(m_client != nullptr)
		{
			m_client->SetPadding(padding);
		}
	}

	void CRedGuiWindow::SetVisibleCaptionButton( enum ECaptionButton button, Bool value )
	{
		switch(button)
		{
		case CB_Help:
			return m_help->SetVisible(value);

		case CB_Minimize:
			return m_minimize->SetVisible(value);

		case CB_Maximize:
			return m_maximize->SetVisible(value);

		case CB_Exit:
			return m_exit->SetVisible(value);
		}
	}

	Bool CRedGuiWindow::GetVisibleCaptionButton( enum ECaptionButton button ) const
	{
		switch(button)
		{
		case CB_Help:
			return m_help->GetVisible();

		case CB_Minimize:
			return m_minimize->GetVisible();

		case CB_Maximize:
			return m_maximize->GetVisible();

		case CB_Exit:
			return m_exit->GetVisible();
		}
		return false;
	}

	void CRedGuiWindow::NotifyKeyButtonPressed( RedGui::CRedGuiEventPackage& eventPackage, enum EInputKey key, Char text )
	{
		RED_UNUSED( eventPackage );

		if(key == IK_Tab)
		{
			GRedGui::GetInstance().GetInputManager()->ResetKeyFocusControl();
			
			//
			Uint32 index = 0;
			if(m_internalKeyFocus != nullptr)
			{
				for(Uint32 i=0; i<GetClientControl()->GetChildCount(); ++i)
				{
					if(GetClientControl()->GetChildAt(i) == m_internalKeyFocus)
					{
						index = i;
						break;
					}
				}
			}

			if(GRedGui::GetInstance().GetInputManager()->IsShiftPressed() == false)
			{
				for(Uint32 i=index; i<GetClientControl()->GetChildCount(); ++i)
				{
					if(GetClientControl()->GetChildAt(i)->GetNeedKeyFocus() == true)
					{
						m_internalKeyFocus = GetClientControl()->GetChildAt(i);
						break;
					}
				}
			}
			else
			{
				for(Int32 i=index; i>=0; --i)
				{
					if(GetClientControl()->GetChildAt(i)->GetNeedKeyFocus() == true)
					{
						m_internalKeyFocus = GetClientControl()->GetChildAt(i);
						break;
					}
				}
			}

			GRedGui::GetInstance().GetInputManager()->SetKeyFocusControl(m_internalKeyFocus);
		}
		if(key == IK_Alt)
		{
			if(m_menuBar != nullptr)
			{
				if(GRedGui::GetInstance().GetInputManager()->GetKeyFocusControl() != m_menuBar)
				{
					GRedGui::GetInstance().GetInputManager()->ResetKeyFocusControl();
					GRedGui::GetInstance().GetInputManager()->SetKeyFocusControl(m_menuBar);
				}
				else
				{
					GRedGui::GetInstance().GetInputManager()->ResetKeyFocusControl();
				}
			}
		}
		if(key == IK_Escape)
		{
			if(GRedGui::GetInstance().GetInputManager()->GetKeyFocusControl() == m_menuBar)
			{
				GRedGui::GetInstance().GetInputManager()->ResetKeyFocusControl();
			}
		}
	}

	void CRedGuiWindow::SetHelpControl( CRedGuiControl* helpControl )
	{
		if(m_helpControl != nullptr)
		{
			RemoveChild(m_helpControl);
			m_helpControl->Dispose();
		}

		m_helpControl = helpControl;
	}

	void CRedGuiWindow::SetTitleBarVisible( Bool value )
	{
		m_captionPanel->SetVisible( value );
	}

	Bool CRedGuiWindow::GetTitleBarVisible() const
	{
		return m_captionPanel->GetVisible();
	}

	Bool CRedGuiWindow::IsAWindow()
	{
		return true;
	}

	void CRedGuiWindow::CheckMinMaxSize( Vector2& size )
	{
		// check minimum size
		if(m_minMaxSize.Min.X >= 0.0 && m_minMaxSize.Min.Y >= 0.0)
		{
			size.X = Max<Float>(size.X, m_minMaxSize.Min.X);
			size.Y = Max<Float>(size.Y, m_minMaxSize.Min.Y);
		}

		// check maximum size
		if(m_minMaxSize.Max.X >= 0.0 && m_minMaxSize.Max.Y >= 0.0)
		{
			size.X = Min<Float>(size.X, m_minMaxSize.Max.X);
			size.Y = Min<Float>(size.Y, m_minMaxSize.Max.Y);
		}
	}

	void CRedGuiWindow::SetPosition( const Vector2& position )
	{
		Vector2 newPosition = position;

		newPosition.X = Max< Float >( 0, newPosition.X );
		newPosition.Y = Max< Float >( 0, newPosition.Y );

		CRedGuiControl* parent = GetParent();
		if( parent != nullptr )
		{
			newPosition.X = Min< Float >( Max< Float >( 10.f, parent->GetSize().X - GetSize().X ), newPosition.X );
			newPosition.Y = Min< Float >( Max< Float >( 10.f, parent->GetSize().Y - GetSize().Y ), newPosition.Y );
		}

		CRedGuiControl::SetPosition( newPosition );
	}

	void CRedGuiWindow::SetPosition( Int32 left, Int32 top )
	{
		SetPosition(Vector2((Float)left, (Float)top));
	}

	void CRedGuiWindow::SetCoord( const Box2& coord )
	{
		SetPosition(coord.Min);
		SetSize(coord.Max);
	}

	void CRedGuiWindow::SetCoord( Int32 left, Int32 top, Int32 width, Int32 height )
	{
		CRedGuiControl::SetCoord(Box2((Float)left, (Float)top, (Float)width, (Float)height));
	}

	void CRedGuiWindow::OnKeyChangeRootFocus( Bool focus )
	{
		if( focus == true )
		{
			m_captionPanel->SetBackgroundColor( Color::DARK_BLUE );
		}
		else
		{
			m_captionPanel->SetBackgroundColor( Color::LIGHT_BLUE );
			m_canResizeByPad = false;
			m_canMoveByPad = false;
		}
	}

	void CRedGuiWindow::AddChild( CRedGuiControl* child )
	{
		CRedGuiControl::AddChild( child );
	}

	void CRedGuiWindow::RemoveChild( CRedGuiControl* child )
	{
		CRedGuiControl::RemoveChild( child );
	}

	Bool CRedGuiWindow::OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data )
	{
		if( CRedGuiControl::OnInternalInputEvent( event, data ) == true )
		{
			return true;
		}

		if( event == RGIE_MoveWindow )
		{
			if( m_movable == true )
			{
				this->SetPosition( GetPosition() + data );
				return true;
			}
		}
		else if( event == RGIE_ResizeWindow )
		{
			if( m_resizable == true )
			{
				this->SetSize( GetSize() + data );
				return true;
			}
		}
		else if( event == RGIE_Move )
		{
			if( m_client != nullptr )
			{
				CRedGuiScrollPanel* scrollPanel = static_cast< CRedGuiScrollPanel* >( m_client );
				scrollPanel->Move( data );
			}
		}

		return false;
	}

}	// namespace RedGui


#endif	// NO_RED_GUI
