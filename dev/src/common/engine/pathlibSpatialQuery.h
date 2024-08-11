#pragma once
#include "../core/mathUtils.h"
#include "pathlib.h"

namespace PathLib
{

////////////////////////////////////////////////////////////////////////////

struct CSpatialQueryData
{
	enum { AUTOFAIL = true };

	CSpatialQueryData()														{}
	CSpatialQueryData( Uint32 flags, const Vector3& basePos )
		: m_flags( flags ), m_basePos( basePos )							{}

	template < class Transformer >
	RED_INLINE void Transform( const Transformer& functor )					{ functor.WorldToLocal( m_basePos ); }
	template < class Transformer >
	RED_INLINE void CancelTransform( const Transformer& functor )			{ functor.LocalToWorld( m_basePos ); }
	template < class Transformer >
	RED_INLINE void TransformOutput( const Transformer& functor )			{}
	template < class Transformer >
	RED_INLINE void CancelTransformOutput( const Transformer& functor )	{}
	RED_INLINE void NoticeAreaEdgeHit() 									{ m_flags |= CT_NO_ENDPOINT_TEST; }

	RED_INLINE void ComputeBBox( Vector3* bbox ) const						{ bbox[ 0 ].Set( -FLT_MAX, -FLT_MAX, -FLT_MAX ); bbox[ 1 ].Set( FLT_MAX, FLT_MAX, FLT_MAX ); }
	RED_INLINE void ComputeBBox( Vector2* bbox ) const						{ bbox[ 0 ].Set( -FLT_MAX, -FLT_MAX ); bbox[ 1 ].Set( FLT_MAX, FLT_MAX ); }

	RED_INLINE Bool IntersectRect( const Vector2& rectMin, const Vector2& rectMax ) const	{ ASSERT( false, TXT("REIMPLEMENT INTERSECTION!\n") ); ASSUME( false ); return false; }
	RED_INLINE Bool Intersect( const Vector2& v1, const Vector2& v2 ) const					{ ASSERT( false, TXT("REIMPLEMENT INTERSECTION!\n") ); ASSUME( false ); return false; }

	Uint32			m_flags;
	Vector3			m_basePos;
};

////////////////////////////////////////////////////////////////////////////
struct CMultiAreaSpatialQuery
{
protected:
	static const Int32 MAX_AREAS = 4;

	AreaId						m_startingArea;
	Int16						m_areasVisited;
	Int16						m_areasHandled;
	AreaId						m_areasStack[ MAX_AREAS ];
	Vector3						m_areasEntyPoints[ MAX_AREAS ];

public:
	CMultiAreaSpatialQuery()
		: m_startingArea( INVALID_AREA_ID )
		, m_areasVisited( 0 )
		, m_areasHandled( 0 )												{}

	RED_INLINE void SetStartingArea( AreaId areaId )						{ m_startingArea = areaId; }
	RED_INLINE void PushAreaUnique( AreaId area, const Vector3& entryPoint );
	RED_INLINE void PushAreaSafe( AreaId area, const Vector3& entryPoint );
	RED_INLINE Bool PopArea( AreaId& outArea, Vector3& outEntryPoint );

	RED_INLINE Bool VisitedArea( AreaId area ) const;
	RED_INLINE Bool HasAreaCapacity()										{ return m_areasVisited < MAX_AREAS; }
};


template < class TQuery >
struct TMultiAreaSpatialQuery : public CMultiAreaSpatialQuery
{

public:
	TQuery						m_query;

	typedef TQuery SubQuery;
	enum { AUTOFAIL = TQuery::AUTOFAIL };

	RED_INLINE TMultiAreaSpatialQuery( TQuery&& query )
		: m_query( Move( query ) )											{ m_query.m_flags |= CT_MULTIAREA; }
	
	RED_INLINE const SubQuery& GetSubQuery() const							{ return m_query; }
	RED_INLINE SubQuery& GetSubQuery()										{ return m_query; }

	RED_INLINE static TMultiAreaSpatialQuery* GetMultiAreaData( TQuery& query );

	RED_INLINE void ComputeBBox( Vector3* bbox ) const						{ m_query.ComputeBBox( bbox ); }
	RED_INLINE void ComputeBBox( Vector2* bbox ) const						{ m_query.ComputeBBox( bbox ); }

	RED_INLINE void NoticeAreaEdgeHit() 									{ m_query.NoticeAreaEdgeHit(); }

