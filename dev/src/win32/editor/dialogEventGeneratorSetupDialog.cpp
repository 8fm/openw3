/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "dialogEventGeneratorSetupDialog.h"
#include "dialogEventGenerator.h"


IMPLEMENT_ENGINE_CLASS( CDialogEventGeneratorConfig );
IMPLEMENT_ENGINE_CLASS( SStorySceneActorAnimationGenData );



BEGIN_EVENT_TABLE( CEdStorySceneEventGeneratorSetupDialog, wxDialog )
	EVT_BUTTON( XRCID( "GenerateButton"), CEdStorySceneEventGeneratorSetupDialog::OnGenerateClick )
	EVT_BUTTON( XRCID( "CancelButton"), CEdStorySceneEventGeneratorSetupDialog::OnCancelClick )
END_EVENT_TABLE()

CEdStorySceneEventGeneratorSetupDialog::CEdStorySceneEventGeneratorSetupDialog( CEdSceneEditor* parent )
	: m_mediator( parent )
{
	wxXmlResource::Get()->LoadDialog( this, parent, TEXT( "DialogGeneratorSetup" ) );

	m_cameraEventsCheckBox		= XRCCTRL( *this, "CameraEventsCheckBox", wxCheckBox );
	m_cameraShakeCheckBox		= XRCCTRL( *this, "CameraShakeCheckBox", wxCheckBox );
	m_lookatEventsCheckBox		= XRCCTRL( *this, "LookAtEventsCheckBox", wxCheckBox );
	m_animEventsCheckBox		= XRCCTRL( *this, "AnimEventsCheckBox", wxCheckBox );
	m_mimicEventsCheckBox		= XRCCTRL( *this, "MimicEventsCheckBox", wxCheckBox );
	m_preserveExistingCheckBox	= XRCCTRL( *this, "PreserveExistingCheckbox", wxCheckBox );

	m_scopeRadioBox				= XRCCTRL( *this, "ScopeRadioBox", wxRadioBox );


	// Create properties browser and quest graph editor and split them
	PropertiesPageSettings settings;
	wxPanel* propertiesPanel = XRCCTRL( *this, "PropertiesPanel", wxPanel );
	m_propertiesBrowser = new CEdPropertiesPage( propertiesPanel, settings, nullptr );	
	m_propertiesBrowser->Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdStorySceneEventGeneratorSetupDialog::OnPropertiesChanged ), NULL, this );
	propertiesPanel->GetSizer()->Add( m_propertiesBrowser, 1, wxEXPAND | wxALL );

	LoadOptionsFromConfig();
	m_propertiesBrowser->SetObject<CDialogEventGeneratorConfig>( &m_config );
}

void CEdStorySceneEventGeneratorSetupDialog::ReloadActorStatus()
{
	const CStorySceneDialogsetInstance* dialogset = m_mediator->GetCurrentDialogsetInstance();
	if ( dialogset )
	{
		m_config.m_actorState.ClearFast();
		const TDynArray<CStorySceneDialogsetSlot*>& slots = dialogset->GetSlots();
		m_config.m_actorState.Resize( slots.Size() );
		for ( Uint32 i = 0; i < slots.Size(); ++i )
		{
			m_config.m_actorState[i].m_actorName = slots[i]->GetActorName(); 
			m_config.m_actorState[i].m_emotionalState = slots[i]->GetBodyFilterEmotionalState(); 
			m_config.m_actorState[i].m_poseType = slots[i]->GetBodyFilterPoseName();
			m_config.m_actorState[i].m_status = slots[i]->GetBodyFilterStatus();						
		}
	}
	m_propertiesBrowser->RefreshValues();
};

