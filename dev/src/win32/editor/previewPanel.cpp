/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "filterPanel.h"
#include "editorExternalResources.h"
#include "previewWorld.h"
#include "../../common/engine/mesh.h"
#include "../../common/core/depot.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/core/directory.h"
#include "../../common/engine/environmentManager.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/renderFrame.h"
#include "../../common/engine/environmentDefinition.h"
#include "../../common/engine/fonts.h"
#include "../../common/engine/renderProxy.h"
#include "../../common/engine/worldTick.h"
#include "../../common/engine/renderFrameInfo.h"

CEdPreviewPanel::CEdPreviewPanel( wxWindow* parent, Bool allowRenderOptionsChange, Bool allowLocking )
	: CEdRenderingPanel( parent )	
	, m_windIndicatorScale( 1.0f )
	, m_lockCheckbox( nullptr )
{
	// Take rendering mask from editor
	m_renderingWindow->GetViewport()->ClearRenderingMask( SHOW_ALL_FLAGS );
	m_renderingWindow->GetViewport()->SetRenderingMask( wxTheFrame->GetFilterPanel()->GetViewportFlags( VFT_PREVIEW ) );
	m_renderingWindow->GetViewport()->SetRenderingMask( SHOW_Grid );

	// Automatic redraw
	GetViewport()->SetAutoRedraw( true );

	// Create fake world
	m_previewWorld = CreateObject< CEdPreviewWorld >();
	m_previewWorld->AddToRootSet();

	WorldInitInfo initInfo;
	initInfo.m_previewWorld = true;
	m_previewWorld->Init( initInfo );

	m_selectionManager = m_previewWorld->GetSelectionManager(); // TODO: create in-place
	m_transformManager = new CNodeTransformManager( m_previewWorld, m_selectionManager );
		
	// Set environment
	CEnvironmentManager *envManager = m_previewWorld->GetEnvironmentManager();
	if  ( envManager )
	{		
		// default off-env definition
		CEnvironmentDefinition* offEnv = new CEnvironmentDefinition();		
		m_envDefs.PushBack( offEnv );

		// allow to switch envs
		// Obtain files
		CDirectory* rootDir = GDepot->FindPath( WORLDENV_DEFINITIONS_ROOT );
		if ( rootDir )
		{
			envManager->SearchForAllAreaEnvs( rootDir, m_envDefs );
		}

		// Prevent GC from deleting these
		for ( auto it = m_envDefs.Begin(); it != m_envDefs.End(); ++it )
		{
			(*it)->AddToRootSet();
		}
		
		CGameEnvironmentParams params = envManager->GetGameEnvironmentParams();
		params.m_dayCycleOverride.m_enableCustomSunRotation = true;
		params.m_dayCycleOverride.m_customSunRotation = EulerAngles( 0.0f, 30.0f, 135.0f );
		
		params.m_displaySettings.m_allowGlobalFog = true;
		params.m_displaySettings.m_allowBloom = true;
		params.m_displaySettings.m_disableTonemapping = true;

		params.m_displaySettings.m_allowDOF = false;
		params.m_displaySettings.m_allowCloudsShadow = false;
		params.m_displaySettings.m_allowVignette = false;
		params.m_displaySettings.m_forceCutsceneDofMode = false;
		params.m_displaySettings.m_displayMode = EMM_None;

		envManager->SetGameEnvironmentParams( params );
	}
	
	if( allowRenderOptionsChange )
	{
		Int32 sunRotationYaw = (envManager ? envManager->GetGameEnvironmentParams().m_dayCycleOverride.m_customSunRotation.Yaw : 0.f);

		wxPanel* panel = new wxPanel( this );
		m_sizer->Add( panel, 0, wxEXPAND, 0 );

		wxBoxSizer *sizer = new wxBoxSizer( wxHORIZONTAL );
		panel->SetSizer( sizer );

		m_lightPositionSlider = new wxSlider( panel, -1, sunRotationYaw, 0, 359 );
		m_shadowsCheckbox = new wxCheckBox( panel, -1, TXT( "" ) );
		m_shadowsCheckbox->SetValue( true );
		m_fovSpin = new wxSpinCtrl( panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 50.0f, -1.0f ), wxSP_ARROW_KEYS, 1, 100, 70 );
				
		m_envChoice = new wxChoice( panel, -1 );
		m_envChoice->Append( TXT("ENV - OFF") );

		for(Uint32 i=1; i<m_envDefs.Size(); ++i)
		{
			String envName = m_envDefs[i]->GetFriendlyName().StringAfter(WORLDENV_DEFINITIONS_ROOT);
			envName.RemoveWhiteSpacesAndQuotes();
			m_envChoice->Append( envName.AsChar() );
		}
		m_envChoice->SetSelection(0);
		m_envChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdPreviewPanel::OnChoiceChanged ), NULL, this );

		m_lightPositionSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxCommandEventHandler( CEdPreviewPanel::OnLightPosChanged ), NULL, this );
		m_lightPositionSlider->Connect( wxEVT_SCROLL_CHANGED, wxCommandEventHandler( CEdPreviewPanel::OnLightPosChanged ), NULL, this );

		m_shadowsCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdPreviewPanel::OnShadowsChanged ), NULL, this );

		m_fovSpin->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxCommandEventHandler( CEdPreviewPanel::OnFovChanged ), NULL, this );
		m_fovSpin->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CEdPreviewPanel::OnFovChanged ), NULL, this );
		
		sizer->Add( new wxStaticText( panel, -1, TXT("Shadows: ") ), 0, wxTOP|wxBOTTOM|wxLEFT|wxRIGHT | wxALIGN_CENTER_VERTICAL, 5 );
		sizer->Add( m_shadowsCheckbox, 0, wxTOP|wxBOTTOM|wxRIGHT | wxALIGN_CENTER_VERTICAL, 5 );
		sizer->Add( new wxStaticText( panel, -1, TXT("FOV: ") ), 0, wxTOP|wxBOTTOM|wxLEFT | wxALIGN_CENTER_VERTICAL, 5 );
		sizer->Add( m_fovSpin, 0, wxTOP|wxBOTTOM | wxALIGN_CENTER_VERTICAL, 5 );
		sizer->Add( new wxStaticText( panel, -1, TXT("Light position: ") ), 0, wxTOP|wxBOTTOM|wxLEFT | wxALIGN_CENTER_VERTICAL, 5 );
		sizer->Add( m_lightPositionSlider, 0, wxTOP|wxBOTTOM|wxRIGHT | wxEXPAND | wxALIGN_CENTER_VERTICAL, 5 );
		sizer->Add( m_envChoice, 0, wxTOP|wxBOTTOM|wxRIGHT | wxALIGN_CENTER_VERTICAL, 5 );

		if ( allowLocking )
		{
			m_lockCheckbox = new wxCheckBox( panel, wxID_ANY, wxT("Lock") );
			sizer->Add( m_lockCheckbox, 0, wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
		}

		m_sizer->Layout();
	}
	else
	{
		m_lightPositionSlider = NULL;
		m_shadowsCheckbox = NULL;
		m_fovSpin = nullptr;
	}

    m_font = SafeCast< CFont >( GDepot->LoadResource( DEFAULT_FONT ) );
	ASSERT( m_font );
	m_font->AddToRootSet();

	wxTheFrame->GetFilterPanel()->RegisterPreviewPanel( this );
	wxTheFrame->GetFilterPanel()->UpdatePreviewPanel( this );

	SetLightPosition( 0 );

	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	RestoreSession( config );
}

