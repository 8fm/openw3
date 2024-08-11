/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "budgetedTickContainer.h"
#include "component.h"

// Forward declarations
class CComponent;
class CTaskBatch;
class CJobImmediateCollector;
struct STickManagerContext;

/**
 *	Component specific tick manager supporting budgeted component tick.
 *
 *	Note: only to be used internally by CTickManager.
 */
class CComponentTickManager
{
	friend class CTickManager;
public:
	/// Tick group per class stats
	struct TickGroupClassStats
	{
		CClass*		m_class;		//!< Component class
		Uint32		m_count;		//!< Number of components
		Uint64		m_time;			//!< Total time

		RED_INLINE TickGroupClassStats()
			: m_class( NULL )
			, m_count( 0 )
			, m_time( 0 )
		{};
	};

	/// Tick group stats data
	struct TickGroupStats
	{
		Uint32						m_statsCount;			//!< Number of ticked components
		Uint64						m_statsTime;			//!< Time it took to tick all components
		TickGroupClassStats			m_statGroups[ 32 ];		//!< Per class stat groups
		Uint32						m_numGroups;			//!< Number of stat groups

		RED_INLINE TickGroupStats()
			: m_statsCount( 0 )
			, m_statsTime( 0 )
			, m_numGroups( 0 )
		{};

		//! Reset stats
		void Reset();

		//! Accumulate stats data
		void AccumulateStats( CClass* componentClass, Uint64 timeTook );
	};

private:
	struct NonBudgetedComponents
	{
		THashSet< CComponentTickProxy* >		m_components;	// Non-budgeted components (in a particular tick group)

		void Add( CComponentTickProxy* proxy );
		void Remove( CComponentTickProxy* proxy );
	};

	struct BudgetedComponents
	{
		typedef TCustomIntrusiveList< CComponentTickProxy, CUSTOM_INTRUSIVE_LIST_ELEMENT_ACCESSOR_CLASS( CComponentTickProxy, tick ) > BudgetedComponentsContainer;
		typedef BudgetedComponentsContainer::safe_iterator BudgetedComponentsIterator;

		Float m_usedBudget;										// Used budget in current frame

		BudgetedComponentsContainer			m_components;		// All budgeted components (across all tick groups)
		BudgetedComponentsIterator			m_safeIterator;		// Safe component container iterator
		THashSet< CComponentTickProxy* >	m_tickedThisFrame;	// Set of components to tick this frame; subset of m_components
																// This is to assure tick-budgeted components are either ticked for all tick groups
																// or none (to avoid state of the component getting out of sync)
		BudgetedComponents();
		void Add( CComponentTickProxy* proxy );
		void Remove( CComponentTickProxy* proxy );
	};

	TickGroupStats						m_stats[ TICK_Max ];				// Stats for all tick groups
	Float								m_maxBudgetedTickTime;				// Max time allowed for budgeted tick
	Uint32								m_minBudgetedTickPercentage;		// Min percentage of ticked budgeted components
	BudgetedComponents					m_budgeted;							// Tick-budgeted components; shared among all tick groups

	Float								m_expectedNonBudgetedTickTime;		// Expected non-budgeted tick time
	Float								m_totalRecentNonBudgetedTickTime;	// The time it took last frame to tick all non-budgeted components
	NonBudgetedComponents				m_nonBudgeted[ TICK_Max ];			// Non-tick-budgeted (always ticked) components

	TDynArray< CComponent* >			m_tickedComponentsCopy;				// Temporary buffer used to store currently ticked components (to support remove during tick)

	CComponentTickManager();

	//! Get tick group statistics for components
	RED_INLINE const TickGroupStats& GetStats( ETickGroup group ) const { return m_stats[ group ]; }

	void AddToGroup( CComponent* component, ETickGroup group );
	void RemoveFromGroup( CComponent* component, ETickGroup group );
	void Remove( CComponent* component );
	void Suppress( CComponent* component, Bool suppress, CComponent::ETickSuppressReason reason );
	void SetBudgeted( CComponent* component, Bool budgeted, CComponent::ETickBudgetingReason reason );

	void TickComponent( CComponent* component, Float deltaTime, ETickGroup tickGroup );
	void TickComponentsSingleThreaded( Float timeDelta, ETickGroup tickGroup );
	void TickComponentsSingleThreadedImpl( Float timeDelta, ETickGroup tickGroup );
	void CollectImmediateJobs( STickManagerContext& context, CTaskBatch& taskBatch );

private:
	void ResetStats();
};
