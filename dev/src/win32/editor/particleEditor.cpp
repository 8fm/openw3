/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include <wx\display.h>
#include "particleEditor.h"
#include "particlePreviewPanel.h"
#include "particlePropertiesBrowser.h"
#include "shortcutsEditor.h"
#include "assetBrowser.h"
#include "emitterGraphEditor.h"
#include "..\..\common\engine\particleSystem.h"
#include "..\..\common\engine\particleEmitter.h"
#include "..\..\common\engine\curve.h"
#include "..\..\common\engine\renderCommands.h"

CEdParticleMaterialList::CEdParticleMaterialList( wxWindow* parent, CParticleSystem* particleSystem, CEdEmitterGraphEditor* graphEditor, CEdParticleEditor* editor )
	: CEdMaterialListManager( parent, particleSystem, TXT("fx\\shaders\\"), nullptr )
	, m_particleSystem( particleSystem )
	, m_graphEditor( graphEditor )
	, m_editor( editor )
{
	EnableListModification( false );
	EnableMaterialRemapping( false );
}

Int32 CEdParticleMaterialList::GetNumMaterials() const
{
	return m_particleSystem->GetEmitters().Size();
}

IMaterial* CEdParticleMaterialList::GetMaterial( Int32 index ) const
{
	const TDynArray< CParticleEmitter* >& emitters = m_particleSystem->GetEmitters();
	if ( index < emitters.SizeInt() )
	{
		return emitters[ index ]->GetMaterial();
	}
	else
	{
		return nullptr;
	}
}

String CEdParticleMaterialList::GetMaterialName( Int32 index ) const
{
	const TDynArray< CParticleEmitter* >& emitters = m_particleSystem->GetEmitters();
	if ( index < emitters.SizeInt() )
	{
		return emitters[ index ]->GetEditorName();
	}
	else
	{
		return nullptr;
	}
}

void CEdParticleMaterialList::SetMaterial( Int32 index, IMaterial* material )
{
	const TDynArray< CParticleEmitter* >& emitters = m_particleSystem->GetEmitters();
	if ( index < emitters.SizeInt() )
	{
		emitters[ index ]->SetMaterial( material );
	}
}

void CEdParticleMaterialList::HighlightMaterial( Int32 index, Bool state )
{
	const TDynArray< CParticleEmitter* >& emitters = m_particleSystem->GetEmitters();
	if ( index < emitters.SizeInt() )
	{
		m_graphEditor->Select( emitters[ index ], state );
		m_graphEditor->Refresh();
	}
}

void CEdParticleMaterialList::MaterialPropertyChanged( CName propertyName, Bool finished )
{
	if ( finished )
	{
		CDrawableComponent::RecreateProxiesOfRenderableComponents();
	}
	else
	{
		m_editor->RefreshPreviewRenderingProxy();
	}
}

#define ID_EDIT_ADD				0
#define ID_EDIT_CLONE			1
#define ID_EDIT_REMOVE			2
#define ID_EDIT_DEBUG			3

BEGIN_EVENT_TABLE( CEdParticleEditor, wxFrame )

	EVT_MENU( XRCID( "save" ), CEdParticleEditor::OnSave )
	EVT_MENU( XRCID( "restart" ), CEdParticleEditor::OnTimeRestart )
	EVT_MENU( XRCID( "showTarget" ), CEdParticleEditor::OnShowTarget )
	EVT_MENU( XRCID( "ResetLayout" ), CEdParticleEditor::OnResetLayout )
	EVT_MENU( XRCID( "SaveCustomLayout" ), CEdParticleEditor::SaveCustomLayout )
	EVT_MENU( XRCID( "LoadCustomLayout" ), CEdParticleEditor::LoadCustomLayout )
	EVT_TOOL( XRCID("play" ), CEdParticleEditor::OnPlayPause )

	EVT_BUTTON( XRCID("m_addLod"), CEdParticleEditor::OnAddLod )
	EVT_BUTTON( XRCID("m_removeLod"), CEdParticleEditor::OnRemoveLod )
	EVT_SPINCTRL( XRCID("m_lod"), CEdParticleEditor::OnSelectLod )
	EVT_TEXT( XRCID("m_lodDistance"), CEdParticleEditor::OnSetLodDistance )
