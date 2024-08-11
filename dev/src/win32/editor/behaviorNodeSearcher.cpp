/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/engine/behaviorGraphContainerNode.h"
#include "../../common/engine/behaviorGraphTransitionNode.h"
#include "../../common/engine/behaviorGraphVariableNode.h"
#include "../../common/engine/behaviorGraphVectorVariableNode.h"
#include "../../common/engine/behaviorGraphTopLevelNode.h"

#include "behaviorEditor.h"
#include "behaviorNodeSearcher.h"

BEGIN_EVENT_TABLE( CEdBehaviorGraphNodeSearcher, CEdBehaviorEditorSimplePanel )
	EVT_BUTTON( XRCID( "buttSearch" ), CEdBehaviorGraphNodeSearcher::OnSearch )
	EVT_TOGGLEBUTTON( XRCID( "buttTransEx" ), CEdBehaviorGraphNodeSearcher::OnToggleTransEx )
	EVT_TOGGLEBUTTON( XRCID( "buttTransContain" ), CEdBehaviorGraphNodeSearcher::OnToggleTransContain )
	EVT_TOGGLEBUTTON( XRCID( "treeList" ), CEdBehaviorGraphNodeSearcher::OnToggleTreeList )
	EVT_TOGGLEBUTTON( XRCID( "treeTree" ), CEdBehaviorGraphNodeSearcher::OnToggleTreeTree )
	EVT_TREE_SEL_CHANGED( XRCID( "tree" ), CEdBehaviorGraphNodeSearcher::OnTreeSelectionChanged )
	EVT_TREE_ITEM_ACTIVATED( XRCID( "tree" ), CEdBehaviorGraphNodeSearcher::OnTreeItemActivated )
END_EVENT_TABLE()

const Float CEdBehaviorGraphNodeSearcher::PAINT_TIMER_DURATION = 0.5f;

CEdBehaviorGraphNodeSearcher::CEdBehaviorGraphNodeSearcher( CEdBehaviorEditor* editor )
	: CEdBehaviorEditorSimplePanel( editor )
	, m_paintTimer( 0.f )
	, m_paintTimerSign( 1.f )
{
	// Load designed frame from resource
	wxPanel* innerPanel = wxXmlResource::Get()->LoadPanel( this, wxT("BehaviorEditorSearcher") );
	SetMinSize( innerPanel->GetSize() );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( innerPanel, 1, wxEXPAND|wxALL, 0 );

	m_tree = XRCCTRL( *this, "tree", wxTreeCtrl );
	
	m_nameFilter = XRCCTRL( *this, "editNames", wxTextCtrl );
	m_classFilter = XRCCTRL( *this, "classChoice", wxChoice );
	m_transFilter = XRCCTRL( *this, "transChoice", wxChoice );
	m_transVarFilter = XRCCTRL( *this, "transVarChoice", wxChoice );
	m_transEventFilter = XRCCTRL( *this, "transEventChoice", wxChoice );
	m_floatFilter = XRCCTRL( *this, "floatChoice", wxChoice );
	m_vectorFilter = XRCCTRL( *this, "vectorChoice", wxChoice );
	m_animFilter = XRCCTRL( *this, "editAnim", wxTextCtrl );

	FillClassFilter();
	FillTransFilter();

	SetSizer( sizer );	
	Layout();

	// by default: list, zoom, mark
	XRCCTRL( *this, "treeTree", wxToggleButton )->SetValue( false );
	XRCCTRL( *this, "treeList", wxToggleButton )->SetValue( true );
	XRCCTRL( *this, "buttZoom", wxToggleButton )->SetValue( true );
	XRCCTRL( *this, "buttMark", wxToggleButton )->SetValue( true );
}

wxAuiPaneInfo CEdBehaviorGraphNodeSearcher::GetPaneInfo() const
{
	wxAuiPaneInfo info = CEdBehaviorEditorPanel::GetPaneInfo();

	info.Floatable( true ).Float().MinSize( GetMinSize() ).Dockable( false );

	return info;
}

