/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "../../common/engine/behaviorGraphInstance.h"
#include "../../common/engine/inputManager.h"

#include "../../common/game/actor.h"
#include "../../common/game/inventoryComponent.h"

#include "behaviorEditor.h"
#include "behaviorGraphEditor.h"
#include "behaviorPreviewPanel.h"
#include "viewportWidgetMoveAxis.h"
#include "viewportWidgetRotateAxis.h"
#include "viewportWidgetScaleAxis.h"
#include "../../common/core/diskFile.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/camera.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/renderFrame.h"


CGatheredResource resPreviewCameraTemplate( TXT("gameplay\\camera\\camera.w2ent"), 0 );

//////////////////////////////////////////////////////////////////////////

CBehaviorVarItem::CBehaviorVarItem( CEdBehaviorPreviewPanel* behPreview, Bool internalVariable )
	: m_preview( behPreview )
	, m_internalVariable( internalVariable )
{
}

Bool CBehaviorVarItem::IsValid() const
{
	return GetVarName() != CName::NONE;
}

void CBehaviorVarItem::Refresh()
{
	const CName varName = GetVarName();
	if ( varName != CName::NONE )
	{
		CBehaviorGraphInstance* instance = m_preview->GetBehaviorGraphInstance();
		Vector newPos = m_internalVariable? instance->GetInternalVectorValue( varName ) : instance->GetVectorValue( varName );
		SetPosition( newPos );
	}
}

void CBehaviorVarItem::SetPositionFromPreview( const Vector& prevPos, Vector& newPos ) 
{
	const CName varName = GetVarName();
	if ( varName != CName::NONE )
	{
		CBehaviorGraphInstance* instance = m_preview->GetBehaviorGraphInstance();
		if (m_internalVariable)
		{
			instance->SetInternalVectorValue( varName, newPos );
		}
		else
		{
			instance->SetVectorValue( varName, newPos );
		}
	}
}

IPreviewItemContainer* CBehaviorVarItem::GetItemContainer() const
{
	return m_preview;
}

CName CBehaviorVarItem::GetVarName() const
{
	// first char is used to differentiate between normal and internal vectors
	return m_preview->GetBehaviorGraphInstance() ? CName( GetName().AsChar() ) : CName::NONE;
}

//////////////////////////////////////////////////////////////////////////

#define ID_MENU_ENTITY			100
#define ID_MENU_STICK			101
#define	ID_MENU_TELEPORT_TO_ZERO 102
#define ID_BEH_DRAW_ITEM		103
#define ID_SAVE_CAMERA			104
#define ID_LOAD_CAMERA			105
#define ID_SHOW_TRANS			106
#define ID_CACHE_TRANS			107
#define ID_MENU_ENTITY_PLAYER		200
#define ID_MENU_ENTITY_INCLUDETEST	201
#define ID_MENU_HISTORY_FIRST	500

BEGIN_EVENT_TABLE( CEdBehaviorPreviewPanel, CEdPreviewPanel )
	EVT_MENU( ID_MENU_ENTITY, CEdBehaviorPreviewPanel::OnLoadEntity )
	EVT_MENU( ID_MENU_ENTITY_PLAYER, CEdBehaviorPreviewPanel::OnLoadEntityPlayer )
	EVT_MENU( ID_MENU_ENTITY_INCLUDETEST, CEdBehaviorPreviewPanel::OnLoadEntityIncludeTest )
	EVT_MENU( ID_MENU_STICK, CEdBehaviorPreviewPanel::OnStickToEntity )
	EVT_MENU( ID_SAVE_CAMERA, CEdBehaviorPreviewPanel::OnSaveCameraOffsets )
	EVT_MENU( ID_LOAD_CAMERA, CEdBehaviorPreviewPanel::OnLoadCameraOffsets )
	EVT_MENU( ID_MENU_TELEPORT_TO_ZERO, CEdBehaviorPreviewPanel::OnTeleportToZero )
	EVT_MENU( ID_SHOW_TRANS, CEdBehaviorPreviewPanel::OnShowTransform )
	EVT_MENU( ID_CACHE_TRANS, CEdBehaviorPreviewPanel::OnCacheTransform )
END_EVENT_TABLE()

