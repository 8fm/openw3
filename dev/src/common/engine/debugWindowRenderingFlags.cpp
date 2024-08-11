/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiPanel.h"
#include "redGuiScrollPanel.h"
#include "redGuiCheckBox.h"
#include "redGuiList.h"
#include "debugWindowRenderingFlags.h"
#include "viewport.h"
#include "game.h"

extern EShowFlags GShowGameFilter[];
extern EShowFlags GShowRenderFilter[];
extern EShowFlags GShowPostProcessFilter[];
extern EShowFlags GShowUmbraDebugFilter[];
extern EShowFlags GShowPhysicsDebugFilter[];

namespace
{
	String GPrefix( TXT("SHOW_") );
}

namespace DebugWindows
{
	CDebugWindowRenderingFlags::CDebugWindowRenderingFlags() 
		: RedGui::CRedGuiWindow(100,100,400,350)
		, m_rightPanel(nullptr)
		, m_postprocessFlagsPanel(nullptr)
		, m_renderingFlagsPanel(nullptr)
		, m_debugFlagsPanel(nullptr)
	{
		SetCaption(TXT("Game filters"));

		// create left panel
		RedGui::CRedGuiList* list = new RedGui::CRedGuiList(0,0,150,300);
		list->EventSelectedItem.Bind( this, &CDebugWindowRenderingFlags::NotifySelectedListItem );
		list->EventDoubleClickItem.Bind( this, &CDebugWindowRenderingFlags::NotifySelectedListItem );
		list->SetMargin(Box2(5, 5, 5, 5));
		list->SetColLabelsVisible(false);
		list->AddItem(TXT("Rendering"));
		list->AddItem(TXT("Postprocess"));
		list->AddItem(TXT("Debug"));
		list->AddItem(TXT("Umbra"));
		list->AddItem(TXT("Physics"));
		list->SetBackgroundColor(Color::CLEAR);
		list->SetDock(RedGui::DOCK_Left);
		list->AppendColumn( TXT(""), 150 );
		AddChild( list );

		// create right panel
		m_rightPanel = new RedGui::CRedGuiPanel(170, 10, 200, 300);
		m_rightPanel->SetMargin(Box2(5, 5, 5, 5));
		m_rightPanel->SetBackgroundColor(Color::CLEAR);
		m_rightPanel->SetDock(RedGui::DOCK_Fill);		
		AddChild(m_rightPanel);

		// collect options
		for ( Uint32 i=0; GShowGameFilter[i] != SHOW_MAX_INDEX; ++i )
		{
			EShowFlags flag = GShowGameFilter[i];
			m_showFlags.PushBack( flag );
		}

		m_showFlags.Remove( SHOW_Profilers );

		// create right panels
		CreateRenderingFlagsWindow();
		CreatePostprocessFlagsWindow();
		CreateDebugFlagsWindow();
		CreateUmbraFlagsWindow();
		CreatePhysicsFlagsWindow();
	}

	CDebugWindowRenderingFlags::~CDebugWindowRenderingFlags()
	{
		/*intentionally empty*/
	}

	void CDebugWindowRenderingFlags::NotifySelectedListItem( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedOption)
	{
		RED_UNUSED( eventPackage );

		HideAllPanels();

		switch(selectedOption)
		{
		case 0:		// rendering flags
			ShowRenderingFlagsPanel();
			break;
		case 1:		// postprocess flags
			ShowPostprocessFlagsPanel();
			break;
		case 2:		// debug flags
			ShowDebugFlagsPanel();
			break;
		case 3:		// umbra flags
			ShowUmbraFlagsPanel();
			break;
		case 4:		// umbra flags
			ShowPhysicsFlagsPanel();
			break;
		}
	}

	void CDebugWindowRenderingFlags::NotifyCheckBoxValueChanged( RedGui::CRedGuiEventPackage& eventPackage, Bool value )
	{
		CRedGuiControl* sender = eventPackage.GetEventSender();

		RedGui::CRedGuiCheckBox* checkBox = static_cast<RedGui::CRedGuiCheckBox*>(sender);
		EShowFlags flag = *(checkBox->GetUserData<EShowFlags>());
		IViewport* gameViewport = GGame->GetViewport();

		if ( value == true )
		{
			gameViewport->SetRenderingMask( flag );
		}
		else
		{
			gameViewport->ClearRenderingMask( flag );
		}
	}

