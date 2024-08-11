/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "debugWindowMemoryObjects.h"

#include "../../common/redSystem/stringWriter.h"
#include "../../common/core/garbageCollector.h"
#include "../core/depot.h"
#include "../redIO/redIO.h"

#include "redGuiManager.h"
#include "redGuiWindow.h"
#include "redGuiTab.h"
#include "redGuiPanel.h"
#include "redGuiButton.h"
#include "redGuiCheckBox.h"
#include "redGuiGridLayout.h"
#include "redGuiTextBox.h"
#include "redGuiComboBox.h"
#include "redGuiTreeNode.h"
#include "redGuiTreeView.h"
#include "redGuiList.h"
#include "redGuiListItem.h"
#include "redGuiSaveFileDialog.h"
#include "redGuiLabel.h"

namespace DebugWindows
{

CDebugWindowMemoryObjects::CDebugWindowMemoryObjects()
	: RedGui::CRedGuiWindow( 200, 200, 500, 500 )
	, m_showAll( false )
	, m_forceGCEveryFrame( false )
	, m_realtimeUpdate( true )
	, m_realtimeSort( false )
	, m_sortTypeCombobox( nullptr )
	, m_showAllCheckbox( nullptr )
	, m_realtimeUpdateCheckbox( nullptr) 
	, m_realtimeSortCheckbox( nullptr )
	, m_classTreeView( nullptr )
	, m_classListView( nullptr )
	, m_sortMode( eSortMode_Count )
{
	SetCaption( TXT("Memory details for each object's class") );

	CreateControls();
}

CDebugWindowMemoryObjects::~CDebugWindowMemoryObjects()
{
	for ( ClassNode* node : m_rootNodes )
		delete node;

	for ( ClassEntry* entry : m_listEntries )
		delete entry;

	m_listEntries.Clear();
	m_rootNodes.Clear();
}

void CDebugWindowMemoryObjects::OnWindowOpened( CRedGuiControl* control )
{
	RefreshClassTree();
	RefreshClassList();
	CaptureObjectsCount();

	GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindowMemoryObjects::NotifyOnTick );
}

void CDebugWindowMemoryObjects::OnWindowClosed( CRedGuiControl* control )
{
	GRedGui::GetInstance().EventTick.Unbind( this, &CDebugWindowMemoryObjects::NotifyOnTick );
}

