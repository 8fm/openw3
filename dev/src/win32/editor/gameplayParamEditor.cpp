#include "build.h"
#include "gameplayParamEditor.h"
#include "../../common/engine/entityTemplate.h"
#include "../../games/r4/vitalSpot.h"
#include "../../games/r4/journalCreature.h"

RED_DEFINE_STATIC_NAME( overrideInherited );

CEdGameplayParamEditor::CEdGameplayParamEditor( CEdEntityEditor * entityEdit )
	: m_entityEditor( entityEdit )
{
	
		wxButton*	btnAddHead		= XRCCTRL( *m_entityEditor, "addGameplayParam",	wxButton );
		wxButton*	btnRemoveHead	= XRCCTRL( *m_entityEditor, "removeGameplayParam",	wxButton );
		
		wxPanel*	propPanel		= XRCCTRL( *m_entityEditor, "gameplayParamsPanel",	wxPanel );
		m_paramsList				= XRCCTRL( *m_entityEditor, "gameplayParamsListBox",	wxListBox );
		
		PropertiesPageSettings settings;
		m_propPage = new CEdPropertiesPage( propPanel, settings, nullptr );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		sizer1->Add( m_propPage, 1, wxEXPAND | wxALL, 0 );
		propPanel->SetSizer( sizer1 );
		propPanel->Layout();

		btnAddHead->Connect(	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler( CEdGameplayParamEditor::OnPropAdded	  ), NULL, this );
		btnRemoveHead->Connect(	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler( CEdGameplayParamEditor::OnPropRemoved  ), NULL, this );
		m_paramsList->Connect(	wxEVT_COMMAND_LISTBOX_SELECTED,	wxCommandEventHandler( CEdGameplayParamEditor::OnListChanged  ), NULL, this );
		m_propPage->Connect(	wxEVT_COMMAND_PROPERTY_CHANGED,	wxCommandEventHandler( CEdGameplayParamEditor::OnPropModified ), NULL, this );


		

		RefreshPropList();
}

void CEdGameplayParamEditor::OnPropAdded	( wxCommandEvent& event )
{

	TDynArray< CClass* > supportedClasses;
	SRTTI::GetInstance().EnumClasses( CGameplayEntityParam::GetStaticClass() , supportedClasses, NULL, false );
	supportedClasses.Remove( CAITemplateParam::GetStaticClass() );
	if ( supportedClasses.Size() )
	{
		wxMenu popup;

		for ( Uint32 i=0; i<supportedClasses.Size(); i++ )
		{
			popup.Append( i, supportedClasses[i]->GetName().AsString().AsChar(), wxEmptyString, false );
			popup.Connect( i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdGameplayParamEditor::OnPropAddedInternal ), new PopupClassWrapper( supportedClasses[i] ), this ); 
		}
		m_entityEditor->PopupMenu( &popup );
	}
}



void CEdGameplayParamEditor::OnPropAddedInternal( wxCommandEvent& event )
{
	PopupClassWrapper* wrapper = ( PopupClassWrapper* ) event.m_callbackUserData;
	CClass* objectClass = wrapper->m_objectClass;
	ASSERT( objectClass );
	ASSERT( !objectClass->IsAbstract() );

	CEntityTemplate * templ = GetTemplate();
	if( ! templ || ! templ->MarkModified() )
	{
		return;
	}
	CGameplayEntityParam* param = templ->FindGameplayParam( objectClass );
	if ( param && IsFromAIPreset( templ, param ) == false )
	{
		wxMessageBox( wxT("This entity template has already parameter from selected class"), wxT("Warning"), wxOK | wxICON_WARNING );
		return;
	}
	String nameStr;
	if ( !InputBox( m_entityEditor , TXT("New parameter"), TXT("Enter name of the new parameter"), nameStr, false ) )
	{
		return;
	}

	param = CreateObject< CGameplayEntityParam >( objectClass, templ );
	if ( param )
	{
		param->SetName( nameStr );
		templ->AddParameterUnique( param );
	}
	m_propPage->SetObject( param );
	RefreshPropList();
}



