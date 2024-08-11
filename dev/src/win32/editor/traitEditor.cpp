#include "build.h"
#if 0
#include "../../games/r6/traitData.h"
#include "traitEditor.h"
#include "gridEditor.h"
#include "gridCustomTypes.h"
#include "traitEdCustomTypes.h"
#include "traitListSelection.h"

// Event table
BEGIN_EVENT_TABLE( CEdTraitEditor, wxSmartLayoutPanel )
	EVT_MENU( XRCID( "menuItemSave" ), CEdTraitEditor::OnSave )
	EVT_MENU( XRCID( "menuItemExit" ), CEdTraitEditor::OnExit )
	EVT_MENU( XRCID( "menuItemValidate" ), CEdTraitEditor::OnValidate )
	EVT_COMMAND( wxID_ANY, wxEVT_GRID_VALUE_CHANGED, CEdTraitEditor::OnGridValueChanged )
	EVT_COMMAND( wxID_ANY, wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, CEdTraitEditor::OnPageChanged )
	EVT_MOUSE_CAPTURE_LOST( CEdTraitEditor::OnMouseCaptureLost )
	 
END_EVENT_TABLE()

CEdTraitEditor::CEdTraitEditor( wxWindow* parent, CTraitData* traitData )
: wxSmartLayoutPanel( parent, TXT( "TraitEditor" ), false )
, m_traitTableProperty( TXT( "i_traitTable" ) )
, m_skillTableProperty( TXT( "i_skillTable" ) )
, m_traitData( traitData )
, m_traitTableGrid( NULL )
, m_skillTableGrid( NULL )
, m_invalidData( true )
, m_dataValidator( traitData )
{
	ASSERT( traitData );

	SEvents::GetInstance().RegisterListener( CNAME( EditorPropertyPreChange ), this );
	SEvents::GetInstance().RegisterListener( CNAME( EditorPropertyPostChange ), this );

	// Add the reference to resources
	m_traitData->AddToRootSet();

	m_notebook = XRCCTRL( *this, "notebook", wxNotebook );
	m_notebook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxCommandEventHandler( CEdTraitEditor::OnPageChanged ), NULL, this );

	// Add tabs
	CClass *classPtr = m_traitData->GetClass();

	// tab 1
	if ( m_traitTableGrid = CreateGridFromProperty( m_notebook, classPtr, TXT( "i_traitTable" ) ) )
	{
		m_traitTableGrid->SetDefaultObjectParent( m_traitData );
		m_notebook->AddPage( m_traitTableGrid, TXT( "Traits" ), true );
	}

	// tab 2
	if ( m_skillTableGrid = CreateGridFromProperty( m_notebook, classPtr, TXT( "i_skillTable" ) ) )
	{
		m_skillTableGrid->SetDefaultObjectParent( m_traitData );
		m_notebook->AddPage( m_skillTableGrid, TXT( "Skills" ), false );
	}

	// Update and finalize layout
	Layout();
	LoadOptionsFromConfig();
	
	wxString titleString;
	if ( m_traitData->GetFile() )
	{
		titleString += m_traitData->GetFile()->GetFileName().AsChar();
	}
	titleString += wxT( " - Trait Editor" );
	SetTitle( titleString );
	Show();

}

CEdTraitEditor::~CEdTraitEditor()
{
	SEvents::GetInstance().UnregisterListener( this );

	// Remove the reference from resources
	m_traitData->RemoveFromRootSet();

	SaveOptionsToConfig();
}

void CEdTraitEditor::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( EditorPropertyPreChange ) )
	{
		m_invalidData = true;
	}
	
	if ( name == CNAME( EditorPropertyPostChange ) )
	{
		m_invalidData = true;
	}
}

void CEdTraitEditor::OnInternalIdle()
{
	if ( m_invalidData )
	{
		CClass *classPtr = m_traitData->GetClass();
		if ( CProperty *propertyPtr = classPtr->FindProperty( m_traitTableProperty ) )
			SetGridObject( m_traitTableGrid, propertyPtr );

		if ( CProperty *propertyPtr = classPtr->FindProperty( m_skillTableProperty ) )
			SetGridObject( m_skillTableGrid, propertyPtr );

		m_invalidData = false;
	}
}

