/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../core/hashset.h"

// Contains information about a single active entity motion
class CEntityMotion
{
	friend class CEntityMotionManager;

protected:
	class CEntityMotionManager*		m_motionManager;	//!< The motion manager this entity applies to
	Vector							m_initialPosition;	//!< The entity's initial position
	EulerAngles						m_initialRotation;	//!< The entity's initial rotation
	Vector							m_initialScale;		//!< The entity's initial scale
	class CEntity*					m_entity;			//!< The entity this motion applies to (set by the manager)
	THandle< CEntity >				m_entityHandle;		//!< Handle for the entity (used to make sure the entity is still there)
	Float							m_startTime;		//!< Motion start time
	Float							m_endTime;			//!< Motion end time
	Float							m_time;				//!< Normalized time for this motion (0=start, 1=end)

protected:
	// Constructor
	CEntityMotion(){}
	virtual ~CEntityMotion(){}

	// Called when the garbage collector is marking reachable objects
	virtual void SerializeForGC( IFile& file );

	// Called to initialize the motion
	virtual void OnStart();

	// Called to finalize the motion
	virtual void OnEnd();

	// Called to update the motion for the current normalized time
	virtual void UpdateMotion();
	
	// Called to tick the motion - the default method updates m_time and calls UpdateMotion
	virtual void Tick( Float timeDelta );

	// Request this motion to be destroyed (called by the default Tick method once the time is reaced)
	void RequestDestroy();
};

// The entity motion manager
class CEntityMotionManager
{
	friend class CWorld;
	friend class CEntityMotion;

	class CWorld*				m_world;				//!< The world this motion manager belongs to
	THashSet< CEntityMotion* >	m_motions;				//!< Active motions
	TDynArray< CEntityMotion* > m_toDestroy;			//!< Motions to destroy
	Float						m_motionTime;			//!< Running motion time

protected:
	// Called when the garbage collector is marking reachable objects
	virtual void SerializeForGC( IFile& file );

	// Tick the entity motion manager
	virtual void Tick( Float timeDelta );

	// Requests to destroy this motion (called by CEntityMotion::RequestDestroy)
	void RequestDestroy( CEntityMotion* motion );

	// Destroys all the motions that requested to be destroyed with RequestDestroy above
	void DestroyMotions();

public:
	// Returns the world this manager belongs to
	RED_INLINE class CWorld* GetWorld() const { return m_world; }

	// Returns the motion manager's running time
	RED_INLINE Float GetMotionTime() const { return m_motionTime; }

public:
	CEntityMotionManager( class CWorld* world );
	virtual ~CEntityMotionManager();

	// Adds the passed motion to this motion manager to run for the given entity and duration
	// The motion will be automatically deleted once the duration ends
	void AddMotion( CEntityMotion* motion, CEntity* entity, Float duration );

	// Returns true if the passed entity motion is still running
	Bool IsRunning( CEntityMotion* motion ) const;
};
