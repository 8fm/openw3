/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "cameraPreviewTool.h"
#include "toolsPanel.h"
#include "../../common/game/actor.h"
#include "../../common/game/movingAgentComponent.h"
#include "../../common/game/movingPhysicalAgentComponent.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/camera.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/layerGroup.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/layerInfo.h"
#include "../../common/engine/meshComponent.h"

namespace
{
	CGatheredResource resPreviewCameraTemplate( TXT("gameplay\\camera\\camera.w2ent"), 0 );
	CGatheredResource resPreviewEntityTemplate( TXT("engine\\templates\\editor\\camera_preview_entity.w2ent"), 0 );
}

IMPLEMENT_ENGINE_CLASS( CEdCameraPreviewTool );

CEdCameraPreviewTool::CEdCameraPreviewTool()
	: m_rotation( 0.f, 0.f, 0.f )
    , m_cameraRotation( 0.f, 0.f, 0.f)
    , m_world( NULL )
	, m_viewport( NULL )
	, m_previewCamera( NULL )
	, m_previewEntity( NULL )
	, m_movementComponent( NULL )
    , m_meshComponent( NULL )
    , m_panel( NULL )
    , m_defaultFOV( 60.f )
{
}

static void SetupNavigationMesh( CWorld* world )
{
	// Get layers
	if ( !world->GetWorldLayers() )
	{
		WARN_EDITOR( TXT("No root layer group. Unable to extract navigation mesh.") );
		return;
	}

	// Find paths layer
	CLayerInfo* pathLayerInfo = world->GetWorldLayers()->FindLayerCaseless( TXT("paths") );
	if ( !pathLayerInfo )
	{
		WARN_EDITOR( TXT("No paths layer found. Unable to extract navigation mesh.") );
		return;
	}

	// Load the layer
	LayerLoadingContext loadingContext;
	if ( !pathLayerInfo->SyncLoad( loadingContext ) )
	{
		WARN_EDITOR( TXT("Unable to load paths layer. Unable to extract navigation mesh.") );
		return;
	}

	// Get path layer
	CLayer* pathLayer = pathLayerInfo->GetLayer();
	if ( !pathLayer )
	{
		WARN_EDITOR( TXT("Failed to get CLayer from paths layer. Unable to extract navigation mesh.") );
		return;
	}

	// Get the data entity
	CEntity* dataEntity = pathLayer->FindEntity( TXT("PathData") );
	if ( !dataEntity )
	{
		WARN_EDITOR( TXT("No PathData entity found on paths layer. Unable to extract navigation mesh.") );
		return;
	}

	// TODO PAKSAS MERGE - tu sie wywala assert?
}

static CMeshComponent* FindMeshComponent( CEntity* entity )
{
    const TDynArray< CComponent* >& components = entity->GetComponents();

	// Linear search
    for ( Uint32 i=0; i<components.Size(); i++ )
	{
        CMeshComponent* mc = Cast< CMeshComponent >( components[i] );
		if ( mc )
		{
			return mc;
		}
	}

	// Not found
	return NULL;
}

void CEdCameraPreviewTool::GetDefaultAccelerator( Int32 &flags, Int32 &key ) const
{
    flags = wxACCEL_CTRL | wxACCEL_SHIFT;
    key = 'C';
}

