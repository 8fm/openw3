/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "areaBSP.h"

#define SIDE_EPSILON		0.003125f
#define POLYGON_SIZE		1000.0f
#define VERTEX_EPSILON		0.02f

CAreaBSP::Polygon::Polygon( const Plane& plane )
	: m_wasSplitter( false )
	, m_sourceEdge( -1 )	
	, m_plane( plane )
{
	// project the origin onto the plane
	Vector planeNormal = plane.NormalDistance;
	planeNormal.W = 0;

	Vector p0 = -planeNormal * plane.NormalDistance.W;

	// find a vector perpendicular to the plane normal and parallel to the plane
	Vector v1 = Vector::Cross( planeNormal, Vector::EY );
	if ( v1.SquareMag3() < 1e-3 )
	{
		v1 = Vector::Cross( planeNormal, -Vector::EZ );
	}
	if ( v1.SquareMag3() < 1e-3 )
	{
		v1 = Vector::Cross( planeNormal, Vector::EX );
	}
	Vector v2 = Vector::Cross( planeNormal, v1 );

	// now create the points
	Vector dir1 = v1 + v2;
	Vector dir2 = v1 - v2;

	m_points.Resize( 4 );
	m_points[0] = p0 + dir1 * POLYGON_SIZE;
	m_points[1] = p0 - dir2 * POLYGON_SIZE;
	m_points[2] = p0 - dir1 * POLYGON_SIZE;
	m_points[3] = p0 + dir2 * POLYGON_SIZE;
	
	// Swap polygon if needed
	Plane testPlane( m_points[0], m_points[1], m_points[2] );
	if ( Vector::Dot3( testPlane.NormalDistance, m_plane.NormalDistance ) < 0.0f )
	{
		Swap( m_points[0], m_points[3] );
		Swap( m_points[1], m_points[2] );
	}
}

CAreaBSP::Polygon::Polygon( const Vector& vertexA, const Vector& vertexB, Float height, Int32 edge )
	: m_wasSplitter( false )
	, m_sourceEdge( edge )	
{
	m_points.Reserve( 4 );
	// Initialize vertices
	m_points.PushBack( vertexA );
	m_points.PushBack( vertexA + Vector( 0,0,height ) );
	m_points.PushBack( vertexB + Vector( 0,0,height ) );
	m_points.PushBack( vertexB );

	// Initialize face plane
	m_plane = Plane( vertexA, vertexB, vertexB + Vector( 0,0,height ) );
}

CAreaBSP::Polygon::Polygon( const Vector& vertexA, const Vector& vertexB, const Vector& vertexC )
	: m_wasSplitter( false )
	, m_sourceEdge( -1 )
{
	::new ( m_points ) Vector( vertexA );
	::new ( m_points ) Vector( vertexB );
	::new ( m_points ) Vector( vertexC );
}

CAreaBSP::Polygon::Polygon( const TDynArray< Vector, MC_RenderData >& vertices, const Plane& plane, Bool wasSplitter, Int32 edge )
	: m_wasSplitter( wasSplitter )
	, m_sourceEdge( edge )
	, m_points( vertices )
	, m_plane( plane )
{
}

Plane::ESide CAreaBSP::Polygon::TestSide( const Plane& plane ) const
{
	Uint32 numFront = 0;
	Uint32 numBack = 0;

	// Test vertices
	for ( Uint32 i=0; i<m_points.Size(); i++ )
	{
		Float dist = plane.DistanceTo( m_points[i] );
		if ( dist > SIDE_EPSILON )
		{
			if ( numBack ) return Plane::PS_Both;
			numFront++;
		}
		else if ( dist < -SIDE_EPSILON )
		{
			if ( numFront ) return Plane::PS_Both;
			numBack++;
		}
	}

	// On plane
	if ( !numBack && !numFront )
	{
		return Plane::PS_None;
	}

	// One side
	if ( numBack )
	{
		return Plane::PS_Back;
	}
	else
	{
		return Plane::PS_Front;
	}
}