CEdBehaviorPreviewPanel::CEdBehaviorPreviewPanel( CEdBehaviorEditor* behaviorEditor )
	: CEdPreviewPanel( behaviorEditor, true )
	, CEdBehaviorEditorPanel( behaviorEditor )
	, m_stickToEntity( true )
	, m_previewCamera( NULL )
	, m_isMovingCameraAlign( false )
	, m_dynamicTarget( NULL )
	, m_showEntityTransform( false )
	, m_showEntityTransformPrevPos( Vector::ZERO_3D_POINT )
	, m_showEntityTransformPrevRot( EulerAngles::ZEROS )
	, m_cachedEntityTransformPos( Vector::ZERO_3D_POINT )
	, m_cachedEntityTransformRot( EulerAngles::ZEROS )
{
	SetCameraPosition( Vector( 0, 4, 2 ) );
	SetCameraRotation( EulerAngles( 0, -10, 180 ) );

	GetViewport()->SetRenderingMode( RM_Shaded );
	GetViewport()->SetRenderingMask( SHOW_Behavior );
	GetViewport()->SetRenderingMask( SHOW_VisualDebug );
	GetViewport()->SetRenderingMask( SHOW_Sprites );
	GetViewport()->SetRenderingMask( SHOW_Gizmo );

	// Add widgets - move only
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMoveAxis( Vector::EX, Color::RED ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMoveAxis( Vector::EY, Color::GREEN ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMoveAxis( Vector::EZ, Color::BLUE ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMovePlane( Vector::EY, Vector::EZ, Color::LIGHT_RED ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMovePlane( Vector::EX, Vector::EZ, Color::LIGHT_GREEN ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMovePlane( Vector::EX, Vector::EY, Color::LIGHT_BLUE ) );

	// Set local space
	m_widgetManager->SetWidgetSpace( RPWS_Local );

	// Select only components
	GetSelectionManager()->SetGranularity( CSelectionManager::SG_Components );

	// Create preview camera
	CreatePreviewCamera();

	// Create item container
	InitItemContainer();

	// Set editor world
	behaviorEditor->SetEditorWorld( m_previewWorld );

	// Register as event listener
	SEvents::GetInstance().RegisterListener( CNAME( SelectionChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( BehaviorEventGenerated ), this );
}

wxAuiPaneInfo CEdBehaviorPreviewPanel::GetPaneInfo() const
{
	wxAuiPaneInfo info = CEdBehaviorEditorPanel::GetPaneInfo();

	info.CloseButton( false ).PinButton( true ).Gripper( true ).Left().LeftDockable( true ).MinSize(100,100).Position(0).BestSize(400, 400);

	return info;
}

CEdBehaviorPreviewPanel::~CEdBehaviorPreviewPanel()
{
	// Destroy item container
	DestroyItemContainer();

	// Destroy preview camera
	DestroyPreviewCamera();

	// Reset editor world
	GetEditor()->SetEditorWorld( NULL );

	ASSERT( !m_dynamicTarget );

	SEvents::GetInstance().UnregisterListener( this );
}

void CEdBehaviorPreviewPanel::SaveSession( CConfigurationManager &config )
{
	config.Write( TXT("/Frames/BehaviorEditor/Shadows"), GetShadowsEnabled() ? 1 : 0 );
	config.Write( TXT("/Frames/BehaviorEditor/LightPosition"), GetLightPosition() );

	String colorStr = ToString( GetClearColor().ToVector() );
	config.Write( TXT("/Frames/BehaviorEditor/Color"), colorStr );
}

void CEdBehaviorPreviewPanel::RestoreSession( CConfigurationManager &config )
{
	SetShadowsEnabled( config.Read( TXT("/Frames/BehaviorEditor/Shadows"), 1 ) == 1 ? true : false );
	SetLightPosition( config.Read( TXT("/Frames/BehaviorEditor/LightPosition"), 135 ) );

	/*
	String colorStr = config.Read( TXT("/Frames/BehaviorEditor/Color"), String::EMPTY );
	if ( colorStr.Empty() == false )
	{
		Vector colorVec;
		FromString( colorStr, colorVec );
		Color color( colorVec );
		m_clearColor = color;
	}
	*/
}

void CEdBehaviorPreviewPanel::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( SelectionChanged ) )
	{
		// Selection has changed
		typedef CSelectionManager::SSelectionEventData SEventData;
		const SEventData& eventData = GetEventData< SEventData >( data );
		if ( eventData.m_world == m_previewWorld )
		{
			TDynArray< CNode* > nodes;
			GetSelectionManager()->GetSelectedNodes( nodes );
			m_widgetManager->EnableWidgets( nodes.Size() > 0 );
		}
	}
}

Bool CEdBehaviorPreviewPanel::IsSkeleton() 
{ 
	return GetEditor()->IsSkeletonBonesVisible(); 
}

Bool CEdBehaviorPreviewPanel::IsBoneNames() 
{ 
	return GetEditor()->IsSkeletonBoneNamesVisible();  
}

Bool CEdBehaviorPreviewPanel::IsBoneAxis() 
{ 
	return GetEditor()->IsSkeletonBoneAxisVisible(); 
}

void CEdBehaviorPreviewPanel::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
	HandleItemSelection( objects );
}

void CEdBehaviorPreviewPanel::HandleContextMenu( Int32 x, Int32 y )
{
	wxMenu menu;
	menu.Append( ID_MENU_ENTITY_PLAYER, TXT("Use player.w2ent") );
	menu.Append( ID_MENU_ENTITY_INCLUDETEST, TXT("Use man_base.w2ent") );
	menu.AppendSeparator();
	menu.Append( ID_MENU_ENTITY, TXT("Use selected entity") );
	menu.AppendSeparator();

	const TDynArray< String >& entityHistory = GetEditor()->GetEntityHistory();

	if( entityHistory.Size() )
	{
		for( Int32 i=(Int32)entityHistory.Size()-1; i>=0; i-- )
		{
			menu.Append( ID_MENU_HISTORY_FIRST + i, entityHistory[ i ].AsChar() );
			menu.Connect( ID_MENU_HISTORY_FIRST + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorPreviewPanel::OnHistory ), NULL, this ); 
		}

		menu.AppendSeparator();
	}

	menu.Append( ID_MENU_TELEPORT_TO_ZERO, TXT("Reset position") );
	menu.Append( ID_SHOW_TRANS, TXT("Show entity transform"), wxEmptyString, wxITEM_CHECK );
	menu.Append( ID_CACHE_TRANS, TXT("Cache entity transform") );

	menu.AppendSeparator();

	menu.Append( ID_MENU_STICK, TXT("Stick camera to entity"), wxEmptyString, wxITEM_CHECK );	
	menu.Check( ID_MENU_STICK, m_stickToEntity );
	menu.Append( ID_BEH_DRAW_ITEM, TXT("Draw weapon") );
	menu.Append( ID_SAVE_CAMERA, TXT("Save camera offsets for entity") );
	menu.Append( ID_LOAD_CAMERA, TXT("Load camera offsets for entity") );

	PopupMenu( &menu, x, y );
}

void CEdBehaviorPreviewPanel::OnHistory( wxCommandEvent& event )
{
	const TDynArray< String >& entityHistory = GetEditor()->GetEntityHistory();

	Uint32 i = event.GetId() - ID_MENU_HISTORY_FIRST;
	if( i < entityHistory.Size() )
	{
		String entity = entityHistory[ i ];
		String defComp = GetAnimatedComponent() ? GetAnimatedComponent()->GetName() : String::EMPTY;
		GetEditor()->LoadEntity( entity, String::EMPTY, defComp );

		// [DrUiD] fixed view offset after we load a new entity
		GetEditor()->GetGraphEditor()->ZoomExtents();
	}
}

void CEdBehaviorPreviewPanel::OnTeleportToZero( wxCommandEvent& event )
{
	if ( CEntity* e = GetEntity() )
	{
		e->Teleport( Vector::ZERO_3D_POINT, EulerAngles::ZEROS );
		m_cachedEntityTransformPos = Vector::ZERO_3D_POINT;
		m_cachedEntityTransformRot = EulerAngles::ZEROS;
	}
}

void CEdBehaviorPreviewPanel::OnShowTransform( wxCommandEvent& event )
{
	m_showEntityTransform = !m_showEntityTransform;
}

void CEdBehaviorPreviewPanel::OnCacheTransform( wxCommandEvent& event )
{
	if ( CEntity* e = GetEntity() )
	{
		m_cachedEntityTransformPos = e->GetWorldPosition();
		m_cachedEntityTransformRot = e->GetWorldRotation();
	}
}

void CEdBehaviorPreviewPanel::OnLoadEntity( wxCommandEvent& event )
{
	String selectedResource;

	if ( GetActiveResource( selectedResource ) )
	{
		GetEditor()->LoadEntity( selectedResource );
	}

	/*
	CEdFileDialog dlg;
	dlg.SetMultiselection( false );
	dlg.SetDirectory( GDepot->GetAbsolutePath() );
	dlg.AddFormat( ResourceExtension< CEntityTemplate >(), TXT( "Entity files" ) );

	if ( dlg.DoOpen( (HWND)GetHandle() ) )
	{				
		String localDepotPath;
		if ( !GDepot->ConvertToLocalPath( dlg.GetFile(), localDepotPath ) )
		{
			WARN_EDITOR( TXT("Couldn't convert '%s' to local depot path!"), dlg.GetFile().AsChar() );
		}
		else
		{
			LoadEntity( localDepotPath );
		}
	}
	*/
}

void CEdBehaviorPreviewPanel::OnLoadEntityPlayer( wxCommandEvent& event )
{
	GetEditor()->LoadEntity( TXT("characters\\player_entities\\geralt\\geralt_player.w2ent") );

	// [DrUiD] fixed view offset after we load a new entity
	GetEditor()->GetGraphEditor()->ZoomExtents();
}

void CEdBehaviorPreviewPanel::OnLoadEntityIncludeTest( wxCommandEvent& event )
{
	GetEditor()->LoadEntity( TXT("characters\\base_entities\\man_base\\man_base.w2ent") );

	// [DrUiD] fixed view offset after we load a new entity
	GetEditor()->GetGraphEditor()->ZoomExtents();
}

void CEdBehaviorPreviewPanel::OnUnloadEntity()
{
	OnReset();

	if( m_previewCamera )
	{
		m_previewCamera->Reset();
	}
}

void CEdBehaviorPreviewPanel::OnLoadEntity()
{
	RecreateHelpers();
}

void CEdBehaviorPreviewPanel::OnReset()
{
	// Set follow camera
	if ( m_previewCamera && GetEditor()->UseEyeCamera() )
	{
		m_previewCamera->Follow( GetEntity() );
	}

	if ( GetEntity() )
	{
		Vector entPos = GetEntity()->GetPosition() + Vector( 0, 4, 2 );
		SetCameraPosition( entPos );
		SetCameraRotation( EulerAngles( 0, -10, 180 ) );
	}

	RecreateHelpers();

	m_ghostContainer.Reset();
}

void CEdBehaviorPreviewPanel::OnStickToEntity( wxCommandEvent &event )
{
	Vector newCameraPosition = m_cameraPosition;	

	const CAnimatedComponent* animatedComponent = GetAnimatedComponent();

	if ( animatedComponent )
	{
		if ( m_stickToEntity )
		{
			newCameraPosition += animatedComponent->GetPosition();
		}
		else
		{
			newCameraPosition -= animatedComponent->GetPosition();
		}
	}

	m_stickToEntity = !m_stickToEntity;

	m_cameraPosition = newCameraPosition;
}

void CEdBehaviorPreviewPanel::OnSaveCameraOffsets( wxCommandEvent& event )
{
	CEntity* entity = GetEntity();
	if( entity && entity->GetEntityTemplate() && entity->GetEntityTemplate()->GetFile() )
	{
		String entFileName = entity->GetEntityTemplate()->GetFile()->GetDepotPath();

		CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

		String camOffsetPosStr = ToString( GetCameraPosition() );
		String camOffsetRotStr = ToString( GetCameraRotation() );
		String camOffsetFovStr = ToString( GetCameraFov() );

		config.Write( TXT("/Frames/BehaviorEditor/Preview/CameraOffset/Pos") , camOffsetPosStr );
		config.Write( TXT("/Frames/BehaviorEditor/Preview/CameraOffset/Rot") , camOffsetRotStr );
		config.Write( TXT("/Frames/BehaviorEditor/Preview/CameraOffset/Fov") , camOffsetFovStr );
	}
}

void CEdBehaviorPreviewPanel::OnLoadCameraOffsets( wxCommandEvent& event )
{
	CEntity* entity = GetEntity();
	if( entity && entity->GetEntityTemplate() && entity->GetEntityTemplate()->GetFile() )
	{
		String entFileName = entity->GetEntityTemplate()->GetFile()->GetDepotPath();

		CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

		String camOffsetPosStr = String::EMPTY;
		String camOffsetRotStr = String::EMPTY;
		String camOffsetFovStr = String::EMPTY;

		camOffsetPosStr = config.Read( TXT("/Frames/BehaviorEditor/Preview/CameraOffset/Pos"), String::EMPTY );
		camOffsetRotStr = config.Read( TXT("/Frames/BehaviorEditor/Preview/CameraOffset/Rot"), String::EMPTY );
		camOffsetFovStr = config.Read( TXT("/Frames/BehaviorEditor/Preview/CameraOffset/Fov"), String::EMPTY );

		if ( camOffsetPosStr.Empty() == false )
		{
			Vector pos;
			FromString( camOffsetPosStr, pos );

			SetCameraPosition( pos );
		}

		if ( camOffsetRotStr.Empty() == false )
		{
			EulerAngles rot;
			FromString( camOffsetRotStr, rot );

			SetCameraRotation( rot );
		}

		if ( camOffsetFovStr.Empty() == false )
		{
			Float fov;
			FromString( camOffsetFovStr, fov );

			SetCameraFov( fov );
		}
	}
}

void CEdBehaviorPreviewPanel::OnViewportTick( IViewport* view, Float timeDelta )
{
	// Camera activation
	if( m_previewCamera )
	{
		if ( GetEditor()->UseEyeCamera() && !m_previewCamera->IsActive() )
		{
			m_previewCamera->SetActive();
		}
		else if ( !GetEditor()->UseEyeCamera() && m_previewCamera->IsActive() )
		{
			//m_previewCamera->SetActive( false );
		}
	}

	if ( m_showEntityTransform )
	{
		if ( CEntity* e = GetEntity() )
		{
			m_showEntityTransformPrevPos = e->GetWorldPosition();
			m_showEntityTransformPrevRot = e->GetWorldRotation();
		}
	}

	// Tick world
	if ( !GetEditor()->IsPaused() && !GetEditor()->IsDebugMode() )
	{
		GetEditor()->Tick( timeDelta * GetEditor()->GetTimeFactor() );

		CAnimatedComponent* ac = GetAnimatedComponent();
		if ( ac )
		{
			m_ghostContainer.UpdateGhosts( timeDelta, ac );
		}
	}
	else if ( GetEditor()->HasPlayOneFrameFlag() )
	{
		if ( GetEditor()->IsDebugMode() )
		{
			CAnimatedComponent* ac = GetAnimatedComponent();
			if ( ac )
			{
				m_ghostContainer.UpdateGhosts( timeDelta, ac );

				if( ac->GetBehaviorStack() )
				{
					ac->GetBehaviorStack()->Update( ac->GetBehaviorGraphUpdateContext(), 1.f/30.f * GetEditor()->GetTimeFactor() );
					ac->ForceBehaviorPose();
					GetEditor()->PlayOneFrame( false );
				}
			}
		}
		else
		{
			GetEditor()->Tick( 1.f/30.f * GetEditor()->GetTimeFactor() );
			GetEditor()->PlayOneFrame( false );
		}
	}
	else if ( GetEditor()->HasPlayOneFrameBackFlag() )
	{
		// This is small hack for update behavior with minus time delta
		// World don't support tick with time delta < 0 :(

		CAnimatedComponent* ac = GetAnimatedComponent();
		if ( ac && ac->GetBehaviorStack() )
		{
			ac->GetBehaviorStack()->Update( ac->GetBehaviorGraphUpdateContext(), -1.f/30.f * GetEditor()->GetTimeFactor() );
			ac->ForceBehaviorPose();
			GetEditor()->PlayOneFrameBack( false );
		}
	}
	else if ( GetEditor()->RequiresCustomTick() )
	{
		GetEditor()->CustomTick( timeDelta * GetEditor()->GetTimeFactor() );
		CAnimatedComponent* ac = GetAnimatedComponent();
		if ( ac )
		{
			m_ghostContainer.UpdateGhosts( timeDelta, ac );
		}
	}

	// Tick panel
	CEdRenderingPanel::OnViewportTick( view, timeDelta );

	// Update item entities
	SItemEntityManager::GetInstance().OnTick( timeDelta );

	if ( m_dynamicTarget )
	{
		CActor* actor = Cast< CActor >( GetEntity() );
		if ( actor && RIM_IS_KEY_DOWN( IK_Space ) )
		{
			actor->DisableLookAts();

			SLookAtDebugStaticInfo info;
			info.m_target = m_dynamicTarget->GetWorldPosition();
			actor->EnableLookAt( info );
		}
		else if ( actor && actor->IsLookAtEnabled() )
		{
			actor->DisableLookAts();
		}
	}
}

void CEdBehaviorPreviewPanel::EnumInputs( TDynArray< CName >& inputs ) const
{
	if ( m_previewInputManager )
	{
		inputs = m_previewInputManager->GetGameEventNames();
	}
}

void CEdBehaviorPreviewPanel::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	// Scene
	CEdPreviewPanel::OnViewportGenerateFragments( view, frame );

	frame->AddDebugAxisOnScreen( 0.2f, 0.2f, Matrix::IDENTITY, 0.1f );

	// Display skeleton from select animated component - color white
	CAnimatedComponent* mainAc = GetAnimatedComponent();
	if ( mainAc )
	{
		// Display skeletons from all animated components
		const TDynArray< CAnimatedComponent* >& entityAnimComponents = GetEditor()->GetAllAnimatedComponents();
		for ( Uint32 i=0; i<entityAnimComponents.Size(); i++ )
		{
			DisplaySkeleton( frame, entityAnimComponents[i], Color( 128, 128, 128, 128 ) );
		}

		DisplaySkeleton( frame, mainAc );

		m_ghostContainer.Draw( frame, mainAc );
	}

	if ( m_dynamicTarget )
	{
		frame->AddDebugSphere( m_dynamicTarget->GetWorldPosition(), 0.2f, Matrix::IDENTITY, Color::LIGHT_YELLOW, true );
	}

	// 2D editor grid
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_Grid ) )
	{
		// Line buffer
		static TDynArray< DebugVertex > lines;
		lines.ClearFast();

		const Float extents = 20.0f;

		// Color
		const Color gridColor( 0, 0, 80/2 );
		const Color axisColor( 0, 0, 127/2 );

		Vector offset( Vector::ZERO_3D_POINT );
		if ( mainAc )
		{
			const Vector acPos = mainAc->GetWorldPosition();
			
			Int32 mulX = (Int32)( acPos.X / extents );
			Int32 mulY = (Int32)( acPos.Y / extents );

			offset += Vector( mulX*extents, mulY*extents, 0.f );
		}

		// Generate lines
		for ( Float y=-extents; y<=extents; y+=1.0f )
		{
			for ( Float x=-extents; x<=extents; x+=1.0f )
			{
				lines.PushBack( DebugVertex( Vector( x, -extents, 0.0f )+offset, gridColor ) );
				lines.PushBack( DebugVertex( Vector( x, extents, 0.0f )+offset, gridColor ) );
				lines.PushBack( DebugVertex( Vector( -extents, y, 0.0f )+offset, gridColor ) );
				lines.PushBack( DebugVertex( Vector( extents, y, 0.0f )+offset, gridColor ) );
			}
		}

		// Center lines
		lines.PushBack( DebugVertex( Vector( 0.0f, -extents, 0.0f )+offset, axisColor ) );
		lines.PushBack( DebugVertex( Vector( 0.0f, extents, 0.0f )+offset, axisColor ) );
		lines.PushBack( DebugVertex( Vector( -extents, 0.0f, 0.0f )+offset, axisColor ) );
		lines.PushBack( DebugVertex( Vector( extents, 0.0f, 0.0f )+offset, axisColor ) );

		// Draw
		frame->AddDebugLines( &lines[0], lines.Size(), false );
	}

	if ( m_showEntityTransform )
	{
		if ( CEntity* e = GetEntity() )
		{
			const Int32 w = frame->GetFrameOverlayInfo().m_width;
			const Int32 h = frame->GetFrameOverlayInfo().m_height;
			const Int32 x = w*0.1f;
			Int32 y = h-50;
			const Int32 yOffset = 21;

			const Vector pos = e->GetWorldPosition();
			const EulerAngles rot = e->GetWorldRotation();
			const Vector dpos = pos - m_showEntityTransformPrevPos;
			const EulerAngles drot = rot - m_showEntityTransformPrevRot;
			const Vector dpos2 = pos - m_cachedEntityTransformPos;
			const EulerAngles drot2 = rot - m_cachedEntityTransformRot;

			const String strA = String::Printf( TXT("Transform: [%1.4f %1.4f %1.4f] [%1.2f %1.2f %1.2f]"),
				pos.X, pos.Y, pos.Z, rot.Roll, rot.Pitch, rot.Yaw );
			const String strB = String::Printf( TXT("Delta ( curr ): [%1.4f %1.4f %1.4f] [%1.2f %1.2f %1.2f]"),
				dpos.X, dpos.Y, dpos.Z, drot.Roll, drot.Pitch, drot.Yaw );
			const String strC = String::Printf( TXT("Delta (cached): [%1.4f %1.4f %1.4f] [%1.2f %1.2f %1.2f]"),
				dpos2.X, dpos2.Y, dpos2.Z, drot2.Roll, drot2.Pitch, drot2.Yaw );

			frame->AddDebugScreenText( x, y, strA, Color( 255,255,255 ), nullptr, true );
			y += yOffset;
			frame->AddDebugScreenText( x, y, strB, Color( 255,255,255 ), nullptr, true );
			y += yOffset;
			frame->AddDebugScreenText( x, y, strC, Color( 255,255,255 ), nullptr, true );
		}
	}
}