void CEdTraitEditor::SaveOptionsToConfig()
{
	String identifier = TXT( "/Frames/TraitEditor" );
	SaveLayout( identifier );
	
	// save current page
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	config.Write( identifier + TXT( "/CurrentPage" ), m_notebook->GetSelection() );

	if ( m_traitTableGrid )
		m_traitTableGrid->SaveLayout( identifier + TXT( "/TraitTable/" ) );
	if ( m_skillTableGrid )
		m_skillTableGrid->SaveLayout( identifier + TXT( "/SkillTable/" ) );
}

void CEdTraitEditor::LoadOptionsFromConfig()
{
	String identifier = TXT( "/Frames/TraitEditor" );
	LoadLayout( identifier );

	// Load current page
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	Int32 currentPage = config.Read( identifier + TXT( "/CurrentPage" ), 0 );
	if ( currentPage >= 0 && currentPage < ( Int32 )m_notebook->GetPageCount() )
		m_notebook->SetSelection( currentPage );

	if ( m_traitTableGrid )
		m_traitTableGrid->LoadLayout( identifier + TXT( "/TraitTable/" ) );
	if ( m_skillTableGrid )
		m_skillTableGrid->LoadLayout( identifier + TXT( "/SkillTable/" ) );
}

void CEdTraitEditor::OnSave( wxCommandEvent& event )
{
	m_traitData->Save();
}

void CEdTraitEditor::OnExit( wxCommandEvent& event )
{
	ClosePanel();
}

void CEdTraitEditor::OnValidate( wxCommandEvent& event )
{
	UpdatePageData( m_notebook->GetSelection() == 0 ? 1 : 0 );
	String errMsg;
	if ( !m_dataValidator.Validate( errMsg ) )
	{
		wxMessageBox( errMsg.AsChar(), TXT( "Error in trait data" ) );
		m_invalidData = true;
		m_traitData->MarkModified();
	}
	else
	{
		wxMessageBox( TXT( "Validation successful." ), TXT( "Trait data valid" ) );
	}
}

void CEdTraitEditor::OnGridValueChanged( wxCommandEvent& event )
{
	m_traitData->MarkModified();
}

void CEdTraitEditor::OnPageChanged( wxCommandEvent& event )
{
	Int32 i = m_notebook->GetSelection();
	UpdatePageData( i );
}

void CEdTraitEditor::UpdatePageData( Int32 index )
{
	if ( index == 0 ) // skill data table is correct
	{
		m_dataValidator.UpdateTraitWithSkillData();
	}
	else // trait data is correct
	{
		RED_ASSERT( index == 1, TXT( "Unknown page with index %d" ), index );

		m_dataValidator.UpdateSkillWithTraitData();
	}
	m_invalidData = true;
	m_traitData->MarkModified();
}

CGridEditor *CEdTraitEditor::CreateGridFromProperty( wxWindow *parent, CClass *classPtr, String propertyName )
{
	if ( CProperty *propertyPtr = classPtr->FindProperty( CName( propertyName.AsChar() ) ) )
	{
		CGridEditor *gridEditor = new CGridEditor( parent );
		
		if ( propertyName == TXT( "i_skillTable" ) )
		{
			for ( int i = 0; i < SSkillTableEntry::SKILL_HIGHEST_LEVEL; i++ )
			{
				CGridTraitNameColumnDesc* colDesc = new CGridTraitNameColumnDesc( m_traitData->GetTraitTable() );
				String propName;
				propName = String::Printf( TXT("Level %d"), i + 1 );
				gridEditor->RegisterCustomColumnDesc( propName, colDesc );
			}
		}
		else if ( propertyName == TXT( "i_traitTable" ) )
		{
			CGridAbilityCellDesc* aTypeDesc = new CGridAbilityCellDesc();
			gridEditor->RegisterCustomType( aTypeDesc );
			CGridRequirementCellDesc* rTypeDesc = new CGridRequirementCellDesc();
			gridEditor->RegisterCustomType( rTypeDesc );
		}
		SetGridObject( gridEditor, propertyPtr );
		return gridEditor;
	}
	else
	{
		WARN_EDITOR( TXT( "Cannot find property %s in the class %s." ), propertyName.AsChar(), classPtr->GetName().AsChar() );
	}

	return NULL;
}

void CEdTraitEditor::SetGridObject( CGridEditor *grid, CProperty *prop )
{
	if ( grid )
	{
		void *data = prop->GetOffsetPtr( m_traitData );
		grid->SetObject( data, prop );
	}
}

#endif