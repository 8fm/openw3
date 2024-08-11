#pragma once

#include "customCameraControllers.h"
#include "..\..\common\engine\cameraFadeoutHelper.h"

class ICustomCameraCollisionController : public ICustomCameraBaseController
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ICustomCameraCollisionController, ICustomCameraBaseController )

protected:
	struct CollosionPoint
	{
		Vector	m_point;
		Float	m_penetration;

		CollosionPoint() : m_point( Vector::ZERO_3D_POINT ), m_penetration( 0.f ) {}
		CollosionPoint( const Vector& point, Float penetration ) : m_point( point ), m_penetration( penetration ) {}

		void Reset() { m_point = Vector::ZERO_3D_POINT; m_penetration = 0.f; }
	};

	struct FadeOutCameraOccludablesOp : public ICameraFadeOutOp
	{
		virtual void operator()( CPhysicsWrapperInterface* physicsInterface, const SActorShapeIndex* actorIndex ) const override;
	};

	struct FadeOutPlayerWeaponsOp : public ICameraFadeOutOp
	{
		virtual void operator()( CPhysicsWrapperInterface* physicsInterface, const SActorShapeIndex* actorIndex ) const override;
	};

	Vector m_originOffset;

public:
	virtual void Activate( const SCameraMovementData& data ) = 0;

	virtual void Update( SCameraMovementData& data, Float timeDelta ) = 0;

	virtual void Force( SCameraMovementData& data ) {};

	virtual Vector		GetPosition() = 0;
	virtual EulerAngles	GetRotation() = 0;

	RED_INLINE void SetColisionOriginOffset( const Vector& offset ) { m_originOffset = offset; }

	virtual void GenerateDebugFragments( CRenderFrame* frame ) {};

	virtual void Reset() {};

protected:
	static Bool SweepCheck( Float radius, const Vector& start, const Vector& end );
	static Bool Sweep( Float radius, const Vector& start, const Vector& end, Float& t, Vector& hitPosition );
	static Bool CheckSphereOverlap( const Vector& position, Float radius, TDynArray< CollosionPoint >* contacts );
	static Bool CheckPlayerOverlap( const Vector& position, Float radius );
	static void FadeOutCameraOccludables( Float radius, const Vector& start, const Vector& end );
	static void FadeOutPlayerWeapons( SCameraMovementData& data );
	static void FadeOutPlayer( SCameraMovementData& data, const Vector& cameraPosition, const Vector& cameraLookAtPosition, const Float sweepRadius, const Float collisionVolumePadding );
};

BEGIN_ABSTRACT_CLASS_RTTI( ICustomCameraCollisionController )
	PARENT_CLASS( ICustomCameraBaseController )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CCustomCameraOutdoorCollisionController : public ICustomCameraCollisionController
{
	DECLARE_ENGINE_CLASS( CCustomCameraOutdoorCollisionController, ICustomCameraCollisionController, 0 )

private:
	Float							m_distanceDmpFactor;
	TFloatCriticalDampRef			m_distanceDamp;
	TFloatCriticalDampRef			m_smallSphereDistDamp;

	enum ECameraCollisionState
	{
		ECCS_Initializing = 0,
		ECCS_Unobstructed,
		ECCS_SlidingOnCollision
	} m_state;

	//////////////////////////////////////////////////////////////////////////

	Vector		m_finalPosition;
	EulerAngles	m_finalRotation;

	// Collision info
	Vector	m_sweepPosition;

	Float	m_sweepRadius;
	Float	m_collisionOffset;

	Bool	m_reached;
	Bool	m_usingSmallSphere;
	Bool	m_isInMotion;

public:
	CCustomCameraOutdoorCollisionController();
	~CCustomCameraOutdoorCollisionController();

	virtual void Activate( const SCameraMovementData& data );

	virtual void Update( SCameraMovementData& data, Float timeDelta );

	RED_INLINE virtual Vector GetPosition() { return m_finalPosition; }
	RED_INLINE virtual EulerAngles GetRotation() { return m_finalRotation; }

	virtual void GenerateDebugFragments( CRenderFrame* frame );

	 void Reset() override;

private:
	void UpdateCollision( const Vector& sweepStartPosition, const Vector& lookAtPosition, const Vector& desiredPosition, Float ratio, Float timeDelta );

	// We need to test this only if predicted sweep doesn't hit
	Bool VerifyPredictedPositionAndUpdateIfNeeded( const Vector& sweepStartPosition, const Vector& lookAtPosition, const Vector& desiredPosition );
};

BEGIN_CLASS_RTTI( CCustomCameraOutdoorCollisionController )
	PARENT_CLASS( ICustomCameraCollisionController )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CCustomCameraAutoAvoidanceCollisionController : public ICustomCameraCollisionController
{
	DECLARE_ENGINE_CLASS( CCustomCameraAutoAvoidanceCollisionController, ICustomCameraCollisionController, 0 )

private:
	Float						m_angleDmpFactor;
	TEulerAnglesCriticalDampRef	m_angleDamp;

	CCurve*		m_catchUpCurve;
	Float		m_timer;

	Float		m_criticalCollisionSquaredDistance;

	struct TEST_SPHERE
	{
		Vector	m_position;
		Float	m_penetration;
		Float	m_radius;

		TEST_SPHERE( const Vector& position, Float penetration, Float radius )
			: m_position( position ), m_penetration( penetration ), m_radius( radius ) {}
	};

	enum ECameraCollisionState
	{
		ECCS_Initializing = 0,
		ECCS_Unobstructed,
		ECCS_SlidingOnCollision
	} m_state;

	//////////////////////////////////////////////////////////////////////////

	Vector		m_finalPosition;
	EulerAngles	m_finalRotation;

	// Collision info
	Float	m_angleStep;
	Float	m_sweapRadius;
	Float	m_smallOverlapRadius;
	Float	m_bigOverlapRadius;
	Float	m_collisionOffset;

	EulerAngles*	m_calculatedRot;
	Bool			m_calculatedRight;

	// Debug
	TDynArray<TEST_SPHERE>	m_dbgSpheres;

public:
	CCustomCameraAutoAvoidanceCollisionController();
	~CCustomCameraAutoAvoidanceCollisionController();

	virtual void OnPostLoad();

	virtual void Activate( const SCameraMovementData& data );

	virtual void Update( SCameraMovementData& data, Float timeDelta );
	virtual void Force( SCameraMovementData& data );

	RED_INLINE virtual Vector GetPosition() { return m_finalPosition; }
	RED_INLINE virtual EulerAngles GetRotation() { return m_finalRotation; }

	virtual void GenerateDebugFragments( CRenderFrame* frame );

private:
	void UpdateLookAt( SCameraMovementData& data, const Vector& criticalPosition, Float timeDelta );
	void UpdateCollision( SCameraMovementData& data, const Vector& playerPosition, const Vector& desiredPosition, Float ratio, Float timeDelta );
	Bool CollectCollisionInfo( const Vector& start, const Vector& end, Float& outLeftPenetration, Float& outRightPenetration, Float& outBottomPenetration );
	Bool CollectOneSidedSoftSphereInfo( const Vector& start, const Vector& end, Bool leftSide, Float& outPenetration );
	Bool CheckSpherePenetration( const Vector& position, Vector& normal, Float& penetration );
};

BEGIN_CLASS_RTTI( CCustomCameraAutoAvoidanceCollisionController )
	PARENT_CLASS( ICustomCameraCollisionController )
	PROPERTY_CUSTOM_EDIT( m_catchUpCurve, TXT(""), TXT("CurveSelection") )
END_CLASS_RTTI()