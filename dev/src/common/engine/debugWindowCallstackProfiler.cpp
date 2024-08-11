/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiTab.h"
#include "redGuiTreeView.h"
#include "redGuiTreeNode.h"
#include "redGuiButton.h"
#include "redGuiTextBox.h"
#include "redGuiPanel.h"
#include "redGuiScrollPanel.h"
#include "redGuiList.h"
#include "redGuiListItem.h"
#include "redGuiLabel.h"
#include "redGuiManager.h"
#include "debugWindowCallstackProfiler.h"

namespace DebugWindows
{
	namespace
	{
		const String	GFunctionNameSeparator = TXT("\\");
		const Double	GMinAverageTime = 0.00001;
		const Char*		GTextFiller = TXT("                                                                                                    ");
			  Uint32	GFillerWidth = 1;
	}

	CDebugWindowCallstackProfiler::SInternalCounterInfo::SInternalCounterInfo(CPerfCounter* counter, SInternalCounterInfo* parent/* = nullptr*/)
		: m_statBox( counter )
		, m_parent( parent )
		, m_child( nullptr )
		, m_sibling( nullptr )
	{
		//recursive
		if( counter->GetFirstChild() != nullptr )
		{
			m_child = new SInternalCounterInfo( counter->GetFirstChild(), this );
		}
		if( counter->GetSibling() != nullptr )
		{
			m_sibling = new SInternalCounterInfo( counter->GetSibling(), parent );
		}
	}

	CDebugWindowCallstackProfiler::SInternalCounterInfo::~SInternalCounterInfo()
	{
		SInternalCounterInfo* node = m_child;
		while(node != nullptr)
		{
			SInternalCounterInfo* t = node->m_sibling;
			delete node;
			node = t;
		}
	}

	void CDebugWindowCallstackProfiler::SInternalCounterInfo::Tick()
	{
		m_statBox.Tick();
		SInternalCounterInfo* node = m_child;
		while(node != nullptr)
		{
			node->Tick();
			node = node->m_sibling;
		}
	}

	CDebugWindowCallstackProfiler::SInternalCouterListInfo::SInternalCouterListInfo( SInternalCounterInfo* counter, const String& name )
		: m_name( name )
		, m_connectedListItem( nullptr )
	{
		Reset();
		m_counters.PushBackUnique( counter );
	}

	CDebugWindowCallstackProfiler::SInternalCouterListInfo::~SInternalCouterListInfo()
	{
		/* intentionally empty */
	}

	void CDebugWindowCallstackProfiler::SInternalCouterListInfo::Reset()
	{
		m_time = 0.0;
		self_time = 0.0;
		m_hitCount = 0.0;
	}

	void CDebugWindowCallstackProfiler::SInternalCouterListInfo::Update()
	{
		Reset();

		const Uint32 counterCount = m_counters.Size();
		for( Uint32 i=0; i<counterCount; ++i )
		{
			//grab stats
			m_time += m_counters[i]->m_statBox.GetAverageTime();
			m_hitCount += m_counters[i]->m_statBox.GetAverageHitCount();
			//adjust for children
			SInternalCounterInfo* child = m_counters[i]->m_child;
			while (child)
			{
				self_time-=child->m_statBox.GetAverageTime();
				child = child->m_sibling;
			}
		}
	}

	CDebugWindowCallstackProfiler::CDebugWindowCallstackProfiler()
		: RedGui::CRedGuiWindow(200,200,800,600)
		, m_functionsTree(nullptr)
		, m_counterCount( 0 )
		, m_currentSearchResultIndex ( -1 )
	{
		GFillerWidth = (Uint32)GRedGui::GetInstance().GetFontManager()->GetStringSize( TXT(" "), RedGui::RGFT_Default ).X;
		GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindowCallstackProfiler::NotifyEventTick );
		SetCaption(TXT("Callstack profiler"));

