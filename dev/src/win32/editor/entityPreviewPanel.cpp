/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "entityPreviewPanel.h"
#include "viewportWidgetMoveAxis.h"
#include "viewportWidgetRotateAxis.h"
#include "viewportWidgetScaleAxis.h"
#include "../../common/game/actor.h"
#include "entityEditorSlotItem.h"
#include "entityEditorWoundItem.h"
#include "entityEditor.h"
#include "animBrowser.h"
#include "../../common/engine/pathlibNavmeshComponent.h"
#include "../../common/engine/curveEntity.h"
#include "../../common/engine/curveControlPointEntity.h"
#include "../../common/engine/curveTangentControlPointEntity.h"
#include "../../common/engine/dismembermentComponent.h"
#include "filterPanel.h"
#include "../../common/core/feedback.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/hitProxyObject.h"
#include "../../common/engine/mimicComponent.h"
#include "../../common/engine/animGlobalParam.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/renderFrame.h"
#include "../../common/engine/fonts.h"
#include "dataError.h"
#include "../../common/engine/actorInterface.h"
#include "../../common/engine/meshTypeComponent.h"
#include "../../common/engine/meshTypeResource.h"

CGatheredResource resAimIcon( TXT("engine\\textures\\icons\\aimicon.xbm"), RGF_Startup );

//RED_DEFINE_STATIC_NAME( SelectionChanged )

#define ID_DISP_SKELETON				4001
#define ID_DISP_BONE_NAME				4002
#define ID_DISP_BONE_AXIS				4003
#define ID_DISP_SLOTS					4004
#define ID_DISP_SLOT_NAME				4005
#define ID_SAVE_SESSION_CAMERA			4007
#define ID_LOAD_SESSION_CAMERA			4006
#define ID_PLAY_ANIM					4010
#define ID_PLAY_DEFAULT_ANIM			4011
#define ID_STOP_ANIM					4012
#define ID_PLAY_PREV_ANIM_START			4013
#define ID_PLAY_PREV_ANIM_END			4018
#define ID_PLAY_ANIM_MIMICS_HIGH		4019
#define ID_DISP_WOUNDS					4020
#define ID_DISP_WOUND_NAME				4021
#define ID_SELECT_ALL					4022
#define ID_UNSELECT_ALL					4023
#define ID_INVERT_SELECTION				4024
#define ID_PAUSE_ANIM					4025
#define ID_PAUSE						4026
#define ID_PLAY_ANIM_MIMICS_LOW			4027

CEdEntityPreviewPanel::CEdEntityPreviewPanel( wxWindow* parent, CEntityTemplate* entityTemplate, EntityPreviewPanelHook* hook )
	: CEdInteractivePreviewPanel( parent, true )
	, CEdNavmeshEditor( 0.1f )
	, m_template( entityTemplate )
	, m_hook( hook )
	, m_dispSkeleton( false )
	, m_dispBoneNames( false )
	, m_dispBoneAxis( false )
	, m_itemPreviewSize( IPreviewItem::PS_Small )
	, m_textureArraysDataSize( TXT("no texture arrays data loaded") )
	, m_textureDataSize( TXT("no texture data loaded") )
	, m_meshDataSize( TXT("no render data loaded") )
	, m_bbDiagonalInfo( TXT("no bbox") )
	, m_showTBN( false )
	, m_showBoundingBox( false )
	, m_showCollision( false )
	, m_showWireframe( false )
	, m_activeSlotProperties( nullptr )
	, m_dispSlotActive( false )
	, m_isPaused( false )
	, m_triangleCountOfSelection( 0 )
{
	// Setup rendering mode
	GetViewport()->SetRenderingMode( RM_Shaded );
	GetViewport()->SetRenderingMask( SHOW_Sprites );
	GetViewport()->SetRenderingMask( SHOW_EnvProbesInstances );

	GetViewport()->SetRenderingDebugOptions( VDCommon_MaxRenderingDistance, wxTheFrame->GetFilterPanel()->GetVisualDebugMaxRenderingDistance() );
	GetViewport()->SetRenderingDebugOptions( VDCommon_DebugLinesThickness, wxTheFrame->GetFilterPanel()->GetDebugLinesThickness() );

	// Create preview entity
	EntitySpawnInfo einfo;
	einfo.m_template = m_template;
	einfo.m_detachTemplate = false;
	einfo.m_previewOnly = true;
	einfo.m_name = TXT("PreviewEntity");
	m_entity = GetPreviewWorld()->GetDynamicLayer()->CreateEntitySync( einfo );
	GetPreviewWorld()->DelayedActions();

	// Entity not spawned
	if ( !m_entity )
	{
		GFeedback->ShowError( TXT("Unable to edit selected entity template.") );
		return;
	}

	// Streaming setup
	m_entity->CreateStreamedComponents( SWN_NotifyWorld );
	m_entity->SetStreamingLock( true );

	for ( CComponent* comp : m_entity->GetComponents() )
	{
		comp->RefreshRenderProxies();
	}
	UpdateBBoxInfo();

	// Keep reference
	m_entity->AddToRootSet();

    // Register as event listener
	SEvents::GetInstance().RegisterListener( CNAME( SelectionChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( SelectionPropertiesChanged ), this );

	// Add widgets
	const Float rotateGismoSize = 0.7f;

    m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMoveAxis( Vector::EX, Color::RED ) );
    m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMoveAxis( Vector::EY, Color::GREEN ) );
    m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMoveAxis( Vector::EZ, Color::BLUE ) );
    m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMovePlane( Vector::EY, Vector::EZ, Color::LIGHT_RED ) );
    m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMovePlane( Vector::EX, Vector::EZ, Color::LIGHT_GREEN ) );
    m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMovePlane( Vector::EX, Vector::EY, Color::LIGHT_BLUE ) );
    m_widgetManager->AddWidget( TXT("Rotate"), new CViewportWidgetRotateAxis( EulerAngles( 0, 1, 0 ), Vector::EX, Color::RED, rotateGismoSize ) );
    m_widgetManager->AddWidget( TXT("Rotate"), new CViewportWidgetRotateAxis( EulerAngles( 1, 0, 0 ), Vector::EY, Color::GREEN, rotateGismoSize ) );
    m_widgetManager->AddWidget( TXT("Rotate"), new CViewportWidgetRotateAxis( EulerAngles( 0, 0, 1 ), Vector::EZ, Color::BLUE, rotateGismoSize ) );
    m_widgetManager->AddWidget( TXT("Scale"), new CViewportWidgetScaleAxis( Vector::EX, Color::RED ) );
    m_widgetManager->AddWidget( TXT("Scale"), new CViewportWidgetScaleAxis( Vector::EY, Color::GREEN ) );
    m_widgetManager->AddWidget( TXT("Scale"), new CViewportWidgetScaleAxis( Vector::EZ, Color::BLUE ) );
    m_widgetManager->AddWidget( TXT("Scale"), new CViewportWidgetScalePlane( Vector::EY, Vector::EZ, Color::LIGHT_RED ) );
    m_widgetManager->AddWidget( TXT("Scale"), new CViewportWidgetScalePlane( Vector::EX, Vector::EZ, Color::LIGHT_GREEN ) );
    m_widgetManager->AddWidget( TXT("Scale"), new CViewportWidgetScalePlane( Vector::EX, Vector::EY, Color::LIGHT_BLUE ) );
    m_widgetManager->AddWidget( TXT("Scale"), new CViewportWidgetScaleUniform( Vector::EX, Vector::EY, Vector::EZ, Color::WHITE ) );

    GetSelectionManager()->SetGranularity( CSelectionManager::SG_Components );

	// Create item container
	InitItemContainer();

	m_aimIcon = resAimIcon.LoadAndGet< CBitmapTexture >();

	LoadSessionAnimations();

	SetLightPosition( 360 );
}

