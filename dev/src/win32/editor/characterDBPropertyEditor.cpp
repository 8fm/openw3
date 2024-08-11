#include "build.h"
#include "characterDBEditor.h"
#include "characterDBPropertyEditor.h"
#include "characterResourceContainer.h"

CCharacterDBPropertyEditor::CCharacterDBPropertyEditor( CPropertyItem* propertyItem )
	: ICustomPropertyEditor( propertyItem )
    , m_characterEditorExt( nullptr )
{
	m_iconDeleteCharacter = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_DELETE") );
	m_iconCharacterEditor = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_PICK") );
}

CCharacterDBPropertyEditor::~CCharacterDBPropertyEditor()
{
	if ( m_characterEditorExt )
	{
		m_characterEditorExt->Unbind( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, wxCommandEventHandler( CCharacterDBPropertyEditor::OnEditorOk ), this );
		m_characterEditorExt->Unbind( wxEVT_CLOSE_PANEL, &CCharacterDBPropertyEditor::OnEditorExit, this );
		m_characterEditorExt = nullptr;
	}
}
void CCharacterDBPropertyEditor::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	m_propertyItem->AddButton( m_iconCharacterEditor, wxCommandEventHandler( CCharacterDBPropertyEditor::OnShowEditor ), this );
	m_propertyItem->AddButton( m_iconDeleteCharacter, wxCommandEventHandler( CCharacterDBPropertyEditor::OnClearValue ), this );
}

void CCharacterDBPropertyEditor::CloseControls()
{
	if ( m_characterEditorExt )
	{
		m_characterEditorExt->Hide();
	}
}

CEdCharacterDBEditor* CCharacterDBPropertyEditor::GetEditorExt()
{
	if ( !m_characterEditorExt )
	{
		m_characterEditorExt = new CEdCharacterDBEditor( m_propertyItem->GetPage(), nullptr, true );
		m_characterEditorExt->Bind( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, &CCharacterDBPropertyEditor::OnEditorOk, this );
		m_characterEditorExt->Bind( wxEVT_CLOSE_PANEL, &CCharacterDBPropertyEditor::OnEditorExit, this );
	}
	return m_characterEditorExt;
}


Bool CCharacterDBPropertyEditor::GrabValue( String& displayValue )
{
	// Read quid value
	CGUID guid;
	m_propertyItem->Read( &guid );

	// Get character name 
	CEdCharacterResourceContainer* resContainer = GetEditorExt()->GetResourceContainer();
	CCharacter* character = resContainer->FindCharacterByGUID( guid );
	if ( !character )
	{
		displayValue = TXT("NULL");
	}
	else
	{
		displayValue = character->GetName().AsString();
	}

	return true;
}

void CCharacterDBPropertyEditor::OnShowEditor( wxCommandEvent& event )
{
	// Read the condition
	CGUID guid;
	m_propertyItem->Read( &guid );

	GetEditorExt()->SelectCharacter( guid );
	GetEditorExt()->Show();
}

void CCharacterDBPropertyEditor::OnClearValue( wxCommandEvent& event )
{
	// Read the value
	CGUID guid;

	// Write empty guid
	m_propertyItem->Write( &guid );

	// Redraw property
	m_propertyItem->GrabPropertyValue();
}

void CCharacterDBPropertyEditor::OnEditorOk( wxCommandEvent &event )
{
	RED_FATAL_ASSERT( m_characterEditorExt, "Editor has to exist because we have a event from it" );

	CGUID guid;
	if ( m_propertyItem->Read( &guid ) )
	{
		const CCharacter* character = m_characterEditorExt->GetSelectedCharacter();
		guid = character->GetGUID();

		m_propertyItem->Write( &guid );
		m_propertyItem->GrabPropertyValue();
	}

	m_characterEditorExt->Hide();
}

void CCharacterDBPropertyEditor::OnEditorExit( wxCloseEvent &event )
{
	m_characterEditorExt = nullptr;
	event.Skip();
}
