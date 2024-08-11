/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "gridCustomCellEditors.h"
#include "attitudeEditor.h"
#include "../../common/core/depot.h"
#include "../../common/core/xmlFileWriter.h"
#include "choice.h"

namespace
{

const wxString NO_PARENT                = wxT( "None" );
const wxString GRID_CELL_ENABLED        = wxT( "1" );
const wxString GRID_CELL_DISABLED       = wxT( "" );
const wxString DEBUG_MODE_ON			= wxT( "Connected to running game" );
const wxString DEBUG_MODE_OFF			= wxT( "Not connected to game" );
const Uint32 GROUP_COLUMN               = 0;
const Uint32 ATTITUDE_COLUMN            = 1;
const Uint32 PARENT_COLUMN              = 2;
const Uint32 CUSTOM_ENTRY_COLUMN        = 3;
const Uint32 BLOCKED_BY_COLUMN	        = 4;
const Bool EMULATE_SINGLE_CLICK_EDIT    = true;

enum AttitudesSortType
{
	AST_None,
	AST_Name,
	AST_Attitude,
	AST_Parent,
	AST_IsCustom,
	AST_BlockedString,
};

class AttitudeEntries
{
	struct Entry
	{
		String		m_name;
		EAIAttitude	m_attitude;
		String		m_parent;
		Bool		m_isCustom;
		Bool		m_isConflict;
		String		m_blockedString;

		Entry()
			: m_name( String::EMPTY ), m_attitude( CAttitudes::GetDefaultAttitude() ), m_parent( String::EMPTY ), m_isCustom( false )
			, m_isConflict( false ), m_blockedString( String::EMPTY ) 
		{ }
		Entry( String name, EAIAttitude attitude, String parent, Bool isCustom, Bool isConflict, String blockedString )
			:	m_name( name ), m_attitude( attitude), m_parent( parent ), m_isCustom( isCustom )
			, m_isConflict( isConflict ), m_blockedString( blockedString )
		{}
	};

	TDynArray< Entry >	m_entries;
	
	static AttitudesSortType	s_sortType;
	static Bool					s_sortAscending;

	struct SortEntriesByName
	{
		Bool m_ascending;
		SortEntriesByName( Bool ascending ) : m_ascending( ascending ) { }
		Bool operator()( const Entry& x, const Entry& y ) const
		{
			return m_ascending ? ( x.m_name < y.m_name ) : ( y.m_name < x.m_name );
		}
	};

	struct SortEntriesByAttitude
	{
		Bool m_ascending;
		SortEntriesByAttitude( Bool ascending ) : m_ascending( ascending ) { }
		Bool operator()( const Entry& x, const Entry& y ) const
		{
			return m_ascending ? ( x.m_attitude < y.m_attitude ) : ( y.m_attitude < x.m_attitude );
		}
	};

	struct SortEntriesByParent
	{
		Bool m_ascending;
		SortEntriesByParent( Bool ascending ) : m_ascending( ascending ) { }
		Bool operator()( const Entry& x, const Entry& y ) const
		{
			return m_ascending ? ( x.m_parent < y.m_parent ) : ( y.m_parent < x.m_parent );
		}
	};

	struct SortEntriesByIsCustom
	{
		Bool m_ascending;
		SortEntriesByIsCustom( Bool ascending ) : m_ascending( ascending ) { }
		Bool operator()( const Entry& x, const Entry& y ) const
		{
			return m_ascending ? ( x.m_isCustom < y.m_isCustom ) : ( y.m_isCustom < x.m_isCustom );
		}
	};

	struct SortEntriesByBlockedString
	{
		Bool m_ascending;
		SortEntriesByBlockedString( Bool ascending ) : m_ascending( ascending ) { }
		Bool operator()( const Entry& x, const Entry& y ) const
		{
			return m_ascending ? ( x.m_blockedString < y.m_blockedString ) : ( y.m_blockedString < x.m_blockedString );
		}
	};

public:

	void Add( String name, EAIAttitude attitude, String parent, Bool isCustom, Bool isConflict, String blockedString )
	{
		m_entries.PushBack( Entry( name, attitude, parent, isCustom, isConflict, blockedString ) );
	}

	Bool Get( Uint32 index, String& name, EAIAttitude& attitude, String& parent, Bool& isCustom, Bool& isConflict, String& blockedString ) const
	{
		if ( index < m_entries.Size() )
		{
			Entry e = m_entries[ index ];
			name = e.m_name;
			attitude = e.m_attitude;
			parent = e.m_parent;
			isCustom = e.m_isCustom;
			isConflict = e.m_isConflict;
			blockedString = e.m_blockedString;
			return true;
		}
		return false;
	}

	Uint32 Size() const  { return m_entries.Size(); }

	void Sort()
	{
		switch ( s_sortType )
		{
		case AST_Name:
			::Sort( m_entries.Begin(), m_entries.End(), SortEntriesByName( s_sortAscending ) );
			break;
		case AST_Attitude:
			::Sort( m_entries.Begin(), m_entries.End(), SortEntriesByAttitude( s_sortAscending ) );
			break;
		case AST_Parent:
			::Sort( m_entries.Begin(), m_entries.End(), SortEntriesByParent( s_sortAscending ) );
			break;
		case AST_IsCustom:
			::Sort( m_entries.Begin(), m_entries.End(), SortEntriesByIsCustom( s_sortAscending ) );
			break;
		case AST_BlockedString:
			::Sort( m_entries.Begin(), m_entries.End(), SortEntriesByBlockedString( s_sortAscending ) );
			break;
		default:
			break;
		}
	}

	static void ChangeSortType( AttitudesSortType newSortType )
	{
		if ( s_sortType == newSortType )
		{
			s_sortAscending = !s_sortAscending;
		}
		else
		{
			s_sortType = newSortType;
			s_sortAscending = true;
		}
	}

