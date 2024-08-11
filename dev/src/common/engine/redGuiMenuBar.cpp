/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiMenu.h"
#include "redGuiManager.h"
#include "redGuiLayer.h"
#include "redGuiButton.h"
#include "redGuiMenuBar.h"
#include "inputKeys.h"

namespace RedGui
{
	namespace
	{
		const Uint32 GDefaultHeight = 18;
		const Uint32 GDefaultFreeSpace = 5;
		const Color GHighlight( 255,170, 0, 100 );
	}

	CRedGuiMenuBar::CRedGuiMenuBar()
		: CRedGuiControl( 0, 0, 0, GDefaultHeight )
		, m_openedMenu(nullptr)
		, m_activeMenuIndex( -1 )
	{
		SetBackgroundColor( Color::CLEAR );
		SetBorderVisible( false );
		AttachToLayer( TXT("Menus") );
		SetNeedKeyFocus( true );
	}

	CRedGuiMenuBar::~CRedGuiMenuBar()
	{
		/*intentionally empty*/
	}

	void CRedGuiMenuBar::Draw()
	{
		GetTheme()->DrawPanel( this );

		GetTheme()->SetCroppedParent( this );

		Uint32 tempWidth = GDefaultFreeSpace;
		const Uint32 menuCount = m_menus.Size();
		for( Uint32 i=0; i<menuCount; ++i )
		{
			Vector2 position( GetAbsolutePosition() + Vector2( (Float)tempWidth - GDefaultFreeSpace, 0.0f ) );
			if( m_activeMenuIndex == (Int32)i )
			{
				GetTheme()->DrawRawFilledRectangle( position, Vector2( (Float)m_menus[i].m_menuWidth, (Float)GDefaultHeight ) , GHighlight );
			}
			position = Vector2( GetAbsolutePosition() + Vector2( (Float)tempWidth, 4.0f ) );
			GetTheme()->DrawRawText( position , m_menus[i].m_menuText, Color::WHITE );
			tempWidth += m_menus[i].m_menuWidth;
		}

		GetTheme()->ResetCroppedParent();
	}

	Int32 CRedGuiMenuBar::CheckPoint( const Vector2& position )
	{
		const Vector2 relativePosition = position - GetAbsolutePosition();
		if( relativePosition.X > 0.0 && relativePosition.Y > 0.0 )
		{
			const Uint32 posX = (Uint32)relativePosition.X;
			Uint32 tempWidth = 0;

			const Uint32 menuCount = m_menus.Size();
			for( Uint32 i=0; i<menuCount; ++i )
			{
				if( posX > tempWidth && posX < ( tempWidth + m_menus[i].m_menuWidth ) )
				{
					return i;
				}
				tempWidth += m_menus[i].m_menuWidth;
			}
		}

		if( m_menuIsOpened == true )
		{
			return m_activeMenuIndex;
		}

		return -1;
	}

	void CRedGuiMenuBar::OnMouseMove( const Vector2& mousePosition )
	{
		m_activeMenuIndex = CheckPoint( mousePosition );

		if( m_menuIsOpened == true )
		{
			UpdateOpened();
		}
	}

	CRedGuiMenu* CRedGuiMenuBar::AppendMenu( CRedGuiMenu* menu, const String& name )
	{
		String properName = name.ToUpper();
		Uint32 textWidth = (Uint32)( GRedGui::GetInstance().GetFontManager()->GetStringSize( properName, RGFT_Default ).X );

		// create new menu
		if( menu == nullptr )
		{
			menu = new CRedGuiMenu( 0, 0, textWidth, 0 );
		}

		// add menu to menu bar
		menu->SetVisible( false );
		menu->AttachToMenuBar( this );
		menu->SetNeedKeyFocus( false );
		menu->EventVisibleChanged.Bind( this, &CRedGuiMenuBar::NotifyEventVisibleChanged );
		AddChild( menu );
		menu->SetCroppedParent( nullptr );

		SMenu newMenu( menu );
		newMenu.m_menuText = properName;
		newMenu.m_menuWidth = textWidth;
		newMenu.m_menuWidth += ( 2 * GDefaultFreeSpace );
		m_menus.PushBack( newMenu );
		
		return menu;
	}

	CRedGuiMenu* CRedGuiMenuBar::AddNewMenu( const String& name )
	{
		return AppendMenu( nullptr, name );
	}

