/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeEditor.h"

#include "../../common/game/behTree.h"
#include "../../common/game/behTreeMachine.h"
#include "../../common/game/behTreeNode.h"
#include "../../common/game/behTreeInstance.h"
#include "../../common/core/diskFile.h"

#include "shortcutsEditor.h"
#include "assetBrowser.h"
#include "undoManager.h"


// Event table
BEGIN_EVENT_TABLE( CEdBehTreeEditor, wxSmartLayoutPanel )
	EVT_MENU( XRCID( "editUndo" ), CEdBehTreeEditor::OnEditUndo )
	EVT_MENU( XRCID( "editRedo" ), CEdBehTreeEditor::OnEditRedo )
	EVT_MENU( XRCID( "editDelete" ), CEdBehTreeEditor::OnEditDelete )
	EVT_MENU( XRCID( "templateSave" ), CEdBehTreeEditor::OnSave )
	EVT_MENU( XRCID( "viewZoomAll" ), CEdBehTreeEditor::OnZoomAll )
END_EVENT_TABLE()

TDynArray< CEdBehTreeEditor * > CEdBehTreeEditor::s_editors;
THandle< CBehTreeMachine > CEdBehTreeEditor::s_activeMachine;

CEdBehTreeEditor::CEdBehTreeEditor( wxWindow* parent, IBehTreeEditedItem* tree )
	: wxSmartLayoutPanel( parent, TXT("BehTreeEditor"), false )
	, m_editedItem( tree )
	, m_graphEditor( NULL )
{
	GetOriginalFrame()->Bind( wxEVT_ACTIVATE, &CEdBehTreeEditor::OnActivate, this );

	// Load icons
	m_btIcon = SEdResources::GetInstance().LoadBitmap( TXT("IMG_VEGETATION") );

	// Set icon
	wxIcon iconSmall;
	iconSmall.CopyFromBitmap( m_btIcon );
	SetIcon( iconSmall );

	// undo manager
	{
		m_undoManager = new CEdUndoManager( this );
		m_undoManager->AddToRootSet();
		m_undoManager->SetMenuItems( GetMenuBar()->FindItem( XRCID( "editUndo" ) ), GetMenuBar()->FindItem( XRCID( "editRedo" ) ) );
	}

	// Create properties
	{
		wxPanel* rp = XRCCTRL( *this, "PropertiesPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );

		PropertiesPageSettings settings;
		m_properties = new CEdPropertiesBrowserWithStatusbar( rp, settings, m_undoManager );
		m_properties->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdBehTreeEditor::OnPropertiesChanged ), NULL, this );
		m_properties->Get().Connect( wxEVT_COMMAND_REFRESH_PROPERTIES, wxCommandEventHandler( CEdBehTreeEditor::OnPropertiesRefresh ), NULL, this );
		sizer1->Add( m_properties, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	// Graph editor
	{
		wxPanel* rp = XRCCTRL( *this, "InnerPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		m_graphEditor = new CEdBehTreeGraphEditor( rp, this );
		sizer1->Add( m_graphEditor , 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
		m_graphEditor->SetEditor( this );
		m_graphEditor->SetUndoManager( m_undoManager );
	}

	// Update and finalize layout
	Layout();
	Show();

	// Set graph
	m_graphEditor->SetTree( m_editedItem );

	// Set title for newly created window
	wxString windowTitle;
	m_editedItem->GetName( windowTitle, false );
	SetTitle( windowTitle );

	SEvents::GetInstance().RegisterListener( CNAME( FileReload ), this );
	SEvents::GetInstance().RegisterListener( CNAME( FileReloadConfirm ), this );

	OnGraphSelectionChanged();

#ifdef EDITOR_AI_DEBUG
	CBehTreeMachine* activeMachine = s_activeMachine.Get();
	if( activeMachine )
	{
		DebugBehTreeStart( activeMachine );
	}
#endif	//EDITOR_AI_DEBUG

	s_editors.PushBackUnique( this );
}

CEdBehTreeEditor::~CEdBehTreeEditor()
{
	if ( m_graphEditor )
	{
		delete m_graphEditor;
		m_graphEditor = NULL;
	}
	if( m_machine.Get() )
	{
		m_machine.Get()->RemoveListener( this );
	}

	s_editors.Remove( this );
	if( s_editors.Size() == 0 )
	{
		s_activeMachine = NULL;
	}

	SaveOptionsToConfig();

	if ( m_editedItem )
	{
		// Also removes resource reference
		delete m_editedItem;
		m_editedItem = NULL;
	}

	{
		m_undoManager->RemoveFromRootSet();
		m_undoManager->Discard();
		m_undoManager = NULL;
	}	
	
	// Unregister listener
	SEvents::GetInstance().UnregisterListener( this );

}

wxString CEdBehTreeEditor::GetShortTitle()
{
	wxString shortName;
	m_editedItem->GetName( shortName, true );
	return shortName;
}

void CEdBehTreeEditor::OnGraphSelectionChanged()
{
	// Get selected blocks
	const TDynArray< IScriptable* > & selection = m_graphEditor->GetSelectedObjects();

	// if nothing is selected, show properties of the whole graph
	if ( selection.Empty() )
	{
		TDynArray< IScriptable* > selection( 1 );
		selection[ 0 ] = m_editedItem->GetParentObject();
		m_properties->Get().SetObjects( selection );
	}
	else
	{
		m_properties->Get().SetObjects( selection );
	}
}

void CEdBehTreeEditor::OnPropertiesChanged( wxCommandEvent& event )
{
	m_graphEditor->ForceLayoutUpdate();
	m_graphEditor->Repaint();
}

void CEdBehTreeEditor::OnPropertiesRefresh( wxCommandEvent& event )
{
	OnGraphSelectionChanged();
}

void CEdBehTreeEditor::OnEditUndo( wxCommandEvent& event )
{
	m_undoManager->Undo();
}

void CEdBehTreeEditor::OnEditRedo( wxCommandEvent& event )
{
	m_undoManager->Redo();
}

void CEdBehTreeEditor::OnEditDelete( wxCommandEvent& event )
{
	m_graphEditor->DeleteSelection();
}

void CEdBehTreeEditor::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( FileReloadConfirm ) )
	{
		CBehTree* behTree = m_editedItem->GetRes();
		if ( behTree )
		{
			CResource* res = GetEventData< CResource* >( data );
			if ( res == behTree )
			{
				SEvents::GetInstance().QueueEvent( CNAME( FileReloadToConfirm ), CreateEventData( CReloadFileInfo( res, NULL, GetTitle().wc_str() ) ) );
			}
		}
	}
	else if ( name == CNAME( FileReload ) )
	{
		const CReloadFileInfo& reloadInfo = GetEventData< CReloadFileInfo >( data );
		CBehTree* behTree = m_editedItem->GetRes();
		if ( behTree )
		{		
			if ( reloadInfo.m_newResource->IsA< CBehTree >() )
			{
				CBehTree* oldTemplate = SafeCast< CBehTree >( reloadInfo.m_oldResource );
				CBehTree* newTemplate = SafeCast< CBehTree >( reloadInfo.m_newResource );
				if ( oldTemplate == behTree )
				{
					YesNo( TXT("TODO!") );
					//m_template = newTemplate;
					//m_template->AddToRootSet();

					//m_graphEditor->SetTree( m_template );

					//OnGraphSelectionChanged();

					//wxTheFrame->GetAssetBrowser()->OnEditorReload(newTemplate, this);
				}
			}
		}
	}
}

void CEdBehTreeEditor::OnSave( wxCommandEvent& event )
{
	IBehTreeNodeDefinition* node = m_editedItem->GetRootNode();
	if ( node )
	{
#ifdef BEHREE_AUTOFIX_TREE_PARENTAGE
		if ( m_editedItem->GetRes() && m_editedItem->GetRes() != node->GetParent() )
		{
			if ( !YesNo( TXT( "Current graph is unusable. Root node had invalid parentage setup. I'll try to fix the problem, right?" ) ) )
			{
				return;
			}
			m_editedItem->GetRes()->FixTreeParentage();
		}
#endif

		if ( !node->IsValid() )
		{
			if ( !YesNo( TXT( "Current graph is unusable. Using this AI template in-game will result in unexpected behavior (crash likely).\n Do you really want to save it?" ) ) )
			{
				return;
			}
		}
	}

	m_graphEditor->PreSave();
	m_editedItem->Save();
	m_graphEditor->PostSave();

	m_properties->Get().RefreshValues();	
}

void CEdBehTreeEditor::OnZoomAll( wxCommandEvent& event )
{
	m_graphEditor->ZoomExtents();
}

Bool CEdBehTreeEditor::OnNodeReport( const IBehTreeNodeInstance* node )
{
#ifdef EDITOR_AI_DEBUG
	if( node && m_graphEditor )
	{
		IBehTreeNodeDefinition* nodeDef = m_editedItem->GetRes()->GetNodeByHash( node->GetDefinitionId() );
		if( nodeDef )
		{
			// passing node->IsActive() here is important to make sure colors are set on first debug query
			return m_graphEditor->OnNodeReport( nodeDef, node->IsActive() );
		}
	}

#endif
	return false;
}

Bool CEdBehTreeEditor::OnNodeResultChanged( const IBehTreeNodeInstance* node, const Bool active )
{
#ifdef EDITOR_AI_DEBUG
	if( node && m_graphEditor )
	{
		// Get matching node definition
		IBehTreeNodeDefinition* nodeDef = m_editedItem->GetRes()->GetNodeByHash( node->GetDefinitionId() );
		if( nodeDef )
		{
			return m_graphEditor->OnNodeResultChanged( nodeDef, active );
		}
	}
#endif

	return false;
}

void CEdBehTreeEditor::OnForceDebugColorSet( const IBehTreeNodeInstance* node, Uint8 R, Uint8 G, Uint8 B )
{
#ifdef EDITOR_AI_DEBUG
	if( node && m_graphEditor )
	{
		// Get matching node definition
		IBehTreeNodeDefinition* nodeDef = m_editedItem->GetRes()->GetNodeByHash( node->GetDefinitionId() );
		if( nodeDef )
		{
			return m_graphEditor->ForceSetDebuggerColor( nodeDef, R, G, B );
		}
	}
#endif
}

void CEdBehTreeEditor::OnTreeStateChanged()
{
	if( m_graphEditor )
	{
		m_graphEditor->OnTreeStateChanged();
	}
}

void CEdBehTreeEditor::OnStopListeningRequest()
{
	if( m_machine.Get() )
	{
		m_machine.Get()->RemoveListener( this );
	}
	m_machine = NULL;

	// Reset temporarily assigned res for template subtrees
	if( m_editedItem->GetRes()->GetRootNode() != m_editedItem->GetRootNode() )
	{
		m_editedItem->SetRes( NULL );
	}

	m_graphEditor->ForceLayoutUpdate();
	m_graphEditor->Repaint();
}

Bool CEdBehTreeEditor::OnBreakpoint( const IBehTreeNodeInstance* node )
{
#ifdef EDITOR_AI_DEBUG
	if( node && m_graphEditor )
	{
		// Get matching node definition
		IBehTreeNodeDefinition* nodeDef = m_editedItem->GetRes()->GetNodeByHash( node->GetDefinitionId() );
		if( nodeDef )
		{
			if( m_graphEditor->OnBreakpoint( nodeDef ) )
			{
				return m_machine.Get()->ProcessBreakpoint( nodeDef );
			}
		}
	}
#endif

	return false;
}

void CEdBehTreeEditor::OnActivate( wxActivateEvent& event )
{
	if ( event.GetActive() )
	{
		if ( wxTheFrame )
		{
			wxTheFrame->SetUndoHistoryFrameManager( m_undoManager, String( GetTitle().c_str() ) );
		}
	}

	event.Skip();
}

#ifdef EDITOR_AI_DEBUG

void CEdBehTreeEditor::DebugBehTreeStart( CBehTreeMachine* machine )
{
	if( machine )
	{
		if( m_machine.Get() )
		{
			m_machine.Get()->RemoveListener( this );
			m_machine = NULL;
		}

		// Missing res for template subtrees, fill in with parent res
		if( !m_editedItem->GetRes() )
		{
			for( Uint32 i=0; i < s_editors.Size(); ++i )
			{
				CBehTree* res = s_editors[i]->m_editedItem->GetRes();
				if( res && res->GetNodeByHash( m_editedItem->GetRootNode()->GetUniqueId() ))
				{
					m_editedItem = new CBehTreeEditedResource( res );
					break;
				}
			}
		}

		m_machine = machine;
		machine->AddListener( this );

		m_graphEditor->ForceLayoutUpdate();
		m_graphEditor->Repaint();

		// Node report
		machine->GetBehTreeInstance()->GetInstanceRootNode()->DebugReport();
		machine->GetBehTreeInstance()->GetInstanceRootNode()->DebugUpdate(true);
	}
}

////////////////////////////////////////////////////////////////////////////

extern IBehTreeDebugInterface* GBehTreeDebugInterface;
static CEdBehTreeDebugInterface GBTDebugInterface;

CEdBehTreeDebugInterface::CEdBehTreeDebugInterface()
{
	GBehTreeDebugInterface = this;
}

CEdBehTreeDebugInterface::~CEdBehTreeDebugInterface()
{
	GBehTreeDebugInterface = NULL;
}

void CEdBehTreeDebugInterface::DebugBehTreeStart( CBehTreeMachine* machine )
{
	if( !machine )
	{
		ERR_EDITOR( TXT("DebugBehTree NULL machine" ) );
		return;
	}

	if( !CEdBehTreeEditor::s_activeMachine.Get() )
	{
		CEdBehTreeEditor::s_activeMachine = machine;
	}

	for( Uint32 i=0; i < CEdBehTreeEditor::s_editors.Size(); ++i )
	{
		CEdBehTreeEditor* editor = CEdBehTreeEditor::s_editors[i];
		if( editor )
		{
			editor->DebugBehTreeStart( machine );
		}
	}
}

void CEdBehTreeDebugInterface::DebugBehTreeStopAll()
{
	for( Uint32 i=0; i<CEdBehTreeEditor::s_editors.Size(); i++ )
	{
		CEdBehTreeEditor * editor = CEdBehTreeEditor::s_editors[i];
		editor->OnStopListeningRequest();
	}
}

#endif //EDITOR_AI_DEBUG