CEdEntityPreviewPanel::~CEdEntityPreviewPanel()
{
	// Remove root reference
	if ( m_entity )
	{
		m_entity->RemoveFromRootSet();
		m_entity = nullptr;
	}

	// Destroy item container
	DestroyItemContainer();

	// Unregister from all events
    SEvents::GetInstance().UnregisterListener( this );

	StoreSessionCamera();

	StoreSessionAnimations();
}

void CEdEntityPreviewPanel::SetWidgetSpace( ERPWidgetSpace space )
{
	m_widgetManager->SetWidgetSpace( space );
}

void CEdEntityPreviewPanel::SetPreviewItemSize( IPreviewItem::PreviewSize size )
{
	m_itemPreviewSize = size;

	for ( Uint32 i=0; i<m_items.Size(); i++ )
	{
		m_items[i]->SetSize( m_itemPreviewSize );
	}
}

void CEdEntityPreviewPanel::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
    // Selection has changed
    if ( name == CNAME( SelectionChanged ) || name == CNAME( SelectionPropertiesChanged) )
	{
		UpdateBBoxInfo();
		if ( name == CNAME( SelectionChanged ) )
		{
			typedef CSelectionManager::SSelectionEventData SEventData;
			const SEventData& eventData = GetEventData< SEventData >( data );
			if ( eventData.m_world == m_previewWorld )
			{
				TDynArray< CNode* > nodes;
				GetSelectionManager()->GetSelectedNodes( nodes );
				m_widgetManager->EnableWidgets( nodes.Size() > 0 );
				ShowStatsFromSelection();
			}
		}
    }
}

Bool CEdEntityPreviewPanel::IsSkeleton()
{
	return m_dispSkeleton;
}

Bool CEdEntityPreviewPanel::IsBoneNames()
{
	return m_dispBoneNames;
}

Bool CEdEntityPreviewPanel::IsBoneAxis()
{
	return m_dispBoneAxis;
}

void CEdEntityPreviewPanel::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
	// Preview items
	if ( HandleItemSelection( objects ) )
	{
		return;
	}

    // Ask edit mode to handle event
    if ( m_tool && m_tool->HandleSelection( objects ) )
    {
        return;
    }

    CWorld* world = m_previewWorld;
    if ( world )
    {
		// Toggle selection
		CSelectionManager* selectionMgr = GetSelectionManager();
		CSelectionManager::CSelectionTransaction transaction(*selectionMgr);

		// Deselect all selected object
		if ( !RIM_IS_KEY_DOWN( IK_Shift ) )
		{
			selectionMgr->DeselectAll();
		}
        
		// Toggle selection
		for ( Uint32 i=0; i<objects.Size(); i++ )
		{
			CComponent* tc = Cast< CComponent >( objects[i]->GetHitObject() );
			if ( tc )
			{
				// Curve editing requires 'entity' granularity

				const Bool isCurveComponent = tc->IsA< CCurveComponent >() || tc->IsA< CCurveControlPointComponent >() || tc->IsA< CCurveTangentControlPointComponent >();

				if ( isCurveComponent && selectionMgr->GetGranularity() != CSelectionManager::SG_Entities )
				{
					continue;
				}
				else if ( !isCurveComponent && selectionMgr->GetGranularity() != CSelectionManager::SG_Components )
				{
					continue;
				}

				// Actually handle selection / deselection of the component

				if ( tc->IsSelected() && !m_selectionBoxDrag )
				{
					selectionMgr->Deselect( tc );
				}
				else if ( !tc->IsSelected() )
				{
					selectionMgr->Select( tc );
				}
			}
		}

        // Selection changed
        if ( EnableGraphEditing() && m_hook )
		{
            m_hook->OnPreviewSelectionChanged();
		}
    }
}

