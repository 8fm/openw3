/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "toolsPanel.h"
#include "undoToolSwitch.h"
#include "../../common/core/garbageCollector.h"

// undefine this if weidness occurs with tools - if enough time passes
// without related bugs since 19/11/2012, remove this, the #ifndefs below
// and the whole CUndoToolSwitch class...                      -badsector
#define NO_UNDO_TOOLSWITCH

wxIMPLEMENT_CLASS( CEdToolsPanel, wxPanel );

//RED_DEFINE_STATIC_NAME( ActiveWorldChanging )

CEdToolsPanel::ToolInfo::ToolInfo( CClass* toolClass, wxWindow* parent, wxSizer* sizer, Uint32 index )
	: m_toolClass( toolClass )
{
	// Create tool name
	IEditorTool* tool = toolClass->GetDefaultObject<IEditorTool>();
	String toolName = tool->GetCaption();

	// Create button
	m_button = new CEdToggleButton( parent, wxDefaultPosition, wxSize( 50, 21 ), toolName.AsChar() );
	m_button->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( CEdToolsPanel::OnSelectTool ), new CEdToolsPanel::ToolInfoWrapper( this ), parent );
	sizer->Add( m_button, 1, wxALL | wxEXPAND, 2 );
}

CEdToolsPanel::ToolInfo::~ToolInfo()
{
}

CEdToolsPanel::CEdToolsPanel( wxWindow* parent, CEdRenderingPanel* viewport )
	: wxPanel( parent, wxID_ANY )
	, m_viewport( viewport )
	, m_tool( NULL )
	, m_panel( NULL )
	, m_buttonSizer( NULL )
	, m_panelSizer( NULL )
{
	// Global box sizer
	wxBoxSizer* boxSizer;
	boxSizer = new wxBoxSizer( wxVERTICAL );

	// Grid sizer
	m_buttonSizer = new wxGridSizer( 0, 2, 0, 0 );
	//m_buttonSizer->SetFlexibleDirection( wxHORIZONTAL );
	//m_buttonSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	// Enumerate tools
	TDynArray< CClass* > editorToolsClasses;
	SRTTI::GetInstance().EnumClasses( ClassID< IEditorTool >(), editorToolsClasses );

	Sort( editorToolsClasses.Begin(), editorToolsClasses.End(), 
		[]( const CClass *elem1, const CClass *elem2 ) { return elem1->GetName().AsString() < elem2->GetName().AsString(); }
		);

	for ( Uint32 i=0; i<editorToolsClasses.Size(); i++ )
	{
		CClass* toolClass = editorToolsClasses[i];
		IEditorTool * tool = toolClass->GetDefaultObject<IEditorTool>();
		
		// Skip tools improper outside the main world editor
		if ( tool->UsableInActiveWorldOnly() && wxTheFrame->GetSolutionBar() != parent )
		{
			continue;
		}

		if ( tool->IsPersistent() )
		{
			continue;
		}

		m_tools.PushBack( new ToolInfo( toolClass, this, m_buttonSizer, i ) );	
	}

	// Add grid sizer to top level box sizer
	boxSizer->Add( m_buttonSizer, 0, wxEXPAND, 5 );

	// Create panel for custom window
	m_panel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	boxSizer->Add( m_panel, 1, wxEXPAND | wxALL, 5 );

	m_panelSizer = new wxBoxSizer( wxVERTICAL );
	m_panel->SetSizer( m_panelSizer );

	// Register listener
	SEvents::GetInstance().RegisterListener( CNAME( ActiveWorldChanging ), this );
	SEvents::GetInstance().RegisterListener( CNAME( GameEnding ), this );

	// Update layout
	SetSizer( boxSizer );
	Layout();
	LoadOptionsFromConfig();
}

CEdToolsPanel::~CEdToolsPanel()
{
	SaveOptionsToConfig();
	// Unregister listener
	SEvents::GetInstance().UnregisterListener( this );
	// Deleting tool info
	TDynArray< ToolInfo * > :: iterator tool;
	for ( tool = m_tools.Begin(); tool != m_tools.End(); tool++ )
	{
		delete *tool;
	}
}