void CDebugWindowMemoryObjects::CreateControls()
{
	m_dumpSaveDialog = new RedGui::CRedGuiSaveFileDialog();
	m_dumpSaveDialog->SetDefaultFileName( TXT("object_stats.txt") );
	m_dumpSaveDialog->AddFilter( TXT("Text file"), TXT("txt") );
	m_dumpSaveDialog->EventFileOK.Bind( this, &CDebugWindowMemoryObjects::NotifyDumpStatsFileOK );

	RedGui::CRedGuiGridLayout* menuRow1 = new RedGui::CRedGuiGridLayout( 0, 0, 100, 26 );
	menuRow1->SetDock( RedGui::DOCK_Top );
	menuRow1->SetMargin( Box2( 5, 5, 5, 5 ) );
	menuRow1->SetDimensions( 5, 1 );
	AddChild( menuRow1 );

	RedGui::CRedGuiGridLayout* menuRow2 = new RedGui::CRedGuiGridLayout( 0, 0, 100, 26 );
	menuRow2->SetDock( RedGui::DOCK_Top );
	menuRow2->SetMargin( Box2( 5, 5, 5, 5 ) );
	menuRow2->SetDimensions( 4, 1 );
	AddChild( menuRow2 );

	RedGui::CRedGuiButton* dumpsStats = new RedGui::CRedGuiButton( 0, 0, 40, 20 );
	dumpsStats->SetText( TXT("Dump stats") );
	dumpsStats->SetMargin( Box2( 15, 5, 10, 0 ) );
	dumpsStats->EventButtonClicked.Bind( this, &CDebugWindowMemoryObjects::NotifyOnClickedDumpStats );
	menuRow1->AddChild( dumpsStats );

	RedGui::CRedGuiButton* refreshButton = new RedGui::CRedGuiButton( 40, 0, 40, 20 );
	refreshButton->SetText( TXT("Refresh") );
	refreshButton->SetMargin( Box2( 15, 5, 10, 0 ) );
	refreshButton->EventButtonClicked.Bind( this, &CDebugWindowMemoryObjects::NotifyOnClickedRefresh );
	menuRow1->AddChild( refreshButton );

	RedGui::CRedGuiButton* gcButton = new RedGui::CRedGuiButton( 80, 0, 40, 20 );
	gcButton->SetText( TXT("Force GC") );
	gcButton->SetMargin( Box2( 15, 5, 10, 0 ) );
	gcButton->EventButtonClicked.Bind( this, &CDebugWindowMemoryObjects::NotifyOnClickedForceGC );
	menuRow1->AddChild( gcButton );

	//Snapshot button
	{
		// create tooltip
		RedGui::CRedGuiPanel* captureButtonToolTip = new RedGui::CRedGuiPanel(0, 0, 200, 50);
		captureButtonToolTip->SetBackgroundColor(Color(0, 0, 0));
		captureButtonToolTip->SetVisible(false);
		captureButtonToolTip->AttachToLayer(TXT("Pointers"));
		captureButtonToolTip->SetPadding(Box2(5, 5, 5, 5));
		captureButtonToolTip->SetAutoSize(true);

		RedGui::CRedGuiLabel* whiteInfo = new RedGui::CRedGuiLabel(10, 10, 10, 15);
		whiteInfo->SetText(TXT("Capture snapshot of the object counts"), Color::WHITE);
		(*captureButtonToolTip).AddChild( whiteInfo );

		RedGui::CRedGuiLabel* greenInfo = new RedGui::CRedGuiLabel(10, 25, 10, 15);
		greenInfo->SetText(TXT(" - there are less objects of type than in a snapshot"), Color::Color(151, 255, 151));
		(*captureButtonToolTip).AddChild( greenInfo );

		RedGui::CRedGuiLabel* redInfo = new RedGui::CRedGuiLabel(10, 40, 10, 15);
		redInfo->SetText(TXT(" - there are more objects of type than in a snapshot"), Color::Color(255, 157, 111));
		redInfo->SetMargin(Box2(0, 0, 0, 10));
		(*captureButtonToolTip).AddChild( redInfo );

		// create button
		RedGui::CRedGuiButton* captureButton = new RedGui::CRedGuiButton( 80, 0, 40, 20 );
		captureButton->SetText( TXT("Snapshot") );
		captureButton->SetNeedToolTip(true);
		captureButton->SetMargin( Box2( 15, 5, 10, 0 ) );
		captureButton->SetToolTip( captureButtonToolTip );
		captureButton->EventButtonClicked.Bind( this, &CDebugWindowMemoryObjects::NotifyOnClickedSnapshot );
		menuRow1->AddChild( captureButton );
	}

	m_showAllCheckbox = new RedGui::CRedGuiCheckBox( 150, 10, 70, 20 );
	m_showAllCheckbox->SetText( TXT("Show all classes") );
	m_showAllCheckbox->SetMargin( Box2( 15, 5, 10, 0 ) );
	m_showAllCheckbox->EventCheckedChanged.Bind( this, &CDebugWindowMemoryObjects::NotifyOnClickedToggleAll );
	m_showAllCheckbox->SetChecked( m_showAll );
	menuRow2->AddChild( m_showAllCheckbox );

	m_forceGCPerFrameCheckbox = new RedGui::CRedGuiCheckBox( 150, 10, 70, 20 );
	m_forceGCPerFrameCheckbox->SetText( TXT("Force GC per frame") );
	m_forceGCPerFrameCheckbox->SetMargin( Box2( 15, 5, 10, 0 ) );
	m_forceGCPerFrameCheckbox->EventCheckedChanged.Bind( this, &CDebugWindowMemoryObjects::NotifyOnClickedForceGCPerFrame );
	m_forceGCPerFrameCheckbox->SetChecked( m_forceGCEveryFrame );
	menuRow2->AddChild( m_forceGCPerFrameCheckbox );

	m_realtimeUpdateCheckbox = new RedGui::CRedGuiCheckBox( 220, 10, 70, 20 );
	m_realtimeUpdateCheckbox->SetText( TXT("RT update") );
	m_realtimeUpdateCheckbox->SetSimpleToolTip( TXT("Realtime update") );
	m_realtimeUpdateCheckbox->SetMargin( Box2( 15, 5, 10, 0 ) );
	m_realtimeUpdateCheckbox->EventCheckedChanged.Bind( this, &CDebugWindowMemoryObjects::NotifyOnClickedRealtimeUpdate );
	m_realtimeUpdateCheckbox->SetChecked( m_realtimeUpdate );
	menuRow2->AddChild( m_realtimeUpdateCheckbox );

	m_realtimeSortCheckbox = new RedGui::CRedGuiCheckBox( 220, 10, 70, 20 );
	m_realtimeSortCheckbox->SetText( TXT("RT sort") );
	m_realtimeSortCheckbox->SetSimpleToolTip( TXT("Realtime sort") );
	m_realtimeSortCheckbox->SetMargin( Box2( 15, 5, 10, 0 ) );
	m_realtimeSortCheckbox->EventCheckedChanged.Bind( this, &CDebugWindowMemoryObjects::NotifyOnClickedRealtimeSort );
	m_realtimeSortCheckbox->SetChecked( m_realtimeSort );
	menuRow2->AddChild( m_realtimeSortCheckbox );

	m_sortTypeCombobox = new RedGui::CRedGuiComboBox( 340, 10, 80, 20 );
	m_sortTypeCombobox->AddItem( TXT("Count") );
	m_sortTypeCombobox->AddItem( TXT("Size") );
	m_sortTypeCombobox->AddItem( TXT("Name") );
	m_sortTypeCombobox->AddItem( TXT("Delta") );
	m_sortTypeCombobox->SetSelectedIndex( m_sortMode );
	m_sortTypeCombobox->SetSimpleToolTip( TXT("Sorting mode") );
	m_sortTypeCombobox->SetMargin( Box2( 15, 5, 10, 0 ) );
	m_sortTypeCombobox->EventSelectedIndexChanged.Bind( this, &CDebugWindowMemoryObjects::NotifyEventSelectedSortType );
	menuRow1->AddChild( m_sortTypeCombobox );

	m_tabs = new RedGui::CRedGuiTab( 0, 0, 100, 100 );
	m_tabs->SetDock( RedGui::DOCK_Fill );
	m_tabs->SetMargin( Box2( 5, 5, 5, 5 ) );
	AddChild( m_tabs );

	// tab0
	{
		const Uint32 tabIndex = m_tabs->AddTab( TXT("Class tree") );
		RedGui::CRedGuiScrollPanel* classStatsTab = m_tabs->GetTabAt( tabIndex );

		m_classTreeView = new RedGui::CRedGuiTreeView( 0, 0, 100, 100 );
		m_classTreeView->SetDock( RedGui::DOCK_Fill );
		m_classTreeView->SetMargin( Box2( 5, 5, 5, 5 ) );
		m_classTreeView->AddColumn( 300, TXT("Class name") );
		m_classTreeView->AddColumn( 80, TXT("# Objects") );
		m_classTreeView->AddColumn( 80, TXT("KB") );
		m_classTreeView->AddColumn( 80, TXT("# Delta") );
		m_classTreeView->EnableTabbedColumns( true );
		classStatsTab->AddChild( m_classTreeView );
	}

	// tab1
	{
		const Uint32 tabIndex = m_tabs->AddTab( TXT("Blame list") );
		RedGui::CRedGuiScrollPanel* classStatsTab = m_tabs->GetTabAt( tabIndex );

		m_classListView = new RedGui::CRedGuiList( 0, 0, 100, 100 );
		m_classListView->SetDock( RedGui::DOCK_Fill );
		m_classListView->SetMargin( Box2( 5, 5, 5, 5 ) );
		m_classListView->SetSorting( true );
		m_classListView->AppendColumn( TXT("Class name"), 500, RedGui::SA_String );
		m_classListView->AppendColumn( TXT("# Objects"), 80, RedGui::SA_Integer );
		m_classListView->AppendColumn( TXT("KB"), 80, RedGui::SA_Real );
		m_classListView->AppendColumn( TXT("# Delta"), 80, RedGui::SA_Integer );
		classStatsTab->AddChild( m_classListView );
	}

	// tree is more useful
	m_tabs->SetActiveTab(0);
}