void CAreaBSP::Polygon::Split( const Plane& plane, Polygon*& front, Polygon*& back )
{
	Float dists[256];
	Int32 sides[256];
	Int32 numPos = 0;
	Int32 numNeg = 0;

	// Classify vertices
	const Uint32 numPoints = m_points.Size();
	for ( Uint32 i=0; i<numPoints; i++ )
	{
		Float dist = dists[i] = plane.DistanceTo( m_points[i] );    
		if ( dist > SIDE_EPSILON )
		{
			sides[i] = Plane::PS_Front;
			numPos++;
		}
		else if ( dist < -SIDE_EPSILON )
		{
			sides[i] = Plane::PS_Back;
			numNeg++;
		}
		else
		{
			sides[i] = Plane::PS_None;
		}    
	}

	// Solve easy cases
	if ( !numPos && !numNeg )
	{
		if ( Vector::Dot3( m_plane.NormalDistance, plane.NormalDistance ) > 0.0f )
		{
			front = this;
			back = NULL;
		}
		else
		{
			front = NULL;
			back = this;
		}
		return;
	}
	else if ( !numNeg )
	{ 
		front = this;
		back = NULL;
		return;
	}
	else if ( !numPos )
	{ 
		front = NULL;
		back = this;
		return;
	}

	// Wrap
	sides[ numPoints ] = sides[0];
	dists[ numPoints ] = dists[0];   

	// Clip polygon
	TDynArray< Vector, MC_RenderData > posVertices;
	TDynArray< Vector, MC_RenderData > negVertices;
	for ( Uint32 i=0; i<numPoints; i++ )
	{
		// Both points
		if ( sides[i] == Plane::PS_None )
		{
			posVertices.PushBack( m_points[i] );
			negVertices.PushBack( m_points[i] );
			continue;
		}

		// Positive side only
		if ( sides[i] == Plane::PS_Front )
		{
			posVertices.PushBack( m_points[i] );
		}

		// Negative side only
		if ( sides[i] == Plane::PS_Back )
		{
			negVertices.PushBack( m_points[i] );
		}

		// Not spanning
		if ( sides[i+1] == Plane::PS_None || (sides[i+1] == sides[i]) )
		{
			continue;
		}

		// Split
		Float frac = dists[i] / (dists[i] - dists[i+1]);
		Vector pos = ( m_points[ (i+1) % numPoints ] - m_points[i] ) * frac + m_points[i];

		// Add vertex to both polygons
		posVertices.PushBack( pos );
		negVertices.PushBack( pos );
	}

	// Create parts
	front = new Polygon( posVertices, m_plane, m_wasSplitter, m_sourceEdge );
	back = new Polygon( negVertices, m_plane, m_wasSplitter, m_sourceEdge );

	// Cleanup source
	delete this;
}

Bool CAreaBSP::Convex::Build( CAreaBSP& bsp, const TDynArray< Plane >& planes )
{
	// Cleanup
	m_axes.Clear();
	m_vertices.Clear();

	// Generate convex polygons by cross clipping planes
	TDynArray< Polygon* > convexPolygons;
	for ( Uint32 i=0; i<planes.Size(); i++ )
	{
		const Plane& srcPlane = planes[i];
		Polygon* srcPolygon = new Polygon( srcPlane );

		// Clip polygon to every other plane
		for ( Uint32 j=0; j<planes.Size() && srcPolygon; j++ )
		{
			if ( i != j )
			{
				const Plane& clipPlane = planes[j];

				// Clip
				Polygon* backPolygon = NULL;
				Polygon* frontPolygon = NULL;
				srcPolygon->Split( clipPlane, frontPolygon, backPolygon );

				// We do not need front polygon
				delete frontPolygon;

				// Use back polygon for next clips
				srcPolygon = backPolygon;
			}
		}

		// Polygon survived clipping, store
		if ( srcPolygon )
		{
			convexPolygons.PushBack( srcPolygon );
		}
	}

	// Sanity check
	if ( convexPolygons.Size() < 4 )
	{
		WARN_ENGINE( TXT("Convex with less than 4 planes") );
		return false;
	}

	// Generate vertices
	for ( Uint32 i=0; i<convexPolygons.Size(); i++ )
	{
		Polygon* polygon = convexPolygons[i];
		for ( Uint32 j=0; j<polygon->m_points.Size(); j++ )
		{
			const Vector& point = polygon->m_points[j];

			// Add if not already added
			Bool added = false;
			for ( Uint32 k=0; k<m_vertices.Size(); k++ )
			{
				if ( m_vertices[k].DistanceTo( point ) < VERTEX_EPSILON )
				{
					added = true;
					break;
				}
			}

			// Add new points
			if ( !added )
			{
				m_vertices.PushBack( point );
			}
		}
	}

	// Generate SAT axes
	for ( Uint32 i=0; i<convexPolygons.Size(); i++ )
	{
		Polygon* polygon = convexPolygons[i];
		const Plane& polygonPlane = polygon->m_plane;

		// Add only if no similar axis exists
		Bool found = false;
		for ( Uint32 j=0; j<m_axes.Size(); j++ )
		{
			Float dot = Vector::Dot3( m_axes[j], polygonPlane.NormalDistance );
			if ( dot > 0.99f )
			{
				found = true;
				break;
			}
		}

		// New axis
		if ( !found )
		{
			m_axes.PushBack( polygonPlane.NormalDistance );
		}
	}

	// Show info
	//LOG_ENGINE( TXT("Convex ( %i axes, %i vertices ) created from %i planes"), m_axes.Size(), m_vertices.Size(), planes.Size() );

	// Cleanup
	convexPolygons.ClearPtr();
	return true;
}

