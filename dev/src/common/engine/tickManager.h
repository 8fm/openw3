/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "gameTime.h"
#include "updateTransformManager.h"
#include "budgetedTickContainer.h"
#include "tickStats.h"
#include "timerManager.h"
#include "componentTickManager.h"
#include "entityTickManager.h"
#include "entity.h"
#include "fxState.h"
#include "..\core\taskBatch.h"

// Forward declarations
class CComponent;
class CEntity;
class CFXState;
class CTaskBatch;
class CPropertyAnimationSet;
class CJobImmediateCollector;

struct STickManagerContext : Red::System::NonCopyable
{
	ETickGroup		m_group;
	Float			m_timeDelta;
	CTaskBatch&		m_taskSync;
	Bool			m_shouldWait;
	Bool			m_asyncTick;

	STickManagerContext( CTaskBatch& recycledTaskSync, ETickGroup group, Float timeDelta, Bool asyncTick );
	~STickManagerContext();

	void Issue( ETaskSchedulePriority pri );
	void Wait();
};

/// World tick manager
class CTickManager
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

	friend class CEffectManager;
	friend class CFXState;

public:
	typedef TBudgetedTickContainer< CFXState, CUSTOM_TICK_BUDGETED_ELEMENT_ACCESSOR_CLASS( CFXState, tick ), 128 >	TickedFXStatesContainer;

protected:

	/// Tick action
	enum ETickAction
	{
		ACTION_Add,
		ACTION_Remove,
	};

	/// Tick command
	struct TickCommand
	{
		ETickAction				m_action;		//!< Action to perform
		THandle< CComponent >	m_component;	//!< Related component
		THandle< CEntity >		m_entity;		//!< Related entity
		CFXState*				m_fx;			//!< Related effect
		ETickGroup				m_group;		//!< MAX for all
		Bool					m_budgeted;		//!< Only used with ADD actions: indicates whether this should be processing-budgeted

		RED_INLINE TickCommand( ETickAction action, ETickGroup group, CComponent* component, Bool budgeted = false )
			: m_action( action )
			, m_component( component )
			, m_entity( NULL )
			, m_fx( NULL )
			, m_group( group )
			, m_budgeted( budgeted )
		{};

		RED_INLINE TickCommand( ETickAction action, CComponent* component, Bool budgeted = false )
			: m_action( action )
			, m_component( component )
			, m_entity( NULL )
			, m_fx( NULL )
			, m_group( TICK_Max )
			, m_budgeted( budgeted )
		{};

		RED_INLINE TickCommand( ETickAction action, CEntity* entity, Bool budgeted = false )
			: m_action( action )
			, m_component( NULL )
			, m_entity( entity )
			, m_fx( NULL )
			, m_group( TICK_Max )
			, m_budgeted( budgeted )
		{};

		RED_INLINE TickCommand( ETickAction action, CFXState* state, Bool budgeted = false )
			: m_action( action )
			, m_component( NULL )
			, m_entity( NULL )
			, m_fx( state )
			, m_group( TICK_Max )
			, m_budgeted( budgeted )
		{};
	};

protected:
	CWorld*								m_world;							//!< Related world
	TDynArray< TickCommand >			m_pendingCommands;					//!< Pending tick command to execute

	CTimerManager						m_timerManager;						//!< Gameplay timer manager
	CComponentTickManager				m_componentTickManager;				//!< Component specific tick manager
	CEntityTickManager					m_entityTickManager;

	TickedFXStatesContainer				m_effects;							//!< Effects to tick
	STickGenericStats					m_statEffects;						//!< Statistics for effects tick

	THashSet< CPropertyAnimationSet* >	m_propertyAnimationSets;			//!< Property animation sets

	CTaskBatch							m_taskSync;

public:
	RED_INLINE const CEntityTickManager&	GetEntityTickManager() const { return m_entityTickManager; }
	RED_INLINE CEntityTickManager&	GetEntityTickManager() { return m_entityTickManager; }

public:
	//! Get tick group statistics for components
	RED_INLINE const CComponentTickManager::TickGroupStats& GetGroupComponentStats( ETickGroup group ) const { return m_componentTickManager.GetStats( group ); }

	//! Get tick group statistics for timers
	RED_INLINE const STickGenericStats& GetGroupTimersStats( ETickGroup group ) const { return m_timerManager.GetStats( group ); }

	//! Get tick statistics for effects
	RED_INLINE const STickGenericStats& GetEffectStats() const { return m_statEffects; }

	//! Get tick statistics for entities
	RED_INLINE const STickGenericStats& GetEntitiesStats() const { return m_entityTickManager.GetStats(); }