//----

CDebugWindowMemoryObjects::ClassNode::ClassNode( const CClass* classInfo )
	: m_class( classInfo )
	, m_totalCount( ~0u )
	, m_totalMemory( ~0u )
	, m_exclusiveCount( ~0u )
	, m_exclusiveMemory( ~0u )
	, m_capturedTotalCount( 0 )
	, m_capturedTotalMemory( 0 )
	, m_parent( nullptr )
{
}

CDebugWindowMemoryObjects::ClassNode::~ClassNode()
{
	for ( ClassNode* node : m_children )
		delete node;
}

Bool CDebugWindowMemoryObjects::ClassNode::ConditionalRefresh( Bool force, Bool capture, ESortMode sortMode )
{
	Bool changed = false;

	const Uint32 classSize = m_class ? (m_class->GetSize() + m_class->GetScriptDataSize()) : 0;

	Uint32 newTotalCount = m_class ? m_class->GetNumAllocatedObjects() : 0;
	Uint32 newTotalMemory = (newTotalCount * classSize);

	m_exclusiveCount = newTotalCount;
	m_exclusiveMemory = newTotalMemory;

	Bool childrenChanged = false;
	for ( ClassNode* node : m_children )
	{
		childrenChanged |= node->ConditionalRefresh( force, capture, sortMode );
		newTotalCount += node->m_totalCount;
		newTotalMemory += node->m_totalMemory;
	}

	if ( childrenChanged || force )
	{
		if ( sortMode == eSortMode_Count )
		{
			auto sorter = [](RedGui::CRedGuiTreeNode* a, RedGui::CRedGuiTreeNode* b) -> bool
			{
				const ClassNode* nodeA = a->GetUserData<ClassNode>();
				const ClassNode* nodeB = b->GetUserData<ClassNode>();
				return nodeA->m_totalCount > nodeB->m_totalCount;
			};

			m_node->SortChildren( sorter );
		}
		else if ( sortMode == eSortMode_Memory )
		{
			auto sorter = [](RedGui::CRedGuiTreeNode* a, RedGui::CRedGuiTreeNode* b) -> bool
			{
				const ClassNode* nodeA = a->GetUserData<ClassNode>();
				const ClassNode* nodeB = b->GetUserData<ClassNode>();
				return nodeA->m_totalMemory > nodeB->m_totalMemory;
			};

			m_node->SortChildren( sorter );
		}
		else if ( sortMode == eSortMode_Name )
		{
			auto sorter = [](RedGui::CRedGuiTreeNode* a, RedGui::CRedGuiTreeNode* b) -> bool
			{
				const ClassNode* nodeA = a->GetUserData<ClassNode>();
				const ClassNode* nodeB = b->GetUserData<ClassNode>();
				return Red::StringCompare( nodeA->m_class->GetName().AsChar(), nodeB->m_class->GetName().AsChar() ) < 0;
			};

			m_node->SortChildren( sorter );
		}
		else if ( sortMode == eSortMode_Delta )
		{
			auto sorter = [](RedGui::CRedGuiTreeNode* a, RedGui::CRedGuiTreeNode* b) -> bool
			{
				const ClassNode* nodeA = a->GetUserData<ClassNode>();
				const ClassNode* nodeB = b->GetUserData<ClassNode>();
				return nodeA->m_capturedTotalCount > nodeB->m_capturedTotalCount;
			};

			m_node->SortChildren( sorter );
		}
	}

	if ( newTotalCount != m_totalCount || newTotalMemory != m_totalMemory || capture )
	{
		changed = true;

		m_totalCount = newTotalCount;
		m_totalMemory = newTotalMemory; 
		
		if( capture )
		{
			m_capturedTotalCount = m_totalCount;
			m_capturedTotalMemory = m_totalMemory;
		}

		UpdateCaption();
	}

	return childrenChanged | changed;
}