void CEdEntityPreviewPanel::HandleContextMenu( Int32 x, Int32 y )
{
	if ( !GetWorld() )
		return;

	Vector clickedWorldPos, clickedWorldNormal;
	GetWorld()->ConvertScreenToWorldCoordinates( GetViewport(), x, y, clickedWorldPos, &clickedWorldNormal );

	if ( m_tool && m_tool->HandleContextMenu( x, y, clickedWorldPos ) )
	{
		return;
	}

	for ( auto it = m_persistentTools.Begin(); it != m_persistentTools.End(); ++it )
	{
		if ( (*it)->HandleContextMenu( x, y, clickedWorldPos ) )
		{
			return;
		}
	}

	wxMenu menu;

	// selection submenu
	wxMenu* selectionMenu = new wxMenu();
	{
		selectionMenu->Append( ID_SELECT_ALL, TXT("Select All") );
		selectionMenu->Append( ID_UNSELECT_ALL, TXT("Unselect All") );
		selectionMenu->Append( ID_INVERT_SELECTION, TXT("Invert Selection") );
	}
	menu.Append( wxID_ANY, TXT("Select"), selectionMenu );
	menu.AppendSeparator();


	menu.AppendCheckItem( ID_DISP_SKELETON, TXT("Display skeleton") );
	menu.AppendCheckItem( ID_DISP_BONE_NAME, TXT("Display bones names") );
	menu.AppendCheckItem( ID_DISP_BONE_AXIS, TXT("Display bones axis") );
	menu.AppendCheckItem( ID_DISP_SLOTS, TXT("Display slots") );
	menu.AppendCheckItem( ID_DISP_SLOT_NAME, TXT("Display slot names") );
	menu.AppendCheckItem( ID_DISP_WOUNDS, TXT("Display dismemberment wounds") );
	menu.AppendCheckItem( ID_DISP_WOUND_NAME, TXT("Display dismemberment wound names") );

	menu.AppendSeparator();

	menu.Append( ID_SAVE_SESSION_CAMERA, TXT("Save session camera") );
	menu.Append( ID_LOAD_SESSION_CAMERA, TXT("Load session camera") );

	menu.AppendSeparator();

	menu.Append( ID_PAUSE, TXT("Pause") );

	menu.AppendSeparator();

	menu.Append( ID_PLAY_ANIM, TXT("Play animation - select") );
	menu.Append( ID_PLAY_DEFAULT_ANIM, TXT("Play default animation") );
	menu.Append( ID_PAUSE_ANIM, TXT("Pause animation") );
	menu.Append( ID_STOP_ANIM, TXT("Stop animation") );

	if ( m_entityPrevAnimationNames.Size() > 0 )
	{
		const Int32 size = Min< Int32 >( m_entityPrevAnimationNames.Size(), ID_PLAY_PREV_ANIM_END - ID_PLAY_PREV_ANIM_START );
		for ( Int32 i=size-1; i>=0; --i )
		{
			String str = String::Printf( TXT("Play animation - '%s'"), m_entityPrevAnimationNames[ i ].AsChar() );
			menu.Append( ID_PLAY_PREV_ANIM_START + i, str.AsChar() );
			menu.Connect( ID_PLAY_PREV_ANIM_START + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEntityPreviewPanel::OnPlayAnimaiton ), nullptr, this );
		}
	}

	menu.AppendSeparator();

	menu.Append( ID_PLAY_ANIM_MIMICS_HIGH, TXT("Mimics animation - high") );
	menu.Connect( ID_PLAY_ANIM_MIMICS_HIGH, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEntityPreviewPanel::OnPlayMimicsAnimaitonHigh ), nullptr, this );
	menu.Append( ID_PLAY_ANIM_MIMICS_LOW, TXT("Mimics animation - low") );
	menu.Connect( ID_PLAY_ANIM_MIMICS_LOW, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEntityPreviewPanel::OnPlayMimicsAnimaitonLow ), nullptr, this );

	menu.Check( ID_DISP_SKELETON, m_dispSkeleton );
	menu.Check( ID_DISP_BONE_NAME, m_dispBoneNames );
	menu.Check( ID_DISP_BONE_AXIS, m_dispBoneAxis );
	menu.Check( ID_DISP_SLOTS, m_dispSlots );
	menu.Check( ID_DISP_SLOT_NAME, m_dispSlotNames );
	menu.Check( ID_DISP_WOUNDS, m_dispWounds );
	menu.Check( ID_DISP_WOUND_NAME, m_dispWoundNames );

	menu.Connect( ID_SELECT_ALL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEntityPreviewPanel::OnSelectAll ), nullptr, this );
	menu.Connect( ID_UNSELECT_ALL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEntityPreviewPanel::OnUnselectAll ), nullptr, this );
	menu.Connect( ID_INVERT_SELECTION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEntityPreviewPanel::OnInvertSelection ), nullptr, this );

	menu.Connect( ID_DISP_SKELETON, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEntityPreviewPanel::OnContextMenuSelected ), nullptr, this ); 
	menu.Connect( ID_DISP_BONE_NAME, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEntityPreviewPanel::OnContextMenuSelected ), nullptr, this ); 
	menu.Connect( ID_DISP_BONE_AXIS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEntityPreviewPanel::OnContextMenuSelected ), nullptr, this ); 
	menu.Connect( ID_DISP_SLOTS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEntityPreviewPanel::OnContextMenuSelected ), nullptr, this ); 
	menu.Connect( ID_DISP_SLOT_NAME, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEntityPreviewPanel::OnContextMenuSelected ), nullptr, this ); 
	menu.Connect( ID_DISP_WOUNDS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEntityPreviewPanel::OnContextMenuSelected ), nullptr, this ); 
	menu.Connect( ID_DISP_WOUND_NAME, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEntityPreviewPanel::OnContextMenuSelected ), nullptr, this ); 
	
	menu.Connect( ID_SAVE_SESSION_CAMERA, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEntityPreviewPanel::OnContextMenuSelected ), nullptr, this );
	menu.Connect( ID_LOAD_SESSION_CAMERA, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEntityPreviewPanel::OnContextMenuSelected ), nullptr, this );

	menu.Connect( ID_PLAY_ANIM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEntityPreviewPanel::OnPlayAnimaiton ), nullptr, this );
	menu.Connect( ID_PLAY_DEFAULT_ANIM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEntityPreviewPanel::OnPlayDefaultAnimaiton ), nullptr, this );
	menu.Connect( ID_STOP_ANIM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEntityPreviewPanel::OnStopAnimation ), nullptr, this );
	menu.Connect( ID_PAUSE_ANIM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEntityPreviewPanel::OnPauseAnimation ), nullptr, this );

	menu.Connect( ID_PAUSE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEntityPreviewPanel::OnPause ), nullptr, this );

	PopupMenu( &menu, x, y );
}

void CEdEntityPreviewPanel::OnWidgetSelected( wxCommandEvent &event )
{
	if ( event.GetId() == XRCID("widgetModeMove") )
	{
        m_widgetManager->SetWidgetMode( RPWM_Move );
	}
	else if ( event.GetId() == XRCID("widgetModeRotate") )
	{
        m_widgetManager->SetWidgetMode( RPWM_Rotate );
	}
	else if ( event.GetId() == XRCID("widgetModeScale") )
	{
        m_widgetManager->SetWidgetMode( RPWM_Scale );
	}
    else if ( event.GetId() == XRCID("widgetModeChange") )
    {
		ERPWidgetMode nextMode = (ERPWidgetMode)( m_widgetManager->GetWidgetMode() + 1 );
		if ( (int)nextMode > (int)RPWM_Scale ) 
		{
			nextMode = RPWM_Move;
		}
        m_widgetManager->SetWidgetMode( nextMode );
    }

	// Enable editor widgets when there is selection
    TDynArray< CNode* > nodes;
    GetSelectionManager()->GetSelectedNodes( nodes );
    m_widgetManager->EnableWidgets( nodes.Size() > 0 );

    // Selection changed
    if ( EnableGraphEditing() && m_hook )
	{
        m_hook->OnPreviewWidgetModeChanged();
	}
}

