/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "cameraDirector.h"
#include "spriteComponent.h"

enum ENearPlaneDistance : CEnum::TValueType
{
	NP_VeryClose5cm,
	NP_Close10cm,
	NP_DefaultEnv,
	NP_Medium25cm,
	NP_Further40cm,
	NP_Far60cm,
	NP_VeryFar120cm,
	NP_Extreme240cm,
	NP_CustomValue,
};

BEGIN_ENUM_RTTI( ENearPlaneDistance );
	ENUM_OPTION( NP_VeryClose5cm );
	ENUM_OPTION( NP_Close10cm );
	ENUM_OPTION( NP_DefaultEnv );
	ENUM_OPTION( NP_Medium25cm );
	ENUM_OPTION( NP_Further40cm );
	ENUM_OPTION( NP_Far60cm );
	ENUM_OPTION( NP_VeryFar120cm );
	ENUM_OPTION( NP_Extreme240cm );
	ENUM_OPTION( NP_CustomValue );
END_ENUM_RTTI();

enum EFarPlaneDistance : CEnum::TValueType
{
	FP_CrazyClose20m,
	FP_EvenCloser30m,
	FP_VeryClose40m,
	FP_Close150m,
	FP_Medium600m,
	FP_DefaultEnv,
	FP_Far1000m,
	FP_MoreFar1500m,
	FP_VeryFar2000m,
	FP_CustomValue,
};

BEGIN_ENUM_RTTI( EFarPlaneDistance );
	ENUM_OPTION( FP_CrazyClose20m );
	ENUM_OPTION( FP_EvenCloser30m );
	ENUM_OPTION( FP_VeryClose40m );
	ENUM_OPTION( FP_Close150m );
	ENUM_OPTION( FP_Medium600m );
	ENUM_OPTION( FP_DefaultEnv );
	ENUM_OPTION( FP_Far1000m );
	ENUM_OPTION( FP_MoreFar1500m );
	ENUM_OPTION( FP_VeryFar2000m );
	ENUM_OPTION( FP_CustomValue );
END_ENUM_RTTI();

struct SCustomClippingPlanes
{
	DECLARE_RTTI_STRUCT( SCustomClippingPlanes );

	SCustomClippingPlanes()
		: m_nearPlaneDistance( 0.4f )
		, m_farPlaneDistance( 8000.0f )
	{
		/* intentionally empty */
	}

	SCustomClippingPlanes( Float near, Float far )
		: m_nearPlaneDistance( near )
		, m_farPlaneDistance( far )
	{
		/* intentionally empty */
	}

	Float m_nearPlaneDistance;
	Float m_farPlaneDistance;
};

BEGIN_CLASS_RTTI( SCustomClippingPlanes )
	PROPERTY_EDIT_RANGE( m_nearPlaneDistance, TXT("Near plane distance"), 0.1f, 8000.0f );
	PROPERTY_EDIT_RANGE( m_farPlaneDistance, TXT("Far plane distance"), 0.1f, 8000.0f );
END_CLASS_RTTI();

/// In game camera
class CCameraComponent : public CSpriteComponent, public ICamera
{
	DECLARE_ENGINE_CLASS( CCameraComponent, CSpriteComponent, 0 );

protected:
	Float					m_aspect;
	Bool					m_lockAspect;
	Float					m_fov; 
	ENearPlaneDistance		m_nearPlane;
	EFarPlaneDistance		m_farPlane;
	SCustomClippingPlanes	m_customClippingPlanes;
	Bool					m_defaultCamera;
	SDofParams				m_dofParam;
	SBokehDofParams			m_bokehDofParam;

	// for interaction with trigger system
	Bool					m_teleported;

	SRenderCameraLastFrameData	m_lastFrameCamera;
	
public:
	RED_INLINE void SetFov( Float fov ) { m_fov = fov; }
	RED_INLINE Float GetFov() const { return m_fov; }

	RED_INLINE const SDofParams& GetDofParams() const { return m_dofParam; }
	RED_INLINE void SetDofParams( SDofParams param ) { m_dofParam = param; }

	RED_INLINE const SBokehDofParams& GetBokehDofParams() const { return m_bokehDofParam; }
	RED_INLINE void SetBokehDofParams( const SBokehDofParams& param ) { m_bokehDofParam = param; }

	RED_INLINE void SetNearPlane( ENearPlaneDistance nearPlane ) { m_nearPlane = nearPlane; };
	RED_INLINE ENearPlaneDistance GetNearPlane() { return m_nearPlane; };
	
	RED_INLINE void SetFarPlane( EFarPlaneDistance farPlane ) { m_farPlane = farPlane; };
	RED_INLINE EFarPlaneDistance GetFarPlane() { return m_farPlane; };