	RED_INLINE Bool IntersectRect( const Vector2& rectMin, const Vector2& rectMax ) const	{ return m_query.IntersectRect( rectMin, rectMax ); }
	RED_INLINE Bool Intersect( const Vector2& v1, const Vector2& v2 ) const					{ return m_query.Intersect( v1, v2 ); }
};

// simplified dummy interface that we can send as a template parameter to avoid compilation error
struct CDummyMultiAreaQueryInterface
{
	RED_INLINE void PushAreaUnique( AreaId area, const Vector3& entryPoint ){ ASSERT( false ); ASSUME( false ); }
	RED_INLINE void PushAreaSafe( AreaId area, const Vector3& entryPoint )	{ ASSERT( false ); ASSUME( false ); }
	RED_INLINE void NoticeAreaEdgeHit()										{ ASSERT( false ); ASSUME( false ); }
};


////////////////////////////////////////////////////////////////////////////

struct CCircleQueryData : CSpatialQueryData
{
private:
	typedef CSpatialQueryData Super;
public:
	typedef TMultiAreaSpatialQuery< CCircleQueryData > MultiArea;

	CCircleQueryData()														{}
	CCircleQueryData( Uint32 flags, const Vector3& center, Float radius )
		: CSpatialQueryData( flags, center )
		, m_circleCenter( center )
		, m_radius( radius )												{}

	template < class Transformer >
	RED_INLINE void Transform( const Transformer& functor )					{ Super::Transform( functor ); functor.WorldToLocal( m_circleCenter ); }
	template < class Transformer >
	RED_INLINE void CancelTransform( const Transformer& functor )			{ Super::CancelTransform( functor ); functor.LocalToWorld( m_circleCenter ); }

	RED_INLINE void ComputeBBox( Vector3* bbox ) const						{ bbox[ 0 ] = m_circleCenter - Vector3( m_radius, m_radius, 0.f ); bbox[ 1 ] = m_circleCenter + Vector3( m_radius, m_radius, DEFAULT_AGENT_HEIGHT ); }
	RED_INLINE void ComputeBBox( Vector2* bbox ) const						{ bbox[ 0 ] = m_circleCenter.AsVector2() - Vector2( m_radius, m_radius ); bbox[ 1 ] = m_circleCenter.AsVector2() + Vector2( m_radius, m_radius ); }

	RED_INLINE Bool IntersectRect( const Vector2& rectMin, const Vector2& rectMax ) const;
	RED_INLINE Bool IntersectCircle( const Vector2& v, Float radius ) const;
	RED_INLINE Bool Intersect( const Vector2& v1, const Vector2& v2 ) const;

	RED_INLINE void ClosestSpotRect( const Vector2& rectMin, const Vector2& rectMax, Vector2& outClosestSpot ) const;
	RED_INLINE void ClosestSpotCircle( const Vector2& v, Float radius, Vector2& outClosestSpot ) const;
	RED_INLINE void ClosestSpotSegment( const Vector2& v1, const Vector2& v2, Vector2& outClosestSpot ) const;

	Vector3			m_circleCenter;
	Float			m_radius;

};

////////////////////////////////////////////////////////////////////////////

struct CClosestObstacleCircleQueryData : CCircleQueryData
{
private:
	typedef CCircleQueryData Super;
public:
	enum { AUTOFAIL = false };

	typedef TMultiAreaSpatialQuery< CClosestObstacleCircleQueryData > MultiArea;

	CClosestObstacleCircleQueryData()										{}
	CClosestObstacleCircleQueryData( Uint32 flags, const Vector3& center, Float radius )
		: CCircleQueryData( flags, center, radius )
		, m_closestDistSq( radius*radius )
		, m_obstacleHit( false )											{}

	template < class Transformer >
	RED_INLINE void TransformOutput( const Transformer& functor )			{ Super::TransformOutput( functor ); functor.WorldToLocal( m_pointOut ); }
	template < class Transformer >
	RED_INLINE void CancelTransformOutput( const Transformer& functor )		{ Super::CancelTransformOutput( functor ); functor.LocalToWorld( m_pointOut ); }

	RED_INLINE Bool HasHit() const											{ return m_obstacleHit; }

	RED_INLINE Bool IntersectRect( const Vector2& rectMin, const Vector2& rectMax ) const;
	RED_INLINE Bool IntersectCircle( const Vector2& v, Float radius ) const;
	RED_INLINE Bool Intersect( const Vector2& v1, const Vector2& v2 ) const;

	Float			m_closestDistSq;
	Vector3			m_pointOut;
	Bool			m_obstacleHit;
};

////////////////////////////////////////////////////////////////////////////

struct CCollectGeometryInCirceQueryData : public CCircleQueryData
{
private:
	typedef CCircleQueryData Super;
public:
	enum { AUTOFAIL = false };

	typedef TMultiAreaSpatialQuery< CCollectGeometryInCirceQueryData > MultiArea;

