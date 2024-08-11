
#include "kdTreeNnItem.h"
#include "kdTreeNNCollector.h"

#pragma once

template< class TFilterOwner > 
class kdTreeFilter
{
public:
	typedef Bool ( TFilterOwner::*FuncPtr )( Int32 ) const;
};

template< class TTree, class TFilterOwner >
struct kdTreeSearchContext : public Red::System::NonCopyable
{
	kdTreeSearchContext( const typename TTree::Point q, Float maxErr, kdTreeNNCollector< TTree >& collector, typename kdTreeFilter< TFilterOwner >::FuncPtr filter = nullptr, TFilterOwner* filterOwner = nullptr )
		: m_queryPoint( q )
		, m_maxErr( maxErr )
		, m_collector( collector )
		, m_numVisited( 0 )
		, m_allowSelfMatch( false )
		, m_filter( filter )
		, m_filterOwner( filterOwner )
	{
		m_nn = new kdTreeNnPool< TTree >( collector.m_nnNum );
	}

	~kdTreeSearchContext()
	{
		delete m_nn;
	}

	const typename TTree::Point							m_queryPoint;
	const Float											m_maxErr;
	const Bool											m_allowSelfMatch;

	Int32												m_numVisited;

	kdTreeNNCollector< TTree >&							m_collector;
	kdTreeNnPool< TTree >*								m_nn;
	typename kdTreeFilter< TFilterOwner >::FuncPtr		m_filter;
	TFilterOwner*										m_filterOwner;

	RED_INLINE Bool Filter( Int32 idx ) const { return m_filter ? ( m_filterOwner->*m_filter )( idx ) : true; }

};
template< class TTree, class TFilterOwner >
struct kdTreeRadiusSearchContext : public kdTreeSearchContext< TTree, TFilterOwner >
{
	typedef kdTreeSearchContext< TTree, TFilterOwner > TBaseClass;

	kdTreeRadiusSearchContext( const typename TTree::Point q, Float maxErr, kdTreeNNCollector< TTree >& collector, Float sqRadius, typename kdTreeFilter< TFilterOwner >::FuncPtr filter = nullptr, TFilterOwner* filterOwner = nullptr )
		: TBaseClass( q, maxErr, collector, filter, filterOwner )
		, m_sqRadius( sqRadius )
		, m_numInRange( 0 )
	{

	}

	const Float			m_sqRadius;
	Int32					m_numInRange;
};
template< class TTree, class TFilterOwner >
struct kdTreeFastSearchContext : public kdTreeSearchContext< TTree, TFilterOwner >
{
	typedef kdTreeSearchContext< TTree, TFilterOwner > TBaseClass;

	kdTreeFastSearchContext( const typename TTree::Point q, Float maxErr, kdTreeNNCollector< TTree >& collector, Int32 numPoints, typename kdTreeFilter< TFilterOwner >::FuncPtr filter = nullptr, TFilterOwner* filterOwner = nullptr )
		: TBaseClass( q, maxErr, collector, filter, filterOwner )
	{
		m_prioQueue = new kdTreeNnPrioQueue< TTree >( numPoints );
	}

	~kdTreeFastSearchContext()
	{
		delete m_prioQueue;
	}

	kdTreeNnPrioQueue< TTree >*	m_prioQueue;
};
