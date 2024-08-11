#include "build.h"
#include "pathlibNodeFinder.h"

#include "pathlibAreaDescription.h"
#include "pathlibKDTree.h"
#include "pathlibNavgraph.h"
#include "pathlibNodeSet.h"
#include "pathlibSpatialQuery.h"
#include "pathlibWalkableSpotQueryRequest.h"

#include "pathlibNodeFinderFunctors.inl"

namespace PathLib
{

	CNodeFinder::CNodeFinder()
	: m_nodeMap( nullptr )
{

}
CNodeFinder::CNodeFinder( CNavGraph& navgraph, AreaRegionId regionId )
{
	m_nodeMap = new KD::CNodeMap( navgraph, regionId );
}
CNodeFinder::CNodeFinder( const CNodeFinder& navgraph )
	: m_nodeMap( nullptr )
{
}

CNodeFinder::CNodeFinder( CNodeFinder&& nf )
	: m_nodeMap( nf.m_nodeMap )
{
	nf.m_nodeMap = nullptr;
}

CNodeFinder::~CNodeFinder()
{
	if ( m_nodeMap )
	{
		delete m_nodeMap;
	}
}

CNodeFinder& CNodeFinder::operator=( const CNodeFinder& nf )
{
	m_nodeMap = nullptr;

	return *this;
}

CNodeFinder& CNodeFinder::operator=( CNodeFinder&& nf )
{
	if ( m_nodeMap )
	{
		delete m_nodeMap;
	}
	m_nodeMap = nf.m_nodeMap;
	nf.m_nodeMap = nullptr;

	return *this;
}

void CNodeFinder::Initialize( CNavGraph& navgraph, AreaRegionId regionId )
{
	if ( !m_nodeMap )
	{
		m_nodeMap = new KD::CNodeMap( navgraph, regionId );
	}

	m_nodeMap->Initialize();
}
void CNodeFinder::Clear() const
{
	if ( m_nodeMap )
	{
		m_nodeMap->Clear();
	}
}
Bool CNodeFinder::IsInitialized() const
{
	return m_nodeMap && m_nodeMap->IsInitialized();
}

Bool CNodeFinder::IsValid() const
{
	return m_nodeMap && m_nodeMap->IsPopulated();
}

void CNodeFinder::Invalidate() const
{
	if ( m_nodeMap )
	{
		m_nodeMap->Invalidate();
	}
}
void CNodeFinder::CompactData() const
{
	m_nodeMap->CompactData();
}

void CNodeFinder::AddDynamicElement( CNavNode* node ) const
{
	m_nodeMap->NodeCreated( node );
}
void CNodeFinder::RemoveDynamicElement( CNavNode* node ) const
{
	m_nodeMap->NodeRemoved( node );
}

void CNodeFinder::PreCenteralInitialization( CNavGraph& navgraph, AreaRegionId regionId )
{
	if ( !m_nodeMap )
	{
		m_nodeMap = new KD::CNodeMap( navgraph, regionId );
	}
	m_nodeMap->PreCenteralInitialization( navgraph, regionId );
}
void CNodeFinder::CentralInitializationBBoxUpdate( CNavNode& node ) const
{
	m_nodeMap->CentralInitializationBBoxUpdate( node );
}
void CNodeFinder::CentralInitilzationComputeCelMap() const
{
	m_nodeMap->CentralInitilzationComputeCelMap();
}
void CNodeFinder::CentralInitializationCollectNode( CNavNode& node ) const
{
	m_nodeMap->CentralInitializationCollectNode( node );
}
void CNodeFinder::PostCentralInitialization() const
{
	m_nodeMap->PostCentralInitialization();
}

///////////////////////////////////////////////////////////////////////////////
// Queries
CNavNode* CNodeFinder::FindClosestNode( const Vector3& pos, Float& maxDist, NodeFlags forbiddenNodeFlags ) const
{
	KD::CNodeMap::Filtering3DAcceptor acceptor( pos, forbiddenNodeFlags );
	return m_nodeMap->FindClosestNode( pos, maxDist, acceptor );
}

CNavNode* CNodeFinder::FindClosestNodeWithLinetest( const Vector3& pos, Float& maxDist, Float lineWidth, Uint32 lineTestFlags, NodeFlags forbiddenNodeFlags ) const
{
	NodeFinderHelper::WidelineAcceptor acceptor( pos, m_nodeMap->GetNavgraph().GetArea(), lineWidth, lineTestFlags, forbiddenNodeFlags );
	return m_nodeMap->FindClosestNode( pos, maxDist, acceptor );
}

CNavNode* CNodeFinder::FindClosestNodeWithLinetestAndTolerance( const Vector3& pos, Float& maxDist, Float lineWidth, Vector3& outAccessiblePosition, Uint32 lineTestFlags, NodeFlags forbiddenNodeFlags, Bool allowVerticalDiversity ) const
{
	Float maxNodesDist = m_nodeMap->GetNavgraph().GetMaxNodesDistance();

	Float testDist = maxDist + maxNodesDist;
	Box testBBox( pos, testDist );
	Float minZ = pos.Z - (allowVerticalDiversity ? testDist : 1.5f );
	Float maxZ = pos.Z + (allowVerticalDiversity ? testDist : 2.f );

	
	NodeFinderHelper::BaseFlagTest flagTest( forbiddenNodeFlags );
	NodeFinderHelper::ClosestPositionWithLinetestPos closestPositionWithLinetest( m_nodeMap->GetNavgraph().GetArea(), pos, lineWidth, lineTestFlags );
	NodeFinderHelper::DistFund3DExtended distFun( maxNodesDist );
	NodeFinderHelper::MainZTest mainTest( minZ, maxZ );
	

	CNavNode* retNode = NodeFinderHelper::PerformQueryInBoundings( m_nodeMap, pos, testBBox, testDist, distFun, flagTest, closestPositionWithLinetest, mainTest, outAccessiblePosition );
	if ( retNode )
	{
		maxDist = testDist - maxNodesDist;
		ASSERT( maxDist >= 0.f );
		if ( maxDist < 0.f )
		{
			maxDist = 0.f;
		}
	}

	return retNode;
}

Bool CNodeFinder::FindNClosestNodes( const Vector3& pos, Float& maxDist, Uint32 n, OutputVector& output, NodeFlags forbiddenNodeFlags ) const
{
	output.Resize( n );

	KD::CNodeMap::Filtering3DAcceptor acceptor( pos, forbiddenNodeFlags );
	Uint32 returnedElements = m_nodeMap->FindNClosestNodes( pos, maxDist, acceptor, n, output.TypedData() );
	
	output.Resize( returnedElements );

	return returnedElements != 0;
}

Bool CNodeFinder::FindNClosestNodesWithLinetest( const Vector3& pos, Float& maxDist, Float lineWidth, Uint32 n, OutputVector& output, Uint32 lineTestFlags, NodeFlags forbiddenNodeFlags ) const
{
	output.Resize( n );

	NodeFinderHelper::WidelineAcceptor acceptor( pos, m_nodeMap->GetNavgraph().GetArea(), lineWidth, lineTestFlags, forbiddenNodeFlags );
	Uint32 returnedElements = m_nodeMap->FindNClosestNodes( pos, maxDist, acceptor, n, output.TypedData() );

	output.Resize( returnedElements );

	return returnedElements != 0;
}



CNavNode* CNodeFinder::FindClosestNode( CWalkableSpotQueryRequest& request, Bool forceClosestPositionWithLinetestComputation ) const
{
	struct RequestTest
	{
		CWalkableSpotQueryRequest&	m_request;

		RequestTest( CWalkableSpotQueryRequest& request )
			: m_request( request )
		{}

		RED_INLINE Bool MainTest( const Vector3& pos ) const
		{
			return m_request.AcceptPosition( pos );
		}
	};

	NodeFinderHelper::BaseFlagTest flagTest( request.m_forbiddenNodeFlags );
	RequestTest mainTest( request );

	CNavNode* outputNode;
	Vector3 outputPosition;
	if( request.IsComputingClosestPositionWithClearLinetest() || forceClosestPositionWithLinetestComputation )
	{
		CNavGraph& navgraph = m_nodeMap->GetNavgraph();
		Float maxNodesDistance = navgraph.GetMaxNodesDistance();
		Box testBBox = request.m_testBox.Extrude( maxNodesDistance );
		NodeFinderHelper::ClosestPositionWithLinetestPos nodePos( navgraph.GetArea(), request.m_destinationPos, request.m_personalSpace, request.m_collisionFlags );
		NodeFinderHelper::MainBBoxTest bboxTest( request.m_testBox );
		NodeFinderHelper::MainCompositeTest< NodeFinderHelper::MainBBoxTest, RequestTest > compositeTest( bboxTest, mainTest );

		if ( request.IsUsing3DDistance() )
		{
			NodeFinderHelper::DistFun3D dist;

			outputNode = NodeFinderHelper::PerformQueryInBoundings( m_nodeMap, request.m_destinationPos, testBBox, request.m_maxDist, dist, flagTest, nodePos, compositeTest, outputPosition );
		}
		else
		{
			NodeFinderHelper::DistFun2D dist;

			outputNode = NodeFinderHelper::PerformQueryInBoundings( m_nodeMap, request.m_destinationPos, testBBox, request.m_maxDist, dist, flagTest, nodePos, compositeTest, outputPosition );
		}
	}
	else
	{
		NodeFinderHelper::BaseZTest zTest( request.m_testBox.Min.Z, request.m_testBox.Max.Z );
		NodeFinderHelper::BaseCompositeTest< NodeFinderHelper::BaseZTest, NodeFinderHelper::BaseFlagTest > baseTest( zTest, flagTest );
		NodeFinderHelper::BaseNodePos nodePos; 
		if ( request.IsUsing3DDistance() )
		{
			NodeFinderHelper::DistFun3D dist;

			outputNode = NodeFinderHelper::PerformQueryInBoundings( m_nodeMap, request.m_destinationPos, request.m_testBox, request.m_maxDist, dist, baseTest, nodePos, mainTest, outputPosition );
		}
		else
		{
			NodeFinderHelper::DistFun2D dist;

			outputNode = NodeFinderHelper::PerformQueryInBoundings( m_nodeMap, request.m_destinationPos, request.m_testBox, request.m_maxDist, dist, baseTest, nodePos, mainTest, outputPosition );
		}
	}

	

	if ( outputNode )
	{
		request.m_output = outputPosition;
		request.m_outputSuccess = true;
		request.m_outAreaId = m_nodeMap->GetNavgraph().GetArea()->GetId();
	}
	return outputNode;
}


//CNavNode* CNodeFinder::FindClosestNode( const Vector3& pos, Float& maxDist, Acceptor& acceptor, NodeFlags forbiddenNodeFlags ) const
//{
//	struct CelAcceptor : public KD::CNodeMap::DefaultPosAcceptor
//	{
//		Acceptor&		m_acceptor;
//		NodeFlags		m_forbiddenNodeFlags;
//
//		CelAcceptor( const Vector3& pos, Acceptor& acceptor, NodeFlags forbiddenNodeFlags )
//			: DefaultPosAcceptor( pos )
//			, m_acceptor( acceptor )
//			, m_forbiddenNodeFlags( forbiddenNodeFlags ) {}
//
//		Float operator()( CNavNode* node )
//		{
//			if ( !node->HaveAnyFlag( m_forbiddenNodeFlags ) && m_acceptor.Accept( *node ) )
//			{
//				return m_acceptor.Is3D() ? Dist3DSq( node ) : Dist2DSq( node );
//			}
//			return FLT_MAX;
//		}
//		
//	} celAcceptor( pos, acceptor, forbiddenNodeFlags );
//
//	return m_nodeMap->FindClosestNode( pos, maxDist, celAcceptor );
//}
//
//CNavNode* CNodeFinder::FindClosestNode( const Vector3& pos, const Box& bbox, Float& maxDist, Acceptor& acceptor, NodeFlags forbiddenNodeFlags ) const
//{
//	struct CelAcceptor : public KD::CNodeMap::DefaultPosAcceptor
//	{
//		Acceptor&		m_acceptor;
//		Box				m_bbox;
//		NodeFlags		m_forbiddenNodeFlags;
//
//		CelAcceptor( const Vector3& pos, Acceptor& acceptor, const Box& boundings, NodeFlags forbiddenNodeFlags )
//			: KD::CNodeMap::DefaultPosAcceptor( pos )
//			, m_acceptor( acceptor )
//			, m_bbox( boundings )
//			, m_forbiddenNodeFlags( forbiddenNodeFlags ) {}
//
//		Float operator()( CNavNode* node )
//		{
//			if ( !node->HaveAnyFlag( m_forbiddenNodeFlags ) )
//			{
//				Vector3 nodePos = node->GetPosition();
//				if ( m_bbox.Contains( nodePos ) && m_acceptor.Accept( *node ) )
//				{
//					return m_acceptor.Is3D() ? Dist3DSq( node ) : Dist2DSq( node );
//				}
//			}
//			
//			return FLT_MAX;
//		}
//	} celAcceptor( pos, acceptor, bbox, forbiddenNodeFlags );
//
//	return m_nodeMap->FindClosestNodeInBoundings( pos, bbox, maxDist, celAcceptor );
//}
//
//Bool CNodeFinder::FindNClosestNodes( const Vector3& pos, Float& maxDist, Uint32 n, OutputVector& output, Acceptor& acceptor, NodeFlags forbiddenNodeFlags ) const
//{
//	output.Resize( n );
//
//	struct CelAcceptor : public KD::CNodeMap::DefaultPosAcceptor
//	{
//		Acceptor&		m_acceptor;
//		NodeFlags		m_forbiddenNodeFlags;
//
//		CelAcceptor( const Vector3& pos, Acceptor& acceptor, NodeFlags forbiddenNodeFlags )
//			: DefaultPosAcceptor( pos )
//			, m_acceptor( acceptor )
//			, m_forbiddenNodeFlags( forbiddenNodeFlags ) {}
//
//		Float operator()( CNavNode* node )
//		{
//			if ( m_acceptor.Accept( *node ) )
//			{
//				return m_acceptor.Is3D() ? Dist3DSq( node ) : Dist2DSq( node );
//			}
//			return FLT_MAX;
//		}
//
//	} celAcceptor( pos, acceptor, forbiddenNodeFlags );
//
//	Uint32 returnedElements = m_nodeMap->FindNClosestNodes( pos, maxDist, celAcceptor, n, output.TypedData() );
//
//	output.Resize( returnedElements );
//
//	return returnedElements != 0;
//}

void CNodeFinder::IterateNodes( const Box& boundings, Handler& handler, NodeFlags forbiddenFlags ) const
{
	struct CelHandler : public Red::System::NonCopyable
	{
		Handler&		m_handler;
		Box				m_bbox;
		NodeFlags		m_forbiddenNodeFlags;

		CelHandler( Handler& handler, const Box& boundings, NodeFlags forbiddenFlags )
			: m_handler( handler )
			, m_bbox( boundings )
			, m_forbiddenNodeFlags( forbiddenFlags ) {}

		void operator()( CNavNode* node )
		{
			if ( !node->HaveAnyFlag( m_forbiddenNodeFlags ) )
			{
				if ( m_bbox.Contains( node->GetPosition() ) )
				{
					m_handler.Handle( *node );
				}
			}
		}
		
	} celHandler( handler, boundings, forbiddenFlags );

	m_nodeMap->IterateNodes( boundings, celHandler );
}


};			// namespace PathLib