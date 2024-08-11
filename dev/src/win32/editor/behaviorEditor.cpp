/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "../../common/engine/appearanceComponent.h"
#include "../../common/engine/behaviorGraphContainerNode.h"
#include "../../common/engine/behaviorGraphEngineValueNode.h"
#include "../../common/engine/behaviorGraphStateMachine.h"
#include "../../common/engine/behaviorGraph.h"
#include "../../common/engine/behaviorGraph.inl"
#include "../../common/engine/behaviorGraphInstance.h"
#include "../../common/engine/behaviorGraphInstance.inl"
#include "../../common/engine/mesh.h"

#include "../../common/game/actor.h"
#include "../../common/game/itemIterator.h"

#include "../../common/core/depot.h"

#include "animBrowser.h"
#include "assetBrowser.h"
#include "behaviorAnimationBrowser.h"
#include "behaviorControlPanel.h"
#include "behaviorDebugger.h"
#include "behaviorEditor.h"
#include "behaviorGraphAnalyser.h"
#include "behaviorGraphEditor.h"
#include "behaviorGraphPoseTracer.h"
#include "behaviorNodeSearcher.h"
#include "behaviorPreviewPanel.h"
#include "behaviorProperties.h"
#include "behaviorRagdollPanel.h"
#include "behaviorSlotTester.h"
#include "behaviorVariableEditor.h"
#include "behaviorEditorGhostsPanel.h"
#include "behaviorEditorAnimationUsagePanel.h"
#include "behaviorAnimationLister.h"
#include "callbackData.h"
#include "editorExternalResources.h"
#include "lazyWin32feedback.h"
#include "shortcutsEditor.h"
#include "tagMiniEditor.h"
#include "popupNotification.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/mimicComponent.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/staticMeshComponent.h"
#include "../../common/engine/meshTypeComponent.h"
#include "../../common/engine/worldTick.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

CGatheredResource resFloor( TXT("engine\\meshes\\editor\\plane.w2mesh"), 0 );

enum
{
	ID_BEH_MENU_RESET = wxID_HIGHEST,
	ID_BEH_MENU_PLAY,
	ID_BEH_MENU_PLAY_ONE,
	ID_BEH_MENU_CAMERA_EYE,
	ID_BEH_MENU_CAMERA_FOLLOW,
	ID_BEH_MENU_MOTION_EX,
	ID_BEH_MENU_SHOW_SKELETON,
	ID_BEH_MENU_SHOW_NAMES,
	ID_BEH_MENU_SHOW_ACTIVATIONS,
	ID_BEH_MENU_PLAY_ONE_BACK,

	ID_BEH_EDITOR_TOOL_FIRST,
	ID_BEH_EDITOR_TOOL_LAST = ID_BEH_EDITOR_TOOL_FIRST + 100,
};

wxIMPLEMENT_CLASS( CEdBehaviorEditor, wxSmartLayoutPanel );

BEGIN_EVENT_TABLE( CEdBehaviorEditor, wxSmartLayoutPanel )
	EVT_MENU( XRCID( "behaviorSave" ), CEdBehaviorEditor::OnSave )
	EVT_MENU( XRCID( "editUndo" ), CEdBehaviorEditor::OnEditUndo )
	EVT_MENU( XRCID( "editRedo" ), CEdBehaviorEditor::OnEditRedo )
	EVT_MENU( XRCID( "editCut" ), CEdBehaviorEditor::OnEditCut )
	EVT_MENU( XRCID( "editCopy" ), CEdBehaviorEditor::OnEditCopy )
	EVT_MENU( XRCID( "editPaste" ), CEdBehaviorEditor::OnEditPaste )
	EVT_MENU( XRCID( "editDelete" ), CEdBehaviorEditor::OnEditDelete )
	EVT_MENU( XRCID( "viewActivation" ), CEdBehaviorEditor::OnViewDebugActivations )
	EVT_MENU( XRCID( "viewConditions" ), CEdBehaviorEditor::OnViewConditions )
	EVT_MENU( XRCID( "viewConditionTests" ), CEdBehaviorEditor::OnViewConditionTests )
	EVT_MENU( XRCID( "SavePers" ), CEdBehaviorEditor::OnSavePerspectives )
	EVT_MENU( XRCID( "SavePersAs" ), CEdBehaviorEditor::OnSaveAsPerspectives )
	EVT_MENU( XRCID( "LoadPers" ), CEdBehaviorEditor::OnLoadPerspectives )
	EVT_MENU( XRCID( "ReloadPers" ), CEdBehaviorEditor::OnReloadPerspectives )
	EVT_MENU( XRCID( "ResetPers" ), CEdBehaviorEditor::OnResetPerspectives )
	EVT_MENU( XRCID( "toolsRemoveUnusedVariablesAndEvents" ), CEdBehaviorEditor::OnRemoveUnusedVariablesAndEvents )
	EVT_MENU( ID_BEH_MENU_RESET, CEdBehaviorEditor::OnMenuReset )
	EVT_MENU( ID_BEH_MENU_PLAY, CEdBehaviorEditor::OnMenuPlay )
	EVT_MENU( ID_BEH_MENU_PLAY_ONE, CEdBehaviorEditor::OnMenuPlayOne )
	EVT_MENU( ID_BEH_MENU_PLAY_ONE_BACK, CEdBehaviorEditor::OnMenuPlayOneBack )
	EVT_MENU( ID_BEH_MENU_CAMERA_EYE, CEdBehaviorEditor::OnMenuCameraEye )
	EVT_MENU( ID_BEH_MENU_CAMERA_FOLLOW, CEdBehaviorEditor::OnMenuCameraFollow )
	EVT_MENU( ID_BEH_MENU_MOTION_EX, CEdBehaviorEditor::OnMenuMotionEx )
	EVT_MENU( ID_BEH_MENU_SHOW_SKELETON, CEdBehaviorEditor::OnMenuShowSkeleton )
	EVT_MENU( ID_BEH_MENU_SHOW_NAMES, CEdBehaviorEditor::OnMenuShowNames )
	EVT_MENU( ID_BEH_MENU_SHOW_ACTIVATIONS, CEdBehaviorEditor::OnMenuShowActivations )
	EVT_MENU_RANGE( ID_BEH_EDITOR_TOOL_FIRST, ID_BEH_EDITOR_TOOL_LAST, CEdBehaviorEditor::OnMenuTool ) 
	EVT_CLOSE( CEdBehaviorEditor::OnClose )
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////
extern CEdFrame* wxTheFrame;

void StartBehaviorGraphDebug( CEntity* entity )
{
	CEdBehaviorDebuggerInstanceSelectionDlg dlg( NULL, entity );
	if ( dlg.DoModal() == wxID_OK )
	{
		CBehaviorGraphInstance* instance = dlg.GetSelectedInstance();

		if( instance )
		{
			CEdBehaviorEditor* behEditor = new CEdBehaviorEditor( wxTheFrame, instance );
			behEditor->Center();
			behEditor->Show();
			behEditor->SetFocus();
		}
	}
}

void StartBehaviorGraphDebug( CEntity* entity, const CName& instanceName )
{
	CBehaviorGraphInstance* instance = NULL;

	if ( entity )
	{
		CAnimatedComponent* root = entity->GetRootAnimatedComponent();
		if ( root )
		{
			CBehaviorGraphStack* stack = root->GetBehaviorStack();
			if ( stack )
			{
				instance = stack->GetBehaviorGraphInstance( instanceName );
			}
		}
	}

	if( instance )
	{
		CEdBehaviorEditor* behEditor = new CEdBehaviorEditor( wxTheFrame, instance );
		behEditor->Center();
		behEditor->Show();
		behEditor->SetFocus();
	}
}

//////////////////////////////////////////////////////////////////////////

Bool CEdBehaviorEditor::HasPanel( const wxString& name )
{
	const wxAuiPaneInfo& paneInfo = m_mgr.GetPane( name );
	return paneInfo.IsOk();
}

wxAuiNotebook* CEdBehaviorEditor::CreateNotebook()
{
	wxSize clientSize = GetClientSize();
	wxAuiNotebook* n = new wxAuiNotebook( this, wxID_ANY, wxPoint( clientSize.x,clientSize.y), wxSize( 300, 600 ) );
	n->SetWindowStyle( wxAUI_NB_TOP | wxAUI_NB_TAB_SPLIT | wxAUI_NB_TAB_MOVE | wxAUI_NB_SCROLL_BUTTONS );

	wxAuiPaneInfo info;
	info.CloseButton( false ).Right().RightDockable( true ).MinSize(100,300).BestSize( 300, 600 ).Name( wxT("Notebook") );

	m_mgr.AddPane( n, info );

	m_mgr.Update();

	return n;
}

void CEdBehaviorEditor::CreateNotebookPanel( CEdBehaviorEditorPanel* panel, wxAuiNotebook* n )
{
	n->AddPage( panel->GetPanelWindow(), panel->GetPanelCaption(), true );

	m_activePanels.PushBack( panel );
}

void CEdBehaviorEditor::CreatePanel( CEdBehaviorEditorPanel* panel )
{
	wxAuiPaneInfo info = panel->GetPaneInfo();

	m_mgr.AddPane( panel->GetPanelWindow(), info );

	m_mgr.Update();

	m_activePanels.PushBack( panel );
}

void CEdBehaviorEditor::CreateToolPanel( CEdBehaviorEditorPanel* panel )
{
	m_toolPanels.PushBack( panel );

	AddToolToMenuBar( panel, m_toolPanels.Size() - 1 );
	
	wxAuiPaneInfo info = panel->GetPaneInfo();
	info.Show( false );

	m_mgr.AddPane( panel->GetPanelWindow(), info );

	m_mgr.Update();
}