	static void ResetSortType()
	{
		s_sortType = AST_None;
		s_sortAscending = false;
	}

	static AttitudesSortType GetSortType() { return s_sortType; }
	static Bool IsSortAscending() { return s_sortAscending; }
};

AttitudesSortType AttitudeEntries::s_sortType = AST_None;
Bool AttitudeEntries::s_sortAscending = true;

}

#define GROUPS_PAIR_STRING( srcGroup, dstGroup ) ( srcGroup.AsString() + TXT( " -> " ) + dstGroup.AsString() ).AsChar()

// Event table
BEGIN_EVENT_TABLE( CEdAttitudeEditor, wxSmartLayoutPanel )
	EVT_MENU( XRCID( "menuItemTargetDLC"), CEdAttitudeEditor::OnSelectTargetDLC )	
	EVT_MENU( XRCID( "menuItemSave" ), CEdAttitudeEditor::OnSave )	
	EVT_MENU( XRCID( "menuItemSubmit" ), CEdAttitudeEditor::OnSubmit )
	EVT_MENU( XRCID( "menuItemExit" ), CEdAttitudeEditor::OnExit )
	EVT_GRID_CMD_CELL_CHANGED( XRCID( "gridMain" ), CEdAttitudeEditor::OnGridCellChange )
	EVT_GRID_CMD_CELL_LEFT_DCLICK( XRCID( "gridMain" ), CEdAttitudeEditor::OnGridCellDoubleClicked )
	EVT_GRID_CMD_COL_SORT( XRCID( "gridMain" ), CEdAttitudeEditor::OnColumnSort )
	EVT_COMBOBOX( XRCID( "comboGroups"), CEdAttitudeEditor::OnGroupSelected )
	EVT_COMBOBOX( XRCID( "comboParent"), CEdAttitudeEditor::OnParentSelected )
	EVT_TEXT( XRCID( "textCtrlFilter") , CEdAttitudeEditor::OnTextFilterChanged )
	EVT_LISTBOX_DCLICK( XRCID( "listParents"), CEdAttitudeEditor::OnParentsListDoubleClicked )
	EVT_LISTBOX_DCLICK( XRCID( "listChildren"), CEdAttitudeEditor::OnChildrenListDoubleClicked )
	EVT_CLOSE( CEdAttitudeEditor::OnClose )
	//EVT_MOUSE_CAPTURE_LOST( CEdAttitudeEditor::OnMouseCaptureLost )
	//EVT_COMMAND( wxID_ANY, wxEVT_GRID_VALUE_CHANGED, CEdAttitudeEditor::OnGridValueChanged )
END_EVENT_TABLE()

CEdAttitudeEditor::CEdAttitudeEditor( wxWindow* parent )
	: wxSmartLayoutPanel( parent, TXT( "AttitudeEditor" ), true )
	, m_attitudeXmlFilePath( ATTITUDES_XML )
	, m_attitudeGroupsFilePath( ATTITUDE_GROUPS_CSV )
	, m_debugMode( false )
	, m_idSaveTool( XRCID( "toolSave" ) )
	, m_idSubmitTool( XRCID( "toolSubmit" ) )
	, m_idHideNeutralTool( XRCID( "toolHideNeutral" ) )
	, m_idHideNonCustomTool( XRCID( "toolHideNonCustom" ) )
	, m_idFilterTool( XRCID( "toolFilter" ) )
	, m_idDebugModeTool( XRCID( "toolDebugMode" ))
	, m_idRefreshTool( XRCID( "toolRefresh" ))
{
	// Get gui elements
	m_guiGrid = XRCCTRL( *this, "gridMain", wxGrid );
	m_defaultGridFont = m_guiGrid->GetFont();
	m_boldGridFont = m_defaultGridFont.Bold();
	m_toolBarSave = XRCCTRL( *this, "toolBarSave", wxToolBar );
	m_toolBarDebug = XRCCTRL( *this, "toolBarDebug", wxToolBar );
	m_toolBarLocal = XRCCTRL( *this, "toolBarLocal", wxToolBar );
	m_comboGroups = XRCCTRL( *this, "comboGroups", wxComboBox );
	m_comboParent = XRCCTRL( *this, "comboParent", wxComboBox );
	m_textCtrlFilter = XRCCTRL( *this, "textCtrlFilter", wxTextCtrl );
	m_textDebugMode = XRCCTRL( *this, "textDebugMode", wxStaticText );
	m_listParents = XRCCTRL( *this, "listParents", wxListBox );
	m_listChildren = XRCCTRL( *this, "listChildren", wxListBox );

	// Connect
	m_toolBarSave->Connect( wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAttitudeEditor::OnToolBar ), 0, this );
	m_toolBarDebug->Connect( wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAttitudeEditor::OnToolBar ), 0, this );
	m_toolBarLocal->Connect( wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAttitudeEditor::OnToolBar ), 0, this );
	GetParent()->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CEdAttitudeEditor::OnClose ), 0, this );

	Connect( wxEVT_CHOICE_CHANGED, wxNotifyEventHandler( CEdAttitudeEditor::OnComboBox ), NULL, this );
	
	if ( EMULATE_SINGLE_CLICK_EDIT )
	{
		m_guiGrid->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( CEdAttitudeEditor::OnGridCellClicked ), 0, this );
	}

	// Load data paths
	LoadConfig();

	// Initialize data
	LoadData();

	// Initialize GUI
	InitGuiData();
	UpdateFilters( false );
	UpdateInheritanceControls();
	UpdateDebugControls();
	InitGrid();

	Layout();
	Show();
}

CEdAttitudeEditor::~CEdAttitudeEditor()
{
}