	CCollectGeometryInCirceQueryData()										{}
	CCollectGeometryInCirceQueryData( Uint32 flags, const Vector3& center, Float radius )
		: CCircleQueryData( flags, center, radius )							{}

	template < class Transformer >
	RED_INLINE void TransformOutput( const Transformer& functor )			{ Super::TransformOutput( functor ); ForEach( m_output, [ &functor ] ( Vector& v ) { functor.WorldToLocal( v.AsVector3() ); } ); }
	template < class Transformer >
	RED_INLINE void CancelTransformOutput( const Transformer& functor )		{ Super::CancelTransformOutput( functor ); ForEach( m_output, [ &functor ] ( Vector& v ) { functor.LocalToWorld( v.AsVector3() ); } ); }

	RED_INLINE Bool HasHit() const											{ return !m_output.Empty(); }

	TDynArray< Vector >			m_output;
};

////////////////////////////////////////////////////////////////////////////

struct CLineQueryData : public CSpatialQueryData
{
private:
	typedef CSpatialQueryData Super;
public:
	typedef TMultiAreaSpatialQuery< CLineQueryData > MultiArea;

	CLineQueryData()														{}
	CLineQueryData( Uint32 flags, const Vector3& v1, const Vector3& v2 )
		: CSpatialQueryData( flags, v1 )
		, m_v1( v1 )
		, m_v2( v2 )														{}

	template < class Transformer >
	RED_INLINE void Transform( const Transformer& functor )					{ Super::Transform( functor ); functor.WorldToLocal( m_v1 ); functor.WorldToLocal( m_v2 ); }
	template < class Transformer >
	RED_INLINE void CancelTransform( const Transformer& functor )			{ Super::CancelTransform( functor ); functor.LocalToWorld( m_v1 ); functor.LocalToWorld( m_v2 ); }

	RED_INLINE void ComputeBBox( Vector3* bbox ) const						{ bbox[ 0 ].Set( Min(m_v1.X, m_v2.X), Min(m_v1.Y, m_v2.Y), Min(m_v1.Z, m_v2.Z) ); bbox[ 1 ].Set( Max(m_v1.X, m_v2.X), Max(m_v1.Y, m_v2.Y), Max(m_v1.Z, m_v2.Z) + DEFAULT_AGENT_HEIGHT ); }
	RED_INLINE void ComputeBBox( Vector2* bbox ) const						{ bbox[ 0 ].Set( Min(m_v1.X, m_v2.X), Min(m_v1.Y, m_v2.Y) ); bbox[ 1 ].Set( Max(m_v1.X, m_v2.X), Max(m_v1.Y, m_v2.Y) ); }

	RED_INLINE Bool IntersectRect( const Vector2& rectMin, const Vector2& rectMax ) const;
	RED_INLINE Bool Intersect( const Vector2& v1, const Vector2& v2 ) const;

	Vector3			m_v1;
	Vector3			m_v2;
};

////////////////////////////////////////////////////////////////////////////

struct CWideLineQueryData : public CLineQueryData
{
private:
	typedef CLineQueryData Super;
public:
	typedef TMultiAreaSpatialQuery< CWideLineQueryData > MultiArea;

	CWideLineQueryData()													{}
	CWideLineQueryData( Uint32 flags, const Vector3& v1, const Vector3& v2, Float radius )
		: CLineQueryData( flags, v1, v2 )
		, m_radius( radius )												{}

	RED_INLINE void ComputeBBox( Vector3* bbox ) const						{ Super::ComputeBBox( bbox ); Vector2 rad( m_radius, m_radius ); bbox[ 0 ].AsVector2() -= rad; bbox[ 1 ].AsVector2() += rad; }
	RED_INLINE void ComputeBBox( Vector2* bbox ) const						{ Super::ComputeBBox( bbox ); Vector2 rad( m_radius, m_radius ); bbox[ 0 ] -= rad; bbox[ 1 ] += rad; }

	RED_INLINE Bool IntersectRect( const Vector2& rectMin, const Vector2& rectMax ) const;
	RED_INLINE Bool IntersectCircle( const Vector2& v, Float radius ) const;
	RED_INLINE Bool Intersect( const Vector2& v1, const Vector2& v2 ) const;

	Float			m_radius;
};

////////////////////////////////////////////////////////////////////////////

struct CClosestObstacleWideLineQueryData : public CWideLineQueryData
{
private:
	typedef CWideLineQueryData Super;
public:
	enum { AUTOFAIL = false };

	typedef TMultiAreaSpatialQuery< CClosestObstacleWideLineQueryData > MultiArea;