void CAreaBSP::CollisionContext::CalculateExtents( const Vector& normal, Float &a, Float& b ) const
{
	// Projection of the box center
	Float centerDist = Vector::Dot3( normal, m_boxCenter );

	// Calculate extents along projection axis
	Float extents = 0.0f;
	extents += fabsf( Vector::Dot3( normal, m_boxAxes[0] ) ) * m_boxExtents.X;
	extents += fabsf( Vector::Dot3( normal, m_boxAxes[1] ) ) * m_boxExtents.Y;
	extents += fabsf( Vector::Dot3( normal, m_boxAxes[2] ) ) * m_boxExtents.Z;

	// Calculate projected segment
	a = centerDist - extents;
	b = centerDist + extents;
}

void CAreaBSP::Convex::CalculateExtents( const Vector& normal, Float &a, Float& b ) const
{
	// Calculate convex extents
	Float minDistnace = FLT_MAX;
	Float maxDistnace = -FLT_MAX;
	for ( Uint32 i=0; i<m_vertices.Size(); i++ )
	{
		Float distance = Vector::Dot3( m_vertices[i], normal );
		minDistnace = Min( minDistnace, distance );
		maxDistnace = Max( maxDistnace, distance );
	}

	// Check that projection is not empty
	ASSERT( minDistnace <= maxDistnace );
	a = minDistnace;
	b = maxDistnace;
}

Bool CAreaBSP::Convex::TestOverlapInAxis( const Vector& normal, const CollisionContext& context ) const
{
	// Project box
	Float boxMin, boxMax;
	context.CalculateExtents( normal, boxMin, boxMax );

	if( m_vertices.Empty() )
	{
		return false;
	}

	// Project convex
	Float convexMin, convexMax;
	CalculateExtents( normal, convexMin, convexMax );

	// Test overlap
	return ( boxMax > convexMin ) && ( convexMax > boxMin );
}

Bool CAreaBSP::Convex::TestOverlap( const CollisionContext& context ) const
{
	// Test using box axes
	if ( !TestOverlapInAxis( context.m_boxAxes[0], context ) ) return false;
	if ( !TestOverlapInAxis( context.m_boxAxes[1], context ) ) return false;
	if ( !TestOverlapInAxis( context.m_boxAxes[2], context ) ) return false;

	// Test using convex axes
	for ( Uint32 i=0; i<m_axes.Size(); i++ )
	{
		if ( !TestOverlapInAxis( m_axes[i], context ) ) return false;
	}

	// Possible overlap
	return true;
}

CAreaBSP::CAreaBSP()
	: m_root( -1 )
{
}

CAreaBSP::~CAreaBSP()
{

}

Bool CAreaBSP::IsValid() const
{
	return m_root != -1;
}

void CAreaBSP::Serialize( IFile& file )
{
	file << m_root;
	file << m_planes;
	file << m_nodes;
	file << m_convexes;
}

Int16 CAreaBSP::IndexPlane( const Plane& plane )
{
	// Linear search
	for ( Uint32 i=0; i<m_planes.Size(); i++ )
	{
		if ( m_planes[i].NormalDistance == plane.NormalDistance )
		{
			return (Int16)i;
		}
	}

	// Add new plane
	Int16 index = (Int16) m_planes.Size();
	m_planes.PushBack( plane );
	return index;
}