void CEdStorySceneEventGeneratorSetupDialog::OnGenerateClick( wxCommandEvent& event )
{

	m_config.m_localScope = m_scopeRadioBox->GetSelection() == 0;	
	m_config.m_generateCam = m_cameraEventsCheckBox->IsChecked();
	m_config.m_generateShake = m_cameraShakeCheckBox->IsChecked();	
	m_config.m_generateLookats = m_lookatEventsCheckBox->IsChecked();
	m_config.m_generateAnim = m_animEventsCheckBox->IsChecked();
	m_config.m_generateMimic = m_mimicEventsCheckBox->IsChecked();
	m_config.m_preserveExisting = m_preserveExistingCheckBox->IsChecked();
	m_mediator->OnEvtGenerator_RunEventsGenerator( m_config );
}

void CEdStorySceneEventGeneratorSetupDialog::OnCancelClick( wxCommandEvent& event )
{
	SaveOptionsToConfig();
	Close();
}

//conf ig page edited
void  CEdStorySceneEventGeneratorSetupDialog::OnPropertiesChanged( wxCommandEvent& event )
{
	CEdPropertiesPage::SPropertyEventData* eventData = static_cast<CEdPropertiesPage::SPropertyEventData*>( event.GetClientData() );
	String propName = eventData->m_propertyName.AsString();
	if ( propName == TXT("overrideDialogsetData") )
	{
	}
};




//Saving state of stuff 
//////////////////////////////////////////////////////////////////////////////////////////////
void CEdStorySceneEventGeneratorSetupDialog::SaveSession( CConfigurationManager &config )
{
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/SceneScriptEditor/EventGenerator") );

	config.Write( TXT( "CameraEvents" ),	m_cameraEventsCheckBox->IsChecked() ? 1 : 0 );
	config.Write( TXT( "CameraShake" ),		m_cameraShakeCheckBox->IsChecked() ? 1 : 0 );
	config.Write( TXT( "LookAtEvents" ),	m_lookatEventsCheckBox->IsChecked() ? 1 : 0 );
	config.Write( TXT( "AnimEvents" ),		m_animEventsCheckBox->IsChecked() ? 1 : 0 );
	config.Write( TXT( "MimicEvents" ),		m_mimicEventsCheckBox->IsChecked() ? 1 : 0 );
	config.Write( TXT( "PreserveExistingCheckBox" ), m_preserveExistingCheckBox->IsChecked() ? 1 : 0 );
	config.Write( TXT( "Scope" ),			m_scopeRadioBox->GetSelection() );

	wxSize windowSize = GetSize();
	config.Write( TXT( "WindowSizeX" ),		windowSize.x );
	config.Write( TXT( "WindowSizeY" ),		windowSize.y );
}

void CEdStorySceneEventGeneratorSetupDialog::LoadOptionsFromConfig()
{
	ISavableToConfig::RestoreSession();
}

void CEdStorySceneEventGeneratorSetupDialog::RestoreSession( CConfigurationManager &config )
{
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/SceneScriptEditor/EventGenerator") );

	m_cameraEventsCheckBox->SetValue( config.Read( TXT( "CameraEvents" ), 0 ) == 1 );
	m_cameraShakeCheckBox->SetValue( config.Read( TXT( "CameraShake" ), 0 ) == 1 );
	m_lookatEventsCheckBox->SetValue( config.Read( TXT( "LookAtEvents" ), 0 ) == 1 );
	m_animEventsCheckBox->SetValue( config.Read( TXT( "AnimEvents" ), 0 ) == 1 );
	m_mimicEventsCheckBox->SetValue( config.Read( TXT( "MimicEvents" ), 0 ) == 1 );
	m_preserveExistingCheckBox->SetValue( config.Read( TXT( "PreserveExistingCheckBox" ), 0 ) == 1 );
	m_scopeRadioBox->SetSelection( config.Read( TXT( "Scope" ),	0 ) );

	wxSize windowSize = GetSize();
	windowSize.x = config.Read( TXT( "WindowSizeX" ), windowSize.x );
	windowSize.y = config.Read( TXT( "WindowSizeY" ), windowSize.y );
	SetSize( windowSize );
}

void CEdStorySceneEventGeneratorSetupDialog::SaveOptionsToConfig()
{
	ISavableToConfig::SaveSession();
}