void CEdAttitudeEditor::OnSave( wxCommandEvent &event )
{
	SaveData();
}

void CEdAttitudeEditor::OnSubmit( wxCommandEvent &event )
{
	SubmitData();
}

void CEdAttitudeEditor::OnExit( wxCommandEvent &event )
{
	Close();
}

void CEdAttitudeEditor::OnClose( wxCloseEvent &event )
{
	if ( m_dataChangedNotSaved && YesNo( TXT("Save modified data?") ) )
	{
		SaveData();
	}
	UndoCheckOutConfigFile();
	Destroy();
}

void CEdAttitudeEditor::OnAttitudeChange( Uint32 row, wxString text )
{
	CheckOutConfigFile();
	String currGroup = m_attitudeGroups[ m_comboGroups->GetCurrentSelection() ];
	String otherGroup = m_guiGrid->GetCellValue( row, GROUP_COLUMN );
	String attitude = text;
	GetAttitudesManager()->SetAttitude( CName( currGroup ), CName( otherGroup ), CAttitudes::AttitudeFromString( attitude ), true );
}

void CEdAttitudeEditor::OnGridCellChange( wxGridEvent &event )
{
	if ( event.GetCol() == ATTITUDE_COLUMN )
	{
		int row = event.GetRow();
		OnAttitudeChange( row, m_guiGrid->GetCellValue( row, ATTITUDE_COLUMN ) );
		UpdateGridRowValue( row );
		UpdateGridRowAttributes( row );
	}
	else if ( event.GetCol() == CUSTOM_ENTRY_COLUMN )
	{
		Bool setToCustom = m_guiGrid->GetCellValue( event.GetRow(), CUSTOM_ENTRY_COLUMN ) == GRID_CELL_ENABLED;
		if ( !setToCustom && wxMessageBox( wxT( "Are you sure? This will reset attitude to parent/default value."), wxT( "Confirm"), wxYES_NO ) == wxNO )
		{
			m_guiGrid->SetCellValue( event.GetRow(), event.GetCol(), GRID_CELL_ENABLED );
		}
		else
		{
			CheckOutConfigFile();
			Uint32 row = event.GetRow();
			String currGroup = m_attitudeGroups[ m_comboGroups->GetCurrentSelection() ];
			String otherGroup = m_guiGrid->GetCellValue( row, GROUP_COLUMN );
			if ( setToCustom )
			{
				String attitude = m_guiGrid->GetCellValue( row, ATTITUDE_COLUMN );
				GetAttitudesManager()->SetAttitude( CName( currGroup ), CName( otherGroup ), CAttitudes::AttitudeFromString( attitude ), true );
			}
			else
			{
				GetAttitudesManager()->RemoveAttitude( CName( currGroup ), CName( otherGroup ), true );
			}
			UpdateGridRowValue( row );
			UpdateGridRowAttributes( row );
		}
	}
}

void CEdAttitudeEditor::OnGridCellClicked( wxGridEvent &event )
{
	CName currGroup = CName( m_attitudeGroups[ m_comboGroups->GetCurrentSelection() ] );
	CName otherGroup = CName( m_guiGrid->GetCellValue( event.GetRow(), GROUP_COLUMN ) );
	CName srcConflictGroup = CName::NONE;
	CName dstConflictGroup = CName::NONE;
	Bool isConflict = GetAttitudesManager()->CanAttitudeCreateConflict( currGroup, otherGroup, srcConflictGroup, dstConflictGroup );
	if ( event.GetCol() == ATTITUDE_COLUMN )
	{
		if ( !isConflict )
		{
			m_guiGrid->SetGridCursor( event.GetRow(), event.GetCol() );
			m_guiGrid->EnableCellEditControl( true );
		}
	}
	else if ( event.GetCol() == CUSTOM_ENTRY_COLUMN )
	{
		// we can change checkbox value only if we're switching it back to "default"
		// or if change wont crate a conflict
		wxGridCellCoords coords( event.GetRow(), event.GetCol() );
		bool enabled = m_guiGrid->GetCellValue( coords ) == GRID_CELL_ENABLED;
		if ( enabled || !isConflict )
		{
			m_guiGrid->SetCellValue( coords, enabled ? GRID_CELL_DISABLED : GRID_CELL_ENABLED );
			OnGridCellChange( event );
		}
	}
	else
	{
		m_guiGrid->SetGridCursor( event.GetRow(), event.GetCol() );
	}
}

void CEdAttitudeEditor::OnGridCellDoubleClicked( wxGridEvent &event )
{
	if ( event.GetCol() == GROUP_COLUMN || event.GetCol() == PARENT_COLUMN )
	{
		JumpToGroup( CName( m_guiGrid->GetCellValue( event.GetRow(), event.GetCol() ) ) );
	}
}

void CEdAttitudeEditor::OnToolBar( wxCommandEvent &event )
{
	const int idSelected = event.GetId();
	if ( idSelected == m_idSaveTool )
	{
		SaveData();
	}
	else if ( idSelected == m_idSubmitTool )
	{
		SubmitData();
	}
	else if ( idSelected == m_idHideNeutralTool )
	{
		UpdateFilters();
	}
	else if ( idSelected == m_idHideNonCustomTool )
	{
		UpdateFilters();
	}
	else if ( idSelected == m_idFilterTool )
	{
		UpdateFilters();
	}
	else if ( idSelected == m_idDebugModeTool )
	{
		// tool is not toggled yet, so we need to negate its state
		SetDebugMode( m_toolBarDebug->GetToolState( m_idDebugModeTool ) );
	}
	else if ( idSelected == m_idRefreshTool )
	{
		UpdateInheritanceControls();
		FillGrid();
	}
}

void CEdAttitudeEditor::OnGroupSelected( wxCommandEvent &event )
{
	UpdateInheritanceControls();
	FillGrid();
}