void CEdToolsPanel::ConnectAccelerators()
{
    /*TEdShortcutArray shortcuts; 
    shortcuts = GetAccelerators();
    CEdShortcutsEditor::Load( * wxTheFrame, shortcuts, TXT("Tools") );

    for ( Uint32 i = 0; i < shortcuts.Size(); ++i )
	{
		SEdShortcut & shortcut = shortcuts[i];
						
		wxTheFrame->Connect( shortcut.m_acceleratorEntry.GetCommand(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdToolsPanel::OnSelectTool ),
			new CEdToolsPanel::ToolInfoWrapper( m_tools[i] ), this );
	}*/

    TEdShortcutArray shortcuts; 
    TDynArray< ToolInfo * > toolInfos;

	TDynArray< ToolInfo * > :: iterator toolIP;
    int i = 0;
	for ( toolIP = m_tools.Begin(); toolIP != m_tools.End(); ++toolIP )
	{
        IEditorTool* tool = (**toolIP).m_toolClass->GetDefaultObject<IEditorTool>();
        CEdToggleButton* button = (**toolIP).m_button;

        if ( !tool || !button )
        {
            continue;
        }

        Int32 flags = 0;
        Int32 key = 0;
        tool->GetDefaultAccelerator( flags, key );
        shortcuts.PushBack( SEdShortcut(wxString("Tools\\") + tool->GetCaption().AsChar(), wxAcceleratorEntry( flags, key, button->GetId() ) ) );
        toolInfos.PushBack( *toolIP );
        ++i;
    }

    CEdShortcutsEditor::Load( * wxTheFrame, shortcuts, TXT("Tools") );

    ASSERT( shortcuts.Size() == toolInfos.Size() );

    for ( Uint32 i = 0; i < shortcuts.Size(); ++i )
	{
		SEdShortcut & shortcut = shortcuts[i];
						
		wxTheFrame->Connect( shortcut.m_acceleratorEntry.GetCommand(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdToolsPanel::OnSelectTool ),
			new CEdToolsPanel::ToolInfoWrapper( toolInfos[i] ), this );
	}
}

TEdShortcutArray CEdToolsPanel::GetAccelerators()
{
    TEdShortcutArray shortcuts;

	TDynArray< ToolInfo * > :: iterator toolIP;
    int i = 0;
	for ( toolIP = m_tools.Begin(); toolIP != m_tools.End(); ++toolIP )
	{
        IEditorTool* tool = (**toolIP).m_toolClass->GetDefaultObject<IEditorTool>();
        CEdToggleButton* button = (**toolIP).m_button;

        if ( !tool || !button )
        {
            continue;
        }

        Int32 flags = 0;
        Int32 key = 0;
        tool->GetDefaultAccelerator( flags, key );
        shortcuts.PushBack(SEdShortcut(wxString("Tools\\") + tool->GetCaption().AsChar(), wxAcceleratorEntry( flags, key, button->GetId() ) ) );
        ++i;
    }

    return shortcuts;
}

TEdShortcutArray CEdToolsPanel::GetAllAccelerators()
{
	TEdShortcutArray shortcuts;

	TDynArray< ToolInfo * > :: iterator toolIP;
	for ( toolIP = m_tools.Begin(); toolIP != m_tools.End(); ++toolIP )
	{
		IEditorTool* tool = (**toolIP).m_toolClass->GetDefaultObject<IEditorTool>();
		TEdShortcutArray * toolShortcuts = tool->GetAccelerators();
		if ( toolShortcuts )
			shortcuts.PushBack( *toolShortcuts );
	}

	return shortcuts;
}

void CEdToolsPanel::OnAccelerator( wxCommandEvent& event )
{
	if  ( m_tool )
		m_tool->OnAccelerator( event );
}

void CEdToolsPanel::SaveOptionsToConfig()
{
}

void CEdToolsPanel::LoadOptionsFromConfig()
{
	ISavableToConfig::RestoreSession();

    ConnectAccelerators();
}

void CEdToolsPanel::SaveSession( CConfigurationManager &config )
{
	Int32 i=0;
	for( TDynArray< ToolInfo * > :: iterator tool = m_tools.Begin(); tool != m_tools.End(); tool++, i++ )
		if( ( *tool )->m_button->IsToggled() )
		{
			config.Write( TXT("Tools/Current"), i );
			return;
		}
	config.Write( TXT("Tools/Current"), -1 );
}

void CEdToolsPanel::RestoreSession( CConfigurationManager &config )
{
	Int32 i = config.Read( TXT("Tools/Current"), -1 ), j=0;
	if( i < 1 )
		CancelTool();
	else
	{
		for( TDynArray< ToolInfo * > :: iterator tool = m_tools.Begin(); tool != m_tools.End(); tool++, j++ )
			if( i == j )
			{
				CancelTool();

				// Open new tool
				StartTool( ( *tool )->m_toolClass );
				break;
			}
	}
}

void CEdToolsPanel::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( ActiveWorldChanging ) || name == CNAME ( GameEnding ) )
	{
		CancelTool();
	}
}