void CEdBehaviorGraphNodeSearcher::OnSearch( wxCommandEvent& event )
{
	m_selectedNodes.Clear();

	FillTree();
}

wxString CEdBehaviorGraphNodeSearcher::GetNodeName( const CBehaviorGraphNode* node ) const
{
	ASSERT( node );

	wxString str;

	if ( !node->GetName().Empty() )
	{
		str += node->GetName().AsChar();
		str += wxT(" - ");
		str += node->GetClass()->GetName().AsString().AsChar();
	}
	else
	{
		str += node->GetClass()->GetName().AsString().AsChar();
	}

	return str;
}

void CEdBehaviorGraphNodeSearcher::FillClassFilter()
{
	TDynArray< CClass* > classes;
	SRTTI::GetInstance().EnumClasses( ClassID<CBehaviorGraphNode>(), classes );

	for ( Uint32 i=0; i<classes.Size(); ++i )
	{
		CClass* iClass = classes[i];

		if ( !iClass->IsAbstract() )
		{
			m_classFilter->AppendString( iClass->GetName().AsString().AsChar() );
		}
	}
}

void CEdBehaviorGraphNodeSearcher::FillTransFilter()
{
	TDynArray< CClass* > classes;
	SRTTI::GetInstance().EnumClasses( ClassID<IBehaviorStateTransitionCondition>(), classes );

	for ( Uint32 i=0; i<classes.Size(); ++i )
	{
		CClass* iClass = classes[i];

		if ( !iClass->IsAbstract() )
		{
			m_transFilter->AppendString( iClass->GetName().AsString().AsChar() );
		}
	}
}

void CEdBehaviorGraphNodeSearcher::FillTransVarFilter()
{
	m_transVarFilter->Freeze();
	m_transVarFilter->Clear();

	TDynArray< CBehaviorVariable* > vars;
	GetBehaviorGraph()->GetVariables().GetVariables( vars );
	
	for ( Uint32 i=0; i<vars.Size(); ++i )
	{
		m_transVarFilter->AppendString( vars[i]->GetName().AsChar() );
	}

	m_transVarFilter->Thaw();
	m_transVarFilter->Refresh();
}

void CEdBehaviorGraphNodeSearcher::FillTransEventFilter()
{
	m_transEventFilter->Freeze();
	m_transEventFilter->Clear();

	Uint32 eventNum = GetBehaviorGraph()->GetEvents().GetNumEvents();

	for ( Uint32 i=0; i<eventNum; ++i )
	{
		m_transEventFilter->AppendString( GetBehaviorGraph()->GetEvents().GetEventName( i ).AsString().AsChar() );
	}

	m_transEventFilter->Thaw();
	m_transEventFilter->Refresh();
}

void CEdBehaviorGraphNodeSearcher::FillFloatFilter()
{
	m_floatFilter->Freeze();
	m_floatFilter->Clear();

	auto vars = GetBehaviorGraph()->GetVariables().GetVariables();
	for ( auto it = vars.Begin(), end = vars.End(); it != end; ++it )
	{
		m_floatFilter->AppendString( it->m_first.AsChar() );
	}

	m_floatFilter->Thaw();
	m_floatFilter->Refresh();
}

void CEdBehaviorGraphNodeSearcher::FillVectorFilter()
{
	m_vectorFilter->Freeze();
	m_vectorFilter->Clear();

	auto vars = GetBehaviorGraph()->GetVectorVariables().GetVariables();
	for ( auto it = vars.Begin(), end = vars.End(); it != end; ++it )
	{
		m_vectorFilter->AppendString( it->m_first.AsChar() );
	}

	m_vectorFilter->Thaw();
	m_vectorFilter->Refresh();
}