void CEdEntityPreviewPanel::OnToggleWidgetSpace( wxCommandEvent& event )
{
	if ( event.GetId() == XRCID("widgetPivotLocal") )
	{
		m_widgetManager->SetWidgetSpace( RPWS_Local );
	}
	else if ( event.GetId() == XRCID("widgetPivotGlobal") )
	{
		m_widgetManager->SetWidgetSpace( RPWS_Global );
	}

	if ( m_hook )
	{
		m_hook->OnPreviewWidgetSpaceChanged();
	}
}
/*
void CEdEntityPreviewPanel::ChangeGranularity( ESelectionGranularity granularity )
{
	m_previewWorld->GetSelectionManager()->SetGranularity( granularity );
}
*/
void CEdEntityPreviewPanel::SetTemplate( CEntityTemplate* entityTemplate )
{
	// Destroy existing entity
	if ( m_entity )
	{
		GetPreviewWorld()->DelayedActions();
		CEntity *entity = m_entity;
		m_entity = nullptr;
		entity->RemoveFromRootSet();
		entity->DetachFromWorld( GetPreviewWorld() );
		GetPreviewWorld()->DelayedActions();
		entity->Destroy();
		GetPreviewWorld()->DelayedActions();
	}

	// Create preview entity
	if ( entityTemplate )
	{
		EntitySpawnInfo einfo;
		einfo.m_template = entityTemplate;
		einfo.m_name = TXT("PreviewEntity");
		einfo.m_detachTemplate = false;
		einfo.m_previewOnly = true;
		m_entity = GetPreviewWorld()->GetDynamicLayer()->CreateEntitySync( einfo );
		GetPreviewWorld()->DelayedActions();
		if ( m_entity != nullptr )
		{
			m_entity->AddToRootSet();
			m_entity->CreateStreamedComponents( SWN_NotifyWorld );
			m_entity->SetStreamingLock( true );

			if ( m_entityAnimationName )
			{
				PlayAnimation( m_entityAnimationName );
			}
		}
	}	
}

Bool CEdEntityPreviewPanel::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	if ( m_tool && m_tool->OnViewportInput( view, key, action, data ) )
	{
		return true;
	}

	for ( auto it = m_persistentTools.Begin(); it != m_persistentTools.End(); ++it )
	{
		if ( (*it)->OnViewportInput( view, key, action, data ) )
		{
			return true;
		}
	}

	if ( key == IK_T && action == IACT_Press )
	{
		m_widgetManager->SetWidgetMode( RPWM_Move );
		return true;
	}
	else if ( key == IK_R && action == IACT_Press )
	{
		m_widgetManager->SetWidgetMode( RPWM_Rotate );
		return true;
	}

	if ( key == IK_Delete && action == IACT_Press )
	{
		OnDelete();
		return true;
	}

	return CEdInteractivePreviewPanel::OnViewportInput( view, key, action, data );
}

void CEdEntityPreviewPanel::OnViewportTick( IViewport* view, Float timeDelta )
{
	// Tick base
	if ( !m_isPaused )
	{
		CEdInteractivePreviewPanel::OnViewportTick( view, timeDelta );
	}
	else
	{
		CEdRenderingPanel::OnViewportTick( view, timeDelta );
	}
}

