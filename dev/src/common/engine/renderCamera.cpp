/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderCamera.h"
#include "mesh.h"

const SRenderCameraLastFrameData SRenderCameraLastFrameData::INVALID;

static const Float INVALID_POSITION_THRESHOLD	= 0.5f;
static const Float INVALID_ROTATION_THRESHOLD	= 5.0f;
static const Float INVALID_FOV_THRESHOLD		= 2.0f;

SRenderCameraLastFrameData::SRenderCameraLastFrameData()
{
	// Dont use any Matrix static variables 
	// in case of instances defined globally.

	m_isValid = false;
	m_engineTime = 0;
	m_position.Set4( 0, 0, 0, 1 );	
	m_rotation = EulerAngles ( 0, 0, 0 );
	m_worldToView.SetIdentity();
	m_viewToScreen.SetIdentity();
	m_fov = 0.0f;
}

SRenderCameraLastFrameData& SRenderCameraLastFrameData::Set( Bool isValid, Float engineTime, const Vector &position, const EulerAngles &rotation, const Matrix &worldToView, const Matrix &viewToScreen , Float fov )
{
	m_isValid = isValid;
	m_engineTime = engineTime;
	m_position = position;
	m_rotation = rotation;
	m_worldToView = worldToView;
	m_viewToScreen = viewToScreen;
	m_fov = fov;
	return *this;
}

SRenderCameraLastFrameData& SRenderCameraLastFrameData::Init( Float engineTime, const Vector &position, const EulerAngles &rotation, Float fov, Float aspect, Float nearPlane, Float farPlane, Float zoom )
{
	// Calculate view matrix
	Matrix axesConversion( Vector( 1,0,0,0), Vector( 0,0,1,0), Vector( 0,1,0,0), Vector( 0,0,0,1 ) );
	Matrix rotMatrix = rotation.ToMatrix().Transposed();
	Matrix transMatrix = Matrix::IDENTITY;
	transMatrix.SetTranslation( - position );
	Matrix worldToView = (transMatrix * rotMatrix) * axesConversion;

	// Build projection matrix
	Matrix viewToScreen;
	if ( fov == 0.0f )
	{
		viewToScreen.BuildOrthoLH( zoom, zoom * aspect, nearPlane, farPlane );
	}
	else
	{
		viewToScreen.BuildPerspectiveLH( DEG2RAD(fov), aspect, nearPlane, farPlane );
		viewToScreen.V[0].A[0] *= zoom;
		viewToScreen.V[1].A[1] *= zoom;
	}

	//
	return Set( true, engineTime, position, rotation, worldToView, viewToScreen, fov );
}


Bool CRenderCamera::CameraInvalidateDifference( const class CRenderCamera& camera ) const
{
	// Check position teleport difference
	Float positionDiff		= m_position.DistanceSquaredTo( camera.GetPosition() );
	if( positionDiff > INVALID_POSITION_THRESHOLD*INVALID_POSITION_THRESHOLD )
		return true;

	// Check fast rotate difference
	EulerAngles angleDiff	= EulerAngles::AngleDistance( m_rotation, camera.GetRotation() );
	Float angleDiffSum		= Abs(angleDiff.Pitch) + Abs(angleDiff.Roll) + Abs(angleDiff.Yaw);

	if( angleDiffSum > INVALID_ROTATION_THRESHOLD )
		return true;
	
	// Check camera fov change
	Float fovDiff			= m_fov - camera.GetFOV();
	if( ::Abs(fovDiff) > INVALID_FOV_THRESHOLD )
		return true;

	return false;
}


SRenderCameraLastFrameData& SRenderCameraLastFrameData::Init( Float engineTime, const CRenderCamera &camera )
{
	return Init( engineTime, camera.GetPosition(), camera.GetRotation(), camera.GetFOV(), camera.GetAspect(), camera.GetNearPlane(), camera.GetFarPlane(), camera.GetZoom() );
}


SCameraChangeTreshold::SCameraChangeTreshold()
	: m_greater( true )
	, m_anyMatch( true )
	, m_positionTreshold( INVALID_POSITION_THRESHOLD )
	, m_rotationTreshold( INVALID_ROTATION_THRESHOLD )
	, m_fovTreshold( INVALID_FOV_THRESHOLD )
	, m_outputSimilarity( 0.0f )
	, m_checkMask( ECM_All )
{}