void CEdPreviewPanel::SetShadowsEnabled( bool enable )
{
	if( GetShadowsEnabled() != enable )
	{
		if( enable )
		{
			GetViewport()->SetRenderingMask( SHOW_Shadows );
		}
		else
		{
			GetViewport()->ClearRenderingMask( SHOW_Shadows );
		}
		if( m_shadowsCheckbox )
		{
			m_shadowsCheckbox->SetValue( enable );
		}
	}
}

Bool CEdPreviewPanel::GetShadowsEnabled()
{
	return GetViewport()->GetRenderingMask()[ SHOW_Shadows ];
}

void CEdPreviewPanel::SetLightPosition( Int32 pos )
{
	if( GetLightPosition() != pos )
	{
		CEnvironmentManager *envManager = m_previewWorld->GetEnvironmentManager();
		if  ( envManager )
		{
			CGameEnvironmentParams params = envManager->GetGameEnvironmentParams();
			params.m_dayCycleOverride.m_enableCustomSunRotation = true;
			params.m_dayCycleOverride.m_customSunRotation = EulerAngles( 0.0f, 30.0f, pos );		
			envManager->SetGameEnvironmentParams( params );
		}

		if( m_lightPositionSlider )
		{
			m_lightPositionSlider->SetValue( pos );
		}
	}
}

