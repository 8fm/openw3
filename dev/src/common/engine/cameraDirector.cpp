
#include "build.h"
#include "cameraDirector.h"
#include "../core/scriptStackFrame.h"
#include "viewport.h"
#include "game.h"
#include "node.h"
#include "mesh.h"
#include "renderFrame.h"

RED_DEFINE_STATIC_NAME( OnFocusedCameraBlendBegin );
RED_DEFINE_STATIC_NAME( OnFocusedCameraBlendUpdate );
RED_DEFINE_STATIC_NAME( OnFocusedCameraBlendEnd );

#ifdef DEBUG_CAMERA_DIRECTOR
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

void CameraDirectorAssertFunc( const Char* msgFile, const Uint32 lineNum, const Char* msg )
{
	RED_LOG_ERROR( TXT("CAMERA ASSERT: file: %ls, line: %d, exp: %ls"), msgFile, lineNum, msg );
	RED_ASSERT( TXT("CAMERA ASSERT: file: %ls, line: %d, exp: %ls"), msgFile, lineNum, msg );
	Int32 i = 0;
	i++;
}

//////////////////////////////////////////////////////////////////////////

void ICamera::Data::Reset()
{
	m_position = Vector::ZEROS;
	m_rotation = EulerAngles::ZEROS;

	m_hasFocus						= false;
	m_focus							= Vector::ZEROS;

	m_fov							= 0.f;
	m_nearPlane						= 0.f;
	m_farPlane						= 0.f;
	
	m_forceNearPlane				= false;
	m_forceFarPlane					= false;

	m_dofParams.overrideFactor		= 0.f;
	m_dofParams.dofIntensity		= 0.f;
	m_dofParams.dofBlurDistNear		= 0.f;
	m_dofParams.dofFocusDistNear	= 0.f;
	m_dofParams.dofFocusDistFar		= 0.f;
	m_dofParams.dofBlurDistFar		= 0.f;

	m_bokehDofParams.Reset();
}

//////////////////////////////////////////////////////////////////////////