Bool SCameraChangeTreshold::IsPositionChanged( const Float differenceSquared )  
{
	const Float p2 = m_positionTreshold*m_positionTreshold;
	const Float d = m_greater ? differenceSquared - p2 : p2 - differenceSquared;
	m_outputSimilarity = ::Max( m_outputSimilarity , - d / p2 );
	return ( d > 0.0f );
}

Bool SCameraChangeTreshold::IsRotationChanged( const Float difference )
{ 
	const Float d = m_greater ? difference - m_rotationTreshold : m_rotationTreshold - difference;
	m_outputSimilarity = ::Max( m_outputSimilarity , - d / m_rotationTreshold );
	return ( d > 0.0f );
}

Bool SCameraChangeTreshold::IsFOVChanged( const Float difference ) 
{
	const Float d = m_greater ? difference - m_fovTreshold : m_fovTreshold - difference;
	m_outputSimilarity = ::Max( m_outputSimilarity , - d / m_fovTreshold );
	return ( d > 0.0f );
}

Bool SCameraChangeTreshold::DoesCameraChanged( const class CRenderCamera& camera, const class CRenderCamera& previousFrameCamera )
{
	// Check position teleport difference
	const Float positionDiff		= previousFrameCamera.GetPosition().DistanceSquaredTo( camera.GetPosition() );

	if( m_anyMatch && ( m_checkMask & ECM_Position ) && IsPositionChanged( positionDiff ) )
		return true;

	// Check fast rotate difference
	const EulerAngles angleDiff	= EulerAngles::AngleDistance( previousFrameCamera.GetRotation(), camera.GetRotation() );
	const Float angleDiffSum		= Abs(angleDiff.Pitch) + Abs(angleDiff.Roll) + Abs(angleDiff.Yaw);

	if( m_anyMatch && ( m_checkMask & ECM_Rotation ) && IsRotationChanged( angleDiffSum ) )
		return true;

	// Check camera fov change
	const Float fovDiff			= previousFrameCamera.GetFOV() - camera.GetFOV();

	if( m_anyMatch && ( m_checkMask & ECM_Fov ) && IsFOVChanged( ::Abs(fovDiff) ) )
		return true;

	// We need to check every term to be true in this case
	if( false == m_anyMatch ) 
	{
		Uint8 res = 0;
		res |= ECM_Position * ( m_checkMask & ECM_Position	? IsPositionChanged( positionDiff ) : 0 );
		res |= ECM_Rotation * ( m_checkMask & ECM_Rotation	? IsRotationChanged( angleDiffSum ) : 0 );
		res |= ECM_Fov		* ( m_checkMask & ECM_Fov		? IsFOVChanged( ::Abs(fovDiff) )	: 0 );
		return res == m_checkMask;
	}

	return false;
}



CRenderCamera::CRenderCamera()
	: m_fov( 90.0f )			// 90 degrees
	, m_aspect( 1.0f )
	, m_position( 0,0,0 )
	, m_rotation( 0,0,0 )
	, m_nearPlane( 0.001f )
	, m_farPlane( 100.0f )
	, m_zoom( 1.0f )
	, m_subPixelOffsetX( 0.0f )
	, m_subPixelOffsetY( 0.0f )
	, m_nonDefaultFarPlane( false )
	, m_nonDefaultNearPlane( false )
	, m_isReversedProjection( false )
{	
	CalculateMatrices();
	CalculateVisibleDetailMultiplier();
	CalculateNearPlaneCornerDist();
	CalculateFOVMultiplier();
}

CRenderCamera::CRenderCamera( const Vector& position, const EulerAngles& rotation, Float fov, Float aspect, Float nearPlane, Float farPlane, Float zoom, Bool isReversedProjection, const SRenderCameraLastFrameData& lastFrameData )
	: m_fov( fov )
	, m_aspect( aspect )
	, m_position( position )
	, m_rotation( rotation )
	, m_nearPlane( nearPlane )
	, m_farPlane( farPlane )
	, m_zoom( zoom )
	, m_subPixelOffsetX( 0.0f )
	, m_subPixelOffsetY( 0.0f )
	, m_nonDefaultFarPlane( false )
	, m_nonDefaultNearPlane( false )
	, m_lastFrameData( lastFrameData )
	, m_isReversedProjection( isReversedProjection )
{
	CalculateMatrices();
	CalculateVisibleDetailMultiplier();
	CalculateNearPlaneCornerDist();
	CalculateFOVMultiplier();
}