Int32 CEdPreviewPanel::GetLightPosition()
{
	Int32 position = 0;

	CEnvironmentManager *envManager = m_previewWorld->GetEnvironmentManager();
	if  ( envManager )
	{
		position = envManager->GetGameEnvironmentParams().m_dayCycleOverride.m_customSunRotation.Yaw;
	}

	return position;
}

void CEdPreviewPanel::OnLightPosChanged( wxCommandEvent &event )
{
	SetLightPosition( m_lightPositionSlider->GetValue() );
}

void CEdPreviewPanel::OnShadowsChanged( wxCommandEvent &event )
{
	SetShadowsEnabled( m_shadowsCheckbox->IsChecked() );
}

CEdPreviewPanel::~CEdPreviewPanel()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	SaveSession( config );

	for ( auto it = m_envDefs.Begin(); it != m_envDefs.End(); ++it )
	{
		(*it)->RemoveFromRootSet();
	}
	m_envDefs.Clear();

	m_font->RemoveFromRootSet();

	if ( wxTheFrame )
		wxTheFrame->GetFilterPanel()->UnregisterPreviewPanel( this );

	if ( m_previewWorld )
	{
		m_previewWorld->Shutdown();
		m_previewWorld->RemoveFromRootSet();
		m_previewWorld = NULL;
	}

	if ( m_transformManager )
	{
		delete m_transformManager;
		m_transformManager = nullptr;
	}
}

CWorld* CEdPreviewPanel::GetWorld()
{
	return m_previewWorld;
}

CSelectionManager* CEdPreviewPanel::GetSelectionManager()
{
	return m_selectionManager;
}

CNodeTransformManager* CEdPreviewPanel::GetTransformManager()
{
	return m_transformManager;
}

CRenderFrame *CEdPreviewPanel::OnViewportCreateFrame( IViewport *view )
{
	if ( m_previewWorld )
	{
		CRenderFrameInfo info( view );
		return m_previewWorld->GenerateFrame( view, info );
	}
	else
	{
		return IViewportHook::OnViewportCreateFrame( view );
	}
}

void CEdPreviewPanel::OnViewportRenderFrame( IViewport *view, CRenderFrame *frame )
{
	CRenderFrameInfo& frameInfo = const_cast< CRenderFrameInfo& >( frame->GetFrameInfo() );
	frameInfo.m_envParametersGame.m_displaySettings.m_enableInstantAdaptation = true;
	frameInfo.m_envParametersGame.m_displaySettings.m_disableTonemapping = true;
	
	if ( m_previewWorld )
	{
		m_previewWorld->RenderWorld( frame );
		m_previewWorld->UpdateCameraPosition( frame->GetFrameInfo().m_camera.GetPosition() );
		m_previewWorld->UpdateCameraForward( frame->GetFrameInfo().m_camera.GetCameraForward() );
		m_previewWorld->UpdateCameraUp( frame->GetFrameInfo().m_camera.GetCameraUp() );
		m_previewWorld->SetStreamingReferencePosition( frame->GetFrameInfo().m_camera.GetPosition() );
	}
	else
	{
		IViewportHook::OnViewportRenderFrame( view, frame );
	}
}

Bool CEdPreviewPanel::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	if( ( key == IK_ScrollLock ) && ( action == IACT_Press ) )
	{
		SGarbageCollector::GetInstance().CollectNow();

		return true;
	}

	return CEdRenderingPanel::OnViewportInput( view, key, action, data );
}