	CRedGuiMenu* CRedGuiMenuBar::GetMenu(const String& name)
	{
		String properName = name.ToUpper();
		TDynArray< SMenu, MC_RedGuiControls, MemoryPool_RedGui >::iterator end = m_menus.End();
		for( TDynArray< SMenu, MC_RedGuiControls, MemoryPool_RedGui >::iterator iter = m_menus.Begin(); iter != end; ++iter )
		{
			if( iter->m_menuText == properName )
			{
				return iter->m_menu;
			}
		}
		return NULL;
	}

	void CRedGuiMenuBar::NotifyEventVisibleChanged( CRedGuiEventPackage& eventPackage, Bool value )
	{
		// menu is invisible
		if( value == false )
		{
			CloseMenu();
		}
	}

	void CRedGuiMenuBar::OnKeyButtonPressed( enum EInputKey key, Char text )
	{
		if( m_openedMenu != nullptr )
		{
			if( m_openedMenu->ProcessInput( key ) == true )
			{
				return;
			}
		}

		if( key == IK_Left || key == IK_Pad_DigitLeft )
		{
			--m_activeMenuIndex;
			if( m_activeMenuIndex < 0 )
			{
				m_activeMenuIndex = (Int32)( m_menus.Size() - 1 );
			}
			if( m_menuIsOpened == true )
			{
				UpdateOpened();
			}
		}
		else if(key == IK_Right || key == IK_Pad_DigitRight )
		{
			++m_activeMenuIndex;
			if( m_activeMenuIndex == (Int32)m_menus.Size() )
			{
				m_activeMenuIndex = 0;
			}
			if( m_menuIsOpened == true )
			{
				UpdateOpened();
			}
		}
		else if( key == IK_Down || key == IK_Enter || key == IK_Pad_DigitDown || key == IK_Up || key == IK_Pad_DigitUp || key == IK_Pad_A_CROSS )
		{
			OpenMenu();
		}
		else if( key == IK_Pad_B_CIRCLE || key == IK_Escape )
		{
			CloseMenu();
		}
	}

	void CRedGuiMenuBar::OnKeyChangeRootFocus( Bool focus )
	{
		if( focus == false )
		{
			if( m_openedMenu != nullptr )
			{
				m_openedMenu->SetVisible( false );
				m_openedMenu = nullptr;
			}
			m_activeMenuIndex = -1;
			m_menuIsOpened = false;
		}
		else
		{
			if( m_activeMenuIndex == -1 )
			{
				m_activeMenuIndex = ( m_menus.Size() > 0 ) ? 0 : -1;
			}
		}
	}

	void CRedGuiMenuBar::Toggle()
	{
		if( m_openedMenu != nullptr )
		{
			CloseMenu();
		}
		else
		{
			OpenMenu();
		}
	}

	void CRedGuiMenuBar::OnMouseLostFocus( CRedGuiControl* newControl )
	{
		if( m_menuIsOpened == false )
		{
			m_activeMenuIndex = -1;
		}
	}

	void CRedGuiMenuBar::OpenMenu()
	{
		if( m_activeMenuIndex != -1 )
		{
			m_openedMenu = m_menus[m_activeMenuIndex].m_menu;

			Uint32 tempWidth = 0;
			Vector2 position( 0.0f, (Float)( GetAbsoluteTop() + GDefaultHeight ) );
			const Uint32 menuCount = m_menus.Size();
			for( Uint32 i=0; i<menuCount; ++i )
			{
				if( m_activeMenuIndex == (Int32)i )
				{
					break;
				}
				position.X += m_menus[i].m_menuWidth;
			}

			m_openedMenu->SetPosition( position );
			m_openedMenu->SetVisible( true );
			m_openedMenu->UpLayerItem();
			m_menuIsOpened = true;
		}
	}

	void CRedGuiMenuBar::CloseMenu()
	{
		if( m_openedMenu != nullptr )
		{
			m_openedMenu->SetVisible( false );
			m_openedMenu = nullptr;
			m_menuIsOpened = false;
		}
	}

	void CRedGuiMenuBar::OnMouseButtonClick( const Vector2& mousePosition, enum EMouseButton button )
	{
		Toggle();
	}

	void CRedGuiMenuBar::UpdateOpened()
	{
		CloseMenu();
		OpenMenu();
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
