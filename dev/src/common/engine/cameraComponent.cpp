/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "cameraComponent.h"
#include "../../common/core/gatheredResource.h"
#include "viewport.h"
#include "game.h"
#include "renderFrame.h"
#include "world.h"
#include "bitmapTexture.h"


IMPLEMENT_ENGINE_CLASS( CCameraComponent );
IMPLEMENT_RTTI_ENUM( ENearPlaneDistance );
IMPLEMENT_RTTI_ENUM( EFarPlaneDistance );
IMPLEMENT_ENGINE_CLASS( SCustomClippingPlanes );

CGatheredResource resCameraIcon( TXT("engine\\textures\\icons\\cameraicon.xbm"), RGF_NotCooked );

CCameraComponent::CCameraComponent()
	: m_fov( 60.0f )
	, m_nearPlane( NP_DefaultEnv )
	, m_farPlane( FP_DefaultEnv )
	, m_aspect( 1280.0f / 720.0f )
	, m_lockAspect( true )
	, m_defaultCamera( false )
	, m_teleported( true )
{
}

CBitmapTexture* CCameraComponent::GetSpriteIcon() const
{
	return resCameraIcon.LoadAndGet< CBitmapTexture >();
}

void CCameraComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Sprites );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_EnvProbesInstances );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Collision );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Behavior );
}

void CCameraComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Sprites );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_EnvProbesInstances );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Collision );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Behavior );
}

void CCameraComponent::SetActive( Bool state )
{
	if ( state )
	{
		GGame->GetActiveWorld()->GetCameraDirector()->ActivateCamera( this, this );

		m_lastFrameCamera = SRenderCameraLastFrameData::INVALID;
	}
	else
	{
		// Ignore - no more camera deactivating!!!
	}
}

void CCameraComponent::OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera )
{
	CCameraDirector::CalcCamera( camera, view->GetWidth(), view->GetHeight(), GetWorldPosition(), GetWorldRotation(), m_fov, m_aspect, Map( m_nearPlane ), Map( m_farPlane ), m_nearPlane != NP_DefaultEnv, m_farPlane != FP_DefaultEnv );

	camera.SetLastFrameData( m_lastFrameCamera );

	m_lastFrameCamera.Init( GGame->GetEngineTime(), camera );
}

void CCameraComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	// Base fragments
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	// Do not draw this shit in active game
	if ( GGame && flag == SHOW_Camera )
	{
		// Lines
		//if ( frame->GetFrameInfo().m_renderingMode != RM_HitProxies )
		{
			// Calculate camera parameters
			CRenderCamera camera;
			camera.Set( GetWorldPosition(), GetWorldRotation(), m_fov, m_aspect, Map( m_nearPlane ) * 0.99f, Map( m_farPlane ), 0.99f );
			if ( m_nearPlane != NP_DefaultEnv )
			{
				camera.SetNonDefaultNearRenderingPlane();
			}
			if ( m_farPlane != FP_DefaultEnv )
			{
				camera.SetNonDefaultFarRenderingPlane();
			}

			// Preview frustum
			frame->AddDebugFrustum( camera.GetScreenToWorld(), Color::BLUE, true, false, IsSelected() ? 1.0f : 0.3f );
		}
	}
}

void CCameraComponent::ViewCoordsToWorldVector( Int32 x, Int32 y, Vector & outRayStart, Vector & outRayDirection ) const
{
	Int32 viewWidth = GGame->GetViewport()->GetWidth();
	Int32 viewHeight = GGame->GetViewport()->GetHeight();

	outRayStart = GetWorldPosition();

	CRenderCamera camera;
	CCameraDirector::CalcCamera( camera, viewWidth, viewHeight, GetWorldPosition(), GetWorldRotation(), m_fov, m_aspect, Map( m_nearPlane ), Map( m_farPlane ), m_nearPlane != NP_DefaultEnv, m_farPlane != FP_DefaultEnv );

	const Float halfWidth  = viewWidth * 0.5f;
	const Float halfHeight = viewHeight * 0.5f;

	// Calculate screen space point
	Vector screenSpacePoint, endPoint;
	screenSpacePoint.X = ( x / halfWidth ) - 1.0f;
	screenSpacePoint.Y = 1.0f - ( y / halfHeight );
	screenSpacePoint.Z = 1.0f;
	screenSpacePoint.W = 1.0f;

	endPoint = camera.GetScreenToWorld().TransformVectorWithW( screenSpacePoint );
	endPoint.Div4( endPoint.W );

	// Use camera origin as ray origin
	outRayDirection = ( endPoint - outRayStart ).Normalized3();
	outRayDirection.W = 0.f;
}

void CCameraComponent::WorldVectorToViewCoords( Vector & worldPos, Int32& x, Int32& y ) const
{
	Int32 viewWidth = GGame->GetViewport()->GetWidth();
	Int32 viewHeight = GGame->GetViewport()->GetHeight();

	CRenderCamera camera;
	CCameraDirector::CalcCamera( camera, viewWidth, viewHeight, GetWorldPosition(), GetWorldRotation(), m_fov, m_aspect, Map( m_nearPlane ), Map( m_farPlane ), m_nearPlane != NP_DefaultEnv, m_farPlane != FP_DefaultEnv );

	Vector screenSpacePoint = camera.GetWorldToScreen().TransformVectorWithW( worldPos );

	if ( screenSpacePoint.W < 0.001f )
	{
		x = -1;
		y = -1;
		return;
	}

	screenSpacePoint /= screenSpacePoint.W;

	if ( screenSpacePoint.X < -1.f || screenSpacePoint.X > 1.f ||
		 screenSpacePoint.Y < -1.f || screenSpacePoint.Y > 1.f ||
		 screenSpacePoint.Z < 0.f )
	{
		x = -1;
		y = -1;
		return;
	}
	
	// Calculate out point
	x = (Int32)( (  screenSpacePoint.X + 1.0f ) * viewWidth  * 0.5f );
	y = (Int32)( ( -screenSpacePoint.Y + 1.0f ) * viewHeight * 0.5f );
}

