/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlib.h"
#include "pathlibConst.h"

#include "pathlibGraph.h"
#include "pathlibNodeFinder.h"

namespace PathLib
{

class CHLNode;
class CHLSubGraph;
class CNavGraph;
class CNavNode;
class CWalkableSpotQueryRequest;

namespace KD
{
	class CNodeMap;
};
///////////////////////////////////////////////////////////////////////////////
// CNodeFinder - KDTree utilization
///////////////////////////////////////////////////////////////////////////////
class CCentralNodeFinder : public Red::System::NonCopyable
{
protected:
	CNavGraph&				m_navgraph;											
	CNodeFinder				m_detachedNodeMap;									// contains nodes not attached to any area region
	Bool					m_isInitialized;

public:
	// Region acceptor - small structure meant to be kept on stack that configures logic of iteration over regions matching given search
	struct RegionAcceptor
	{
		enum EType
		{
			ACCEPT_ANY,														// run query through all nodes
			ACCEPT_ANY_VALID,												// run query through all nodes, except invalid ones
			ACCEPT_SPECYFIC,												// run query through nodes of specyfic area region
			ACCEPT_REACHABLE,												// run query through all nodes reacheable from specyfic region
			ACCEPT_INVALID,													// quit the test
		}						m_type : 16;
		AreaRegionId			m_areaRegion;
		CSearchNode::Marking	m_marker;	
		Bool					m_bailOutOnSuccess;

		Bool					IsInvalid() const							{ return m_type == ACCEPT_INVALID; }
	};

	// Procedures that spawn RegionAcceptor structure
	static RegionAcceptor	AnyRegion( Bool includeInvalid, Bool bailOutOnSuccess = true );
	static RegionAcceptor	SpecyficRegion( AreaRegionId regionId );
	RegionAcceptor			ReacheableRegion( AreaRegionId regionId, Bool bailOutOnSuccess = true, NodeFlags pathfindingForbiddenFlags = NFG_FORBIDDEN_BY_DEFAULT ) const;
	RegionAcceptor			ReacheableRegion( CoherentRegion regionId, Bool bailOutOnSuccess = true, NodeFlags pathfindingForbiddenFlags = NFG_FORBIDDEN_BY_DEFAULT ) const;
	RegionAcceptor			SpecyficAt( const Vector3& pos ) const;
	RegionAcceptor			ReachableFrom( const Vector3& pos, Float personalSpace, Bool bailOutOnSuccess = true, Uint32 collisionFlags = CT_DEFAULT, NodeFlags pathfindingForbiddenFlags = NFG_FORBIDDEN_BY_DEFAULT ) const;
	static RegionAcceptor	ReachableFrom( CPathLibWorld& pathlib, const Vector3& pos, Float personalSpace, Bool bailOutOnSuccess = true, Uint32 collisionFlags = CT_DEFAULT, NodeFlags pathfindingForbiddenFlags = NFG_FORBIDDEN_BY_DEFAULT );

	// Internal iterator for regions matching region acceptors
	// Implementation is trashy, but it wraps up very elegant system for iteration over set of regions
	struct RegionIterator : public Red::System::NonCopyable
	{
	protected:
		const CNodeFinder*		m_nodeFinder;
		CHLSubGraph*			m_topGraph;
		Uint32					m_index;
		Uint32					m_indexCount;
		CSearchNode::Marking	m_marker;
		void					(*m_next)( RegionIterator& it );

	public:
		RegionIterator( const CCentralNodeFinder& centralNF, const RegionAcceptor& regionAcceptor );

		operator Bool() const													{ return m_nodeFinder != nullptr; }
		void operator++()														{ m_next( *this ); }

		const CNodeFinder&	operator*() const									{ return *m_nodeFinder; }
		const CNodeFinder* operator->() const									{ return m_nodeFinder; }
	};
	
	static const Uint32 MAX_OUTPUT = 64;

	typedef TStaticArray< CPathNode::Id, MAX_OUTPUT > OutputVector;