END_EVENT_TABLE()

CEdParticleEditor::CEdParticleEditor( wxWindow* parent )
	: ISmartLayoutWindow( this )
	, m_properties( nullptr )
	, m_curveEditor( nullptr )
	, m_gradientEditor( nullptr )
	, m_particleSystem( nullptr )
	, m_internalMatSelection( false )
	, m_auiNotebook( nullptr )
{
	// Load designed frame from resource
	wxXmlResource::Get()->LoadFrame( this, parent, TEXT("ParticleEditor2") );
}
CEdParticleEditor::CEdParticleEditor( wxWindow* parent, CParticleSystem* particleSystem )
	: ISmartLayoutWindow( this )
	, m_properties( nullptr )
	, m_curveEditor( nullptr )
	, m_gradientEditor( nullptr )
	, m_particleSystem( nullptr )
	, m_internalMatSelection( false )
	, m_auiNotebook( nullptr )
	, m_auiManager( nullptr, wxAUI_MGR_DEFAULT | wxAUI_MGR_LIVE_RESIZE )
{
	// Load designed frame from resource
	wxXmlResource::Get()->LoadFrame( this, parent, TEXT("ParticleEditor2") );

	wxPanel* containerPanel = XRCCTRL( *this, "SmartContainerPanel", wxPanel );
	// Load icon
	wxIcon iconSmall;
	iconSmall.CopyFromBitmap( SEdResources::GetInstance().LoadBitmap( _T("IMG_TOOL") ) );
	SetIcon( iconSmall );

	// Assign particle system
	m_particleSystem = particleSystem;
	ASSERT( m_particleSystem );


	m_lodDistance = XRCCTRL( *this, "m_lodDistance", wxTextCtrl );
	m_lodDistance->SetValidator( wxTextValidator( wxFILTER_NUMERIC, &m_lodDistValue ) );

	RED_ASSERT( m_particleSystem->GetLODCount() > 0 );
	m_selectedLod = XRCCTRL( *this, "m_lod", wxSpinCtrl );
	m_selectedLod->SetMin( 0 );
	m_selectedLod->SetMax( Max( m_particleSystem->GetLODCount(), 1u ) - 1 );
	m_selectedLod->SetValue( 0 );

	// Set title
	String title = GetTitle().wc_str();
	SetTitle( String::Printf( TEXT("%s - %s"), title.AsChar(), particleSystem->GetFriendlyName().AsChar() ).AsChar() );

	// Create Gradient Editor, but do not show it
	{
		m_gradientEditor = new CEdGradientEditor( this );
		ASSERT( m_gradientEditor );
		m_gradientEditor->Hide();
	}

	// Create Curve Editor and place it in CurvePanel
	wxPanel* curvePanel = wxXmlResource::Get()->LoadPanel( containerPanel, wxT("CurvePanel") );
	{
		m_curveEditor = new CEdParticleCurveEditor( curvePanel, this );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		sizer1->Add( m_curveEditor, 1, wxEXPAND );
		curvePanel->SetSizer( sizer1 );
		curvePanel->Layout();
	}

	// Assign Curve Editor to Gradient Editor and Gradient Editor to Curve Editor
	{
		m_curveEditor->SetGradientEditor( m_gradientEditor );
	}

	// Create Particle Preview and place it in PreviewRenderPanel
	wxPanel* previewPanel = wxXmlResource::Get()->LoadPanel( containerPanel, wxT("PreviewRenderPanel") );
	m_particleToolbar = wxXmlResource::Get()->LoadToolBar( previewPanel, wxT( "ParticleToolbar" ) );
	{
		m_playIcon	= SEdResources::GetInstance().LoadBitmap( TEXT("IMG_CONTROL_PLAY") );
		m_pauseIcon = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_CONTROL_PAUSE") );

		m_previewPanel = new CEdParticlePreviewPanel( previewPanel, this );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		sizer1->Add( m_particleToolbar, 0, wxEXPAND );
		previewPanel->SetSizer( sizer1 );
		previewPanel->GetSizer()->Add( m_previewPanel, 1, wxEXPAND );
		previewPanel->Layout();
	}

	// Create EmitterGraphEditor and place it in EmitterGraphPanel
	wxPanel* emitterGraphPanel = wxXmlResource::Get()->LoadPanel( containerPanel, wxT("EmitterGraphPanel") );
	{
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );	
		m_emitterGraphEditor = new CEdEmitterGraphEditor( this, emitterGraphPanel, m_particleSystem );
		sizer1->Add( m_emitterGraphEditor, 1, wxEXPAND, 0 );
		emitterGraphPanel->SetSizer( sizer1 );
		emitterGraphPanel->Layout();
	}

	// Create System Properties Browser and place it in PropertiesPanel
	wxPanel* systemPropertiesTabPanel = wxXmlResource::Get()->LoadPanel( containerPanel, wxT("SystemPropertiesPanel") );
	{
		CEdPropertiesBrowserWithStatusbar* systemProperties = new CEdPropertiesBrowserWithStatusbar( systemPropertiesTabPanel, PropertiesPageSettings(), nullptr );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		systemProperties->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdParticleEditor::OnSystemPropertyChanged ), NULL, this );
		systemProperties->Get().SetObject( m_particleSystem );
		sizer1->Add( systemProperties, 1, wxEXPAND, 0 );
		systemPropertiesTabPanel->SetSizer( sizer1 );
		systemPropertiesTabPanel->Layout();
	}

	// Create Properties Browser and place it in PropertiesPanel
	wxPanel* emitterPropsTabPanel = wxXmlResource::Get()->LoadPanel( containerPanel, wxT("PropertiesPanel") );

	wxPanel* emitterPropsPanel = XRCCTRL( *emitterPropsTabPanel, "EmitterPropertiesPanel", wxPanel );
	{
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		{
			m_properties = new CEdPropertiesBrowserWithStatusbar( emitterPropsPanel, PropertiesPageSettings(), nullptr );
			m_properties->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdParticleEditor::OnPropertyChanged ), NULL, this );
			sizer1->Add( m_properties, 1, wxEXPAND, 0 );
		}
		emitterPropsPanel->SetSizer( sizer1 );
		emitterPropsPanel->Layout();
	}

	wxPanel* lodPropertiesPanel = XRCCTRL( *emitterPropsTabPanel, "EmitterLodPropertiesPanel", wxPanel );
	{
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		{
			m_lodProperties = new CEdPropertiesBrowserWithStatusbar( lodPropertiesPanel, PropertiesPageSettings(), nullptr );
			m_lodProperties->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdParticleEditor::OnLodPropertyChanged ), NULL, this );
			sizer1->Add( m_lodProperties, 1, wxEXPAND, 0 );
		}
		lodPropertiesPanel->SetSizer( sizer1 );
		lodPropertiesPanel->Layout();
	}

	// Setup material list
	wxPanel* materialTabPanel = wxXmlResource::Get()->LoadPanel( containerPanel, wxT("MaterialsPanel") );
	{
		materialTabPanel->SetSizer( new wxBoxSizer( wxVERTICAL ) );
		m_materialList = new CEdParticleMaterialList( materialTabPanel, m_particleSystem, m_emitterGraphEditor, this );
		materialTabPanel->GetSizer()->Add( m_materialList, 1, wxEXPAND );
		m_materialList->UpdateMaterialList();
	}

	// Bind to events
	SEvents::GetInstance().RegisterListener( CNAME( FileReload ), this );
	SEvents::GetInstance().RegisterListener( CNAME( FileReloadConfirm ), this );

	// Panel Layout
	{
		m_auiManager.SetManagedWindow( containerPanel );

		m_auiNotebook =  new wxAuiNotebook( containerPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_TOP| wxAUI_NB_SCROLL_BUTTONS | wxAUI_NB_TAB_EXTERNAL_MOVE | wxAUI_NB_TAB_MOVE | wxAUI_NB_TAB_SPLIT );
		m_auiNotebook->AddPage( systemPropertiesTabPanel, wxT( "System Properties" ), false );
		m_auiNotebook->AddPage( emitterPropsTabPanel, wxT( "Emitter Properties" ), true );
		m_auiNotebook->AddPage( materialTabPanel, wxT( "Material" ), false );

		m_auiManager.AddPane( previewPanel,  wxTOP | wxLEFT);
		m_auiManager.AddPane( curvePanel, wxCENTER );
		m_auiManager.AddPane( m_auiNotebook, wxBOTTOM );
		m_auiManager.AddPane( emitterGraphPanel, wxBOTTOM );

		m_auiManager.GetPane( previewPanel ).CloseButton( false ).Name( wxT( "Main" ) ).CaptionVisible( true ).BestSize( 800, 800 );
		m_auiManager.GetPane( m_auiNotebook ).CloseButton( false ).Name( wxT( "Properties" ) ).CaptionVisible( true ).BestSize( 400, 400 );
		m_auiManager.GetPane( emitterGraphPanel ).CloseButton( false ).Name( wxT( "Emitter" ) ).CaptionVisible( true ).BestSize( 400, 400 );
		m_auiManager.GetPane( curvePanel ).CloseButton( false ).Name( wxT( "Curves" ) ).CaptionVisible( true ).BestSize( 1000, 400 );
	}

	//m_previewPanel->ChangeEnvironment(1);

	// Update and finalize layout
	m_auiManager.Update();
	Layout();

	wxCommandEvent dummyEvent;

	m_defaultLayout = SaveDefaultLayout( dummyEvent );
	LoadCustomLayout( dummyEvent );


	LoadOptionsFromConfig();
	Show();

	// Update graph editor
	UpdateGraphEditor(true);

	UpdateLodProperties();
}

