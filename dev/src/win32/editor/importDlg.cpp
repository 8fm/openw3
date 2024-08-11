/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "importDlg.h"
#include "commonDlgs.h"
#include "editorExternalResources.h"
#include "../../common/core/factory.h"
#include "../../common/core/depot.h"
#include "../../common/core/configFileManager.h"
#include "../../common/engine/materialDefinition.h"

#define STD_SHADER_ROOT_DIR TXT( "engine\\materials\\graphs\\" )

BEGIN_EVENT_TABLE( CEdImportDlg, wxDialog )
	EVT_BUTTON( XRCID("OK"), CEdImportDlg::OnOK )
	EVT_BUTTON( XRCID("All"), CEdImportDlg::OnOKAll )
	EVT_BUTTON( XRCID("Cancel"), CEdImportDlg::OnCancel )
	EVT_BUTTON( XRCID("Skip"), CEdImportDlg::OnSkip )
	EVT_CHECKBOX( XRCID("AddToFavs"), CEdImportDlg::OnAddToFavsCheck )
	EVT_UPDATE_UI( wxID_ANY, CEdImportDlg::OnUpdateUI )
END_EVENT_TABLE()

CEdImportDlg::CEdImportDlg( 
	wxWindow* parent, const CClass* resourceClass, const CDirectory* fileDirectory, const String& fileName, CObject* configObject,
	TDynArray< String >* favClasses
	)
	: m_fileName( fileName )
	, m_createOnly( configObject && configObject->IsA< IFactory >() )
	, m_resourceClassName( resourceClass ? resourceClass->GetName().AsString() : String::EMPTY  )
	, m_favClasses( favClasses )
	, m_favFlipped( false )
{
	// Load designed frame from resource
	wxXmlResource::Get()->LoadDialog( this, parent, TXT("ImportCreateDialog") );

	// Set class
	XRCCTRL( *this, "ClassList", wxChoice )->Append( m_resourceClassName.Empty() ? TXT("Not needed") : m_resourceClassName.AsChar() );
	XRCCTRL( *this, "ClassList", wxChoice )->SetSelection( 0 );
	XRCCTRL( *this, "ClassList", wxChoice )->Disable();

	if ( m_favClasses && m_resourceClassName.Empty() )
	{
		XRCCTRL( *this, "AddToFavs", wxCheckBox )->Hide();
	}

	// Disable some buttons on create only mode
	if ( m_createOnly )
	{
		XRCCTRL( *this, "All", wxButton )->Hide();
		XRCCTRL( *this, "Skip", wxButton )->Hide();
	}

	if ( fileDirectory )
	{
		// Set directory
		String extraPath = fileDirectory->GetDepotPath();
		XRCCTRL( *this, "Directory", wxTextCtrl )->SetValue( extraPath.AsChar() );
		XRCCTRL( *this, "Directory", wxTextCtrl )->Disable();
		XRCCTRL( *this, "FileName", wxTextCtrl )->SetValue( fileName.AsChar() );
		XRCCTRL( *this, "FileName", wxTextCtrl )->Enable();
	}
	else
	{
		XRCCTRL( *this, "Directory", wxTextCtrl )->SetValue( TXT("Not used") );
		XRCCTRL( *this, "Directory", wxTextCtrl )->Disable();
		XRCCTRL( *this, "FileName", wxTextCtrl )->SetValue( TXT("Not used") );
		XRCCTRL( *this, "FileName", wxTextCtrl )->Disable();
	}

	// Set properties
	if ( configObject )
	{
		wxPanel* panel = XRCCTRL( *this, "PropPanel", wxPanel );

		TDynArray< CProperty* > props;
		configObject->GetClass()->GetProperties( props );
		if ( !props.Empty() )
		{
			// Create property panel
			wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
			PropertiesPageSettings settings;
			CEdPropertiesBrowserWithStatusbar* props = new CEdPropertiesBrowserWithStatusbar( panel, settings, nullptr );
			sizer->Add( props, 1, wxALL | wxEXPAND, 1 );
			panel->SetSizer( sizer );
			panel->Layout();

			// Set config object
			props->Get().SetObject( configObject );
		}
		else
		{
			panel->Hide();
		}
	}

	if ( m_favClasses && !m_resourceClassName.Empty() && m_favClasses->Exist( m_resourceClassName ) )
	{
		XRCCTRL( *this, "AddToFavs", wxCheckBox )->SetValue( 1 );
	}

	DoLayoutAdaptation();
}

CEdImportDlg::~CEdImportDlg()
{

}

EImportCreateReturn CEdImportDlg::DoModal()
{
	CenterOnParent();
	XRCCTRL( *this, "FileName", wxTextCtrl )->SetFocus();
	return static_cast< EImportCreateReturn >( wxDialog::ShowModal() );
}