void CEdPreviewPanel::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	// Toggle grid
	if ( ShouldDrawGrid() )
	{
		m_renderingWindow->GetViewport()->SetRenderingMask( SHOW_Grid );
	}
	else
	{
		m_renderingWindow->GetViewport()->ClearRenderingMask( SHOW_Grid );
	}

	IRenderScene* scene = m_previewWorld->GetRenderSceneEx();
	if ( scene )
	{
		SceneRenderingStats stats = scene->GetRenderStats();

		Uint32 x = 30;

		// Draw rendering stats
		if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_TriangleStats ) )
		{
			// Start at bottom
			Uint32 y = frame->GetFrameOverlayInfo().m_height - 130;

			//dex++:	slightly changed
			//kk		even more slightly changed
			frame->AddDebugScreenText( x, y, String::Printf( TXT("Scene triangles: %i"), stats.m_numSceneTriangles ), 0, false, Color(255,255,255), Color(0,0,0) );  y += 15;
			frame->AddDebugScreenText( x, y, String::Printf( TXT("Scene chunks: %i"), stats.m_numSceneChunks ), 0, false, Color(255,255,255), Color(0,0,0) );  y += 15;
			frame->AddDebugScreenText( x, y, String::Printf( TXT("Scene vertices: %i"), stats.m_numSceneVerts ), 0, false, Color(255,255,255), Color(0,0,0) );  y += 15;			
			y += 20;

			frame->AddDebugScreenText( x, y, String::Printf( TXT("Particle emitters: %i"), stats.m_numParticleEmitters ), 0, false, Color(255,255,255), Color(0,0,0) );  y += 15;
			frame->AddDebugScreenText( x, y, String::Printf( TXT("Particle count: %i"), stats.m_numParticles ), 0, false, Color(255,255,255), Color(0,0,0) );  y += 15;
			frame->AddDebugScreenText( x, y, String::Printf( TXT("Particle mesh chunks: %i"), stats.m_numParticleMeshChunks ), 0, false, Color(255,255,255), Color(0,0,0) );  y += 15;
			frame->AddDebugScreenText( x, y, String::Printf( TXT("Particle mesh triangles: %i"), stats.m_numParticleMeshTriangles ), 0, false, Color(255,255,255), Color(0,0,0) );  y += 15;
			//dex--

			x += 170;
		}
	
		if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexStats ) )
		{
			Uint32 y = frame->GetFrameOverlayInfo().m_height - 130;

			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Apex cloths: %i updated / %i rendered"), stats.m_numApexClothsUpdated, stats.m_numApexClothsRendered ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Apex destruction: %i updated / %i rendered"), stats.m_numApexDestructiblesUpdated, stats.m_numApexDestructiblesRendered ); y += 15;
			y += 20;

			// Render resources
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Apex resources rendered: %d"), stats.m_numApexResourcesRendered ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Apex buffers updated (non fast path): %d VB / %d IB / %d BB"), stats.m_numApexVBUpdated, stats.m_numApexIBUpdated, stats.m_numApexBBUpdated ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Apex VB semantics updated: %d"), stats.m_numApexVBSemanticsUpdated ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Apex VB fast path update: %d"), stats.m_numApexVBUpdatedFastPath ); y += 15;
		}
	}


	// Draw wind direction if we have wind
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_Wind ) )
	{
		if ( frame->GetFrameInfo().m_renderingMode != RM_HitProxies )
		{
			Vector windDir = m_previewWorld->GetWindAtPoint( frame->GetFrameInfo().m_camera.GetPosition() ).Normalized3();

			// The indicator should cover approximately 20% of each viewport dimension. We use Min to keep it uniform, and avoid clipping with a very tall or wide viewport.
			Uint32 pxSize = Min( view->GetWidth(), view->GetHeight() ) * 0.2f;

			// Position it in the top-right corner of the viewport.
			Uint32 pxX = view->GetWidth() - pxSize / 2;
			Uint32 pxY = view->GetHeight() - pxSize / 2;

			Float x = (Float)pxX / (Float)view->GetWidth() * 2.0f - 1.0f;
			Float y = (Float)pxY / (Float)view->GetHeight() * 2.0f - 1.0f;

			Vector center_c( x, y, 0.5f, 1.0f );
			Vector edge_c( 1.0f, y, 0.5f, 1.0f );
			Vector center_w = frame->GetFrameInfo().m_camera.GetScreenToWorld().TransformVectorWithW( center_c );
			Vector edge_w = frame->GetFrameInfo().m_camera.GetScreenToWorld().TransformVectorWithW( edge_c );
			center_w /= center_w.W;
			edge_w /= edge_w.W;

			Float scale = edge_w.DistanceTo( center_w ) * 0.5f;


			// Draw sphere around the wind indicator. Helps to give a little more context.
			frame->AddDebugSphere( center_w, scale, Matrix::IDENTITY, Color( 255, 255, 0, 64 ), true );
			// Draw axis, so we can see what direction is what.
			frame->AddDebugAxis( center_w, Matrix::IDENTITY, scale );

			// And draw an arrow in the direction of the wind.
			frame->AddDebug3DArrow( center_w, windDir, scale * m_windIndicatorScale, 0.05f * scale, 0.1f * scale, 0.25f * scale, Color::WHITE, Color::MAGENTA );

			// Finally... a label to pull it all together...
			frame->AddDebugScreenFormatedText( pxX - pxSize / 2, 10, Color::WHITE, TXT("Wind Direction:") );
		}
	}


    // Generate base fragments
	CEdRenderingPanel::OnViewportGenerateFragments( view, frame );
}