CEdParticleEditor::~CEdParticleEditor()
{
	m_auiManager.UnInit();

	SEvents::GetInstance().UnregisterListener( this );

	SaveOptionsToConfig();
}

void CEdParticleEditor::SaveOptionsToConfig()
{
	SaveLayout(TXT("/Frames/ParticleEditor"));
}

void CEdParticleEditor::LoadOptionsFromConfig()
{
    LoadLayout(TXT("/Frames/ParticleEditor"));

	// Make the particle start when the editor is loaded.
	m_particleSystem->ResetInstances();

}

void CEdParticleEditor::OnSave( wxCommandEvent& event )
{
	m_particleSystem->Save();
}

void CEdParticleEditor::OnPlayPause( wxCommandEvent& event )
{
	if ( m_previewPanel->GetTimeMultiplier() == 0.0f )
	{
		m_previewPanel->SetTimeMultiplier( 1.0f );
	}
	else
	{
		m_previewPanel->SetTimeMultiplier( 0.0f );
	}
	UpdatePlayPauseButton();
}

void CEdParticleEditor::UpdatePlayPauseButton()
{
	const wxBitmap & icon = m_previewPanel->IsPause() ? m_playIcon : m_pauseIcon;
	m_particleToolbar->SetToolNormalBitmap( XRCID("play"), icon );
}