void CDebugWindowMemoryObjects::ClassNode::UpdateCaption()
{
	Red::System::StackStringWriter<Char, 512> buffer;

	if ( m_class )
		buffer.Append( m_class->GetName().AsChar() );
	else
		buffer.Append( TXT("Everything") );

	buffer.Append( TXT("\t") );
	buffer.Appendf( TXT("%d"), m_totalCount );
	buffer.Append( TXT("\t") );
	buffer.Appendf( TXT("%1.3f"), (Float)m_totalMemory / 1024.0f );
	buffer.Append( TXT("\t") );
	
	Int32 balance = GetDelta();
	buffer.Appendf( TXT("%d"), balance);
	if( balance > 0 )
	{
		m_node->SetTextColor(Color::Color(255, 157, 111));
	}
	else if( balance < 0)
	{
		m_node->SetTextColor(Color::Color(151, 255, 151));
	}
	else
	{
		m_node->SetTextColor(Color::WHITE);
	}

	m_node->SetText( buffer.AsChar() );
}

//----

CDebugWindowMemoryObjects::ClassEntry::ClassEntry( const CClass* classInfo )
	: m_class( classInfo )
	, m_exclusiveCount( ~0u )
	, m_exclusiveMemory( ~0u )
	, m_capturedExclusiveCount( 0 )
	, m_capturedExclusiveMemory( 0 )
	, m_item( nullptr )
{
}