void CAreaBSP::Compile( const TDynArray< Vector >& localVertices, Float height )
{
	// Cleanup
	m_root = 0;
	m_planes.ClearFast();
	m_convexes.ClearFast();
	m_nodes.ClearFast();
	//m_nodes.Reserve( 32 );

	// Generate initial polygons
	TDynArray< Polygon* > initialPolygons;
	for ( Uint32 i=0; i<localVertices.Size(); i++ )
	{
		const Vector vertexA = localVertices[i];
		const Vector vertexB = localVertices[ (i+1) % localVertices.Size() ];

		// Create polygons
		Polygon* poly = new Polygon( vertexA, vertexB, height, i );
		initialPolygons.PushBack( poly );
	}

	// Create Z down node
	m_nodes.Grow( 1 );
	m_nodes[0].m_plane = IndexPlane( Plane( Vector( 0,0,-1 ), Vector(0,0,0) ) );
	m_nodes[0].m_parent = -1;
	m_nodes[0].m_children[0] = -1;
	m_nodes[0].m_children[1] = 1;

	// Create Z up node
	m_nodes.Grow( 1 );
	m_nodes[1].m_plane = IndexPlane( Plane( Vector( 0,0,1 ), Vector(0,0,height) ) );
	m_nodes[1].m_parent = 0;
	m_nodes[1].m_children[0] = -1;
	m_nodes[1].m_children[1] = 2;

	// Compile BSP
	TDynArray< Plane > planeList;
	planeList.PushBack( m_planes[ m_nodes[0].m_plane ] );
	planeList.PushBack( m_planes[ m_nodes[1].m_plane ] );
	CompileBSP( 1, initialPolygons, planeList );

	// Show compiled BSP
	//PrintNode( 2, 0 );
}

CAreaBSP::Polygon* CAreaBSP::FindSplitter( const TDynArray< Polygon* >& polygons )
{
	Polygon* bestSplitter = NULL;
	Uint32 bestSplits = INT_MAX;

	// Test each polygon as a splitter
	for ( Uint32 i=0; i<polygons.Size(); i++ )
	{
		Polygon* splitter = polygons[i];
		if ( !splitter->m_wasSplitter )
		{
			// Calculate splits
			Uint32 numSplits = 0;
			for ( Uint32 j=0; j<polygons.Size() && numSplits < bestSplits; j++ )
			{
				// Test only non splitted polygons
				Polygon* splitted = polygons[j];
				if ( !splitted->m_wasSplitter )
				{
					Plane::ESide side = splitted->TestSide( splitter->m_plane );
					if ( side == Plane::PS_Both )
					{
						numSplits++;
					}
				}
			}

			// Better splitter ?
			if ( numSplits < bestSplits )
			{
				bestSplits = numSplits;
				bestSplitter = splitter;
			}
		}
	}

	// Return polygon that was the best splitter
	return bestSplitter;
}

void CAreaBSP::SplitPolygons( const TDynArray< Polygon* >& polygons, Polygon* splitter, TDynArray< Polygon* >& frontPolygons, TDynArray< Polygon* >& backPolygons )
{
	for ( Uint32 i=0; i<polygons.Size(); i++ )
	{
		Polygon* polygon = polygons[i];

		// Test side
		Plane::ESide side = polygon->TestSide( splitter->m_plane );

		// On plane, mark as used as splitter and distribute
		if ( side == Plane::PS_None )
		{
			// Mark as used as splitter
			polygon->m_wasSplitter = true;

			// Drop to front or back list depending on the normal
			if ( Vector::Dot3( splitter->m_plane.NormalDistance, polygon->m_plane.NormalDistance ) > 0.0f )
			{
				side = Plane::PS_Front;
			}
			else
			{
				side = Plane::PS_Back;
			}
		}

		// Add to lists
		if ( side == Plane::PS_Front )
		{
			frontPolygons.PushBack( polygon );
		}
		else if ( side == Plane::PS_Back )
		{
			backPolygons.PushBack( polygon );
		}
		else
		{
			ASSERT( side == Plane::PS_Both );

			// Split
			Polygon* frontPart = NULL;
			Polygon* backPart = NULL;
			polygon->Split( splitter->m_plane, frontPart, backPart );
			ASSERT( frontPart );
			ASSERT( backPart );

			// Add fragments to list
			frontPolygons.PushBack( frontPart );
			backPolygons.PushBack( backPart );
		}
	}
}