void CEdEntityPreviewPanel::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	// Scene
	CEdInteractivePreviewPanel::OnViewportGenerateFragments( view, frame );

	// Display skeleton - check flag for efficiency
	if ( m_entity && ( IsSkeleton() || IsBoneNames() || IsBoneAxis() ) )
	{
		// Get all animated components from entity
		TDynArray< CAnimatedComponent* > animComps;
		CollectEntityComponents( m_entity, animComps );

		// Display skeleton
		for ( Uint32 i=0; i<animComps.Size(); i++ )
		{
			DisplaySkeleton( frame, animComps[i] );
		}
	}

	Int32	lineIndex = 1;
	Uint32 	lineHeight = 1.2f*m_font->GetLineDist();

	// Distance to entity
	if ( m_entity && frame->GetFrameInfo().IsShowFlagOn( SHOW_TriangleStats ) )
	{
		const Float distance = m_entity->GetWorldPosition().DistanceTo( frame->GetFrameInfo().m_camera.GetPosition() );		
		frame->AddDebugScreenText( 30, 10 + lineIndex*lineHeight, String::Printf( TXT("Distance: %1.2fm"), distance ), 0, true, Color(255,255,255), Color(0,0,0) );
		++lineIndex;
	}

	// Generate special shit for entity
	if ( m_entity )
	{
		// Entity debug fragments
		m_entity->GenerateDebugFragments( frame );

		// Center sprite at the aim position
		CGameplayEntity *gameplayEntity = Cast< CGameplayEntity >( m_entity );
		if ( gameplayEntity )
		{
			Vector aimPos = gameplayEntity->GetAimPosition();
			if ( aimPos != Vector::ZEROS )
			{
				Float screenScale = frame->GetFrameInfo().CalcScreenSpaceScale( aimPos );
				const Float size = Max( 0.25f, 0.25f * screenScale * 0.33f );
				frame->AddSprite( aimPos, size, Color::WHITE, CHitProxyID(), m_aimIcon, true );
			}
		}

		if ( IActorInterface* actor = m_entity->QueryActorInterface() )
		{
			if ( CMimicComponent* m = actor->GetMimicComponent() )
			{
				m->Editor_RefreshCameraPosition( GetCameraPosition(), GetCameraFov() );

				String desc;
				m->Editor_GetStateDesc( desc );

				frame->AddDebugScreenText( 130, 10 + (lineIndex-1)*lineHeight, desc, 0, true, Color(255,255,255), Color(0,0,0) );
			}
		}

		// Slots and slot names
		if ( m_dispSlots || m_dispSlotNames || m_dispSlotActive )
		{
			TDynArray< const EntitySlot* > slots;
			// collect selected slot
			if ( m_dispSlotActive && m_activeSlotProperties )
			{
				const EntitySlot* slot = m_activeSlotProperties->GetSlot();
				if ( !slot ) return;
				slots.PushBack( slot );
			}
			// otherwise collect all slots from entity template
			else
			{
				// Get all the slots
				m_template->CollectSlots( slots, true );
			}

			Matrix slotMatrix = Matrix::IDENTITY;
			const Uint32 slotSize = slots.Size();
			for ( Uint32 i = 0; i<slotSize; ++i )
			{
				if ( slots[i]->CalcMatrix( m_entity, slotMatrix, nullptr ) )
				{
					const Vector o = slotMatrix.TransformPoint( Vector::ZEROS );

					// Draw axes
					if ( m_dispSlots || m_dispSlotActive )
					{
						const Vector dx = slotMatrix.GetAxisX();
						const Vector dy = slotMatrix.GetAxisY();
						const Vector dz = slotMatrix.GetAxisZ();
						frame->AddDebugLine( o, o+dx*0.3f, Color::RED, false );
						frame->AddDebugLine( o, o+dy*0.3f, Color::GREEN, false );
						frame->AddDebugLine( o, o+dz*0.3f, Color::BLUE, false );
					}

					// Draw slot names
					if ( m_dispSlotNames )
					{
						frame->AddDebugText( o, slots[i]->GetName().AsString(), 0, 0 );
					}
				}
			}
		}


		if ( m_dispWounds || m_dispWoundNames )
		{
			GenerateWoundFragments( view, frame );
		}

		// Estimate the memory size
		frame->AddDebugScreenText( 30, 10 + lineIndex*lineHeight, m_textureArraysDataSize.wc_str(), 0, true, Color(255,255,255), Color(0,0,0) );
		++lineIndex;
		frame->AddDebugScreenText( 30, 10 + lineIndex*lineHeight, m_textureDataSize.wc_str(), 0, true, Color(255,255,255), Color(0,0,0) );
		++lineIndex;
		frame->AddDebugScreenText( 30, 10 + lineIndex*lineHeight, m_meshDataSize.wc_str(), 0, true, Color(255,255,255), Color(0,0,0) );		
		++lineIndex;

		frame->AddDebugScreenText( 30, 10 + lineIndex*lineHeight, m_bbDiagonalInfo.wc_str(), 0, true, Color(255,255,255), Color(0,0,0) );
		++lineIndex;

#ifdef PROFILING_PHYSX
		static CPerfCounter* physicsFetchTimePerfCounter = 0;
		if( !physicsFetchTimePerfCounter ) physicsFetchTimePerfCounter = CProfiler::GetCounter( "Physics scene preview simulation" );

		if( physicsFetchTimePerfCounter )
		{
			const Double freq = Red::System::Clock::GetInstance().GetTimer().GetFrequency();

			static Uint64 previousTime = 0;
			const Uint64 time = physicsFetchTimePerfCounter->GetTotalTime();
			static Uint32 previousHit = 0;
			wxString string = wxString( "Physics simulation: " ) + wxString::Format( wxT("%1.6f ms"), (float)((time - previousTime)/freq)*1000.0f );
			frame->AddDebugScreenText( 30, 10 + lineIndex*lineHeight, string.wc_str(), 0, false, Color(255,255,255), Color(0,0,0) );	
			previousTime = time;
		}
#endif

		lineIndex++;

		// draw informations from data error reporter
		CEdEntityEditor* entityEditor = dynamic_cast< CEdEntityEditor* >( m_hook );
		if ( entityEditor )
		{
			// few extra lines to prevent from displaying on UMBRA thing information
			lineIndex += 3;
			TDynArray< String > dataErrors;
			entityEditor->GetDataErrors( dataErrors );
			Uint32 errorCount = dataErrors.Size();
			if( errorCount > 0 )
			{
				const Uint32 maxDisplayMessages = 5;

				// draw header
				const String errorsHeader = TXT("DATA ERROR REPORTS:");
				frame->AddDebugScreenText( 30, 50 + lineIndex*lineHeight, errorsHeader.AsChar(), 0, false, Color(255,0,0), Color(0,0,0) );	
				++lineIndex;

				// draw priorities' legend 
				frame->AddDebugScreenText( 30, 50 + lineIndex*lineHeight, TXT("Priorities: 1 - critical; 2 - error; 3 - warning; 4 - bug"), 
					0, false, Color(0,255,0), Color(0,0,0) );
				++lineIndex;

				// draw messages
				const Uint32 displayMessageCount = ( errorCount > maxDisplayMessages ) ? maxDisplayMessages : errorCount;
				for( Uint32 i = 0; i < displayMessageCount; ++i )
				{
					frame->AddDebugScreenText( 30, 50 + lineIndex*lineHeight, dataErrors[i].AsChar(), 0, false, Color(255,0,0), Color(0,0,0) );	
					++lineIndex;
				}

				if( errorCount > maxDisplayMessages )
				{
					frame->AddDebugScreenText( 30, 50 + lineIndex*lineHeight, String::Printf( TXT("and %d more messages"), errorCount-maxDisplayMessages ), 0, false, Color(255,0,0), Color(0,0,0) );	
					++lineIndex;
				}
			}
		}	
	}
	// Start at middle and bottom
	Uint32 x = frame->GetFrameOverlayInfo().m_width / 2;
	Uint32 y = frame->GetFrameOverlayInfo().m_height - 20;
	// if they are meshcomponents calculate selected triangles count
	frame->AddDebugScreenText( x, y, String::Printf( TXT("Selected Triangles %d"), m_triangleCountOfSelection ), 0, false, Color(255,255,255), Color(0,0,0) );	
	
	if ( m_showBoundingBox ) 
	{
		this->GetViewport()->SetRenderingMask( SHOW_Bboxes );
	}
	else
	{
		this->GetViewport()->ClearRenderingMask( SHOW_Bboxes );
	}
	if ( m_showCollision )
	{
		this->GetViewport()->SetRenderingMask( SHOW_Collision );
	}
	else
	{
		this->GetViewport()->ClearRenderingMask( SHOW_Collision );
	}
	if ( m_showTBN )
	{
		this->GetViewport()->SetRenderingMask( SHOW_TBN );
	}
	else
	{
		this->GetViewport()->ClearRenderingMask( SHOW_TBN );
	}
	if ( m_showWireframe )
	{
		this->GetViewport()->SetRenderingMask( SHOW_Wireframe );
	}
	else
	{
		this->GetViewport()->ClearRenderingMask( SHOW_Wireframe );
	}
	CEdNavmeshEditor::HandleViewportGenerateFragments( view, frame );
}

