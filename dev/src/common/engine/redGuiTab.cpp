/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiPanel.h"
#include "redGuiButton.h"
#include "redGuiScrollPanel.h"
#include "redGuiTab.h"

namespace
{
	const Uint32 GDefaultButtonHeight = 20;
}

namespace RedGui
{
	CRedGuiTab::CRedGuiTab( Uint32 left, Uint32 top, Uint32 width, Uint32 height )
		: CRedGuiControl( left, top, width, height )
		, m_panelForActiveTab( nullptr )
		, m_activeTab( nullptr )
		, m_panelForTabs( nullptr )
	{
		SetBackgroundColor( Color::CLEAR );
		SetBorderVisible( false );
		SetNeedKeyFocus( true );

		// create area for tab's buttons
		m_panelForTabs = new CRedGuiPanel( 0, 0, GetWidth(), GDefaultButtonHeight );
		m_panelForTabs->SetBorderVisible( false );
		m_panelForTabs->SetBackgroundColor( Color::CLEAR );
		m_panelForTabs->SetDock( DOCK_Top );
		m_panelForTabs->SetAutoSize( true );
		AddChild( m_panelForTabs );

		// create area for active tab
		m_panelForActiveTab = new CRedGuiPanel( 0, GDefaultButtonHeight, GetWidth(), GetHeight() - GDefaultButtonHeight );
		m_panelForActiveTab->SetPadding( Box2::IDENTITY );
		m_panelForActiveTab->SetDock( DOCK_Fill );
		AddChild( m_panelForActiveTab );
	}

	CRedGuiTab::~CRedGuiTab()
	{
		/* intentionally empty */
	}

	Uint32 CRedGuiTab::AddTab( const String& name )
	{
		const Int32 panWidth = Clamp( GetWidth(), 0, INT_MAX );
		const Int32 panHeight = Clamp( GetHeight() - (Int32)GDefaultButtonHeight, 0, INT_MAX );
		CRedGuiScrollPanel* newPanel = new CRedGuiScrollPanel( 0, GDefaultButtonHeight, panWidth, panHeight );
		newPanel->SetVisible( false );
		(*m_panelForActiveTab).AddChild( newPanel );
		newPanel->SetDock( DOCK_Fill );
		m_tabs.PushBack( newPanel );

		CRedGuiButton* newButton = new CRedGuiButton( 0, 0, 25, GDefaultButtonHeight );
		newButton->SetFitToText( true );
		newButton->SetText( name );
		newButton->SetToggleMode( true );
		newButton->SetUserData( newPanel );
		newButton->SetNeedKeyFocus( false );
		newButton->EventCheckedChanged.Bind( this, &CRedGuiTab::NotifyEventCheckedChanged );
		m_tabsButtons.PushBack( newButton );
		newButton->SetDock( DOCK_Left );

		newPanel->SetUserData(newButton);

		return m_tabs.Size() - 1;
	}

	CRedGuiScrollPanel* CRedGuiTab::GetTabAt( Uint32 index )
	{
		if( index < m_tabs.Size() )
		{
			return static_cast< CRedGuiScrollPanel* >( m_tabs[index] );
		}

		return nullptr;
	}


	CRedGuiScrollPanel* CRedGuiTab::GetTabByName( const String& tabName )
	{
		for( Uint32 i=0; i<m_tabsButtons.Size(); ++i )
		{
			if( m_tabsButtons[i]->GetText() == tabName )
			{
				return GetTabAt( i );
			}
		}
		return nullptr;
	}


	Uint32 CRedGuiTab::GetTabCount() const
	{
		return m_tabs.Size();
	}

	void CRedGuiTab::NotifyEventCheckedChanged( CRedGuiEventPackage& eventPackage, Bool value )
	{
		CRedGuiControl* sender = eventPackage.GetEventSender();

		if( value == true )
		{
			if( m_activeTab != nullptr )
			{
				m_activeTab->SetVisible( false );
				CRedGuiButton* button = m_activeTab->GetUserData< CRedGuiButton >();
				button->SetToggleValue( false, true );
			}

			m_activeTab = sender->GetUserData< CRedGuiScrollPanel >();
			if( m_activeTab != nullptr )
			{
				m_activeTab->SetVisible( true );
				EventTabChanged( this, m_activeTab );
			}
		}
		else
		{
			if( m_activeTab != nullptr )
			{
				CRedGuiButton* button = m_activeTab->GetUserData< CRedGuiButton >();
				button->SetToggleValue( true, true );
				return;
			}
		}
	}

	void CRedGuiTab::Draw()
	{
		GetTheme()->DrawPanel( this );
	}

	void CRedGuiTab::SetActiveTab( Uint32 value )
	{
		if( value < m_tabs.Size() )
		{
			m_tabsButtons[value]->SetToggleValue( true );
		}
	}