Int16 CAreaBSP::CompileBSP( Int16 parent, const TDynArray< Polygon* >& polygons, TDynArray< Plane >& planeList )
{
	// No polygons, solid leaf
	if ( !polygons.Size() )
	{
		// Create convex
		Int16 convexIndex = (Int16) m_convexes.Grow( 1 );
		m_convexes[ convexIndex ].Build( *this, planeList );

		// Add convex
		return (Int16) ( - ( convexIndex + 2 ) );
	}

	// Find best splitter for current polygon soup
	Polygon* splitter = FindSplitter( polygons );
	if ( !splitter )
	{
		// Delete polygon fragments
		for ( Uint32 i=0; i<polygons.Size(); i++ )
		{
			delete polygons[i];
		}

		// Empty leaf		
		return -1;
	}

	// Split polygon list
	TDynArray< Polygon* > frontPolygons;
	TDynArray< Polygon* > backPolygons;
	SplitPolygons( polygons, splitter, frontPolygons, backPolygons );

	// Create node
	Node node;
	node.m_parent = parent;
	Plane nodePlane = splitter->m_plane;
	node.m_plane = IndexPlane( nodePlane );

	// Add node to BSP
	Int16 nodeIndex = (Int16) m_nodes.Size();
	m_nodes.PushBack( node );

	// Recurse front
	{
		planeList.PushBack( -nodePlane );
		//CM: WARNING! DO NOT change this two lines below, because compiler will optimize it BADLY!
		Int16 result = CompileBSP( nodeIndex, frontPolygons, planeList );
		m_nodes[nodeIndex].m_children[0] = result;
		planeList.PopBack();
	}

	// Recurse back
	{
		planeList.PushBack( nodePlane );
		//CM: WARNING! DO NOT change this two lines below, because compiler will optimize it BADLY!
		Int16 result = CompileBSP( nodeIndex, backPolygons, planeList );
		m_nodes[nodeIndex].m_children[1] = result;
		planeList.PopBack();
	}

	// Return BSP node index
	return nodeIndex;
}

void CAreaBSP::PrintNode( Uint32 level, Int16 nodeIndex )
{
	Char spaces[ 60 ] = {0};
	for ( Uint32 i=0; i<level; i++ )
	{
		spaces[ i ] = ' ';
	}

	if ( nodeIndex == -1 )
	{
		LOG_ENGINE( TXT("%sEMPTY"), spaces );
		return;
	}

	if ( nodeIndex <= -2 )
	{
		Int32 convexIndex = -( nodeIndex + 2 );
		const Convex& convex = m_convexes[ convexIndex ];
		LOG_ENGINE( TXT("%sCONVEX %i, %i axes, %i vertices"), spaces, convexIndex, convex.m_axes.Size(), convex.m_vertices.Size() );
		return;
	}

	const Node& node = m_nodes[ nodeIndex ];
	const Plane& plane = m_planes[ node.m_plane ];
	LOG_ENGINE( TXT("%sNode %i, plane %i [%1.2f,%1.2f,%1.2f] : %1.4f"), spaces, nodeIndex, node.m_plane, plane.NormalDistance.X, plane.NormalDistance.Y, plane.NormalDistance.Z, plane.NormalDistance.W );

	// Recurse
	PrintNode( level+2, node.m_children[0] );
	PrintNode( level+2, node.m_children[1] );
}

Bool CAreaBSP::PointOverlap( const Vector& point ) const
{
	// Recurse down the tree searching for a leaf
	Int32 nodeIndex = m_root;
	while ( nodeIndex >= 0 )
	{
		// Recurse
		const Node& node = m_nodes[ nodeIndex ];
		const Plane& plane = m_planes[ node.m_plane ];
		if ( plane.DistanceTo( point ) > 0.0f )
		{
			nodeIndex = node.m_children[0];
		}
		else
		{
			nodeIndex = node.m_children[1];
		}
	}

	// Outside
	return nodeIndex != -1;
}