void CEdBehaviorPreviewPanel::OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera )
{
	const CAnimatedComponent* animatedComponent = GetAnimatedComponent();

	// Use camera from entity
	if ( GetEditor()->UseEyeCamera() && m_previewCamera )
	{
		m_previewCamera->GetSelectedCameraComponent()->OnViewportCalculateCamera( view, camera );
		return;
	}
	else if ( GetEditor()->UseMovingCamera() && animatedComponent )
	{
		AlignMovingCamera( animatedComponent->GetEntity()->GetPosition() );
	}

	// Use panel camera
	Vector cameraPosition = m_cameraPosition;
	if ( m_stickToEntity && animatedComponent )
	{
		cameraPosition += animatedComponent->GetPosition();
	}

	if ( m_cameraMode == RPCM_DefaultFlyby )
	{
		camera = CRenderCamera( cameraPosition, m_cameraRotation, GetCameraFov(), view->GetWidth() / (Float)view->GetHeight(), 0.01f, 250.0f );
	}
	else if ( m_cameraMode == RPCM_DefaultOrbiting )
	{
		Vector dir, u, r;
		m_cameraRotation.ToAngleVectors( &dir, &r, &u );
		camera = CRenderCamera( cameraPosition - dir * m_cameraZoom, m_cameraRotation, GetCameraFov(), view->GetWidth() / (Float)view->GetHeight(), 0.01f, 250.0f );
	}
}