	void CDebugWindowRenderingFlags::CreateRenderingFlagsWindow()
	{
		m_renderingFlagsPanel = new RedGui::CRedGuiScrollPanel(0, 0, 200, 300);
		m_renderingFlagsPanel->SetBorderVisible(true);
		m_renderingFlagsPanel->SetVisible(false);
		m_renderingFlagsPanel->SetNeedKeyFocus( true );
		m_rightPanel->AddChild(m_renderingFlagsPanel);
		m_renderingFlagsPanel->SetDock(RedGui::DOCK_Fill);

		// Fill with rendering related options
		for ( Uint32 i=0; GShowRenderFilter[i] != SHOW_MAX_INDEX; ++i )
		{
			EShowFlags flag = GShowRenderFilter[i];
			RedGui::CRedGuiCheckBox* newCheckbox = new RedGui::CRedGuiCheckBox(10, i*20 + 10, 0, 0);
			newCheckbox->SetUserData(&GShowRenderFilter[i]);
			String text = CEnum::ToString< EShowFlags >( GShowRenderFilter[i] );
			newCheckbox->SetText( text.RightString( text.GetLength() - GPrefix.GetLength() ) );
			newCheckbox->SetMargin(Box2(3, 3, 0, 3));
			newCheckbox->EventCheckedChanged.Bind(this, &CDebugWindowRenderingFlags::NotifyCheckBoxValueChanged);
			m_renderingFlags.PushBack(newCheckbox);
			m_showFlags.Remove( flag );
			newCheckbox->SetDock(RedGui::DOCK_Top);
		}

		SortAndFillMenu( m_renderingFlagsPanel, m_renderingFlags );
	}

	void CDebugWindowRenderingFlags::CreatePostprocessFlagsWindow()
	{
		m_postprocessFlagsPanel = new RedGui::CRedGuiScrollPanel(0, 0, 200, 300);
		m_postprocessFlagsPanel->SetBorderVisible(true);
		m_postprocessFlagsPanel->SetVisible(false);
		m_postprocessFlagsPanel->SetNeedKeyFocus( true );
		m_rightPanel->AddChild(m_postprocessFlagsPanel);
		m_postprocessFlagsPanel->SetDock(RedGui::DOCK_Fill);

		// Fill with rendering related options
		for ( Uint32 i=0; GShowPostProcessFilter[i] != SHOW_MAX_INDEX; ++i )
		{
			EShowFlags flag = GShowPostProcessFilter[i];
			RedGui::CRedGuiCheckBox* newCheckbox = new RedGui::CRedGuiCheckBox(10, i*20 + 10, 0, 0);
			newCheckbox->SetUserData(&GShowPostProcessFilter[i]);
			String text = CEnum::ToString< EShowFlags >( GShowPostProcessFilter[i] );
			newCheckbox->SetText( text.RightString( text.GetLength() - GPrefix.GetLength() ) );
			newCheckbox->SetMargin(Box2(3, 3, 0, 3));
			newCheckbox->EventCheckedChanged.Bind(this, &CDebugWindowRenderingFlags::NotifyCheckBoxValueChanged);
			m_postprocessFlags.PushBack(newCheckbox);
			m_showFlags.Remove( flag );
			newCheckbox->SetDock(RedGui::DOCK_Top);
		}

		SortAndFillMenu( m_postprocessFlagsPanel, m_postprocessFlags );
	}