	Int32 CRedGuiTab::GetActiveTabIndex() const
	{
		if( m_activeTab == nullptr )
		{
			return -1;
		}

		for( Uint32 i=0; i<m_tabs.Size(); ++i )
		{
			if( m_activeTab == m_tabs[i] )
			{
				return i;
			}
		}

		return -1;
	}

	CRedGuiScrollPanel* CRedGuiTab::GetActiveTab() const
	{
		return static_cast< CRedGuiScrollPanel* >( m_activeTab );
	}

	void CRedGuiTab::RecalculateTabsPosition()
	{
		// calculate size
		Uint32 tabsWidth = 0;
		Uint32 startTabForRow = 0;
		TDynArray< Uint32 > tabCountInRow;

		const Uint32 tabCount = m_tabsButtons.Size();
		const Uint32 maxWidth = (Uint32)m_panelForTabs->GetSize().X;

		for( Uint32 i=0; i<tabCount; ++i )
		{
			tabsWidth += (Uint32)m_tabsButtons[i]->GetSize().X;
			if( tabsWidth >= maxWidth )
			{
				tabCountInRow.PushBack( i - startTabForRow );
				startTabForRow = i;
				tabsWidth = (Uint32)m_tabsButtons[i]->GetSize().X;
			}
		}
		if( tabsWidth != 0 )
		{
			tabCountInRow.PushBack( tabCount - startTabForRow );
		}

		// check whether you need to change anything
		if( tabCountInRow.Size() != m_rowsWithTabs.Size() )
		{
			// remove old rows
			const Uint32 oldRowCount = m_rowsWithTabs.Size();
			for( Uint32 i=0; i<oldRowCount; ++i )
			{
				CRedGuiPanel* panel = m_rowsWithTabs[i];
				const TDynArray< CRedGuiButton*, MC_RedGuiControls, MemoryPool_RedGui >& tabButtons = m_mapRowsTabButtons.GetRef( panel );

				// remove tabs from row
				for( Uint32 j=0; j<tabButtons.Size(); ++j )
				{
					panel->RemoveChild( tabButtons[j] );
				}

				// delete row
				m_panelForTabs->RemoveChild( panel );
				static Uint32 counter = 0;
				++counter;
				panel->Dispose();
			}
			m_rowsWithTabs.Clear();
			m_mapRowsTabButtons.Clear();

		// create new rows
		const Uint32 rowCount = tabCountInRow.Size();
		Uint32 currentTabIndex = 0;
		for( Uint32 i=0; i<rowCount; ++i )
		{
			// create area for active tab
			CRedGuiPanel* panel = new CRedGuiPanel( 0, i*GDefaultButtonHeight, (Uint32)GetSize().X, GDefaultButtonHeight );
			panel->SetBackgroundColor( Color::CLEAR );
			panel->SetBorderVisible( false );
			panel->SetDock( DOCK_Top );
			m_rowsWithTabs.PushBack( panel );
			m_panelForTabs->AddChild( panel );

			TDynArray< CRedGuiButton*, MC_RedGuiControls, MemoryPool_RedGui >& tabButtons = m_mapRowsTabButtons.GetRef( panel );

			// add tabs to row
			for( Uint32 j=0; j<tabCountInRow[i]; ++j )
			{
				panel->AddChild( m_tabsButtons[currentTabIndex] );
				tabButtons.PushBack( m_tabsButtons[currentTabIndex] );
				++currentTabIndex;
			}
		}
	}
	}

	void CRedGuiTab::OnSizeChanged( const Vector2& oldSize, const Vector2& newSize )
	{
		RecalculateTabsPosition();
	}

	Bool CRedGuiTab::OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data )
	{
		if( CRedGuiControl::OnInternalInputEvent( event, data ) == true )
		{
			return true;
		}

		if( event == RGIE_Left )
		{
			Int32 activeIndex = GetActiveTabIndex();
			--activeIndex;
			if( activeIndex < 0 )
			{
				activeIndex = (Int32)( m_tabsButtons.Size() - 1 );
			}
			SetActiveTab( activeIndex );
			return true;
		}
		else if( event == RGIE_Right )
		{
			Int32 activeIndex = GetActiveTabIndex();
			++activeIndex;
			if( activeIndex == (Int32)m_tabsButtons.Size() )
			{
				activeIndex = 0;
			}
			SetActiveTab( activeIndex );
			return true;
		}
		else if( event == RGIE_Move )
		{
			if( m_activeTab != nullptr )
			{
				CRedGuiScrollPanel* scrollPanel = static_cast< CRedGuiScrollPanel* >( m_activeTab );
				scrollPanel->Move( data );
				return true;
			}
		}

		return false;
	}


} // namespace RedGui

#endif	// NO_RED_GUI