void CEdImportDlg::ReadValuesBack()
{
	m_fileName = XRCCTRL( *this, "FileName", wxTextCtrl )->GetValue().wc_str();

	if ( m_favClasses && !m_resourceClassName.Empty() )
	{
		if ( XRCCTRL( *this, "AddToFavs", wxCheckBox )->IsChecked() )
		{
			m_favClasses->PushBackUnique( m_resourceClassName );
		}
		else
		{
			m_favClasses->Remove( m_resourceClassName );
		}
	}
}

void CEdImportDlg::OnOK( wxCommandEvent& event )
{
	ReadValuesBack();
	// file name can be empty as user might change only 'favorite' check
	EndDialog( m_fileName.Empty() ? ECR_Cancel : ECR_OK );
}

void CEdImportDlg::OnOKAll( wxCommandEvent& event )
{
	ReadValuesBack();
	// file name can be empty as user might change only 'favorite' check
	EndDialog( m_fileName.Empty() ? ECR_Cancel : ECR_OKAll );
}

void CEdImportDlg::OnSkip( wxCommandEvent& event )
{
	EndDialog( ECR_Skip );
}

void CEdImportDlg::OnCancel( wxCommandEvent& event )
{
	EndDialog( ECR_Cancel );
}

void CEdImportDlg::OnAddToFavsCheck( wxCommandEvent& event )
{
	m_favFlipped = true;
}

void CEdImportDlg::OnUpdateUI( wxUpdateUIEvent& event )
{
	Bool isFileName = XRCCTRL( *this, "FileName", wxTextCtrl )->GetValue().Length() > 0;
	XRCCTRL( *this, "OK", wxButton )->Enable( isFileName || m_favFlipped );
	XRCCTRL( *this, "All", wxButton )->Enable( ( isFileName || m_favFlipped ) && !m_createOnly );
}



BEGIN_EVENT_TABLE( CEdImportMeshDlg, wxDialog )
	EVT_BUTTON( XRCID("OK"), CEdImportMeshDlg::OnOK )
	EVT_BUTTON( XRCID("OKAll"), CEdImportMeshDlg::OnOKAll )
	EVT_BUTTON( XRCID("Skip"), CEdImportMeshDlg::OnSkip )
	EVT_BUTTON( XRCID("Cancel"), CEdImportMeshDlg::OnCancel )
	EVT_LISTBOX( XRCID("RecentMaterials"), CEdImportMeshDlg::OnMaterialChanged )
	EVT_LISTBOX( XRCID("AllMaterials"), CEdImportMeshDlg::OnMaterialChanged )
	EVT_TEXT( XRCID("TextSearch"), CEdImportMeshDlg::OnText )
END_EVENT_TABLE()

CEdImportMeshDlg::CEdImportMeshDlg( wxWindow* parent, CMeshTypeResource* mesh )
	: m_mesh( mesh )
{
	// Load designed frame from resource
	wxXmlResource::Get()->LoadDialog( this, parent, TXT("ImportMeshDialog") );
}

CEdImportMeshDlg::~CEdImportMeshDlg()
{

}

void CEdImportMeshDlg::UpdateMaterialList()
{
	// Load config
	SUserConfigurationManager::GetInstance().ReadParam( TXT("User"), TXT("ImportMeshDialog"), TXT("RecentMaterial"), m_recentMaterial );
	SUserConfigurationManager::GetInstance().ReadParams( TXT("User"), TXT("ImportMeshDialog"), TXT("RecentMaterialList"), m_recentMaterialsList );

	// No recent material, use standard material
	if ( m_recentMaterial.Empty() )
	{
		m_recentMaterial = DEFAULT_MATERIAL;
	}

	// No recent materials on list, add standard material
	if ( !m_recentMaterialsList.Size() )
	{
		m_recentMaterialsList.PushBack( m_recentMaterial );
	}

	// Fill list of all materials
	{
		// Begin update
		wxListBox *materialsList = XRCCTRL( *this, "AllMaterials", wxListBox );
		materialsList->Freeze();
		materialsList->Clear();


		// scan for std shaders	
		CDirectory* rootDir = GDepot->FindPath( STD_SHADER_ROOT_DIR );
		ASSERT( rootDir );

		const TFiles& files = rootDir->GetFiles();
		TDynArray< CDiskFile* > materials;		

		for ( auto it=files.Begin(); it != files.End(); ++it )
		{
			CDiskFile* file = *it;

			if ( file->GetFileName().EndsWith( TXT(".w2mg") ) )
			{		
				file->Load();
				CResource* res = file->GetResource();

				if( res && res->IsA< IMaterial >() )
				{
					materials.PushBack( file );					
				}
			}
		}

		// Fill list
		for( TDynArray< CDiskFile* >::iterator it=materials.Begin(); it!=materials.End(); it++ )
		{
			Int32 index = materialsList->Append( ( *it )->GetDepotPath().AsChar() );
			if ( m_recentMaterial == ( *it )->GetDepotPath() )
			{
				materialsList->SetSelection( index );
			}
		}

		// Refresh
		materialsList->Thaw();
		materialsList->Refresh();
	}

	// Fill list of recent materials
	{
		// Begin update
		wxListBox *materialsList = XRCCTRL( *this, "RecentMaterials", wxListBox );
		materialsList->Freeze();
		materialsList->Clear();

		// Fill list
		for ( Uint32 i=0; i<m_recentMaterialsList.Size(); i++ )
		{
			Int32 index = materialsList->Append( m_recentMaterialsList[i].AsChar() );
			if ( m_recentMaterial == m_recentMaterialsList[i] )
			{
				materialsList->SetSelection( index );
			}
		}

		// Refresh
		materialsList->Thaw();
		materialsList->Refresh();
	}

	// Update textures of material
	UpdateMaterialTextures();
}


