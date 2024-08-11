/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CActor;

/// Action type, defined as flags because it's sometimes used as mask
enum EActorActionType
{
	ActorAction_None				= 0,
	ActorAction_Moving				= FLAG( 0 ),	//!< Actor is on the move
	ActorAction_Rotating			= FLAG( 1 ),	//!< Actor is rotating in place
	ActorAction_Animation			= FLAG( 2 ),	//!< Actor is playing animation
	ActorAction_RaiseEvent			= FLAG( 3 ),	//!< Actor is rising behavior event
	ActorAction_Sliding				= FLAG( 4 ),	//!< Actor is sliding
	ActorAction_Working				= FLAG( 5 ),	//<! Actor is working
	ActorAction_ChangeEmotion		= FLAG( 6 ),	//<! Actor is changing an emotional state
	ActorAction_Exploration			= FLAG( 7 ),	//<! Actor is traversing an exploration area
	ActorAction_UseDevice			= FLAG( 8 ),	//<! Actor is using device
	ActorAction_DynamicMoving		= FLAG( 9 ),	//<! Actor is moving to a dynamically moving node
	ActorAction_MovingOnCurve		= FLAG( 10 ),	//<! Actor is moving on a curve
	ActorAction_CustomSteer			= FLAG( 11 ),	//<! Actor is moving with custom params
	ActorAction_R4Reserved_PC		= FLAG( 12 ),	//<! R4 project player action
	ActorAction_MovingOutNavdata	= FLAG( 13 ),	//<! Actor is moving without being on navmesh
};

BEGIN_ENUM_RTTI( EActorActionType )
	ENUM_OPTION( ActorAction_None );
	ENUM_OPTION( ActorAction_Moving );
	ENUM_OPTION( ActorAction_Rotating );
	ENUM_OPTION( ActorAction_Animation );
	ENUM_OPTION( ActorAction_RaiseEvent );
	ENUM_OPTION( ActorAction_Sliding );
	ENUM_OPTION( ActorAction_Working );
	ENUM_OPTION( ActorAction_ChangeEmotion );
	ENUM_OPTION( ActorAction_Exploration );
	ENUM_OPTION( ActorAction_UseDevice );
	ENUM_OPTION( ActorAction_DynamicMoving );
	ENUM_OPTION( ActorAction_MovingOnCurve );
	ENUM_OPTION( ActorAction_CustomSteer);
	ENUM_OPTION( ActorAction_R4Reserved_PC );
	ENUM_OPTION( ActorAction_MovingOutNavdata );
END_ENUM_RTTI()

/// Action result
enum EActorActionResult
{
	ActorActionResult_InProgress,
	ActorActionResult_Succeeded,			//!< actor action has finished successfully
	ActorActionResult_Failed,	
};

enum EPushingDirection : CEnum::TValueType;
class IDebugFrame;

///////////////////////////////////////////////////////////////////////////////

/// Base class for actor action
class ActorAction
{
protected:
	CActor*					m_actor;		//!< The actor
	EActorActionType		m_type;			//!< Type of action

public:
	//! Get action type
	RED_INLINE EActorActionType GetType() const { return m_type; }

public:
	ActorAction( CActor* actor, EActorActionType type );
	~ActorAction();

	//! Update, if false returned no further updates will be done
	virtual Bool Update( Float timeDelta );

	//! Is canceling of this action by other actions allowed
	virtual Bool IsCancelingAllowed() const { return true; }

	//! Tells if a look-at can be used with the current action
	virtual Bool CanUseLookAt() const { return false; }

	//! Can actor react on interest point while performing current action
	virtual Bool CanReact() const { return true; }

	//! Called when an actor is being pushed by another actor
	virtual void OnBeingPushed( const Vector& direction, Float rotation, Float speed, EPushingDirection animDirection );

	//! Stop action
	virtual void Stop()=0;

	//! Debug draw
	virtual void GenerateDebugFragments( CRenderFrame* frame ) {}

	//! Additional debug
	virtual void DebugDraw( IDebugFrame& debugFrame ) const {}

	//! A debug description of an action
	virtual String GetDescription() const { return TXT(""); }

	//! GC serialization
	virtual void OnGCSerialize( IFile& file ) {}
};

///////////////////////////////////////////////////////////////////////////////
