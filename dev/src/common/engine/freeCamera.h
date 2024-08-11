/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

enum EInputKey : Int32;
enum EInputAction : Int32;
class IViewport;
class CRenderFrame;
class CRenderCamera;

/// Free camera that allows to view the world without the active game
class CGameFreeCamera
{
protected:
	Vector			m_position;
	EulerAngles		m_rotation;
	Vector			m_visiblePosition;
	EulerAngles		m_visibleRotation;
	Float			m_moveUpDown;
	Float			m_moveForwardBackward;
	Float			m_moveLeftRight;
	Float			m_lookUpDown;
	Float			m_lookLeftRight;
	Float			m_cameraRoll;
	Float			m_speed;
	Float			m_fov;
	Float			m_fovDistanceMultiplier;
	Float			m_dofMultiplier;
	Float			m_dofFarOffset;
	Float			m_dofNearOffset;
	Bool			m_skipInputs;
	Bool			m_attachedToPlayer;
	Vector			m_attachOffset;

	CRenderCamera	m_cachedCamera;
	SRenderCameraLastFrameData	m_lastFreeCameraData;

public:
	//! Get camera position
	RED_INLINE const Vector& GetPosition() const { return m_position; }

	//! Get camera rotation
	RED_INLINE const EulerAngles& GetRotation() const { return m_rotation; }

	//! Get camera direction
	RED_INLINE Vector GetDirection() const { Vector dir; m_rotation.ToAngleVectors( &dir, NULL, NULL ); return dir; }

	//! Get camera FOV
	RED_INLINE Float GetFOV() const { return m_fov; }

	//! Set camera FOV
	RED_INLINE void SetFOV( Float fov ) { m_fov = fov; CacheFovDistanceMultiplier(); }

	//! Get camera fov distance multiplier
	RED_INLINE Float GetFovDistanceMultiplier() const { return m_fovDistanceMultiplier; }

public:
	CGameFreeCamera();
	~CGameFreeCamera();

	//! Move to target position and rotation
	void MoveTo( const Vector& position, const EulerAngles& rotation );

	//! Process input
	Bool ProcessInput( EInputKey key, EInputAction action, Float data );

	//! Update camera
	void Tick( Float timeDelta );

	//! Override camera
	void CalculateCamera( CRenderCamera &camera ) const;

	//! Override frame info parameters
	void ConfigureFrameInfo( CRenderFrameInfo& info ) const;

	//! Can process inputs
	Bool CanProcessedInputs() const;

public:
	void Reset();

private:
	void CacheFovDistanceMultiplier();
};
