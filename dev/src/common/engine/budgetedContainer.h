#pragma once

#include "customIntrusiveList.h"

/**
 *	General purpose container of elements capable of budgeting processing time.
 *
 *	Stores 2 buckets of elements:
 *	- non-budgetable: ones that get processed on every call to Process()
 *	- budgetable: ones with limited processing time (maxProcessingTime)
 */
template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
class TBudgetedContainer
{
protected:
	typedef TCustomIntrusiveList< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor > ElementList;
	typedef typename ElementList::safe_iterator SafeIterator;

	ElementList			m_nonBudgeted;					// Ones that get processed every time in Process() regardless
	ElementList			m_budgeted;						// Ones that have budgeted processing time
	SafeIterator		m_budgetedIterator;				// Safe iterator linked to budgeted list
	Float				m_maxBudgetedProcessingTime;	// Max time allowed for processing (only affects budgeted elements)

	// Stats

	Float				m_recentlyUsedTimeBudget;		// Time in seconds it took to process budgeted elements
	Uint32				m_numRecentlyProcessedBudgeted; // Number of processed budgeted elements

public:
	TBudgetedContainer();

	// Sets max processing time (in seconds) per Process() call
	void SetMaxBudgetedProcessingTime( Float maxBudgetedProcessingTime );
	// Sets element accessor object
	void SetAccessor( TCustomIntrusiveListElementAccessor& accessor );
	// Clears list elements
	void Clear();
	// Adds stored elements to given array; NOTE: does not clear given array
	void Fill( TDynArray< ELEMENT_TYPE* >& elements );
	// Gets number of elements
	RED_FORCE_INLINE Uint32 Size() const;
	// Gets number of budgeted elements
	RED_FORCE_INLINE Uint32 GetNumBudgeted() const;
	// Gets number of non-budgeted elements
	RED_FORCE_INLINE Uint32 GetNumNonBudgeted() const;
	// Gets number of recently processed (as part of Process() calls) budgeted elements
	RED_FORCE_INLINE Uint32 GetNumRecentlyProcessedBudgeted() const;

	// Adds new element to either budgeted or non-budgeted bucket
	RED_FORCE_INLINE void Add( ELEMENT_TYPE* element, Bool budgeted );
	// Removes an element from either budgeted or non-budgeted bucket
	RED_FORCE_INLINE void Remove( ELEMENT_TYPE* element, Bool budgeted );
	// Changes element budgeted state
	RED_FORCE_INLINE void ChangeBudgeted( ELEMENT_TYPE* element, Bool budgeted );
	// Gets whether given element exist in this container NOTE: only works assuming given element is either in this container or is not in any other container (won't work if it is in another container)
	RED_FORCE_INLINE Bool Exist( ELEMENT_TYPE* element, Bool budgeted );
	// Gets whether given element exist in this container (budgeted or not)
	RED_FORCE_INLINE Bool Exist( ELEMENT_TYPE* element );

	// Processes all non-budgeted elements and as many budgeted elements as can fit within maxProcessingTime (measured across the whole Process call)
	template < class Processor >
	void Process( Processor processor );
	// Processes all elements
	template < class Processor >
	void ProcessAll( Processor processor );
};

// Template implementation

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
TBudgetedContainer< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::TBudgetedContainer()
	: m_maxBudgetedProcessingTime( 0.001f )
	, m_numRecentlyProcessedBudgeted( 0 )
	, m_budgetedIterator( &m_budgeted )
{}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
void TBudgetedContainer< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::SetMaxBudgetedProcessingTime( Float maxBudgetedProcessingTime )
{
	m_maxBudgetedProcessingTime = maxBudgetedProcessingTime;
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
void TBudgetedContainer< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::SetAccessor( TCustomIntrusiveListElementAccessor& accessor )
{
	m_nonBudgeted.SetElementAccessor( accessor );
	m_budgeted.SetElementAccessor( accessor );
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
void TBudgetedContainer< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::Clear()
{
	m_budgeted.Clear();
	m_nonBudgeted.Clear();
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
void TBudgetedContainer< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::Fill( TDynArray< ELEMENT_TYPE* >& elements )
{
	m_budgeted.Fill( elements );
	m_nonBudgeted.Fill( elements );
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
RED_FORCE_INLINE void TBudgetedContainer< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::Add( ELEMENT_TYPE* element, Bool budgeted )
{
	( budgeted ? m_budgeted : m_nonBudgeted ).PushBack( element );
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
RED_FORCE_INLINE void TBudgetedContainer< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::Remove( ELEMENT_TYPE* element, Bool budgeted )
{
	( budgeted ? m_budgeted : m_nonBudgeted ).Remove( element );
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
RED_FORCE_INLINE void TBudgetedContainer< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::ChangeBudgeted( ELEMENT_TYPE* element, Bool budgeted )
{
	Remove( element, !budgeted );
	Add( element, budgeted );
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
RED_FORCE_INLINE Bool TBudgetedContainer< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::Exist( ELEMENT_TYPE* element, Bool budgeted )
{
	return ( budgeted ? m_budgeted : m_nonBudgeted ).Exist( element );
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
RED_FORCE_INLINE Bool TBudgetedContainer<ELEMENT_TYPE, TCustomIntrusiveListElementAccessor>::Exist(ELEMENT_TYPE* element)
{
	return m_budgeted.Exist( element ) || m_nonBudgeted.Exist( element );
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
RED_FORCE_INLINE Uint32 TBudgetedContainer< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::Size() const
{
	return m_budgeted.Length() + m_nonBudgeted.Length();
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
RED_FORCE_INLINE Uint32 TBudgetedContainer< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::GetNumBudgeted() const
{
	return m_budgeted.Length();
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
RED_FORCE_INLINE Uint32 TBudgetedContainer< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::GetNumNonBudgeted() const
{
	return m_nonBudgeted.Length();
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
RED_FORCE_INLINE Uint32 TBudgetedContainer< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::GetNumRecentlyProcessedBudgeted() const
{
	return m_numRecentlyProcessedBudgeted;
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
template < class Processor >
void TBudgetedContainer< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::Process( Processor processor )
{
	// Process non-budgeted
	{
		PC_SCOPE_PIX( ProcessNonBudgeted );

		for ( SafeIterator it( &m_nonBudgeted ); !it.IsEnd(); ++it )
		{
			processor.Process( *it );
		}
	}

	// Process budgeted
	{
		PC_SCOPE_PIX( ProcessBudgeted );

		CTimeCounter timer;

		m_numRecentlyProcessedBudgeted = 0;
		for ( m_budgetedIterator.Begin();
			( !m_budgetedIterator.IsEnd() && timer.GetTimePeriod() < m_maxBudgetedProcessingTime );
			++m_budgetedIterator, ++m_numRecentlyProcessedBudgeted )
		{
			processor.Process( *m_budgetedIterator );
		}

		m_recentlyUsedTimeBudget = timer.GetTimePeriod();
	}
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
template < class Processor >
void TBudgetedContainer< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::ProcessAll( Processor processor )
{
	for ( ELEMENT_TYPE* element : m_nonBudgeted )
	{
		processor.Process( element );
	}

	for ( ELEMENT_TYPE* element : m_budgeted )
	{
		processor.Process( element );
	}
}
