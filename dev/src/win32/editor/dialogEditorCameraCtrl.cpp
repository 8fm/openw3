
#include "build.h"
#include "dialogEditorCameraCtrl.h"
#include "../../common/engine/viewport.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

CEdSceneCameraCtrl
	::CEdSceneCameraCtrl()
	: m_selectedCameraDefinition( NULL )
	, m_mode( SPCM_PREVIEW )
	, m_previewViewportCamera( NULL )
	, m_gameViewportCamera( NULL )
	, m_activeViewportCamera( NULL )
{
	m_freeCameraDefinition.m_cameraName = CName(TXT("free camera"));
	m_freeCameraDefinition.m_cameraFov  = 20.f ;
}

void CEdSceneCameraCtrl::Init( CEdRenderingPanel* vc )
{
	ASSERT( !m_previewViewportCamera );
	ASSERT( !m_gameViewportCamera );
	ASSERT( !m_activeViewportCamera );

	m_previewViewportCamera = vc;
	m_gameViewportCamera = wxTheFrame->GetWorldEditPanel();
	m_activeViewportCamera = m_previewViewportCamera;
}

void CEdSceneCameraCtrl::SetFreeMode()
{
	if ( m_mode == SPCM_PREVIEW )
	{
		SyncFromPreviewCamera();
	}

	m_mode = SPCM_FREE_CAM;

	m_selectedCameraDefinition = &m_freeCameraDefinition;
}

void CEdSceneCameraCtrl::SetEditMode()
{
	if ( m_mode == SPCM_PREVIEW )
	{
		SyncFromPreviewCamera();
	}

	m_mode = SPCM_EDIT;

	m_selectedCameraDefinition = NULL;
}

void CEdSceneCameraCtrl::SetPreviewMode()
{
	m_mode = SPCM_PREVIEW;

	m_selectedCameraDefinition = NULL;
}

void CEdSceneCameraCtrl::SetGameplayMode()
{
	SyncFromGameplayCamera();

	m_mode = SPCM_GAMEPLAY;

	m_selectedCameraDefinition = &m_freeCameraDefinition;
}

void CEdSceneCameraCtrl::ResetGameplayMode()
{
	m_mode = SPCM_PREVIEW;

	SetPreviewMode();
}

Bool CEdSceneCameraCtrl::IsPreviewMode() const
{
	return m_mode == SPCM_PREVIEW;
}

Bool CEdSceneCameraCtrl::IsEditMode() const
{
	return m_mode == SPCM_EDIT;
}

Bool CEdSceneCameraCtrl::IsFreeMode() const
{
	return m_mode == SPCM_FREE_CAM;
}

Bool CEdSceneCameraCtrl::IsGameplayMode() const
{
	return m_mode == SPCM_GAMEPLAY;
}

void CEdSceneCameraCtrl::SetCameraPreviewMode()
{
	ASSERT( m_activeViewportCamera == m_gameViewportCamera );
	m_activeViewportCamera = m_previewViewportCamera;
}

void CEdSceneCameraCtrl::SetCameraGameMode()
{
	ASSERT( m_activeViewportCamera == m_previewViewportCamera );
	m_activeViewportCamera = m_gameViewportCamera;
}

const StorySceneCameraDefinition* CEdSceneCameraCtrl::GetSelectedDefinition() const
{
	ASSERT( m_mode != SPCM_PREVIEW );
	return m_selectedCameraDefinition;
}

StorySceneCameraDefinition* CEdSceneCameraCtrl::GetSelectedDefinition()
{
	ASSERT( m_mode != SPCM_PREVIEW );
	return m_selectedCameraDefinition ;
}

void CEdSceneCameraCtrl::SelectDefinition( StorySceneCameraDefinition* cameraDefinition, CStorySceneEvent* cameraDefOwner )
{
	ASSERT( m_mode != SPCM_PREVIEW );
	if( m_mode == SPCM_FREE_CAM )
	{
		CName tempName = m_freeCameraDefinition.m_cameraName;
		m_freeCameraDefinition = *cameraDefinition;
		m_freeCameraDefinition.m_cameraName = tempName;
	}
	else if( m_mode == SPCM_EDIT )
	{
		m_selectedCameraDefinition = cameraDefinition;
	}
	m_cameraDefOwner = cameraDefOwner;
}

void CEdSceneCameraCtrl::Update( const StorySceneCameraState& state )
{
	m_sceneCamera = state;
}

void CEdSceneCameraCtrl::SyncFromPreviewCamera()
{
	m_activeViewportCamera->SetCameraFov( m_sceneCamera.m_fov );
	m_activeViewportCamera->SetCameraPosition( m_sceneCamera.m_position );
	m_activeViewportCamera->SetCameraRotation( m_sceneCamera.m_rotation );
}