Bool CEdBehaviorPreviewPanel::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	if ( m_dynamicTarget )
	{
		Vector move;
		if ( DynamicTargetInput( key, action, data, move ) )
		{
			MoveDynamicTarget( move );
			return true;
		}
	}

	return CEdPreviewPanel::OnViewportInput( view, key, action, data );
}

Bool CEdBehaviorPreviewPanel::OnViewportMouseMove( const CMousePacket& packet )
{
	if ( m_dynamicTarget )
	{
		Vector move;
		if ( DynamicTargetInput( packet, move ) )
		{
			MoveDynamicTarget( move );
			return true;
		}
	}

	return CEdPreviewPanel::OnViewportMouseMove( packet );
}

Bool CEdBehaviorPreviewPanel::OnDropResources( wxCoord x, wxCoord y, TDynArray< CResource* > &resources )
{
	GetEditor()->LoadEntity( resources[ 0 ]->GetFile()->GetDepotPath() );

	return false;
}

void CEdBehaviorPreviewPanel::AlignMovingCamera( const Vector& pos )
{
	if ( m_isMovingCameraAlign )
	{
		m_cameraPosition = m_cameraPosition - m_prevEntityPos + pos;
		m_prevEntityPos = pos;
	}
	else
	{
		m_cameraPosition = pos + Vector( 0,4,2 );
		m_isMovingCameraAlign = true;
	}

	OnCameraMoved();
}

