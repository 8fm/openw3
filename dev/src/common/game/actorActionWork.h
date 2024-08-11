/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/engine/behaviorGraphAnimationSlotNode.h"

#include "actionPointManager.h"
#include "jobTree.h"
#include "jobTreeLeaf.h"
#include "jobTreeNode.h"
#include "../core/listener.h"

#ifndef NO_LOG

#define WORK_LOG( format, ... )		RED_LOG( Work, format, ## __VA_ARGS__ )
#define WORK_WARN( format, ... )	RED_LOG( Work, format, ## __VA_ARGS__ )
#define WORK_ERR( format, ... )		RED_LOG( Work, format, ## __VA_ARGS__ )

#else

#define WORK_LOG( format, ... )	
#define WORK_WARN( format, ... ) 
#define WORK_ERR( format, ... )	

#endif

enum EActionWorkActivity
{
	AWA_Initialize,
	AWA_InitialPositionAdjustment,
	AWA_AdjustPosition,
	AWA_StartWorking,
	AWA_NextAction,
	AWA_Animation,
	AWA_BreakAnimation,
	AWA_Stopping,
	AWA_Failing,
	AWA_Idle,
	AWA_ProcessItems,
	AWA_StandbyItems
};

enum EJobExecutionMode
{
	JEM_Normal,
	JEM_Breaking,
	JEM_ForcedBreaking
};

class CItemEntityProxy;
enum ENavigationStatus : CEnum::TValueType;
enum EJobMovementMode : CEnum::TValueType;
class CMovePIBasic;

class ActorActionWorkFreezer
{
	Float	m_animFreezingTime;
	Bool	m_isAnimFreezing;

	Float	m_invisibilityTime;

public:
	ActorActionWorkFreezer();

	void Update( CActor* actor, Float timeDelta, Bool isCurrentActionLeaf );

	void ActorPlayAnimation( CActor* actor, Bool isCurrentActionLeaf );

	void UpdateActorPose( CActor* actor ) const;

	Bool IsActorFrozen() const;
	Bool IsAnimTimeoutOccured() const;
	void ResetAnimTimer();

	void StartWorking( CActor* actor );
	void FinishWorking( CActor* actor );

private:
	Bool IsOutsideVisibleRange( Float distSqr ) const;
	Bool IsOutsideInvisibleRange( Float distSqr ) const;
	Bool IsActorVisible( CEntity* entity ) const;

	Bool IsInvisibilityTimeoutOccured() const;

	void FreezeActor( CActor* actor );
	void UnfreezeActor( CActor* actor );
	void RestoreActor( CActor* actor );
	void Reset();
};

class ActorActionWork : public ActorAction, public ISlotAnimationListener, public IEventHandler< CAnimationEventFired >
{
public:
	EActionWorkActivity				m_workActivity;				//!< Is npc moving to job action place actually
	EJobExecutionMode				m_executionMode;			//!< Is this work breaking or something?
	Bool							m_isInfinite;				//!< Is this work an infinite one

	SJobTreeExecutionContext		m_jobContext;				//!< Current job context
	THandle<CJobTree>				m_jobTree;					//!< Job tree played by this action
	THandle<CJobTreeNode>			m_jobTreeRoot;				//!< Root of the job tree played by this action
	const CJobActionBase*			m_currentAction;			//!< Currently executed job action
	SJobActionExecutionContext		m_actionContext;			//!< Action execution context

	TActionPointID					m_currentApId;				//!< Id of an action point in which the job is executed
	CName							m_currentCategory;
	CEntity*						m_currentOwner;
	CNewNPC*						m_NPCActor;					//!< NPC ptr stored for community system interaction
	CAnimatedComponent*				m_animated;					//!< NPC's animated component

	ActorActionWorkFreezer			m_freezer;

	Float							m_startAnimationOffset;		//!< Time offset for the first animation

	Float							m_actorInteractionPriority;

	static const CName				WORK_ANIM_SLOT_NAME;
	static const Float				WORK_INTERACTION_PRIORITY;

public:
	ActorActionWork( CActor* actor );
	~ActorActionWork();
	
	Bool StartWorking( CJobTree* jobTree, CName category, Bool skipEntryAnimations );

	//experimental solution to avoid idle anim before start work after scene
	void StartWorkAnimInstant();