Bool CAreaBSP::BoxOverlap( const Vector& center, const Vector& extents, const Vector& axisX, const Vector& axisY, const Vector& axisZ ) const
{
	// Setup collision context
	CollisionContext context;
	context.m_boxCenter = center;
	context.m_boxExtents = extents;
	context.m_boxAxes[0] = axisX;
	context.m_boxAxes[1] = axisY;
	context.m_boxAxes[2] = axisZ;

	// Test BSP
	return CheckBoxOverlapNode( m_root, context );
}

Bool CAreaBSP::Intersects( const Vector& start, const Vector& end ) const
{
	(void)start;
	(void)end;
	/*Int32 index = m_root;
	while ( index >= 0 )
	{
		const Node& node = m_nodes[ index ];
		const Plane& plane = m_planes[ node.m_plane ];
	}*/
	return false;
}

Bool CAreaBSP::CheckBoxOverlapNode( Int16 nodeIndex, CollisionContext& context ) const
{
	// Empty leaf, no overlap
	if ( nodeIndex == -1 )
	{
		return false;
	}

	// Solid leaf
	if ( nodeIndex <= -2 )
	{
		const Convex& convex = m_convexes[ -( nodeIndex + 2 ) ];
		return convex.TestOverlap( context );
	}

	// Extract node plane
	const Node& node = m_nodes[ nodeIndex ];
	const Plane& plane = m_planes[ node.m_plane ];

	// Calculate box extents
	Float nearDistance, farDistance;
	context.CalculateExtents( plane.NormalDistance, nearDistance, farDistance );

	// Test overlap with plane
	nearDistance += plane.NormalDistance.W;
	farDistance += plane.NormalDistance.W;

	// Calculate overlap
	if ( nearDistance > 0.0f )
	{
		return CheckBoxOverlapNode( node.m_children[0], context );
	}
	else if ( farDistance < 0.0f )
	{
		return CheckBoxOverlapNode( node.m_children[1], context  );
	}
	else
	{
		// Check near side
		if ( CheckBoxOverlapNode( node.m_children[0], context ) )
		{
			return true;
		}

		// Check far side
		return CheckBoxOverlapNode( node.m_children[1], context );
	}
}

void CAreaBSP::TraceContext::CalculateExtents( const Vector& normal, Float &extents ) const
{
	extents = 0.0f;
	extents += fabsf( Vector::Dot3( normal, m_boxAxes[0] ) ) * m_boxExtents.X;
	extents += fabsf( Vector::Dot3( normal, m_boxAxes[1] ) ) * m_boxExtents.Y;
	extents += fabsf( Vector::Dot3( normal, m_boxAxes[2] ) ) * m_boxExtents.Z;
}