void CEdAttitudeEditor::OnComboBox( wxNotifyEvent &event )
{
	int row = m_guiGrid->GetGridCursorRow();

	CEdChoice* choice = ( CEdChoice* ) ( event.GetEventObject( ) );

	if ( choice )
	{
		OnAttitudeChange( row, choice->GetString( choice->GetSelection() ) );
	}
}


void CEdAttitudeEditor::OnParentSelected( wxCommandEvent &event )
{
	if ( !YesNo( TXT("Do you really want to change parent for current group?") ) )
	{
		UpdateParentComboValue();
		return;
	}
	// checking if parenthood can create a conflict
	CName currGroupName = CName( m_attitudeGroups[ m_comboGroups->GetCurrentSelection() ] );
	CName parentGroupName = m_possibleParentGroups[ m_comboParent->GetCurrentSelection() ];
	CName childConflictGroup = CName::NONE;
	CName parentConflictGroup = CName::NONE;
	IAttitudesManager* attitudesManager = GetAttitudesManager();
	if ( attitudesManager->CanParenthoodCreateConflict( currGroupName, parentGroupName, childConflictGroup, parentConflictGroup ) )
	{
		String msg = String( TXT( "Cannot set " ) ) + parentGroupName.AsString() + TXT( " as a parent of " ) + currGroupName.AsString()
					 + TXT( " because it will create conflict of " ) + currGroupName.AsString() + TXT( " -> " ) + childConflictGroup.AsString()
					 + TXT( " with " ) + parentGroupName.AsString() + TXT( " -> " ) + parentConflictGroup.AsString();
		wxMessageBox( msg.AsChar(), wxT( "Conflict"), wxOK );
		UpdateParentComboValue();
		return;
	}

	CheckOutConfigFile();
	// if NO_PARENT selected
	if ( parentGroupName == CName::NONE )
	{
		attitudesManager->RemoveParentForGroup( currGroupName, true );
		m_hasParent = false;
	}
	else
	{
		attitudesManager->SetParentForGroup( currGroupName, parentGroupName, true );
		m_hasParent = true;
	}
	UpdateInheritanceControls( false );
	FillGrid();
}

void CEdAttitudeEditor::OnTextFilterChanged( wxCommandEvent &event )
{
	if ( m_toolBarLocal->GetToolState( m_idFilterTool ) )
	{
		UpdateFilters();
	}
}

void CEdAttitudeEditor::OnParentsListDoubleClicked( wxCommandEvent &event )
{
	JumpToGroup( CName( m_listParents->GetStringSelection() ) );
}

void CEdAttitudeEditor::OnChildrenListDoubleClicked( wxCommandEvent &event )
{
	JumpToGroup( CName( m_listChildren->GetStringSelection() ) );
}

void CEdAttitudeEditor::OnColumnSort( wxGridEvent &event )
{
	AttitudesSortType newSortType = AST_None;
	switch ( event.GetCol() )
	{
	case GROUP_COLUMN:
		newSortType = AST_Name;
		break;
	case ATTITUDE_COLUMN:
		newSortType = AST_Attitude;
		break;
	case PARENT_COLUMN:
		newSortType = AST_Parent;
		break;
	case CUSTOM_ENTRY_COLUMN:
		newSortType = AST_IsCustom;
		break;
	case BLOCKED_BY_COLUMN:
		newSortType = AST_BlockedString;
		break;
	default:
		break;
	}
	if ( newSortType != AST_None )
	{
		AttitudeEntries::ChangeSortType( newSortType );
		m_guiGrid->SetSortingColumn( event.GetCol(), AttitudeEntries::IsSortAscending() );
		FillGrid();
	}
}

void CEdAttitudeEditor::InitGuiData()
{
	ASSERT ( m_attitudeGroups.Size() > 0 );

	m_attitudeChoices.push_back( ToString( AIA_Friendly ).AsChar() );
	m_attitudeChoices.push_back( ToString( AIA_Neutral ).AsChar() );
	m_attitudeChoices.push_back( ToString( AIA_Hostile ).AsChar() );

	m_comboParent->Append( NO_PARENT );
	m_possibleParentGroups.PushBack( CName::NONE );
	Uint32 size = m_attitudeGroups.Size();
	for ( Uint32 i = 0; i < size; i++ )
	{
		m_comboGroups->Append( m_attitudeGroups[i].AsChar() );
	}
	m_comboGroups->Select(0);
}

void CEdAttitudeEditor::InitGrid()
{
	m_guiGrid->CreateGrid( 0, 5 );
	m_guiGrid->SetColLabelValue( GROUP_COLUMN, wxT("Group") );
	m_guiGrid->SetColLabelValue( ATTITUDE_COLUMN, wxT("Attitude") );
	m_guiGrid->SetColLabelValue( PARENT_COLUMN, wxT("Parent") );
	m_guiGrid->SetColLabelValue( CUSTOM_ENTRY_COLUMN, wxT("Custom") );
	m_guiGrid->SetColFormatBool( CUSTOM_ENTRY_COLUMN );
	m_guiGrid->SetColLabelValue( BLOCKED_BY_COLUMN, wxT("Blocked by") );
	m_guiGrid->SetRowLabelSize( 0 ); // hide left labels column
	FillGrid();
}

