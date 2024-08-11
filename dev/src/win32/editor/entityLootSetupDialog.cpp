#include "build.h"
#include "entityLootSetupDialog.h"
#include "gameplayParamEditor.h"


RED_DEFINE_STATIC_NAME( overrideInherited );


CEdEntityLootSetupDialog::CEdEntityLootSetupDialog( wxWindow* parent, CEntityTemplate* entTemplate  )
	: m_entTemplate( entTemplate )
{
	RED_ASSERT( entTemplate );

	wxXmlResource::Get()->LoadDialog( this, parent, TEXT( "EntityLootEditDialog" ) );
	SetSize( 700, 500 );

	wxButton*	btnAddHead		= XRCCTRL( *this, "addGameplayParam",	wxButton );
	wxButton*	btnRemoveHead	= XRCCTRL( *this, "removeGameplayParam",	wxButton );
	wxButton*	btnSaveAndExit	= XRCCTRL( *this, "SaveAndExit", wxButton );

	wxPanel*	propPanel		= XRCCTRL( *this, "gameplayParamsPanel",	wxPanel );
	m_paramsList				= XRCCTRL( *this, "gameplayParamsListBox",	wxListBox );

	PropertiesPageSettings settings;
	m_propPage = new CEdPropertiesPage( propPanel, settings, nullptr );
	wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
	sizer1->Add( m_propPage, 1, wxEXPAND | wxALL, 0 );
	propPanel->SetSizer( sizer1 );
	propPanel->Layout();

	btnAddHead->Connect(	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler( CEdEntityLootSetupDialog::OnPropAdded ), NULL, this );
	btnRemoveHead->Connect(	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler( CEdEntityLootSetupDialog::OnPropRemoved  ), NULL, this );
	btnSaveAndExit->Connect( wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler( CEdEntityLootSetupDialog::OnSaveAndExit ), NULL, this );
	m_paramsList->Connect(	wxEVT_COMMAND_LISTBOX_SELECTED,	wxCommandEventHandler( CEdEntityLootSetupDialog::OnListChanged  ), NULL, this );
	m_propPage->Connect(	wxEVT_COMMAND_PROPERTY_CHANGED,	wxCommandEventHandler( CEdEntityLootSetupDialog::OnPropModified ), NULL, this );

	RefreshPropList();
}


void CEdEntityLootSetupDialog::OnPropAdded( wxCommandEvent& event )
{
	CClass* objectClass = SRTTI::GetInstance().FindClass( CName( TXT( "CR4LootParam" ) ) );
	if( !objectClass )
	{
		wxMessageBox( wxT( "Code error, please contact programmers and say them that CR4LootParam was not found. Are you working on R6?" ), wxT( "Error" ), wxOK | wxICON_ERROR );
		EndModal( wxID_CANCEL );
		return;
	}

	RED_ASSERT( !objectClass->IsAbstract() );
	RED_ASSERT( m_entTemplate );

	if( ! m_entTemplate || ! m_entTemplate->MarkModified() )
	{
		return;
	}
	CGameplayEntityParam* param = m_entTemplate->FindGameplayParam( objectClass );
	if ( param && CEdGameplayParamEditor::IsFromAIPreset( m_entTemplate, param ) == false )
	{
		wxMessageBox( wxT("This entity template has already parameter from selected class"), wxT("Warning"), wxOK | wxICON_WARNING );
		return;
	}
	String nameStr;
	if ( !InputBox( this , TXT("New parameter"), TXT("Enter name of the new parameter"), nameStr, false ) )
	{
		return;
	}

	param = CreateObject< CGameplayEntityParam >( objectClass, m_entTemplate );
	if ( param )
	{
		param->SetName( nameStr );
		m_entTemplate->AddParameterUnique( param );
	}

	m_propPage->SetObject( param );
	RefreshPropList();
}