void CEdSceneCameraCtrl::SyncFromGameplayCamera()
{
	const ICamera::Data& camData = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraData();

	m_activeViewportCamera->SetCameraFov( camData.m_fov );
	m_activeViewportCamera->SetCameraPosition( camData.m_position );
	m_activeViewportCamera->SetCameraRotation( camData.m_rotation );
}

void CEdSceneCameraCtrl::ReadFrom( const StorySceneCameraDefinition* cameraDefinition, const Matrix& cameraWs )
{
	ASSERT( cameraDefinition );
	ASSERT( m_mode == SPCM_EDIT || m_mode == SPCM_FREE_CAM || m_mode == SPCM_GAMEPLAY );

	if ( !cameraDefinition )
	{
		return;
	}

	if ( m_mode == SPCM_EDIT || m_mode == SPCM_FREE_CAM || m_mode == SPCM_GAMEPLAY )
	{
		m_activeViewportCamera->SetCameraFov( cameraDefinition->m_cameraFov );
		m_activeViewportCamera->SetCameraZoom( cameraDefinition->m_cameraZoom );

		m_activeViewportCamera->SetCameraPosition( cameraWs.GetTranslation() );
		m_activeViewportCamera->SetCameraRotation( cameraWs.ToEulerAngles() );
	}
}

void CEdSceneCameraCtrl::WriteTo( StorySceneCameraDefinition* cameraDefinition, const Matrix& sceneL2W )
{
	Matrix cameraWs;

	if( m_mode == SPCM_EDIT || m_mode == SPCM_FREE_CAM || m_mode == SPCM_GAMEPLAY )
	{
		cameraWs = m_activeViewportCamera->GetCameraRotation().ToMatrix();
		cameraWs.SetTranslation( m_activeViewportCamera->GetCameraPosition() );
		
		cameraDefinition->m_cameraFov = m_activeViewportCamera->GetCameraFov();
		cameraDefinition->m_cameraZoom = m_activeViewportCamera->GetCameraZoom();

		const StorySceneCameraDefinition* srcCamDef = m_selectedCameraDefinition;
		if( !srcCamDef )
		{
			srcCamDef = &m_freeCameraDefinition;
		}

		cameraDefinition->m_dofBlurDistFar = srcCamDef->m_dofBlurDistFar;
		cameraDefinition->m_dofBlurDistNear = srcCamDef->m_dofBlurDistNear;
		cameraDefinition->m_dofFocusDistFar = srcCamDef->m_dofFocusDistFar;
		cameraDefinition->m_dofFocusDistNear = srcCamDef->m_dofFocusDistNear;
		cameraDefinition->m_dofIntensity = srcCamDef->m_dofIntensity;
		cameraDefinition->m_dof = srcCamDef->m_dof;

	}
	else if ( m_mode == SPCM_PREVIEW )
	{
		cameraWs = m_sceneCamera.m_rotation.ToMatrix();
		cameraWs.SetTranslation( m_sceneCamera.m_position );

		cameraDefinition->m_cameraFov = m_sceneCamera.m_fov;
		cameraDefinition->m_cameraZoom = 0.f;

		cameraDefinition->m_dofBlurDistFar		= m_sceneCamera.m_dof.dofBlurDistFar;
		cameraDefinition->m_dofBlurDistNear		= m_sceneCamera.m_dof.dofBlurDistNear;
		cameraDefinition->m_dofFocusDistFar		= m_sceneCamera.m_dof.dofFocusDistFar;
		cameraDefinition->m_dofFocusDistNear	= m_sceneCamera.m_dof.dofFocusDistNear;
		cameraDefinition->m_dofIntensity		= m_sceneCamera.m_dof.dofIntensity;
	}
	else
	{
		ASSERT( 0 );
	}

	Matrix cameraTs( Matrix::Mul( sceneL2W.FullInverted(), cameraWs ) );
	cameraDefinition->m_cameraTransform.SetPosition( cameraTs.GetTranslation() );
	cameraDefinition->m_cameraTransform.SetRotation( cameraTs.ToEulerAngles() );	
}

Bool CEdSceneCameraCtrl::CalculateCamera() const
{
	return m_mode != SPCM_PREVIEW;
}

Bool CEdSceneCameraCtrl::CanMoveViewportCamera() const
{ 
	return m_mode == SPCM_FREE_CAM || m_mode == SPCM_EDIT || m_mode == SPCM_GAMEPLAY; 
}

void CEdSceneCameraCtrl::OnWorldViewportInput( enum EInputKey key, Float data )
{
	if ( key == IK_MouseZ && RIM_IS_KEY_DOWN( IK_Shift ) )
	{
		Float currentFov = m_activeViewportCamera->GetCameraFov();

		if( data > 0.0f )
		{
			currentFov -= 2.5f;
		}
		else if( data < 0.0f )
		{
			currentFov += 2.5f;
		}

		currentFov = Clamp( currentFov, 1.0f, 180.0f );
		m_activeViewportCamera->SetCameraFov( currentFov );
	}
};