void CEdAttitudeEditor::FillGrid()
{
	m_guiGrid->ClearGrid();
	Bool firstTime = m_guiGrid->GetNumberRows() == 0;
	if ( !firstTime )
	{
		m_guiGrid->DeleteRows( 0, m_guiGrid->GetNumberRows() );
	}

	IAttitudesManager* attitudesManager = GetAttitudesManager( false );
	AttitudeEntries entries;
	Uint32 currGroupIndex = m_comboGroups->GetCurrentSelection();
	CName currGroupName = CName( m_attitudeGroups[currGroupIndex] );
		
	Bool isCurrentGroupVisible = m_visibleAttitudeGroups.Exist( currGroupName.AsChar() );

	for ( Uint32 groupIndex = 0; groupIndex < m_attitudeGroups.Size(); ++groupIndex )
	{
		if ( groupIndex == currGroupIndex )
		{
			continue;
		}

		CName otherGroupName = CName( m_attitudeGroups[groupIndex] );
		CName currGroupParentName = currGroupName;
		CName otherGroupParentName = otherGroupName;

		// Get attitude for this row
		EAIAttitude attitude = CAttitudes::GetDefaultAttitude();
		Bool isCustom = false;
		attitudesManager->GetAttitudeWithParents( currGroupName, otherGroupName, attitude, isCustom, currGroupParentName, otherGroupParentName );

		// Hide neutral filter
		if ( m_hideNeutral && attitude == AIA_Neutral )
		{
			continue;
		}

		// Hide inherited filter
		if ( m_hideNonCustom && !isCustom )
		{
			continue;
		}

		// String filter
		if ( m_filter != String::EMPTY )
		{
			if ( !m_attitudeGroups[groupIndex].ContainsSubstring( m_filter ) )
			{
				continue;
			}
		}

		//! if current group is not visible we show only visible group entries
		if( !isCurrentGroupVisible )
		{
			if( !m_visibleAttitudeGroups.Exist( otherGroupName.AsString() ) )
			{
				continue;
			}
		}	

		CName srcConflictGroup = CName::NONE;
		CName dstConflictGroup = CName::NONE;
		bool isConflict = !isCustom && attitudesManager->CanAttitudeCreateConflict( currGroupName, otherGroupName, srcConflictGroup, dstConflictGroup );
		String conflictString = !isConflict ? TXT("") : GROUPS_PAIR_STRING( srcConflictGroup, dstConflictGroup );
		entries.Add( otherGroupName.AsString(), attitude, currGroupParentName.AsString(), isCustom, isConflict, conflictString );
	}

	entries.Sort();

	for ( Uint32 i = 0; i < entries.Size(); i++ )
	{
		String groupName = String::EMPTY;
		EAIAttitude attitidue = CAttitudes::GetDefaultAttitude();
		String parentName = String::EMPTY;
		Bool isCustom = false;
		Bool isConflict = false;
		String blockedString = String::EMPTY;
		if ( !entries.Get( i, groupName, attitidue, parentName, isCustom, isConflict, blockedString ) )
		{
			continue;
		}

		m_guiGrid->AppendRows( 1 );
		Int32 rowNum = m_guiGrid->GetNumberRows() - 1;

		// column 0 - other group
		m_guiGrid->SetCellValue( rowNum, GROUP_COLUMN, groupName.AsChar() );
		m_guiGrid->SetReadOnly( rowNum, GROUP_COLUMN, true );

		// column 1 - attitude
		m_guiGrid->SetCellEditor( rowNum, ATTITUDE_COLUMN, new CGridCellChoiceEditor( m_attitudeChoices, false ) );
		m_guiGrid->SetCellValue( rowNum, ATTITUDE_COLUMN, ToString( attitidue ).AsChar() );
		m_guiGrid->SetReadOnly( rowNum, ATTITUDE_COLUMN, isConflict );

		// column 2 - parent
		m_guiGrid->SetCellValue( rowNum, PARENT_COLUMN, parentName.AsChar() );
		m_guiGrid->SetReadOnly( rowNum, PARENT_COLUMN, true );

		// column 3 - is custom entry (if not -> default or inherited)
		m_guiGrid->SetCellValue( rowNum, CUSTOM_ENTRY_COLUMN, isCustom ? GRID_CELL_ENABLED : GRID_CELL_DISABLED );
		m_guiGrid->SetReadOnly( rowNum, CUSTOM_ENTRY_COLUMN, EMULATE_SINGLE_CLICK_EDIT || isConflict );

		// column 4 - can entry cause an attitudes conflict (ambiguity)
		m_guiGrid->SetCellValue( rowNum, BLOCKED_BY_COLUMN, blockedString.AsChar() );
		m_guiGrid->SetReadOnly( rowNum, BLOCKED_BY_COLUMN, true );

		UpdateGridRowAttributes( rowNum );
	}

	m_guiGrid->AutoSizeColumns( firstTime );
	m_guiGrid->Refresh();
}

void CEdAttitudeEditor::UpdateGridRowValue( Uint32 row )
{
	IAttitudesManager* attitudesManager = GetAttitudesManager();
	CName currGroup = CName( m_attitudeGroups[ m_comboGroups->GetCurrentSelection() ] );
	CName otherGroup = CName( m_guiGrid->GetCellValue( row, GROUP_COLUMN ) );
	CName parentGroup = CName::NONE;
	CName otherParentGroup = CName::NONE;
	EAIAttitude attitude = CAttitudes::GetDefaultAttitude();
	Bool isCustom = false;
	attitudesManager->GetAttitudeWithParents( currGroup, otherGroup, attitude, isCustom, parentGroup, otherParentGroup );
	CName srcConflictGroup = CName::NONE;
	CName dstConflictGroup = CName::NONE;
	Bool isConflict = !isCustom && attitudesManager->CanAttitudeCreateConflict( currGroup, otherGroup, srcConflictGroup, dstConflictGroup );
	m_guiGrid->SetCellValue( row, ATTITUDE_COLUMN, ToString( attitude ).AsChar() );
	m_guiGrid->SetCellValue( row, PARENT_COLUMN, parentGroup.AsString().AsChar() );
	m_guiGrid->SetCellValue( row, CUSTOM_ENTRY_COLUMN, isCustom ? GRID_CELL_ENABLED : GRID_CELL_DISABLED );
	m_guiGrid->SetCellValue( row, BLOCKED_BY_COLUMN, !isConflict ? TXT("") : GROUPS_PAIR_STRING( srcConflictGroup, dstConflictGroup ) );
}