void CEdPreviewPanel::OnViewportTick( IViewport* view, Float timeDelta )
{
	// Tick world
	CWorldTickInfo info( m_previewWorld, timeDelta );
	info.m_updatePhysics = true;
	CRenderFrameInfo frameInfo( view );

	m_previewWorld->Tick( info, &frameInfo );

	// Tick panel
	CEdRenderingPanel::OnViewportTick( view, timeDelta );
}

void CEdPreviewPanel::OnChoiceChanged( wxCommandEvent &event )
{	
	ChangeEnvironment( event.GetSelection() );
}

void CEdPreviewPanel::ChangeEnvironment( Int32 newSelection )
{
	CEnvironmentManager *envManager = m_previewWorld->GetEnvironmentManager();
	if( envManager )
	{
		ASSERT( newSelection-1 < m_envDefs.SizeInt() );
		
		CAreaEnvironmentParams aep = m_envDefs[ newSelection ]->GetAreaEnvironmentParams();
		if( aep.m_finalColorBalance.m_activatedBalanceMap )
		{
			if( aep.m_finalColorBalance.m_balanceMap0 != NULL ) aep.m_finalColorBalance.m_balanceMap0.Get();
			if( aep.m_finalColorBalance.m_balanceMap1 != NULL ) aep.m_finalColorBalance.m_balanceMap1.Get();
		}

		SWorldEnvironmentParameters wep = m_previewWorld->GetEnvironmentParameters();
		wep.m_environmentDefinition = m_envDefs[ newSelection ];
		wep.m_environmentDefinition->SetAreaEnvironmentParams( aep );
		m_previewWorld->SetEnvironmentParameters( wep );
	}
}

void CEdPreviewPanel::SaveSession( CConfigurationManager &config )
{
	if( m_envChoice )
	{
		Int32 sel = m_envChoice->GetSelection();

		if( sel > -1 && sel - 1 < m_envDefs.SizeInt() && ! m_envDefs.Empty() ) 
		{		
			config.Write( TXT("/Frames/PreviewPanel/EnvironmentUsed"), m_envDefs[ sel ]->GetFriendlyName().StringAfter(WORLDENV_DEFINITIONS_ROOT).AsChar() );
		}
	}

	config.Write( TXT("/Frames/PreviewPanel/LightPosition"), GetLightPosition() );
}

void CEdPreviewPanel::RestoreSession( CConfigurationManager &config )
{
	String envNameSaved = config.Read( TXT("/Frames/PreviewPanel/EnvironmentUsed"), TXT("none") );

	Bool envFound = false;

	if( m_envChoice )
	{
		for( Uint32 i=0; i<m_envDefs.Size(); ++i )
		{
			if( Red::System::StringCompare( m_envDefs[i]->GetFriendlyName().StringAfter(WORLDENV_DEFINITIONS_ROOT).AsChar(), envNameSaved.AsChar() ) == 0 )
			{
				m_envChoice->SetSelection( i );
				ChangeEnvironment( i );
				envFound = true;
				break;
			}
		}	

		if( !envFound )
		{
			m_envChoice->SetSelection( 0 );
			ChangeEnvironment( 0 );
		}
	}
	
	const Int32 lightPosition = config.Read( TXT("/Frames/PreviewPanel/LightPosition"), 0 );
	SetLightPosition( lightPosition );
}

void CEdPreviewPanel::OnFovChanged( wxCommandEvent& event )
{
	SetCameraFov( (Float)event.GetInt() );
}

void CEdPreviewPanel::SetEnableFOVControl( Bool value )
{
	m_fovSpin->Enable( value );
}

void CEdPreviewPanel::ShowZoomExtents( const Box& b )
{
	this->SetCameraPosition( b.CalcCenter() + Vector( 1.f, 1.f, 1.f ) * b.CalcSize().Mag3() );
	this->SetCameraRotation( EulerAngles( 0.f, -45.f, 135.f ) );
}
