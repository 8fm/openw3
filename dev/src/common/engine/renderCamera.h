/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/math.h"

/// Camera vectors
enum ECameraVector
{
	CV_Right = 0,
	CV_Forward = 1,
	CV_Up = 2,
};

RED_WARNING_PUSH();
RED_DISABLE_WARNING_MSC( 4324 );

/// Camera last frame info
struct SRenderCameraLastFrameData
{
	static const SRenderCameraLastFrameData INVALID;

public:
	Bool		m_isValid;
	Float		m_engineTime;
	Float		m_fov;
	Vector		m_position;
	EulerAngles	m_rotation;
	Matrix		m_worldToView;
	Matrix		m_viewToScreen;

public:
	SRenderCameraLastFrameData();
	
	SRenderCameraLastFrameData& Set( Bool isValid, Float engineTime, const Vector &position, const EulerAngles &rotation, const Matrix &worldToView, const Matrix &viewToScreen, Float fov );
	SRenderCameraLastFrameData& Init( Float engineTime, const Vector &position, const EulerAngles &rotation, Float fov, Float aspect, Float nearPlane, Float farPlane, Float zoom );
	SRenderCameraLastFrameData& Init( Float engineTime, const class CRenderCamera &camera );

};

RED_WARNING_POP();

// Detects difference between cameras

struct SCameraChangeTreshold
{
	enum ECheckMask
	{
		ECM_Position = FLAG(0), 
		ECM_Rotation = FLAG(1), 
		ECM_Fov		 = FLAG(2), 
		ECM_All		 = ECM_Position | ECM_Rotation | ECM_Fov
	};

	Bool				m_greater;
	Bool				m_anyMatch;

	Float				m_positionTreshold;
	Float				m_rotationTreshold;
	Float				m_fovTreshold;

	Float				m_outputSimilarity;

	Uint8				m_checkMask;

	// - - - - - - - - - - - - - - - - - - - 

	SCameraChangeTreshold();

	// - - - - - - - - - - - - - - - - - - - 

	Bool IsPositionChanged( const Float differenceSquared ) ;

	Bool IsRotationChanged( const Float difference ) ;

	Bool IsFOVChanged( const Float difference ) ;

	// - - - - - - - - - - - - - - - - - - - 

	Bool DoesCameraChanged( const class CRenderCamera& camera , const class CRenderCamera& previousFrameCamera );

};

/// Low level camera
class CRenderCamera
{
private:
	Vector			m_position;			//!< Camera position
	EulerAngles		m_rotation;			//!< Rotation angles
	Float			m_fov;				//!< FOV ( in degrees )
	Float			m_fovMultiplier;	//!< FOV multiplier for the LOD calculation
	Float			m_fovMultiplierUnclamped;	//!< Unclamped FOV multiplier
	Float			m_aspect;			//!< Aspect ratio
	Float			m_zoom;				//!< Linear zoom
	Float			m_nearPlane;		//!< Near clipping plane
	Float			m_farPlane;			//!< Far clipping plane
	Float			m_nearCornerDist;	//!< Near plane corner distance
	Matrix			m_worldToView;		//!< View matrix
	Matrix			m_worldToCamera;	//!< World to camera matrix
	Matrix			m_cameraToWorld;	//!< Camera to world matrix
	Matrix			m_viewToWorld;		//!< Inverse of view matrix
	Matrix			m_screenToView;		//!< Inverse of projection matrix
	Matrix			m_viewToScreen;		//!< Projection matrix
	Matrix			m_worldToScreen;	//!< ViewProjection matrix
	Matrix			m_screenToWorld;	//!< Inverse of view projection matrix
	Vector			m_cameraVectors[3];	//!< Camera vectors
	Float			m_visDetailMul;		//!< Visible detail multiplier
	Bool			m_nonDefaultNearPlane;//!< Non default near plane
	Bool			m_nonDefaultFarPlane;//!< Non default far plane

	Bool			m_isReversedProjection;			//!< Is this camera using reversed projectio (rendering only)
	Matrix			m_worldToScreenRevProjAware;	//!< ViewProjection matrix, reversed projection aware
	Matrix			m_viewToScreenRevProjAware;		//!< Projection matrix, reversed projection aware
	Matrix			m_screenToWorldRevProjAware;	//!< Inverse of view projection matrix, reversed projection aware

	Float			m_subPixelOffsetX; //!<Subpixel offset X, 1.0f equals one pixel
	Float			m_subPixelOffsetY; //!<Subpixel offset Y, 1.0f equals one pixel

	SRenderCameraLastFrameData	m_lastFrameData; 

public:
	CRenderCamera();
	CRenderCamera( const Vector& position, const EulerAngles& rotation, Float fov, Float aspect, Float nearPlane, Float farPlane, Float zoom=1.0f, Bool isReversedProjection=false, const SRenderCameraLastFrameData& lastFrameData = SRenderCameraLastFrameData::INVALID );
	CRenderCamera( const CRenderCamera& camera );

	void operator =  ( const CRenderCamera& camera );