	void CDebugWindowRenderingFlags::CreateDebugFlagsWindow()
	{
		m_debugFlagsPanel = new RedGui::CRedGuiScrollPanel(0, 0, 200, 300);
		m_debugFlagsPanel->SetBorderVisible(true);
		m_debugFlagsPanel->SetVisible(false);
		m_debugFlagsPanel->SetNeedKeyFocus( true );
		m_rightPanel->AddChild(m_debugFlagsPanel);
		m_debugFlagsPanel->SetDock(RedGui::DOCK_Fill);

		// Fill with rest of the options
		for ( Uint32 i=0; i<m_showFlags.Size(); ++i )
		{
			RedGui::CRedGuiCheckBox* newCheckbox = new RedGui::CRedGuiCheckBox(10, i*20 + 10, 0, 0);
			newCheckbox->SetUserData(&m_showFlags[i]);
			String text = CEnum::ToString< EShowFlags >( m_showFlags[i] );
			newCheckbox->SetText( text.RightString( text.GetLength() - GPrefix.GetLength() ) );
			newCheckbox->SetMargin(Box2(3, 3, 0, 3));
			newCheckbox->EventCheckedChanged.Bind(this, &CDebugWindowRenderingFlags::NotifyCheckBoxValueChanged);
			m_debugFlags.PushBack(newCheckbox);
			newCheckbox->SetDock(RedGui::DOCK_Top);
		}

		SortAndFillMenu( m_debugFlagsPanel, m_debugFlags );
	}

	void CDebugWindowRenderingFlags::CreateUmbraFlagsWindow()
	{
		m_umbraFlagsPanel = new RedGui::CRedGuiScrollPanel(0, 0, 200, 300);
		m_umbraFlagsPanel->SetBorderVisible(true);
		m_umbraFlagsPanel->SetVisible(false);
		m_umbraFlagsPanel->SetNeedKeyFocus( true );
		m_rightPanel->AddChild(m_umbraFlagsPanel);
		m_umbraFlagsPanel->SetDock(RedGui::DOCK_Fill);

		// collect options
		for ( Uint32 i=0; GShowUmbraDebugFilter[i] != SHOW_MAX_INDEX; ++i )
		{
			RedGui::CRedGuiCheckBox* newCheckbox = new RedGui::CRedGuiCheckBox(10, i*20 + 10, 0, 0);
			newCheckbox->SetUserData( &GShowUmbraDebugFilter[i] );
			String text = CEnum::ToString< EShowFlags >( GShowUmbraDebugFilter[i] );
			newCheckbox->SetText( text.RightString( text.GetLength() - GPrefix.GetLength() ) );
			newCheckbox->SetMargin(Box2(3, 3, 0, 3));
			newCheckbox->EventCheckedChanged.Bind(this, &CDebugWindowRenderingFlags::NotifyCheckBoxValueChanged);
			m_umbraFlags.PushBack(newCheckbox);
			newCheckbox->SetDock(RedGui::DOCK_Top);
		}

		SortAndFillMenu( m_umbraFlagsPanel, m_umbraFlags );
	}

	void CDebugWindowRenderingFlags::CreatePhysicsFlagsWindow()
	{
		m_physicsFlagsPanel = new RedGui::CRedGuiScrollPanel(0, 0, 200, 300);
		m_physicsFlagsPanel->SetBorderVisible(true);
		m_physicsFlagsPanel->SetVisible(false);
		m_physicsFlagsPanel->SetNeedKeyFocus( true );
		m_rightPanel->AddChild(m_physicsFlagsPanel);
		m_physicsFlagsPanel->SetDock(RedGui::DOCK_Fill);

		// collect options
		for ( Uint32 i=0; GShowPhysicsDebugFilter[i] != SHOW_MAX_INDEX; ++i )
		{
			RedGui::CRedGuiCheckBox* newCheckbox = new RedGui::CRedGuiCheckBox(10, i*20 + 10, 0, 0);
			newCheckbox->SetUserData( &GShowPhysicsDebugFilter[i] );
			String text = CEnum::ToString< EShowFlags >( GShowPhysicsDebugFilter[i] );
			newCheckbox->SetText( text.RightString( text.GetLength() - GPrefix.GetLength() ) );
			newCheckbox->SetMargin(Box2(3, 3, 0, 3));
			newCheckbox->EventCheckedChanged.Bind(this, &CDebugWindowRenderingFlags::NotifyCheckBoxValueChanged);
			m_physicsFlags.PushBack(newCheckbox);
			newCheckbox->SetDock(RedGui::DOCK_Top);
		}

		SortAndFillMenu( m_physicsFlagsPanel, m_physicsFlags );
	}

