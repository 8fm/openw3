/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiMenu.h"
#include "redGuiMenuBar.h"
#include "redGuiImage.h"
#include "redGuiMenuItem.h"
#include "redGuiManager.h"
#include "inputKeys.h"

namespace RedGui
{
	namespace
	{
		const Color GHighlight( 255,170, 0, 100 );
		const Uint32 GDefaultFreeSpace = 5;
		const Uint32 GArrowSize = 16;
	}

	CRedGuiMenu::CRedGuiMenu( Uint32 left, Uint32 top, Uint32 width, Uint32 height )
		: CRedGuiControl( left, top, width, height )
		, m_parentMenuBar( nullptr )
		, m_openedSubmenu( nullptr )
		, m_activeMenuIndex( -1 )
	{
		AttachToLayer( TXT("Menus") );

		SetBorderVisible( false );
		SetNeedKeyFocus( false );
		SetVisible( false );
		GRedGui::GetInstance().EventTick.Bind( this, &CRedGuiMenu::NotifyEventTick );

		m_arrowIcon = new CRedGuiImage( 0, 0, GArrowSize, GArrowSize );
		m_arrowIcon->SetImage( Resources::GRightArrowIcon );
	}

	CRedGuiMenu::~CRedGuiMenu()
	{
		GRedGui::GetInstance().EventTick.Unbind( this, &CRedGuiMenu::NotifyEventTick );
	}

	CRedGuiMenuItem* CRedGuiMenu::AppendItem( const String& item, RedGuiAny userData/* = nullptr*/ )
	{
		CRedGuiMenuItem* newMenuItem = new CRedGuiMenuItem( MENUITEM_Normal );
		newMenuItem->SetText(item);
		newMenuItem->SetUserData( userData );

		// check max size for all menu
		Uint32 maxWidth = (Uint32)Max< Float >( GetSize().X, newMenuItem->GetSize().X );
		Uint32 height = (Uint32)( GetSize().Y + newMenuItem->GetSize().Y );
		SetSize( maxWidth, height );

		// add to containers
		m_entries.PushBack( newMenuItem );

		return newMenuItem;
	}

	CRedGuiMenuItem* CRedGuiMenu::AppendSeparator()
	{
		CRedGuiMenuItem* newMenuItem = new CRedGuiMenuItem( MENUITEM_Separator );

		// check max size for all menu
		Uint32 maxWidth = (Uint32)Max< Float >( GetSize().X, newMenuItem->GetSize().X );
		Uint32 height = (Uint32)( GetSize().Y + newMenuItem->GetSize().Y );
		SetSize( maxWidth, height );

		// add to containers
		m_entries.PushBack( newMenuItem );

		return newMenuItem;
	}

	CRedGuiMenuItem* CRedGuiMenu::AppendSubMenuItem( const String& item, CRedGuiMenu* submenu, RedGuiAny userData /*= nullptr*/ )
	{
		CRedGuiMenuItem* newMenuItem = new CRedGuiMenuItem( MENUITEM_SubMenu );
		newMenuItem->SetSubmenu( submenu );
		newMenuItem->SetText( item );
		newMenuItem->SetUserData( userData );

		// check max size for all menu
		Uint32 maxWidth = (Uint32)Max< Float >( GetSize().X, newMenuItem->GetSize().X );
		Uint32 height = (Uint32)( GetSize().Y + newMenuItem->GetSize().Y );
		SetSize( maxWidth, height );

		// add to containers
		m_entries.PushBack( newMenuItem );

		return newMenuItem;
	}

	void CRedGuiMenu::SetVisible( Bool value )
	{
		if( value == false )
		{
			CloseSubmenu();
			m_activeMenuIndex = -1;
		}
		else
		{
			m_activeMenuIndex = 0;
		}

		CRedGuiControl::SetVisible(value);
	}

