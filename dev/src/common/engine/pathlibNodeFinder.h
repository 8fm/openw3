#pragma once

#include "pathlib.h"
#include "pathlibConst.h"
#include "pathlibGraph.h"

namespace PathLib
{

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
class CNodeFinder
{
protected:
	KD::CNodeMap*			m_nodeMap;
public:
	static const Uint32 MAX_OUTPUT = 64;

	typedef TStaticArray< CPathNode::Id, MAX_OUTPUT > OutputVector;

	CNodeFinder();
	CNodeFinder( CNavGraph& navgraph, AreaRegionId regionId );
	CNodeFinder( const CNodeFinder& nf );
	CNodeFinder( CNodeFinder&& nf );
	~CNodeFinder();

	CNodeFinder&			operator=( const CNodeFinder& nf );
	CNodeFinder&			operator=( CNodeFinder&& nf );

	///////////////////////////////////////////////////////////////////////////
	// Life cycle control
	void					Initialize( CNavGraph& navgraph, AreaRegionId regionId );
	void					Clear() const;
	Bool					IsInitialized() const;
	Bool					IsValid() const;

	void					Invalidate() const;
	void					CompactData() const;

	void					AddDynamicElement( CNavNode* pNode ) const;
	void					RemoveDynamicElement( CNavNode* pNode ) const;

	// central initialization
	void					PreCenteralInitialization( CNavGraph& navgraph, AreaRegionId regionId );
	void					CentralInitializationBBoxUpdate( CNavNode& node ) const;
	void					CentralInitilzationComputeCelMap() const;
	void					CentralInitializationCollectNode( CNavNode& node ) const;
	void					PostCentralInitialization() const;
	///////////////////////////////////////////////////////////////////////////
	// Query functions
	CNavNode*				FindClosestNode( const Vector3& pos, Float& maxDist, NodeFlags forbiddenNodeFlags = NFG_FORBIDDEN_BY_DEFAULT ) const;
	CNavNode*				FindClosestNodeWithLinetest( const Vector3& pos, Float& maxDist, Float lineWidth, Uint32 lineTestFlags = CT_DEFAULT, NodeFlags forbiddenNodeFlags = NFG_FORBIDDEN_BY_DEFAULT ) const;
	CNavNode*				FindClosestNodeWithLinetestAndTolerance( const Vector3& pos, Float& maxDist, Float lineWidth, Vector3& outAccessiblePosition, Uint32 lineTestFlags = CT_DEFAULT, NodeFlags forbiddenNodeFlags = NFG_FORBIDDEN_BY_DEFAULT, Bool allowVerticalDiversity = true ) const;

	Bool					FindNClosestNodes( const Vector3& pos, Float& maxDist, Uint32 n, OutputVector& output, NodeFlags forbiddenNodeFlags = NFG_FORBIDDEN_BY_DEFAULT ) const;
	Bool					FindNClosestNodesWithLinetest( const Vector3& pos, Float& maxDist, Float lineWidth, Uint32 n, OutputVector& output, Uint32 lineTestFlags = CT_DEFAULT, NodeFlags forbiddenNodeFlags = NFG_FORBIDDEN_BY_DEFAULT ) const;
	// General query functions
	struct Acceptor : public Red::System::NonCopyable
	{
		virtual Bool Accept( const CNavNode& node )								= 0;
	};

	struct Handler : public Red::System::NonCopyable
	{
		virtual void Handle( CNavNode& node )									= 0;
	};

	CNavNode*				FindClosestNode( CWalkableSpotQueryRequest& request, Bool forceClosestPositionWithLinetestComputation = false ) const;

	//CNavNode*				FindClosestNode( const Vector3& pos, Float& maxDist, Acceptor& acceptor, NodeFlags forbiddenNodeFlags = NFG_FORBIDDEN_BY_DEFAULT ) const;
	//CNavNode*				FindClosestNode( const Vector3& pos, const Box& bbox, Float& maxDist, Acceptor& acceptor, NodeFlags forbiddenNodeFlags = NFG_FORBIDDEN_BY_DEFAULT ) const;
	//Bool					FindNClosestNodes( const Vector3& pos, Float& maxDist, Uint32 n, OutputVector& output, Acceptor& acceptor, NodeFlags forbiddenNodeFlags = NFG_FORBIDDEN_BY_DEFAULT ) const;

	void					IterateNodes( const Box& boundings, Handler& handler, NodeFlags forbiddenNodeFlags = NFG_FORBIDDEN_BY_DEFAULT ) const;
};


};				// namespace PathLib