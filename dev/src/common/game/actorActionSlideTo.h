/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "actorAction.h"
#include "moveLocomotion.h"
#include "moveSlideLocomotionSegment.h"


// A simple sliding movement action - no path finding involved
class ActorActionSlideTo : public ActorAction, public CMoveLocomotion::IController
{
private:
	EActorActionResult					m_status;

public:
	ActorActionSlideTo( CActor* actor, EActorActionType type = ActorAction_Sliding );
	~ActorActionSlideTo();

	//! Start slide movement, no heading, duration is controlled
	Bool StartSlide( const Vector& target, Float duration );

	//! Start slide movement with end heading, duration is controlled
	Bool StartSlide( const Vector& target, Float heading, Float duration, ESlideRotation rotation );

	//! Start moving using a custom targeter
	Bool StartCustomMove( IMovementTargeter* targeter );

	//! Stop movement
	virtual void Stop();

	//! Update
	virtual Bool Update( Float timeDelta );

	//! Tells if a look-at can be used with the current action
	virtual Bool CanUseLookAt() const { return true; }

	//! Called when an actor is being pushed by another actor
	virtual void OnBeingPushed( const Vector& direction, Float rotation, Float speed, EPushingDirection animDirection ) {}

	// -------------------------------------------------------------------------
	// CMoveLocomotion::IController
	// -------------------------------------------------------------------------
	virtual void OnSegmentFinished( EMoveStatus status );
	virtual void OnControllerAttached();
	virtual void OnControllerDetached();

protected:
	//! Get slide duration
	Float GetSlideDuration( Float dist ) const;

	//! Reset moving agent data
	void ResetAgentMovementData();

	//! Sets the specified movement type and speed of the controlled agent
	void SetMoveSpeed( CMovingAgentComponent& mac, EMoveType moveType, Float absSpeed ) const;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include "animationSlider.h"
#include "../../common/engine/animationTrajectorySync.h"

struct SAnimatedSlideSettings
{
	DECLARE_RTTI_STRUCT( SAnimatedSlideSettings );

	CName	m_animation;
	CName	m_slotName;
	Float	m_blendIn;
	Float	m_blendOut;
	Bool	m_useGameTimeScale;
	Bool	m_useRotationDeltaPolicy;

	SAnimatedSlideSettings() : m_blendIn( 0.f ), m_blendOut( 0.f ), m_useGameTimeScale( true ), m_useRotationDeltaPolicy( false ) {}
};

BEGIN_CLASS_RTTI( SAnimatedSlideSettings );
	PROPERTY_EDIT( m_animation, String::EMPTY );
	PROPERTY_EDIT( m_slotName, String::EMPTY );
	PROPERTY_EDIT( m_blendIn, String::EMPTY );
	PROPERTY_EDIT( m_blendOut, String::EMPTY );
	PROPERTY_EDIT( m_useGameTimeScale, String::EMPTY );
	PROPERTY_EDIT( m_useRotationDeltaPolicy, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CMoveLSAnimationSlide;

class ActorActionAnimatedSlideTo : public ActorAction, public CMoveLocomotion::IController
{
private:
	EActorActionResult						m_status;
	THandle< CActionMoveAnimationProxy >	m_proxy;

public:
	ActorActionAnimatedSlideTo( CActor* actor );

	Bool StartSlide( const SAnimatedSlideSettings& settings, const SAnimSliderTarget& target );
	Bool StartSlide( const SAnimatedSlideSettings& settings, const SAnimSliderTarget& target, THandle< CActionMoveAnimationProxy >& proxy );

	virtual void Stop();
	virtual Bool Update( Float timeDelta );

	virtual Bool CanUseLookAt() const { return true; }
	virtual void OnBeingPushed( const Vector& direction, Float rotation, Float speed, EPushingDirection animDirection ) {}

	virtual void OnGCSerialize( IFile& file );

	// -------------------------------------------------------------------------
	// CMoveLocomotion::IController
	// -------------------------------------------------------------------------
	virtual void OnSegmentFinished( EMoveStatus status );
	virtual void OnControllerAttached();
	virtual void OnControllerDetached();

protected:
	void ResetAgentMovementData();
};