CRenderCamera::CRenderCamera( const CRenderCamera& camera )
	: m_fov( camera.m_fov )
	, m_aspect( camera.m_aspect )
	, m_position( camera.m_position )
	, m_rotation( camera.m_rotation )
	, m_nearPlane( camera.m_nearPlane )
	, m_farPlane( camera.m_farPlane )
	, m_zoom( camera.m_zoom )
	, m_subPixelOffsetX( camera.m_subPixelOffsetX )
	, m_subPixelOffsetY( camera.m_subPixelOffsetY )
	, m_nonDefaultFarPlane( camera.m_nonDefaultFarPlane )
	, m_nonDefaultNearPlane( camera.m_nonDefaultNearPlane )
	, m_lastFrameData( camera.m_lastFrameData )
	, m_isReversedProjection( camera.m_isReversedProjection )
{
	CalculateMatrices();
	CalculateVisibleDetailMultiplier();
	CalculateNearPlaneCornerDist();
	CalculateFOVMultiplier();
}

void CRenderCamera::operator = ( const CRenderCamera& camera )
{
	m_fov = camera.m_fov;
	m_aspect = camera.m_aspect;
	m_position = camera.m_position;
	m_rotation = camera.m_rotation;
	m_nearPlane = camera.m_nearPlane;
	m_farPlane = camera.m_farPlane;
	m_zoom = camera.m_zoom;
	m_subPixelOffsetX = camera.m_subPixelOffsetX;
	m_subPixelOffsetY = camera.m_subPixelOffsetY;
	m_nonDefaultFarPlane = camera.m_nonDefaultFarPlane;
	m_nonDefaultNearPlane = camera.m_nonDefaultNearPlane;
	m_lastFrameData = camera.m_lastFrameData;
	m_isReversedProjection = camera.m_isReversedProjection;

	CalculateMatrices();
	CalculateVisibleDetailMultiplier();
	CalculateNearPlaneCornerDist();
	CalculateFOVMultiplier();
}

//dex++: extracted world to screen texture (0-1 normalized space) computation for shadowmaps
Matrix CRenderCamera::CalcWorldToTexture( const Vector& position, const EulerAngles& rotation, Float fov, Float aspect, Float nearPlane, Float farPlane, Float zoom/*=1.0f*/ )
{
	// the magic axis conversion crap
	Matrix axesConversion( Vector( 1,0,0,0), Vector( 0,0,1,0), Vector( 0,1,0,0), Vector( 0,0,0,1 ) );

	// compute view space matrix
	Matrix rotMatrix = rotation.ToMatrix().Transposed();
	Matrix transMatrix = Matrix::IDENTITY;
	transMatrix.SetTranslation( -position );
	Matrix worldToView = (transMatrix * rotMatrix) * axesConversion;

	// Build projection matrix
	Matrix viewToScreen;
	if ( fov == 0.0f )
	{
		viewToScreen.BuildOrthoLH( zoom, zoom * aspect, nearPlane, farPlane );
	}
	else
	{
		viewToScreen.BuildPerspectiveLH( DEG2RAD(fov), aspect, nearPlane, farPlane );
		viewToScreen.V[0].A[0] *= zoom;
		viewToScreen.V[1].A[1] *= zoom;
	}

	// Calculate view-projection matrix
	Matrix worldToScreen = worldToView * viewToScreen;

	// Apply final <-1,1> to <0,1> range mapping
	Matrix mapping;
	mapping.SetIdentity();
	mapping.V[0].X = 0.5f;
	mapping.V[1].Y = -0.5f;
	mapping.V[3].X = 0.5f;
	mapping.V[3].Y = 0.5f;
	return worldToScreen * mapping;
}
//dex--