	CClosestObstacleWideLineQueryData()										{}
	CClosestObstacleWideLineQueryData( Uint32 flags, const Vector3& v1, const Vector3& v2, Float radius )
		: CWideLineQueryData( flags, v1, v2, radius )
		, m_closestDistSq(  radius*radius  )
		, m_obstacleHit( false )											{}

	template < class Transformer >
	RED_INLINE void TransformOutput( const Transformer& functor )			{ Super::TransformOutput( functor ); functor.WorldToLocal( m_closestPointOnSegment ); functor.WorldToLocal( m_closestGeometryPoint ); }
	template < class Transformer >
	RED_INLINE void CancelTransformOutput( const Transformer& functor )		{ Super::CancelTransformOutput( functor ); functor.LocalToWorld( m_closestPointOnSegment ); functor.LocalToWorld( m_closestGeometryPoint ); }

	RED_INLINE Bool HasHit() const											{ return m_obstacleHit; }

	RED_INLINE Bool IntersectRect( const Vector2& rectMin, const Vector2& rectMax ) const;
	RED_INLINE Bool Intersect( const Vector2& v1, const Vector2& v2 ) const;

	Float			m_closestDistSq;
	Vector3			m_closestPointOnSegment;
	Vector3			m_closestGeometryPoint;
	Bool			m_obstacleHit;
};

////////////////////////////////////////////////////////////////////////////
struct CClearWideLineInDirectionQueryData : public CWideLineQueryData
{
private:
	typedef CWideLineQueryData Super;
public:
	enum { AUTOFAIL = false };

	typedef TMultiAreaSpatialQuery< CClearWideLineInDirectionQueryData > MultiArea;

	CClearWideLineInDirectionQueryData()									{}
	CClearWideLineInDirectionQueryData( Uint32 flags, const Vector3& v1, const Vector3& v2, Float radius )
		: CWideLineQueryData( flags, v1, v2, radius )
		, m_wasHit( false )
		, m_isFailedAtBasePos( false )										{}

	template < class Transformer >
	RED_INLINE void TransformOutput( const Transformer& functor )			{ Super::TransformOutput( functor ); }
	template < class Transformer >
	RED_INLINE void CancelTransformOutput( const Transformer& functor )		{ Super::CancelTransformOutput( functor ); }

	RED_INLINE Bool IntersectRect( const Vector2& rectMin, const Vector2& rectMax ) const	{ return Super::IntersectRect( rectMin, rectMax ); }
	RED_INLINE Bool IntersectCircle( const Vector2& v, Float radius ) const					{ return Super::IntersectCircle( v, radius ); }
	RED_INLINE Bool Intersect( const Vector2& v1, const Vector2& v2 ) const					{ return Super::Intersect( v1, v2 ); }

	template < class Fun >
	RED_INLINE Bool OnIntersection( const Fun& fun );

	RED_INLINE Bool GetHitPos( Vector3& outHitPos )							{ outHitPos = m_v2; return m_wasHit; }
	RED_INLINE void SetFailedAtBasePos()									{ m_v2 = m_v1; m_wasHit = true; m_isFailedAtBasePos = true; }

	Bool					m_wasHit;
	Bool					m_isFailedAtBasePos;

};

////////////////////////////////////////////////////////////////////////////

struct CRectangleQueryData : public CSpatialQueryData
{
private:
	typedef CSpatialQueryData Super;
public:
	typedef TMultiAreaSpatialQuery< CRectangleQueryData > MultiArea;

	CRectangleQueryData()													{}
	CRectangleQueryData( Uint32 flags, const Vector2 rectMin, const Vector2 rectMax, Float z )
		: CSpatialQueryData( flags, Vector3( (rectMin.X + rectMax.X) * 0.5f, (rectMin.Y + rectMax.Y) * 0.5f, z ) )
		, m_vMin( rectMin )
		, m_vMax( rectMax )
		, m_z( z )															{}

	template < class Transformer >
	RED_INLINE void Transform( const Transformer& functor )					{ Super::Transform( functor ); functor.WorldToLocal( m_vMin ); functor.WorldToLocal( m_vMax ); m_z = functor.WorldToLocalZ( m_z ); }

#if 0 //FIXME: no m_v1 or m_v2!
	template < class Transformer >
	RED_INLINE void CancelTransform( const Transformer& functor )			{ Super::CancelTransform( functor ); functor.LocalToWorld( m_v1 ); functor.LocalToWorld( m_v2 ); m_z = functor.LocalToWorldZ( m_z ); }
#endif

	RED_INLINE Bool IntersectRect( const Vector2& rectMin, const Vector2& rectMax ) const;
	RED_INLINE Bool Intersect( const Vector2& v1, const Vector2& v2 ) const;