void CEdBehaviorPreviewPanel::CreatePreviewCamera()
{
	// Create preview camera
	ASSERT( m_previewWorld && m_previewWorld->GetDynamicLayer() );

	CEntityTemplate* cameraTemplate = resPreviewCameraTemplate.LoadAndGet< CEntityTemplate >();
	if ( cameraTemplate )
	{
		EntitySpawnInfo einfo;
		einfo.m_detachTemplate = false;
		einfo.m_template = cameraTemplate;

		m_previewCamera = Cast< CCamera >( m_previewWorld->GetDynamicLayer()->CreateEntitySync( einfo ) );
		if ( !m_previewCamera )
		{
			WARN_EDITOR( TXT("CEdBehaviorPreviewPanel: Couldn't create camera") );
		}
	}
	else
	{
		WARN_EDITOR( TXT("CEdBehaviorPreviewPanel: Couldn't create camera - no resource %s"), resPreviewCameraTemplate.GetPath().ToString().AsChar() );
		m_previewCamera = NULL;
	}
}

void CEdBehaviorPreviewPanel::DestroyPreviewCamera()
{
	ASSERT( m_previewWorld && m_previewWorld->GetDynamicLayer() );

	if ( m_previewCamera )
	{
		m_previewCamera->Destroy();

		GetPreviewWorld()->DelayedActions();

		m_previewCamera = NULL;
	}
}