	void CRedGuiMenu::Draw() 
	{ 
		GetTheme()->DrawPanel( this );

		GetTheme()->SetCroppedParent( this );

		Uint32 tempHeight = 0;
		const Uint32 itemCount = m_entries.Size();
		for( Uint32 i=0; i<itemCount; ++i )
		{
			Vector2 position( GetAbsolutePosition() + Vector2( 0.0f, (Float)tempHeight ) );
			if( m_activeMenuIndex == (Int32)i &&  m_entries[i]->GetType() != MENUITEM_Separator )
			{
				GetTheme()->DrawRawFilledRectangle( position, Vector2( GetSize().X, m_entries[i]->GetSize().Y ), GHighlight );
			}
			GetTheme()->DrawRawText( position + Vector2( (Float) GDefaultFreeSpace, 5.0f ) , m_entries[i]->GetText(), Color::WHITE );

			if( m_entries[i]->GetType() == MENUITEM_SubMenu )
			{
				GetTheme()->DrawRawImage( position + Vector2( GetSize().X - (Float)GArrowSize, 2.0f ), Vector2( (Float)GArrowSize, (Float)GArrowSize ), m_arrowIcon->GetImage(), Color::WHITE );
			}
			else if( m_entries[i]->GetType() == MENUITEM_Separator )
			{
				GetTheme()->DrawRawLine( position + Vector2( (Float)GDefaultFreeSpace, m_entries[i]->GetSize().Y / 2.0f ), position + Vector2( GetSize().X - (Float)( 2*GDefaultFreeSpace), m_entries[i]->GetSize().Y / 2.0f ), Color::WHITE );
			}

			tempHeight += (Uint32)( m_entries[i]->GetSize().Y );
		}

		GetTheme()->ResetCroppedParent();
	}

	void CRedGuiMenu::AttachToMenuBar( CRedGuiMenuBar* parentMenuBar )
	{
		if( m_parentMenuBar != nullptr )
		{
			m_parentMenuBar->EventPositionChanged.Unbind( this, &CRedGuiMenu::NotifyParentPositionChanged );
			m_parentMenuBar = nullptr;
		}

		m_parentMenuBar = parentMenuBar;

		if( m_parentMenuBar != nullptr )
		{
			m_parentMenuBar->EventPositionChanged.Bind( this, &CRedGuiMenu::NotifyParentPositionChanged );
		}
	}

	void CRedGuiMenu::NotifyParentPositionChanged( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& oldPosition, const Vector2& newPosition)
	{
		RED_UNUSED( eventPackage );

		SetPosition( GetLeft() + (Int32)( newPosition.X - oldPosition.X ), GetTop() + (Int32)( newPosition.Y - oldPosition.Y ) );
	}