void CEdEntityPreviewPanel::OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera )
{
	CEdInteractivePreviewPanel::OnViewportCalculateCamera( view, camera );

	Vector forward, up;
	camera.GetRotation().ToAngleVectors( &forward, nullptr, &up );

	if( !GetEntity() ) return;

	UpdateSoundListener( camera.GetPosition(), up, forward, GetEntity()->GetSoundEmitterComponent( false ) );
}

Bool CEdEntityPreviewPanel::OnViewportMouseMove( const CMousePacket& packet )
{
	Bool b = CEdNavmeshEditor::HandleViewportMouseMove( packet );
	return CEdInteractivePreviewPanel::OnViewportMouseMove( packet ) || b;
}
Bool CEdEntityPreviewPanel::OnViewportTrack( const CMousePacket& packet )
{
	Bool b = CEdNavmeshEditor::HandleViewportTrack( packet );
	return CEdInteractivePreviewPanel::OnViewportTrack( packet ) || b;
}



void CEdEntityPreviewPanel::OnContextMenuSelected( wxCommandEvent &event )
{
	switch ( event.GetId() )
	{
	case ID_DISP_SKELETON:
		m_dispSkeleton = event.IsChecked();
		break;
	case ID_DISP_BONE_NAME:
		m_dispBoneNames = event.IsChecked();
		break;
	case ID_DISP_BONE_AXIS:
		m_dispBoneAxis = event.IsChecked();
		break;

	case ID_DISP_SLOTS:
		m_dispSlots = event.IsChecked();
		break;
	case ID_DISP_SLOT_NAME:
		m_dispSlotNames = event.IsChecked();
		break;

	case ID_DISP_WOUNDS:
		m_dispWounds = event.IsChecked();
		break;
	case ID_DISP_WOUND_NAME:
		m_dispWoundNames = event.IsChecked();
		break;

	case ID_SAVE_SESSION_CAMERA:
		StoreSessionCamera();
		break;
	case ID_LOAD_SESSION_CAMERA:
		LoadSessionCamera();
		break;
	}
}

void CEdEntityPreviewPanel::OnPlayAnimaiton( wxCommandEvent& event )
{
	if ( !m_entity )
	{
		return;
	}

	CAnimatedComponent* component = m_entity->GetRootAnimatedComponent();
	if ( !component )
	{
		return;
	}

	if ( event.GetId() >= ID_PLAY_PREV_ANIM_START && event.GetId() < ID_PLAY_PREV_ANIM_END )
	{
		const Uint32 index = event.GetId() - ID_PLAY_PREV_ANIM_START;
		ASSERT( index < m_entityPrevAnimationNames.Size() );

		if ( index < m_entityPrevAnimationNames.Size() )
		{
			const CName& animName = m_entityPrevAnimationNames[ index ];
			PlayAnimation( animName );
		}
	}
	else
	{
		// Spawn animation browser	
		CEdAnimBrowser* animBrowser = new CEdAnimBrowser( this );
		animBrowser->CloneEntityFromComponent( component );

		animBrowser->ShowForSelection();

		animBrowser->SelectAnimation( m_entityAnimationName.AsString() );

		animBrowser->Connect( wxEVT_ANIM_CONFIRMED, wxCommandEventHandler( CEdEntityPreviewPanel::OnAnimationConfirmed ), nullptr, this );
		animBrowser->Connect( wxEVT_ANIM_ABANDONED, wxCommandEventHandler( CEdEntityPreviewPanel::OnAnimationAbandoned ), nullptr, this );
	}
}

void CEdEntityPreviewPanel::OnPlayMimicsAnimaitonHigh( wxCommandEvent& event )
{
	PlayMimicsAnimation( CName( TXT("normal_blend_test_face") ), true );
}

void CEdEntityPreviewPanel::OnPlayMimicsAnimaitonLow( wxCommandEvent& event )
{
	PlayMimicsAnimation( CName( TXT("npc_pose_hit_01") ), false );
}

Bool CEdEntityPreviewPanel::PlayAnimation( const CName& animationName )
{
	Bool ret = false;

	m_entityAnimationName = animationName;

	if ( m_entity )
	{
		CAnimatedComponent* component = m_entity->GetRootAnimatedComponent();
		if ( component && m_entityAnimationName )
		{
			ret = component->PlayAnimationOnSkeleton( m_entityAnimationName );
		}
	}

	if ( m_entityPrevAnimationNames.Exist( m_entityAnimationName ) )
	{
		m_entityPrevAnimationNames.Remove( m_entityAnimationName );
		m_entityPrevAnimationNames.PushBack( m_entityAnimationName );
	}
	else
	{
		const Uint32 maxSize = ( ID_PLAY_PREV_ANIM_END - ID_PLAY_PREV_ANIM_START );
		if ( m_entityPrevAnimationNames.Size() + 1 >= maxSize )
		{
			Int32 i = m_entityPrevAnimationNames.Size() - maxSize + 1;
			while ( i > 0 )
			{
				m_entityPrevAnimationNames.RemoveAt( 0 );
				i--;
			}
		}

		m_entityPrevAnimationNames.PushBack( m_entityAnimationName );
	}

	return ret;
}

Bool CEdEntityPreviewPanel::PlayMimicsAnimation( const CName& animationName, Bool high )
{
	if ( !m_entity )
	{
		return false;
	}

	IActorInterface* actor = m_entity->QueryActorInterface();
	if ( !actor )
	{
		return false;
	}

	CMimicComponent* component = actor->GetMimicComponent();
	if ( component )
	{
		if ( high )
		{
			actor->MimicOn();

			return component->PlayMimicAnimation( animationName, CNAME( MIMIC_SLOT ) );
		}
		else
		{
			actor->MimicOff();

			return component->PlayMimicAnimation( animationName, CNAME( MIMIC_GMPL_GESTURE_SLOT ) );
		}
	}

	return false;
}

void CEdEntityPreviewPanel::OnAnimationConfirmed( wxCommandEvent &event )
{
	wxString animationStr = event.GetString();
	PlayAnimation( CName( animationStr.wc_str() ) );
}

void CEdEntityPreviewPanel::OnAnimationAbandoned( wxCommandEvent &event )
{
	if ( m_entity )
	{
		CAnimatedComponent* component = m_entity->GetRootAnimatedComponent();
		if ( component )
		{
			component->StopAllAnimationsOnSkeleton();
		}
	}
}

