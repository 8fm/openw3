/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

namespace PathLib
{

namespace NodeFinderHelper
{
	struct WidelineAcceptor : public KD::CNodeMap::Filtering3DAcceptor
	{
		CAreaDescription*		m_area;
		Float					m_personalSpace;
		Uint32					m_testFlags;
		NodeFlags				m_forbiddenNodeFlags;

		RED_INLINE WidelineAcceptor( const Vector3& pos, CAreaDescription* area, Float ps, Uint32 flags = CT_DEFAULT, NodeFlags forbiddenNodeFlags = NFG_FORBIDDEN_BY_DEFAULT )
			: KD::CNodeMap::Filtering3DAcceptor( pos, forbiddenNodeFlags )
			, m_area( area )
			, m_personalSpace( ps )
			, m_testFlags( flags ) {}

		RED_INLINE Float operator()( CNavNode* node )
		{
			if ( !CheckFlags( node ) )
			{
				return FLT_MAX;
			}
			CWideLineQueryData::MultiArea query( CWideLineQueryData( m_testFlags, node->GetPosition(), m_pos, m_personalSpace ) );
			if ( !m_area->TMultiAreaQuery( query ) )
			{
				return FLT_MAX;
			}

			return Filtering3DAcceptor::DistSq( node );
		}
	};

	struct WidelineAcceptorWithTolerance : public WidelineAcceptor
	{
		Float		m_tolerance;
		Float		m_toleranceSq;
		Vector3		m_outPosition;
		Float		m_maxDistSq;

		RED_INLINE WidelineAcceptorWithTolerance( const Vector3& pos, CAreaDescription* area, Float ps, Float tolerance, Float maxDist, Uint32 flags = CT_DEFAULT, NodeFlags forbiddenNodeFlags = NFG_FORBIDDEN_BY_DEFAULT )
			: WidelineAcceptor( pos, area, ps, flags, forbiddenNodeFlags )
			, m_tolerance( tolerance )
			, m_toleranceSq( tolerance*tolerance )
			, m_maxDistSq( maxDist*maxDist ) {}

		Float operator()( CNavNode* node )
		{
			if ( !CheckFlags( node ) )
			{
				return FLT_MAX;
			}

			const Vector3& nodePos = node->GetPosition();
			Float distSq = (nodePos - m_pos).SquareMag();

			if ( distSq <= m_toleranceSq )
			{
				if( distSq >= m_maxDistSq )
				{
					return FLT_MAX;
				}
				m_maxDistSq = distSq;
				m_outPosition = nodePos;
				return distSq;
			}

			if ( m_maxDistSq <= m_toleranceSq )
			{
				return FLT_MAX;
			}

			Vector2 nodeDiff = m_pos.AsVector2() - nodePos.AsVector2();

			Float dist = sqrt( distSq );
			Float ratio = (dist - m_tolerance) / dist;

			Vector3 testPos;
			testPos.AsVector2() = nodePos.AsVector2() + nodeDiff * ratio;

			m_area->VComputeHeightFrom( testPos.AsVector2(), nodePos, testPos.Z );

			CWideLineQueryData lineQuery( CT_IGNORE_OTHER_AREAS, testPos, nodePos, m_personalSpace );
			if ( m_area->VSpatialQuery( lineQuery ) )
			{
				m_maxDistSq = m_toleranceSq;
				m_outPosition = testPos;
				return m_toleranceSq;
			}
			return FLT_MAX;
		}
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Distance functors
	struct DistFun3D
	{
		static RED_INLINE Float	DistSq( const Vector3& v0, const Vector3& v1 )						{ return (v0 - v1).SquareMag(); }
	};

	struct DistFun2D
	{
		static RED_INLINE Float	DistSq( const Vector3& v0, const Vector3& v1 )						{ return (v0.AsVector2() - v1.AsVector2()).SquareMag(); }
	};

	struct DistFund3DExtended
	{
		RED_INLINE DistFund3DExtended( Float ext )	
			: m_extDist( ext )																			{}