void CEdBehaviorEditor::ShowTool( const wxString& panelName, Bool canBeClose )
{
	wxAuiPaneInfo& paneInfo = m_mgr.GetPane( panelName );
	paneInfo.CloseButton( canBeClose );
	paneInfo.Show( true );

	CEdBehaviorEditorPanel* tool = GetToolPanel( panelName );
	m_activePanels.PushBackUnique( tool );

	tool->OnReset();

	m_mgr.Update();
}

void CEdBehaviorEditor::HideTool( const wxString& panelName )
{
	wxAuiPaneInfo& paneInfo = m_mgr.GetPane( panelName );
	paneInfo.Show( false );

	CEdBehaviorEditorPanel* tool = GetToolPanel( panelName );
	Bool ret = m_activePanels.Remove( tool );
	ASSERT( ret );

	tool->OnPanelClose();

	m_mgr.Update();
}

void CEdBehaviorEditor::AddToolToMenuBar( CEdBehaviorEditorPanel* panel, Uint32 id )
{
	wxMenuBar* menuBar = GetMenuBar();
	wxMenu* menu = menuBar->GetMenu( 2 );

	Uint32 toolNum = ID_BEH_EDITOR_TOOL_FIRST + id;

	menu->Append( toolNum, panel->GetPanelCaption(), panel->GetInfo() );
}

void CEdBehaviorEditor::AddSeparatorToMenuBar()
{
	wxMenuBar* menuBar = GetMenuBar();
	wxMenu* menu = menuBar->GetMenu( 2 );
	menu->AppendSeparator();
}

void CEdBehaviorEditor::OnMenuTool( wxCommandEvent& event )
{
	Uint32 toolNum = event.GetId() - ID_BEH_EDITOR_TOOL_FIRST;

	ASSERT( toolNum < m_toolPanels.Size() );

	if( toolNum < m_toolPanels.Size() )
	{
		CEdBehaviorEditorPanel* tool = m_toolPanels[ toolNum ];
		ShowTool( tool->GetPanelName() );
	}
}

CEdBehaviorEditorPanel*	CEdBehaviorEditor::GetToolPanel( const wxString& panelName ) const
{
	for ( Uint32 i=0; i<m_toolPanels.Size(); ++i )
	{
		CEdBehaviorEditorPanel* tool = m_toolPanels[i];
		if ( tool->GetPanelName() == panelName )
		{
			return tool;
		}
	}

	ASSERT( 0 );
	return NULL;
}

Bool CEdBehaviorEditor::IsToolActive( const CEdBehaviorEditorPanel* panel )
{
	for ( Uint32 i=0; i<m_toolPanels.Size(); ++i )
	{
		CEdBehaviorEditorPanel* tool = m_toolPanels[i];
		if ( tool == panel )
		{
			wxAuiPaneInfo& paneInfo = m_mgr.GetPane( tool->GetPanelName() );

			if ( paneInfo.IsOk() && paneInfo.IsShown() )
			{
				return true;
			}
		}
	}

	return false;
}

void CEdBehaviorEditor::CheckPanelsActivation()
{
	for ( Uint32 i=0; i<m_toolPanels.Size(); ++i )
	{
		CEdBehaviorEditorPanel* tool = m_toolPanels[i];

		wxAuiPaneInfo& paneInfo = m_mgr.GetPane( tool->GetPanelName() );

		if ( paneInfo.IsOk() && paneInfo.IsShown() )
		{
			m_activePanels.PushBackUnique( tool );
		}
		else
		{
			m_activePanels.Remove( tool );
		}
	}
}