IEditorTool* CEdToolsPanel::StartTool( CClass* toolClass )
{
	// Cancel current edit mode
	CancelTool();

	// Start new mode
	CWorld* world = m_viewport->GetWorld();
	if ( world )
	{
		// Create tool
		IEditorTool* tool = CreateObject< IEditorTool >( toolClass );
		if ( tool )
		{
            // Capture selection
			m_originalSelectionMode = world->GetSelectionManager()->GetGranularity();
			ASSERT( m_originalSelectionComp.Empty() );
			world->GetSelectionManager()->GetSelectedComponentsFiltered( ClassID<CComponent>(), m_originalSelectionComp );
			ASSERT( m_originalSelectionEnt.Empty() );
			world->GetSelectionManager()->GetSelectedEntities( m_originalSelectionEnt );

			// Start
			if ( tool->Start( m_viewport, world, m_panelSizer, m_panel, m_originalSelectionComp ) )
			{
				world->GetSelectionManager()->DeselectAll();

				tool->AddToRootSet();
				m_viewport->SetTool( tool );
				m_tool = tool;

				// Push button
				for ( Uint32 i=0; i<m_tools.Size(); i++ )
				{
					ToolInfo* info = m_tools[i];
					if ( info->m_toolClass == toolClass )
					{
						info->m_button->SetToggle( true );
						break;
					}
				}

				TEdShortcutArray * shortcuts = m_tool->GetAccelerators();
				if ( shortcuts )
				{
					m_prevTable = *m_viewport->GetAcceleratorTable();
					CEdShortcutsEditor::Load( * m_viewport, *shortcuts, TXT("Tools") );
					for ( Uint32 i = 0; i < shortcuts->Size(); ++i )
					{
						SEdShortcut & shortcut = (*shortcuts)[i];
						
						m_viewport->Connect( shortcut.m_acceleratorEntry.GetCommand(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdToolsPanel::OnAccelerator ),
							NULL, this );
					}
				}

#ifndef NO_UNDO_TOOLSWITCH
				if ( m_viewport->GetUndoManager() )
				{
					CUndoToolSwitch::CreateStep( *m_viewport->GetUndoManager(), toolClass, this, true );
				}
#endif

				// 
				m_panel->Layout();
			}
			else
			{
				// Restore selection
				RestoreSelection();
			}
		}
	}

	// Return flag
	return m_tool;
}

void CEdToolsPanel::CancelTool()
{
	if ( m_tool )
	{
		CWorld* world = m_viewport->GetWorld();
		
		// Clear shortcuts
		TEdShortcutArray * shortcuts = m_tool->GetAccelerators();
		if ( shortcuts )
		{
			for ( Uint32 i = 0; i < shortcuts->Size(); ++i )
			{
				SEdShortcut & shortcut = (*shortcuts)[i];

				m_viewport->Disconnect( shortcut.m_acceleratorEntry.GetCommand(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdToolsPanel::OnAccelerator ),
					NULL, this );
			}

			m_viewport->SetAcceleratorTable( m_prevTable );
		}

		// Pop button
		for ( Uint32 i=0; i<m_tools.Size(); i++ )
		{
			ToolInfo* info = m_tools[i];
			if ( info->m_button->IsToggled() )
			{
				ASSERT( m_tool->GetClass() == info->m_toolClass );
				info->m_button->SetToggle( false );
			}
		}

		// End tool
		m_tool->End();
		m_tool->RemoveFromRootSet();

		// Close editor panel
		m_panelSizer->Clear( true );

#ifndef NO_UNDO_TOOLSWITCH
		if ( m_viewport->GetUndoManager() )
		{
			CUndoToolSwitch::CreateStep( *m_viewport->GetUndoManager(), m_tool->GetClass(), this, false );
		}
#endif

		// Reset tool pointers
		m_viewport->SetTool( NULL );
		m_tool = NULL;

		// Collect tool garbage
		SGarbageCollector::GetInstance().CollectNow();

		// Restore selection
		RestoreSelection();
	}
}

void CEdToolsPanel::RestoreSelection()
{
	CWorld* world = m_viewport->GetWorld();
	if ( world )
	{
		CSelectionManager::CSelectionTransaction transaction(*world->GetSelectionManager());
		world->GetSelectionManager()->DeselectAll();
		world->GetSelectionManager()->SetGranularity( m_originalSelectionMode );

		if ( m_originalSelectionMode == CSelectionManager::SG_Entities )
		{
			for ( Uint32 i=0; i<m_originalSelectionEnt.Size(); i++ )
			{
				world->GetSelectionManager()->Select( m_originalSelectionEnt[i] );
			}
		}
		else
		{
			for ( Uint32 i=0; i<m_originalSelectionComp.Size(); i++ )
			{
				world->GetSelectionManager()->Select( m_originalSelectionComp[i] );
			}
		}
	}

	// Cleanup selection list
	m_originalSelectionComp.Clear();
	m_originalSelectionEnt.Clear();
}

void CEdToolsPanel::OnSelectTool( wxCommandEvent& event )
{
	// Get clicked tool
	ToolInfoWrapper* wrapper = ( ToolInfoWrapper* ) event.m_callbackUserData;
	ToolInfo* tool = wrapper->m_tool;

	// Close active tool
	if ( tool->m_button->IsToggled() )
	{
		// Cancel current tool
		CancelTool();
	}
	else
	{
		// Cancel current tool
		CancelTool();

		// Open new tool
		StartTool( tool->m_toolClass );
	}
}