Bool CEdBehaviorPreviewPanel::HasHelper( const String& helperName, Bool forInternalVariable ) const
{
	return GetHelper( helperName, forInternalVariable ) != NULL;
}

void CEdBehaviorPreviewPanel::ToggleHelper( const String& helperName, Bool forInternalVariable )
{
	if (IPreviewItem* item = GetHelper( helperName, forInternalVariable ))
	{
		ClearItem(item);
	}
	else
	{
		CreateHelper(helperName, forInternalVariable);
	}
}

IPreviewItem* CEdBehaviorPreviewPanel::GetHelper( const String& helperName, Bool forInternalVariable ) const
{
	for ( Int32 i=(Int32)m_items.Size()-1; i>=0; --i )
	{
		if (CBehaviorVarItem* behVarItem = static_cast< CBehaviorVarItem* >(m_items[i]))
		{
			if (behVarItem->GetName() == helperName &&
				behVarItem->DoesMatchInternal(forInternalVariable))
			{
				return m_items[i];
			}
		}
	}
	return NULL;
}

void CEdBehaviorPreviewPanel::CreateHelper( const String& helperName, Bool forInternalVariable )
{
	CBehaviorVarItem* item = new CBehaviorVarItem( this, forInternalVariable );
	item->Init( helperName );
	AddItem( item );
}