public:
	CTickManager( CWorld* world );
	~CTickManager();

	// setup reference position for streaming and other calculations
	void SetReferencePosition( const Vector& position );

	// Components
	// ----------

	// Add component to tick group
	void AddToGroup( CComponent* component, ETickGroup group ) { ASSERT( ValidateMatchingWorld( component ) ); m_componentTickManager.AddToGroup( component, group ); }
	// Remove component from tick group
	void RemoveFromGroup( CComponent* component, ETickGroup group ) { ASSERT( ValidateMatchingWorld( component ) ); m_componentTickManager.RemoveFromGroup( component, group ); }
	// Remove component form all tick groups
	void Remove( CComponent* component ) { ASSERT( ValidateMatchingWorld( component ) ); m_componentTickManager.Remove( component ); }

	// Suppress/unsuppress component ticking
	void Suppress( CComponent* component, Bool suppress, CComponent::ETickSuppressReason reason ) { ASSERT( ValidateMatchingWorld( component ) ); m_componentTickManager.Suppress( component, suppress, reason ); }

	// Toggles component tick budgeting
	void SetBudgeted( CComponent* component, Bool budgeted, CComponent::ETickBudgetingReason reason ) { ASSERT( ValidateMatchingWorld( component ) ); m_componentTickManager.SetBudgeted( component, budgeted, reason ); }

	// Add component to tick group - delayed (if its possible that we are processing tick)
	void AddToGroupDelayed( CComponent* component, ETickGroup group );
	// Remove component from tick group - delayed (if its possible that we are processing tick)
	void RemoveFromGroupDelayed( CComponent* component, ETickGroup group );

	// CEntity
	// -------

	// Add actor so it will be on tick list
	void AddEntity( CEntity* entity );
	// Remove actor form tick list
	void RemoveEntity( CEntity* entity );

	// Add actor so it will be on tick list - delayed (if its possible that we are processing tick)
	void AddEntityDelayed( CEntity* entity );
	// Remove actor form tick list - delayed (if its possible that we are processing tick)
	void RemoveEntityDelayed( CEntity* entity );

	// CPropertyAnimationSet
	// ---------------------

	// Adds property animation set
	void AddPropertyAnimationSet( CPropertyAnimationSet* set );
	// Removes property animation set
	void RemovePropertyAnimationSet( CPropertyAnimationSet* set );

	// Ticks property animations
	void TickPropertyAnimations( Float deltaTime );

	// Effects
	// -------

private:

	// Add effect to tick list
	void AddEffect( CFXState* effect, Bool budgeted );
	// Remove effect from tick list
	void RemoveEffect( CFXState* effect, Bool budgeted );
	// Changes budgeted state of the effect
	void SetBudgeted( CFXState* effect, Bool budgeted );

	// Validates that given node belongs to world this tick manager is used for
	Bool ValidateMatchingWorld( CNode* node );

public:
	// Process tick of given tick group
	void TickEffects( Float timeDelta );

	// Tick all effects, bypassing any budgeting.
	void TickAllEffects( Float timeDelta );

#ifndef NO_DEBUG_PAGES
	TickedFXStatesContainer& GetEffects() { return m_effects; }
#endif

	// Timers
	// ------

	CTimerManager& GetTimerManager() { return m_timerManager; }

	// Miscellaneous
	// -------------

	//! Start new frame
	void BeginFrame();

	// Advance time for timers
	void AdvanceTime( Float timeDelta );

	// Process tick of given tick group
	void Tick( STickManagerContext& context );
	void TickSingleThreaded( STickManagerContext& context, CTaskBatch& taskSync );
	void TickImmediateJobs( STickManagerContext& context, CTaskBatch& taskSync );

#ifdef USE_ANSEL
	void TickNPCLODs();
#endif // USE_ANSEL

	// Process tick of given tick group
	void BeginProcessingStandardAnimations( Float timeDelta );
	void FlushProcessingStandardAnimations( Float timeDelta );

	// Process pending tick commands, should be called once a frame
	void ProcessPendingCommands();

	void OnSaveGameplayState( IGameSaver* saver );
	void OnLoadGameplayState( IGameLoader* loader );
};
