/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

///////////////////////////////////////////////////////////////////////////////

class IMovableRepresentation : public Red::System::NonCopyable
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_AI );
public:
	virtual ~IMovableRepresentation() {}

	virtual void OnInit( const Vector& position, const EulerAngles& orientation ) = 0;

	virtual void OnActivate( const Vector& position, const EulerAngles& orientation ) = 0;

	virtual void OnDeactivate() = 0;

	virtual void OnMove( const Vector& deltaPosition, const EulerAngles& deltaOrientation ) = 0;

	virtual void OnSeparate( const Vector& deltaPosition ) = 0;

	// timeDelta here may be needed for budgeted updates
	virtual void OnSetPlacement( Float timeDelta, const Vector& newPosition, const EulerAngles& newOrientation, const Bool correctZ = false ) = 0;

	//unlike OnSetPlacement does not need to be Update() after calling
	virtual void OnFollowPlacement( Float timeDelta, const Vector& newPosition, const EulerAngles& newOrientation )
	{
		//works the same in most cases
		OnSetPlacement( timeDelta, newPosition, newOrientation );
	}

	virtual Vector GetRepresentationPosition( Bool smooth = false ) const = 0;

	virtual EulerAngles GetRepresentationOrientation() const = 0;

	virtual CName GetName() const = 0;

	virtual Bool IsAlwaysActive() const = 0;

	virtual void Update1_PreSeparation( Float timeDelta ) {};
    virtual Bool Update2_PostSeparation( Float timeDelta, Bool continousMovement ) { return true; }; // return true = object was moved

	virtual class CPhysicsCharacterWrapper* GetCharacterController() const { return 0; }

protected:
	//! A helper method that changes the movement delta so that we can avoid colliding with other agents.
	Vector CheckCollisions( CMovingAgentComponent& host, const Vector& deltaPosition ) const;

	//! Queries the moving agent components in the vicinity
	void QueryMovingAgents( const Vector& queriedPos, Float queryRad, TDynArray< CMovingAgentComponent* >& outAgents, Bool queryAliveOnly = false ) const;
};

///////////////////////////////////////////////////////////////////////////////