		RED_INLINE Float			DistSq( const Vector3& v0, const Vector3& v1 )
		{
			Float dist3DSq = (v0 - v1).SquareMag();
			Float distExt = sqrt( dist3DSq ) + m_extDist;
			return distExt*distExt;
		}

		Float						m_extDist;
	};

	struct DistFun2DExtended
	{
		RED_INLINE DistFun2DExtended( Float ext )
			: m_extDist( ext )																			{}

		RED_INLINE Float			DistSq( const Vector3& v0, const Vector3& v1 )
		{
			Float dist2DSq = (v0.AsVector2() - v1.AsVector2()).SquareMag();
			Float distExt = sqrt( dist2DSq ) + m_extDist;
			return distExt*distExt;
		}

		Float						m_extDist;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Node functors
	struct BaseNodePos
	{
		static RED_INLINE void	NodePos( const CNavNode& node, Vector3& outPos )					{ outPos = node.GetPosition(); }
	};

	struct ClosestPositionWithLinetestPos
	{
		CAreaDescription*			m_area;
		Vector3						m_desiredSpot;
		Float						m_personalSpace;
		Uint32						m_collisionFlags;

		RED_INLINE ClosestPositionWithLinetestPos( CAreaDescription* area, const Vector3& desiredPos, Float personalSpace, Uint32 collisionFlags )
			: m_area( area )
			, m_desiredSpot( desiredPos )
			, m_personalSpace( personalSpace )
			, m_collisionFlags( collisionFlags )														{}

		RED_INLINE void NodePos( const CNavNode& node, Vector3& outPos ) const
		{
			const Vector3& nodePos = node.GetPosition();
			CClearWideLineInDirectionQueryData::MultiArea query(
				CClearWideLineInDirectionQueryData( m_collisionFlags, nodePos, m_desiredSpot, m_personalSpace )
				);
			m_area->TMultiAreaQuery( query );
			query.GetSubQuery().GetHitPos( outPos );
			m_area->VComputeHeightFrom( outPos.AsVector2(), nodePos, outPos.Z );
		}
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Base acceptors
	// Thats the acceptors that take away 'cheap' test done b4 ndoe distance calculation
	struct BaseFlagTest
	{
		RED_INLINE BaseFlagTest( NodeFlags forbiddenFlags )
			: m_forbiddenFlags( forbiddenFlags )														{}

		NodeFlags					m_forbiddenFlags;

		RED_INLINE Bool				BaseTest( CNavNode& node ) const									{ return !node.HaveAnyFlag( m_forbiddenFlags ); }
	};

	struct BaseZTest
	{
		RED_INLINE BaseZTest( Float zMin, Float zMax )
			: m_zMin( zMin )
			, m_zMax( zMax )																			{}

		RED_INLINE Bool				BaseTest( CNavNode& node ) const									{ Float z = node.GetPosition().Z; return z >= m_zMin && z <= m_zMax; }

		Float m_zMin;
		Float m_zMax;
	};

	template < class T0, class T1 >
	struct BaseCompositeTest
	{
		T0&		m_t0;
		T1&		m_t1;

		RED_INLINE BaseCompositeTest( T0& t0, T1& t1 )
			: m_t0( t0 )
			, m_t1( t1 )																				{}

		RED_INLINE Bool				BaseTest( CNavNode& node ) const
		{
			return m_t0.BaseTest( node ) && m_t1.BaseTest( node );
		}
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Main acceptors
	// Heavy acceptors doing last calculations, after checking node distance and position
	struct EmptyMainTest
	{
		static RED_INLINE Bool		MainTest( const Vector3& pos )										{ return true; }
	};

	struct MainBBoxTest
	{
		RED_INLINE MainBBoxTest( const Box& bbox )
			: m_bbox( bbox )																			{}
		const Box&					m_bbox;

		RED_INLINE Bool				MainTest( const Vector3& pos ) const								{ return m_bbox.Contains( pos ); }
	};