Bool CEdCameraPreviewTool::Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection )
{
	m_world = world;
	m_viewport = viewport;
	ASSERT( m_world && m_viewport );
	if ( !m_world || !m_viewport )
	{
		return false;
	}

	// Create virtual pivot entity
	CLayer* dynamicLayer = world->GetDynamicLayer();
	ASSERT( dynamicLayer != NULL );
	if ( !dynamicLayer )
	{
		return false;
	}

	// Setup spawn info
	EntitySpawnInfo info;
	info.m_template = resPreviewEntityTemplate.LoadAndGet< CEntityTemplate >();
	
	CEntityTemplate* cameraTemplate = resPreviewCameraTemplate.LoadAndGet< CEntityTemplate >();
	if ( cameraTemplate )
	{
		EntitySpawnInfo einfo;
		einfo.m_detachTemplate = false;
		einfo.m_template = cameraTemplate;

		// Create pivot entity
		m_previewEntity = dynamicLayer->CreateEntitySync( info );
		ASSERT( m_previewEntity );
		if ( !m_previewEntity )
		{
			return false;
		}

		m_previewCamera = Cast< CCamera >( m_world->GetDynamicLayer()->CreateEntitySync( einfo ) );
		ASSERT( m_previewCamera );
		if ( !m_previewCamera )
		{
			return false;
		}

		CActor* previewActor = Cast<CActor>( m_previewEntity ); 
        ASSERT( previewActor );
		if ( !previewActor )
		{
			return false;
		}

        m_meshComponent = FindMeshComponent( previewActor );
        ASSERT( m_meshComponent );
		if ( !m_meshComponent )
		{
			return false;
		}

		m_movementComponent = previewActor->FindComponent<CMovingPhysicalAgentComponent>();
		ASSERT(m_movementComponent);
		if ( !m_movementComponent )
		{
			return false;
		}


		// setup
		SetupNavigationMesh( world );
		world->DelayedActions();

		m_previewCamera->SetPosition( viewport->GetCameraPosition() );
		EulerAngles angle( 0.f, 0.f, viewport->GetCameraRotation().Yaw );

		m_movementComponent->SetPhysicalRepresentationRequest( CMovingAgentComponent::Req_On, CMovingAgentComponent::LS_Default );
		Vector position( viewport->GetCameraPosition() );
		m_movementComponent->SnapToMesh( position );
		m_movementComponent->TeleportTo( position, angle );

		m_previewCamera->Follow( m_previewEntity );
        m_previewCamera->SetRotation( angle );

        m_defaultFOV = m_previewCamera->GetFov();

        viewport->GetViewport()->AdjustSizeWithCachets( viewport->GetViewportCachetAspectRatio() );

		//FIXME<<<
        //viewport->GetRenderingWindow()->GetViewport()->CaptureInput( ICM_Full );

        m_panel = new CEdCameraPreviewToolPanel( this, panel, world );

		panelSizer->Add( m_panel, 1, wxEXPAND, 5 );
		panel->Layout();

        return true;
	}
    return false;
}

void CEdCameraPreviewTool::End()
{
	// Destroy pivot entity
	if ( ! m_previewEntity->IsDestroyed() )
	{
		m_previewEntity->Destroy();
	}
	
	if ( ! m_previewCamera->IsDestroyed() )
	{
		m_previewCamera->Destroy();
	}

    m_viewport->GetViewport()->RestoreSize();

	//FIXME<<<
    //m_viewport->GetRenderingWindow()->GetViewport()->CaptureInput( ICM_Background );

    m_world = NULL;
    m_viewport = NULL;
    m_previewEntity = NULL;
}

Bool CEdCameraPreviewTool::OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera )
{
	ASSERT( m_previewCamera );

	// Use camera from entity
	if ( m_previewCamera )
	{
		m_previewCamera->GetSelectedCameraComponent()->OnViewportCalculateCamera( view, camera );
		return true;
	}	

	return true;
}

Bool CEdCameraPreviewTool::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	if ( key == IK_W || key == IK_Up ) m_moveKeys[ RPMK_Forward ] = ( action == IACT_Press );
	if ( key == IK_S || key == IK_Down ) m_moveKeys[ RPMK_Back ] = ( action == IACT_Press );
	if ( key == IK_A || key == IK_Left ) m_moveKeys[ RPMK_StrafeLeft ] = ( action == IACT_Press );
	if ( key == IK_D || key == IK_Right ) m_moveKeys[ RPMK_StrafeRight ] = ( action == IACT_Press );
	if ( key == IK_Q ) m_moveKeys[ RPMK_Up ] = ( action == IACT_Press );
	if ( key == IK_E ) m_moveKeys[ RPMK_Down ] = ( action == IACT_Press );

    if ( key == IK_MouseX ) 
    {
        m_rotation.Yaw -= 0.25f * data;
    }

    if ( key == IK_MouseY )
    {
        m_cameraRotation.Pitch = 0.25f * data;
    }

    if ( key == IK_LeftMouse && action == IACT_Release )
	{
		//FIXME<<<
// 		if ( SRawInputManager::GetInstance().GetInputCaptureMode() != ICM_Full )
//         {
//             m_viewport->GetViewport()->CaptureInput( ICM_Full );
//         }
    }

    if ( ( key == IK_RightMouse || key == IK_Pause ) && action == IACT_Release )
	{
		//FIXME<<<
		//m_viewport->GetViewport()->CaptureInput( ICM_Background );
    }

    if ( key == IK_F10 && action == IACT_Release )
    {
        wxTheFrame->GetToolsPanel()->CancelTool();
    }

	return true;
}