void CCameraProxy::UpdateWeight( Float timeDelta )
{
	if( m_weight != m_desiredWeight )
	{
		if ( m_blendTime == 0.f )
		{
			m_weight = m_desiredWeight;
		}
		else
		{
			m_blendTimeElapsed += timeDelta;
			if ( m_blendTimeElapsed >= m_blendTime)
			{
				m_weight = m_desiredWeight;
			}
			else
			{
				Float lerpScale = m_blendTimeElapsed / m_blendTime;

#if 1	// Smooth start and end
				lerpScale = lerpScale * M_PI - M_PI * 0.5f;
				lerpScale = MSin( lerpScale );
				lerpScale = lerpScale * 0.5f + 0.5f;
#else	// Sudden start, smooth end
				lerpScale = 1.0f - lerpScale;
				lerpScale *= lerpScale;
				lerpScale = 1.0f - lerpScale;
#endif
				m_weight = Lerp( lerpScale, m_blendStartWeight, m_desiredWeight );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CCameraDirector );

IMPLEMENT_RTTI_ENUM( EApertureValue );

CCameraDirector::CCameraDirector()
	: m_viewportWidth( 1 )
	, m_viewportHeight( 1 )
	, m_fovDistanceMultiplier( 1.f )
	, m_isCameraResetDisabled( false )
	, m_invalidateLastFrameCamera( false )
	, m_cameraInvalidated( false )
	, m_inputWeight( 1.0f )
	, m_isCachedRenderCameraValid( false )
{
	// Create the 'always on stack' camera
	CSimpleFreeCamera* camera = new CSimpleFreeCamera;

	ActivateCamera( camera, this );
}

CCameraDirector::~CCameraDirector()
{
	// Delete the 'always on stack' camera
	delete m_cameras[0].GetCamera();

	m_cameras.ClearFast();
}

#define WEIGHT_EPSILON 0.0001f

void CCameraDirector::ClearLastFrameDataInvalidation()
{

	m_invalidateLastFrameCamera = false;
	m_cameraInvalidated = false;

}

void CCameraDirector::Update( Float timeDelta )
{
	Float weightSum = 0.f;
	Float invalidatedWeight = 0.f;
	
	for( Int32 i = m_cameras.SizeInt() - 1; i >= 0; --i )
	{
		// Remove cameras on the bottom of the stack that are not taken into account any more
		// However do not remove the bottom-most which is the fail-safe camera
		if( MAbs( 1.f - weightSum ) < WEIGHT_EPSILON )
		{
			if( i > 0 )
			{
				m_cameras.RemoveAt( i );
			}
			else
			{
				m_cameras[i].SetWeight( 0.f );
			}

			continue;
		}

		CCameraProxy& proxy = m_cameras[i];

		if( proxy.IsValid() && proxy.GetCamera()->Update( timeDelta ) )
		{
			if( invalidatedWeight > 0.f )
			{
				// Inherit weight from the invalidated camera
				proxy.SetWeight( proxy.GetWeight() + invalidatedWeight );
				invalidatedWeight = 0.f;
			}

			proxy.UpdateWeight( timeDelta );
			weightSum += proxy.GetWeight();
		}
		else
		{
			// Remove invalid cameras
			invalidatedWeight = proxy.GetWeight();
			m_cameras.RemoveAt( i );
		}
	}

	// Normalize the weights

	if ( weightSum != 0.0f && weightSum != 1.0f )
	{
		for( auto it = m_cameras.Begin(), end = m_cameras.End(); it != end; ++it )
		{
			it->SetWeight( it->GetWeight() / weightSum );
		}
	}

	// Update input blend factor

	Float blendProgress;
	if ( m_cameras.Size() <= 2 )
	{
		blendProgress = 1.0f;
	}
	else
	{
		const CCameraProxy& proxy = m_cameras.Back();
		blendProgress = proxy.m_blendTime == 0.0f ? 1.0f : Min( proxy.m_blendTimeElapsed / proxy.m_blendTime, 1.0f );
	}

	if ( blendProgress < 1.0f )
	{
		m_inputWeight = 0.2f;
	}
	else
	{
		const Float inputWeightBlendSpeed = 1.0f;
		m_inputWeight = Min( 1.0f, m_inputWeight + timeDelta * inputWeightBlendSpeed );
	}
}

Float CCameraDirector::GetInputWeight() const
{
	return m_inputWeight;
}

void CCameraDirector::CacheCameraData()
{
	m_cachedData.Reset();

	Float invalidatedWeight = 0.f;

	TDynArray<BlendCameraInfo>	activeOutputs;

	for( Int32 i = m_cameras.SizeInt() - 1; i >= 0; --i )
	{
		CCameraProxy& proxy = m_cameras[i];

		ASSERT( proxy.IsValid() );

		if( invalidatedWeight > 0.f )
		{
			// Inherit weight from the invalidated camera
			proxy.SetWeight( proxy.GetWeight() + invalidatedWeight );
			invalidatedWeight = 0.f;
		}

		const Float weight = proxy.GetWeight();
		if( weight > 0.f )
		{
			BlendCameraInfo info;
			if( proxy.GetCamera()->GetData( info.m_data ) )
			{
				info.m_camera = proxy.GetCamera();
				info.m_weight = weight;
				info.m_blendProgress = proxy.m_blendTime>FLT_EPSILON ? (proxy.m_blendTimeElapsed / proxy.m_blendTime) : 0.0f;

				activeOutputs.PushBack( info );
			}
			else
			{
				// Remove invalid cameras
				invalidatedWeight = proxy.GetWeight();
				m_cameras.RemoveAt( i );
			}
		}
	}

	// Stop focused blend if there's only 1 "real" camera left (ignore camera at slot 0 because it's the default unused one)

	if ( m_cameras.Size() <= 2 )
	{
		if ( m_focusedBlend.m_focusCamera )
		{
			OnFocusedBlendEnd();
			m_focusedBlend.m_focusCamera = nullptr;
		}
	}

	if ( activeOutputs.Empty() )
	{
		return;
	}

	if( activeOutputs.Size() == 1 )
	{
		m_cachedData = activeOutputs[0].m_data;
	}
	else
	{
		BlendData( activeOutputs );
	}

	m_cachedData.m_rotation.ToMatrix( m_cachedTransform );
	m_cachedTransform.SetTranslation( m_cachedData.m_position );

	m_fovDistanceMultiplier = MeshUtilities::CalcFovDistanceMultiplier( GetFov() );
}

void CCameraDirector::BlendData( const TDynArray<BlendCameraInfo>& dataArray )
{
	ASSERT( !dataArray.Empty() );

	const Uint32 size = dataArray.Size();
	for( Uint32 i = 0; i < size; ++i )
	{
		const ICamera::Data& data = dataArray[i].m_data;
		const Float weight = dataArray[i].m_weight;

		m_cachedData.m_position						+= data.m_position						* weight;

		m_cachedData.m_fov							+= data.m_fov							* weight;
		m_cachedData.m_nearPlane					+= data.m_nearPlane						* weight;
		m_cachedData.m_farPlane						+= data.m_farPlane						* weight;

		m_cachedData.m_dofParams.overrideFactor		+= data.m_dofParams.overrideFactor		* weight;
		m_cachedData.m_dofParams.dofIntensity		+= data.m_dofParams.dofIntensity		* weight;
		m_cachedData.m_dofParams.dofBlurDistNear	+= data.m_dofParams.dofBlurDistNear		* weight;
		m_cachedData.m_dofParams.dofFocusDistNear	+= data.m_dofParams.dofFocusDistNear	* weight;
		m_cachedData.m_dofParams.dofFocusDistFar	+= data.m_dofParams.dofFocusDistFar		* weight;
		m_cachedData.m_dofParams.dofBlurDistFar		+= data.m_dofParams.dofBlurDistFar		* weight;
	}

	EulerAngles outRotation;

	// Blend rotations

	outRotation = dataArray[ 0 ].m_data.m_rotation;
	Float outWeight = dataArray[ 0 ].m_weight;
	for ( Uint32 i = 1; i < size; i++ )
	{
		EulerAngles nextRotation = dataArray[ i ].m_data.m_rotation;
		const Float nextWeight = dataArray[ i ].m_weight;

		outWeight += nextWeight;
		const Float lerpWeight = nextWeight / outWeight;

		outRotation = EulerAngles::Interpolate( outRotation, nextRotation, lerpWeight );
	}

	// Store rotation

	m_cachedData.m_rotation = outRotation;

	// Override transform based on special "focus target" camera

	if ( m_focusedBlend.m_focusCamera )
	{
		ApplyFocusCamera( dataArray );
	}
}

void CCameraDirector::ResetCameraData()
{
	/*
	// If there is only one camera, reset it fast
	if( m_cameras.Size() == 1 )
	{
		m_cameras[0].GetCamera()->ResetCamera();
	}
	else
	{
		// Delete first camera on stack if exists
		if( !m_cameras.Empty() && m_cameras[0].GetCamera() )
		{
			// It will destroy rest of the cameras on stack
			delete m_cameras[0].GetCamera();
		}
		m_cameras.ClearFast();

		// Create the new 'always on stack' camera
		ActivateCamera( new CSimpleFreeCamera(), this );
	}
	*/

	for( auto& it : m_cameras )
	{
		if( it.GetCamera() )
		{
			it.GetCamera()->ResetCamera();
		}
		
		if( it.IsAbandoned() && it.IsValid() )
		{
			it.Abandon();
		}
	}

}

Float CCameraDirector::GetFovDistanceMultiplier() const 
{ 
	ASSERT( m_fovDistanceMultiplier == MeshUtilities::CalcFovDistanceMultiplier( GetFov() ) );
	return m_fovDistanceMultiplier;
}

void CCameraDirector::ApplyFocusCamera( const TDynArray<BlendCameraInfo>& dataArray )
{
	// Process all cameras to figure out rotation and distance weights and other params

	Float focusRotationWeight = 1.0f;
	Float focusDistanceWeight = 0.0f;

	const ICamera::Data* data = nullptr;
	Vector blendToPositionXY = Vector::ZEROS;
	Vector blendFromPositionXY = Vector::ZEROS;
	Float blendFromWeightSum = 0.0f;
	Float focusedBlendProgress = 0.0f;
	for ( auto it = dataArray.Begin(), end = dataArray.End(); it != end; ++it )
	{
		if ( it->m_camera == m_focusedBlend.m_focusCamera )
		{
			focusedBlendProgress = it->m_blendProgress;

			focusRotationWeight = focusedBlendProgress * 4.0f; // Blend rotation in quickly at the beginning
			focusRotationWeight = ( focusRotationWeight >= 1.0f ) ? 1.0f : ( 1.0f - Red::Math::MSqr( 1.0f - focusRotationWeight ) ); // Start fast, end slow

			focusDistanceWeight = focusedBlendProgress;
			focusDistanceWeight = 1.0f - Red::Math::MPow( 1.0f - focusDistanceWeight, 1.2f ); // Start fast, end slow

			data = &it->m_data;
			blendToPositionXY = it->m_data.m_position;
		}
		else
		{
			blendFromPositionXY += it->m_data.m_position * it->m_weight;
			blendFromWeightSum += it->m_weight;
		}
	}

	// Normalize "from position" based on weights

	blendFromPositionXY *= 1.0f / blendFromWeightSum;

	// Remove Z from positions

	Vector focusXY = data->m_focus;
	focusXY.Z = 0.0f;
	blendFromPositionXY.Z = 0.0f;
	blendToPositionXY.Z = 0.0f;

	// Figure out start and end angles

	const EulerAngles startAngles = ( blendFromPositionXY - focusXY ).ToEulerAngles();
	EulerAngles endAngles = ( blendToPositionXY - focusXY ).ToEulerAngles();

	if ( m_focusedBlend.m_justStarted )
	{
		// Make sure end angles are as close as possible to start angles

		endAngles = startAngles + EulerAngles::AngleDistance( startAngles, endAngles );

		// Store initial angles

		m_focusedBlend.m_justStarted = false;
		m_focusedBlend.m_startAngles = startAngles;
		m_focusedBlend.m_endAngles = endAngles;
	}

	// Calculate interpolated angle needed to calculate position
	// Make sure the camera doesn't jump from one side to another when blended game camera rotates by much - this is achieved by _not_ wrapping start/end blend angles as they change during blend

	EulerAngles angle;
#define BLEND_ANGLE_COMPONENT( component ) \
	m_focusedBlend.m_startAngles.component += EulerAngles::AngleDistance( m_focusedBlend.m_startAngles.component, startAngles.component ); \
	m_focusedBlend.m_endAngles.component += EulerAngles::AngleDistance( m_focusedBlend.m_endAngles.component, endAngles.component ); \
	angle.component = startAngles.component + ( m_focusedBlend.m_endAngles.component - m_focusedBlend.m_startAngles.component ) * focusDistanceWeight;

	BLEND_ANGLE_COMPONENT( Yaw );
	BLEND_ANGLE_COMPONENT( Pitch );
	BLEND_ANGLE_COMPONENT( Roll );

	//RED_LOG( camera, TXT("yaw: start = %f end = %f angle = %f"), m_blendStartAngles.Yaw, m_blendEndAngles.Yaw, angle.Yaw );

	// Override position with one calculated using distance between focus point and the camera

	const Float blendFromDistance = focusXY.DistanceTo( blendFromPositionXY );
	const Float blendToDistance = focusXY.DistanceTo( blendToPositionXY );
	const Float desiredDistance = blendFromDistance + focusDistanceWeight * ( blendToDistance - blendFromDistance );

	Vector dir = angle.TransformVector( Vector( 0.0f, 1.0f, 0.0f, 1.0f ) );
	dir.Z = 0.0f;
	dir.Normalize3();

	Vector newPosition = data->m_focus + dir * desiredDistance; // Only blend in XY to avoid sudden ups & downs
	newPosition.Z = m_cachedData.m_position.Z; // Preserve original interpolated height

	m_cachedData.m_position = newPosition;

	// Override rotation with one calculated using focus target point

	dir = data->m_focus - m_cachedData.m_position;

	EulerAngles focusRotation;
	focusRotation.Yaw = dir.Y != 0.0f ? RAD2DEG( -atan2f( dir.X, dir.Y ) ) : 0.0f;
	const Float lenSqr = dir.X * dir.X + dir.Y * dir.Y;
	focusRotation.Pitch = ( lenSqr != 0.0f && dir.Z != 0.0f ) ? RAD2DEG( atan2f( dir.Z, sqrtf( lenSqr ) ) ) : 0.0f;
	focusRotation.Roll = 0.0f;

	if ( focusRotationWeight < 1.0f ) // Note: This isn't meant to be optimization; this is here to avoid camera jumps
	{
		m_cachedData.m_rotation = EulerAngles::Interpolate( m_cachedData.m_rotation, focusRotation, focusRotationWeight );
	}
	else
	{
		m_cachedData.m_rotation = focusRotation;
	}

	// Reset DOF to prevent dialogs from making things blurry (e.g. during dialog-gameplay transitions)

	m_cachedData.m_dofParams.Reset();

	OnFocusedBlendUpdate( focusedBlendProgress );
}

void CCameraDirector::ActivateCamera( ICamera* camera, const THandle< IScriptable >& parent, Float blendTime, Bool useFocusTarget, Bool resetCamera )
{
	// Blend in if exists

	Bool found = false;
	for( auto it = m_cameras.Begin(), end = m_cameras.End(); it != end; ++it )
	{
		if ( it->GetCamera() == camera )
		{
			it->StartBlending( 1.0f, blendTime );

			TDynArray<CCameraProxy>::iterator next = it + 1;

			// Move the activated camera on top of the stack
			while( next != end )
			{
				m_cameras.Swap( it++, next++ );
			}
			found = true;
			break;
		}
	}

	// Add if doesn't exist

	if ( !found )
	{
		const IScriptable* previousCamObject = m_cameras.Empty() ? nullptr : m_cameras.Back().GetParent().GetConst();

		m_cameras.PushBack( CCameraProxy( camera, parent, blendTime, 1.0f ) );
		m_cameras.Back().GetCamera()->OnActivate( previousCamObject, resetCamera );
	}

	// Blend out other cameras

	for( auto it = m_cameras.Begin(), end = m_cameras.End(); it != end; ++it )
	{
		if ( it->GetCamera() != camera )
		{
			it->StartBlending( 0.0f, blendTime );
		}
	}

	// Remove dead cameras immediately (and update weights) if blendTime is 0.0f

	if ( blendTime == 0.0f )
	{
		TDynArray< CCameraProxy > tempProxies = Move( m_cameras );
		for ( Uint32 i = 0; i < tempProxies.Size(); ++i )
		{
			CCameraProxy& camera = tempProxies[ i ];

			camera.m_weight = camera.m_desiredWeight;
			if ( camera.m_weight != 0.0f || i == 0 ) // Only keep the camera if it has non-zero weight (or is the first, fail-safe, camera)
			{
				m_cameras.PushBack( camera );
			}
		}
	}

	// Set "focus target" camera

	else if ( useFocusTarget )
	{
		m_focusedBlend.m_focusCamera = camera;
		m_focusedBlend.m_justStarted = true;
		OnFocusedBlendBegin();
	}

	CacheCameraData();
}

Bool CCameraDirector::IsCameraActive( const ICamera* camera ) const
{
	const TDynArray<CCameraProxy>::const_iterator end = m_cameras.End();
	for( TDynArray<CCameraProxy>::const_iterator it = m_cameras.Begin(); it != end; ++it )
	{
		if( it->GetCamera() == camera )
		{
			return it->GetWeight() > 0.f;
		}
	}

	return false;
}

void CCameraDirector::AbandonCamera( ICamera* camera )
{
	const TDynArray<CCameraProxy>::iterator end = m_cameras.End();
	for( TDynArray<CCameraProxy>::iterator it = m_cameras.Begin(); it != end; ++it )
	{
		if( it->GetCamera() == camera )
		{
			it->Abandon();
			return;
		}
	}
}

void CCameraDirector::AbandonTopmostCamera()
{
	m_cameras.Back().Abandon();
}

void CCameraDirector::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	const TDynArray<CCameraProxy>::iterator end = m_cameras.End();
	for( TDynArray<CCameraProxy>::iterator it = m_cameras.Begin(); it != end; ++it )
	{
		// sustain object lifetime if abandoned
		if( it->IsAbandoned() && it->IsValid() )
		{
			it->GetParent()->OnSerialize( file );
		}
	}
}

void CCameraDirector::CalcVisibilityDataAndCacheRenderCamera( SCameraVisibilityData& data )
{
	// TODO - calc it in proper way - this is a hack
	CFrustum f;
	{
		const Uint32 viewWidth = GGame->GetViewport()->GetWidth();
		const Uint32 viewHeight = GGame->GetViewport()->GetHeight();

		CRenderCamera camera;
		GGame->GetActiveWorld()->GetCameraDirector()->OnSetupCamera( camera, viewWidth, viewHeight );

		f.InitFromCamera( camera.GetWorldToScreen() );

#ifdef DEBUG_CAM_ASAP
		m_cachedDebugVisRenderCam = camera;
#endif

		// Cache render camera because it is used in many places like view coords to/from world coords
		m_cachedRenderCamera = camera;
		m_isCachedRenderCameraValid = true;
	}

	// Make to frustum larger for safety reason
	data.Set( f );
}

void CCameraDirector::OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera )
{
	if( view->IsMinimized() )
		return;

	m_viewportWidth = view->GetWidth();
	m_viewportHeight = view->GetHeight();

	OnSetupCamera( camera, m_viewportWidth, m_viewportHeight );

	if( m_cameraInvalidated || ( m_invalidateLastFrameCamera && m_cachedRenderCamera.CameraInvalidateDifference( camera ) ) )
	{
		m_lastFrameCamera = SRenderCameraLastFrameData::INVALID;
		m_cameraInvalidated = true;
	}
	else
	{
		m_lastFrameCamera.Init( GGame->GetEngineTime(), m_cachedRenderCamera );
	}

	camera.SetLastFrameData( m_lastFrameCamera );

	m_cachedRenderCamera = camera;
	m_isCachedRenderCameraValid = true;
}

void CCameraDirector::OnSetupCamera( CRenderCamera& camera, IViewport* viewport ) const
{
#ifndef _WIN64
	OnSetupCamera( camera, viewport->GetWidth(), viewport->GetHeight() );
#else
	RED_UNUSED(viewport);
	OnSetupCamera( camera, 640, 480 );
#endif
}

void CCameraDirector::CalcCamera( CRenderCamera& camera, Uint32 viewportWidth, Uint32 viewportHeight, const Vector& position, const EulerAngles& rotation, Float fov, Float aspect, Float nearPlane, Float farPlane, Bool forceNearPlane, Bool forceFarPlane )
{
	// Calculate aspect ratio of target viewport
	Float viewAspect = viewportWidth / (Float)viewportHeight;

	// Setup camera
	if ( aspect > 0.f && viewportHeight > 1 && viewportWidth > 1 )
	{
		Float finalAspect = 1.0f;
		Float finalZoom = 1.0f;

		// Adjust
		if ( viewAspect <= aspect )
		{
			// Calculate render viewport size
			Float cameraHeight = viewportWidth / aspect;
			ASSERT( cameraHeight <= viewportHeight );

			// Calculate resizing factor
			Float resizeFactor = cameraHeight / viewportHeight;
			finalZoom = resizeFactor;
			finalAspect = aspect * resizeFactor;
		}
		else
		{
			// Calculate render viewport size
			Float cameraWidth = viewportHeight * aspect;
			ASSERT( cameraWidth <= viewportWidth );

			// Calculate resizing factor
			Float resizeFactor = cameraWidth / viewportWidth;
			finalZoom = 1.0f;
			finalAspect = aspect / resizeFactor;
		}

		// Setup camera
		camera.Set( position, rotation, fov, finalAspect, nearPlane, farPlane, finalZoom, false );

		if ( forceNearPlane )
		{
			camera.SetNonDefaultNearRenderingPlane();
		}

		if ( forceFarPlane )
		{
			camera.SetNonDefaultFarRenderingPlane();
		}
	}
	else
	{
		// Stretch camera to full viewport, aspect ratio is not preserved
		camera.Set( position, rotation, fov, viewAspect, nearPlane, farPlane, 1.f, false );

		if ( forceNearPlane )
		{
			camera.SetNonDefaultNearRenderingPlane();
		}

		if ( forceFarPlane )
		{
			camera.SetNonDefaultFarRenderingPlane();
		}
	}
}

void CCameraDirector::ViewCoordsToWorldVector( const Vector& pos, const EulerAngles& rot, Float fov, Int32 x, Int32 y, Int32 width, Int32 height, Float nearPlane, Float farPlane, Vector& outRayStart, Vector& outRayDirection )
{
	outRayStart = pos;

	CRenderCamera camera;
	CalcCamera( camera, width, height, pos, rot, fov, 0.f, nearPlane, farPlane, false, false );

	const Float halfWidth  = width * 0.5f;
	const Float halfHeight = height * 0.5f;

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

void CCameraDirector::ViewCoordsToWorldVector( Int32 x, Int32 y, Vector & outRayStart, Vector & outRayDirection ) const
{
	outRayStart = m_cachedData.m_position;

	const Float halfWidth  = m_viewportWidth * 0.5f;
	const Float halfHeight = m_viewportHeight * 0.5f;

	// Calculate screen space point
	Vector screenSpacePoint, endPoint;
	screenSpacePoint.X = ( x / halfWidth ) - 1.0f;
	screenSpacePoint.Y = 1.0f - ( y / halfHeight );
	screenSpacePoint.Z = 1.0f;
	screenSpacePoint.W = 1.0f;

	RED_CAMERA_ASSERT( m_isCachedRenderCameraValid );
	endPoint = m_cachedRenderCamera.GetScreenToWorld().TransformVectorWithW( screenSpacePoint );
	endPoint.Div4( endPoint.W );

	// Use camera origin as ray origin
	outRayDirection = ( endPoint - outRayStart ).Normalized3();
	outRayDirection.W = 0.f;
}

void CCameraDirector::WorldVectorToViewCoords( const Vector& point, const Vector& pos, const EulerAngles& rot, Float fov, Int32 width, Int32 height, Float nearPlane, Float farPlane, Int32& x, Int32& y )
{
	CRenderCamera camera;
	CalcCamera( camera, width, height, pos, rot, fov, 0.f, nearPlane, farPlane, false, false );

	Vector screenSpacePoint = camera.GetWorldToScreen().TransformVectorWithW( point );

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
	x = (Int32)( (  screenSpacePoint.X + 1.0f ) * width  * 0.5f );
	y = (Int32)( ( -screenSpacePoint.Y + 1.0f ) * height * 0.5f );
}

Bool CCameraDirector::WorldVectorToViewCoords( const Vector& point, const Vector& pos, const EulerAngles& rot, Float fov, Int32 width, Int32 height, Float nearPlane, Float farPlane, Float& xSS, Float& ySS )
{
	CRenderCamera camera;
	CalcCamera( camera, width, height, pos, rot, fov, 0.f, nearPlane, farPlane, false, false );

	Vector screenSpacePoint = camera.GetWorldToScreen().TransformVectorWithW( point );

	if ( screenSpacePoint.W < 0.001f )
	{
		xSS = 0.f;
		ySS = 0.f;
		return false;
	}

	screenSpacePoint /= screenSpacePoint.W;

	xSS = screenSpacePoint.X;
	ySS = -screenSpacePoint.Y;

	return true;
}

void CCameraDirector::WorldVectorToViewCoords( const Vector& worldPos, Int32& x, Int32& y ) const
{
	RED_CAMERA_ASSERT( m_isCachedRenderCameraValid );
	Vector screenSpacePoint = m_cachedRenderCamera.GetWorldToScreen().TransformVectorWithW( worldPos );

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
	x = (Int32)( (  screenSpacePoint.X + 1.0f ) * m_viewportWidth  * 0.5f );
	y = (Int32)( ( -screenSpacePoint.Y + 1.0f ) * m_viewportHeight * 0.5f );
}

Bool CCameraDirector::WorldVectorToViewRatio( const Vector& worldPos, Float& x, Float& y ) const
{
	RED_CAMERA_ASSERT( m_isCachedRenderCameraValid );
	Vector screenSpacePoint = m_cachedRenderCamera.GetWorldToScreen().TransformVectorWithW( worldPos );

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

Bool CCameraDirector::TestWorldVectorToViewRatio( const Vector& worldPos, Float& x, Float& y, const Vector& cameraPos, const EulerAngles& cameraRot ) const
{
	CRenderCamera camera;
	CalcCamera( camera, m_viewportWidth, m_viewportHeight, cameraPos, cameraRot, GetFov(), 0.0f, m_cachedData.m_nearPlane, m_cachedData.m_farPlane, m_cachedData.m_forceNearPlane, m_cachedData.m_forceFarPlane );

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

Bool CCameraDirector::IsPointInView( const Vector& point, const Matrix& camera, Float fov, Uint32 viewportWidth, Uint32 viewportHeight )
{
	PC_SCOPE_PIX( IsPointInView );

	const Matrix worldToLocal = camera.FullInverted();

	Vector pointLocal = worldToLocal.TransformPoint( point );

	// Skip tests for points behind the camera
	if ( pointLocal.Y <= 0.f )
	{
		return false;
	}

	Float tanHalfFov = MTan( DEG2RAD( fov ) * 0.5f );
	Float yInv = 1.0f / pointLocal.Y;

	// Horizontal FOV test
	Float tanHorzAngle = MAbs( pointLocal.X ) * yInv;
	// Aspect ratio must be applied in the tangents space!
	if ( tanHorzAngle > ( tanHalfFov * (Float)viewportWidth ) / (Float)viewportHeight )
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

Bool CCameraDirector::IsPointInView( const Vector& point, Float fovMultipier /* = 1.0f */) const
{
	PC_SCOPE_PIX( IsPointInView );

	const Matrix worldToLocal = m_cachedTransform.FullInverted();

	Vector pointLocal = worldToLocal.TransformPoint( point );

	// Skip tests for points behind the camera
	if ( pointLocal.Y <= 0.f )
	{
		return false;
	}

	Float tanHalfFov = MTan( DEG2RAD( GetFov() * fovMultipier ) * 0.5f );
	Float yInv = 1.0f / pointLocal.Y;

	// Horizontal FOV test
	Float tanHorzAngle = MAbs( pointLocal.X ) * yInv;
	// Aspect ratio must be applied in the tangents space!
	if ( tanHorzAngle > ( tanHalfFov * (Float)m_viewportWidth ) / (Float)m_viewportHeight )
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

Vector CCameraDirector::ProjectPoint( const Vector& worldSpacePoint, const CRenderCamera& renderCamera ) const
{
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

Vector CCameraDirector::UnprojectPoint( const Vector& screenSpacePoint, const CRenderCamera& renderCamera ) const
{
	Vector unprojected = renderCamera.GetScreenToWorld().TransformVectorWithW( screenSpacePoint );
	unprojected /= unprojected.W;

	return unprojected;
}

Float CCameraDirector::GetNodeAngleInCameraSpace( const CNode* node ) const
{
	Vector cameraDir = GetCameraForward();
	Vector entityDir = node->GetWorldForward();

	entityDir.Z = 0.0f;
	entityDir.Normalize3();

	cameraDir.Z = 0.0f;
	cameraDir.Normalize3();

	Float entityHeading = atan2f( entityDir.X, entityDir.Y );
	Float cameraHeading = atan2f( cameraDir.X, cameraDir.Y );

	if ( entityHeading < 0.0f ) 
	{
		entityHeading += 2.0f * M_PI;
	}

	if ( cameraHeading < 0.0f )
	{
		cameraHeading += 2.0f * M_PI;
	}

	Float diff = entityHeading - cameraHeading;
	if ( diff > M_PI )
	{
		diff = - ( 2.0f * M_PI - diff );
	}

	if ( diff < -M_PI )
	{
		diff = 2.0f * M_PI + diff;
	}


	return diff / M_PI;
}

void CCameraDirector::GenerateEditorFragments( CRenderFrame* frame )
{
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_CameraVisibility ) )
	{
#ifdef DEBUG_CAM_ASAP
		frame->AddDebugFrustum( m_cachedDebugVisRenderCam.GetScreenToWorld(), Color( 0, 0, 255 ), true, false, 1000.f, true );
#endif
	}
}

void CCameraDirector::GetCameraProxiesForDebugOnly( TDynArray< const CCameraProxy* >& outProxies ) const
{
	outProxies.Reserve( m_cameras.Size() );

	for ( const auto& it : m_cameras )
	{
		outProxies.PushBack( &it );
	}
}

Bool CCameraDirector::IsAnyCameraActive() const
{
	RED_ASSERT( m_cameras.Size() > 0, TXT( "What about the backup camera (instance of CSimpleFreeCamera)? "));
	return m_cameras.Size() > 1;
}

void CCameraDirector::OnFocusedBlendBegin()
{
	if ( CEntity* player = GGame->GetPlayerEntity() )
	{
		CallFunction( player, CNAME( OnFocusedCameraBlendBegin ) );
	}
}

void CCameraDirector::OnFocusedBlendUpdate( Float progress )
{
	if ( CEntity* player = GGame->GetPlayerEntity() )
	{
		CallFunction( player, CNAME( OnFocusedCameraBlendUpdate ), progress );
	}
}

void CCameraDirector::OnFocusedBlendEnd()
{
	if ( CEntity* player = GGame->GetPlayerEntity() )
	{
		CallFunction( player, CNAME( OnFocusedCameraBlendEnd ) );
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


void CCameraDirector::funcViewCoordsToWorldVector( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, x, 0 );
	GET_PARAMETER( Int32, y, 0 );
	GET_PARAMETER_REF( Vector, rayStart, Vector() );
	GET_PARAMETER_REF( Vector, rayDirection, Vector() );
	FINISH_PARAMETERS;

	ViewCoordsToWorldVector( x, y, rayStart, rayDirection );
}

void CCameraDirector::funcWorldVectorToViewCoords( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, worldPos, Vector() );
	GET_PARAMETER_REF( Int32, x, 0 );
	GET_PARAMETER_REF( Int32, y, 0 );
	FINISH_PARAMETERS;

	WorldVectorToViewCoords( worldPos, x, y );
}

void CCameraDirector::funcWorldVectorToViewRatio( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, worldPos, Vector() );
	GET_PARAMETER_REF( Float, x, 0 );
	GET_PARAMETER_REF( Float, y, 0 );
	FINISH_PARAMETERS;

	RETURN_BOOL( WorldVectorToViewRatio( worldPos, x, y ) );
}

void CCameraDirector::funcGetCameraPosition( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_STRUCT( Vector, GetCameraPosition() );
}

void CCameraDirector::funcGetCameraRotation( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_STRUCT( EulerAngles, GetCameraRotation() );
}



void CCameraDirector::funcGetCameraForward( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_STRUCT( Vector, GetCameraForward() );
}

void CCameraDirector::funcGetCameraRight( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_STRUCT( Vector, GetCameraRight() );
}

void CCameraDirector::funcGetCameraUp( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_STRUCT( Vector, GetCameraUp() );
}

void CCameraDirector::funcGetCameraHeading( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_FLOAT( GetCameraRotation().Yaw );
}

void CCameraDirector::funcGetCameraDirection( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_STRUCT( Vector, GetCameraForward() );
}

void CCameraDirector::funcGetFov( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_FLOAT( GetFov() );
}

void CCameraDirector::funcGetTopmostCameraObject( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetTopmostCameraObject() );
}

void CCameraDirector::funcProjectPoint( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, point, Vector() );
	FINISH_PARAMETERS;

	RETURN_STRUCT( Vector, ProjectPoint( point ) );
}

void CCameraDirector::funcUnprojectPoint( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, point, Vector() );
	FINISH_PARAMETERS;

	RETURN_STRUCT( Vector, UnprojectPoint( point ) );
}

void CCameraDirector::funcIsPointInView( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, point, Vector() );
	FINISH_PARAMETERS;

	RETURN_BOOL( IsPointInView( point ) );
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 


Bool CSimpleFreeCamera::Update( Float timeDelta )
{
	return true;
}

Bool CSimpleFreeCamera::GetData( Data& outData ) const
{
	outData = m_data;

	return true;
}

void CSimpleFreeCamera::OnActivate( const IScriptable* prevCameraObject, Bool resetCamera )
{
	if ( !resetCamera )
	{
		return;
	}

	m_data.m_nearPlane = 0.25f;
	m_data.m_farPlane = 1000.0f;
	m_data.m_forceNearPlane = false;
	m_data.m_forceFarPlane = false;
	m_data.m_fov = 70.f;
	m_data.m_position = Vector::ZERO_3D_POINT;
	m_data.m_rotation = EulerAngles::ZEROS;
	m_data.m_dofParams.Reset();
}

#ifdef DEBUG_CAMERA_DIRECTOR
#pragma optimize("",on)
#endif