	//! Generally, switch to use only job tree on leave actions
	Bool ExitWorking( Bool fast = false );

	//! Stop action
	void Stop();

	//! Update, if false returned no further updates will be done
	virtual Bool Update( Float timeDelta );

	//! Check if actual work is infinite
	Bool IsInfinite();

	//! Tells if a look-at can be used with the current action
	virtual Bool CanUseLookAt() const;

	//! Can actor react on interest point while performing current action
	virtual Bool CanReact() const;

	//! Called when an actor is being pushed by another actor
	virtual void OnBeingPushed( const Vector& direction, Float rotation, Float speed, EPushingDirection animDirection );

	//! Check if this work is being stopped
	Bool IsBreaking() { return m_executionMode == JEM_Breaking || m_executionMode == JEM_ForcedBreaking; }

	// -------------------------------------------------------------------------
	// Debugging
	// -------------------------------------------------------------------------
	// Debug draw
	void GenerateDebugFragments( CRenderFrame* frame );
	String GetDescription() const;

	// -------------------------------------------------------------------------
	// Work execution notifications
	// -------------------------------------------------------------------------
protected:
	void OnWorkSequenceStart();

	void OnWorkSequenceFinished();

	void OnJobActionStarted();

	//! Job action it's finished, proceed to the next one if any, or signal finish
	void OnJobActionFinished();

	void PutOutAny( SJobActionExecutionContext& actionContext );
	void PutOutAny( SJobActionExecutionContext& actionContext, SJobActionExecutionContext::SItemInfo& itemInfo );

private:
	//! Get next job action
	const CJobActionBase* GetNextAction();

	// Retrieves a moving agent component, initializing dependent subsystems if needed
	CMovingAgentComponent* GetMAC() const;

	// Adjusts the position of the actor before it starts executing the job tree.
	// Returns 'true' if the position adjustment process was completed successfully,
	// or 'false' if it needs more time.
	Bool AdjustPosition() const;

	// Apply inventory setup for node
	Bool SetupItems();

	// -------------------------------------------------------------------------
	// Animations
	// -------------------------------------------------------------------------
	//! Start animation
	Bool StartAnimation( const CName& slotName, const CJobActionBase* actionBase, Bool noBlendIn = false );

	//! Register this as anim event handler
	RED_INLINE void RegisterAnimEventHandler();

	//! Unregister this as anim event handler
	RED_INLINE void UnregisterAnimEventHandler();

	//! Handle animation event
	void HandleEvent( const CAnimationEventFired &event );

	//! Animation in slot has ended
	virtual void OnSlotAnimationEnd( const CBehaviorGraphAnimationBaseSlotNode * sender, CBehaviorGraphInstance& instance, ISlotAnimationListener::EStatus status );
	virtual String GetListenerName() const { return TXT("ActorActionWork"); }
};

template<> RED_INLINE String ToString( const EActionWorkActivity& value )
{
	switch ( value )
	{
	case AWA_Initialize:
		return TXT("AWA_Initialize");
	case AWA_InitialPositionAdjustment:
		return TXT("AWA_InitialPositionAdjustment");
	case AWA_AdjustPosition:
		return TXT("AWA_AdjustPosition");
	case AWA_NextAction:
		return TXT("AWA_NextAction");
	case AWA_StartWorking:
		return TXT("AWA_StartWorking");
	case AWA_Animation:
		return TXT("Animation");
	case AWA_BreakAnimation:
		return TXT("AWA_BreakAnimation");
	case AWA_Stopping:
		return TXT("Stopping");
	case AWA_Failing:
		return TXT("AWA_Failing");
	case AWA_Idle:
		return TXT("Idle");
	case AWA_ProcessItems:
		return TXT("ProcessItems");
	case AWA_StandbyItems:
		return TXT("StandbyItems");
	default:
		return TXT("Unknown");
	}
}

template<> RED_INLINE String ToString( const EJobExecutionMode& value )
{
	switch ( value )
	{
	case JEM_Normal:
		return TXT("Normal");
	case JEM_Breaking:
		return TXT("Breaking");
	case JEM_ForcedBreaking:
		return TXT("Fast breaking");
	default:
		return TXT("Unknown");
	}
}
