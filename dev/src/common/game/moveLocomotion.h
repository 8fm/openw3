/**
 * Copyright © 2010 CDProjekt Red, Inc. All Rights Reserved.
 */
#pragma once

#include "moveStateModifier.h"
#include "moveLocomotionSegment.h"

class IMoveLocomotionSegment;
class CRenderFrame;
class CMovingAgentComponent;
class IMovementTargeter;
class CMoveLSSteering;
class IMoveStateModifier;
class CMoveSteeringBehavior;
class IPathEngineOffMeshConnection;

///////////////////////////////////////////////////////////////////////////////

enum EMoveStatus
{
	MS_InProgress,		//!< Move is still in progress
	MS_Completed,		//!< Move has completed
	MS_Failed			//!< Move has failed (it did not end with the execution of the entire locomotion command)
};

class IMoveLocomotionListener
{
public:
	virtual void OnSegmentPushed( IMoveLocomotionSegment* segment ) = 0;
};

///////////////////////////////////////////////////////////////////////////////
//! A class processing the movement on a single segment of a path.
class CMoveLocomotion : public Red::System::NonCopyable
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Gameplay );

public:
	// An interface describing a controller of the locomotion class instance
	class IController
	{
		friend class CMoveLocomotion;

	private:
		CMoveLocomotion*			m_locomotion;

	public:		

		virtual ~IController();

		virtual void OnSegmentFinished( EMoveStatus status ) = 0;

		virtual void OnControllerAttached() = 0;
		
		virtual void OnControllerDetached() = 0;

		// Tells if the controller can issue locomotion commands
		RED_INLINE Bool CanUseLocomotion() const { return m_locomotion != NULL; }

	protected:
		IController();

		RED_INLINE CMoveLocomotion& locomotion()
		{
			ASSERT( m_locomotion );
			return *m_locomotion;
		}

		void OnControlGranted( CMoveLocomotion& locomotion );

		void OnControlDenied( CMoveLocomotion& locomotion );
	};
	friend class IController;

protected:
	CMovingAgentComponent&							m_agent;

	TList< IMoveLocomotionSegment* >				m_segsQueue;
	TList< IMoveStateModifier* >					m_modifiers;
	CMoveLSSteering*								m_idleSegment;
	IMoveLocomotionSegment*							m_activeSegment;
	Bool											m_wasSegmentActivated;

	IController*									m_controller;
	TDynArray< IMoveLocomotionListener * >			m_listeners;
public:
	CMoveLocomotion( CMovingAgentComponent& agent );
	virtual ~CMoveLocomotion();

	//! Serialization support
	void OnSerialize( IFile& file );

	//! Get moving agent
	CMovingAgentComponent& GetAgent() { return m_agent; }

	// Processes the locomotion
	void Tick( Float timeDelta );

	//! Checks if current movement can be canceled
	Bool CanCancelMovement() const;

	// Cancels current movement
	void CancelMove();

	// -------------------------------------------------------------------------
	// Locomotion commands
	// -------------------------------------------------------------------------

	//! Creates and pushes a new steering segment
	CMoveLSSteering& PushSteeringSegment( Bool switchToEntityRepresentation = false );

	//! Pushes a new locomotion segment
	void PushSegment( IMoveLocomotionSegment* segment );

	//! Pushes a new state modifier
	void PushStateModifier( IMoveStateModifier* modifier );

	//! Cancels the currently executed command
	void CancelCurrentCommand();

	// -------------------------------------------------------------------------
	// Controller management
	// -------------------------------------------------------------------------
	// Sets a new controller. Only one controller can be attached at a time
	void AttachController( IController& controller );

	// Detaches an existing controller
	void DetachController( IController& controller );

	// 
	void AddTargeter_MoveLSSteering(  IMovementTargeter *const targeter );
	// 
	void RemoveTargeter_MoveLSSteering(  IMovementTargeter *const targeter );

	// Adding a listener that will be notified of move locomotion actions
	void AddMoveLocomotionListener( IMoveLocomotionListener *const listener );

	// removing a listener
	void RemoveMoveLocomotionListener( IMoveLocomotionListener *const listener );

	// get currently processed segment
	IMoveLocomotionSegment* GetCurrentSegment();

	void OnSteeringBehaviorChanged( CMovingAgentComponent* owner, CMoveSteeringBehavior* prevSteeringGraph, InstanceBuffer* prevRuntimeData, CMoveSteeringBehavior* newSteeringGraph, InstanceBuffer* newRuntimeData );
	// -------------------------------------------------------------------------
	// Debugging
	// -------------------------------------------------------------------------
	// Debug draw
	void GenerateDebugFragments( CRenderFrame* frame );
	void GenerateDebugPage( TDynArray< String >& debugLines ) const;

private:
	void ProcessLocomotion( Float timeDelta );

	Bool PopSegment();

	// Called by the controller when it's being deleted
	void OnControllerDetached();

	void DeactivateStateModifiers();
};
///////////////////////////////////////////////////////////////////////////////

class CMoveLomotionCommandsBuffer
{
public:
	void Clear();
};

///////////////////////////////////////////////////////////////////////////////