Bool CAreaBSP::Convex::TraceAxis( const Vector& normal, const TraceContext& context, TraceResults& results ) const
{
	// Calculate convex extents along the projection axis
	Float convexMin, convexMax;
	CalculateExtents( normal, convexMin, convexMax );

	// Calculate box projected extents
	Float projectedExtents;
	context.CalculateExtents( normal, projectedExtents );

	// Expand convex segment by the box size
	convexMin -= projectedExtents;
	convexMax += projectedExtents;

	// Project start and end point
	const Float projectedStart = Vector::Dot3( context.m_traceStart, normal );
	const Float projectedEnd = Vector::Dot3( context.m_traceEnd, normal );

	// Calculate movement direction
	const Float projectedDirection = projectedEnd - projectedStart;

	// Nearly parallel 
	const Float projectedDirAbs = fabsf( projectedDirection );
	if ( projectedDirAbs < 0.01f )
	{
		if ( projectedStart < convexMin || projectedStart > convexMax )
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	// Calculate impact normal and entry/exit times
	Vector impactNormal;
	Float entryTime, exitTime;
	if ( projectedDirection > 0.0f )
	{
		entryTime = ( convexMin - projectedStart ) / projectedDirection;
		exitTime = ( convexMax - projectedStart ) / projectedDirection;
		impactNormal = -normal;
	}
	else
	{
		entryTime = ( convexMax - projectedStart ) / projectedDirection;
		exitTime = ( convexMin - projectedStart ) / projectedDirection;
		impactNormal = normal;
	}	

	// Update results data
	if ( entryTime > results.m_impactTime )
	{
		results.m_impactTime = entryTime;
		results.m_impactNormal = impactNormal;
	}

	// Update results data
	if ( exitTime < results.m_exitTime )
	{
		results.m_exitTime = exitTime;
		results.m_exitNormal = -impactNormal;
	}

	// SAT found
	if ( results.m_exitTime < results.m_impactTime )
	{
		return false;
	}

	// Exited before real impact
	if ( results.m_exitTime < 0.0f )
	{
		return false;
	}

	// Intersection found
	return true;
}

Bool CAreaBSP::Convex::Trace( const TraceContext& context, TraceResults& results ) const
{
	// Test using box axes
	if ( !TraceAxis( context.m_boxAxes[0], context, results ) ) return false;
	if ( !TraceAxis( context.m_boxAxes[1], context, results ) ) return false;
	if ( !TraceAxis( context.m_boxAxes[2], context, results ) ) return false;

	// Test using convex axes
	for ( Uint32 i=0; i<m_axes.Size(); i++ )
	{
		if ( !TraceAxis( m_axes[i], context, results ) ) return false;
	}

	// Collision :)
	return true;
}

Bool CAreaBSP::BoxTrace( const Vector& start, const Vector& end, const Vector& extents, const Vector& axisX, const Vector& axisY, const Vector& axisZ, Float& hitTime, Vector& hitNormal ) const
{
	// Setup trace context
	TraceContext context;
	context.m_traceStart = start;
	context.m_traceEnd = end;
	context.m_boxExtents = extents;
	context.m_boxAxes[0] = axisX;
	context.m_boxAxes[1] = axisY;
	context.m_boxAxes[2] = axisZ;

	// Trace all convexes
	Bool hitFlag = false;
	for ( Uint32 i=0; i<m_convexes.Size(); i++ )
	{
		// Setup trace results
		TraceResults results;
		results.m_impactTime = -FLT_MAX;
		results.m_exitTime = FLT_MAX;

		// Trace convex
		const Convex& convex = m_convexes[i];
		if ( convex.Trace( context, results ) )
		{

			// Started solid			
			/*  BS: I need full info
			if ( results.m_impactTime < 0.0f )
			{
				hitTime = 0.0f;
				hitNormal = ( end - start ).Normalized3();
				return true;
			}
			*/

			// Update hit info
			if ( results.m_impactTime < hitTime && results.m_impactTime != -FLT_MAX )
			{
				hitTime = results.m_impactTime;
				hitNormal = results.m_impactNormal;
				hitFlag = true;
			}
		}
	}

	// Return collision status
	return hitFlag;
}

Bool CAreaBSP::ClipToNode( Int16 nodeIndex, Polygon* polygon, TDynArray< Polygon* >& outPolygons ) const
{
	// Empty leaf
	if ( nodeIndex == -1 )
	{
		return false;
	}

	// Solid leaf, add to output list
	if ( nodeIndex <= -2 )
	{
		outPolygons.PushBack( polygon );
		return true;
	}

	// Test with node's plane
	const Node& node = m_nodes[ nodeIndex ];
	const Plane& plane = m_planes[ node.m_plane ];

	// Test side
	Plane::ESide side = polygon->TestSide( plane );

	// On plane, discard
	if ( side == Plane::PS_None )
	{
		delete polygon;
		return false;
	}

	// On front side, recurse
	if ( side == Plane::PS_Front )
	{
		// Recurse to subtree
		return ClipToNode( node.m_children[0], polygon, outPolygons );
	}
	else if ( side == Plane::PS_Back )
	{
		// Recurse to subtree
		return ClipToNode( node.m_children[1], polygon, outPolygons );
	}
	else
	{
		// Split
		Polygon* frontPart = NULL;
		Polygon* backPart = NULL;
		polygon->Split( plane, frontPart, backPart );
		ASSERT( frontPart );
		ASSERT( backPart );

		// Recuse
		Bool flag = ClipToNode( node.m_children[0], frontPart, outPolygons );
		flag |= ClipToNode( node.m_children[1], backPart, outPolygons );
		return flag;
	}
}

Bool CAreaBSP::ClipToTree( Polygon* polygon, TDynArray< Polygon* >& outPolygons ) const
{
	const Int16 rootNode = m_root;
	return ClipToNode( rootNode, polygon, outPolygons );
}