void CEdEntityLootSetupDialog::OnPropRemoved( wxCommandEvent& event )
{
	RED_ASSERT( m_entTemplate );
	RED_ASSERT( m_paramsList );
	void* clientData = m_paramsList->GetClientObject( m_paramsList->GetSelection() );
	if ( clientData && m_entTemplate && m_entTemplate->MarkModified() )
	{
		RED_ASSERT( m_propPage );
		m_propPage->SetNoObject();
		m_entTemplate->RemoveParameter( static_cast< TClientDataWrapper< CGameplayEntityParam* >* >( clientData )->GetData() );
		RefreshPropList();		
	}
}

void CEdEntityLootSetupDialog::OnPropModified( wxCommandEvent& event )
{
	void * clientData = event.GetClientData();
	if( !clientData )
	{
		return;
	}

	CEdPropertiesPage::SPropertyEventData* eventData = static_cast< CEdPropertiesPage::SPropertyEventData* >( event.GetClientData() );

	CName fieldName = eventData->m_propertyName;
	if(  fieldName == CNAME( overrideInherited ) || fieldName == CNAME( name ) ) 
	{
		RefreshPropList();	
	}

	CBasePropItem * item = m_propPage->GetRootItem();
	CGameplayEntityParam *param = item->GetRootObject( 0 ).As< CGameplayEntityParam >();
	if( param && param->OnPropModified( fieldName ) )
	{
		m_propPage->RefreshValues();
	}
}

void CEdEntityLootSetupDialog::OnListChanged( wxCommandEvent& event )
{
	if ( !event.GetClientObject() )
	{
		m_propPage->SetNoObject();
		return;
	}

	TClientDataWrapper< CGameplayEntityParam* >* data = static_cast< TClientDataWrapper< CGameplayEntityParam* >* >( event.GetClientObject() );
	CGameplayEntityParam * gameplayEntityParam = data->GetData();
	m_propPage->SetObject( gameplayEntityParam );	
	
	RED_ASSERT( m_entTemplate );
	if ( m_entTemplate && ( !CEdGameplayParamEditor::IsIncluded( m_entTemplate, gameplayEntityParam ) && !CEdGameplayParamEditor::IsFromAIPreset( m_entTemplate, gameplayEntityParam ) ) ) 
	{
		m_propPage->SetReadOnly( false );
	}
	else
	{
		m_propPage->SetReadOnly( true );
	}
}

void CEdEntityLootSetupDialog::OnSaveAndExit( wxCommandEvent& event )
{
	RED_ASSERT( m_entTemplate );
	m_entTemplate->MarkModified();
	m_entTemplate->Save();
	EndModal( wxID_OK );
}

void CEdEntityLootSetupDialog::RefreshPropList()
{
	RED_ASSERT( m_entTemplate );

	m_paramsList->Freeze();
	m_paramsList->Clear();

	CClass* cl = SRTTI::GetInstance().FindClass( CName( TXT( "CR4LootParam" ) ) );
	if( !cl )
	{
		wxMessageBox( wxT( "Code error, please contact programmers and say them that CR4LootParam was not found. Are you working on R6?" ), wxT( "Error" ), wxOK | wxICON_ERROR );
		EndModal( wxID_CANCEL );
		return;
	}

	{
		TDynArray< CGameplayEntityParam* >gameplayEntityParamArray;
		m_entTemplate->CollectGameplayParams( gameplayEntityParamArray, cl );
		for ( Uint32 j = 0; j < gameplayEntityParamArray.Size(); ++j )
		{
			CGameplayEntityParam * const gameplayEntityParam = gameplayEntityParamArray[ j ];
			wxString str = gameplayEntityParam->GetName().AsChar();

			if ( CEdGameplayParamEditor::IsIncluded( m_entTemplate, gameplayEntityParam ) )
			{
				str += wxT("- (Included)");	
			}

			if ( CEdGameplayParamEditor::IsFromAIPreset( m_entTemplate, gameplayEntityParam ) )
			{
				str += wxT("- (From ai preset)");	
			}

			m_paramsList->Append( str, new TClientDataWrapper< CGameplayEntityParam* >( gameplayEntityParam ) );
		}
	}

	if ( m_paramsList->IsEmpty() )
	{
		m_paramsList->AppendString( wxT("(Empty)") );
	}

	m_paramsList->Thaw();
}