void CEdEntityPreviewPanel::OnPlayDefaultAnimaiton( wxCommandEvent& event )
{
	if ( m_template && m_entity )
	{
		CAnimatedComponent* component = m_entity->GetRootAnimatedComponent();
		if ( component )
		{
			CAnimGlobalParam* param = m_template->FindParameter< CAnimGlobalParam >();
			if ( param )
			{
				if ( !PlayAnimation( param->GetDefaultAnimationName() ) )
				{
					wxMessageBox( wxT("Default animation is empty. Check CAnimGlobalParam in 'Animation' tab."), wxT("Entity Editor") );
				}
			}
			else
			{
				wxMessageBox( wxT("Entity template hasn't got CAnimGlobalParam in 'Animation' tab."), wxT("Entity Editor") );
			}
		}
	}
}

void CEdEntityPreviewPanel::OnStopAnimation( wxCommandEvent& event )
{
	if ( m_entity )
	{
		CAnimatedComponent* component = m_entity->GetRootAnimatedComponent();
		if ( component )
		{
			component->StopAllAnimationsOnSkeleton();
		}
	}

	m_entityAnimationName = CName::NONE;
}

void CEdEntityPreviewPanel::OnPauseAnimation( wxCommandEvent &event )
{
	if ( m_entity )
	{
		CAnimatedComponent* component = m_entity->GetRootAnimatedComponent();
		if ( component )
		{
			component->TogglePauseAllAnimationsOnSkeleton();
		}
	}
}

void CEdEntityPreviewPanel::OnPause( wxCommandEvent &event )
{
	m_isPaused = !m_isPaused;
}

void CEdEntityPreviewPanel::StoreSessionCamera()
{
	// Store camera positioning
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	{
		CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/EntityEditor") + m_template->GetFile()->GetFileName());

		config.Write( TXT("SessionCameraX"), GetCameraPosition().X );
		config.Write( TXT("SessionCameraY"), GetCameraPosition().Y );
		config.Write( TXT("SessionCameraZ"), GetCameraPosition().Z );

		config.Write( TXT("SessionCameraYaw"), GetCameraRotation().Yaw );
		config.Write( TXT("SessionCameraPitch"), GetCameraRotation().Pitch );
		config.Write( TXT("SessionCameraRoll"), GetCameraRotation().Roll );
	}
}

void CEdEntityPreviewPanel::LoadSessionCamera()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	{
		CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/EntityEditor/") + m_template->GetFile()->GetFileName() );

		Float x = config.Read( TXT("SessionCameraX"), GetCameraPosition().X );
		Float y = config.Read( TXT("SessionCameraY"), GetCameraPosition().Y );
		Float z = config.Read( TXT("SessionCameraZ"), GetCameraPosition().Z );

		SetCameraPosition( Vector( x,y,z ) );

		Float yaw   = config.Read( TXT("SessionCameraYaw"), GetCameraRotation().Yaw );
		Float pitch = config.Read( TXT("SessionCameraPitch"), GetCameraRotation().Pitch );
		Float roll  = config.Read( TXT("SessionCameraRoll"), GetCameraRotation().Roll );

		SetCameraRotation( EulerAngles( roll, pitch, yaw ) );
	}
}

void CEdEntityPreviewPanel::StoreSessionAnimations()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	String str;
	const Uint32 size = m_entityPrevAnimationNames.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		str += m_entityPrevAnimationNames[ i ].AsString();
		str += TXT(",");
	}

	config.Write( TXT("/Frames/EntityEditor/Animations"), str );
}

void CEdEntityPreviewPanel::LoadSessionAnimations()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	const String str = config.Read( TXT("/Frames/EntityEditor/Animations"), String::EMPTY );
	TDynArray< String > tokens = str.Split( TXT(",") );

	const Uint32 size = tokens.Size();
	m_entityPrevAnimationNames.Resize( size );
	for ( Uint32 i=0; i<size; ++i )
	{
		m_entityPrevAnimationNames[ i ] = CName( tokens[ i ] );
	}
}

PathLib::CNavmesh* CEdEntityPreviewPanel::GetEditedNavmesh()
{
	CNavmeshComponent* component = GetEditedNavmeshComponent();
	return component ? component->GetNavmesh() : nullptr;
}
PathLib::CNavmesh* CEdEntityPreviewPanel::FindNavmeshForEdition( const CMousePacket& packet )
{
	CNavmeshComponent* component = GetEditedNavmeshComponent();
	return component ? component->GetNavmesh() : nullptr;
}
CWorld* CEdEntityPreviewPanel::GetEditedWorld()
{
	return m_entity->GetLayer()->GetWorld();
}
CNavmeshComponent* CEdEntityPreviewPanel::GetEditedNavmeshComponent()
{
	const auto& components = m_entity->GetComponents();
	for ( auto it = components.Begin(), end = components.End(); it != end; ++it )
	{
		if ( (*it)->IsA< CNavmeshComponent >() )
		{
			return static_cast< CNavmeshComponent* >( *it );
		}
	}
	return nullptr;
}

Bool CEdEntityPreviewPanel::EnableGraphEditing()
{
	TDynArray< CNode* > nodes;
	GetSelectionManager()->GetSelectedNodes( nodes );

	// Make sure no curve is selected

	for ( auto it = nodes.Begin(); it != nodes.End(); ++it )
	{
		CNode* node = *it;
		if ( node->IsA< CCurveComponent >() || node->IsA< CCurveControlPointComponent >() || node->IsA< CCurveTangentControlPointComponent >() )
		{
			return false;
		}
	}
	return true;
}

void CEdEntityPreviewPanel::OnItemTransformChangedFromPreview( IPreviewItem* item )
{
	if ( EnableGraphEditing() && m_hook )
	{
		m_hook->OnPreviewItemTransformChanged();
	}
}

void CEdEntityPreviewPanel::RefreshPreviewSlotItem( const EntitySlot* selectedSlot )
{
	if ( selectedSlot && HasItem( selectedSlot->GetName().AsString() ) )
	{
		RefreshItems();
		return;
	}

	ClearItems();

	if ( selectedSlot )
	{
		CEntityEditorSlotItem* item = new CEntityEditorSlotItem( this, selectedSlot->GetName() );
		item->Init( selectedSlot->GetName().AsString() );
		item->SetColor( Color::YELLOW );
		item->SetSize( m_itemPreviewSize );

		Matrix mat;
		selectedSlot->CalcMatrix( m_entity, mat, nullptr );

		item->RefreshTransform( mat.GetTranslationRef(), mat.ToEulerAngles() );

		AddItem( item );
		item->Select();
	}
}