void CRenderCamera::CalcCameraVectors( const EulerAngles &rotation, Vector &outCameraForward, Vector &outCameraRight, Vector &outCameraUp )
{
	const Matrix axesConversion( Vector( 1,0,0,0), Vector( 0,0,1,0), Vector( 0,1,0,0), Vector( 0,0,0,1 ) );
	const Matrix rotMatrix = rotation.ToMatrix().Transposed();
	
	const Matrix &worldToCamera = rotMatrix;
	const Matrix worldToView = worldToCamera * axesConversion;
	const Matrix viewToWorld = worldToView.Inverted();

	// output result
	outCameraRight = viewToWorld.GetAxisX();
	outCameraForward = viewToWorld.GetAxisZ();
	outCameraUp = viewToWorld.GetAxisY();
}

void CRenderCamera::CalculateMatrices()
{
	// Calculate view matrix
	Matrix axesConversion( Vector( 1,0,0,0), Vector( 0,0,1,0), Vector( 0,1,0,0), Vector( 0,0,0,1 ) );
	Matrix rotMatrix = m_rotation.ToMatrix().Transposed();
	Matrix transMatrix = Matrix::IDENTITY;
	transMatrix.SetTranslation( - m_position );
	m_worldToCamera = transMatrix * rotMatrix;
	m_worldToView = m_worldToCamera * axesConversion;

	m_viewToWorld = m_worldToView.Inverted();
	m_cameraToWorld = m_worldToCamera.Inverted();

	// Build projection matrix
	if ( m_fov == 0.0f )
	{
		m_viewToScreen.BuildOrthoLH( m_zoom, m_zoom * m_aspect, m_nearPlane, m_farPlane );
	}
	else
	{
		m_viewToScreen.BuildPerspectiveLH( DEG2RAD(m_fov), m_aspect, m_nearPlane, m_farPlane );

		m_viewToScreen.V[0].A[0] *= m_zoom;
		m_viewToScreen.V[1].A[1] *= m_zoom;
	}

	m_viewToScreen.V[2].A[0] = -m_subPixelOffsetX;
	m_viewToScreen.V[2].A[1] = m_subPixelOffsetY;

	// Calculate view-projection matrix
	m_screenToView = m_viewToScreen.FullInverted();
	m_worldToScreen = m_worldToView * m_viewToScreen;
	m_screenToWorld = m_screenToView * m_viewToWorld;

	// Camera vectors - axes are flipped because "axesConversion"
	m_cameraVectors[ CV_Right ] = m_viewToWorld.GetAxisX();
	m_cameraVectors[ CV_Forward ] = m_viewToWorld.GetAxisZ();
	m_cameraVectors[ CV_Up ] = m_viewToWorld.GetAxisY();

	// Calculate reversed projection aware matrices
	CalculateRevProjAwareMatrices();
}

void CRenderCamera::CalculateRevProjAwareMatrices()
{
	if ( !m_isReversedProjection )
	{
		m_viewToScreenRevProjAware = m_viewToScreen;
		m_worldToScreenRevProjAware	= m_worldToScreen;
		m_screenToWorldRevProjAware = m_screenToWorld;
		return;
	}

	ASSERT( !IsOrtho() && "Reversed ortho not handled/tested" );

	Matrix reverseMatrix;
	reverseMatrix.SetIdentity();
	reverseMatrix.SetScale33( Vector ( 1.f, 1.f, -1.f ) );
	reverseMatrix.SetTranslation( 0.f, 0.f, 1.f );

	m_viewToScreenRevProjAware = Matrix::Mul( reverseMatrix, GetViewToScreen() );
	m_worldToScreenRevProjAware = GetWorldToView() * m_viewToScreenRevProjAware;
	m_screenToWorldRevProjAware = m_worldToScreenRevProjAware.FullInverted();
}

void CRenderCamera::CalculateVisibleDetailMultiplier()
{
	m_visDetailMul = 1.f / Max( 0.0005f, MTan(0.5f * DEG2RAD( GetFOV() )) );
}

void CRenderCamera::CalculateNearPlaneCornerDist()
{
	Float n = GetNearPlane();
	Float a = n * tanf( 0.5f * DEG2RAD(GetFOV()) );
	Float b = a * GetAspect();
	m_nearCornerDist = sqrtf( n*n + a*a + b*b );
}

