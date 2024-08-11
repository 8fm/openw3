/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "lootEditor.h"
#include "../../common/game/lootDefinitions.h"
#include "../../common/core/depot.h"
#include "../../common/core/xmlFileReader.h"
#include "../../common/core/xmlFileWriter.h"

namespace
{
struct CNameComparer
{
	Bool operator()( const CName& x, const CName& y ) const
	{
		return x.AsString().ToLower() < y.AsString().ToLower();
	}
};
}

// Event table
BEGIN_EVENT_TABLE( CEdLootEditor, wxSmartLayoutPanel )
	EVT_MENU( XRCID( "menuItemSave" ), CEdLootEditor::OnSave )
	EVT_MENU( XRCID( "menuItemSubmit" ), CEdLootEditor::OnSubmit )
	EVT_MENU( XRCID( "menuItemExit" ), CEdLootEditor::OnExit )
	EVT_MENU( XRCID( "menuReloadDefinitions" ), CEdLootEditor::OnReloadDefinitions )
	EVT_COMBOBOX( XRCID( "comboFilenames" ), CEdLootEditor::OnFilenameSelected )
	EVT_LISTBOX( XRCID( "listDefinitions" ), CEdLootEditor::OnDefinitionSelected )
	EVT_BUTTON( XRCID( "buttonAddDefinition"), CEdLootEditor::OnAddDefinition )
	EVT_BUTTON( XRCID( "buttonDeleteDefinition"), CEdLootEditor::OnDeleteDefinition )
	EVT_CLOSE( CEdLootEditor::OnClose )
END_EVENT_TABLE()

CEdLootEditor::CEdLootEditor( wxWindow* parent )
	: wxSmartLayoutPanel( parent, TXT( "LootEditor" ), true )
	, m_lootDefinitions( NULL )
	, m_dataChangedNotSaved( false )
{
	m_creatorTag = Red::System::GUID::Create();

	m_comboFilenames = XRCCTRL( *this, "comboFilenames", wxComboBox );
	m_listDefinitions = XRCCTRL( *this, "listDefinitions", wxListBox );

	GetParent()->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CEdLootEditor::OnClose ), 0, this );

	InitPropertiesControl();
	InitData();

	m_currentFilename = String::EMPTY;
	m_comboFilenames->Select( 0 );
	OnFilenameSelected( wxCommandEvent() );

	Layout();
	Show();
}

CEdLootEditor::~CEdLootEditor()
{
	delete m_lootDefinitions;
}

void CEdLootEditor::InitData()
{
	m_lootDefinitions = GCommonGame->CreateLootDefinitions();
	ASSERT ( m_lootDefinitions != NULL );
	m_lootDefinitions->GetLootDefinitionsFilenames( m_filenames );
	ASSERT( m_filenames.Size() > 0 );

	m_comboFilenames->Clear();
	for ( TDynArray< String >::iterator it = m_filenames.Begin(); it != m_filenames.End(); ++it )
	{
		m_comboFilenames->Append( it->AsChar() );
	}
}

void CEdLootEditor::InitPropertiesControl()
{
	wxPanel* panel = XRCCTRL( *this, "panelProperties", wxPanel );
	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

	PropertiesPageSettings settings;
	m_properties = new CEdPropertiesBrowserWithStatusbar( panel, settings, nullptr );
	m_properties->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdLootEditor::OnPropertiesChanged ), NULL, this );
	sizer->Add( m_properties, 1, wxEXPAND, 0 );
	panel->SetSizer( sizer );
	panel->Layout();
}

void CEdLootEditor::UpdateDefinitionsListBox()
{
	m_lootDefinitions->GetDefinitionsNames( m_definitionsNames );
	Sort( m_definitionsNames.Begin(), m_definitionsNames.End(), CNameComparer() );
	m_listDefinitions->Clear();
	for ( TDynArray< CName >::iterator it = m_definitionsNames.Begin(); it != m_definitionsNames.End(); ++it )
	{
		m_listDefinitions->Append( it->AsString().AsChar() );
	}
}

