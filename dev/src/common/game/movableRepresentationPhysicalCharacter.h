/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "movableRepresentation.h"

class CPhysicsCharacterWrapper;
///////////////////////////////////////////////////////////////////////////////
class CMRPhysicalCharacter : public IMovableRepresentation
{
protected:
	CMovingPhysicalAgentComponent&			m_host;

	CPhysicsCharacterWrapper*				m_characterController;
	EulerAngles								m_actualRotation;

    Vector                                  m_prevPosition;
	Vector									m_requestedMovementDelta;

	Bool DoTraceZTest( const Vector& pointWS, Vector& outPosition ) const;

public:
	CMRPhysicalCharacter( CMovingPhysicalAgentComponent& host );
	virtual ~CMRPhysicalCharacter();

	//! Set susceptibility to gravity
	void SetGravity( Bool enable );
	Bool IsGravity() const;

	// enable/disable physical movement
	void EnablePhysicalMovement( Bool b );
	Bool IsPhysicalMovementEnabled() const;

	//! Set need for behavior callback (horse doesnt like it)
	void SetNeedsBehaviorCallback( Bool enable );
	Bool IsBehaviorCallbackNeeded() const;

	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );

	Bool GetCollisionControllerExtents( AACylinder& outCylinder, const Vector& worldPosition ) const;
	Bool GetCollisionControllerExtents( AACylinder& outCylinder ) const;

	Float GetCombatRadius() const;
	Float GetCurrentHeight() const;

	// is falling
	Bool IsFalling() const;
	
	//! swimming/diving
	void SetSwimming( Bool enable );
	void SetDiving( Bool diving );
	Bool IsDiving() const;
	Float GetWaterLevel();
	void SetEmergeSpeed( Float value );
	Float GetEmergeSpeed() const;
	void SetSubmergeSpeed( Float value );
	Float GetSubmergeSpeed() const;
	void SetRagdollToSwimming( Bool enable );

	// sliding data
	Bool IsSliding() const;
	Float GetSlideCoef() const;
	Vector GetSlidingDir() const;

    //! Force the character to move to the position
    void ForceMoveToPosition( const Vector& position, Bool resetZAxis );

	//! Apply velocity to physical character
	virtual void ApplyVelocity( const Vector& vel );

	//! Register listener that will receive physical events
	virtual void RegisterEventListener( const THandle<IScriptable>& listener );

	//! Unregister listener
	virtual void UnregisterEventListener( const THandle<IScriptable>& listener );

//	RED_INLINE void SnapToNavigableSpace( Bool b ) { m_snapToNavigableSpace = b; }

	RED_INLINE virtual Float GetSlopePitch() const { return m_actualRotation.Pitch; }

	// ------------------------------------------------------------------------
	// IMovableRepresentation implementation
	// ------------------------------------------------------------------------
	virtual void OnInit( const Vector& position, const EulerAngles& orientation );

	void OnActivate( const Vector& position, const EulerAngles& orientation );

	void OnDeactivate();

	virtual void OnMove( const Vector& deltaPosition, const EulerAngles& deltaOrientation ) override;
	
	virtual void OnSeparate( const Vector& deltaPosition ) override;

	virtual void OnSetPlacement( Float timeDelta, const Vector& newPosition, const EulerAngles& newOrientation, const Bool correctZ ) override;

	virtual void OnFollowPlacement( Float timeDelta, const Vector& newPosition, const EulerAngles& newOrientation ) override;

	virtual void Update1_PreSeparation( Float timeDelta );
    virtual Bool Update2_PostSeparation( Float timeDelta, Bool continousMovement ) override;

	virtual Vector GetRepresentationPosition( Bool smooth = false ) const;

	virtual Vector GetRepresentationPosition( Int32 index) const;

	virtual Int32 GetNumOfRepresentations() const { return 1; }

	virtual EulerAngles GetRepresentationOrientation() const;

	// For debug purposes
	virtual CName GetName() const;

	RED_INLINE virtual Bool IsAlwaysActive() const { return false; }

	RED_INLINE virtual Bool AlwaysTestDeltaTransform() const { return true; }

	virtual CPhysicsCharacterWrapper* GetCharacterController() const { return m_characterController; }

	virtual void DestroyCharacterController();

	/// @see CPhysicsCharacterWrapper::DownCollision
	Bool DownCollision() const;

	const CCharacterControllerParam* GetCharacterControllerParam();


protected:
	virtual void SpawnCharacterController( const Vector& position, const EulerAngles& orientation );

private:
	void ForceUpdatePhysicalContextPosition( const Vector2& position );
};
///////////////////////////////////////////////////////////////////////////////
