#pragma once

#include "pathlibActiveNodesQueue.h"
#include "pathlibConst.h"
#include "pathlibGraph.h"

class IPathRater;

namespace PathLib
{

class CHLNode;
class CNavGraph;
class CNavgraphRegion;
class CPathNode;
class CSearchEngine;
class CSearchNode;

//#define PATHLIB_SUPPORT_PATH_RATER

class CBrowseData : public Red::System::NonCopyable
{
public:
	CBrowseData()															{}

	RED_INLINE Bool			CheckFlags( NodeFlags flags)				{ return ( m_flagsForbidden & flags ) == 0; }

	CSearchNode*				m_start;
	NodeFlags					m_flagsForbidden;
};

class CQueryData : public CBrowseData
{
public:
	CQueryData()															{}

	typedef TDynArray< const CPathNode* > tPath;

	Vector3						m_destinationPosition;
	tPath						m_path;
	CSearchNode*				m_destination;
#ifdef PATHLIB_SUPPORT_PATH_RATER
	IPathRater*					m_pathRater;
#endif
};

class CFindClosestSpotQueryData : public CQueryData
{
public:
	CFindClosestSpotQueryData()												{}

	 Float						m_distanceLimit;
	 Bool						m_lookForConnectors;
};

class CMultiQueryData : public CBrowseData
{
public:
	static const Uint32 MAX_TARGETS = 32;

	typedef TStaticArray< CSearchNode*, MAX_TARGETS > NodeList;

	CNavGraph*					m_graph;
	NodeList					m_originsList;
	NodeList					m_destinationList;
	PathCost					m_searchLimit;
	Bool						m_breakIfAnyNodeIsFound;
};

class CRegionFloodFillData : public CBrowseData
{
public:
	CRegionFloodFillData( CSearchNode* baseNode, CNavGraph* navgraph, AreaRegionId regionColor, NodeFlags forbiddenFlags, NodeFlags requiredFlags );

	RED_INLINE Bool			CheckFlags( NodeFlags flags)				{ return CBrowseData::CheckFlags( flags ) && ( m_requiredFlags & flags ) == m_requiredFlags; }

	CNavGraph*					m_graph;
	NodeFlags					m_requiredFlags;
	AreaRegionId				m_regionColor;
};

class CRegionUpdateData : public Red::System::NonCopyable
{
public:
	typedef TDynArray< CNavNode* > SortedNodeList;

	CRegionUpdateData( TDynArray< CNavNode* >& nodeList, CNavGraph& navgraph )
		: m_nodeList( nodeList )
		, m_graph( navgraph )												{}

	SortedNodeList&				m_nodeList;
	CNavGraph&					m_graph;
};

class CSearchEngine
{
protected:
	CActiveNodesQueue								m_activeNodes;
	TDynArray< CSearchNode* >						m_processingList;

	Uint16											m_marker;

	RED_INLINE void				InitializeAstarStartingNode( CSearchNode* node );
	template < class TImplementation >
	RED_INLINE Bool				TAstar( CBrowseData& data, TImplementation& t );
public:
	CSearchEngine();

	Bool						FindPath( CQueryData& data );																								// Main pathfinding algorithm
	Bool						FindPathToClosestSpot( CFindClosestSpotQueryData& data );																	// A* search to find closest spot
	Uint16						QueryMultipleDestinations( CMultiQueryData& data );																			// Extended pathfinding algorithm
	void						RegionFloodFill( CRegionFloodFillData& data, CNavGraph* navgraph, CHLNode* node, Bool internalDontModifyMarker = false );	// Initial region flood fill algorithm
	void						ComputeRegionConnection( CRegionFloodFillData& data, CNavGraph* navgraph, CHLNode* node );									// Iterates like the previous function and updates region connection. Its separated as usualy we want first to compute every region, and only then start to recompute its borders.
	Bool						RegionsUpdate( CRegionUpdateData& data );																					// Update region marking for given nodes (used after graph was modified)
	CSearchNode::Marking		SpreadMarking( CBrowseData& data );

	static Float				ComputeOptimizedPathLength( CPathLibWorld& pathlib, const Vector3& destination, const Vector3& origin, CSearchNode* destinationNode, Float personalSpace, CollisionFlags colFlags = CT_DEFAULT );

	Uint16						ObtainNewMarker()													{ return ++m_marker; }
	Bool						IsMarkerValid( Uint16 marker )										{ return m_marker == marker; }
};

};		// namespace PathLib