	Vector2			m_vMin;
	Vector2			m_vMax;
	Float			m_z;
};

////////////////////////////////////////////////////////////////////////////

struct CPolyTestQueryData : public CSpatialQueryData
{
private:
	typedef CSpatialQueryData Super;
public:
	typedef TMultiAreaSpatialQuery< CPolyTestQueryData > MultiArea;

	CPolyTestQueryData()													{}
	CPolyTestQueryData( Uint32 flags, const Vector3& basePos, const Box& polyBoundings, const TDynArray< Vector2 >& poly )
		: CSpatialQueryData( flags, basePos )
		, m_polyBoundings( polyBoundings )
		, m_poly( poly )													{}
	CPolyTestQueryData( Uint32 flags, const Vector3& basePos, const Box& polyBoundings, TDynArray< Vector2 >&& poly )
		: CSpatialQueryData( flags, basePos )
		, m_polyBoundings( polyBoundings )
		, m_poly( Move( poly ) )											{}

	RED_INLINE void ComputeBBox( Vector3* bbox ) const						{ bbox[ 0 ] = m_polyBoundings.Min.AsVector3(); bbox[ 1 ] = m_polyBoundings.Max.AsVector3(); }
	RED_INLINE void ComputeBBox( Vector2* bbox ) const						{ bbox[ 0 ] = m_polyBoundings.Min.AsVector2(); bbox[ 1 ] = m_polyBoundings.Max.AsVector2(); }

	template < class Transformer >
	RED_INLINE void Transform( const Transformer& functor )					{ Super::Transform( functor ); functor.WorldToLocal( m_polyBoundings ); ForEach( m_poly, [ &functor ] ( Vector2& v ) { functor.WorldToLocal( v ); } ); }
	// TODO: if we start to use transformed navmesh area LocalToWorld on boxes will loose precision. So we will need reference to base boundings or recompute boundings on fly.
	template < class Transformer >
	RED_INLINE void CancelTransform( const Transformer& functor )			{ Super::CancelTransform( functor ); functor.LocalToWorld( m_polyBoundings ); ForEach( m_poly, [ &functor ] ( Vector2& v ) { functor.LocalToWorld( v ); } ); }

	RED_INLINE Bool IntersectRect( const Vector2& rectMin, const Vector2& rectMax ) const;
	RED_INLINE Bool Intersect( const Vector2& v1, const Vector2& v2 ) const;

	Box										m_polyBoundings;
	TDynArray< Vector2 >					m_poly;

};

////////////////////////////////////////////////////////////////////////////

struct CCustomTestQueryData : public CSpatialQueryData
{
private:
	typedef CSpatialQueryData Super;
public:
	typedef TMultiAreaSpatialQuery< CCustomTestQueryData > MultiArea;

	CCustomTestQueryData()													{}
	CCustomTestQueryData( Uint32 flags, const Vector3& basePos, SCustomCollisionTester& customTester )
		: CSpatialQueryData( flags, basePos )
		, m_customTester( &customTester )
		, m_baseTester( &customTester )										{}

	template < class Transformer >
	RED_INLINE void Transform( const Transformer& functor );
	template < class Transformer >
	RED_INLINE void CancelTransform( const Transformer& functor )			{ Super::CancelTransform( functor ); m_customTester = m_baseTester; }

	RED_INLINE void ComputeBBox( Vector3* bbox ) const						{ Box bb; m_customTester->ComputeBBox( bb ); bbox[ 0 ] = bb.Min.AsVector3(); bbox[ 1 ] = bb.Max.AsVector3(); }
	RED_INLINE void ComputeBBox( Vector2* bbox ) const						{ Box bb; m_customTester->ComputeBBox( bb ); bbox[ 0 ] = bb.Min.AsVector2(); bbox[ 1 ] = bb.Max.AsVector2(); }

	RED_INLINE Bool IntersectRect( const Vector2& rectMin, const Vector2& rectMax ) const;
	RED_INLINE Bool Intersect( const Vector2& v1, const Vector2& v2 ) const;

	SCustomCollisionTester*		m_customTester;