Bool CCameraComponent::WorldVectorToViewRatio( const Vector& worldPos, Float& x, Float& y ) const
{
	const Int32 viewWidth = GGame->GetViewport()->GetWidth();
	const Int32 viewHeight = GGame->GetViewport()->GetHeight();

	CRenderCamera camera;
	CCameraDirector::CalcCamera( camera, viewWidth, viewHeight, GetWorldPosition(), GetWorldRotation(), m_fov, m_aspect, Map( m_nearPlane ), Map( m_farPlane ), m_nearPlane != NP_DefaultEnv, m_farPlane != FP_DefaultEnv );

	Vector screenSpacePoint = camera.GetWorldToScreen().TransformVectorWithW( worldPos );

	if ( screenSpacePoint.W < 0.001f )
	{
		return false;
	}

	screenSpacePoint /= screenSpacePoint.W;

	if ( screenSpacePoint.Z < 0.f )
	{
		return false;
	}

	x = screenSpacePoint.X;
	y = -screenSpacePoint.Y;

	return true;
}

Bool CCameraComponent::TestWorldVectorToViewRatio( const Vector& worldPos, Float& x, Float& y, const Vector& cameraPos, const EulerAngles& cameraRot ) const
{
	const Int32 viewWidth = GGame->GetViewport()->GetWidth();
	const Int32 viewHeight = GGame->GetViewport()->GetHeight();

	CRenderCamera camera;
	CCameraDirector::CalcCamera( camera, viewWidth, viewHeight, cameraPos, cameraRot, m_fov, m_aspect, Map( m_nearPlane ), Map( m_farPlane ), m_nearPlane != NP_DefaultEnv, m_farPlane != FP_DefaultEnv );

	Vector screenSpacePoint = camera.GetWorldToScreen().TransformVectorWithW( worldPos );

	if ( screenSpacePoint.W < 0.001f )
	{
		return false;
	}

	screenSpacePoint /= screenSpacePoint.W;

	if ( screenSpacePoint.Z < 0.f )
	{
		return false;
	}

	x = screenSpacePoint.X;
	y = -screenSpacePoint.Y;

	return true;
}

Bool CCameraComponent::IsPointInView( const Vector& point, Float fowMultipier /* = 1.0f */) const
{
	PC_SCOPE_PIX( IsPointInView );

	Matrix worldToLocal;
	GetWorldToLocal( worldToLocal );
	Vector pointLocal = worldToLocal.TransformPoint( point );

	// Skip tests for points behind the camera
	if ( pointLocal.Y <= 0.f )
	{
		return false;
	}
	
	Float tanHalfFov = MTan( DEG2RAD( m_fov * fowMultipier ) * 0.5f );
	Float yInv = 1.0f / pointLocal.Y;
	
	// Horizontal FOV test
	Float tanHorzAngle = MAbs( pointLocal.X ) * yInv;
	// Aspect ratio must be applied in the tangents space!
	if ( tanHorzAngle > ( tanHalfFov * GGame->GetViewport()->GetWidth() ) / GGame->GetViewport()->GetHeight() )
	{
		return false;
	}

	// Vertical FOV test
	Float tanVertAngle = MAbs( pointLocal.Z ) * yInv;
	if ( tanVertAngle > tanHalfFov )
	{
		return false;
	}

	return true;
}



Vector CCameraComponent::ProjectPoint( const Vector& worldSpacePoint )
{
	CRenderCamera renderCamera( GetWorldPosition(), GetWorldRotation(), m_fov, m_aspect, Map( m_nearPlane ), Map( m_farPlane ) );

	Vector screenSpacePoint = renderCamera.GetWorldToScreen().TransformVectorWithW( worldSpacePoint );

	// Behind near plane
	if ( screenSpacePoint.W < 0.001f )
	{
		// Clamp to one of the screen corners
		screenSpacePoint.X = ( screenSpacePoint.X < 0.0f) ? -1.0f : 1.0f;
		screenSpacePoint.Y = ( screenSpacePoint.Y < 0.0f) ? -1.0f : 1.0f;
		screenSpacePoint.Z = 0.0f;
		screenSpacePoint.W = 0.0f;
	}
	else
	{
		// Vertex inside screen space, project
		screenSpacePoint.Div4( screenSpacePoint.W );
	}

	return screenSpacePoint;
}

Vector CCameraComponent::UnprojectPoint( const Vector& screenSpacePoint )
{
	CRenderCamera renderCamera( GetWorldPosition(), GetWorldRotation(), m_fov, m_aspect, Map( m_nearPlane ), Map( m_farPlane ) );

	Vector unprojected = renderCamera.GetScreenToWorld().TransformVectorWithW( screenSpacePoint );
	unprojected /= unprojected.W;

	return unprojected;
}

Bool CCameraComponent::GetData( Data& outData ) const
{
	outData.m_nearPlane = Map( m_nearPlane );
	outData.m_farPlane = Map( m_farPlane );
	outData.m_forceNearPlane = m_nearPlane != NP_DefaultEnv;
	outData.m_forceFarPlane = m_farPlane != FP_DefaultEnv;
	outData.m_fov = m_fov;
	outData.m_position = GetWorldPosition();
	outData.m_rotation = GetWorldRotation();
	outData.m_dofParams = m_dofParam;
	outData.m_bokehDofParams = m_bokehDofParam;

	return true;
}