Bool CEdCameraPreviewTool::OnViewportTick( IViewport* view, Float timeDelta )
{
	//FIXME<<<
	//if ( SRawInputManager::GetInstance().GetInputCaptureMode() == ICM_Full )
    {
	    // calculate input direction vector
	    Vector direction(
		    m_moveKeys[ RPMK_StrafeRight ] - m_moveKeys[ RPMK_StrafeLeft ],
		    m_moveKeys[ RPMK_Forward ] - m_moveKeys[ RPMK_Back],
		    m_moveKeys[ RPMK_Up ] - m_moveKeys[ RPMK_Down ] );

		if ( m_movementComponent )
		{
			const Matrix& localToWorld = m_movementComponent->GetLocalToWorld();
			direction = localToWorld.TransformVector( direction );
			m_movementComponent->AddDeltaMovement( direction * timeDelta * 10.f, m_rotation );
		}
		if ( m_previewCamera )
		{
			m_previewCamera->Rotate( -m_rotation.Yaw, m_cameraRotation.Pitch );
		}
    }

    m_rotation = EulerAngles::ZEROS;
    m_cameraRotation = EulerAngles::ZEROS;

	return true;
}

wxIMPLEMENT_CLASS( CEdCameraPreviewToolPanel, CEdDraggablePanel );

BEGIN_EVENT_TABLE( CEdCameraPreviewToolPanel, CEdDraggablePanel )
	EVT_BUTTON( XRCID("buttonReset"), CEdCameraPreviewToolPanel::OnResetFOV )
    EVT_SPINCTRL( XRCID("spinFOV"), CEdCameraPreviewToolPanel::OnChangeFOV )
    EVT_CHECKBOX( XRCID("checkNavMesh"), CEdCameraPreviewToolPanel::OnCheckNavMesh )
    EVT_CHECKBOX( XRCID("checkMesh"), CEdCameraPreviewToolPanel::OnCheckMesh )
END_EVENT_TABLE()

CEdCameraPreviewToolPanel::CEdCameraPreviewToolPanel( CEdCameraPreviewTool* tool, wxWindow* parent, CWorld* world )
    : m_tool( tool )
	, m_world( world )
{
    wxXmlResource::Get()->LoadPanel( this, parent, wxT("CameraPreviewTool") );

    m_spinCtrl = XRCCTRL( *this, "spinFOV", wxSpinCtrl );
    m_spinCtrl->SetValue( tool->m_defaultFOV );

	m_detachablePanel.Initialize( this, TXT( "Camera Preview" ) );
}

void CEdCameraPreviewToolPanel::OnResetFOV( wxCommandEvent &event )
{
    m_spinCtrl->SetValue( m_tool->m_defaultFOV );
    m_tool->m_previewCamera->SetFov( m_spinCtrl->GetValue() );
}

void CEdCameraPreviewToolPanel::OnChangeFOV( wxSpinEvent &event )
{
    m_tool->m_previewCamera->SetFov( static_cast<float>( event.GetValue() ) );
}

void CEdCameraPreviewToolPanel::OnCheckNavMesh( wxCommandEvent &event )
{
    m_tool->m_movementComponent->SetPhysicalRepresentationRequest( event.IsChecked() ? CMovingAgentComponent::Req_Off : CMovingAgentComponent::Req_On, CMovingAgentComponent::LS_Default );
}

void CEdCameraPreviewToolPanel::OnCheckMesh( wxCommandEvent &event )
{
    m_tool->m_meshComponent->SetVisible( event.IsChecked() );
}