void CEdAttitudeEditor::UpdateGridRowAttributes( Uint32 row )
{
	IAttitudesManager* attitudesManager = GetAttitudesManager();
	CName currGroup = CName( m_attitudeGroups[ m_comboGroups->GetCurrentSelection() ] );
	CName otherGroup = CName( m_guiGrid->GetCellValue( row, GROUP_COLUMN ) );
	CName srcConflictGroup = CName::NONE;
	CName dstConflictGroup = CName::NONE;
	EAIAttitude attitude = CAttitudes::GetDefaultAttitude();
	Bool isCustom = false;
	attitudesManager->GetAttitude( currGroup, otherGroup, attitude, isCustom );
	Bool isConflict = !isCustom && attitudesManager->CanAttitudeCreateConflict( currGroup, otherGroup, srcConflictGroup, dstConflictGroup );
	if ( isConflict )
	{
		m_guiGrid->SetCellFont( row, ATTITUDE_COLUMN, m_defaultGridFont );
		m_guiGrid->SetCellTextColour( row, ATTITUDE_COLUMN, *wxLIGHT_GREY );
		m_guiGrid->SetCellTextColour( row, PARENT_COLUMN, *wxBLACK );
	}
	else if ( !isCustom )
	{
		m_guiGrid->SetCellFont( row, ATTITUDE_COLUMN, m_defaultGridFont );
		m_guiGrid->SetCellTextColour( row, ATTITUDE_COLUMN, *wxBLACK );
		m_guiGrid->SetCellTextColour( row, PARENT_COLUMN, *wxBLACK );
	}
	else
	{
		m_guiGrid->SetCellFont( row, ATTITUDE_COLUMN, m_boldGridFont );
		m_guiGrid->SetCellTextColour( row, ATTITUDE_COLUMN, *wxBLACK );
		m_guiGrid->SetCellTextColour( row, PARENT_COLUMN, *wxLIGHT_GREY );
	}
	m_guiGrid->SetReadOnly( row, ATTITUDE_COLUMN, isConflict );
	m_guiGrid->SetReadOnly( row, CUSTOM_ENTRY_COLUMN, EMULATE_SINGLE_CLICK_EDIT || isConflict );
}

void CEdAttitudeEditor::UpdateFilters( bool fillGrid )
{
	m_hideNeutral = m_toolBarLocal->GetToolState( m_idHideNeutralTool );
	m_hideNonCustom = m_toolBarLocal->GetToolState( m_idHideNonCustomTool );
	m_filter = String::EMPTY;
	if ( m_toolBarLocal->GetToolState( m_idFilterTool ) )
	{
		m_filter = m_textCtrlFilter->GetValue();
	}
	if ( fillGrid )
	{
		FillGrid();
	}
}

void CEdAttitudeEditor::UpdateParentComboValue()
{
	CName currGroupName = CName( m_attitudeGroups[ m_comboGroups->GetCurrentSelection() ] );
	CName parentGroupName = CName::NONE;
	IAttitudesManager* attitudesManager = GetAttitudesManager();
	attitudesManager->GetParentForGroup( currGroupName, parentGroupName );
	Int32 index = m_possibleParentGroups.GetIndex( parentGroupName );
	ASSERT( index >= 0 );
	if ( parentGroupName == CName::NONE || index == -1 )
	{
		m_comboParent->Select( 0 );
	}
	else
	{
		m_comboParent->Select( index );
	}
}

void CEdAttitudeEditor::UpdateInheritanceControls( Bool updateComboParent )
{
	IAttitudesManager* attitudesManager = GetAttitudesManager();
	Uint32 currentGroupIndex = m_comboGroups->GetCurrentSelection();
	CName currGroupName = CName( m_attitudeGroups[ currentGroupIndex ] );
	CName parentGroupName = CName::NONE;
	attitudesManager->GetParentForGroup( currGroupName, parentGroupName );
	m_hasParent = false;

	if ( updateComboParent )
	{
		m_comboParent->Clear();
		m_comboParent->Append( NO_PARENT );
		m_comboParent->Select( 0 );
		m_possibleParentGroups.Clear();
		m_possibleParentGroups.PushBack( CName::NONE );
	}

	TDynArray< CName > groups;
	attitudesManager->GetAllParents( currGroupName, groups );
	m_listParents->Clear();
	for ( TDynArray< CName >::iterator it = groups.Begin(); it != groups.End(); ++it )
	{
		m_listParents->Append( it->AsString().AsChar() );
	}
	groups.Clear();
	attitudesManager->GetAllChildren( currGroupName, groups );
	m_listChildren->Clear();
	for ( TDynArray< CName >::iterator it = groups.Begin(); it != groups.End(); ++it )
	{
		m_listChildren->Append( it->AsString().AsChar() );
	}

	for ( Uint32 i = 0; i < m_attitudeGroups.Size(); ++i )
	{
		// cannot set itself as a parent
		if ( i == currentGroupIndex )
		{
			continue;
		}
		CName otherGroupName = CName( m_attitudeGroups[i] );
		// cannot set its own child as a parent
		if ( attitudesManager->IsParentForGroup( otherGroupName, currGroupName ) )
		{
			continue;
		}
		if ( updateComboParent )
		{
			m_comboParent->Append( m_attitudeGroups[i].AsChar() );
			m_possibleParentGroups.PushBack( otherGroupName );
		}
		if ( !m_hasParent && parentGroupName != CName::NONE && otherGroupName == parentGroupName )
		{
			if ( updateComboParent )
			{
				m_comboParent->Select( m_comboParent->GetCount() - 1 );
			}
			m_hasParent = true;
		}
	}
}