	RED_INLINE void SetCustomClippingPlanes( const SCustomClippingPlanes& planes ) { m_customClippingPlanes = planes; };
	RED_INLINE SCustomClippingPlanes GetCustomClippingPlanes() { return m_customClippingPlanes; };

	RED_INLINE void SetAspect( Float aspect ) { m_aspect = aspect; };
	RED_INLINE Float GetAspect() { return m_aspect; };

	RED_INLINE void InvalidateLastFrameCamera() { m_lastFrameCamera = SRenderCameraLastFrameData::INVALID; }

public:
	CCameraComponent();

	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );

	// Called when component is detached from world ( layer gets hidden, etc )
	virtual void OnDetached( CWorld* world );

	// Setup camera
	virtual void OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera );

	// Generate editor fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );

public:
	// Activate camera
	void SetActive( Bool state );

	// Is default camera
	Bool IsDefaultCamera() const { return m_defaultCamera; }

	void FlagTeleported() { m_teleported = true; }

	void ViewCoordsToWorldVector( Int32 x, Int32 y, Vector & outRayStart, Vector & outRayDirection ) const;
	void WorldVectorToViewCoords( Vector & worldPos, Int32& x, Int32& y ) const;
	Bool WorldVectorToViewRatio( const Vector& worldPos, Float& x, Float& y ) const;
	Bool TestWorldVectorToViewRatio( const Vector& worldPos, Float& x, Float& y, const Vector& cameraPos, const EulerAngles& cameraRot ) const;

	Bool IsPointInView( const Vector& point, Float fowMultipier = 1.0f ) const;

	RED_INLINE Float Map( ENearPlaneDistance distance ) const;
	RED_INLINE Float Map( EFarPlaneDistance distance ) const;

	Vector ProjectPoint( const Vector& worldSpacePoint );
	Vector UnprojectPoint( const Vector& screenSpacePoint );

	virtual void LogicInitialize		( )							{};
	virtual void LogicStartEntering		( Float timeToComplete )	{};
	virtual void LogicCompleteEntering	( )							{};
	virtual void LogicStartLeaving		( Float timeToComplete )	{};
	virtual void LogicCompleteLeaving	( )							{};
	virtual void LogicUpdate			( float _Dt )				{};

	// ICamera implementation
	virtual Bool Update( Float timeDelta ) override { return true; }

	virtual Bool GetData( Data& outData ) const override;

public:
	// Get sprite icon
	virtual CBitmapTexture* GetSpriteIcon() const;

};

Float CCameraComponent::Map( ENearPlaneDistance distance ) const 
{
	switch ( distance )
	{
	case NP_VeryClose5cm:
		return 0.05f;
	case NP_Close10cm:
		return 0.1f;
	case NP_DefaultEnv:
		return 0.25f;
	case NP_Medium25cm:
		return 0.25f;
	case NP_Further40cm:
		return 0.4f;
	case NP_Far60cm:
		return 0.6f;
	case NP_VeryFar120cm:
		return 1.2f;
	case NP_Extreme240cm:
		return 2.4f;
	case NP_CustomValue:
		return m_customClippingPlanes.m_nearPlaneDistance;
	}
	return 0.25f;
}

Float CCameraComponent::Map( EFarPlaneDistance distance ) const 
{
	switch ( distance )
	{
	case FP_CrazyClose20m:
		return 20.0f;
	case FP_EvenCloser30m:
		return 30.0f;
	case FP_VeryClose40m:
		return 40.0f;
	case FP_Close150m:
		return 150.0f;
	case FP_Medium600m:
		return 600.0f;
	case FP_DefaultEnv:
		return 1000.0f;
	case FP_Far1000m:
		return 1000.0f;
	case FP_MoreFar1500m:
		return 1500.0f;
	case FP_VeryFar2000m:
		return 2000.0f;
	case FP_CustomValue:
		return m_customClippingPlanes.m_farPlaneDistance;
	}
	return 1000.0f;
}



BEGIN_CLASS_RTTI( CCameraComponent );
	PARENT_CLASS( CSpriteComponent );
	PROPERTY_EDIT( m_fov, TXT("FOV") );
	PROPERTY_EDIT( m_nearPlane, TXT("Near Plane") );
	PROPERTY_EDIT( m_farPlane, TXT("Far Plane") );
	PROPERTY_INLINED( m_customClippingPlanes, TXT("Custom clipping planes") );
	PROPERTY_EDIT( m_aspect, TXT("Camera aspect ratio") );
	PROPERTY_EDIT( m_lockAspect, TXT("Lock camera aspect ratio") );
	PROPERTY_EDIT( m_defaultCamera, TXT("Choose as default camera") );
END_CLASS_RTTI();