void CDebugWindowMemoryObjects::ClassEntry::UpdateCaption()
{
	m_item->SetText( m_class->GetName().AsChar(), 0 );

	{
		Red::System::StackStringWriter<Char, 64> buffer;
		buffer.Appendf( TXT("%d"), m_exclusiveCount );
		m_item->SetText( buffer.AsChar(), 1 );
	}

	{
		Red::System::StackStringWriter<Char, 64> buffer;
		buffer.Appendf( TXT("%1.3f"), (Float)m_exclusiveMemory / 1024.0f );
		m_item->SetText( buffer.AsChar(), 2 );
	}

	{
		Int32 balance = GetDelta();
		Red::System::StackStringWriter<Char, 64> buffer;
		buffer.Appendf( TXT("%d"), balance);
		if( balance > 0 )
		{
			m_item->SetTextColor(Color::Color(255, 157, 111));
		}
		else if( balance < 0 )
		{
			m_item->SetTextColor(Color::Color(151, 255, 151));
		}
		else
		{
			m_item->SetTextColor(Color::WHITE);
		}
		
		m_item->SetText( buffer.AsChar(), 3 );
	}
}

Bool CDebugWindowMemoryObjects::ClassEntry::ConditionalRefresh( Bool force, Bool capture /* = false */ )
{
	Bool changed = false;

	const Uint32 classSize = (m_class->GetSize() + m_class->GetScriptDataSize());

	const Uint32 newCount = m_class->GetNumAllocatedObjects();
	const Uint32 newMemory = classSize * newCount;

	if ( force || (newCount != m_exclusiveCount) || (newMemory != m_exclusiveMemory) || capture)
	{
		m_exclusiveMemory = newMemory;
		m_exclusiveCount = newCount;

		if( capture )
		{
			m_capturedExclusiveCount = m_exclusiveCount;
			m_capturedExclusiveMemory = m_exclusiveMemory;
		}

		UpdateCaption();

		changed = true;
	}

	return changed;
}

//----