	// Get attributes
	RED_INLINE Float GetFOV() const { return m_fov; }
	RED_INLINE Float GetFOVMultiplier() const { return m_fovMultiplier; }
	RED_INLINE Float GetFOVMultiplierUnclamped() const { return m_fovMultiplierUnclamped; }
	RED_INLINE Float GetAspect() const { return m_aspect; }
	RED_INLINE Float GetNearPlane() const { return m_nearPlane; }
	RED_INLINE Float GetFarPlane() const { return m_farPlane; }
	RED_INLINE void SetNearPlane( Float val ) { m_nearPlane = val; }
	RED_INLINE void SetFarPlane( Float val ) { m_farPlane = val; }
	RED_INLINE Float GetZoom() const { return m_zoom; }
	RED_INLINE Float GetVisibleDetailMultiplier() const { return m_visDetailMul; }
	RED_INLINE Float GetNearCornerDistance() const { return m_nearCornerDist; }
	RED_INLINE const Vector& GetPosition() const { return m_position; }
	RED_INLINE const EulerAngles& GetRotation() const { return m_rotation; }
	RED_INLINE const Matrix& GetWorldToView() const { return m_worldToView; }
	RED_INLINE const Matrix& GetWorldToCamera() const { return m_worldToCamera; }
	RED_INLINE const Matrix& GetCameraToWorld() const { return m_cameraToWorld; }
	RED_INLINE const Matrix& GetViewToWorld() const { return m_viewToWorld; }
	RED_INLINE const Matrix& GetScreenToView() const { return m_screenToView; }
	RED_INLINE const Matrix& GetViewToScreen() const { return m_viewToScreen; }
	RED_INLINE const Matrix& GetWorldToScreen() const { return m_worldToScreen; }
	RED_INLINE const Matrix& GetScreenToWorld() const { return m_screenToWorld; }
	RED_INLINE const Vector& GetCameraVector( Int32 index ) const { return m_cameraVectors[ index ]; }
	RED_INLINE const Vector& GetCameraRight() const { return m_cameraVectors[ CV_Right ]; }
	RED_INLINE const Vector& GetCameraForward() const { return m_cameraVectors[ CV_Forward ]; }
	RED_INLINE const Vector& GetCameraUp() const { return m_cameraVectors[ CV_Up ]; }
	RED_INLINE Bool IsOrtho() const { return m_fov == 0.0f; }
	RED_INLINE Bool IsPerspective() const { return m_fov != 0.0f; }
	RED_INLINE const SRenderCameraLastFrameData& GetLastFrameData() const { return m_lastFrameData; }
	RED_INLINE void SetLastFrameData( const SRenderCameraLastFrameData& data ) { m_lastFrameData = data; }
	RED_INLINE Bool IsReversedProjection() const { return m_isReversedProjection; }
	RED_INLINE const Matrix& GetWorldToScreenRevProjAware() const { return m_worldToScreenRevProjAware; }
	RED_INLINE const Matrix& GetViewToScreenRevProjAware() const { return m_viewToScreenRevProjAware; }
	RED_INLINE const Matrix& GetScreenToWorldRevProjAware() const { return m_screenToWorldRevProjAware; }
	// Not so eficient as we want to know only the postprojected depth value
	RED_INLINE Float ProjectDepth( float distanceFromNearPlane ) const
	{
		const Vector pos = GetCameraForward() * distanceFromNearPlane + GetPosition();
		const Vector vsp = GetWorldToScreen().TransformVectorWithW( pos );
		return vsp.Z / vsp.W;
	}


	// Get corners of the frustum for given plane
	void GetFrustumCorners( const Float plane, Vector* corners, Bool localCorners = false ) const;

	// Set all attributes
	void Set( const Vector& position, const EulerAngles& rotation, Float fov, Float aspect, Float nearPlane, Float farPlane, Float zoom=1.0f, Bool isReversedProjection=false );
	void SetRotation( const EulerAngles& rotation );
	void SetReversedProjection( Bool isReversed );
	void SetFOV( Float fov );

	void SetSubpixelOffset( Float x, Float y, Uint32 screenW, Uint32 screenH );
	Float GetSubpixelOffsetX() const { return m_subPixelOffsetX; };
	Float GetSubpixelOffsetY() const { return m_subPixelOffsetY; };

public:
	RED_INLINE void SetNonDefaultFarRenderingPlane(){ m_nonDefaultFarPlane = true; }
	RED_INLINE void SetNonDefaultNearRenderingPlane(){ m_nonDefaultNearPlane = true; }
	RED_INLINE Bool GetNonDefaultFarRenderingPlane() const { return m_nonDefaultFarPlane; }
	RED_INLINE Bool GetNonDefaultNearRenderingPlane() const { return m_nonDefaultNearPlane; }
	void CalculateMatrices();

	Bool CameraInvalidateDifference( const class CRenderCamera& camera ) const;

	// Extracted world to screen texture (0-1 normalized space) computation for shadowmaps
	static Matrix CalcWorldToTexture( const Vector& position, const EulerAngles& rotation, Float fov, Float aspect, Float nearPlane, Float farPlane, Float zoom=1.0f );

	// Calculate camera vectors for given rotation
	static void CalcCameraVectors( const EulerAngles &rotation, Vector &outCameraForward, Vector &outCameraRight, Vector &outCameraUp );

	// Calculate and return frustum planes of current camera
	void CalculateFrustumPlanes( Vector* outFrustumPlanes ) const;

protected:
	void CalculateVisibleDetailMultiplier();
	void CalculateNearPlaneCornerDist();
	void CalculateRevProjAwareMatrices();
	void CalculateFOVMultiplier();
};