	struct MainZTest
	{
		RED_INLINE MainZTest( Float zMin, Float zMax )
			: m_zMin( zMin )
			, m_zMax( zMax )																			{}

		RED_INLINE Bool				MainTest( const Vector3& pos ) const								{ Float z = pos.Z; return z >= m_zMin && z <= m_zMax; }

		Float m_zMin;
		Float m_zMax;
	};

	struct MainWideLineTest
	{
		RED_INLINE MainWideLineTest( CAreaDescription* area, const Vector3& destination, Float ps, Uint32 lineTestFlags )
			: m_destination( destination )
			, m_area( area )
			, m_personalSpace( ps )
			, m_linetestFlags( lineTestFlags )															{}

		CAreaDescription*			m_area;
		Vector3						m_destination;
		Float						m_personalSpace;
		Uint32						m_linetestFlags;

		RED_INLINE Bool				MainTest( const Vector3& pos ) const
		{
			CWideLineQueryData::MultiArea query( CWideLineQueryData( m_linetestFlags, pos, m_destination, m_personalSpace ) );
			return m_area->TMultiAreaQuery( query );
		}
	};

	template < class T0, class T1 >
	struct MainCompositeTest
	{
		T0&		m_t0;
		T1&		m_t1;

		RED_INLINE MainCompositeTest( T0& t0, T1& t1 )
			: m_t0( t0 )
			, m_t1( t1 )																				{}

		RED_INLINE Bool				MainTest( const Vector3& pos ) const
		{
			return m_t0.MainTest( pos ) && m_t1.MainTest( pos );
		}
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Query
	// Main composite object that represents node finder query
	template < class TDist, class TBaseTest, class TNodePos, class TMainTest >
	struct Query
	{
		Vector3				m_destination;
		Float				m_closestSoFarSq;
		Vector3&			m_outputPosition;

		TDist&				m_dist;
		TBaseTest&			m_baseTest;
		TNodePos&			m_nodePos;
		TMainTest&			m_mainTest;

		
		Query( const Vector3& dest, Float maxDist, TDist& dist, TBaseTest& baseTest, TNodePos& nodePos, TMainTest& mainTest, Vector3& outputPosition )
			: m_destination( dest )
			, m_closestSoFarSq( maxDist*maxDist )
			, m_outputPosition( outputPosition )
			, m_dist( dist )
			, m_baseTest( baseTest )
			, m_nodePos( nodePos )
			, m_mainTest( mainTest )
		{
		}

		static CNavNode* InvalidElement() { return nullptr; }

		RED_INLINE Float operator()( CNavNode* node )
		{
			if ( !m_baseTest.BaseTest( *node ) )
			{
				return FLT_MAX;
			}

			Vector3 nodePos;
			m_nodePos.NodePos( *node, nodePos );

			// not interesting
			Float distSq = m_dist.DistSq( nodePos, m_destination );
			if ( distSq >= m_closestSoFarSq )
			{
				return FLT_MAX;
			}

			if ( !m_mainTest.MainTest( nodePos ) )
			{
				return FLT_MAX;
			}

			m_closestSoFarSq = distSq;
			m_outputPosition = nodePos;

			return distSq;
		}
	};
	
	template < class TDist, class TBaseTest, class TNodePos, class TMainTest >
	RED_INLINE CNavNode*
		PerformQueryInBoundings( KD::CNodeMap* nodeMap, const Vector3& destPos, const Box& bbox, Float& maxDist, TDist& dist, TBaseTest& baseTest, TNodePos& nodePos, TMainTest& mainTest, Vector3& outputPosition )
	{
		auto query = Query< TDist, TBaseTest, TNodePos, TMainTest >( destPos, maxDist, dist, baseTest, nodePos, mainTest, outputPosition );
		return nodeMap->FindClosestNodeInBoundings( destPos, bbox, maxDist, query );
	}

};		// namespace NodeFinderHelper


};		// namespace PathLib