void CEdBehaviorGraphNodeSearcher::FillTree()
{
	m_tree->Freeze();
	m_tree->DeleteAllItems();

	CBehaviorGraph* graph = GetBehaviorGraph();

	class CTreeProvider
	{
		wxTreeCtrl*		m_tree;
		wxTreeItemId	m_currRoot;

	public:
		TDynArray< CBehaviorGraphNode* > m_selectedNodes;

	public:
		CTreeProvider( wxTreeCtrl* tree ) 
			: m_tree( tree )
		{
		}

		Bool Fill( CEdBehaviorGraphNodeSearcher* searcher, CBehaviorGraphNode* node, Bool justTest = false ) 
		{
			if ( node )
			{
				wxTreeItemId currItem = m_currRoot;
				Bool passedAllFilters = true;
				Bool childrenPassedAllFilters = false;
				if ( node->IsA( CBehaviorGraphTopLevelNode::GetStaticClass() ) )
				{
					currItem = m_tree->AddRoot( wxT("Nodes") );
					m_tree->Expand( currItem );
					m_currRoot = currItem;
				}
				else
				{
					// go through children to find if any of them passed all filters, if so, we need to add this
					if ( node->IsA< CBehaviorGraphContainerNode >() && ! searcher->IsTreeListStyle() )
					{
						CBehaviorGraphContainerNode *containerNode = SafeCast< CBehaviorGraphContainerNode >( node );

						TDynArray< CGraphBlock* >& children = containerNode->GetConnectedChildren();

						for( Uint32 i=0; i<children.Size(); ++i )
						{
							if ( CBehaviorGraphNode* child = Cast< CBehaviorGraphNode >( children[i] ) )
							{
								childrenPassedAllFilters |= Fill( searcher, child, true );
							}
						}
					}

					// add only items that are not filtered out
					passedAllFilters = false;
					{
						passedAllFilters = true;
						if ( searcher->UseFilterActive() && ! node->IsActive( *(searcher->GetBehaviorGraphInstance()) ) )
						{
							passedAllFilters = false;
						}

						if ( searcher->UseFilterTransition() )
						{
							if ( CBehaviorGraphStateTransitionNode * trans = Cast< CBehaviorGraphStateTransitionNode >( node ) )
							{
								const IBehaviorStateTransitionCondition* condition = trans->GetTransitionCondition();
								
								CName transTypeFilterName( searcher->GetFilterTransType().wc_str() );
								String transVarFilerStr = searcher->GetFilterTransVar().wc_str();
								CName transEventFilterStr = CName( searcher->GetFilterTransEvent().wc_str() );

								Bool transHier = !searcher->UseFilterTransEx();

								if ( searcher->UseFilterTransType() && condition && condition->ContainClass( CName( searcher->GetFilterTransType().wc_str() ), transHier ) )
								{
									passedAllFilters = false;	
								}

								if ( searcher->UseFilterTransEvent() && condition &&! condition->UseEvent( CName( searcher->GetFilterTransEvent().wc_str() ), transHier ) )
								{
									passedAllFilters = false;
								}

								if ( searcher->UseFilterTransVar() && condition && !condition->UseVariable( searcher->GetFilterTransVar().wc_str(), transHier ) )
								{
									passedAllFilters = false;
								}
							}
							else
							{
								passedAllFilters = false;
							}
						}

						if ( searcher->UseFilterName() && ! node->GetName().ContainsSubstring( searcher->GetFilterName().wc_str() ))
						{
							passedAllFilters = false;
						}

						if ( searcher->UseFilterClass() && ! node->GetClass()->GetName().AsString().ContainsSubstring( searcher->GetFilterClass().wc_str() ) )
						{
							passedAllFilters = false;
						}

						if ( searcher->UseFilterFloat() )
						{
							bool filterPassed = false;
							String filter = searcher->GetFilterFloat().wc_str();

							TDynArray<CName> var;
							TDynArray<CName> vecVar;
							TDynArray<CName> events;
							TDynArray<CName> intVar;
							TDynArray<CName> intVecVar;
							node->GetUsedVariablesAndEvents(var, vecVar, events, intVar, intVecVar);
							for ( auto it = var.Begin(); it != var.End(); ++ it )
							{
								if ( it->AsString().ContainsSubstring( filter ) )
								{
									filterPassed = true;
									break;
								}
							}

							if ( ! filterPassed )
							{
								passedAllFilters = false;
							}
						}

						if ( searcher->UseFilterVector() )
						{
							bool filterPassed = false;
							String filter = searcher->GetFilterVector().wc_str();

							TDynArray<CName> var;
							TDynArray<CName> vecVar;
							TDynArray<CName> events;
							TDynArray<CName> intVar;
							TDynArray<CName> intVecVar;
							node->GetUsedVariablesAndEvents(var, vecVar, events, intVar, intVecVar);
							for ( auto it = vecVar.Begin(); it != vecVar.End(); ++ it )
							{
								if ( it->AsString().ContainsSubstring( filter ) )
								{
									filterPassed = true;
									break;
								}
							}

							if ( ! filterPassed )
							{
								passedAllFilters = false;
							}
						}

						if ( searcher->UseFilterAnim() )
						{
							bool filterPassed = false;
							String animName = searcher->GetFilterAnim().wc_str();

							TDynArray< CName > anims;
							node->CollectUsedAnimations(anims);
							for ( auto iAnim = anims.Begin(); iAnim != anims.End(); ++ iAnim )
							{
								if ( iAnim->AsString().ContainsSubstring( animName ) )
								{
									filterPassed = true;
									break;
								}
							}

							if ( ! filterPassed )
							{
								passedAllFilters = false;
							}
						}

						if ( passedAllFilters || childrenPassedAllFilters )
						{
							if ( ! justTest ) // this is just a test
							{
								wxString nodeName = searcher->GetNodeName( node );

								ASSERT ( m_currRoot.IsOk() );
								currItem = m_tree->AppendItem( m_currRoot, nodeName, -1, -1, new SBehaviorNodeData( node ) );
								m_tree->Expand( currItem );
								if ( passedAllFilters )
								{
									m_selectedNodes.PushBack( node );
								}
							}
						}
					}
				}

				// if not test, add everything as it goes
				if ( ! justTest && node->IsA< CBehaviorGraphContainerNode >() )
				{
					CBehaviorGraphContainerNode *containerNode = SafeCast< CBehaviorGraphContainerNode >( node );

					TDynArray< CGraphBlock* >& children = containerNode->GetConnectedChildren();

					for( Uint32 i=0; i<children.Size(); ++i )
					{
						if ( CBehaviorGraphNode* child = Cast< CBehaviorGraphNode >( children[i] ) )
						{
							if ( ! searcher->IsTreeListStyle() )
							{
								// make sure we'll be adding to this item
								m_currRoot = currItem;
							}
							Fill( searcher, child );
						}
					}
				}

				if ( currItem.IsOk() )
				{
					m_tree->Expand( currItem );
				}

				return passedAllFilters || childrenPassedAllFilters;
			}
			return false;
		}
	};

	{
		CTreeProvider provider( m_tree );
		provider.Fill( this, graph->GetRootNode() );
		m_selectedNodes = provider.m_selectedNodes;
		if ( m_selectedNodes.Size() > 0 )
		{
			// Only last node
			CBehaviorGraphNode* node = m_selectedNodes.Back();

			if ( ZoomNodes() )
			{
				GetEditor()->FocusOnBehaviorNode( node );
			}
			if ( MarkNodes() )
			{
				GetEditor()->GetGraphEditor()->DeselectAllBlocks();
				GetEditor()->GetGraphEditor()->SelectBlock( node, true );
			}
		}
	}

	m_tree->Thaw();
}