void CDebugWindowMemoryObjects::RefreshClassTree()
{
	// remove current content
	m_classTreeView->RemoveAllNodes();

	// delete root nodes
	for ( ClassNode* node : m_rootNodes )
		delete node;
	m_rootNodes.Clear();

	// create the mapping table
	TDynArray< ClassNode* > allNodes;
	allNodes.Resize( SRTTI::GetInstance().GetIndexedClasses().Size() );
	Red::MemoryZero( &allNodes[0], allNodes.DataSize() );

	// get the class range
	Uint32 firstClass = 1;
	Uint32 lastClass = SRTTI::GetInstance().GetIndexedClasses().Size()-1;
	ClassNode* forcedRoot = nullptr;
	if ( !m_showAll )
	{
		firstClass = ClassID< IReferencable >()->GetClassIndex();
		lastClass = ClassID< IReferencable >()->GetLastChildClassIndex();
	}
	else
	{
		forcedRoot = new ClassNode(nullptr);
		forcedRoot->m_node = new RedGui::CRedGuiTreeNode();
		forcedRoot->m_node->SetText( TXT("Everything") );
		forcedRoot->m_parent = nullptr;

		m_classTreeView->AddRootNode( forcedRoot->m_node );
		m_rootNodes.PushBack( forcedRoot );
	}

	// allocate the node buffer

	// create the nodes (in order)
	for ( Uint32 i=firstClass; i<=lastClass; ++i )
	{
		const CClass* objectClass = SRTTI::GetInstance().GetIndexedClasses()[i];

		// create the wrapper
		ClassNode* classNode = new ClassNode( objectClass );
		allNodes[ i ] = classNode;

		// create the node
		classNode->m_node = new RedGui::CRedGuiTreeNode();
		classNode->m_node->SetUserData( classNode );

		// link the wrapper
		if ( objectClass->HasBaseClass() && allNodes[ objectClass->GetBaseClass()->GetClassIndex() ] )
		{
			ClassNode* baseClassNode = allNodes[ objectClass->GetBaseClass()->GetClassIndex() ];
			baseClassNode->m_children.PushBack( classNode );
			classNode->m_parent = baseClassNode;

			baseClassNode->m_node->AddNode( classNode->m_node );
		}	
		else if ( forcedRoot )
		{
			forcedRoot->m_children.PushBack( classNode );
			classNode->m_parent = forcedRoot;

			forcedRoot->m_node->AddNode( classNode->m_node );
		}
		else
		{
			m_rootNodes.PushBack( classNode );
			classNode->m_parent = nullptr;

			m_classTreeView->AddRootNode( classNode->m_node );
		}
	}

	// update the captions
	for ( ClassNode* classNode : m_rootNodes )
		classNode->ConditionalRefresh( m_realtimeSort, false, m_sortMode );

	// rewind
	m_classTreeView->Refresh();
}

void CDebugWindowMemoryObjects::RefreshClassList()
{
	// remove stuff from list
	m_classListView->RemoveAllItems();

	// delete entries
	for ( ClassEntry* entry : m_listEntries )
		delete entry;
	m_listEntries.Clear();

	// get the class range
	Uint32 firstClass = 1;
	Uint32 lastClass = SRTTI::GetInstance().GetIndexedClasses().Size()-1;
	if ( !m_showAll )
	{
		firstClass = ClassID< IReferencable >()->GetClassIndex();
		lastClass = ClassID< IReferencable >()->GetLastChildClassIndex();
	}

	// create entry per class
	for ( Uint32 i=firstClass; i<=lastClass; ++i )
	{
		const CClass* objectClass = SRTTI::GetInstance().GetIndexedClasses()[i];

		// create the wrapper
		ClassEntry* entry  = new ClassEntry( objectClass );
		m_listEntries.PushBack( entry );

		// create the node
		entry->m_item = new RedGui::CRedGuiListItem();
		entry->m_item->SetUserData( entry );

		// update initial caption
		entry->UpdateCaption();

		// add to list
		m_classListView->AddItem( entry->m_item );
	}
}

void CDebugWindowMemoryObjects::CaptureObjectsCount()
{
	bool capture = true;
	for ( ClassNode* classNode : m_rootNodes )
		classNode->ConditionalRefresh( false, capture, m_sortMode );
	for ( ClassEntry* classEntry : m_listEntries )
		classEntry->ConditionalRefresh( false, capture );
}