void CEdBehaviorPreviewPanel::RemoveHelper( const String& helperName, Bool forInternalVariable )
{
	if (IPreviewItem* item = GetHelper( helperName, forInternalVariable ))
	{
		ClearItem( item );
	}
}

void CEdBehaviorPreviewPanel::RecreateHelpers()
{
	RefreshItems();
}

void CEdBehaviorPreviewPanel::CreateDynamicTarget()
{
	ASSERT( GetItemEntityContainer() );

	// Create new component
	m_dynamicTarget = Cast< CPreviewHelperComponent >( GetItemEntityContainer()->CreateComponent( ClassID< CPreviewHelperComponent >(), SComponentSpawnInfo() ) );
	ASSERT( m_dynamicTarget );

	m_dynamicTarget->SetName( TXT("__D_T__") );
	m_dynamicTarget->SetPosition( Vector::ZERO_3D_POINT );
	m_dynamicTarget->SetManualControl( false );
}
void CEdBehaviorPreviewPanel::DestroyDynamicTarget()
{
	if ( m_dynamicTarget )
	{
		m_dynamicTarget->Destroy();
		m_dynamicTarget = NULL;
	}
}

void CEdBehaviorPreviewPanel::MoveDynamicTarget( const Vector& pos )
{
	if ( m_dynamicTarget )
	{
		m_dynamicTarget->SetPosition( m_dynamicTarget->GetPosition() + pos );
	}
}