void CEdBehaviorGraphNodeSearcher::OnReset()
{
	FillTransVarFilter();
	FillTransEventFilter();
	FillFloatFilter();
	FillVectorFilter();

	Clear();
}

void CEdBehaviorGraphNodeSearcher::OnInstanceReload()
{
	Clear();
}

void CEdBehaviorGraphNodeSearcher::OnPanelClose()
{
	Clear();
}

void CEdBehaviorGraphNodeSearcher::Clear()
{
	m_tree->DeleteAllItems();
	m_selectedNodes.Clear();
}

void CEdBehaviorGraphNodeSearcher::OnTick( Float dt )
{
	m_paintTimer = Clamp( m_paintTimer + m_paintTimerSign * dt, 0.f, PAINT_TIMER_DURATION );

	if ( m_paintTimer >= PAINT_TIMER_DURATION || m_paintTimer <= 0.f )
	{
		m_paintTimerSign *= -1.f;
	}
}

void CEdBehaviorGraphNodeSearcher::OnPrintNodes( CEdGraphEditor* graphCanvas ) 
{
	for ( Uint32 i=0; i<m_selectedNodes.Size(); ++i )
	{
		CGraphBlock* block = m_selectedNodes[i];

		const CEdGraphEditor::BlockLayoutInfo* layout = graphCanvas->GetBlockLayout( block );

		if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
		{				
			Uint32 progress = Min< Uint32 >( ( Uint32 )( m_paintTimer / PAINT_TIMER_DURATION * 255.f + 128.f ), 255 );

			wxColour borderColor( 200, 230, 255 );
			wxColour interiorColor( 200, 230, 255, progress );

			wxPoint location( block->GetPosition().X, block->GetPosition().Y );
			wxPoint corner( location + layout->m_windowSize );

			graphCanvas->DrawTipRect( location.x, location.y, corner.x, corner.y, borderColor, interiorColor, 6 );
		}
	}
}