EImportCreateReturn CEdImportMeshDlg::DoModal()
{
	UpdateMaterialList();
	return ( EImportCreateReturn ) wxDialog::ShowModal();
}

/*CResource* CEdImportMeshDlg::GetTexture( const String &name )
{
}*/

void CEdImportMeshDlg::SaveConfig()
{
	// Add to list of recent materials
	if ( !m_recentMaterialsList.Exist(m_recentMaterial) )
	{
		m_recentMaterialsList.Insert( 0, m_recentMaterial );
		if ( m_recentMaterialsList.Size() > 10 )
		{
			m_recentMaterialsList.PopBack();
		}
	}

	// Save config
	SUserConfigurationManager::GetInstance().WriteParam( TXT("User"), TXT("ImportMeshDialog"), TXT("RecentMaterial"), m_recentMaterial );
	SUserConfigurationManager::GetInstance().WriteParamArray( TXT("User"), TXT("ImportMeshDialog"), TXT("RecentMaterialList"), m_recentMaterialsList );
	SUserConfigurationManager::GetInstance().SaveAll();

	// Save material to auto assign config
	m_autoAssignConfig.m_materialName = m_recentMaterial;

	// Get diffuse param name
	{
		wxChoice *propertiesDiffuseList = XRCCTRL( *this, "choiceDiffuse", wxChoice );
		m_autoAssignConfig.m_setDiffuseParam = (propertiesDiffuseList->GetSelection() > 0);
		if ( m_autoAssignConfig.m_setDiffuseParam )
		{
			m_autoAssignConfig.m_diffuseParameterName = propertiesDiffuseList->GetStringSelection().wc_str();
		}
	}

	// Get normal param name
	{
		wxChoice *propertiesNormalList = XRCCTRL( *this, "choiceNormal", wxChoice );
		m_autoAssignConfig.m_setNormalParam = (propertiesNormalList->GetSelection() > 0);
		if ( m_autoAssignConfig.m_setNormalParam )
		{
			m_autoAssignConfig.m_normalParameterName = propertiesNormalList->GetStringSelection().wc_str();
		}
	}

	// Get normal param name
	{
		wxChoice *propertiesSpecularList = XRCCTRL( *this, "choiceSpecular", wxChoice );
		m_autoAssignConfig.m_setSpecularParam = (propertiesSpecularList->GetSelection() > 0);
		if ( m_autoAssignConfig.m_setSpecularParam )
		{
			m_autoAssignConfig.m_specularParameterName = propertiesSpecularList->GetStringSelection().wc_str();
		}
	}
	// Get mask param name
	{
		wxChoice *propertiesMaskList = XRCCTRL( *this, "choiceMask", wxChoice );
		m_autoAssignConfig.m_setMaskParam = (propertiesMaskList->GetSelection() > 0);
		if ( m_autoAssignConfig.m_setMaskParam )
		{
			m_autoAssignConfig.m_maskParameterName = propertiesMaskList->GetStringSelection().wc_str();
		}
	}
}

void CEdImportMeshDlg::OnOKAll( wxCommandEvent& event )
{
	SaveConfig();
	EndDialog( ECR_OKAll );
}

void CEdImportMeshDlg::OnOK( wxCommandEvent& event )
{
	SaveConfig();
	EndDialog( ECR_OK );
}

void CEdImportMeshDlg::OnSkip( wxCommandEvent& event )
{
	EndDialog( ECR_Skip );
}

void CEdImportMeshDlg::OnCancel( wxCommandEvent& event )
{
	EndDialog( ECR_Cancel );
}