void CRenderCamera::Set( const Vector& position, const EulerAngles& rotation, Float fov, Float aspect, Float nearPlane, Float farPlane, Float zoom, Bool isReversedProjection )
{
	m_fov		= fov;
	m_aspect	= aspect;
	m_position	= position;
	m_rotation	= rotation;
	m_nearPlane = nearPlane;
	m_farPlane	= farPlane;		
	m_zoom		= zoom;
	m_isReversedProjection = isReversedProjection;
	
	CalculateMatrices();
	CalculateVisibleDetailMultiplier();
	CalculateNearPlaneCornerDist();
	CalculateFOVMultiplier();
}

void CRenderCamera::SetRotation( const EulerAngles& rotation )
{
	m_rotation = rotation;

	CalculateMatrices();
	CalculateVisibleDetailMultiplier();
	CalculateNearPlaneCornerDist();
}

void CRenderCamera::SetFOV( Float fov )
{
	m_fov = fov;

	CalculateMatrices();
	CalculateVisibleDetailMultiplier();
	CalculateNearPlaneCornerDist();
	CalculateFOVMultiplier();
}

void CRenderCamera::SetReversedProjection( Bool isReversed )
{
	if ( isReversed == m_isReversedProjection )
	{
		return;
	}

	m_isReversedProjection = isReversed;
	CalculateRevProjAwareMatrices();
}

void CRenderCamera::GetFrustumCorners( const Float plane, Vector* corners, Bool localCorners ) const
{
	if ( localCorners )
	{
		Matrix mat;
		{
			// Calculate view matrix
			Matrix axesConversion( Vector( 1,0,0,0), Vector( 0,0,1,0), Vector( 0,1,0,0), Vector( 0,0,0,1 ) );
			Matrix rotMatrix = m_rotation.ToMatrix().Transposed();
			Matrix worldToCamera = rotMatrix;
			Matrix worldToView = worldToCamera * axesConversion;
			Matrix viewToWorld = worldToView.Inverted();
		
			// Build projection matrix
			Matrix viewToScreen;
			if ( m_fov == 0.0f )
			{
				viewToScreen.BuildOrthoLH( m_zoom, m_zoom * m_aspect, m_nearPlane, m_farPlane );
			}
			else
			{
				viewToScreen.BuildPerspectiveLH( DEG2RAD(m_fov), m_aspect, m_nearPlane, m_farPlane );

				viewToScreen.V[0].A[0] *= m_zoom;
				viewToScreen.V[1].A[1] *= m_zoom;
			}

			viewToScreen.V[2].A[0] = -m_subPixelOffsetX;
			viewToScreen.V[2].A[1] = m_subPixelOffsetY;

			Matrix screenToView = viewToScreen.FullInverted();
			mat = screenToView * viewToWorld;
		}

		corners[0] = mat.TransformVectorWithW( Vector(+1,-1,plane) );
		corners[1] = mat.TransformVectorWithW( Vector(+1,+1,plane) );
		corners[2] = mat.TransformVectorWithW( Vector(-1,+1,plane) );
		corners[3] = mat.TransformVectorWithW( Vector(-1,-1,plane) );

		corners[0].Div4( corners[0].W );
		corners[1].Div4( corners[1].W );
		corners[2].Div4( corners[2].W );
		corners[3].Div4( corners[3].W );
	}
	else
	{
		corners[0] = GetScreenToWorld().TransformVectorWithW( Vector(+1,-1,plane) );
		corners[1] = GetScreenToWorld().TransformVectorWithW( Vector(+1,+1,plane) );
		corners[2] = GetScreenToWorld().TransformVectorWithW( Vector(-1,+1,plane) );
		corners[3] = GetScreenToWorld().TransformVectorWithW( Vector(-1,-1,plane) );

		corners[0].Div4( corners[0].W );
		corners[1].Div4( corners[1].W );
		corners[2].Div4( corners[2].W );
		corners[3].Div4( corners[3].W );
	}

	/*
	{
		CRenderCamera cam = *this;
		cam.m_position = Vector ( 0, 0, 0, 1 );
		cam.CalculateMatrices();

		// Calc corners for given plane
		corners[0] = cam.GetScreenToWorld().TransformVectorWithW( Vector(+1,-1,plane) );
		corners[1] = cam.GetScreenToWorld().TransformVectorWithW( Vector(+1,+1,plane) );
		corners[2] = cam.GetScreenToWorld().TransformVectorWithW( Vector(-1,+1,plane) );
		corners[3] = cam.GetScreenToWorld().TransformVectorWithW( Vector(-1,-1,plane) );
		corners[0].Div4( corners[0].W );
		corners[1].Div4( corners[1].W );
		corners[2].Div4( corners[2].W );
		corners[3].Div4( corners[3].W );

		if ( !localCorners )
		{
			corners[0] += Vector ( m_position.X, m_position.Y, m_position.Z, 0.f );
			corners[1] += Vector ( m_position.X, m_position.Y, m_position.Z, 0.f );
			corners[2] += Vector ( m_position.X, m_position.Y, m_position.Z, 0.f );
			corners[3] += Vector ( m_position.X, m_position.Y, m_position.Z, 0.f );
		}
	}
	*/
}