Bool CEdBehaviorGraphNodeSearcher::UseFilterName() const
{
	return XRCCTRL( *this, "useNames", wxToggleButton )->GetValue();
}

Bool CEdBehaviorGraphNodeSearcher::UseFilterTransition() const
{
	return UseFilterTransType() || UseFilterTransVar() || UseFilterTransEvent();
}

Bool CEdBehaviorGraphNodeSearcher::UseFilterClass() const
{
	return XRCCTRL( *this, "useClass", wxToggleButton )->GetValue();
}

Bool CEdBehaviorGraphNodeSearcher::UseFilterTransType() const
{
	return XRCCTRL( *this, "useTrans", wxToggleButton )->GetValue();
}

Bool CEdBehaviorGraphNodeSearcher::UseFilterTransEx() const
{
	return XRCCTRL( *this, "buttTransEx", wxToggleButton )->GetValue();
}

Bool CEdBehaviorGraphNodeSearcher::UseFilterTransContain() const
{
	return XRCCTRL( *this, "buttTransContain", wxToggleButton )->GetValue();
}

Bool CEdBehaviorGraphNodeSearcher::UseFilterTransVar() const
{
	return XRCCTRL( *this, "useTransVar", wxToggleButton )->GetValue();
}

Bool CEdBehaviorGraphNodeSearcher::UseFilterTransEvent() const
{
	return XRCCTRL( *this, "useTransEvent", wxToggleButton )->GetValue();
}

Bool CEdBehaviorGraphNodeSearcher::UseFilterAnim() const
{
	return XRCCTRL( *this, "useAnim", wxToggleButton )->GetValue();
}

Bool CEdBehaviorGraphNodeSearcher::MarkNodes() const
{
	return XRCCTRL( *this, "buttMark", wxToggleButton )->GetValue();
}

Bool CEdBehaviorGraphNodeSearcher::ZoomNodes() const
{
	return XRCCTRL( *this, "buttZoom", wxToggleButton )->GetValue();
}