void CEdBehaviorEditor::ShowDefaultNode()
{
	CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();
	const CBehaviorGraphStateMachineNode* ma = m_behaviorGraph->GetDefaultStateMachine();
	if ( ma && instance )
	{
		CBehaviorGraphNode* node = ma->GetCurrentState( *instance );
		if ( node )
		{
			if ( !node->IsActive( *instance ) )
			{

				return;
			}

			FocusOnBehaviorNode( node );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void CEdBehaviorEditor::Tick( Float timeDelta )
{
	if ( m_world && !m_debugMode )
	{
		CWorldTickInfo info( m_world, timeDelta );
		info.m_updatePhysics = true;

		m_world->Tick( info );
	}

	if ( m_autoTrackingEnabled )
	{
		ShowDefaultNode();
	}

	CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();
	if ( instance  && m_activationAlphaEnabled )
	{
		instance->ProcessActivationAlphas();
		GetGraphEditor()->GetPanelWindow()->Refresh();
	}

	OnTick( timeDelta );
}

void CEdBehaviorEditor::CustomTick( Float timeDelta ) 
{
	OnCustomTick( timeDelta);
}

void CEdBehaviorEditor::LoadEntity( const String& fileName, const String& component, const String& defaultComponent )
{
	ASSERT( m_world );

	// Destroy previous entity
	UnloadEntity();

	m_entityFileName = String::EMPTY;

	CEntityTemplate *entTemplate = LoadResource< CEntityTemplate>( fileName );
	if ( !entTemplate )
	{
		WARN_EDITOR( TXT("Failed to load %s entity"), fileName.AsChar() );
		return;
	}

	EntitySpawnInfo einfo;
	einfo.m_template = entTemplate;
	einfo.m_name = String::Printf( TXT("BehaviorEditorEntity_%s"), fileName.AsChar() );
	einfo.m_previewOnly = true;

	CEntity* ent = m_world->GetDynamicLayer()->CreateEntitySync( einfo );
	m_entity = ent;

	CEntity* tempEnt = m_entity.Get();

	if ( m_entity.Get() )
	{
		m_entityFileName = fileName;

		CActor* actor = Cast< CActor >( m_entity.Get() ); 
		if( actor != NULL )
		{
			if ( actor->GetInventoryComponent() )
			{
				actor->InitInventory();
				actor->GetInventoryComponent()->SpawnMountedItems();

				CTimeCounter timer;
				while ( timer.GetTimePeriod() < 10.f && SItemEntityManager::GetInstance().IsDoingSomething() )
				{
					SItemEntityManager::GetInstance().OnTick( 0.001f );
				}

				//if ( actor->GetMimicComponent() )
				//{
				//	m_entityAnimComponents.PushBackUnique( actor->GetMimicComponent() );
				//}
			}
		}

		CollectEntityComponentsWithItems( m_entity.Get(), m_entityAnimComponents );

		m_animatedComponent = NULL;

		if ( m_entityAnimComponents.Size() == 0 )
		{
			String text = String::Printf( TXT("No animated componets in entity '%s'!"), fileName.AsChar() );
			wxMessageBox(  text.AsChar(), wxT("Error"), wxOK|wxCENTRE, this );
			WARN_EDITOR( text.AsChar() );
			
			// Tick world, so entity becomes visible
			PlayOneFrame();

			return;
		}
		else if ( m_entityAnimComponents.Size() == 1 )
		{
			m_animatedComponent = m_entityAnimComponents[0];
		}
		else
		{
			// Create anim comp name list
			TDynArray< String > animCopmsName;

			String defaultCompToSelect = m_entityAnimComponents.Size() > 0 ? m_entityAnimComponents[0]->GetName() : String::EMPTY;

			// Fill list
			for ( Uint32 i=0; i<m_entityAnimComponents.Size(); i++ )
			{
				animCopmsName.PushBack( m_entityAnimComponents[i]->GetName() );

				if ( m_entityAnimComponents[i]->GetName() == component )
				{
					m_animatedComponent =  m_entityAnimComponents[i];
					break;
				}

				if ( m_entityAnimComponents[i]->GetName() == defaultComponent )
				{
					defaultCompToSelect = defaultComponent;
				}
			}

			if ( !m_animatedComponent.Get() )
			{
				const String selectedComponent = InputComboBox( this, TXT("Select component"), TXT("Entity has got more then one animated component. Choose one."), defaultCompToSelect, animCopmsName);

				// Find select component
				for ( Uint32 i=0; i<animCopmsName.Size(); i++ )
				{
					if ( animCopmsName[i] == selectedComponent )
					{
						m_animatedComponent = m_entityAnimComponents[i];
						break;
					}
				}
			}
		}

		// Tick world to make the newly loaded entity visible
		{
			ASSERT( m_world );
			CWorldTickInfo info( m_world, 0.1f );
			info.m_updatePhysics = true;
			m_world->Tick( info );
		}

		ASSERT( m_animatedComponent.Get() );

		// Convert moving agent component to animated component
		for ( Uint32 i=0; i<m_entityAnimComponents.Size(); i++ )
		{
			m_entityAnimComponents[i]->SetACMotionEnabled( false );
			m_entityAnimComponents[i]->SetACMotionEnabled( true );
		}

		CAnimatedComponent* ac = m_animatedComponent.Get();
		if ( ac->IsA< CMimicComponent >() )
		{
			CMimicComponent* head = Cast< CMimicComponent >( ac );
			head->MimicHighOn();

			if ( CAnimatedComponent* root = ent->GetRootAnimatedComponent() )
			{
				if ( root->GetBehaviorStack() )
				{
					root->GetBehaviorStack()->Deactivate();
				}
			}
		}

		CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( ac );
		if ( !mac )
		{
			mac = Cast< CMovingAgentComponent >( ent->GetRootAnimatedComponent() );
		}
		if ( mac )
		{
			mac->ForceEntityRepresentation( true );
		}

		// Remove select animated component from component list
		m_entityAnimComponents.Remove( ac );

		m_entityHistory.Remove( fileName );

		if( m_entityHistory.Size() > 9 )
			m_entityHistory.Resize( 9 );

		// Expected max size = 6
		m_entityHistory.PushBack( fileName );

		// Analyze entity
		AnalyzeEntity();

		// Set extracted flags
		SetExtractedTrajectory( m_extractTrajectory );
		SetExtractedMotion( m_extractMotion );

		OnLoadEntity();
	}
	else
	{
		WARN_EDITOR( TXT("Failed to load %s entity"), fileName.AsChar() );
	}

	RecreateBehaviorGraphInstance();
}
void CEdBehaviorEditor::UnloadEntity()
{
	OnUnloadEntity();

	if ( m_entity.Get() && !m_debugMode )
	{
		m_entityAnimComponents.Clear();

		ASSERT( m_world );
		m_entity.Get()->Destroy();
	}

	m_animatedComponent = NULL;
	m_entity = NULL;
	m_originalBehaviorGraphInstance = CName::NONE;
}

void CEdBehaviorEditor::AnalyzeEntity()
{
	CAnimatedComponent* ac = GetAnimatedComponent();
	
	if ( !ac->GetBehaviorStack() )
	{
		ASSERT( ac->GetBehaviorStack() );
		return;
	}

	// Appearance
	if ( !ac->IsIncludedInAllAppearances() && !ac->IsUsedInAppearance() )
	{
		// Active appearance with this animated component
		ActiveAnimComponentInApp();
	}

	if ( ac->GetBehaviorStack()->HasPoseConstraints() )
	{
		ac->GetBehaviorStack()->SetPoseConstraintsEnable( false );
		// for editor we don't want any constraint graph to dangle around
		// remove everything. this way we will create graph instance as only instance (we may recreate it though without any issue).
		ac->GetBehaviorStack()->ClearAllStack();
	}

	// Instance stack
	TDynArray< CBehaviorGraphInstance* > instances;
	ac->GetBehaviorStack()->GetBehaviorInstancesBasedOn( m_originalBehaviorGraph, instances );

	if ( instances.Size() > 0 )
	{
		// Deactivate and unbind instance
		CBehaviorGraphInstance* oldIsta = instances[0];
		oldIsta->Deactivate();
		oldIsta->Unbind();

		// Remember original instance name
		m_originalBehaviorGraphInstance = oldIsta->GetInstanceName();

		// Replace this instance
		ac->GetBehaviorStack()->RecreateBehaviorInstance( m_behaviorGraph, oldIsta, CNAME( BehaviorEditorGraph ) );
	}
	else
	{
		// Add and activate new instance on the top
		Bool ret = ac->GetBehaviorStack()->ActivateBehaviorInstance( m_behaviorGraph, CNAME( BehaviorEditorGraph ) );
		ASSERT( ret );
	}
}

void CEdBehaviorEditor::ActiveAnimComponentInApp()
{
	CEntity *entity = m_entity.Get();
	CAnimatedComponent *animatedComponent = m_animatedComponent.Get();
	ASSERT( entity );
	ASSERT( animatedComponent );

	if ( entity && animatedComponent )
	{
		if ( animatedComponent->IsIncludedInAllAppearances() || animatedComponent->IsUsedInAppearance() )
		{
			// Ok, this animated component will be updating
			return;
		}

		// Get template
		CEntityTemplate* templ = entity->GetEntityTemplate();
		ASSERT( templ );

		//TODO
		//if ( templ )
		//{
		//	// Get all appearances
		//	TDynArray< CEntityAppearance* > apps;
		//	templ->GetAllAppearances( apps );

		//	TDynArray< CName > validApps;

		//	for ( Uint32 i=0; i<apps.Size(); i++ )
		//	{
		//		// Get all parts
		//		const CEntityAppearance* appearance = apps[i];

		//		const TDynArray< CName >& partsName = appearance->GetBodyParts();

		//		for ( Uint32 j=0; j<partsName.Size(); ++j )
		//		{
		//			// Get all states
		//			const CEntityBodyPart* bodyPart = templ->GetBodyPart( partsName[j], true );
		//			const TDynArray< CEntityBodyPartState >& states = bodyPart->GetStates();

		//			for ( Uint32 k=0; k<states.Size(); ++k )
		//			{
		//				const CEntityBodyPartState& bodyState = states[k];

		//				if ( bodyState.UseComponent( animatedComponent) )
		//				{
		//					// Appearance
		//					//m_entity->ApplyAppearance( *appearance );

		//					CName componentName = animatedComponent->GetName();
		//					
		//					CAppearanceComponent* apperanceComponent = CAppearanceComponent::GetAppearanceComponent( entity );

		//					if ( apperanceComponent )
		//					{
		//						// Body part
		//						apperanceComponent->SetBodyPartState( bodyPart->GetName(), bodyState.GetName() );
		//					}

		//					// Have to recreate component now
		//					m_world->DelayedActions();

		//					CAnimatedComponent* ac = entity->FindComponent< CAnimatedComponent >( componentName );
		//					//ASSERT( ac && ac->IsUsableInAppearance() && ac->GetEnabledInAppearance() );

		//					m_animatedComponent = ac;

		//					return;
		//				}
		//			}
		//		}
		//	}
		//}
	}
}

void CEdBehaviorEditor::CreateFloor()
{
	ASSERT( !m_floor );
	
	m_floor = m_world->GetDynamicLayer()->CreateEntitySync( EntitySpawnInfo() );

	CMeshComponent* component = Cast< CMeshComponent >( m_floor->CreateComponent( ClassID< CMeshComponent >(), SComponentSpawnInfo() ) );
	CMesh* mesh = resFloor.LoadAndGet< CMesh >();
	component->SetResource( mesh );

	OnToggleFloor();
}

void CEdBehaviorEditor::DestroyFloor()
{
	if ( m_floor )
	{
		m_floor->Destroy();
	}
}

//////////////////////////////////////////////////////////////////////////

void CEdBehaviorEditor::OnLoadEntity()
{
	for ( Uint32 i=0; i<m_activePanels.Size(); ++i )
	{
		CEdBehaviorEditorPanel* panel = m_activePanels[i];
		panel->OnLoadEntity();
	}
}

void CEdBehaviorEditor::OnUnloadEntity()
{
	for ( Uint32 i=0; i<m_activePanels.Size(); ++i )
	{
		CEdBehaviorEditorPanel* panel = m_activePanels[i];
		panel->OnUnloadEntity();
	}
}

void CEdBehaviorEditor::OnPreInstanceReload()
{
	for ( Uint32 i=0; i<m_activePanels.Size(); ++i )
	{
		CEdBehaviorEditorPanel* panel = m_activePanels[i];
		panel->OnPreInstanceReload();
	}
}

void CEdBehaviorEditor::OnInstanceReload()
{
	for ( Uint32 i=0; i<m_activePanels.Size(); ++i )
	{
		CEdBehaviorEditorPanel* panel = m_activePanels[i];
		panel->OnInstanceReload();
	}
}

void CEdBehaviorEditor::OnGraphModified()
{
	for ( Uint32 i=0; i<m_activePanels.Size(); ++i )
	{
		CEdBehaviorEditorPanel* panel = m_activePanels[i];
		panel->OnGraphModified();
	}
}

void CEdBehaviorEditor::OnNodesSelect( const TDynArray< CGraphBlock* >& nodes )
{
	for ( Uint32 i=0; i<m_activePanels.Size(); ++i )
	{
		CEdBehaviorEditorPanel* panel = m_activePanels[i];
		panel->OnNodesSelect( nodes );
	}
}

void CEdBehaviorEditor::OnNodesDeselect()
{
	for ( Uint32 i=0; i<m_activePanels.Size(); ++i )
	{
		CEdBehaviorEditorPanel* panel = m_activePanels[i];
		panel->OnNodesDeselect();
	}
}

void CEdBehaviorEditor::OnReset()
{
	for ( Uint32 i=0; i<m_activePanels.Size(); ++i )
	{
		CEdBehaviorEditorPanel* panel = m_activePanels[i];
		panel->OnReset();
	}
}

void CEdBehaviorEditor::OnDebug( Bool flag )
{
	for ( Uint32 i=0; i<m_activePanels.Size(); ++i )
	{
		CEdBehaviorEditorPanel* panel = m_activePanels[i];
		panel->OnDebug( flag );
	}
}

void CEdBehaviorEditor::OnTick( Float dt ) 
{
	for ( Uint32 i=0; i<m_activePanels.Size(); ++i )
	{
		CEdBehaviorEditorPanel* panel = m_activePanels[i];
		panel->OnTick( dt );
	}

	if ( const CAnimatedComponent* ac = GetAnimatedComponent() )
	{
		const Matrix& motion = ac->GetAnimationMotionDelta();

		if ( m_floor )
		{
			Float floorAngle( 0.f );

			const Float motionM = motion.GetTranslation().Mag3();
			if ( motionM > 0.001f )
			{
				const Float motionZ = motion.GetTranslation().Z;
				if ( MAbs( motionZ ) > 0.001f )
				{
					const Float floorAngleRad = MAsin_safe( motionZ / motionM );
					floorAngle = RAD2DEG( floorAngleRad );
				}
			}
			else
			{
				const SBehaviorUsedAnimations& anims = ac->GetRecentlyUsedAnims();
				const SBehaviorUsedAnimationDataSet& animSet = anims.m_anims;
				const auto& usedAnims = animSet.m_usedAnims;

				if ( usedAnims.Size() == 1 )
				{
					if ( const CSkeletalAnimationSetEntry* animEntry = usedAnims[ 0 ].m_animation )
					{
						if ( const CSkeletalAnimation* anim = animEntry->GetAnimation() )
						{
							if ( anim->HasExtractedMotion() )
							{
								Matrix motion( Matrix::IDENTITY );
								anim->GetMovementAtTime( anim->GetDuration(), motion );

								const Float motionZ = motion.GetTranslation().Z;
								if ( MAbs( motionZ ) > 0.05f )
								{
									const Float motionM = motion.GetTranslation().Mag3();
									if ( motionM > 0.f )
									{
										const Float floorAngleRad = MAsin_safe( motionZ / motionM );
										floorAngle = RAD2DEG( floorAngleRad );
									}
								}
							}
						}
					}
				}

				/*for ( Uint32 i=0; i<usedAnims.Size(); ++i )
				{
					const SBehaviorUsedAnimationData& usedAnim = usedAnims[ i ];
				}*/
			}

			EulerAngles rot( 0.f, 0.f, 0.f );
			rot.Pitch = floorAngle;
			m_floor->SetRotation( rot );
		}
	}
}

void CEdBehaviorEditor::OnCustomTick( Float dt ) 
{
	for ( Uint32 i=0; i<m_activePanels.Size(); ++i )
	{
		CEdBehaviorEditorPanel* panel = m_activePanels[i];
		panel->OnCustomTick( dt );
	}
}

Bool CEdBehaviorEditor::RequiresCustomTick() const 
{
	for ( Uint32 i=0; i<m_activePanels.Size(); ++i )
	{
		CEdBehaviorEditorPanel* panel = m_activePanels[i];
		if ( panel->RequiresCustomTick() )
		{
			return true;
		}
	}
	return false;
}

void CEdBehaviorEditor::OnDebuggerPostSamplePose( const SBehaviorGraphOutput& pose )
{
	for ( Uint32 i=0; i<m_activePanels.Size(); ++i )
	{
		CEdBehaviorEditorPanel* panel = m_activePanels[i];
		panel->OnDebuggerPostSamplePose( pose );
	}
}

void CEdBehaviorEditor::OnPrintNodes( CEdGraphEditor* graphCanvas ) 
{
	for ( Uint32 i=0; i<m_activePanels.Size(); ++i )
	{
		CEdBehaviorEditorPanel* panel = m_activePanels[i];
		panel->OnPrintNodes( graphCanvas );
	}
}

//////////////////////////////////////////////////////////////////////////

CEdBehaviorEditorProperties* CEdBehaviorEditor::GetStaticPropertyBrowser()
{
	if ( HasPanel( CEdBehaviorEditorProperties::GetPanelNameStatic() ) )
	{
		wxAuiPaneInfo& paneInfo = m_mgr.GetPane( CEdBehaviorEditorProperties::GetPanelNameStatic() );
		CEdBehaviorEditorProperties* properties = wxStaticCast( paneInfo.window, CEdBehaviorEditorProperties );
		return properties;
	}

	return NULL;
}

CEdBehaviorGraphEditor*	CEdBehaviorEditor::GetGraphEditor()
{
	if ( HasPanel( CEdBehaviorGraphEditor::GetPanelNameStatic() ) )
	{
		wxAuiPaneInfo& paneInfo = m_mgr.GetPane( CEdBehaviorGraphEditor::GetPanelNameStatic() );
		CEdBehaviorGraphEditor* graphEditor = wxStaticCast( paneInfo.window, CEdBehaviorGraphEditor );
		return graphEditor;
	}

	return NULL;
}

CEdBehaviorDebugger* CEdBehaviorEditor::GetDebugger()
{
	if ( HasPanel( CEdBehaviorGraphEditor::GetPanelNameStatic() ) )
	{
		wxAuiPaneInfo& paneInfo = m_mgr.GetPane( CEdBehaviorDebugger::GetPanelNameStatic() );
		CEdBehaviorDebugger* debugger = wxStaticCast( paneInfo.window, CEdBehaviorDebugger );
		return debugger;
	}

	return NULL;
}

CEdBehaviorGraphControlPanel* CEdBehaviorEditor::GetControlPanel()
{
	if ( HasPanel( CEdBehaviorGraphControlPanel::GetPanelNameStatic() ) )
	{
		wxAuiPaneInfo& paneInfo = m_mgr.GetPane( CEdBehaviorGraphControlPanel::GetPanelNameStatic() );
		CEdBehaviorGraphControlPanel* cp = wxStaticCast( paneInfo.window, CEdBehaviorGraphControlPanel );
		return cp;
	}

	return NULL;
}

CEdBehaviorPreviewPanel* CEdBehaviorEditor::GetPreviewPanel()
{
	if ( HasPanel( CEdBehaviorPreviewPanel::GetPanelNameStatic() ) )
	{
		wxAuiPaneInfo& paneInfo = m_mgr.GetPane( CEdBehaviorPreviewPanel::GetPanelNameStatic() );
		CEdBehaviorPreviewPanel* cp = wxStaticCast( paneInfo.window, CEdBehaviorPreviewPanel );
		return cp;
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////

void CEdBehaviorEditor::RelinkToInstance( CBehaviorGraphInstance* behaviorInstance )
{
	if( ! behaviorInstance )
	{
		return;
	}

	m_behaviorInstanceName = behaviorInstance->GetInstanceName();

	if ( !CheckAnimatedComponentForDebugging( m_animatedComponent.Get() ) )
	{
		Close();
		return;
	}

	ASSERT( m_animatedComponent.Get() );
	ASSERT( m_animatedComponent.Get()->GetEntity() );

	CBehaviorGraph* graph = behaviorInstance->GetGraph();
	ASSERT( graph );

	// Set graph resource
	m_behaviorGraph = graph;
	m_originalBehaviorGraph = graph;

	OnLoadEntity();

	// Reset all panels
	OnReset();

	ShowDefaultNode();
}

void CEdBehaviorEditor::FocusOnBehaviorNode( CBehaviorGraphNode* node )
{
	ASSERT( node );

	CBehaviorGraphContainerNode* root = NULL;

	if ( node->GetParentNode() )
	{
		root = Cast< CBehaviorGraphContainerNode >( node->GetParentNode() );
		ASSERT( root );
	}

	if ( root && GetGraphEditor()->GetRootNode() != root )
	{
		GetGraphEditor()->SetRootNode( root, false );
	}
	
	GetGraphEditor()->FocusOnBehaviorNode( node );
}

void CEdBehaviorEditor::SetEyeCamera( Bool flag )				
{ 
	m_eyeCamera = flag; 

	if ( m_eyeCamera && m_movingCamera )
	{
		m_movingCamera = false;
	}
}

void CEdBehaviorEditor::SetMovingCamera( Bool flag )			
{ 
	m_movingCamera = flag; 

	if ( m_eyeCamera && m_movingCamera )
	{
		m_eyeCamera = false;
	}
}

void CEdBehaviorEditor::EnableActivationAlpha( Bool flag )		
{ 
	m_activationAlphaEnabled = flag;

	CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();
	if ( !flag && instance )
	{
		instance->ClearActivationAlphas();
		GetGraphEditor()->GetPanelWindow()->Refresh();
	}
}

void CEdBehaviorEditor::EnableAutoTracking( Bool flag )
{
	m_autoTrackingEnabled = flag;
}

void CEdBehaviorEditor::ToggleSkeletonBones()
{ 
	m_dispGlobalSkeletonBones = !m_dispGlobalSkeletonBones;

	if ( m_debugMode )
	{
		CAnimatedComponent* ac = m_animatedComponent.Get();
		if( ac )
		{
			ac->SetDispSkeleton( ACDD_SkeletonBone, m_dispGlobalSkeletonBones );
		}
	}
}

void CEdBehaviorEditor::ToggleSkeletonBoneNames()
{ 
	m_dispGlobalSkeletonBoneNames = !m_dispGlobalSkeletonBoneNames; 

	if ( m_debugMode )
	{
		CAnimatedComponent* ac = m_animatedComponent.Get();
		if( ac )
		{
			ac->SetDispSkeleton( ACDD_SkeletonName, m_dispGlobalSkeletonBoneNames );
		}
	}
}

void CEdBehaviorEditor::ToggleSkeletonBoneAxis()
{ 
	m_dispGlobalSkeletonBoneAxis = !m_dispGlobalSkeletonBoneAxis; 

	if ( m_debugMode )
	{
		CAnimatedComponent* ac = m_animatedComponent.Get();
		if( ac )
		{
			ac->SetDispSkeleton( ACDD_SkeletonAxis, m_dispGlobalSkeletonBoneAxis );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CEdBehaviorEditor::CEdBehaviorEditor( wxWindow* parent, CBehaviorGraph* behaviorGraph )
	: wxSmartLayoutPanel( parent, TXT("BehaviorEditor2"), false )
    , m_behaviorGraph( NULL )
	, m_isPaused( true )
	, m_timeFactor( 1.f )
	, m_eyeCamera( false )
	, m_movingCamera( false )
	, m_extractTrajectory( true )
	, m_extractMotion( false )
	, m_playOneFrame( 0 )
	, m_entity( NULL )
	, m_animatedComponent( NULL )
	, m_dispGlobalSkeletonBones( false )
	, m_dispGlobalSkeletonBoneNames( false )
	, m_dispGlobalSkeletonBoneAxis( false )
	, m_floor( NULL )
	, m_debugMode( false )
	, m_debugUnsafeMode( false )
	, m_lodLevel( BL_Lod0 )
	, m_inputConnected( false )
	, m_meshLodLevel( -1 )
{
	m_behaviorInstanceName = CNAME( BehaviorEditorGraph);

	// Set title
	SetTitle( wxT( "Behavior Editor" ) );

	// Setup editor
	SetupEditor();

	// Set graph resource
	SetBehaviorGraph( behaviorGraph );

	// Load options from config
	LoadOptionsFromConfig();

	if ( !GetEntity() )
	{
		// Load default entity
		LoadEntity( DEFAULT_BEHAVIOR_ENTITY );
	}

	// Reset all panels
	OnReset();

	RestorePanels();
}

Bool CEdBehaviorEditor::CheckAnimatedComponentForDebugging( CAnimatedComponent* component ) const
{
	if ( !component )
	{
		return false;
	}

	CBehaviorGraphInstance* instance = component && component->GetBehaviorStack() ? component->GetBehaviorStack()->GetBehaviorGraphInstance( CNAME( BehaviorEditorGraph ) ) : NULL;
	if ( instance )
	{
		ERR_EDITOR( TXT("Behavior graph instance is 'BehaviorEditorGraph'") );
		return false;
	}

	return true;
}

CEdBehaviorEditor::CEdBehaviorEditor( wxWindow* parent, CBehaviorGraphInstance* behaviorInstance )
	: wxSmartLayoutPanel( parent, TXT("BehaviorEditor2"), false )
	, m_behaviorGraph( NULL )
	, m_isPaused( true )
	, m_timeFactor( 1.f )
	, m_eyeCamera( false )
	, m_movingCamera( false )
	, m_extractTrajectory( true )
	, m_extractMotion( false )
	, m_playOneFrame( 0 )
	, m_entity( NULL )
	, m_animatedComponent( NULL )
	, m_dispGlobalSkeletonBones( false )
	, m_dispGlobalSkeletonBoneNames( false )
	, m_dispGlobalSkeletonBoneAxis( false )
	, m_floor( NULL )
	, m_debugMode( true )
	, m_debugUnsafeMode( false )
	, m_lodLevel( BL_Lod0 )
	, m_inputConnected( false )
	, m_meshLodLevel( -1 )
{
	m_behaviorInstanceName = behaviorInstance->GetInstanceName();
	m_animatedComponent = behaviorInstance->GetAnimatedComponentUnsafe();

	if ( !CheckAnimatedComponentForDebugging( m_animatedComponent.Get() ) )
	{
		Close();
		return;
	}

	// Set title
	SetTitle( wxString::Format( wxT("Behavior Editor - Debugging - '%s' '%s'"), m_animatedComponent.Get()->GetEntity()->GetName().AsChar(), m_animatedComponent.Get()->GetName().AsChar() ) );

	// Setup editor
	SetupEditor();

	ASSERT( m_animatedComponent.Get() );
	ASSERT( m_animatedComponent.Get()->GetEntity() );

	m_entity = m_animatedComponent.Get()->GetEntity();

	CBehaviorGraph* graph = behaviorInstance->GetGraph();
	ASSERT( graph );

	// Set graph resource
	m_behaviorGraph = graph;
	m_originalBehaviorGraph = graph;

	// Load options from config
	LoadOptionsFromConfig();

	static Bool enableToApplyChanges = false;

	// Show debug panel
	if ( !enableToApplyChanges )
	{
		ShowTool( GetDebugger()->GetPanelName(), false );
	}

	// Reset all panels
	OnReset();

	// Set debug mode
	if ( !enableToApplyChanges )
	{
		OnDebug( true );
	}

	OnLoadEntity();

	// Show default node
	ShowDefaultNode();
}

TEdShortcutArray* CEdBehaviorEditor::GetAccelerators()
{
	if ( m_shortcuts.Empty() )
	{
		m_shortcuts.PushBack(SEdShortcut(TXT("BehaviorEditor\\Reset"),wxAcceleratorEntry(			wxACCEL_CTRL,	WXK_HOME,	ID_BEH_MENU_RESET)) );
		m_shortcuts.PushBack(SEdShortcut(TXT("BehaviorEditor\\Play"),wxAcceleratorEntry(			wxACCEL_CTRL,	WXK_UP,		ID_BEH_MENU_PLAY)) );
		m_shortcuts.PushBack(SEdShortcut(TXT("BehaviorEditor\\PlayOneFrame"),wxAcceleratorEntry(	wxACCEL_CTRL,	WXK_RIGHT,	ID_BEH_MENU_PLAY_ONE)) );
		m_shortcuts.PushBack(SEdShortcut(TXT("BehaviorEditor\\PlayOneFrameBack"),wxAcceleratorEntry(wxACCEL_CTRL,	WXK_LEFT,	ID_BEH_MENU_PLAY_ONE_BACK)) );
		m_shortcuts.PushBack(SEdShortcut(TXT("BehaviorEditor\\CameraEye"),wxAcceleratorEntry(		wxACCEL_CTRL,	'F',		ID_BEH_MENU_CAMERA_EYE)) );
		m_shortcuts.PushBack(SEdShortcut(TXT("BehaviorEditor\\CameraFollow"),wxAcceleratorEntry(	wxACCEL_CTRL,	'G',		ID_BEH_MENU_CAMERA_FOLLOW)) );
		m_shortcuts.PushBack(SEdShortcut(TXT("BehaviorEditor\\MotionEx"),wxAcceleratorEntry(		wxACCEL_CTRL,	'B',		ID_BEH_MENU_MOTION_EX)) );
		m_shortcuts.PushBack(SEdShortcut(TXT("BehaviorEditor\\ShowSkeleton"),wxAcceleratorEntry(	wxACCEL_CTRL,	'M',		ID_BEH_MENU_SHOW_SKELETON)) );
		m_shortcuts.PushBack(SEdShortcut(TXT("BehaviorEditor\\ShowNames"),wxAcceleratorEntry(		wxACCEL_CTRL,	'N',		ID_BEH_MENU_SHOW_NAMES)) );
		m_shortcuts.PushBack(SEdShortcut(TXT("BehaviorEditor\\Activations"),wxAcceleratorEntry(		wxACCEL_CTRL,	'A',		ID_BEH_MENU_SHOW_ACTIVATIONS)) );
	}

	return &m_shortcuts;
}

void CEdBehaviorEditor::SetupEditor()
{
	SetMinSize( wxSize( 800, 800 ) );

	// Set manager
	m_mgr.SetManagedWindow( this );

	wxIcon iconSmall;
	iconSmall.CopyFromBitmap( SEdResources::GetInstance().LoadBitmap( _T("IMG_TOOL") ) );
	SetIcon( iconSmall );

	// Preview panel
	CreatePanel( new CEdBehaviorPreviewPanel( this ) );

	// Create control panel
	CreatePanel( new CEdBehaviorGraphControlPanel( this ) );

	// Graph panel
	CEdBehaviorGraphEditor* graphEditor = new CEdBehaviorGraphEditor( this );
	CreatePanel( graphEditor );

	{
		// Create undo manager
		m_undoManager = new CEdUndoManager( GetOriginalFrame() );
		m_undoManager->AddToRootSet();
		m_undoManager->SetMenuItems( GetMenuBar()->FindItem( XRCID( "editUndo" ) ), GetMenuBar()->FindItem( XRCID( "editRedo" ) ) );

		graphEditor->SetUndoManager( m_undoManager );
	}

	// Create properties
	{
		PropertiesPageSettings pageSettings;
		pageSettings.m_showEntityComponents = false;
		CreatePanel( new CEdBehaviorEditorProperties( this, pageSettings, m_undoManager ) );
	}

	// Create notebook
	wxAuiNotebook* notebook = CreateNotebook();

	// Animation browser panel
	CreateNotebookPanel( new CEdBehaviorAnimationBrowserPanel( this, notebook ), notebook );

	// Variable panel
	CEdBehaviorVariableEditor* varEditor = new CEdBehaviorVariableEditor( this, notebook );
	CreateNotebookPanel( varEditor, notebook );
	varEditor->SetUndoManager( m_undoManager );

	// Add separator
	AddSeparatorToMenuBar();

	// Runtime property panel
	CreateToolPanel( new CEdBehaviorEditorRuntimeProperties( this ) );

	// Layers panel
	CreateToolPanel( new CEdBehaviorGraphLayerEditor( this ) );

	// Tree tool panel
	CreateToolPanel( new CEdBehaviorGraphNodeSearcher( this ) );

	// Instance data
	CreateToolPanel( new CEdBehaviorInstanceProfiler( this ) );

	// Slot tester
	CreateToolPanel( new CEdBehaviorGraphSlotTester( this ) );

	// Pose tracer
	CreateToolPanel( new CEdBehaviorGraphPoseTracer( this ) );

	// Pose matcher
	CreateToolPanel( new CEdBehaviorGraphPoseMatcher( this ) );

	// Pose matcher
	CreateToolPanel( new CEdBehaviorDebugger( this ) );

	// Stack panel
	//CreateToolPanel( new CEdBehaviorGraphStackPanel( this ) ); // what was the purpose of this tool? does anyone use it?

	// Look at panel
	CreateToolPanel( new CEdBehaviorGraphLookAtPanel( this ) );

	// Motion panel
	CreateToolPanel( new CEdBehaviorGraphMotionAnalyzer( this ) );

	// Profiler
	CreateToolPanel( new CEdBehaviorGraphProfiler( this ) );

	// Sim input panel
	CreateToolPanel( new CEdBehaviorSimInputTool( this ) );
	
	// Ghosts panel
	CreateToolPanel( new CEdBehaviorEditorGhostsPanel( this ) );

	// Script panel
	CreateToolPanel( new CEdBehaviorEditorScriptPanel( this ) );

	// Animation usage panel
	CreateToolPanel( new CEdBehaviorEditorAnimationUsagePanel( this ) );
	
	// Animation lister panel
	CreateToolPanel( new CEdBehaviorGraphAnimationLister( this ) );

	 m_mgr.Update();

	// Update and finalize layout
	Layout();
	Show();

	// Create floor
	CreateFloor();
}

CEdBehaviorEditor::~CEdBehaviorEditor()
{
	if ( m_debugMode && GetBehaviorGraphInstance() )
	{
		// HACK
		GetBehaviorGraphInstance()->RemoveEditorListener();
	}

	CheckPanelsActivation();

	SaveOptionsToConfig();

	{
		m_undoManager->RemoveFromRootSet();
		m_undoManager->Discard();
		m_undoManager = NULL;
	}

	// Revert editor changes. Pop special editor behavior
	CBehaviorGraphInstance* editorInstance = GetBehaviorGraphInstance();
	if ( !m_debugMode && editorInstance )
	{
		CAnimatedComponent* ac = GetAnimatedComponent();
		ASSERT( ac && ac->GetBehaviorStack() );

		Bool ret = ac->GetBehaviorStack()->DetachBehaviorInstance( editorInstance->GetInstanceName() );
		ASSERT( ret );
	}
	else if ( m_debugMode )
	{
		CAnimatedComponent* ac = m_animatedComponent.Get();

		if ( ac )
		{
			if ( m_dispGlobalSkeletonBones )
			{
				ac->SetDispSkeleton( ACDD_SkeletonBone, false );
			}
			if ( m_dispGlobalSkeletonBoneNames )
			{
				ac->SetDispSkeleton( ACDD_SkeletonName, false );
			}
			if ( m_dispGlobalSkeletonBoneAxis )
			{
				ac->SetDispSkeleton( ACDD_SkeletonAxis, false );
			}
		}
	}

	UnloadEntity();

	DestroyFloor();

	if ( !m_debugMode )
	{
		// Remove from root set
		if ( m_originalBehaviorGraph )
		{
			m_originalBehaviorGraph->RemoveFromRootSet();
			m_originalBehaviorGraph = NULL;
		}

		if ( m_behaviorGraph )
		{
			m_behaviorGraph->RemoveFromRootSet();
			m_behaviorGraph->Discard();
			m_behaviorGraph = NULL;
		}
	}

	// Uninit manager
	m_mgr.UnInit();
}

void CEdBehaviorEditor::OnClose( wxCloseEvent &event )
{
	for ( Uint32 i=0; i<m_activePanels.Size(); ++i )
	{
		CEdBehaviorEditorPanel* panel = m_activePanels[i];
		panel->OnClose();
	}

	wxSmartLayoutPanel::OnClose( event );
}

void CEdBehaviorEditor::SaveOptionsToConfig()
{
	if ( ! GetGraphEditor() )
	{
		return;
	}

	SaveLayout(TXT("/Frames/BehaviorEditor"));
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	Int32 displayConditions = (Int32)GetGraphEditor()->GetDisplayConditions();
	config.Write( TXT("/Frames/BehaviorEditor/displayConditions"), displayConditions );

	Int32 displayConditionTests = (Int32)GetGraphEditor()->GetDisplayConditionTests();
	config.Write( TXT("/Frames/BehaviorEditor/displayConditionTests"), displayConditionTests );

	if ( !m_debugMode && m_originalBehaviorGraph && m_originalBehaviorGraph->GetFile() )
	{
		String pathForPanels = String::Printf( TXT("/Frames/BehaviorEditor/%s/"), m_originalBehaviorGraph->GetFile()->GetDepotPath().AsChar() );
		for ( Uint32 i=0; i<m_activePanels.Size(); ++i )
		{
			m_activePanels[i]->SaveSession( config, pathForPanels );
		}
	}

	ISavableToConfig::SaveSession();
}

void CEdBehaviorEditor::RestorePanels()
{
	ASSERT( m_originalBehaviorGraph );
	if ( m_originalBehaviorGraph )
	{
		CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

		String pathForPanels = String::Printf( TXT("/Frames/BehaviorEditor/%s/"), m_originalBehaviorGraph->GetFile()->GetDepotPath().AsChar() );
		for ( Uint32 i=0; i<m_activePanels.Size(); ++i )
		{
			m_activePanels[i]->RestoreSession( config, pathForPanels );
		}
	}
}

void CEdBehaviorEditor::LoadOptionsFromConfig()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	// Set display conditions
	Int32 displayConditions = config.Read( TXT("/Frames/BehaviorEditor/displayConditions"), 0 );
	GetGraphEditor()->SetDisplayConditions( displayConditions > 0 );

	Int32 displayConditionTests = config.Read( TXT("/Frames/BehaviorEditor/displayConditionTests"), 0 );
	GetGraphEditor()->SetDisplayConditionTests( displayConditionTests > 0 );

	wxMenuBar* menuBar = GetMenuBar();
	ASSERT( menuBar );
	wxMenu* menuView = menuBar->GetMenu( menuBar->FindMenu( TXT( "View") ) );
	ASSERT( menuView );
	if( menuView )
	{
		wxMenuItem* displayConditionsItem = menuView->FindItem( XRCID( "viewConditions" ) );
		ASSERT( displayConditionsItem );
		if( displayConditionsItem && displayConditions > 0 )
		{
			displayConditionsItem->Check();
		}
		wxMenuItem* displayConditionTestsItem = menuView->FindItem( XRCID( "viewConditionTests" ) );
		ASSERT( displayConditionTestsItem );
		if( displayConditionTestsItem && displayConditionTests > 0 )
		{
			displayConditionTestsItem->Check();
		}
	}

    //CEdShortcutsEditor::Load(*this->GetMenuBar(), GetOriginalLabel(), wxString(), true, false );
	CEdShortcutsEditor::Load(*this, *GetAccelerators(), GetOriginalLabel(), false, true );

    // Load layout after the shortcuts (duplicate menu after the shortcuts loading)
    LoadLayout(TXT("/Frames/BehaviorEditor"));

	ASSERT( m_originalBehaviorGraph );
	if ( m_originalBehaviorGraph )
	{
		String pathForPanels = String::Printf( TXT("/Frames/BehaviorEditor/%s/"), m_originalBehaviorGraph->GetFile()->GetDepotPath().AsChar() );
		for ( Uint32 i=0; i<m_activePanels.Size(); ++i )
		{
			m_activePanels[i]->RestoreSession( config, pathForPanels );
		}
	}
	 
	ISavableToConfig::RestoreSession();
}

void CEdBehaviorEditor::SaveSession( CConfigurationManager &config )
{
	if ( !m_debugMode && m_originalBehaviorGraph != NULL && m_originalBehaviorGraph->GetFile() != NULL )
	{
		String path = m_originalBehaviorGraph->GetFile()->GetDepotPath();

		config.Write( TXT("Frames/BehaviorEditor/Entity/") + path, m_entityFileName );
			
		CAnimatedComponent *animatedComponent = m_animatedComponent.Get();
		if ( animatedComponent )
		{
			config.Write( TXT("Frames/BehaviorEditor/Component/") + path, animatedComponent->GetName() );
		}

		config.Write( TXT("Frames/BehaviorEditor/EntityHistory/") + path, String::Join( m_entityHistory, TXT(";") ) );
	}

	if ( !IsDebugMode() )
	{
		config.Write( TXT("CurrentPerspective"), m_perspective );
	}

	SavePerspective( m_mgr.SavePerspective().wc_str() );
}

void CEdBehaviorEditor::RestoreSession( CConfigurationManager &config )
{
 	String path = m_originalBehaviorGraph->GetFile()->GetDepotPath();

 	String entity = config.Read( TXT("Frames/BehaviorEditor/Entity/") + path, String::EMPTY );
	String component = config.Read( TXT("Frames/BehaviorEditor/Component/") + path, String::EMPTY );
 	String history = config.Read( TXT("Frames/BehaviorEditor/EntityHistory/") + path, String::EMPTY );

	m_entityHistory.Clear();
 	history.Slice( m_entityHistory, TXT(";") );
 	
 	if( !entity.Empty() && m_entityFileName != entity && !m_debugMode )
	{
 		LoadEntity( entity, component );
	}

	m_perspective = config.Read( TXT("CurrentPerspective"), TXT("Default") );
	
	ASSERT( !m_perspective.Empty() );
	if ( m_perspective.Empty() )
	{
		m_perspective = TXT("Default");
	}

	if ( IsDebugMode() )
	{
		m_perspective = TXT("Debug");
	}

	LoadPerspective();

	RestorePanels();
}

void CEdBehaviorEditor::EnumPerspectives( TDynArray< String >& list )
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/BehaviorEditor/Perspectives") );

	String allPers = config.Read( TXT("All"), String::EMPTY );

	allPers.Slice( list, TXT(";") );
}

void CEdBehaviorEditor::SavePerspective( const String& data )
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/BehaviorEditor/Perspectives") );

	if ( m_perspective.Empty() )
	{
		m_perspective = TXT("Default");
	}

	String allPers = config.Read( TXT("All"), String::EMPTY );
	
	TDynArray< String > persTable;
	allPers.Slice( persTable, TXT(";") );

	Bool found = false;

	for ( Uint32 i=0; i<persTable.Size(); ++i )
	{
		if ( persTable[i] == m_perspective )
		{
			found = true;
			break;
		}
	}

	if ( !found )
	{
		allPers += TXT(";") + m_perspective;
	}

	config.Write( TXT("All"), allPers );
	config.Write( m_perspective, data );
}

void CEdBehaviorEditor::LoadPerspective()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/BehaviorEditor/Perspectives") );

	String data = config.Read( m_perspective, String::EMPTY );
	if ( data.Empty() )
	{
		return;
	}

	m_mgr.LoadPerspective( data.AsChar(), true );

	CheckPanelsActivation();

	OnReset();
}

void CEdBehaviorEditor::ResetPerspective()
{
	wxString str = wxT("layout2|name=Preview;caption=Preview;state=16781308;dir=4;layer=0;row=0;pos=0;prop=163721;bestw=400;besth=400;minw=100;minh=100;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Control;caption=Control panel;state=2044;dir=4;layer=0;row=0;pos=1;prop=36279;bestw=200;besth=50;minw=200;minh=25;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Properties;caption=Static properties;state=2044;dir=4;layer=0;row=0;pos=2;prop=100000;bestw=300;besth=600;minw=100;minh=300;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Graph;caption=Graph;state=768;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=600;besth=600;minw=100;minh=100;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Notebook;caption=;state=2044;dir=2;layer=0;row=0;pos=0;prop=100000;bestw=300;besth=600;minw=100;minh=300;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=PropertiesRuntime;caption=Runtime properties;state=2099198;dir=4;layer=0;row=0;pos=3;prop=100000;bestw=300;besth=600;minw=100;minh=300");

	m_mgr.LoadPerspective( str, true );

	m_mgr.Update();
}

Bool CEdBehaviorEditor::CanBeModify() 
{
	return GetAnimatedComponent() && (!m_debugMode || m_debugUnsafeMode)? true : false; 
}

void CEdBehaviorEditor::UseBehaviorInstance( Bool flag )
{
	CAnimatedComponent* ac = GetAnimatedComponent();
	if ( !ac || !ac->GetBehaviorStack() )
	{
		return;
	}

	// Get instance
	CBehaviorGraphInstance* selectedInstance = GetBehaviorGraphInstance();
	if ( !selectedInstance )
	{
		return;
	}

	if ( !flag && selectedInstance->IsActive() )
	{
		// Deactivate deprecated instance
		selectedInstance->Deactivate();
	}
	else if ( flag )
	{
		ASSERT( !selectedInstance->IsActive() );
		ASSERT( selectedInstance->IsBinded() );

		selectedInstance->Unbind();

		CBehaviorGraphInstance* currInstance = ac->GetBehaviorStack()->RecreateBehaviorInstance( m_behaviorGraph, selectedInstance );
		ASSERT( currInstance );

		// Set open in editor flag
		if ( !m_debugMode )
		{
			currInstance->OnOpenInEditor();
		}
	}
}

void CEdBehaviorEditor::BehaviorGraphModified()
{
	RecreateBehaviorGraphInstance();

	OnGraphModified();
}

void CEdBehaviorEditor::RecreateBehaviorGraphInstance()
{
	if ( m_debugMode )
	{
		ASSERT( !m_debugMode );
		return;
	}

	OnPreInstanceReload();

	UseBehaviorInstance( false );

	if ( m_behaviorGraph )
	{
		// Reload graph
		m_behaviorGraph->Reload();

		UseBehaviorInstance( true );

		OnInstanceReload();
	}
}

void CEdBehaviorEditor::SetBehaviorGraph( CBehaviorGraph *graph )
{
	if ( graph != m_originalBehaviorGraph )
	{
		// Remove from root set
		if ( m_originalBehaviorGraph )
		{
			m_originalBehaviorGraph->RemoveFromRootSet();
			m_originalBehaviorGraph = NULL;
		}

		if ( m_behaviorGraph )
		{
			m_behaviorGraph->RemoveFromRootSet();
			m_behaviorGraph->Discard();
			m_behaviorGraph = NULL;
		}

		// Set original graph
		m_originalBehaviorGraph = graph;
	
		if ( m_originalBehaviorGraph )
		{
			// Clone graph for editor
			m_behaviorGraph = SafeCast< CBehaviorGraph >( m_originalBehaviorGraph->Clone( m_originalBehaviorGraph ) );

			// Add to root set
			m_originalBehaviorGraph->AddToRootSet();
		
			// Add to root set
			m_behaviorGraph->AddToRootSet();
		}
	}

	// Recreate behavior graph instance
	RecreateBehaviorGraphInstance();

	if( m_behaviorGraph && m_behaviorGraph->GetFile() )
	{
		SetTitle( wxString::Format( wxT("Behavior Editor - %s"), m_behaviorGraph->GetFile()->GetDepotPath().AsChar() ) );
	}
}

void CEdBehaviorEditor::SetExtractedMotion( Bool flag )
{
	m_extractMotion = flag;

	// Select animated component
	if ( m_animatedComponent.Get() )
	{
		m_animatedComponent.Get()->SetUseExtractedMotion( flag );

		// All animated components
		for ( Uint32 i=0; i<m_entityAnimComponents.Size(); i++ )
		{
			m_entityAnimComponents[i]->SetUseExtractedMotion( flag );
		}
	}
}

void CEdBehaviorEditor::SetExtractedTrajectory( Bool flag )
{
	m_extractTrajectory = flag;

	// Select animated component
	if ( m_animatedComponent.Get() )
	{
		m_animatedComponent.Get()->SetUseExtractedTrajectory( flag );

		// All animated components
		for ( Uint32 i=0; i<m_entityAnimComponents.Size(); i++ )
		{
			m_entityAnimComponents[i]->SetUseExtractedTrajectory( flag );
		}
	}
}

void CEdBehaviorEditor::OnToggleFloor()
{
	m_floor->SetHideInGame( !m_floor->IsHiddenInGame() );
}

void CEdBehaviorEditor::ToggleDebuggerUnsafeMode()
{
	m_debugUnsafeMode = true;

	if ( m_debugUnsafeMode )
	{
		CEdBehaviorDebugger* d = GetDebugger();
		CEdBehaviorGraphEditor* g = GetGraphEditor();

		for ( Uint32 i=0; i<m_activePanels.Size(); ++i )
		{
			CEdBehaviorEditorPanel* panel = m_activePanels[i];

			if ( panel != d )
			{
				panel->OnDebug( false );
			}
		}
	}
}

void CEdBehaviorEditor::OnDynTarget( Bool flag )
{
	GetPreviewPanel()->OnDynTarget( flag );
}

void CEdBehaviorEditor::OnSave( wxCommandEvent& event )
{
	if ( !CanBeModify() )
	{	
		return;
	}

	ValidateBehaviorGraph();

	// Hack save before risk reload action
	HackSaftySave();

	// Map
	TDynArray< TPair< CAnimatedComponent*, TDynArray< CBehaviorGraphInstance* > > > acMap;

	for ( ObjectIterator<CAnimatedComponent> it; it; ++it )
	{
		CAnimatedComponent* ac = (*it);

		if ( ac->IsAttached() && ac->GetBehaviorStack() )
		{
			TDynArray< CBehaviorGraphInstance* > instancesToRecreate;
	
			ac->GetBehaviorStack()->GetBehaviorInstancesBasedOn( m_originalBehaviorGraph, instancesToRecreate );

			if ( instancesToRecreate.Size() > 0 )
			{
				acMap.PushBack( TPair< CAnimatedComponent*, TDynArray< CBehaviorGraphInstance* > >( ac, instancesToRecreate ) );
			}
		}
	}

	// Unbind all instances to recreate
	for ( Uint32 i=0; i<acMap.Size(); ++i )
	{
		const TDynArray< CBehaviorGraphInstance* >& instances = acMap[i].m_second;

		for ( Uint32 j=0; j<instances.Size(); ++j )
		{
			instances[j]->Deactivate();
			instances[j]->Unbind();
		}
	}

	// Save original graph
	m_originalBehaviorGraph->SetAsCloneOf( m_behaviorGraph );
	m_originalBehaviorGraph->Reload();

	// Restore instances
	for ( Uint32 i=0; i<acMap.Size(); i++ )
	{
		CAnimatedComponent* ac = acMap[i].m_first;
		const TDynArray< CBehaviorGraphInstance* >& instances = acMap[i].m_second;
	
		for ( Uint32 j=0; j<instances.Size(); ++j )
		{
			ac->GetBehaviorStack()->RecreateBehaviorInstance( m_originalBehaviorGraph, instances[j] );
		}
	}

	// Uff... Done!
	SEdPopupNotification::GetInstance().Show( this, TXT("SAVE"), TXT("Behavior Graph") );
}

void CEdBehaviorEditor::HackSaftySave()
{
	CDiskFile* file = m_originalBehaviorGraph->GetFile();
	ASSERT( file );
	
	file->Rebind( m_behaviorGraph );
	m_behaviorGraph->Save();

	file->Rebind( m_originalBehaviorGraph );
	ASSERT( m_originalBehaviorGraph->GetFile() == file );
}

void CEdBehaviorEditor::OnGraphSelectionChanged()
{
	// Grab objects
	TDynArray< CGraphBlock* > blocks;
	GetGraphEditor()->GetSelectedBlocks( blocks );

	// Select them at properties browser
	if ( blocks.Size() )
	{
		OnNodesSelect( blocks );
	}
	else
	{
		OnNodesDeselect();
	}
}

void CEdBehaviorEditor::OnGraphStructureWillBeModified( IGraphContainer* graph )
{
	// Use with care!!!
	UseBehaviorInstance( false );
}

void CEdBehaviorEditor::OnGraphStructureModified( IGraphContainer* graph )
{
	RecreateBehaviorGraphInstance();
}

void CEdBehaviorEditor::GetSelectedNodes( TDynArray< CGraphBlock* >& nodes )
{
	TDynArray< CGraphBlock* > blocks;
	GetGraphEditor()->GetSelectedBlocks( nodes );
}

void CEdBehaviorEditor::OnEditUndo( wxCommandEvent& event )
{
	m_undoManager->Undo();
}

void CEdBehaviorEditor::OnEditRedo( wxCommandEvent& event )
{
	m_undoManager->Redo();
}

void CEdBehaviorEditor::OnEditCopy( wxCommandEvent& event )
{
	GetGraphEditor()->CopySelection();
}

void CEdBehaviorEditor::OnEditCut( wxCommandEvent& event )
{
	GetGraphEditor()->CutSelection();
}

void CEdBehaviorEditor::OnEditPaste( wxCommandEvent& event )
{
	GetGraphEditor()->PasteOnCenter();
}

void CEdBehaviorEditor::OnEditDelete( wxCommandEvent& event )
{
	GetGraphEditor()->DeleteSelection();
}

void CEdBehaviorEditor::OnMenuReset( wxCommandEvent& event )
{
	RecreateBehaviorGraphInstance();

	CEdBehaviorGraphControlPanel* cp = GetControlPanel();
	if ( cp )
	{
		cp->RefreshAllButtons();
	}
}

void CEdBehaviorEditor::OnMenuPlayOne( wxCommandEvent& event )
{
	PlayOneFrame();
}

void CEdBehaviorEditor::OnMenuPlayOneBack( wxCommandEvent& event )
{
	PlayOneFrameBack();
}

void CEdBehaviorEditor::OnMenuPlay( wxCommandEvent& event )
{
	//OnPlayPause( event );
	SetPause( !IsPaused() );

	CEdBehaviorGraphControlPanel* cp = GetControlPanel();
	if ( cp )
	{
		cp->RefreshAllButtons();
	}
}

void CEdBehaviorEditor::OnMenuCameraEye( wxCommandEvent& event )
{
	SetEyeCamera( !m_eyeCamera );

	CEdBehaviorGraphControlPanel* cp = GetControlPanel();
	if ( cp )
	{
		cp->RefreshAllButtons();
	}
}

void CEdBehaviorEditor::OnMenuCameraFollow( wxCommandEvent& event )
{
	SetMovingCamera( !m_movingCamera );

	CEdBehaviorGraphControlPanel* cp = GetControlPanel();
	if ( cp )
	{
		cp->RefreshAllButtons();
	}
}

void CEdBehaviorEditor::OnMenuMotionEx( wxCommandEvent& event )
{
	SetExtractedMotion( !m_extractMotion );

	CEdBehaviorGraphControlPanel* cp = GetControlPanel();
	if ( cp )
	{
		cp->RefreshAllButtons();
	}
}

void CEdBehaviorEditor::OnMenuShowSkeleton( wxCommandEvent& event )
{
	ToggleSkeletonBones();

	CEdBehaviorGraphControlPanel* cp = GetControlPanel();
	if ( cp )
	{
		cp->RefreshAllButtons();
	}
}

void CEdBehaviorEditor::OnMenuShowNames( wxCommandEvent& event )
{
	ToggleSkeletonBoneNames();

	CEdBehaviorGraphControlPanel* cp = GetControlPanel();
	if ( cp )
	{
		cp->RefreshAllButtons();
	}
}

void CEdBehaviorEditor::OnMenuShowActivations( wxCommandEvent& event )
{
	EnableActivationAlpha( !IsActivationAlphaEnabled() );

	CEdBehaviorGraphControlPanel* cp = GetControlPanel();
	if ( cp )
	{
		cp->RefreshAllButtons();
	}
}

void CEdBehaviorEditor::OnSavePerspectives( wxCommandEvent& event )
{
	wxString data = m_mgr.SavePerspective();
	SavePerspective( data.wc_str() );
}

void CEdBehaviorEditor::OnSaveAsPerspectives( wxCommandEvent& event )
{
	TDynArray< String > list;
	EnumPerspectives( list );

	String str = InputComboBox( this, TXT("Save as..."), TXT("Choose or write perspective name"), m_perspective, list );

	if ( !str.Empty() )
	{
		wxString data = m_mgr.SavePerspective();
		m_perspective = str;
		SavePerspective( data.wc_str() );
	}
}

void CEdBehaviorEditor::OnReloadPerspectives( wxCommandEvent& event )
{
	LoadPerspective();
}

void CEdBehaviorEditor::OnResetPerspectives( wxCommandEvent& event )
{
	ResetPerspective();
}

void CEdBehaviorEditor::OnLoadPerspectives( wxCommandEvent& event )
{
	TDynArray< String > list;
	EnumPerspectives( list );

	String str = InputComboBox( this, TXT("Load"), TXT("Choose perspective"), m_perspective, list );

	if ( !str.Empty() )
	{
		m_perspective = str;
		LoadPerspective();
	}
}

void CEdBehaviorEditor::OnViewDebugActivations( wxCommandEvent& event )
{
	GetGraphEditor()->DisplayBlockActivations( event.IsChecked() );
}

void CEdBehaviorEditor::OnViewConditions( wxCommandEvent& event )
{
	GetGraphEditor()->SetDisplayConditions( event.IsChecked() );
	GetGraphEditor()->Repaint();
}

void CEdBehaviorEditor::OnViewConditionTests( wxCommandEvent& event )
{
	GetGraphEditor()->SetDisplayConditionTests( event.IsChecked() );
	GetGraphEditor()->Repaint();
}

void CEdBehaviorEditor::OnRemoveUnusedVariablesAndEvents( wxCommandEvent& event )
{
	Int32 removedVariablesAndEventsCount = m_behaviorGraph? m_behaviorGraph->RemoveUnusedVariablesAndEvents() : 0;
	if ( removedVariablesAndEventsCount )
	{
		String text = String::Printf( TXT("Removed variables and events\n\nRemoved %i variable(s) and event(s) that nodes declared were not used.\n\nBut just to be sure, test if not too much was removed."), removedVariablesAndEventsCount );
		wxMessageBox(  text.AsChar(), wxT("Information"), wxOK|wxCENTRE, this );
		RecreateBehaviorGraphInstance();
	}
	else
	{
		String text = String::Printf( TXT("Nothing to remove\n\nAll variables and events were declared by nodes as used.") );
		wxMessageBox(  text.AsChar(), wxT("Information"), wxOK|wxCENTRE, this );
	}
}

void CEdBehaviorEditor::CheckGraphEngineValues()
{
	// Deactivate manually control for all engine value nodes
	
	if ( m_behaviorGraph )
	{
		TDynArray< CBehaviorGraphEngineValueNode* > nodesToChange;

		// Graph's engine nodes
		TDynArray< CBehaviorGraphEngineValueNode* > engineNodes;
		m_behaviorGraph->GetNodesOfClass< CBehaviorGraphEngineValueNode >( engineNodes );

		for ( Uint32 i=0; i<engineNodes.Size(); i++ )
		{
			CBehaviorGraphEngineValueNode* node = engineNodes[i];
			if ( node->IsManuallyControlled() )
			{
				nodesToChange.PushBack( node );
			}
		}

		// Ask user
		if ( !nodesToChange.Empty() && m_behaviorGraph->MarkModified() )
		{
			if ( YesNo( TXT("Some engine value nodes are under manual control.\nDo you want to deactivate maual control for this nodes?") ) )
			{
				// Change control from manual to engine
				for ( Uint32 i=0; i<nodesToChange.Size(); i++ )
				{
					nodesToChange[i]->SetManualControl( false );
				}
			}
		}
	}
}

void CEdBehaviorEditor::CheckStateMachines( IFeedbackSystem* sys )
{
	ASSERT( sys );

	if ( m_behaviorGraph )
	{
		TDynArray< CBehaviorGraphStateMachineNode* > ms;

		m_behaviorGraph->GetNodesOfClass( ms );

		const Uint32 size = ms.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			CBehaviorGraphStateMachineNode* m = ms[ i ];
			if ( m && !m->GetDefaultState() )
			{
				if ( m != m_behaviorGraph->GetDefaultStateMachine() )
				{
					sys->ShowError( TXT("Graph's default state machine '%s' doesn't have default state"), m->GetName().AsChar() );
				}
				else
				{
					sys->ShowError( TXT("State machine '%s' doesn't have default state"), m->GetName().AsChar() );
				}
			}
		}
	}
}

void CEdBehaviorEditor::EnumInputs( TDynArray< CName >& inputs )
{
	GetPreviewPanel()->EnumInputs( inputs );
}

CActor* CEdBehaviorEditor::GetActorEntity()
{
	CEntity* entity = m_entity.Get();
	if ( entity )
	{
		return Cast< CActor >( entity );
	}

	return NULL;
}

void CEdBehaviorEditor::SetMeshLodLevel( Int32 lod )
{ 
	m_meshLodLevel = lod; 

	if ( CActor* a = Cast< CActor >( m_entity.Get() ) )
	{
		for ( EntityWithItemsComponentIterator< CMeshTypeComponent > it( a ); it; ++it )
		{
			CMeshTypeComponent* c = *it;
			c->ForceLODLevel( lod );
		}
	}
}

void CEdBehaviorEditor::VaildateGraphNodes()
{
	if ( m_behaviorGraph )
	{
		TDynArray< CBehaviorGraphNode* > nodes;
		m_behaviorGraph->GetAllNodes( nodes );

		Bool validationFailed = false;
		wxString htmlPage;
		
		const Uint32 nodesSize = nodes.Size();
		for ( Uint32 i = 0; i < nodesSize; ++i ) // validate nodes
		{
			const CBehaviorGraphNode* const currNode = nodes[i];

			TDynArray< String > errorStringVector;
			if ( currNode && !currNode->ValidateInEditor( errorStringVector ) )
			{
				// report founded problems:
				htmlPage += wxT("<p> Errors found in ");
				htmlPage += currNode->GetCaption().AsChar();
				htmlPage += wxT(":</p>");

				for ( const auto& text: errorStringVector )
				{
					htmlPage += wxT("<p><i> - ");
					htmlPage += text.AsChar();
					htmlPage += wxT("</i></p>");
				}
				
				htmlPage += wxT("<br><br><a href=\"");
				htmlPage += wxString::Format( wxT("%i"), currNode->GetObjectIndex() ); // encode object id as href, cuz GetId() can be chaneged graph is being reloaded
				htmlPage += wxT("\">Show Node</a>");
				htmlPage += wxT("<hr>");
				validationFailed = true;
			}
		}

		if ( validationFailed )
		{
			// graph have some errors, so show report to user
			wxDialog* dialog = new wxDialog( this, wxID_ANY, wxT("BehaviorGraph Validation Report") );
			wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
			wxHtmlWindow* htmlWindow = new wxHtmlWindow( dialog );

			htmlWindow->SetPage( htmlPage );

			// bind event
			htmlWindow->Bind( wxEVT_COMMAND_HTML_LINK_CLICKED, &CEdBehaviorEditor::OnBehaviorGraphValidatorLinkClicked, this);

			sizer->Add( htmlWindow, 1, wxEXPAND, 0 );
			dialog->SetSizer( sizer );
			dialog->Show();	
		}
	}
}

void CEdBehaviorEditor::ValidateBehaviorGraph()
{
	CheckGraphEngineValues();

	CLazyWin32Feedback feedback;

	CheckStateMachines( &feedback );

	VaildateGraphNodes();

	feedback.ShowAll();
}

void CEdBehaviorEditor::OnBehaviorGraphValidatorLinkClicked( wxHtmlLinkEvent& event )
{
	const wxHtmlLinkInfo& linkInfo = event.GetLinkInfo();
	wxString href = linkInfo.GetHref();

	Uint32 objectId = wxAtoi( href ); // object id is encoded as href

	if ( m_behaviorGraph )
	{
		TDynArray< CBehaviorGraphNode* > nodes;
		m_behaviorGraph->GetAllNodes( nodes );

		const Uint32 nodesSize = nodes.Size();
		for ( Uint32 i = 0; i < nodesSize; ++i ) // validate nodes
		{
			CBehaviorGraphNode* currNode = nodes[i];

			if ( currNode->GetObjectIndex() == objectId )
			{
				FocusOnBehaviorNode( currNode );
				break;
			}
		}
	}
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