void CDebugWindowMemoryObjects::NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime )
{
	if ( m_realtimeUpdate )
	{
		// realtime update of the class tree
		{
			// update the root nodes
			Bool nodesUpdated = false;
			for ( ClassNode* classNode : m_rootNodes )
				nodesUpdated |= classNode->ConditionalRefresh( m_realtimeSort, false, m_sortMode );

			// refresh layout
			if ( nodesUpdated )
				m_classTreeView->Refresh();
		}

		// realtime update of the class list
		{
			Bool entriesUpdated = false;
			for ( ClassEntry* classEntry : m_listEntries )
				entriesUpdated |= classEntry->ConditionalRefresh( false, false );
		}
	}

	if( m_forceGCEveryFrame )
	{
		SGarbageCollector::GetInstance().CollectNow();
	}
}

void CDebugWindowMemoryObjects::NotifyOnClickedForceGCPerFrame(RedGui::CRedGuiEventPackage& eventPackage, Bool value)
{
	m_forceGCEveryFrame = value;
}

void CDebugWindowMemoryObjects::NotifyOnClickedRealtimeUpdate( RedGui::CRedGuiEventPackage& eventPackage, Bool value )
{
	m_realtimeUpdate = value;
}

void CDebugWindowMemoryObjects::NotifyOnClickedRealtimeSort(RedGui::CRedGuiEventPackage& eventPackage, Bool value)
{
	m_realtimeSort = value;
	if( value )
		m_classListView->SetColumnSortIndex( m_sortMode );
	else
		m_classListView->SetColumnSortIndex( -1 );
}

void CDebugWindowMemoryObjects::NotifyOnClickedToggleAll( RedGui::CRedGuiEventPackage& eventPackage, Bool value )
{
	m_showAll = value;
	RefreshClassTree();
	RefreshClassList();
}

void CDebugWindowMemoryObjects::NotifyOnClickedRefresh( RedGui::CRedGuiEventPackage& eventPackage )
{
	RefreshClassTree();
	RefreshClassList();
}

void CDebugWindowMemoryObjects::NotifyOnClickedSnapshot(RedGui::CRedGuiEventPackage& eventPackage)
{
	CaptureObjectsCount();
}

void CDebugWindowMemoryObjects::NotifyOnClickedDumpStats( RedGui::CRedGuiEventPackage& eventPackage )
{
	RED_UNUSED( eventPackage );

	m_dumpSaveDialog->SetVisible( true );
}

void CDebugWindowMemoryObjects::NotifyDumpStatsFileOK( RedGui::CRedGuiEventPackage& eventPackage )
{
	RED_UNUSED( eventPackage );

	String path = String::EMPTY;
	GDepot->GetAbsolutePath( path );
	path += m_dumpSaveDialog->GetFileName();

	// gather information
	String text = String::EMPTY;
	text += TXT("==================================================================================================================================|\n");
	text += TXT("|                                                                            Class name |  # Objects  |  Memory MB  |   # Delta   |\n");
	text += TXT("==================================================================================================================================|\n");
	for( Uint32 i = 0; i < m_classListView->GetItemCount(); ++i)
	{
		text += String::Printf( TXT("| %86s"), m_classListView->GetItemText(i, 0).AsChar() );
		text += String::Printf( TXT("| %12s"), m_classListView->GetItemText(i, 1).AsChar() );
		text += String::Printf( TXT("| %12s"), m_classListView->GetItemText(i, 2).AsChar() );
		text += String::Printf( TXT("| %12s"), m_classListView->GetItemText(i, 3).AsChar() );
		text += TXT("|\n");
	}
	text += TXT("==================================================================================================================================|\n");

	text += TXT("\n\n\n\n");

	// write text to file
	GFileManager->SaveStringToFile(path, text);
}

void CDebugWindowMemoryObjects::NotifyOnClickedForceGC(RedGui::CRedGuiEventPackage& eventPackage)
{
	SGarbageCollector::GetInstance().CollectNow();
}

void CDebugWindowMemoryObjects::NotifyEventSelectedSortType( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedIndex )
{
	m_sortMode = (ESortMode) selectedIndex;
	
	if( m_realtimeSort )
		m_classListView->SetColumnSortIndex( m_sortMode );

	// resort
	for ( ClassNode* classNode : m_rootNodes )
		classNode->ConditionalRefresh( true, false, m_sortMode );
}

} // DebugWindows

#endif
#endif