void CEdEntityPreviewPanel::RefreshPreviewSlotChanges()
{
	RefreshItems();
}


void CEdEntityPreviewPanel::RefreshPreviewWoundItem( const CDismembermentWound* selectedWound )
{
	if ( selectedWound && HasItem( selectedWound->GetName().AsString() ) )
	{
		RefreshItems();
		return;
	}

	ClearItems();

	if ( selectedWound )
	{
		CEntityEditorWoundItem* item = new CEntityEditorWoundItem( this, selectedWound->GetName() );
		item->Init( selectedWound->GetName().AsString() );
		item->SetColor( Color::YELLOW );
		item->SetSize( m_itemPreviewSize );

		item->RefreshTransform( selectedWound->GetTransform() );

		AddItem( item );
	}
}

void CEdEntityPreviewPanel::RefreshPreviewWoundChanges()
{
	RefreshItems();
}


void CEdEntityPreviewPanel::GenerateWoundFragments( IViewport *view, CRenderFrame *frame )
{
	const CDismembermentComponent* dismemberComponent = m_entity->FindComponent< CDismembermentComponent >();
	if ( dismemberComponent == nullptr )
	{
		return;
	}

	TDynArray< const CDismembermentWound* > wounds;
	CEntityDismemberment::GetEnabledWoundsRecursive( m_entity, wounds );

	// Display wounds
	for ( const CDismembermentWound* wound : wounds )
	{
		Matrix woundMatrix;
		wound->GetTransform().CalcLocalToWorld( woundMatrix );

		const Vector o = woundMatrix.TransformPoint( Vector::ZEROS );

		if ( m_dispWounds )
		{
			Matrix mX, mY;
			mX.SetIdentity();
			mX.SetRotX33( DEG2RAD(90) );
			mY.SetIdentity();
			mY.SetRotY33( DEG2RAD(90) );

			frame->AddDebugCircle( Vector::ZEROS, 1.0f, woundMatrix, Color::RED, 24 );
			frame->AddDebugCircle( Vector::ZEROS, 1.0f, mX * woundMatrix, Color::RED, 24 );
			frame->AddDebugCircle( Vector::ZEROS, 1.0f, mY * woundMatrix, Color::RED, 24 );
		}

		if ( m_dispWoundNames )
		{
			frame->AddDebugText( o, wound->GetName().AsString(), 0, 0 );
		}
	}
}

void CEdEntityPreviewPanel::ShowTBN( Bool val )
{
	m_showTBN = val;
}

void CEdEntityPreviewPanel::ShowWireframe( Bool val )
{
	m_showWireframe = val;
}

void CEdEntityPreviewPanel::ShowBB( Bool val )
{
	m_showBoundingBox = val;
}

void CEdEntityPreviewPanel::ShowCollision( Bool val )
{
	m_showCollision = val;
}

void CEdEntityPreviewPanel::SetEntitySlotProperties( CEdEntitySlotProperties* entSlotProperties )
{
	m_activeSlotProperties = entSlotProperties;
}

void CEdEntityPreviewPanel::OnSelectAll( wxCommandEvent& event )
{
	SelectAll();
}

void CEdEntityPreviewPanel::OnUnselectAll( wxCommandEvent& event )
{
	UnselectAll();
}

void CEdEntityPreviewPanel::OnInvertSelection( wxCommandEvent& event )
{
	InvertSelection();
}

void CEdEntityPreviewPanel::SelectAll()
{
	if( m_previewWorld != nullptr )
	{
		GetSelectionManager()->SelectAll();

		// Selection changed
		if ( EnableGraphEditing() && m_hook != nullptr )
		{
			m_hook->OnPreviewSelectionChanged();
		}
	}
}

void CEdEntityPreviewPanel::UnselectAll()
{
	CWorld* world = m_previewWorld;
	if( m_previewWorld != nullptr )
	{
		CSelectionManager::CSelectionTransaction transaction(*GetSelectionManager());

		GetSelectionManager()->DeselectAll();

		// Selection changed
		if ( EnableGraphEditing() && m_hook != nullptr )
		{
			m_hook->OnPreviewSelectionChanged();
		}
	}
}

void CEdEntityPreviewPanel::InvertSelection()
{
	if ( m_previewWorld != nullptr )
	{
		GetSelectionManager()->InvertSelection();

		// Selection changed
		if ( EnableGraphEditing() && m_hook != nullptr )
		{
			m_hook->OnPreviewSelectionChanged();
		}
	}
}

void CEdEntityPreviewPanel::UpdateBBoxInfo()
{
	if ( m_entity )
	{
		Box bbox = m_entity->CalcBoundingBox();
		Float halfDiag = ( bbox.Max - bbox.Min ).Mag3() * 0.5f;
		m_bbDiagonalInfo = wxString( "Half of bbox diagonal: " ) + ToString( halfDiag ).AsChar();
	}
	else
	{
		m_bbDiagonalInfo = TXT("");
	}
}

void CEdEntityPreviewPanel::ShowStatsFromSelection()
{
	m_triangleCountOfSelection = 0;
	TDynArray< CNode* > nodes = GetSelectionManager()->GetSelectedNodes();

	for ( auto it = nodes.Begin(); it != nodes.End(); ++it )
	{
		CNode* node = *it;
		if ( node->IsA< CMeshTypeComponent >() )
		{
			CMeshTypeComponent* meshComp = Cast<CMeshTypeComponent>( node );
			CMeshTypeResource* meshRes = meshComp->GetMeshTypeResource();
			if( meshRes )
			{
				Uint32 tris = meshRes->CountLODTriangles(0);
				m_triangleCountOfSelection += tris;
			}	
		}
	}
}

void CEdEntityPreviewPanel::EnableManualSlotChange( Bool status )
{
	if ( status )
	{
		const EntitySlot* slot = m_activeSlotProperties->GetSlot();
		RefreshPreviewSlotItem( slot );
	}
	else
	{
		RefreshPreviewSlotItem( nullptr );
	}
}

void CEdEntityPreviewPanel::ShowActiveSlot(Bool status)
{
	m_dispSlotActive = status;
	m_dispSlotNames = status;
	if ( !status )
	{
		m_dispSlots = false;
	}
}