Bool CEdBehaviorGraphNodeSearcher::UseFilterFloat() const
{
	return XRCCTRL( *this, "useFloat", wxToggleButton )->GetValue();
}

Bool CEdBehaviorGraphNodeSearcher::UseFilterVector() const
{
	return XRCCTRL( *this, "useVector", wxToggleButton )->GetValue();
}

Bool CEdBehaviorGraphNodeSearcher::IsTreeListStyle() const
{
	Bool tree = XRCCTRL( *this, "treeTree", wxToggleButton )->GetValue();
	Bool list = XRCCTRL( *this, "treeList", wxToggleButton )->GetValue();

	ASSERT( ( tree && !list ) || ( !tree && list ) );

	return list;
}

wxString CEdBehaviorGraphNodeSearcher::GetFilterName() const
{
	return m_nameFilter->GetValue();
}

wxString CEdBehaviorGraphNodeSearcher::GetFilterClass() const
{
	return m_classFilter->GetStringSelection();
}

wxString CEdBehaviorGraphNodeSearcher::GetFilterTransType() const
{
	return m_transFilter->GetStringSelection();
}

wxString CEdBehaviorGraphNodeSearcher::GetFilterTransVar() const
{
	return m_transVarFilter->GetStringSelection();
}

wxString CEdBehaviorGraphNodeSearcher::GetFilterTransEvent() const
{
	return m_transEventFilter->GetStringSelection();
}

wxString CEdBehaviorGraphNodeSearcher::GetFilterFloat() const
{
	return m_floatFilter->GetStringSelection();
}

wxString CEdBehaviorGraphNodeSearcher::GetFilterVector() const
{
	return m_vectorFilter->GetStringSelection();
}

wxString CEdBehaviorGraphNodeSearcher::GetFilterAnim() const
{
	return m_animFilter->GetValue();
}

Bool CEdBehaviorGraphNodeSearcher::UseFilterActive() const
{
	return XRCCTRL( *this, "onlyActive", wxToggleButton )->GetValue();
}

void CEdBehaviorGraphNodeSearcher::OnToggleTransEx( wxCommandEvent& event )
{
	XRCCTRL( *this, "buttTransContain", wxToggleButton )->SetValue( !event.IsChecked() );
}

void CEdBehaviorGraphNodeSearcher::OnToggleTransContain( wxCommandEvent& event )
{
	XRCCTRL( *this, "buttTransEx", wxToggleButton )->SetValue( !event.IsChecked() );
}

void CEdBehaviorGraphNodeSearcher::OnToggleTreeList( wxCommandEvent& event )
{
	XRCCTRL( *this, "treeTree", wxToggleButton )->SetValue( false );
	XRCCTRL( *this, "treeList", wxToggleButton )->SetValue( true );
}

void CEdBehaviorGraphNodeSearcher::OnToggleTreeTree( wxCommandEvent& event )
{
	XRCCTRL( *this, "treeTree", wxToggleButton )->SetValue( true );
	XRCCTRL( *this, "treeList", wxToggleButton )->SetValue( false );
}

void CEdBehaviorGraphNodeSearcher::OnTreeSelectionChanged( wxTreeEvent& event )
{
	wxTreeItemId item = event.GetItem();

	SBehaviorNodeData* data = static_cast< SBehaviorNodeData* >( m_tree->GetItemData( item ) );
		
	if ( data )
	{
		CBehaviorGraphNode* node = data->m_node;
		if ( ZoomNodes() )
		{
			GetEditor()->FocusOnBehaviorNode( node );
		}
		if ( MarkNodes() )
		{
			GetEditor()->GetGraphEditor()->DeselectAllBlocks();
			GetEditor()->GetGraphEditor()->SelectBlock(node, true);
		}
	}
}

void CEdBehaviorGraphNodeSearcher::OnTreeItemActivated( wxTreeEvent& event )
{

}