	CCentralNodeFinder( CNavGraph& navgraph );
	~CCentralNodeFinder();

	CNavGraph&				GetNavGraph() const									{ return m_navgraph; }
	const CNodeFinder&		GetDetachedNodesMap() const							{ return m_detachedNodeMap; }

	///////////////////////////////////////////////////////////////////////////
	// Life cycle control
	void					Initialize()										{ if ( !m_isInitialized ) { Compute(); } }
	void					Compute();
	void					Clear();
	Bool					IsInitialized() const;

	void					Invalidate();
	void					CompactData() const;

	void					AddDynamicElement( CNavNode* pNode ) const;
	void					RemoveDynamicElement( CNavNode* pNode ) const;
	///////////////////////////////////////////////////////////////////////////
	// Query functions
	template < class Functor >
	RED_INLINE CNavNode*	TQuery( Functor& fun, const RegionAcceptor& region ) const;
	template < class Functor >
	RED_INLINE Bool		TQueryN( Functor& fun, const RegionAcceptor& region ) const;

	CNavNode*				FindClosestNode( const Vector3& pos, Float maxDist, const RegionAcceptor& region, NodeFlags forbiddenNodeFlags = NFG_FORBIDDEN_BY_DEFAULT ) const;
	CNavNode*				FindClosestNodeWithLinetest( const Vector3& pos, Float maxDist, Float lineWidth, const RegionAcceptor& region, Uint32 lineTestFlags = CT_DEFAULT, NodeFlags forbiddenNodeFlags = NFG_FORBIDDEN_BY_DEFAULT ) const;
	CNavNode*				FindClosestNodeWithLinetestAndTolerance( const Vector3& pos, Float& toleranceRadius, Float lineWidth, Vector3& outAccessiblePosition, const RegionAcceptor& region, Uint32 lineTestFlags = CT_DEFAULT, NodeFlags forbiddenNodeFlags = NFG_FORBIDDEN_BY_DEFAULT, Bool allowVerticalDiversity = true ) const;

	Bool					FindNClosestNodes( const Vector3& pos, Float maxDist, Uint32 n, OutputVector& output, const RegionAcceptor& region, NodeFlags forbiddenNodeFlags = NFG_FORBIDDEN_BY_DEFAULT ) const;
	Bool					FindNClosestNodesWithLinetest( const Vector3& pos, Float maxDist, Float lineWidth, Uint32 n, OutputVector& output, const RegionAcceptor& region, Uint32 lineTestFlags = CT_DEFAULT, NodeFlags forbiddenNodeFlags = NFG_FORBIDDEN_BY_DEFAULT ) const;

	// General query functions
	//typedef					CNodeFinder::Acceptor								Acceptor;
	typedef					CNodeFinder::Handler								Handler;

	CNavNode*				FindClosestNode( CWalkableSpotQueryRequest& request, const RegionAcceptor& region ) const;
	//CNavNode*				FindClosestNode( const Vector3& pos, Float maxDist, Acceptor& acceptor, const RegionAcceptor& region, NodeFlags forbiddenNodeFlags = NFG_FORBIDDEN_BY_DEFAULT ) const;
	//CNavNode*				FindClosestNode( const Vector3& pos, const Box& bbox, Float maxDist, Acceptor& acceptor, const RegionAcceptor& region, NodeFlags forbiddenNodeFlags = NFG_FORBIDDEN_BY_DEFAULT ) const;
	//Bool					FindNClosestNodes( const Vector3& pos, Float maxDist, Uint32 n, OutputVector& output, Acceptor& acceptor, const RegionAcceptor& region, NodeFlags forbiddenNodeFlags = NFG_FORBIDDEN_BY_DEFAULT ) const;

	void					IterateNodes( const Box& boundings, Handler& handler, const RegionAcceptor& region, NodeFlags forbiddenNodeFlags = NFG_FORBIDDEN_BY_DEFAULT ) const;
};



};			// namespace PathLib