void CEdParticleEditor::OnTimeRestart( wxCommandEvent& event )
{
	m_particleSystem->ResetInstances();
	m_previewPanel->SetTimeMultiplier( 1.0f );
	UpdatePlayPauseButton();
}

void CEdParticleEditor::OnTimeNormal( wxCommandEvent& event )
{
	m_previewPanel->SetTimeMultiplier( 1.0f );
}
void CEdParticleEditor::OnTimePause( wxCommandEvent& event )
{
	m_previewPanel->SetTimeMultiplier( 0.0f );
}
void CEdParticleEditor::OnTimeSpeedup( wxCommandEvent& event )
{
	m_previewPanel->SetTimeMultiplier( 2.0f * m_previewPanel->GetTimeMultiplier() );
}
void CEdParticleEditor::OnTimeSlowdown( wxCommandEvent& event )
{
	m_previewPanel->SetTimeMultiplier( 0.5f * m_previewPanel->GetTimeMultiplier() );
}

void CEdParticleEditor::UpdateGraphEditor(Bool fitToClient)
{
	ASSERT(m_emitterGraphEditor);
	m_emitterGraphEditor->UpdateLayout();

	if (fitToClient)
	{
		m_emitterGraphEditor->ScaleToClientView();
	}
}

void CEdParticleEditor::UpdateProperties()
{
	if ( m_internalMatSelection )
	{
		return;
	}

	ASSERT(m_emitterGraphEditor);
	IParticleModule *module = m_emitterGraphEditor->GetEditedModule();
	if ( module && !module->IsA<CParticleEmitter>() )
	{
		m_auiNotebook->SetSelection( 1 );
	}
	
	m_properties->Get().SetObject(module);
	if ( module )
	{
		m_internalMatSelection = true;
		m_materialList->SelectByName( module->GetEmitter()->GetEditorName() );
		m_internalMatSelection = false;
	}
}

