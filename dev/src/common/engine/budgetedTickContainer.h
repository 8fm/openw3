#pragma once

#include "budgetedContainer.h"

/**
 *	Tick specialized budgeted tick container.
 *
 *	The main thing here is Tick() that makes sure all elements are given correct accumulated delta times.
 *
 *	This is achieved by grouping elements processed within single Tick() call and assigning them group id.
 *	Then, instead of accumulating delta time for each ticked element separately (which might be slow if the list
 *	is long), only "delta time" groups' get their delta times accumulated.
 *
 *	End result example:
 *		If we have 1000 entities and only 100 of them manage to get ticked over specified time limit, each one
 *		will get 10 * deltaTime passed to Tick() function *on average* (as opposed to only delta time from
 *		last frame).
 */
template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor, Uint32 MaxDeltaTimeGroups >
class TBudgetedTickContainer : public TBudgetedContainer< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >
{
	typedef TBudgetedContainer< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor > TBaseClass;

private:
	Float m_deltaTimeGroups[ MaxDeltaTimeGroups ];		// Accumulated delta times for each group
	Uint8 m_currentDeltaTimeGroup;						// Index of the current "delta time" group

public:
	TBudgetedTickContainer()
	{
		ASSERT( MaxDeltaTimeGroups <= ( sizeof( m_currentDeltaTimeGroup ) << 8 ), TXT("MaxDeltaTime mustn't be larger than what m_currentDeltaTimeGroup allows for.") );
		for ( Uint32 i = 0; i < MaxDeltaTimeGroups; ++i )
		{
			m_deltaTimeGroups[ i ] = 0.0f;
		}
		m_currentDeltaTimeGroup = 0;
	}

	RED_FORCE_INLINE void Add( ELEMENT_TYPE* element, Bool budgeted )
	{
		TBaseClass::Add( element, budgeted );
		if ( budgeted )
		{
			TBaseClass::m_budgeted.GetElementAccessor().SetDeltaTimeGroup( element, m_currentDeltaTimeGroup );
		}
	}

	template < class Ticker >
	void Tick( Ticker ticker, Float deltaTime )
	{
		Tick( ticker, deltaTime, TBaseClass::m_maxBudgetedProcessingTime );
	}

	// Tick all elements, ignoring any time budgets.
	template < class Ticker >
	void TickAll( Ticker ticker, Float deltaTime )
	{
		Tick( ticker, deltaTime, NumericLimits< Float >::Max() );
	}


private:
	template < class Ticker >
	void Tick( Ticker ticker, Float deltaTime, Float timeBudget );
};

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor, Uint32 MaxDeltaTimeGroups >
template < class Ticker >
void TBudgetedTickContainer< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor, MaxDeltaTimeGroups >::Tick( Ticker ticker, Float deltaTime, Float timeBudget )
{
	// Accumulate delta time for all groups and zero out delta time for new group

	for ( Uint32 i = 0; i < MaxDeltaTimeGroups; ++i )
	{
		m_deltaTimeGroups[ i ] += deltaTime;
	}
	if ( ++m_currentDeltaTimeGroup == MaxDeltaTimeGroups )
	{
		m_currentDeltaTimeGroup = 0;
	}
	m_deltaTimeGroups[ m_currentDeltaTimeGroup ] = 0.0f;

	// Process non-budgeted
	{
		PC_SCOPE_PIX( TickNonBudgeted );

		for ( typename TBaseClass::SafeIterator it( &( TBaseClass::m_nonBudgeted ) ); !it.IsEnd(); ++it )
		{
			ticker.Tick( *it, deltaTime );
		}
	}

	// Process budgeted
	{
		PC_SCOPE_PIX( TickBudgeted );

		CTimeCounter timer;

		TCustomIntrusiveListElementAccessor accessor = TBaseClass::m_budgeted.GetElementAccessor(); // Make a copy, it'll be faster to do Get/SetDeltaTimeGroup()

		TBaseClass::m_numRecentlyProcessedBudgeted = 0;
		for ( TBaseClass::m_budgetedIterator.Begin();
			( !TBaseClass::m_budgetedIterator.IsEnd() && timer.GetTimePeriod() < timeBudget );
			++TBaseClass::m_budgetedIterator, ++TBaseClass::m_numRecentlyProcessedBudgeted )
		{
			ELEMENT_TYPE* element = *TBaseClass::m_budgetedIterator;

			// Determine total accumulated delta time for this element

			const Uint8 deltaTimeGroup = accessor.GetDeltaTimeGroup( element );
			const Float accumulatedDeltaTime = m_deltaTimeGroups[ deltaTimeGroup ];

			// Assign the element new "delta time" group
			// This is done before tick because element can be deleted in Tick function.
			accessor.SetDeltaTimeGroup( element, m_currentDeltaTimeGroup );

			// Tick the element 
			ticker.Tick( element, accumulatedDeltaTime );

		}

		TBaseClass::m_recentlyUsedTimeBudget = timer.GetTimePeriod();
	}
}

//! Gets the name of tick-budgeted intrusive list element accessor for a given type and a 'list purpose name'
#define CUSTOM_TICK_BUDGETED_ELEMENT_ACCESSOR_CLASS( type, name )				\
	type::CustomIntrusiveListElementAccessor_##name

//! Defines default tick-budgeted intrusive list element accessor for a given type and a 'list purpose name'; to be inserted inside of the body of the stored list element
#define DEFINE_TICK_BUDGETED_LIST_ELEMENT( type, name )						\
private:																	\
	TCustomIntrusiveListPtr< type > m_prevElement_##name;					\
	TCustomIntrusiveListPtr< type > m_nextElement_##name;					\
	Uint8							m_deltaTimeGroup_##name;				\
public:																		\
	struct CustomIntrusiveListElementAccessor_##name						\
	{																		\
		friend class type;													\
		RED_FORCE_INLINE type* GetPrev( type* element ) const				\
		{																	\
			return element->m_prevElement_##name.m_ptr;						\
		}																	\
		RED_FORCE_INLINE void SetPrev( type* element, type* prev )			\
		{																	\
			element->m_prevElement_##name.m_ptr = prev;						\
		}																	\
		RED_FORCE_INLINE type* GetNext( type* element ) const				\
		{																	\
			return element->m_nextElement_##name.m_ptr;						\
		}																	\
		RED_FORCE_INLINE void SetNext( type* element, type* next )			\
		{																	\
			element->m_nextElement_##name.m_ptr = next;						\
		}																	\
		RED_FORCE_INLINE Uint8 GetDeltaTimeGroup( type* element ) const		\
		{																	\
			return element->m_deltaTimeGroup_##name;						\
		}																	\
		RED_FORCE_INLINE void SetDeltaTimeGroup( type* element, Uint8 deltaTimeGroup ) \
		{																	\
			element->m_deltaTimeGroup_##name = deltaTimeGroup;				\
		}																	\
	};