	SCustomCollisionTester*		m_baseTester;
	////////////////////////////////////////////////////////////////////////
	// Hacky but powerful mechanism that allows custom tester to easily
	// transform test input from local space to world space without any
	// additional external mechanism and with all data kept on the stack
	// NOTICE: we assume that transformer is persistant (for duration of
	// spatial test)
	struct UberTester : public SCustomCollisionTester
	{
		Bool IntersectLine( const Vector2& point1, const Vector2& point2 ) override		{ ASSERT( false ); return false; };
		void ComputeBBox( Box& outBBox ) override										{ ASSERT( false ); ASSUME( false );}
		const void*				m_transformer;
		SCustomCollisionTester* m_customTester;
	};
	char						m_uberTester[ sizeof(UberTester) ];
};

////////////////////////////////////////////////////////////////////////////
void CMultiAreaSpatialQuery::PushAreaUnique( AreaId areaId, const Vector3& entryPoint )
{
	// check if area was visited b4
	if ( areaId == m_startingArea )
		return;

	for ( Int32 i = 0; i < m_areasVisited; ++i )
	{
		if ( m_areasStack[ i ] == areaId )
			return;
	}
	if ( m_areasVisited == MAX_AREAS )
		return;

	// push area
	m_areasStack[ m_areasVisited ] = areaId;
	m_areasEntyPoints[ m_areasVisited ] = entryPoint;

	++m_areasVisited;
}
void CMultiAreaSpatialQuery::PushAreaSafe( AreaId areaId, const Vector3& entryPoint )
{
	// push area
	m_areasStack[ m_areasVisited ] = areaId;
	m_areasEntyPoints[ m_areasVisited ] = entryPoint;

	++m_areasVisited;
}
Bool CMultiAreaSpatialQuery::PopArea( AreaId& outArea, Vector3& outEntryPoint )
{
	if ( m_areasHandled < m_areasVisited )
	{
		outArea = m_areasStack[ m_areasHandled ];
		outEntryPoint = m_areasEntyPoints[ m_areasHandled ];

		++m_areasHandled;

		return true;
	}
	return false;
}
Bool CMultiAreaSpatialQuery::VisitedArea( AreaId area ) const
{
	if ( m_startingArea == area )
	{
		return true;
	}
	for ( Int16 i = 0; i < m_areasVisited; ++i )
	{
		if ( m_areasStack[ i ] == area )
		{
			return true;
		}
	}
	return false;
}

template < class TQuery >
TMultiAreaSpatialQuery< TQuery >* TMultiAreaSpatialQuery< TQuery >::GetMultiAreaData( TQuery& query )
{
	ASSERT( query.m_flags & CT_MULTIAREA );
	// fucking nasty, but this shit enables seamless transition from
	// all other spatial queries to multi area query data
	Int8* shift = reinterpret_cast< Int8* >( &static_cast< TMultiAreaSpatialQuery* >(NULL)->m_query );
	return reinterpret_cast< TMultiAreaSpatialQuery* >( (reinterpret_cast< Int8* >(&query)) - shift );
}	

template < class Transformer >
void CCustomTestQueryData::Transform( const Transformer& functor )
{
	Super::Transform( functor );

	struct TUberTester : public UberTester
	{
		TUberTester( const Transformer& functor, SCustomCollisionTester* tester )
		{
			m_transformer = &functor;
			m_customTester = tester;
		}
		Bool IntersectRect( const Vector2& rectMin, const Vector2& rectMax ) override
		{
			const Transformer* transformer = reinterpret_cast< const Transformer* >( m_transformer );
			ASSERT( !transformer->IsNavmeshArea(), TXT("IntersectRect implementation don't work on transformed navmeshes!") );
			Vector2 v1 = rectMin;
			Vector2 v2 = rectMax;
			transformer->LocalToWorld( v1 );
			transformer->LocalToWorld( v2 );
			return m_customTester->IntersectRect( v1, v2 );
		}
		Bool IntersectLine( const Vector2& point1, const Vector2& point2 ) override
		{
			const Transformer* transformer = reinterpret_cast< const Transformer* >( m_transformer );
			Vector2 v1 = point1;
			Vector2 v2 = point2;
			transformer->LocalToWorld( v1 );
			transformer->LocalToWorld( v2 );
			return m_customTester->IntersectLine( v1, v2 );
		}
		void ComputeBBox( Box& outBBox ) override
		{
			m_customTester->ComputeBBox( outBBox );

			Vector3 corners[ 4 ];
			corners[ 0 ] = outBBox.Min;
			corners[ 1 ] = Vector3( outBBox.Max.X, outBBox.Min.Y, outBBox.Min.Z );
			corners[ 2 ] = outBBox.Max;
			corners[ 3 ] = Vector3( outBBox.Min.X, outBBox.Max.Y, outBBox.Max.Z );

			const Transformer* transformer = reinterpret_cast< const Transformer* >( m_transformer );
			transformer->WorldToLocal( corners[ 0 ] );
			transformer->WorldToLocal( corners[ 1 ] );
			transformer->WorldToLocal( corners[ 2 ] );
			transformer->WorldToLocal( corners[ 3 ] );

			outBBox.Min.X = Min( corners[ 0 ].X, Min( corners[ 1 ].X, Min( corners[ 2 ].X, corners[ 3 ].X ) ) );
			outBBox.Max.X = Max( corners[ 0 ].X, Max( corners[ 1 ].X, Max( corners[ 2 ].X, corners[ 3 ].X ) ) );
			outBBox.Min.Y = Min( corners[ 0 ].Y, Min( corners[ 1 ].Y, Min( corners[ 2 ].Y, corners[ 3 ].Y ) ) );
			outBBox.Max.Y = Max( corners[ 0 ].Y, Max( corners[ 1 ].Y, Max( corners[ 2 ].Y, corners[ 3 ].Y ) ) );
			outBBox.Min.Z = corners[ 0 ].Z;
			outBBox.Max.Z = corners[ 2 ].Z;
		}
	};

	// insert data and virtual function pointer into structure
	static_assert( sizeof(TUberTester) == sizeof(UberTester), "PathLib critical problem!" );
	new (&m_uberTester) TUberTester( functor, m_baseTester );

	m_customTester = reinterpret_cast< TUberTester* >(m_uberTester);
}
////////////////////////////////////////////////////////////////////////////
// Spatial queries default intersection function implementation
// This are just a simple implementation, and areas can reimplement this tests
// if ever they see it fit


// CCircleQueryData
RED_INLINE Bool CCircleQueryData::IntersectRect( const Vector2& rectMin, const Vector2& rectMax ) const
{
	return MathUtils::GeometryUtils::TestIntersectionCircleRectangle2D( rectMin, rectMax, m_circleCenter.AsVector2(), m_radius );
}
RED_INLINE Bool CCircleQueryData::IntersectCircle( const Vector2& v, Float radius ) const
{
	Float radiusSq = radius + m_radius; radiusSq *= radiusSq;
	return (v - m_circleCenter.AsVector2()).SquareMag() <= radiusSq;
}
RED_INLINE Bool CCircleQueryData::Intersect( const Vector2& v1, const Vector2& v2 ) const
{
	return MathUtils::GeometryUtils::TestIntersectionCircleLine2D( m_circleCenter.AsVector2(), m_radius, v1, v2 );
}
RED_INLINE void CCircleQueryData::ClosestSpotRect( const Vector2& rectMin, const Vector2& rectMax, Vector2& outClosestSpot ) const
{
	MathUtils::GeometryUtils::ClosestPointToRectangle2D( rectMin, rectMax, m_circleCenter.AsVector2(), outClosestSpot );
}
RED_INLINE void CCircleQueryData::ClosestSpotCircle( const Vector2& v, Float radius, Vector2& outClosestSpot ) const
{
	Vector2 diff = m_circleCenter.AsVector2() - v;
	Float dist = diff.Mag();
	outClosestSpot =
		( dist > radius )
		? v + diff * (radius / dist)
		: m_circleCenter.AsVector2();
}
RED_INLINE void CCircleQueryData::ClosestSpotSegment( const Vector2& v1, const Vector2& v2, Vector2& outClosestSpot ) const
{
	MathUtils::GeometryUtils::TestClosestPointOnLine2D( m_circleCenter.AsVector2(), v1, v2, outClosestSpot );
}


// CClosestObstacleCircleQueryData
RED_INLINE Bool CClosestObstacleCircleQueryData::IntersectRect( const Vector2& rectMin, const Vector2& rectMax ) const
{
	// TODO: could be optimized with m_closestDistSq
	return Super::IntersectRect( rectMin, rectMax );
}
RED_INLINE Bool CClosestObstacleCircleQueryData::IntersectCircle( const Vector2& v, Float radius ) const
{
	// NOTICE: circle test is done exclusively for obstacles so there is no point of minimizing that call with m_closestDistSq
	return Super::IntersectCircle( v, radius );
}

RED_INLINE Bool CClosestObstacleCircleQueryData::Intersect( const Vector2& v1, const Vector2& v2 ) const
{
	Vector2 pointOut;
	MathUtils::GeometryUtils::TestClosestPointOnLine2D( m_circleCenter.AsVector2(), v1, v2, pointOut );
	return (pointOut - m_circleCenter.AsVector2()).SquareMag() <= m_closestDistSq;
}


// CLineQueryData
RED_INLINE Bool CLineQueryData::IntersectRect( const Vector2& rectMin, const Vector2& rectMax ) const
{
	return MathUtils::GeometryUtils::TestIntersectionLineRectangle2D( m_v1.AsVector2(), m_v2.AsVector2(), rectMin, rectMax );
}
RED_INLINE Bool CLineQueryData::Intersect( const Vector2& v1, const Vector2& v2 ) const
{
	return MathUtils::GeometryUtils::TestIntersectionLineLine2D( v1, v2, m_v1.AsVector2(), m_v2.AsVector2() );
}


// CWideLineQueryData
RED_INLINE Bool CWideLineQueryData::IntersectRect( const Vector2& rectMin, const Vector2& rectMax ) const
{
	Float distSq = MathUtils::GeometryUtils::TestDistanceSqrLineRectangle2D( m_v1.AsVector2(), m_v2.AsVector2(), rectMin, rectMax );
	return distSq < m_radius*m_radius;
}
RED_INLINE Bool CWideLineQueryData::IntersectCircle( const Vector2& v, Float radius ) const
{
	return MathUtils::GeometryUtils::TestIntersectionCircleLine2D( v, radius, m_v1.AsVector2(), m_v2.AsVector2() );
}
RED_INLINE Bool CWideLineQueryData::Intersect( const Vector2& v1, const Vector2& v2 ) const
{
	return MathUtils::GeometryUtils::TestDistanceSqrLineLine2D( v1, v2, m_v1.AsVector2(), m_v2.AsVector2() ) <= m_radius*m_radius;
}


// CClosestObstacleWideLineQueryData
RED_INLINE Bool CClosestObstacleWideLineQueryData::IntersectRect( const Vector2& rectMin, const Vector2& rectMax ) const
{
	Float distSq = MathUtils::GeometryUtils::TestDistanceSqrLineRectangle2D( m_v1.AsVector2(), m_v2.AsVector2(), rectMin, rectMax );
	return distSq < m_closestDistSq;
}
RED_INLINE Bool CClosestObstacleWideLineQueryData::Intersect( const Vector2& v1, const Vector2& v2 ) const
{
	return MathUtils::GeometryUtils::TestDistanceSqrLineLine2D( v1, v2, m_v1.AsVector2(), m_v2.AsVector2() ) <= m_closestDistSq;
}

// CClearWideLineInDirectionQueryData
template < class Fun >
RED_INLINE Bool CClearWideLineInDirectionQueryData::OnIntersection( const Fun& fun )
{
	m_wasHit = true;

	if( !fun( m_v1.AsVector2(), m_radius ) )
	{
		m_isFailedAtBasePos = true;
		m_v2 = m_v1;
		m_wasHit = true;
		return false;
	}

	Vector2 minVal = m_v1.AsVector2();
	Vector2 maxVal = m_v2.AsVector2();
	Vector2 diff = maxVal - minVal;
	while( diff.SquareMag() > (0.1f * 0.1f) )
	{
		Vector2 val = minVal + (diff * 0.5f);
		if ( fun( m_v1.AsVector2(), val, m_radius ) )
		{
			minVal = val;
		}
		else
		{
			maxVal = val;
		}
		diff = maxVal - minVal;
	}

	m_v2.AsVector2() = minVal;

	return true;
}

// CRectangleQueryData
RED_INLINE Bool CRectangleQueryData::IntersectRect( const Vector2& rectMin, const Vector2& rectMax ) const
{
	return
		MathUtils::GeometryUtils::RangeOverlap1D( m_vMin.X, m_vMax.X, rectMin.X, rectMax.X ) &&
		MathUtils::GeometryUtils::RangeOverlap1D( m_vMin.Y, m_vMax.Y, rectMin.Y, rectMax.Y );
}
RED_INLINE Bool CRectangleQueryData::Intersect( const Vector2& v1, const Vector2& v2 ) const
{
	return MathUtils::GeometryUtils::TestIntersectionLineRectangle2D( v1, v2, m_vMin, m_vMax );
}

// CPolyTestQueryData
RED_INLINE Bool CPolyTestQueryData::IntersectRect( const Vector2& rectMin, const Vector2& rectMax ) const
{
	return MathUtils::GeometryUtils::TestIntersectionPolygonRectangle2D( m_poly, rectMin, rectMax, 0.f );
}
RED_INLINE Bool CPolyTestQueryData::Intersect( const Vector2& v1, const Vector2& v2 ) const
{
	return MathUtils::GeometryUtils::TestIntersectionPolygonLine2D( m_poly, v1, v2 );
}

// CCustomTestQueryData
RED_INLINE Bool CCustomTestQueryData::IntersectRect( const Vector2& rectMin, const Vector2& rectMax ) const
{
	return m_customTester->IntersectRect( rectMin, rectMax );
}
RED_INLINE Bool CCustomTestQueryData::Intersect( const Vector2& v1, const Vector2& v2 ) const
{
	return m_customTester->IntersectLine( v1, v2 );
}



////////////////////////////////////////////////////////////////////////////



};			// namespace PathLib