void CRenderCamera::SetSubpixelOffset( Float x, Float y, Uint32 screenW, Uint32 screenH )
{
	ASSERT( screenW > 0 );
	ASSERT( screenH > 0 );

	m_subPixelOffsetX = x / static_cast<Float>( screenW ) * 2.0f;
	m_subPixelOffsetY = y / static_cast<Float>( screenH ) * 2.0f;

	CalculateMatrices();
}

void CRenderCamera::CalculateFOVMultiplier()
{
	m_fovMultiplier = MeshUtilities::CalcFovDistanceMultiplier( GetFOV() );
	m_fovMultiplierUnclamped = MeshUtilities::CalcFovDistanceMultiplierNoClamp( GetFOV() );
}

void CRenderCamera::CalculateFrustumPlanes( Vector* outFrustumPlanes ) const
{
	Matrix m = m_worldToScreen;

	// Left plane
	outFrustumPlanes[0].X = m.GetRow(0).W + m.GetRow(0).X;
	outFrustumPlanes[0].Y = m.GetRow(1).W + m.GetRow(1).X;
	outFrustumPlanes[0].Z = m.GetRow(2).W + m.GetRow(2).X;
	outFrustumPlanes[0].W = m.GetRow(3).W + m.GetRow(3).X;

	// Right plane
	outFrustumPlanes[1].X = m.GetRow(0).W - m.GetRow(0).X;
	outFrustumPlanes[1].Y = m.GetRow(1).W - m.GetRow(1).X;
	outFrustumPlanes[1].Z = m.GetRow(2).W - m.GetRow(2).X;
	outFrustumPlanes[1].W = m.GetRow(3).W - m.GetRow(3).X;

	// Top plane
	outFrustumPlanes[2].X = m.GetRow(0).W - m.GetRow(0).Y;
	outFrustumPlanes[2].Y = m.GetRow(1).W - m.GetRow(1).Y;
	outFrustumPlanes[2].Z = m.GetRow(2).W - m.GetRow(2).Y;
	outFrustumPlanes[2].W = m.GetRow(3).W - m.GetRow(3).Y;

	// Bottom plane
	outFrustumPlanes[3].X = m.GetRow(0).W + m.GetRow(0).Y;
	outFrustumPlanes[3].Y = m.GetRow(1).W + m.GetRow(1).Y;
	outFrustumPlanes[3].Z = m.GetRow(2).W + m.GetRow(2).Y;
	outFrustumPlanes[3].W = m.GetRow(3).W + m.GetRow(3).Y;

	// Near plane
	outFrustumPlanes[4].X = m.GetRow(0).Z;
	outFrustumPlanes[4].Y = m.GetRow(1).Z;
	outFrustumPlanes[4].Z = m.GetRow(2).Z;
	outFrustumPlanes[4].W = m.GetRow(3).Z;

	// Far plane
	outFrustumPlanes[5].X = m.GetRow(0).W - m.GetRow(0).Z;
	outFrustumPlanes[5].Y = m.GetRow(1).W - m.GetRow(1).Z;
	outFrustumPlanes[5].Z = m.GetRow(2).W - m.GetRow(2).Z;
	outFrustumPlanes[5].W = m.GetRow(3).W - m.GetRow(3).Z;

	// Normalize plane
	for( Uint8 i = 0; i < 6; ++i )
	{
		outFrustumPlanes[i] /= outFrustumPlanes[i].Mag3();
	}
}