void CEdGameplayParamEditor::OnPropRemoved	( wxCommandEvent& event )
{
	CEntityTemplate * templ = GetTemplate();
	void * clientData = m_paramsList->GetClientObject( m_paramsList->GetSelection() );
	if ( clientData && templ && templ->MarkModified() )
	{
		m_propPage->SetNoObject();
		templ->RemoveParameter( static_cast< TClientDataWrapper< CGameplayEntityParam* >* >( clientData )->GetData() );
		RefreshPropList();		
	}
}

void CEdGameplayParamEditor::OnPropModified	( wxCommandEvent& event )
{
	void * clientData = event.GetClientData();
	if(clientData == NULL) return;

	CEdPropertiesPage::SPropertyEventData* eventData = static_cast<CEdPropertiesPage::SPropertyEventData*>( event.GetClientData() );

	CName fieldName = eventData->m_propertyName;
	if(  fieldName == CNAME(overrideInherited) || fieldName == CNAME(name) ) 
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

void CEdGameplayParamEditor::OnListChanged	( wxCommandEvent& event )
{
	if ( !event.GetClientObject() )
	{
		m_propPage->SetNoObject();
		return;
	}
	TClientDataWrapper< CGameplayEntityParam* >* data = static_cast< TClientDataWrapper< CGameplayEntityParam* >* >( event.GetClientObject() );
	CGameplayEntityParam * gameplayEntityParam = data->GetData();
	m_propPage->SetObject( gameplayEntityParam );	
	CEntityTemplate * entTemplate = GetTemplate();
	if ( entTemplate && ( IsIncluded( entTemplate, gameplayEntityParam ) == false && IsFromAIPreset( entTemplate, gameplayEntityParam ) == false ) ) 
	{
		m_propPage->SetReadOnly( false );
	}
	else
	{
		m_propPage->SetReadOnly( true );
	}
}

Bool CEdGameplayParamEditor::IsIncluded( const CEntityTemplate *const entTemplate, const CGameplayEntityParam *const gameplayEntityParam )
{
	TDynArray< CGameplayEntityParam* > localGameplayEntityParamArray;
	entTemplate->CollectGameplayParams( localGameplayEntityParamArray, gameplayEntityParam->GetClass(), true, false  );
	Bool found = false;
	for ( Uint32 k = 0; k < localGameplayEntityParamArray.Size(); ++k )
	{
		CGameplayEntityParam *const localGameplayEntityParam = localGameplayEntityParamArray[ k ];
		if ( localGameplayEntityParam == gameplayEntityParam )
		{
			return false;
		}
	}
	return true;
}

Bool CEdGameplayParamEditor::IsFromAIPreset( const CEntityTemplate *const entTemplate, const CGameplayEntityParam *const gameplayEntityParam )
{
	const CObject *const parent = gameplayEntityParam->GetParent();
	if ( parent->IsA< CEntityTemplate >() )
	{
		const CDiskFile *const file = static_cast< const CEntityTemplate* >( parent )->GetFile();
		// My entity template is an ai_preset
		if ( file->GetFileName().BeginsWith( TXT("ai_preset") ) )
		{
			// to be from ai preset I must not be in an AIPreset
			if ( entTemplate != parent )
			{
				return true;
			}
		}
	}
	return false;
}


void CEdGameplayParamEditor::RefreshPropList()
{
	CEntityTemplate * entTemplate = GetTemplate();
	if( entTemplate == NULL )
	{
		return;
	}
	TDynArray<CClass*> classes;
	

	m_paramsList->Freeze();
	m_paramsList->Clear();

	SRTTI::GetInstance().EnumDerivedClasses( CGameplayEntityParam::GetStaticClass(), classes );
	
	for( auto i = classes.Begin(); i != classes.End(); i++ )
	{
		TDynArray< CGameplayEntityParam* >gameplayEntityParamArray;
		entTemplate->CollectGameplayParams( gameplayEntityParamArray, *i );
		for ( Uint32 j = 0; j < gameplayEntityParamArray.Size(); ++j )
		{
			CGameplayEntityParam * const gameplayEntityParam = gameplayEntityParamArray[ j ];
			wxString str = gameplayEntityParam->GetName().AsChar();

			if ( IsIncluded( entTemplate, gameplayEntityParam ) )
			{
				str += wxT("- (Included)");	
			}
			
			if ( IsFromAIPreset( entTemplate, gameplayEntityParam ) )
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