void CEdParticleEditor::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( FileReloadConfirm ) )
	{
		CResource* res = GetEventData< CResource* >( data );
		if (res==m_particleSystem)
		{
			SEvents::GetInstance().QueueEvent( CNAME( FileReloadToConfirm ), CreateEventData( CReloadFileInfo( res, NULL, GetTitle().wc_str() ) ) );
		}
	}
	if ( name == CNAME( FileReload ) )
	{
		const CReloadFileInfo& reloadInfo = GetEventData< CReloadFileInfo >( data );
		if ( reloadInfo.m_newResource->IsA<CParticleSystem>() )
		{
			CParticleSystem* oldParticle = (CParticleSystem*)(reloadInfo.m_oldResource);
			CParticleSystem* newParticle = (CParticleSystem*)(reloadInfo.m_newResource);
			if ( oldParticle == m_particleSystem )
			{
				m_particleSystem = newParticle;

				m_properties->Get().SetObject( m_particleSystem );

				UpdateGraphEditor(true);

				m_previewPanel->Reload();

				wxTheFrame->GetAssetBrowser()->OnEditorReload( m_particleSystem, this );
			}
		}
	}
}

void CEdParticleEditor::OnSystemPropertyChanged( wxCommandEvent& event )
{
	UpdateGraphEditor(false);

	ResetAllEmitters();
}

void CEdParticleEditor::OnPropertyChanged( wxCommandEvent& event )
{
	UpdateGraphEditor(false);

	ResetSelectedEmitter();
}

void CEdParticleEditor::EditCurve( CurveParameter* curve, const String& moduleName, Bool pinned )
{
	if ( curve )
	{
		const bool ctrl = RIM_IS_KEY_DOWN( IK_Ctrl );
		if ( curve->GetCurveCount() == 4 && ctrl )
		{
			// Edit as gradient
			m_gradientEditor->SetCurves( curve->GetCurve( 0 ), curve->GetCurve( 1 ), curve->GetCurve( 2 ), curve->GetCurve( 3 ) );
			m_gradientEditor->Show();
		}
		else
		{
			// Add as normal curve the curve editor 
			m_curveEditor->AddCurve( curve, moduleName, pinned );
		}
	}
}

void CEdParticleEditor::OnShowTarget( wxCommandEvent& event )
{
	m_previewPanel->SetTargetVisible( event.IsChecked() );
}

void CEdParticleEditor::OnEmitterAdded( CParticleEmitter* emitter )
{
	m_previewPanel->OnUpdateEmitter( emitter );
	m_materialList->UpdateMaterialList();
}

void CEdParticleEditor::OnEmitterRemove( CParticleEmitter* emitter )
{
	// Do the module cleanup stuff
	OnModuleRemove( emitter );

	// Do the child modules cleanup stuff
	for ( Uint32 i=0; i<emitter->GetModules().Size(); ++i )
	{
		OnModuleRemove( emitter->GetModules()[i] );
	}

	// Do the rendering update
	m_previewPanel->OnRemoveEmitter( emitter );

	// Update material list later, when the emitter will be actually removed from the system
	RunLaterOnce( [ this ]() { m_materialList->UpdateMaterialList(); } );
}