void CEdAttitudeEditor::JumpToGroup( const CName& groupName )
{
	if ( groupName == CName::NONE )
	{
		return;
	}
	Uint32 index = m_attitudeGroups.GetIndex( groupName.AsString() );
	if ( index >= 0 )
	{
		m_comboGroups->Select( index );
		OnGroupSelected( wxCommandEvent() );
	}
}

void CEdAttitudeEditor::LoadData()
{
	m_attitudeGroups.Clear();
	m_attitudes.ClearAllAttitudes();
	m_dataChangedNotSaved = false;
	m_dataCheckedOutNotSaved = false;
	// Load attitude groups
	{		
		SAttitudesResourcesManager::GetInstance().Sync();
		m_attitudes.LoadAttitudeGroups();

		TDynArray< CName > attitudeGroups;
		Uint32 attitudeGroupsCount = m_attitudes.GetAttitudeGroups( attitudeGroups );
		for ( Uint32 i = 0; i < attitudeGroupsCount; ++i )
		{
			m_attitudeGroups.PushBack( attitudeGroups[i].AsString() );
		}
		m_visibleAttitudeGroups.Clear();
		SAttitudesResourcesManager::GetInstance().GetAttitudeGroupsFormFile( m_attitudeGroupsFilePath, m_visibleAttitudeGroups );
	}

	// Load attitudes
	{
		CDiskFile *diskFile = m_attitudes.GetDiskFile( m_attitudeXmlFilePath );
		if ( diskFile && diskFile->IsNotSynced() )
		{
			diskFile->Sync();
		}
		m_attitudes.LoadDataFromXml( m_attitudeXmlFilePath );
	}
}

void CEdAttitudeEditor::SaveData()
{
	if ( !CheckOutConfigFile() )
	{
		return;
	}
	m_attitudes.RemoveUnusedGroups( m_attitudeGroups );
	m_attitudes.SaveDataToXmlFile( m_attitudeXmlFilePath );
	m_dataChangedNotSaved = false;
	m_dataCheckedOutNotSaved = false;
}

Bool CEdAttitudeEditor::SubmitData()
{
	SaveData();

	CDiskFile *diskFile = GDepot->FindFile( m_attitudeXmlFilePath );
	if ( diskFile == NULL )
	{
		wxMessageBox( wxT("Cannot find attitudes data file."), wxT("Error") );
		return false;
	}

	Bool submitResult = diskFile->Submit();
	if ( !submitResult )
	{
		if ( YesNo( TXT("Attitudes data file wasn't submited. Do you wan't to revert it?") ) )
		{
			diskFile->Revert();
		}
	}

	return submitResult;
}

Bool CEdAttitudeEditor::CheckOutConfigFile()
{
	// do not need to checkout data file while in debug mode (operating on in-game loaded data)
	if ( m_debugMode )
	{
		return true;
	}

	m_dataChangedNotSaved = true;

	CDiskFile *diskFile = GDepot->FindFile( m_attitudeXmlFilePath );
	if ( diskFile == NULL )
	{
		wxMessageBox( wxT("Cannot find attitudes data file."), wxT("Error") );
		return false;
	}

	diskFile->GetStatus();
	if ( !diskFile->IsCheckedOut() )
	{
		if ( !YesNo( TXT("Do you want to checkout attitudes data file?") ) )
		{
			return false;
		}

		// we make an exclusive checkout
		if ( !diskFile->CheckOut( true ) )
		{
			ERR_EDITOR( TXT( "Cannot checkout attitudes data file '%s'" ), m_attitudeXmlFilePath.AsChar() );
			wxMessageBox( wxT("Cannot checkout attitudes data file. Changes will not be saved."), wxT("Error") );
			return false;
		}
		m_dataCheckedOutNotSaved = true;
	}

	return true;
}

Bool CEdAttitudeEditor::UndoCheckOutConfigFile()
{
	CDiskFile *diskFile = GDepot->FindFile( m_attitudeXmlFilePath );
	if ( diskFile == NULL )
	{
		wxMessageBox( wxT("Cannot find attitudes data file."), wxT("Error") );
		return false;
	}

	diskFile->GetStatus();
	if ( diskFile->IsCheckedOut() && m_dataCheckedOutNotSaved )
	{
		if ( YesNo( TXT("Revert attitudes data file?") ) )
		{
			diskFile->Revert();
		}
	}

	return true;
}

IAttitudesManager* CEdAttitudeEditor::GetAttitudesManager( Bool updateGrid )
{
	if ( m_debugMode )
	{
		CAttitudeManager* attMan = GetInGameAttitudesManager();
		if ( attMan != NULL )
		{
			return attMan;
		}
		SetDebugMode( false );
	}
	return &m_attitudes;
}

CAttitudeManager* CEdAttitudeEditor::GetInGameAttitudesManager()
{
	return GCommonGame->IsActive() ? GCommonGame->GetSystem< CAttitudeManager >() : NULL;
}

Bool CEdAttitudeEditor::SetDebugMode( Bool debugMode )
{
	if ( debugMode == m_debugMode )
	{
		return false;
	}
	// if we want to connect to a game and game is not running
	if ( debugMode && GetInGameAttitudesManager() == NULL )
	{
		UpdateDebugControls();
		return false;
	}
	m_debugMode = debugMode;
	UpdateDebugControls();
	return true;
}

Bool CEdAttitudeEditor::IsDebugMode()
{
	if ( !m_debugMode )
	{
		return false;
	}
	if ( GetInGameAttitudesManager() == NULL )
	{
		SetDebugMode( false );
		return false;
	}
	return true;
}