Bool CEdSceneCameraCtrl::GetCameraData( Matrix& camera, Float& fov, Uint32& w, Uint32& h ) const
{
	if ( !m_activeViewportCamera )
	{
		return false;
	}

	if ( IsEditMode() || IsFreeMode() || IsGameplayMode() )
	{
		camera = m_activeViewportCamera->GetCameraRotation().ToMatrix();
		camera.SetTranslation( m_activeViewportCamera->GetCameraPosition() );
		fov = m_activeViewportCamera->GetCameraFov();
	}
	else
	{
		SCENE_ASSERT( IsPreviewMode() );

		camera = m_sceneCamera.m_rotation.ToMatrix();
		camera.SetTranslation( m_sceneCamera.m_position );
		fov = m_sceneCamera.m_fov;
	}

	w = m_activeViewportCamera->GetViewport()->GetWidth();
	h = m_activeViewportCamera->GetViewport()->GetHeight();

	return true;
}

Bool CEdSceneCameraCtrl::IsPointOnScreen( const Vector& _pointWS, Vector& screenBestPosition ) const
{
	Matrix camera( Matrix::IDENTITY );
	Float fov( 60.f );
	Uint32 vWidth( 100 );
	Uint32 vHeight( 100 );

	GetCameraData( camera, fov, vWidth, vHeight );

	SCENE_ASSERT( _pointWS.W == 1.f );
	Vector pointWS( _pointWS );
	pointWS.W = 1.f;
	Bool ret = CCameraDirector::IsPointInView( pointWS, camera, fov, vWidth, vHeight );
	if ( !ret )
	{
		Int32 pointSSX = vWidth/2;
		Int32 pointSSY = vHeight/2;

		Float ssX = 0.f;
		Float ssY = 0.f;
		if ( CCameraDirector::WorldVectorToViewCoords( pointWS, camera.GetTranslation(), camera.ToEulerAngles(), fov, vWidth, vHeight, 0.1f, 1000.f, ssX, ssY ) ) // TODO do not calc this twice
		{
			const Float thr = 0.9f;

			if ( ssX < -thr )
			{
				pointSSX -= vWidth/4;
			}
			else if ( ssX > thr )
			{
				pointSSX += vWidth/4;
			}
			else
			{
				pointSSX = ( ssX + 1.f ) * vWidth * 0.5f;
			}

			if ( ssY < -thr )
			{
				pointSSY -= vHeight/4;
			}
			else if ( ssY > thr )
			{
				pointSSY += vHeight/4;
			}
			else
			{
				pointSSY = ( ssY + 1.f ) * vHeight * 0.5f;
			}
		}

		Vector rayStart( Vector::ZERO_3D_POINT );
		Vector rayDirection( Vector::EY );
		CCameraDirector::ViewCoordsToWorldVector( camera.GetTranslation(), camera.ToEulerAngles(), fov, pointSSX, pointSSY, vWidth, vHeight, 0.1f, 1000.f, rayStart, rayDirection );

		screenBestPosition = rayStart + rayDirection * 3.f;
	}
	else
	{
		screenBestPosition = Vector::ZERO_3D_POINT;
	}

	return ret;
}

Bool CEdSceneCameraCtrl::CalcCameraRay( Vector& rayStart, Vector& rayDirection ) const
{
	Matrix camera( Matrix::IDENTITY );
	Float fov( 60.f );
	Uint32 vWidth( 100 );
	Uint32 vHeight( 100 );

	if ( GetCameraData( camera, fov, vWidth, vHeight ) )
	{
		CCameraDirector::ViewCoordsToWorldVector( camera.GetTranslation(), camera.ToEulerAngles(), fov, vWidth/2, vHeight/2, vWidth, vHeight, 0.1f, 1000.f, rayStart, rayDirection );
		return true;
	}

	return false;
}

void CEdSceneCameraCtrl::SetCameraFromNet( const Vector& pos, const EulerAngles& rot, Float fov )
{
	if ( IsEditMode() || IsFreeMode() )
	{
		m_activeViewportCamera->SetCameraFov( fov );
		m_activeViewportCamera->SetCameraPosition( pos );
		m_activeViewportCamera->SetCameraRotation( rot );
	}
}

void CEdSceneCameraCtrl::GetCameraLocalToWorld( Matrix& out )
{
	Float fov( 0.f );
	Uint32 vWidth( 0 );
	Uint32 vHeight( 0 );
	GetCameraData( out, fov, vWidth, vHeight );
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