void CEdBehaviorPreviewPanel::OnDynTarget( Bool flag )
{
	if ( flag )
	{
		if ( !m_dynamicTarget )
		{
			CreateDynamicTarget();
		}
		else
		{
			ASSERT( !m_dynamicTarget );
		}
	}
	else
	{
		if ( m_dynamicTarget )
		{
			DestroyDynamicTarget();
		}
		else
		{
			ASSERT( m_dynamicTarget );
		}
	}
}

Bool CEdBehaviorPreviewPanel::DynamicTargetInput( enum EInputKey key, enum EInputAction action, Float data, Vector& outMovement ) const
{
	Bool ret = false;
	Vector move( Vector::ZERO_3D_POINT );

	static Float factor = 1.f;

	if ( key == IK_MouseWheelUp )
	{
		move.Z = factor * data;
		ret = true;
	}
	else if ( key == IK_MouseWheelDown )
	{
		move.Z = - factor * data;
		ret = true;
	}
	else if ( key == IK_MouseX || key == IK_MouseY )
	{
		return true;
	}

	if ( ret )
	{
		Matrix mat = m_cameraRotation.ToMatrix();
		outMovement = mat.TransformVector( move );
	}
	
	return ret;
}

Bool CEdBehaviorPreviewPanel::DynamicTargetInput( const CMousePacket& packet, Vector& outMovement ) const
{
	CRenderFrameInfo frameInfo( packet.m_viewport );

	Matrix mat = m_cameraRotation.ToMatrix();

	static Float factor = 1.f/100.f;

	if ( RIM_IS_KEY_DOWN( IK_LShift ) )
	{
		Vector move = mat.GetAxisZ();

		move *= factor * (Float)packet.m_dy;

		outMovement = move;
	}
	else
	{
		Vector moveX = mat.GetAxisX();
		Vector moveY = mat.GetAxisY();

		moveX *= factor * (Float)packet.m_dx;
		moveY *= -factor * (Float)packet.m_dy;

		outMovement = moveX + moveY;
	}

	return true;
}

void CEdBehaviorPreviewPanel::ShowGhosts( Uint32 number, PreviewGhostContainer::EGhostType type )
{
	m_ghostContainer.InitGhosts( number, type );
}

void CEdBehaviorPreviewPanel::HideGhosts()
{
	m_ghostContainer.DestroyGhosts();
}

Bool CEdBehaviorPreviewPanel::HasGhosts() const
{
	return m_ghostContainer.HasGhosts();
}