void CEdAttitudeEditor::UpdateDebugControls()
{
	m_toolBarDebug->ToggleTool( m_idDebugModeTool, m_debugMode );
	m_textDebugMode->SetLabel( m_debugMode ? DEBUG_MODE_ON : DEBUG_MODE_OFF );
}

//////////////////////////////////////////////////////////////////////////

Bool CAttitudesEditor::SaveDataToXmlFile( const String& filePath )
{
	CDiskFile *diskFile = GetDiskFile( filePath );
	if ( !diskFile->IsCheckedOut() )
	{
		return false;
	}

	IFile* file = GFileManager->CreateFileWriter( filePath );
	if ( file == NULL )
	{
		ERR_EDITOR( TXT( "Could not open attitudes data file '%s'" ), filePath.AsChar() );
		wxMessageBox( wxT("Cannot open attitudes data file for writting. Remove read-only attribute.") );
		return false;
	}
	CXMLFileWriter* fileWriter = new CXMLFileWriter( *file );

	Bool result = SaveDataToXml( fileWriter );

	delete file;

	return result;
}

CDiskFile* CAttitudesEditor::GetDiskFile( const String& filePath )
{
	CDiskFile *diskFile = GDepot->FindFile( filePath );
	if ( diskFile == NULL )
	{
		ERR_EDITOR( TXT( "Could not open attitudes data file '%s'" ), filePath.AsChar() );
	}

	return diskFile;
}

void CEdAttitudeEditor::OnSelectTargetDLC( wxCommandEvent &event )
{
	TDynArray< String > targets;
	targets.PushBack( wxT("(global)") );

	Int32 selection = 0;
	String dlcFolderName;
	String fullFilePathDLC;

	// Scan DLC folder
	CDirectory* dlc = GDepot->FindPath( TXT("dlc\\") );
	if ( dlc != nullptr )
	{
		for ( CDirectory* dlcDir : dlc->GetDirectories() )
		{
			dlcFolderName = dlcDir->GetName();
			fullFilePathDLC = String::Printf( TXT("dlc\\%ls\\data\\%ls"), dlcFolderName.AsChar(), ATTITUDES_XML );
			if( GDepot->FileExist( fullFilePathDLC ) )
			{				
				if( dlcFolderName == m_targetDLC )
				{
					selection = targets.Size();
				}
				targets.PushBack( dlcFolderName );
			}			
		}
	}

	// Make sure there is a DLC directory to select
	if ( targets.Size() == 1 )
	{
		wxMessageBox( wxString::Format( wxT("No DLC directories found with data\\%s, please make a DLC with data\\%s under dlc\\ first"),  ATTITUDES_XML, ATTITUDES_XML ), wxT("No DLC"), wxOK	| wxCENTER, this );
		return;
	}

	// Show a selection dialog for DLCs
	if ( FormattedDialogBox( this, wxT("Select Target DLC"), wxString::Format( wxT("H{L%s=200|V{B@'&OK'|B'&Cancel'}}=150"), CEdFormattedDialog::ArrayToList( targets ).AsChar() ), &selection ) != 0 )
	{
		return;
	}

	// Store the name and update title
	if ( selection == 0 )
	{
		m_targetDLC = String::EMPTY;
		m_attitudeXmlFilePath	 = ATTITUDES_XML;
		m_attitudeGroupsFilePath = ATTITUDE_GROUPS_CSV;
		SetTitle( wxT("Attitude Editor") );
	}
	else
	{
		m_targetDLC = targets[selection];
		m_attitudeXmlFilePath	 = String::Printf( TXT("dlc\\%ls\\data\\%ls"), m_targetDLC.AsChar(), ATTITUDES_XML );
		m_attitudeGroupsFilePath = String::Printf( TXT("dlc\\%ls\\data\\%ls"), m_targetDLC.AsChar(), ATTITUDE_GROUPS_CSV );
		SetTitle( wxString::Format( wxT("Attitude Editor (Target DLC: %s)"), m_targetDLC.AsChar() ) );
	}
	SaveConfig();

	// Initialize data
	LoadData();

	// Initialize Controls
	m_comboGroups->Select(0);
	UpdateInheritanceControls();
	FillGrid();
}

void CEdAttitudeEditor::LoadConfig()
{
	SUserConfigurationManager::GetInstance().ReadParam( TXT("User"), TXT("AttitudeEditor"), TXT("SelectedDLC"), m_targetDLC );
	if( m_targetDLC.Empty() )
	{
		m_attitudeXmlFilePath	 = ATTITUDES_XML;
		m_attitudeGroupsFilePath = ATTITUDE_GROUPS_CSV;
	}
	else
	{
		String fullFilePathDLC = String::Printf( TXT("dlc\\%ls\\data\\%ls"), m_targetDLC.AsChar(), ATTITUDES_XML );
		if ( !GDepot->FileExist( fullFilePathDLC ) )
		{
			m_attitudeXmlFilePath	 = ATTITUDES_XML;
			m_attitudeGroupsFilePath = ATTITUDE_GROUPS_CSV;
			m_targetDLC.Clear();
			SaveConfig();
		}
		else
		{
			SetTitle( wxString::Format( wxT("Attitude Editor (Target DLC: %s)"), m_targetDLC.AsChar() ) );
			m_attitudeXmlFilePath	 = fullFilePathDLC;
			m_attitudeGroupsFilePath = String::Printf( TXT("dlc\\%ls\\data\\%ls"), m_targetDLC.AsChar(), ATTITUDE_GROUPS_CSV );
		}
	}
}

void CEdAttitudeEditor::SaveConfig()
{
	SUserConfigurationManager::GetInstance().WriteParam( TXT("User"), TXT("AttitudeEditor"), TXT("SelectedDLC"), m_targetDLC );
}