void CEdImportMeshDlg::UpdateMaterialTextures()
{
	// Clear combo for diffuse texture
	wxChoice *propertiesDiffuseList = XRCCTRL( *this, "choiceDiffuse", wxChoice );
	propertiesDiffuseList->Clear();
	propertiesDiffuseList->Append( TXT("<none>") );

	// Clear combo for normal texture
	wxChoice *propertiesNormalList = XRCCTRL( *this, "choiceNormal", wxChoice );
	propertiesNormalList->Clear();
	propertiesNormalList->Append( TXT("<none>") );

	// Clear combo for specular texture
	wxChoice *propertiesSpecularList = XRCCTRL( *this, "choiceSpecular", wxChoice );
	propertiesSpecularList->Clear();
	propertiesSpecularList->Append( TXT("<none>") );

	// Clear combo for mask texture
	wxChoice *propertiesMaskList = XRCCTRL( *this, "choiceMask", wxChoice );
	propertiesMaskList->Clear();
	propertiesMaskList->Append( TXT("<none>") );

	// Load material
	IMaterial* baseMaterial = Cast< IMaterial >( GDepot->LoadResource( m_recentMaterial ) );
	if ( baseMaterial )
	{
		IMaterialDefinition* definition = Cast< IMaterialDefinition >( baseMaterial );
		if ( definition )
		{
			// Get parameters
			const IMaterialDefinition::TParameterArray &parameters = definition->GetPixelParameters();

			// Find the default parameters for defualt maps
			Int32 selectD = -1;
			Int32 selectN = -1;
			Int32 selectS = -1;	
			Int32 selectM = -1;	
			for( auto it=parameters.Begin(); it!=parameters.End(); it++ )
			{
				if ( it->m_type == IMaterialDefinition::PT_Texture )
				{
					// Add it to all lists
					Int32 di = propertiesDiffuseList->Append( it->m_name.AsString().AsChar() );
					Int32 ni = propertiesNormalList->Append( it->m_name.AsString().AsChar() );
					Int32 si = propertiesSpecularList->Append( it->m_name.AsString().AsChar() );
					Int32 mi = propertiesMaskList->Append( it->m_name.AsString().AsChar() );

					// Is this a diffuse map binding ?
					String name( it->m_name.AsString().ToLower() );
					if ( selectD == -1 && name.BeginsWith( TXT("diffuse") ) )
					{
						selectD = di;
					}

					// Is this a normal map binding ?
					if ( selectN == -1 && name.BeginsWith( TXT("normal") ) )
					{
						selectN = di;
					}

					// Is this a specular map binding ?
					if ( selectS == -1 && name.BeginsWith( TXT("specular") ) )
					{
						selectS = di;
					}
					// Is this a mask map binding ?
					if ( selectM == -1 && name.BeginsWith( TXT("mask") ) )
					{
						selectM = mi;
					}
				}
			}

			// Select in combo box
			propertiesDiffuseList->SetSelection( selectD > 0 ? selectD : 0 );
			propertiesNormalList->SetSelection( selectN > 0 ? selectN : 0 );
			propertiesSpecularList->SetSelection( selectS > 0 ? selectS : 0 );
			propertiesMaskList->SetSelection( selectM > 0 ? selectM : 0 );
		}
	}
}

void CEdImportMeshDlg::OnMaterialChanged( wxCommandEvent& event )
{
	wxNotebook* tabs = XRCCTRL( *this, "Tabs", wxNotebook );
	if ( tabs->GetSelection() == 0 )
	{
		// Recent list
		wxListBox *materialsList = XRCCTRL( *this, "RecentMaterials", wxListBox );
		m_recentMaterial = materialsList->GetStringSelection();
	}
	else
	{
		// All materials list
		wxListBox *materialsList = XRCCTRL( *this, "AllMaterials", wxListBox );
		m_recentMaterial = materialsList->GetStringSelection();
	}

	// Update textures
	UpdateMaterialTextures();
}

void CEdImportMeshDlg::OnText( wxCommandEvent& event )
{
	wxTextCtrl *textctrl = XRCCTRL( *this, "TextSearch", wxTextCtrl );
	wxListBox *materialsList = XRCCTRL( *this, "AllMaterials", wxListBox );
	wxString str = textctrl->GetValue();

	int foundindex = -1;
	for(unsigned int i=0;i<materialsList->GetCount();i++)
	{
		wxString str2 = materialsList->GetString(i);
		int p = str2.Find(str);
		if(p!=-1)
		{
			foundindex=i;
			break;
		}
	}
	if(foundindex!=-1)
	{
		materialsList->SetSelection(foundindex);

		wxNotebook* tabs = XRCCTRL( *this, "Tabs", wxNotebook );
		if ( tabs->GetSelection() == 0 )
		{
			// Recent list
			wxListBox *materialsList = XRCCTRL( *this, "RecentMaterials", wxListBox );
			m_recentMaterial = materialsList->GetStringSelection();
		}
		else
		{
			m_recentMaterial = materialsList->GetStringSelection();
		}

		UpdateMaterialTextures();
	}
}