	void CRedGuiMenu::NotifyEventTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta )
	{
		if( GetVisible() == true )
		{
			if( m_timerOn == true )
			{
				m_timer += timeDelta;

				if( m_timer > 0.5 )
				{
					if( m_openedSubmenu != nullptr )
					{
						m_openedSubmenu->SetVisible( false );
					}

					if( m_activeMenuIndex != -1 )
					{
						OpenSubmenu();

					}

					m_timerOn = false;
				}
			}
		}
	}

	Bool CRedGuiMenu::ProcessInput( enum EInputKey key )
	{
		if( m_openedSubmenu != nullptr )
		{
			if( m_openedSubmenu->ProcessInput( key ) == true )
			{
				return true;
			}
		}

		if( key == IK_Down || key == IK_Pad_DigitDown )
		{
			if( m_entries.Empty() == false )
			{
				do
				{
					++m_activeMenuIndex;
					if( m_activeMenuIndex == (Int32)m_entries.Size() )
					{
						m_activeMenuIndex = 0;
					}

					if( m_entries[m_activeMenuIndex]->GetType() != MENUITEM_Separator )
					{
						break;
					}
				}while( m_activeMenuIndex != 0 );
			}

			return true;
		}
		else if( key == IK_Up || key == IK_Pad_DigitUp )
		{
			if( m_entries.Empty() == false )
			{
				do
				{
					--m_activeMenuIndex;
					if( m_activeMenuIndex < 0 )
					{
						m_activeMenuIndex = (Int32)( m_entries.Size() - 1 );
					}

					if( m_entries[m_activeMenuIndex]->GetType() != MENUITEM_Separator )
					{
						break;
					}
				}while( m_activeMenuIndex != 0 );
			}

			return true;
		}
		else if( key == IK_Space || key == IK_Enter || key == IK_Pad_A_CROSS )
		{
			if( m_entries.Empty() == true )
			{
				m_activeMenuIndex = -1;
			}
			if( m_activeMenuIndex != -1 )
			{
				EventMenuItemSelected( this, m_entries[m_activeMenuIndex] );
			}
			return true;
		}
		else if( key == IK_Right || key == IK_Pad_DigitRight )
		{
			if( m_entries.Empty() == true )
			{
				m_activeMenuIndex = -1;
			}
			if( m_activeMenuIndex != -1 )
			{
				if( m_entries[m_activeMenuIndex]->GetType() == MENUITEM_SubMenu )
				{
					OpenSubmenu();
					return true;
				}
			}
		}
		else if( key == IK_Left || key == IK_Pad_DigitLeft )
		{
			if( m_openedSubmenu != nullptr )
			{
				m_openedSubmenu->SetVisible( false );
				m_openedSubmenu = nullptr;
				return true;
			}
		}

		return false;
	}

	Int32 CRedGuiMenu::CheckPoint( const Vector2& position )
	{
		const Vector2 relativePosition = position - GetAbsolutePosition();
		if( relativePosition.X > 0.0 && relativePosition.Y > 0.0 )
		{
			const Uint32 posY = (Uint32)relativePosition.Y;
			Uint32 tempHeight = 0;

			const Uint32 itemCount = m_entries.Size();
			for( Uint32 i=0; i<itemCount; ++i )
			{
				if( posY > tempHeight && posY < ( tempHeight + (Uint32)( m_entries[i]->GetSize().Y ) ) )
				{
					return i;
				}
				tempHeight += (Uint32)( m_entries[i]->GetSize().Y );
			}
		}

		return -1;
	}

	void CRedGuiMenu::OnMouseMove( const Vector2& mousePosition )
	{
		Int32 oldActiveMenuIndex = m_activeMenuIndex;

		m_activeMenuIndex = CheckPoint( mousePosition );

		if( oldActiveMenuIndex != m_activeMenuIndex )
		{
			m_timer = 0.0f;
			CloseSubmenu();

			if( m_activeMenuIndex != -1 )
			{
				if( m_entries[m_activeMenuIndex]->GetType() == MENUITEM_SubMenu )
				{
					m_timerOn = true;
				}
			}
		}
	}

	void CRedGuiMenu::OnMouseButtonClick( const Vector2& mousePosition, enum EMouseButton button )
	{
		if( m_activeMenuIndex != -1 )
		{
			EventMenuItemSelected( this, m_entries[m_activeMenuIndex] );
		}
	}

	void CRedGuiMenu::OpenSubmenu()
	{
		if( m_entries[m_activeMenuIndex]->GetType() == MENUITEM_SubMenu )
		{
			Uint32 height = 0;
			for( Int32 i=0; i<m_activeMenuIndex; ++i )
			{
				height += (Uint32)( m_entries[i]->GetSize().Y );
			}
			Vector2 position( GetAbsolutePosition() + Vector2( GetSize().X, (Float)height ) );

			m_openedSubmenu = m_entries[m_activeMenuIndex]->GetSubMenu();
			m_openedSubmenu->SetPosition( position );
			m_openedSubmenu->SetVisible( true );
			m_openedSubmenu->UpLayerItem();
		}
	}

	void CRedGuiMenu::CloseSubmenu()
	{
		if( m_openedSubmenu != nullptr )
		{
			m_openedSubmenu->SetVisible( false );
			m_openedSubmenu = nullptr;
		}
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