CName CEdLootEditor::FindUniqueDefinitionName() const
{
	String str = TXT( "NewDefinition" );
	CName name = CName( str );
	if ( m_lootDefinitions->GetDefinition( name ) == NULL )
	{
		return name;
	}
	for ( Uint32 i = 0; i < 10000; i++ )
	{
		name = CName( str + ToString( i ) );
		if ( m_lootDefinitions->GetDefinition( name ) == NULL )
		{
			return name;
		}
	}
	return CName::NONE;
}

void CEdLootEditor::OnFilenameSelected( wxCommandEvent &event )
{
	Int32 index = static_cast< Int32 >( m_comboFilenames->GetSelection() );
	if ( index < 0 || index >= static_cast< Int32 >( m_filenames.Size() ) )
	{
		return;
	}

	if ( m_filenames[index] == m_currentFilename )
	{
		return;
	}

	SaveChangedData();
	m_currentFilename = m_filenames[index];
	
	m_properties->Get().SetNoObject();
	Load( m_currentFilename );
	UpdateDefinitionsListBox();

	m_listDefinitions->Select( 0 );
	OnDefinitionSelected( wxCommandEvent() );
}

void CEdLootEditor::OnDefinitionSelected( wxCommandEvent &event )
{
	Int32 index = static_cast< Int32 >( m_listDefinitions->GetSelection() );
	if ( index < 0 || index >= static_cast< Int32 >( m_definitionsNames.Size() ) )
	{
		m_properties->Get().SetNoObject();
		return;
	}

	m_properties->Get().SetObject( m_lootDefinitions->GetDefinition( m_definitionsNames[index] ) );
}

void CEdLootEditor::OnAddDefinition( wxCommandEvent &event )
{
	CName name = FindUniqueDefinitionName();
	if ( name == CName::NONE )
	{
		return;
	}
	CLootDefinitionBase* newDefinition = NULL;
	if ( m_lootDefinitions->AddNewDefinition( name, &newDefinition ) )
	{
		m_dataChangedNotSaved = true;
		UpdateDefinitionsListBox();
		Uint32 index = m_definitionsNames.GetIndex( name );
		if ( index >= 0 )
		{
			m_listDefinitions->Select( index );
		}		
		OnDefinitionSelected( wxCommandEvent() );
	}
}

void CEdLootEditor::OnDeleteDefinition( wxCommandEvent &event )
{
	Int32 index = static_cast< Int32 >( m_listDefinitions->GetSelection() );
	if ( index < 0 || index >= static_cast< Int32 >( m_definitionsNames.Size() ) )
	{
		return;
	}
	CName name = m_definitionsNames[index];
	// we need to remove object from the properties control
	// to prevent from leaving dangling reference after object destruction
	m_properties->Get().SetNoObject();
	if ( m_lootDefinitions->RemoveDefinition( name ) )
	{
		m_dataChangedNotSaved = true;
		UpdateDefinitionsListBox();
		if ( m_definitionsNames.Size() > 0 )
		{
			if ( index >= static_cast< Int32 >( m_definitionsNames.Size() ) )
			{
				index--;
			}
			m_listDefinitions->Select( index );
		}
	}
	OnDefinitionSelected( wxCommandEvent() );
}

void CEdLootEditor::OnPropertiesChanged( wxCommandEvent &event )
{
	Int32 index = static_cast< Int32 >( m_listDefinitions->GetSelection() );
	if ( index < 0 || index >= static_cast< Int32 >( m_definitionsNames.Size() ) )
	{
		return;
	}

	m_dataChangedNotSaved = true;
	CName currentName = m_definitionsNames[index];
	CLootDefinitionBase* definition = m_lootDefinitions->GetDefinition( currentName );
	// if definition name was changes, we need to check if it is still unique
	if ( definition != NULL && definition->GetName() != currentName )
	{
		CName newName = definition->GetName();
		// if name is unique we need to update it and refresh definitions list box
		if ( m_lootDefinitions->IsDefinitionNameUnique( newName ) )
		{
			// we need to remove object from the properties control
			// to prevent from leaving dangling reference after updating name
			// (updating name removes one object and inserts its copy)
			m_properties->Get().SetNoObject();
			m_lootDefinitions->UpdateDefinitionName( currentName, newName );
			UpdateDefinitionsListBox();
			index = m_definitionsNames.GetIndex( newName );
			if ( index >= 0 )
			{
				m_listDefinitions->Select( index );
			}
			OnDefinitionSelected( wxCommandEvent() );
		}
		// otherwise change back definition name and inform user
		else
		{
			definition->SetName( currentName );
			m_properties->Get().RefreshValues();
			wxMessageBox( TXT( "Definition names have to be unique! Try another name." ) );
		}
	}
}