		CreateControls();
	}

	CDebugWindowCallstackProfiler::~CDebugWindowCallstackProfiler()
	{
		GRedGui::GetInstance().EventTick.Unbind( this, &CDebugWindowCallstackProfiler::NotifyEventTick );
	}

	void CDebugWindowCallstackProfiler::CreateControls()
	{
		CreateSearchControls();

		// create tabs control
		m_tabs = new RedGui::CRedGuiTab( 0, 0, 100, 100 );
		m_tabs->SetDock( RedGui::DOCK_Fill );
		m_tabs->SetMargin( Box2( 5, 5, 5, 5 ) );
		AddChild( m_tabs );

		CreateTreeViewControls();
		CreateFlatViewControls();

		m_tabs->SetActiveTab( TT_TreeView );
	}

	void CDebugWindowCallstackProfiler::CreateTreeViewControls()
	{
		m_tabs->AddTab( TXT("Tree view") );

		RedGui::CRedGuiScrollPanel* treeViewTab = m_tabs->GetTabAt( TT_TreeView );
		if( treeViewTab != nullptr )
		{
			m_functionsTree = new RedGui::CRedGuiTreeView( 0, 0, 100, 100 );
			m_functionsTree->SetMargin( Box2( 5, 5, 5, 5 ) );
			m_functionsTree->SetDock( RedGui::DOCK_Fill );
			m_functionsTree->SetHeaderVisible( true );
			m_functionsTree->SetHeaderCaption( TXT("Name     |     Percent     |     Time [ms] / Max time [ms]     |     Hit count") );
			treeViewTab->AddChild( m_functionsTree );
		}
	}

	void CDebugWindowCallstackProfiler::CreateFlatViewControls()
	{
		m_tabs->AddTab( TXT("Flat view") );

		RedGui::CRedGuiScrollPanel* flatViewTab = m_tabs->GetTabAt( TT_FlatView );
		if( flatViewTab != nullptr )
		{
			m_functionsList = new RedGui::CRedGuiList( 0, 0, 100, 100 );
			m_functionsList->SetMargin( Box2( 5, 5, 5, 5 ) );
			m_functionsList->SetDock( RedGui::DOCK_Fill );
			m_functionsList->AppendColumn( TXT("Name"), 350 );
			m_functionsList->AppendColumn( TXT("Time [ms]"), 100, RedGui::SA_Real );
			m_functionsList->AppendColumn( TXT("Self time [ms]"), 100, RedGui::SA_Real );
			m_functionsList->AppendColumn( TXT("Hit count"), 100, RedGui::SA_Integer );
			m_functionsList->SetSorting( true );
			flatViewTab->AddChild( m_functionsList );
		}
	}

	void CDebugWindowCallstackProfiler::CreateSearchControls()
	{
		RedGui::CRedGuiPanel* bottomPanel = new RedGui::CRedGuiPanel(0, 0, 100, 20);
		bottomPanel->SetMargin( Box2(5, 5, 5, 5) );
		bottomPanel->SetDock(RedGui::DOCK_Bottom);
		bottomPanel->SetBorderVisible( false );
		bottomPanel->SetBackgroundColor( Color::CLEAR );
		AddChild(bottomPanel);
		{
			m_searchButton = new RedGui::CRedGuiButton(0, 0, 150, 20);
			m_searchButton->SetDock( RedGui::DOCK_Right );
			m_searchButton->SetText( TXT("Search") );
			m_searchButton->SetMargin( Box2(5, 0, 0, 0) );
			bottomPanel->AddChild( m_searchButton );
			m_searchButton->EventButtonClicked.Bind( this, &CDebugWindowCallstackProfiler::NotifyEventButtonClickedSearch );

			m_nextButton = new RedGui::CRedGuiButton( 0, 0, 20, 20 );
			m_nextButton->SetDock( RedGui::DOCK_Right );
			m_nextButton->SetText( TXT(">>") );
			m_nextButton->SetMargin( Box2(5, 0, 0, 0) );
			m_nextButton->SetEnabled( false );
			bottomPanel->AddChild( m_nextButton );
			m_nextButton->EventButtonClicked.Bind( this, &CDebugWindowCallstackProfiler::NotifyEventButtonClickedNextResult );

			m_previousButton = new RedGui::CRedGuiButton(0, 0, 20, 20);
			m_previousButton->SetDock( RedGui::DOCK_Right );
			m_previousButton->SetText( TXT("<<") );
			m_previousButton->SetMargin( Box2(5, 0, 0, 0) );
			m_previousButton->SetEnabled( false );
			bottomPanel->AddChild( m_previousButton );
			m_previousButton->EventButtonClicked.Bind( this, &CDebugWindowCallstackProfiler::NotifyEventButtonClickedPreviousResult );

			m_searchInfo = new RedGui::CRedGuiLabel( 0, 0, 50, 20 );
			m_searchInfo->SetDock( RedGui::DOCK_Right );
			m_searchInfo->SetText( TXT("0 / 0") );
			m_searchInfo->SetMargin( Box2(5, 0, 0, 0) );
			bottomPanel->AddChild( m_searchInfo );

			m_searchLine = new RedGui::CRedGuiTextBox( 0, 0, 100, 20 );
			m_searchLine->SetDock( RedGui::DOCK_Fill );
			bottomPanel->AddChild( m_searchLine );
		}
	}

	void CDebugWindowCallstackProfiler::NotifyEventTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta )
	{
		if( GetVisible() == false )
		{
			return;
		}

		// rebuild counters views
		RebuildCountersViews();

		// update counters info
		const Uint32 conterInfoCount = m_countersInfo.Size();
		for( Uint32 i=0; i<conterInfoCount; ++i)
		{
			m_countersInfo[i]->Tick();
		}

		// update views
		const Int32 activeTab = m_tabs->GetActiveTabIndex();
		if( activeTab == 0 )
		{
			UpdateTreeView();
		}
		else if( activeTab == 1 )
		{
			UpdateListView();
		}
	}

	void CDebugWindowCallstackProfiler::FillTreeView()
	{
		// Enumerate profilers
		for (Uint32 i=0; i<CProfiler::GetThreadCount(); ++i)
		{
			RecursiveFillTreeView( m_countersInfo[i], nullptr );
		}

		m_functionsTree->Refresh();
	}

	void CDebugWindowCallstackProfiler::RecursiveFillTreeView( SInternalCounterInfo* counterInfo, RedGui::CRedGuiTreeNode* parentNode )
	{
		RedGui::CRedGuiTreeNode* childNode = GetDCBProfiler( counterInfo, parentNode );

		//breadth first
		if ( counterInfo->m_sibling != nullptr)
		{
			RecursiveFillTreeView( counterInfo->m_sibling, parentNode );
		}

		if( counterInfo->m_child != nullptr )
		{
			RecursiveFillTreeView( counterInfo->m_child, childNode );
		}
	}

	RedGui::CRedGuiTreeNode* CDebugWindowCallstackProfiler::GetDCBProfiler( SInternalCounterInfo* statNode, RedGui::CRedGuiTreeNode* checkParent )
	{
		String name = StripPath(statNode);
		Uint32 textWidth = (Uint32)GRedGui::GetInstance().GetFontManager()->GetStringSize( name, RedGui::RGFT_Default ).X;

		RedGui::CRedGuiTreeNode* ret = nullptr;
		if (checkParent == nullptr)
		{
			TDynArray< RedGui::CRedGuiTreeNode* >& rootNodes = m_functionsTree->GetRootNodes();
			for(Uint32 i=0; i<rootNodes.Size(); ++i)
			{
				if( rootNodes[i]->GetUserString( TXT("KeyName") ) == name)
				{
					ret = rootNodes[i];
					break;
				}
			}

			if ( ret == nullptr )
			{
				ret = m_functionsTree->AddRootNode( name );
			}
		}
		else
		{
			TDynArray< RedGui::CRedGuiTreeNode* > childrenNodes = checkParent->GetChildrenNodes();
			for(Uint32 i=0; i<childrenNodes.Size(); ++i)
			{
				if( childrenNodes[i]->GetUserString( TXT("KeyName") ) == name)
				{
					ret = childrenNodes[i];
					break;
				}
			}

			if ( ret == nullptr )
			{
				ret = checkParent->AddNode(name);
			}
		}

		ret->SetUserData( statNode );
		ret->SetUserString( TXT("KeyName"), name );
		ret->SetUserString( TXT("NameWidth"), ToString( textWidth ) );

		return ret;
	}

	void CDebugWindowCallstackProfiler::UpdateTreeView()
	{
		RedGui::TreeNodeCollection nodes;
		m_functionsTree->GetAllNodes( nodes );

		for(Uint32 i=0; i<nodes.Size(); ++i)
		{
			if( nodes[i]->GetVisible() == true )
			{
				SInternalCounterInfo* stats = nullptr;
				stats = nodes[i]->GetUserData< SInternalCounterInfo >();
				if( stats != nullptr )
				{
					//grab stats
					double time = stats->m_statBox.GetAverageTime();
					double maxTime = stats->m_statBox.GetMaxTime();
					double hitCount = stats->m_statBox.GetAverageHitCount();
					double percent = 0;
					double percentMax = 0.0f;
					if ( stats->m_parent == nullptr )
					{
						//adjust for children
						SInternalCounterInfo* child = stats->m_child;
						while( child != nullptr )
						{
							time += child->m_statBox.GetAverageTime();
							maxTime += child->m_statBox.GetMaxTime();
							child = child->m_sibling;
						}
					}
					else if( stats->m_parent->m_statBox.GetAverageTime() > GMinAverageTime )
					{
						double avg = stats->m_parent->m_statBox.GetAverageTime();
						percent = 100*time/avg;
						percentMax = 100*maxTime/avg;
					}

					String rawName = nodes[i]->GetUserString( TXT("KeyName") );
					Uint32 textWidth;
					FromString< Uint32 >( nodes[i]->GetUserString( TXT("NameWidth") ), textWidth );
					Uint32 fillerCount = Clamp<Uint32>( (300 - textWidth) / GFillerWidth, 1, 1000 );
					rawName.Append( GTextFiller, fillerCount );
					nodes[i]->SetText( String::Printf(TXT("%ls   %.2lf          |          %.3lf / %.3lf          |          %.0lf   "), rawName.AsChar(), percent, time, maxTime, hitCount) );
				}
			}
		}
	}

	void CDebugWindowCallstackProfiler::RebuildCountersViews()
	{
		// if counter count is the same as in the previous frame we cannot rebuild counters view
		if( m_counterCount == (Int32)CProfiler::GetCountersCount() )
		{
			return;
		}

		// collect existing counters
		m_counterCount = CProfiler::GetCountersCount();
		CollectCounters();

		// create tree and list from collected counters
		FillTreeView();
		FillListView();
	}

	void CDebugWindowCallstackProfiler::CollectCounters()
	{
		//free hierarchy
		for (Uint32 i=0; i<m_countersInfo.Size(); ++i)
		{
			delete m_countersInfo[i];
		}
		m_countersInfo.Clear();

		//create counter info hierarchy
		for (Uint32 i=0; i<CProfiler::GetThreadCount(); ++i)
		{
			m_countersInfo.PushBack( new SInternalCounterInfo( CProfiler::GetThreadRoot(i), nullptr ) );
		}
	}

	void CDebugWindowCallstackProfiler::SetVisible( Bool value )
	{
		CRedGuiWindow::SetVisible( value );

		if( value == true )
		{
			RebuildCountersViews();
		}
	}

	String CDebugWindowCallstackProfiler::StripPath(SInternalCounterInfo* statNode)
	{
		if ( !statNode->m_statBox.GetPerfCounter()->GetName() || 
			!strlen( statNode->m_statBox.GetPerfCounter()->GetName() ) || 
			statNode->m_statBox.GetPerfCounter()->GetName()[0] == ' ' )
		{
			return TXT("Empty perf counter name!");
		}

		const Char* str = ANSI_TO_UNICODE( statNode->m_statBox.GetPerfCounter()->GetName() );

		//special case for Roots!
		Int32 rootIndex = -1;
		for ( Uint32 i=0; i<CProfiler::GetThreadCount(); ++i )
		{
			if ( statNode->m_statBox.GetPerfCounter() == CProfiler::GetThreadRoot(i) )
			{
				rootIndex = i;
				break;
			}
		}

		TDynArray<String> path = String(str).Split( GFunctionNameSeparator );
		if ( rootIndex > -1 )
		{
			return String::Printf(TXT("%s %d"),path.Back().AsChar(), rootIndex);
		}
		return path.Back();
	}

	void CDebugWindowCallstackProfiler::NotifyEventButtonClickedSearch( RedGui::CRedGuiEventPackage& eventPackage )
	{
		if( m_functionsTree != nullptr )
		{
			String searchingText = m_searchLine->GetText();
			SelectCounter( searchingText ); 
		}
	}

	void CDebugWindowCallstackProfiler::SelectCounter( const String& counterName )
	{
		m_searchResults.Clear();
		m_currentSearchResultIndex = -1;
		m_nextButton->SetEnabled( false );
		m_previousButton->SetEnabled( false );

		Bool found = m_functionsTree->FindNodeByName( counterName, m_searchResults );
		if( found == true )
		{
			if( m_searchResults.Size() > 1 )
			{
				m_nextButton->SetEnabled( true );
			}

			m_currentSearchResultIndex = 0;
			m_functionsTree->SetSelectedNode( m_searchResults[m_currentSearchResultIndex] );
			m_functionsTree->SetFirstNode( m_searchResults[m_currentSearchResultIndex] );

			m_functionsList->SetSelection( m_searchResults[m_currentSearchResultIndex]->GetText(), true );
			m_functionsList->SetFirstItem( m_searchResults[m_currentSearchResultIndex]->GetText() );
		}
		UpdateSearchTextLabel();
	}

	void CDebugWindowCallstackProfiler::NotifyEventButtonClickedNextResult( RedGui::CRedGuiEventPackage& eventPackage )
	{
		if( m_currentSearchResultIndex < ( (Int32)m_searchResults.Size() - 1 ) )
		{
			++m_currentSearchResultIndex;
			m_previousButton->SetEnabled( true );

			if( m_currentSearchResultIndex == (Int32)m_searchResults.Size() - 1 )
			{
				m_nextButton->SetEnabled( false );
			}

			m_functionsTree->SetSelectedNode( m_searchResults[m_currentSearchResultIndex] );
			m_functionsTree->SetFirstNode( m_searchResults[m_currentSearchResultIndex] );

			m_functionsList->SetSelection( m_searchResults[m_currentSearchResultIndex]->GetText(), true );
			m_functionsList->SetFirstItem( m_searchResults[m_currentSearchResultIndex]->GetText() );

			UpdateSearchTextLabel();
		}
	}

	void CDebugWindowCallstackProfiler::NotifyEventButtonClickedPreviousResult( RedGui::CRedGuiEventPackage& eventPackage )
	{
		if( m_currentSearchResultIndex > 0 )
		{
			--m_currentSearchResultIndex;
			m_nextButton->SetEnabled( true );

			if( m_currentSearchResultIndex == 0 )
			{
				m_previousButton->SetEnabled( false );
			}

			m_functionsTree->SetSelectedNode( m_searchResults[m_currentSearchResultIndex] );
			m_functionsTree->SetFirstNode( m_searchResults[m_currentSearchResultIndex] );

			m_functionsList->SetSelection( m_searchResults[m_currentSearchResultIndex]->GetText(), true );
			m_functionsList->SetFirstItem( m_searchResults[m_currentSearchResultIndex]->GetText() );

			UpdateSearchTextLabel();
		}
	}

	void CDebugWindowCallstackProfiler::UpdateSearchTextLabel()
	{
		m_searchInfo->SetText( String::Printf( TXT("%d / %d"), m_currentSearchResultIndex+1, m_searchResults.Size() ) );
	}

	void CDebugWindowCallstackProfiler::FillListView()
	{
		m_listInfo.ClearFast();
		m_functionsList->RemoveAllItems();

		const Uint32 threadCount = (Uint32)CProfiler::GetThreadCount();
		for( Uint32 i=0; i<threadCount; ++i )
		{
			RecursiveFillListView( m_countersInfo[i] );
		}
	}

	void CDebugWindowCallstackProfiler::RecursiveFillListView( SInternalCounterInfo* counterInfo )
	{
		if( counterInfo->m_parent != nullptr )
		{
			String name = StripPath( counterInfo );
			Int32 foundElement = -1;

			const Uint32 listCounterCount = m_listInfo.Size();
			for( Uint32 i=0; i<listCounterCount; ++i )
			{
				if( m_listInfo[i].m_name == name )
				{
					foundElement = i;
					m_listInfo[i].m_counters.PushBackUnique( counterInfo );
					break;
				}
			}

			if( foundElement == -1 )
			{
				m_listInfo.PushBack( SInternalCouterListInfo( counterInfo, name ) );
			}
		}

		//breadth first
		if ( counterInfo->m_sibling != nullptr)
		{
			RecursiveFillListView( counterInfo->m_sibling );
		}

		if( counterInfo->m_child != nullptr )
		{
			RecursiveFillListView( counterInfo->m_child );
		}
	}

	void CDebugWindowCallstackProfiler::UpdateListView()
	{
		//
		const Uint32 listRowCount = m_listInfo.Size();
		for( Uint32 i=0; i<listRowCount; ++i )
		{
			m_listInfo[i].Update();

			RedGui::CRedGuiListItem* newItem = m_listInfo[i].m_connectedListItem;
			if( newItem != nullptr )
			{
				newItem->SetText( String::Printf( TXT("%.3lf"), m_listInfo[i].m_time ), 1 );
				newItem->SetText( String::Printf( TXT("%.3lf"), m_listInfo[i].self_time ), 2 );
				newItem->SetText( String::Printf( TXT("%.0lf"), m_listInfo[i].m_hitCount ), 3 );
			}
			else
			{
				RedGui::CRedGuiListItem* newItem = new RedGui::CRedGuiListItem( m_listInfo[i].m_name, &m_listInfo[i] );
				m_listInfo[i].m_connectedListItem = newItem;

				Uint32 newItemIndex = m_functionsList->AddItem( newItem );
				m_functionsList->SetItemText( String::Printf( TXT("%.3lf"), m_listInfo[i].m_time ), newItemIndex, 1 );
				m_functionsList->SetItemText( String::Printf( TXT("%.3lf"), m_listInfo[i].self_time ), newItemIndex, 2 );
				m_functionsList->SetItemText( String::Printf( TXT("%.0lf"), m_listInfo[i].m_hitCount ), newItemIndex, 3 );
			}
		}

	}

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