void CEdParticleEditor::OnEmitterChanged( CParticleEmitter* emitter )
{
	m_previewPanel->OnUpdateEmitter( emitter );
}

void CEdParticleEditor::OnModuleRemove( IParticleModule* module )
{
	TDynArray< CurveParameter* > curveParams;
	module->GetCurves( curveParams );

	// Clean curves editor of this module's curves
	for ( Uint32 i=0; i<curveParams.Size(); ++i )
	{
		CurveParameter* curveParameter = curveParams[i];
		for ( Uint32 j=0; j<curveParameter->GetCurveCount(); ++j )
		{
			m_curveEditor->RemoveCurve( &curveParameter->GetCurve( j )->GetCurveData() );
		}
	}
}

void CEdParticleEditor::DeleteAllCurveGroups()
{
	m_curveEditor->RemoveAllCurveGroups();
}

void CEdParticleEditor::DeleteCurveGroup( const String moduleName )
{
	CName modName = CName(moduleName);
	m_curveEditor->RemoveCurveGroup( modName );
}

void CEdParticleEditor::OnCurvesChanged( const TDynArray< CCurve* >& curves )
{
	TDynArray< CParticleEmitter* > emittersChanged;
	for ( Uint32 i=0; i<curves.Size(); ++i )
	{
		CParticleEmitter* parentEmitter = curves[i]->FindParent< CParticleEmitter >();
		ASSERT( parentEmitter );
		ASSERT( m_particleSystem->GetEmitters().Exist( parentEmitter ) );
		emittersChanged.PushBackUnique( parentEmitter );
	}

	for ( Uint32 i=0; i<emittersChanged.Size(); ++i )
	{
		OnEmitterChanged( emittersChanged[i] );
	}
}

void CEdParticleEditor::OnResetLayout( wxCommandEvent& event )
{
	m_auiManager.LoadPerspective( m_defaultLayout.AsChar(), true );
}

String CEdParticleEditor::SaveDefaultLayout( wxCommandEvent& event )
{
	return m_auiManager.SavePerspective().wc_str();
}

void CEdParticleEditor::SaveCustomLayout( wxCommandEvent& event )
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/ParticleEditor") );

	String layoutPerspective = m_auiManager.SavePerspective().wc_str();
	config.Write( TXT("custom_layout"), layoutPerspective );
}


void CEdParticleEditor::LoadCustomLayout( wxCommandEvent& event )
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/ParticleEditor") );

	String layoutPerspective = config.Read( TXT("custom_layout"), String::EMPTY );

	if ( !layoutPerspective.Empty() )
	{
		m_auiManager.LoadPerspective( layoutPerspective.AsChar(), true );
	}
}

wxString CEdParticleEditor::GetShortTitle() 
{ 
	return m_particleSystem->GetFile()->GetFileName().AsChar() + wxString(TXT(" - Particle Editor")); 
}



void CEdParticleEditor::UpdateLodProperties()
{
	Uint32 lodIndex = (Uint32)m_selectedLod->GetValue();

	ASSERT(m_emitterGraphEditor);
	CParticleEmitter* emitter = Cast< CParticleEmitter >( m_emitterGraphEditor->GetEditedModule() );
	RED_ASSERT( emitter == nullptr || lodIndex < emitter->GetLODCount(), TXT("LOD index %u >= LOD count %u"), lodIndex, emitter->GetLODCount() );
	if ( emitter != nullptr && lodIndex < emitter->GetLODCount() )
	{
		SParticleEmitterLODLevel& emitterLod = emitter->GetLOD( lodIndex );
		STypedObject obj( &emitterLod, ClassID< SParticleEmitterLODLevel >() );
		m_lodProperties->Get().SetTypedObject( obj );
	}
	else
	{
		m_lodProperties->Get().SetNoObject();
	}
	// SParticleEmitterLODLevel is not a CObject, so we won't be able to instantiate any IFloatEvaluator for m_birthRate. So, we give the
	// emitter itself to the property page to use as a parent object.
	m_lodProperties->Get().SetDefaultParentObject( emitter );

	RED_ASSERT( lodIndex < m_particleSystem->GetLODCount(), TXT("LOD index %u >= LOD count %u"), lodIndex, m_particleSystem->GetLODCount() );
	if ( lodIndex < m_particleSystem->GetLODCount() )
	{
		const SParticleSystemLODLevel& systemLod = m_particleSystem->GetLOD( lodIndex );
		m_lodDistance->SetValue( ToString( systemLod.m_distance ).AsChar() );
	}
	XRCCTRL( *this, "m_numLods", wxStaticText )->SetLabel( wxString::Format( "(%u)", m_particleSystem->GetLODCount() ) );
}