void CEdLootEditor::OnSave( wxCommandEvent &event )
{
	SaveData();
}

void CEdLootEditor::OnSubmit( wxCommandEvent &event )
{
	SubmitData();
}

void CEdLootEditor::OnExit( wxCommandEvent &event )
{
	Close();
}

void CEdLootEditor::OnReloadDefinitions( wxCommandEvent &event )
{
	ASSERT( GCommonGame->GetDefinitionsManager() != NULL );
	GCommonGame->GetDefinitionsManager()->ReloadAll();
}

void CEdLootEditor::OnClose( wxCloseEvent &event )
{
	SaveChangedData();
	Destroy();
}

Bool CEdLootEditor::Load( const String& filename )
{
	CDiskFile *diskFile = GDepot->FindFile( filename );
	if ( diskFile == NULL )
	{
		return false;
	}

	IFile* file = diskFile->CreateReader();
	if ( file == NULL )
	{
		return false;
	}

	CXMLFileReader* reader = new CXMLFileReader( *file );
	m_lootDefinitions->Clear();
	m_lootDefinitions->Load( reader, m_creatorTag );
	delete reader;
	delete file;
	m_dataChangedNotSaved = false;
	return true;
}

Bool CEdLootEditor::Save( const String& filename )
{
	// we're assuming here that file exists and is checked out
	// caller of this method should be responsible for that

	CDiskFile *diskFile = GDepot->FindFile( filename );
	if ( diskFile == NULL )
	{
		return false;
	}

	if ( !diskFile->IsCheckedOut() )
	{
		return false;
	}

	IFile* file = GFileManager->CreateFileWriter( filename );
	if ( file == NULL )
	{
		return false;
	}

	CXMLFileWriter* writer = new CXMLFileWriter( *file );
	m_lootDefinitions->Save( writer );
	delete writer;
	delete file;
	m_dataChangedNotSaved = false;
	return true;
}

Bool CEdLootEditor::CheckoutFile( const String& filename )
{
	CDiskFile *diskFile = GDepot->FindFile( filename );
	if ( diskFile == NULL )
	{
		ERR_EDITOR( TXT( "Could not open file '%s'" ), filename.AsChar() );
		wxMessageBox( TXT("Could not open file!") );
		return false;
	}

	diskFile->GetStatus();
	if ( !diskFile->IsCheckedOut() )
	{
		if ( !YesNo( TXT("Do you want to checkout data file?") ) )
		{
			return false;
		}

		if ( !diskFile->CheckOut() )
		{
			ERR_EDITOR( TXT( "Cannot checkout attitude file '%s'" ), filename.AsChar() );
			if ( wxMessageBox( TXT("Cannot checkout file!") ) )
			{
				return false;
			}
		}
	}

	return true;
}

Bool CEdLootEditor::SaveChangedData()
{
	if ( m_dataChangedNotSaved && YesNo( TXT("Save modified data?") ) )
	{
		return SaveData();
	}
	return true;
}

Bool CEdLootEditor::SaveData()
{
	if ( !CheckoutFile( m_currentFilename ) )
	{
		return false;
	}
	if ( !Save( m_currentFilename ) )
	{
		return false;
	}
	return true;
}

Bool CEdLootEditor::SubmitData()
{
	if ( m_dataChangedNotSaved )
	{
		if ( !SaveData() )
		{
			return false;
		}
	}

	// SaveData should check if the file exists and is checked out
	CDiskFile *diskFile = GDepot->FindFile( m_currentFilename );
	if ( diskFile == NULL )
	{
		return false;
	}

	Bool submitResult = diskFile->Submit();
	if ( !submitResult )
	{
		if ( YesNo( TXT("File wasn't submited. Do you wan't to revert it?") ) )
		{
			diskFile->Revert();
		}
	}

	return submitResult;
}