	void CDebugWindowRenderingFlags::ShowRenderingFlagsPanel()
	{
		IViewport* gameViewport = GGame->GetViewport();
		for ( Uint32 i=0; GShowRenderFilter[i] != SHOW_MAX_INDEX; ++i )
		{
			EShowFlags flag = *(m_renderingFlags[i]->GetUserData<EShowFlags>());
			m_renderingFlags[i]->SetChecked(gameViewport->GetRenderingMask()[ flag ]);
		}

		m_renderingFlagsPanel->SetVisible(true);
	}

	void CDebugWindowRenderingFlags::ShowPostprocessFlagsPanel()
	{
		IViewport* gameViewport = GGame->GetViewport();
		for ( Uint32 i=0; GShowPostProcessFilter[i] != SHOW_MAX_INDEX; ++i )
		{
			EShowFlags flag = *(m_postprocessFlags[i]->GetUserData<EShowFlags>());
			m_postprocessFlags[i]->SetChecked(gameViewport->GetRenderingMask()[ flag ]);
		}

		m_postprocessFlagsPanel->SetVisible(true);
	}

	void CDebugWindowRenderingFlags::ShowDebugFlagsPanel()
	{
		IViewport* gameViewport = GGame->GetViewport();
		for ( Uint32 i=0; i < m_showFlags.Size(); ++i )
		{
			EShowFlags flag = *(m_debugFlags[i]->GetUserData<EShowFlags>());
			m_debugFlags[i]->SetChecked(gameViewport->GetRenderingMask()[ flag ]);
		}

		m_debugFlagsPanel->SetVisible(true);
	}

	void CDebugWindowRenderingFlags::ShowUmbraFlagsPanel()
	{
		IViewport* gameViewport = GGame->GetViewport();
		for ( Uint32 i=0; i < m_umbraFlags.Size(); ++i )
		{
			EShowFlags flag = *(m_umbraFlags[i]->GetUserData<EShowFlags>());
			m_umbraFlags[i]->SetChecked(gameViewport->GetRenderingMask()[ flag ]);
		}

		m_umbraFlagsPanel->SetVisible(true);
	}

	void CDebugWindowRenderingFlags::ShowPhysicsFlagsPanel()
	{
		IViewport* gameViewport = GGame->GetViewport();
		for ( Uint32 i=0; i < m_physicsFlags.Size(); ++i )
		{
			EShowFlags flag = *(m_physicsFlags[i]->GetUserData<EShowFlags>());
			m_physicsFlags[i]->SetChecked(gameViewport->GetRenderingMask()[ flag ]);
		}

		m_physicsFlagsPanel->SetVisible(true);
	}

	void CDebugWindowRenderingFlags::HideAllPanels()
	{
		m_renderingFlagsPanel->SetVisible(false);
		m_postprocessFlagsPanel->SetVisible(false);
		m_debugFlagsPanel->SetVisible(false);
		m_umbraFlagsPanel->SetVisible( false );
		m_physicsFlagsPanel->SetVisible( false );
	}

	void CDebugWindowRenderingFlags::SortAndFillMenu( RedGui::CRedGuiScrollPanel* menuPanel, TDynArray< RedGui::CRedGuiCheckBox* >& flags )
	{
		struct OrderByRedGuiCheckBox
		{
			RED_INLINE Bool operator()( RedGui::CRedGuiCheckBox* const & e1, RedGui::CRedGuiCheckBox* const & e2 ) const
			{
				return e1->GetText() < e2->GetText();
			}
		};

		Sort( flags.Begin(), flags.End(), OrderByRedGuiCheckBox() );

		// Fill the menu
		for ( Uint32 i = 0; i < flags.Size(); ++i )
		{
			menuPanel->AddChild( flags[ i ] );
		}
	}

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