void CEdParticleEditor::OnLodPropertyChanged( wxCommandEvent& event )
{
	ResetSelectedEmitter();
}

void CEdParticleEditor::OnSelectLod( wxSpinEvent& event )
{
	UpdateLodProperties();
	UpdateGraphEditor( false );

	( new CRenderCommand_ChangeSceneForcedLOD( m_previewPanel->GetPreviewWorld()->GetRenderSceneEx(), GetEditingLOD() ) )->Commit();
}

void CEdParticleEditor::OnSetLodDistance( wxCommandEvent& event )
{
	String distanceString = m_lodDistance->GetValue();

	Uint32 lodIndex = GetEditingLOD();
	RED_ASSERT( lodIndex < m_particleSystem->GetLODCount(), TXT("LOD index %u >= LOD count %u"), lodIndex, m_particleSystem->GetLODCount() );
	if ( lodIndex < m_particleSystem->GetLODCount() )
	{
		SParticleSystemLODLevel& systemLod = m_particleSystem->GetLOD( GetEditingLOD() );

		FromString( distanceString, systemLod.m_distance );
	}
}


void CEdParticleEditor::OnAddLod( wxCommandEvent& event )
{
	m_particleSystem->AddLOD();
	m_selectedLod->SetMax( m_particleSystem->GetLODCount() - 1 );
	m_selectedLod->SetValue( m_particleSystem->GetLODCount() - 1 );
	OnSelectLod( wxSpinEvent() );

	ResetAllEmitters();
}

void CEdParticleEditor::OnRemoveLod( wxCommandEvent& event )
{
	if ( m_particleSystem->GetLODCount() <= 1 )
	{
		return;
	}

	m_particleSystem->RemoveLOD( m_selectedLod->GetValue() );
	m_selectedLod->SetMax( m_particleSystem->GetLODCount() - 1 );
	OnSelectLod( wxSpinEvent() );

	ResetAllEmitters();
}


Uint32 CEdParticleEditor::GetEditingLOD() const
{
	return (Uint32)m_selectedLod->GetValue();
}


void CEdParticleEditor::ResetAllEmitters()
{
	for ( CParticleEmitter* emitter : m_particleSystem->GetEmitters() )
	{
		OnEmitterChanged( emitter );
		emitter->ResetInstances();
	}
}


void CEdParticleEditor::ResetSelectedEmitter()
{
	IParticleModule* module = m_emitterGraphEditor->GetEditedModule();

	if ( module == nullptr  && m_properties->Get().GetRootItem() != nullptr )
	{
		module = m_properties->Get().GetRootItem()->GetRootObject( 0 ).As< IParticleModule >();
	}

	if ( module == nullptr )
	{
		return;
	}

	CParticleEmitter* emitter = Cast< CParticleEmitter >( module );
	if ( emitter == nullptr )
	{
		emitter = module->FindParent< CParticleEmitter >();
	}

	if ( emitter != nullptr )
	{
		OnEmitterChanged( emitter );
		emitter->ResetInstances();
	}
}

void CEdParticleEditor::RefreshPreviewRenderingProxy()
{
	m_previewPanel->GetParticleComponent()->RefreshRenderProxies();
}
