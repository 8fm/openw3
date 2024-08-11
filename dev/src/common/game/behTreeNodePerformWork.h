/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "behTreeNode.h"
#include "behTreeNodeAtomicAction.h"
#include "behTreeWorkData.h"
#include "jobTreeNode.h"
#include "jobTreeLeaf.h"
#include "behTreeCounterData.h"

#include "..\engine\characterControllerParam.h"

class CBehTreeNodePerformWorkInstance;

//#define DEBUG_PERFORM_WORK_ADJUSTMENT

//////////////////////////////////////////////////////////////////////////
// Work freezer helper
//////////////////////////////////////////////////////////////////////////

class ActorWorkFreezer
{
	Float	m_animFreezingTime;
	Bool	m_isAnimFreezing;

	Float	m_invisibilityTime;

public:
	ActorWorkFreezer();

	void Update( CActor* actor, Float timeDelta, Bool isCurrentActionLeaf );

	void ActorPlayAnimation( CActor* actor, Bool isCurrentActionLeaf );

	void UpdateActorPose( CActor* actor ) const;

	Bool IsActorFrozen() const;
	Bool IsAnimTimeoutOccured() const;
	void ResetAnimTimer();

	void StartWorking( CActor* actor );
	void FinishWorking( CActor* actor );

private:
	static Bool IsOutsideInvisibleRange( Float distSqr );
	static Bool IsActorVisible( CEntity* entity );

	Bool IsInvisibilityTimeoutOccured() const;

	void FreezeActor( CActor* actor );
	void UnfreezeActor( CActor* actor );

	void RestoreActor( CActor* actor );
	void Reset();
};

////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodePerformWorkDefinition : public CBehTreeNodeAtomicActionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodePerformWorkDefinition, IBehTreeNodeDefinition, CBehTreeNodePerformWorkInstance, PerformWork );
public:
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodePerformWorkDefinition );
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// Instance
////////////////////////////////////////////////////////////////////////
class CBehTreeNodePerformWorkInstance : public CBehTreeNodeAtomicActionInstance, public IEventHandler< CAnimationEventFired >, public ISlotAnimationListener
{
	typedef CBehTreeNodeAtomicActionInstance Super;

protected:
	enum EWorkActivity
	{
		WA_Initialize,
		WA_InitialPositionAdjustment,
		WA_StartWorking,
		WA_NextAction,
		WA_Animation,
		WA_BreakAnimation,
		WA_Stopping,
		WA_Failing,
		WA_Idle,
		WA_ProcessItems,
		WA_StandbyItems,
		WA_Completed,
		WA_JustBreak
	};

	enum EJobExecutionMode
	{
		JEM_Normal,
		JEM_Breaking,
		JEM_ForcedBreaking
	};

	enum EProcessLogicResult
	{
		PLR_Success,
		PLR_Faild,
		PLR_InProgress,
	};

	Bool						m_immediateActivation;
	Bool						m_immediateAdjustment;
	Bool						m_initialAnimationWithOffset;
	Bool						m_ignoreCollisions;
	Bool						m_inFastLeaveAnimation;
	Bool						m_isAdjusted;
	Bool						m_isZCorrectionPending;
	Bool						m_isDone;
	Bool						m_workPaused;
	Bool						m_startAnimationWithoutBlend;
	Bool						m_disabledIK;
#ifdef DEBUG_PERFORM_WORK_ADJUSTMENT
	Vector3						m_debugShift;
	Vector3						m_debugActionStartedLocation;
	Float						m_debugRotation;
#endif
	CBehTreeWorkDataPtr			m_workData;

	EWorkActivity				m_workActivity;				//!< Is npc moving to job action place actually
	THandle< CJobTree >			m_jobTree;					//!< Job tree played by this action
	THandle< CJobTreeNode >		m_jobTreeRoot;				//!< Root of the job tree played by this action
	EJobExecutionMode			m_executionMode;			//!< Is this work breaking or something?

	ActorWorkFreezer			m_freezer;
	CAnimatedComponent*			m_animated;					//!< NPC's animated component
	SJobTreeExecutionContext	m_jobContext;				//!< Current job context
	SJobActionExecutionContext	m_actionContext;			//!< Action execution context
	CName						m_currentCategory;
	Float						m_startAnimationOffset;		//!< Time offset for the first animation
	Float						m_globalBreakingBlendOutTime;       //!< Blend time used when stopping animation
	CName						m_spawnedLeftItem;
	CName						m_spawnedRightItem;

	TActionPointID				m_currentApId;				//!< Id of an action point in which the job is executed
	const CJobActionBase*		m_currentAction;			//!< Currently executed job action
	

	static const Int32			FORCE_REQUEST_PRIORITY_BONUS;
	static const InteractionPriorityType BREAKABLE_WORK_INTERACTION_PRIORITY;
	static const InteractionPriorityType WORK_INTERACTION_PRIORITY;

	

public:
	typedef CBehTreeNodePerformWorkDefinition Definition;

	CBehTreeNodePerformWorkInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	void OnDestruction() override;

	Bool Activate() override;
	void Deactivate() override;
	void Update() override;

	//! Custom interface
	Bool Interrupt() override;

	Int32 Evaluate() override;

	Bool OnEvent( CBehTreeEvent& e ) override;
	Bool OnListenedEvent( CBehTreeEvent& e ) override;

	// -------------------------------------------------------------------------
	// Work execution notifications
	// -------------------------------------------------------------------------
protected:
	void PauseSotAnimation( Bool pause );

	void UpdateIsSitting( );

	void OnWorkSequenceStart();

	void OnWorkSequenceFinished();

	void OnJobActionStarted();

	//! Job action it's finished, proceed to the next one if any, or signal finish
	void OnJobActionFinished();

	void PutOutAny( SJobActionExecutionContext& actionContext );
	void PutOutAny( SJobActionExecutionContext& actionContext, SJobActionExecutionContext::SItemInfo& itemInfo );

	void UnmountItems();

	//! Generally, switch to use only job tree on leave actions
	Bool ExitWorking( Bool fast = false );

	void ResetContexts();

	EProcessLogicResult ProcessLogic();

	Bool HasValidForceRequest() const;

	//! Get next job action
	const CJobActionBase* GetNextAction();

	// Apply inventory setup for node
	void SetupItems();

	//! Fire skipped item events (if any)
	void FireSkippedItemEvents();

	// -------------------------------------------------------------------------
	// Animations
	// -------------------------------------------------------------------------
	//! Start animation
	Bool StartAnimation( CName slotName, const CJobActionBase* actionBase );

	//! Register this as anim event handler
	RED_INLINE void RegisterAnimEventHandler();

	//! Unregister this as anim event handler
	RED_INLINE void UnregisterAnimEventHandler();

	//! Handle animation event
	void HandleEvent( const CAnimationEventFired &event );

	//! Animation in slot has ended
	virtual void OnSlotAnimationEnd( const CBehaviorGraphAnimationBaseSlotNode * sender, CBehaviorGraphInstance& instance, ISlotAnimationListener::EStatus status );
	virtual String GetListenerName() const { return TXT( "PerformWorkBehaviorTreeTask" ); }
	
	void AdjustPosIfNeeded();
	void AdjustLeaveAction( Bool considerCurrentAnimation = true );
	// -------------------------------------------------------------------------
	// Debugging
	// -------------------------------------------------------------------------

public:
	virtual void OnGenerateDebugFragments( CRenderFrame* frame ) override;

#ifndef NO_EDITOR	
private:
	String GetDebugWorkActivityString() const;
#endif

	void DestroyItemsOnInterrupt();

};


