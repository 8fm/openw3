#include "build.h"
#include "areaConvex.h"
#include "areaShapeBuilder.h"
#include "../core/fileSkipableBlock.h"
#include "renderer.h"
#include "renderFrame.h"
#include "baseEngine.h"
//---------------------------------------------------------------------------

#define CONVEX_VERSION	4

namespace
{
	template< typename T >
	static Bool Match2(const T& a0, const T& a1, const T& b0, const T& b1)
	{
		return ((a0==b0) && (a1==b1)) || ((a0==b1) && (a1==b0));
	}

	static Bool AddBevelPlane( const Vector* vertices, Uint32 numVertices, const Vector& offset, TDynArray< Vector >& outBevelPlanes, const Vector& localDir, const Matrix& space, const Float bevel, const Float bevelVertical )
	{
		// use special bevel radius for "+Z" and "-Z" directions
		Float allowedBevel = bevel;
		if ( Abs(localDir.Z) > Abs(localDir.X) && Abs(localDir.Z) > Abs(localDir.Y) )
		{
			allowedBevel = bevelVertical;
		}

		// calculate the offset
		Float dotMax = -FLT_MAX;
		for ( Uint32 i=0; i<numVertices; ++i )
		{
			const Vector vertPos = space.TransformVector( offset + vertices[i] );
			dotMax = Max< Float >( dotMax, Vector::Dot3( localDir, vertPos ) );
		}

		// new plane distance value
		const Float newPlaneW = -(dotMax + allowedBevel);

		// do not add new plane in similar direction
		const Uint32 numPlanes = outBevelPlanes.Size();
		for ( Uint32 i=0; i<numPlanes; ++i )
		{
			Vector& curPlane = outBevelPlanes[i];

			const Float dot = Vector::Dot3( curPlane, localDir );
			const Float similarityDot = 0.999f;
			if ( dot >= similarityDot )
			{
				// already there - shift the plane
				curPlane.W = Min<Float>( curPlane.W, newPlaneW );
				return false;
			}
		}

		// create new plane
		Vector bevelPlane( localDir.X, localDir.Y, localDir.Z, newPlaneW );
		outBevelPlanes.PushBack( bevelPlane );

		// added
		return true;
	}

	static void ClipFaces( const Color& faceColor, Uint32 filterIndex, const TDynArray< TDynArray<Vector> >& bevelPlanes, TDynArray<DebugVertex>& outVertices, TDynArray<Uint32>& outIndices )
	{
		// Generate the output geometry by clipping the faces
		const TDynArray< Vector >& ourPlanes = bevelPlanes[ filterIndex ];
		for ( Uint32 i=0; i<ourPlanes.Size(); ++i )
		{
			Plane testPlane;
			testPlane.NormalDistance = ourPlanes[i];

			CAreaShapeBuilder::Face testFace;
			testFace.BuildPlanarFace( testPlane );

			// clip to all other planes
			for ( Uint32 j=0; j<ourPlanes.Size(); ++j )
			{
				if ( j != i )
				{
					Plane clipPlane;
					clipPlane.NormalDistance = ourPlanes[j];

					CAreaShapeBuilder::Face frontFace, backFace;
					testFace.SplitFace( clipPlane, frontFace, backFace, false );

					Swap( testFace, backFace );

					if ( testFace.m_vertices.Empty() )
					{
						break;
					}
				}
			}

			// input face clipped (should not happen)
			if ( testFace.m_vertices.Empty() )
			{
				continue;
			}

			// part list
			TDynArray< CAreaShapeBuilder::Face > frontFragList;
			frontFragList.PushBack( testFace );

			// color variant
			Uint32 realFaceColor = faceColor.ToUint32();
			if (Abs(testPlane.NormalDistance.Z) < 0.7f )
			{
				const Float randFrac = GEngine->GetRandomNumberGenerator().Get< Float >();
				realFaceColor = Color::Mul3(faceColor, 0.5f + 0.25f * randFrac).ToUint32();
			}

			// filter with other shapes
			const Uint32 numShapes = bevelPlanes.Size();
			for ( Uint32 k=0; k<numShapes; ++k )
			{
				if ( k == filterIndex )
				{
					continue;
				}

				// clip each fragment
				TDynArray< CAreaShapeBuilder::Face > curClipList;
				Swap( curClipList, frontFragList );
				for ( Uint32 z=0; z<curClipList.Size(); ++z )
				{
					CAreaShapeBuilder::Face clipFace = curClipList[z];

					// clip only the OUTSIDE part
					const TDynArray< Vector >& shapePlanes = bevelPlanes[ k ];
					for ( Uint32 j=0; j<shapePlanes.Size(); ++j )
					{
						Plane clipPlane;
						clipPlane.NormalDistance = shapePlanes[j];

						const Bool removePlanar = (k < filterIndex);

						CAreaShapeBuilder::Face frontFace, backFace;
						clipFace.SplitFace( clipPlane, frontFace, backFace, removePlanar );

						// front part
						if ( !frontFace.m_vertices.Empty() )
						{
							frontFragList.PushBack( frontFace );
						}

						// no more data
						if ( backFace.m_vertices.Empty() )
						{
							break;
						}

						// keep clipping the back face
						Swap( clipFace, backFace );
					}
				}
			}

			// generate output faces
			for ( Uint32 z=0; z<frontFragList.Size(); ++z )
			{
				const Uint32 firstVertex = outVertices.Size();

				// generate vertices
				const CAreaShapeBuilder::Face& testFace = frontFragList[z];
				const Uint32 numVertices = testFace.m_vertices.Size();
				for ( Uint32 i=0; i<numVertices; ++i )
				{
					Vector pos = testFace.m_vertices[i];
					new (outVertices) DebugVertex( pos, realFaceColor );
				}

				// generate indices
				for (Uint32 v=2; v<numVertices; ++v)
				{
					outIndices.PushBack(firstVertex + v);
					outIndices.PushBack(firstVertex + v-1);
					outIndices.PushBack(firstVertex + 0);
				}
			}
		}
	}

	template< typename T >
	T* CopyDataBuffer( const T* ptr, const Uint32 count, const Uint32 align )
	{
		T* data = (T*) RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_TriggerSystem, sizeof(T)*count, align );
		Red::System::MemoryCopy( data, ptr, sizeof(T)*count );
		return data;
	}
}

//---------------------------------------------------------------------------

CAreaConvex::CAreaConvex()
	: m_numPlanes(0)
	, m_planes(NULL)
	, m_numVertices(0)
	, m_vertices(0)
	, m_numEdges(0)
	, m_edges(NULL)
	, m_numFaces(0)
	, m_faces(NULL)
{
}

CAreaConvex::~CAreaConvex()
{
	if (NULL != m_planes)
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_TriggerSystem, m_planes );
		m_planes = NULL;
		m_numPlanes = 0;
	}

	if (NULL != m_vertices)
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_TriggerSystem, m_vertices );
		m_vertices = NULL;
		m_numVertices = 0;
	}

	if (NULL != m_edges)
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_TriggerSystem, m_edges );
		m_edges = NULL;
		m_numEdges = 0;
	}

	if (NULL != m_faces)
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_TriggerSystem, m_faces );
		m_faces = NULL;
		m_numFaces = 0;
	}
}

const Uint32 CAreaConvex::CalcMemorySize() const
{
	Uint32 memSize = sizeof(CAreaConvex);
	memSize += m_numVertices * sizeof(Vector);
	memSize += m_numEdges * sizeof(Edge);
	memSize += m_numPlanes * sizeof(Vector);
	memSize += m_numFaces * sizeof(Uint16);
	return memSize;
}

Bool CAreaConvex::PointOverlap(const Vector& point) const
{
	// General bounding box test
	if (!m_box.Contains(point))
	{
		return false;
	}

	// Create test point with w=1
	Vector testPoint(point);
	testPoint.SetW(1.0f);

	// Test all planes
	for (Uint32 i=0; i<m_numPlanes; ++i)
	{
		const Vector& plane = m_planes[i];
		if (Vector::Dot4(plane, testPoint) > 0.0f)
		{
			return false;
		}
	}

	// Ovelap found
	return true;
}

Bool CAreaConvex::BoxOverlap( const Vector& center, const Vector& extents, const Vector& axisX, const Vector& axisY, const Vector& axisZ ) const
{
	// Create test point with w=1
	Vector testPoint(center);
	testPoint.SetW(1.0f);

	// Test all planes
	for (Uint32 i=0; i<m_numPlanes; ++i)
	{
		const Vector& plane = m_planes[i];

		// extend the plane by the box size
		// TODO: create better SIMD implementation here
		const Float offsetX = Abs(Vector::Dot3(plane, axisX)) * extents.X;
		const Float offsetY = Abs(Vector::Dot3(plane, axisY)) * extents.Y;
		const Float offsetZ = Abs(Vector::Dot3(plane, axisZ)) * extents.Z;
		const Float offset = offsetX + offsetY + offsetZ;
		const Vector movedPlane( plane.X, plane.Y, plane.Z, plane.W - offset );

		// test against beveled plane
		if (Vector::Dot4(movedPlane, testPoint) > 0.0f)
		{
			return false;
		}
	}

	// Ovelap found
	return true;
}

Bool CAreaConvex::FindClosestPoint( const Vector& referencePoint, Vector& outPoint, Float& outDistance ) const
{
	Float bestDistance = outDistance;
	Vector bestPoint(0,0,0);

	// Offset the test point
	Vector testPoint( referencePoint );
	testPoint -= m_offset;
	testPoint.SetW( 1.0f );

	// Check faces (not beveled planes)
	Bool inInside = true;
	for (Uint32 i=0; i<m_numFaces; ++i)
	{
		const Vector& facePlane = m_planes[i];

		// Project the test point on the face
		const Float distToPlane = Vector::Dot4( testPoint, facePlane );
		if ( distToPlane <= 0.0f )
		{
			// back side of the plane, do not project
			continue;
		}

		// We are in front of at least one plane so the test point cannot be inside the convex
		inInside = false;

		// The point is to far away to be new best
		if ( distToPlane >= bestDistance )
		{
			continue;
		}

		// Project onto the plane
		Vector planePoint = testPoint - ( facePlane * distToPlane );
		planePoint.SetW( 1.0f );

		// Process the Voronoi set for the face
		Bool wasClipped = false;
		const Uint16 firstEdge = m_faces[ i ];
		Uint16 curEdge = firstEdge;
		do
		{
			const Edge& e = m_edges[ curEdge ];

			// calculate distance to edge plane (prependicular to face plane)
			const Float pointEdgeDist = Vector::Dot4( planePoint, e.m_plane );
			if ( pointEdgeDist <= 0.0f )
			{
				// in the back, we are not interested
				curEdge = e.m_next;
				continue;
			}

			// calculate the placement of the point against the second plane
			const Edge& nextEdge = m_edges[ e.m_next ];
			const Float nextEdgeDist = Vector::Dot4( planePoint, nextEdge.m_plane );

			// case 1 (vertex) - both positive distances
			if ( nextEdgeDist >= 0.0f )
			{
				planePoint = m_vertices[ nextEdge.m_vertex ];
			}
			else
			{
				// case 2 (edge) - project point back on the edge
				const Float dot = Vector::Dot4( e.m_dir, planePoint ); // 0 - edge len
				const Float maxDot = Vector::Dot4( e.m_dir, m_vertices[ nextEdge.m_vertex ] );
				if ( dot <= 0.0f )
				{
					planePoint = m_vertices[ e.m_vertex ];
				}
				else if ( dot >= maxDot )
				{
					planePoint = m_vertices[ nextEdge.m_vertex ];
				}
				else
				{
					planePoint = m_vertices[ e.m_vertex ];
					planePoint += e.m_dir * dot;
				}
			}

			// in any case, we got clipped here
			wasClipped = true;
		}
		while ( !wasClipped && curEdge != firstEdge );

		// Update stats
		const Float newDistnace = planePoint.DistanceTo( testPoint );
		if ( newDistnace < bestDistance )
		{
			bestDistance = newDistnace;
			bestPoint = planePoint;
		}
	}

	// We are totally inside !
	if (inInside)
	{
		outPoint = referencePoint;
		outDistance = 0.0f;
		return true;
	}

	// Was the best distance improved ?
	if ( bestDistance < outDistance )
	{
		outDistance = bestDistance;
		outPoint = bestPoint + m_offset;
		return true;
	}

	// No improvement
	return false;
}

Bool CAreaConvex::IntersectLine( const Vector& start, const Vector& end, Float& outHit0, Float& outHit1 ) const
{
	// convert to local space
	Vector baseStart = start;
	baseStart.SetW( 1.0f );
	Vector baseEnd = end;
	baseEnd.SetW( 1.0f );

	// current segment
	Vector startPos = baseStart;
	Vector endPos = baseEnd;	
	Float startTime = 0.0f;
	Float endTime = 1.0f;

	// test all planes
	for ( Uint32 i=0; i<m_numPlanes; ++i )
	{
		// calculate distance to plane
		const Vector& plane = m_planes[i];
		const Float startDist = Vector::Dot4( plane, startPos );
		const Float endDist = Vector::Dot4( plane, endPos );

		// sign
		const Bool startNeg = ( startDist < 0.0f );
		const Bool endNeg = ( endDist < 0.0f );

		// no intersection at all
		if ( !startNeg && !endNeg )
		{
			return false;
		}

		// totally inside, continue
		if ( startNeg && endNeg )
		{
			continue;
		}

		// calculate intersection ratio
		const Float frac = startDist / ( startDist - endDist );
		ASSERT( frac >= 0.0f && frac <= 1.0f );

		// calculate intersection time
		const Float time = Lerp( frac, startTime, endTime );
		const Vector pos = Lerp( time, baseStart, baseEnd );

		// clip the segment
		if ( startDist > 0.0f )
		{
			// clip the front segment
			startTime = time;
			startPos = pos;			
		}
		else 
		{
			// clip from the back
			endTime = time;
			endPos = pos;
		}
	}

	// the [startPos-endPos] segment is inside, update the segment clip info
	outHit0 = startTime;
	outHit1 = endTime;

	// we had intersection
	return true;
}

Bool CAreaConvex::GetSurfacePoint( const Vector& origin, const Vector& dir, Vector& outPoint ) const
{
	// calculate size of the convex in given direction
	Float minDist = FLT_MAX;
	Float maxDist = -FLT_MAX;
	for ( Uint32 i=0; i<m_numVertices; ++i )
	{
		const Float dist = Vector::Dot3( dir, m_vertices[i] );
		minDist = Min< Float >( minDist, dist );
		maxDist = Max< Float >( maxDist, dist );
	}

	// calculate start and end points for trace - make sure that they always span the full convex volume
	// the "surface searching ray" always goes along the "dir" vector.
	const Float pointDist = Vector::Dot3( dir, origin );
	const Float margin = 1.0f; // margin to make sure traces start and end outside the convex shape
	const Vector start = origin - (dir * Max<Float>( 0.0f, pointDist - (minDist-margin) ));
	const Vector end = origin + (dir * Max<Float>( 0.0f, (maxDist+margin) - pointDist ));

	// intersect the shape
	Float hit0 = FLT_MAX, hit1 = FLT_MAX;
	if ( !IntersectLine( start, end, hit0, hit1 ) )
	{
		// the "floor searching ray" is not intersecting the shape
		return false;
	}

	// we shouldn't have intersections within the shape (that's why the margin was added)
	ASSERT( hit0 > 0.0f );

	// since the ray was going along the "dir" direction calculate we should use the first intersection point as the floor level
	outPoint = Lerp( hit0, start, end );
	return true;
}

void CAreaConvex::RenderOutline( CRenderFrame* frame, const Matrix& drawMatrix, const Color& color, const Bool overlay ) const
{
	// Get data
	TDynArray< DebugVertex > vertices;
	vertices.Reserve( m_numEdges * 2 );
	TDynArray<Vector> localVertices;
	localVertices.Reserve( 10 );

	// Generate faces
	for (Uint32 i=0; i<m_numFaces; ++i)
	{
		const Uint32 firstEdgeIndex = m_faces[i];
		const Uint32 firstVertex = vertices.Size();
		
		// generate vertices
		Uint32 numVertices = 0;
		Uint32 edge = firstEdgeIndex;
		localVertices.ClearFast();
		do
		{
			Vector pos = drawMatrix.TransformPoint( m_vertices[ m_edges[ edge ].m_vertex ] );
			localVertices.PushBack(pos);

			numVertices += 1;
			edge = m_edges[edge].m_next;
		}
		while (edge != firstEdgeIndex);

		// generate indices
		for (Uint32 v=0; v<localVertices.Size(); ++v)
		{
			new (vertices) DebugVertex( localVertices[v], color.ToUint32() );
			new (vertices) DebugVertex( localVertices[(v+1) % localVertices.Size()], color.ToUint32() );
		}
	}

	// Generate fragment
	frame->AddDebugLines( vertices.TypedData(), vertices.Size(), overlay );
}

void CAreaConvex::SerialzieBuffers(IFile& file)
{
	// save planes
	for (Uint32 i=0; i<m_numPlanes; ++i)
	{
		file << m_planes[i];
	}

	// save vertices
	for (Uint32 i=0; i<m_numVertices; ++i)
	{
		file << m_vertices[i];
	}

	// save edges
	for (Uint32 i=0; i<m_numEdges; ++i)
	{
		Edge& e = m_edges[i];
		file << e.m_vertex;
		file << e.m_next;
		file << e.m_face;
		file << e.m_other;
		file << e.m_next;
		file << e.m_dir;
		file << e.m_plane;
	}

	// save faces
	for (Uint32 i=0; i<m_numFaces; ++i)
	{
		file << m_faces[i];
	}
}

void CAreaConvex::Serialize(IFile& file)
{
	if (file.IsGarbageCollector())
	{
		return;
	}

	if (file.IsWriter())
	{
		// save version number
		Uint8 ver = CONVEX_VERSION;
		file << ver;

		// General data
		file << m_offset;
		file << m_box.Min;
		file << m_box.Max;

		// save counts
		file << CCompressedNumSerializer(m_numPlanes);
		file << CCompressedNumSerializer(m_numVertices);
		file << CCompressedNumSerializer(m_numEdges);
		file << CCompressedNumSerializer(m_numFaces);

		// save buffers
		SerialzieBuffers(file);
	}
	else if (file.IsReader())
	{
		// load version number
		Uint8 ver = 0;
		file << ver;

		// General data
		if (ver > 1)
		{
			file << m_offset;
			file << m_box.Min;
			file << m_box.Max;
		}

		// load counts
		file << CCompressedNumSerializer(m_numPlanes);
		file << CCompressedNumSerializer(m_numVertices);
		file << CCompressedNumSerializer(m_numEdges);
		file << CCompressedNumSerializer(m_numFaces);

		// create buffers
		m_planes = (Vector*) RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_TriggerSystem, sizeof(Vector) * m_numPlanes, 16 );
		m_vertices = (Vector*) RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_TriggerSystem, sizeof(Vector) * m_numVertices, 16 );
		m_edges = (Edge*) RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_TriggerSystem, sizeof(Edge) * m_numEdges, 4 );
		m_faces = (Uint16*) RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_TriggerSystem, sizeof(Uint16) * m_numFaces, 4 );

		// load buffers
		SerialzieBuffers(file);
	}
}

void CAreaConvex::BuildBeveledConvex( const Float bevel, const Float bevelVertical, const Matrix& bevelSpace, TDynArray<Vector>& outPlanes ) const
{
	// Bevel space transform
	Matrix bevelSpaceInvTrans( bevelSpace );
	bevelSpaceInvTrans.FullInvert();
	bevelSpaceInvTrans.Transpose();

	// Create a set of beveled planes - start with faces
	TDynArray< Vector > beveledPlanes;
	for ( Uint32 i=0; i<m_numFaces; ++i )
	{
		const Vector& planeVector = m_planes[i];
		const Vector worldPlaneDir = bevelSpaceInvTrans.TransformVector( planeVector ).Normalized3();
		AddBevelPlane( m_vertices, m_numVertices, m_offset, beveledPlanes, planeVector, bevelSpace, bevel, bevelVertical );
	}

	// Add the "bevel set" of axis aligned bevel planes in the bevel space
	const Uint32 numBevelPlanes = 16;
	for (Uint32 i=0; i<numBevelPlanes; ++i)
	{
		const Float angle = ((Float)i / (Float)numBevelPlanes) * 360.0f;

		const Float dirX = Red::Math::MCos( DEG2RAD(angle) );
		const Float dirY = Red::Math::MSin( DEG2RAD(angle) );

		AddBevelPlane( m_vertices, m_numVertices, m_offset, beveledPlanes, Vector( dirX, dirY, 0.0f, 1.0f), bevelSpace, bevel, bevelVertical );
	}

	// Copy planes
	Swap( outPlanes, beveledPlanes );
}

void CAreaConvex::CompileDebugMesh( const Color& faceColor, TDynArray<DebugVertex>& outVertices, TDynArray<Uint32>& outIndices ) const
{
	// Generate faces directly from content
	for (Uint32 i=0; i<m_numFaces; ++i)
	{
		const Uint32 firstEdgeIndex = m_faces[i];
		const Uint32 firstVertex = outVertices.Size();

		// color variant
		const Float randFrac = GEngine->GetRandomNumberGenerator().Get< Float >();
		Uint32 realFaceColor = Color::Mul3(faceColor, 0.6f + 0.4f * randFrac).ToUint32();

		// generate vertices
		Uint32 numVertices = 0;
		Uint32 edge = firstEdgeIndex;
		do
		{
			Vector pos = m_vertices[m_edges[edge].m_vertex] + m_offset;
			new (outVertices) DebugVertex( pos, realFaceColor);
			numVertices += 1;
			edge = m_edges[edge].m_next;
		}
		while (edge != firstEdgeIndex);

		// generate indices
		for (Uint32 v=2; v<numVertices; ++v)
		{
			outIndices.PushBack(firstVertex + v);
			outIndices.PushBack(firstVertex + v-1);
			outIndices.PushBack(firstVertex + 0);
		}
	}
}

void CAreaConvex::CompileDebugEdges( const Color& faceColor, TDynArray<DebugVertex>& outVertices ) const
{
	// Generate faces
	for (Uint32 i=0; i<m_numFaces; ++i)
	{
		const Uint32 firstEdgeIndex = m_faces[i];
		const Uint32 firstVertex = outVertices.Size();

		TDynArray<Vector> localVertices;

		// generate vertices
		Uint32 numVertices = 0;
		Uint32 edge = firstEdgeIndex;
		do
		{
			const Vector& pos = m_vertices[m_edges[edge].m_vertex] + m_offset;
			localVertices.PushBack(pos);
			numVertices += 1;
			edge = m_edges[edge].m_next;
		}
		while (edge != firstEdgeIndex);

		// generate indices
		for (Uint32 v=0; v<localVertices.Size(); ++v)
		{
			new (outVertices) DebugVertex( localVertices[v], faceColor.ToUint32() );
			new (outVertices) DebugVertex( localVertices[(v+1) % localVertices.Size()], faceColor.ToUint32() );
		}
	}
}

void CAreaConvex::CacheEdgeData()
{
	// Map oposite edges
	for ( Uint32 i=0; i<m_numEdges; ++i )
	{
		const Edge& curEdge = m_edges[i];
		const Edge& nextEdge = m_edges[ curEdge.m_next ];

		// already matched
		if ( curEdge.m_other != 0 )
		{
			const Edge* other = curEdge.GetOtherEdge();
			ASSERT( other->GetOtherEdge() == &curEdge );

			continue;
		}

		// match edges
		Bool wasMatched = false;
		for ( Uint32 j=i+1; j<m_numEdges; ++j )
		{
			const Edge& otherEdge = m_edges[j];
			const Edge& otherNextEdge = m_edges[ otherEdge.m_next ];

			if (Match2(curEdge.m_vertex, nextEdge.m_vertex, 
				otherEdge.m_vertex, otherNextEdge.m_vertex))
			{
				const Int16 diff = (const Int16)j - (const Int16)i;
				m_edges[i].m_other = diff;
				m_edges[j].m_other = -diff;

				wasMatched = true;
				break;
			}
		}

		// edge not matched - should never ever happen
		if ( !wasMatched )
		{
			WARN_ENGINE( TXT("Unmatched edge in convex data") );
			continue;
		}
	}

	// Precalculate edge direction and inner vector
	for ( Uint32 i=0; i<m_numEdges; ++i )
	{
		Edge& e = m_edges[i];

		// calculate edge direction
		const Vector& a = m_vertices[ e.m_vertex ];
		const Vector& b = m_vertices[ m_edges[ e.m_next ].m_vertex ];
		e.m_dir = (b-a).Normalized3();
		e.m_dir.SetW( -Vector::Dot3( e.m_dir, a ) );

		// calculate edge plane (for Voronoi sets)
		const Vector facePlane = m_planes[ e.m_face ];
		const Vector edgeNormal = Vector::Cross( e.m_dir, facePlane ).Normalized3();
		e.m_plane = edgeNormal;
		e.m_plane.SetW( -Vector::Dot3( edgeNormal, a ) );
	}
}

CAreaConvex* CAreaConvex::CompileBakedCopy( const Matrix& bakeMatrix ) const
{
	// copy data
	CAreaConvex* copy = new CAreaConvex();
	copy->m_numEdges = m_numEdges;
	copy->m_edges = CopyDataBuffer( m_edges, m_numEdges, 16 );
	copy->m_numFaces = m_numFaces;
	copy->m_faces = CopyDataBuffer( m_faces, m_numFaces, 16 );
	copy->m_numPlanes = m_numPlanes;
	copy->m_planes = CopyDataBuffer( m_planes , m_numPlanes, 16 );
	copy->m_numVertices = m_numVertices;
	copy->m_vertices = CopyDataBuffer( m_vertices , m_numVertices, 16 );
	copy->m_offset = Vector::ZEROS;

	// calculate plane transform matrix
	Matrix bakeMatrixIT = bakeMatrix;
	bakeMatrixIT.FullInvert();
	bakeMatrixIT.Transpose();

	// transform all vertices and calculate new bounding box
	copy->m_box.Clear();
	for ( Uint32 i=0; i<copy->m_numVertices; ++i )
	{
		const Vector shapePos = m_vertices[i] + m_offset;
		const Vector newPos = bakeMatrix.TransformPoint( shapePos );

		copy->m_vertices[i] = newPos;
		copy->m_vertices[i].SetW( 1.0f );
		copy->m_box.AddPoint( newPos );
	}

	// transform planes
	ASSERT( copy->m_numPlanes == copy->m_numFaces );
	for ( Uint32 i=0; i<copy->m_numPlanes; ++i )
	{
		// transform plane normal
		copy->m_planes[i] = bakeMatrixIT.TransformVector( copy->m_planes[i] ).Normalized3();

		// calculate new offset using new plane position
		const Vector& facePoint = copy->m_vertices[ copy->m_edges[ copy->m_faces[i] ].m_vertex ];
		copy->m_planes[i].SetW( -Vector::Dot3( copy->m_planes[i], facePoint ) );
	}

	// recalculate edge data
	for ( Uint32 i=0; i<copy->m_numEdges; ++i )
	{
		Edge& e = copy->m_edges[i];

		// calculate edge direction
		const Vector& a = copy->m_vertices[ e.m_vertex ];
		const Vector& b = copy->m_vertices[ copy->m_edges[ e.m_next ].m_vertex ];
		e.m_dir = (b-a).Normalized3();
		e.m_dir.SetW( -Vector::Dot3( e.m_dir, a ) );

		// calculate edge plane (for Voronoi sets)
		const Vector facePlane = copy->m_planes[ e.m_face ];
		const Vector edgeNormal = Vector::Cross( e.m_dir, facePlane ).Normalized3();
		e.m_plane = edgeNormal;
		e.m_plane.SetW( -Vector::Dot3( edgeNormal, a ) );
	}

	return copy;
}

CAreaConvex* CAreaConvex::Build( const IAreaConvexDataSource& dataSource )
{
	// Generated vertex list
	TDynArray< Vector > vertices;
	TDynArray< Edge > edges;
	TDynArray< Uint16 > faces;

	// Generate faces
	const Uint32 numFaces = dataSource.GetNumFaces();
	for (Uint32 faceIndex = 0; faceIndex < numFaces; ++faceIndex)
	{
		const Uint32 numVertices = dataSource.GetNumFaceVertices(faceIndex);

		// Face has to few vertices
		if (numVertices < 3)
		{
			continue;
		}

		// Create vertices
		Uint16 firstEdge = 0;
		Vector centerPoint(0,0,0);
		for (Uint32 vertexIndex = 0; vertexIndex < numVertices; ++vertexIndex)
		{
			const Vector pos = dataSource.GetFaceVertex(faceIndex, vertexIndex);

			// Find in existing vertex list
			Int32 currentIndex = -1;
			for (Uint32 i=0; i<vertices.Size(); ++i)
			{
				const Float distanceLimit = 0.0005f;
				if (vertices[i].DistanceSquaredTo(pos) < (distanceLimit*distanceLimit))
				{
					currentIndex = i;
					break;
				}
			}

			// Add as new vertex
			if (currentIndex == -1)
			{
				currentIndex = vertices.Size();
				vertices.PushBack(pos);
			}

			// Create edge
			const Uint16 edgeIndex = (Uint16) edges.Size();
			Edge* newEdge = new ( edges ) Edge();
			newEdge->m_vertex = (Uint16)currentIndex;
			newEdge->m_other = 0;

			// Remember first edge
			if (vertexIndex == 0)
			{
				firstEdge = edgeIndex;
			}
		}


		// Fixup edges
		bool hasDuplicatedVertex = false;
		const Uint16 face = (Uint16) faces.Size();
		for (Uint32 vertexIndex = 0; vertexIndex < numVertices; ++vertexIndex)
		{
			Edge& currentEdge = edges[ firstEdge + vertexIndex ];
			currentEdge.m_next = (Uint16)(firstEdge + ((vertexIndex+1) % numVertices));
			currentEdge.m_face = face;

			// validate shape geometry
			for (Uint32 otherVertexIndex = 0; otherVertexIndex < numVertices; ++otherVertexIndex)
			{
				Edge& otherEdge = edges[ firstEdge + otherVertexIndex ];
				if ( otherVertexIndex != vertexIndex )
				{
					if ( otherEdge.m_vertex == currentEdge.m_vertex )
					{
						hasDuplicatedVertex = true;
					}
				}
			}
		}

		// Do not create area convex from invalid geometry
		if ( hasDuplicatedVertex )
		{
			ERR_ENGINE( TXT("Area convex has duplicated vertices in one of the face's geometry. No convex will be created.") );
			return nullptr;
		}

		// Create face (add index to first edge)
		faces.PushBack(firstEdge);
	}

	// Calculate and subtract center point
	Vector center(0,0,0);
	for (Uint32 i=0; i<vertices.Size(); ++i)
	{
		center += vertices[i];
	}
	center /= (Float)vertices.Size();

	// Subtract center point from vertex coordinates
	// The center point offset is preserved
	for (Uint32 i=0; i<vertices.Size(); ++i)
	{
		vertices[i] -= center;
		vertices[i].SetW(1.0f);
	}

	// Create initial plane list (one plane for each face)
	TDynArray< Vector > planes;
	planes.Resize(faces.Size());
	for (Uint32 i=0; i<faces.Size(); ++i)
	{
		const Uint16 firstEdge = faces[i];

		// Extract 3 first face's vertices 
		const Edge& edgeA = edges[ firstEdge ];
		const Vector& a = vertices[ edgeA.m_vertex ];

		const Edge& edgeB = edges[ edgeA.m_next ]; 
		const Vector& b = vertices[ edgeB.m_vertex ];

		const Edge& edgeC = edges[ edgeB.m_next ]; 
		const Vector& c = vertices[ edgeC.m_vertex ];

		// Calculate plane equation
		const Plane p( a, b, c );
		ASSERT(p.NormalDistance.Mag3() > 0.0f); 
		planes[i] = p.NormalDistance; // keep in vector form
	}

	// Create output data
	CAreaConvex* outData = new CAreaConvex();
	outData->m_numEdges = edges.Size();
	outData->m_numVertices = vertices.Size();
	outData->m_numFaces = faces.Size();
	outData->m_numPlanes = planes.Size();

	// Create buffers
	outData->m_edges = CopyDataBuffer( edges.TypedData(), edges.Size(), 16 );
	outData->m_faces = CopyDataBuffer( faces.TypedData(), faces.Size(), 16 );
	outData->m_vertices = CopyDataBuffer( vertices.TypedData(), vertices.Size(), 16 );
	outData->m_planes = CopyDataBuffer( planes.TypedData(), planes.Size(), 16 );

	// Store the offset
	outData->m_offset = center;

	// Calculate bounding box
	outData->m_box.Clear();
	for (Uint32 i=0; i<vertices.Size(); ++i)
	{
		outData->m_box.AddPoint(vertices[i]);
	}

	// Cache edge edge data
	outData->CacheEdgeData();

	// Return created convex
	LOG_ENGINE( TXT("Convex created, %d faces, %d vertices, %d edges, %d planes (%d bevel planes)"), 
		outData->m_numFaces, outData->m_numVertices, outData->m_numEdges, 
		outData->m_numPlanes, outData->m_numPlanes - outData->m_numFaces);
	return outData;
}

//---------------------------------------------------------------------------

CAreaShapeClosestPointFinder::CAreaShapeClosestPointFinder( const Matrix& localToWorld, class CAreaShape* data )
	: m_worldOffset( localToWorld.GetTranslation() )
	, m_shapeData( data )
{
	// keep a reference to original shape data (we share some of the pointers)
	m_shapeData->AddRef();

	// remove the translation from the local to world matrix but keep the rotation - this will be used as a "baked" shape reference
	// original translation is stored in m_worldOffset
	Matrix bakeMatrix = localToWorld;
	bakeMatrix.SetTranslation( Vector::ZEROS );

	// selective copy of the data - copy planes and vertices but keep the edges and faces shared (saves memory)
	const Uint32 numShapes = data->GetNumShapes();
	m_convexData.Resize( numShapes );
	for ( Uint32 i=0; i<numShapes; ++i )
	{
		m_convexData[i] = data->GetConvex(i)->CompileBakedCopy( bakeMatrix );
	}
}

CAreaShapeClosestPointFinder::~CAreaShapeClosestPointFinder()
{
	// release baked shapes
	m_convexData.ClearPtr();

	// we no longer need original data
	if ( NULL != m_shapeData )
	{
		m_shapeData->Release();
		m_shapeData = NULL;
	}
}

Bool CAreaShapeClosestPointFinder::FindClosestPoint( const Vector& worldPoint, const Float maxSearchDistance, Vector& outPoint, Float& outDistance ) const
{
	Bool result = false;

	// Keep the itermediate results on stack (faster)
	Vector closestSoFar(0,0,0);
	Float closestDistDoFar = FLT_MAX;

	// Convert test point to more SSE convinient vector
	Vector testPoint( worldPoint - m_worldOffset );
	testPoint.SetW( 1.0f );

	// Build the search volume, limit the search distance to already known limit
	Box searchBox( testPoint, maxSearchDistance );

	// Test each convex
	const Uint32 numShapes = m_convexData.Size();
	for (Uint32 i=0; i<numShapes; ++i)
	{
		const CAreaConvex& shape = *m_convexData[i];

		// Convex is not in the search area
		Box offsetBox( shape.GetBox() );
		offsetBox.Min += shape.GetOffset();
		offsetBox.Max += shape.GetOffset();
		if ( !offsetBox.Touches( searchBox ) )
		{
			continue;
		}

		// Find closest point
		if ( shape.FindClosestPoint( testPoint, closestSoFar, closestDistDoFar ) )
		{
			// inside
			if ( closestDistDoFar == 0.0f )
			{
				outPoint = worldPoint;
				outDistance = 0.0f;
				return true;
			}

			// limit the update the search bounds
			searchBox = Box( testPoint, closestDistDoFar );
			result = true;
		}
	}

	// Emit results
	if ( result )
	{
		outDistance = closestDistDoFar;
		outPoint = closestSoFar + m_worldOffset;
	}

	// Return result
	return result;
}

//---------------------------------------------------------------------------

CAreaShape::CAreaShape()
	: m_refCount(1)
{
}

CAreaShape::~CAreaShape()
{
	m_shapes.ClearPtr();
}

void CAreaShape::AddRef()
{
	m_refCount.Increment();
}

void CAreaShape::Release()
{
	ASSERT( m_refCount.GetValue() > 0 );
	if ( m_refCount.Decrement() == 0 )
	{
		delete this;
	}
}

Bool CAreaShape::Serialize(IFile& file, CAreaShape*& shape)
{
	if (!file.IsGarbageCollector())
	{
		CFileSkipableBlock fileBlock(file);

		if (file.IsWriter())
		{
			if (shape != NULL)
			{
				shape->Serialize(file);
			}
		}
		else if (file.IsReader())
		{
			if (fileBlock.HasData())
			{
				// Release reference on the existing shape since we are overriding it
				if( shape != nullptr )
				{
					shape->Release();
				}

				shape = new CAreaShape();
				if ( !shape->Serialize(file) )
				{
					// failed to load
					fileBlock.Skip();
					SAFE_RELEASE(shape);
					return false;
				}

			}
			else
			{
				SAFE_RELEASE(shape);
			}
		}
	}

	return true;
}

CAreaShape& CAreaShape::EMPTY()
{
	static CAreaShape EmptyShape;
	return EmptyShape;
}

Bool CAreaShape::IsEmpty() const
{
	return m_shapes.Empty();
}

void CAreaShape::Clear()
{
	m_shapes.ClearPtr();
}

void CAreaShape::AddConvex( const IAreaConvexDataSource& dataSource )
{
	CAreaConvex* convex = CAreaConvex::Build(dataSource);
	if (NULL != convex)
	{
		m_shapes.PushBack(convex);
	}
}

const Uint32 CAreaShape::CalcMemorySize() const
{
	Uint32 memSize = sizeof(CAreaShape);
	memSize += sizeof( m_shapes );
	for (Uint32 i=0; i<m_shapes.Size(); ++i)
	{
		memSize += m_shapes[i]->CalcMemorySize();
	}
	return memSize;
}

IRenderResource* CAreaShape::CompileDebugMesh( const Float bevel, const Float bevelVertical, const Matrix& bevelSpace ) const
{
	TDynArray<DebugVertex> vertices;
	TDynArray<Uint32> indices;

	// Reset random number generator (debug)
	GEngine->GetRandomNumberGenerator().Seed( 0 );

	// Non-beveled shape
	if ( Abs(bevel) < 0.01f )
	{
		// general face color
		Color faceColor;
		faceColor.R = 128;
		faceColor.G = 128;
		faceColor.B = 100;
		faceColor.A = 150;

		// Update covnex pieces
		for (Uint32 i=0; i<m_shapes.Size(); ++i)
		{
			m_shapes[i]->CompileDebugMesh( faceColor, vertices, indices );
		}
	}
	else
	{
		// general face color
		Color faceColor;
		faceColor.R = 64;
		faceColor.G = 64;
		faceColor.B = 64;
		faceColor.A = 100;

		// generate beveling planes for all convex parts
		TDynArray< TDynArray< Vector > > bevelPlanes;
		const Uint32 numShapes = m_shapes.Size();
		bevelPlanes.Resize( numShapes );
		for (Uint32 i=0; i<numShapes; ++i)
		{
			m_shapes[i]->BuildBeveledConvex( bevel, bevelVertical, bevelSpace, bevelPlanes[i] );
		}

		// create output geometry
		for (Uint32 i=0; i<numShapes; ++i)
		{
			ClipFaces( faceColor, i, bevelPlanes, vertices, indices );
		}
	}

	// Create mesh
	return GRender->UploadDebugMesh(vertices, indices);
}

Bool CAreaShape::Serialize( IFile& file )
{
	if ( !file.IsGarbageCollector() )
	{
		if ( file.IsWriter() )
		{
			// save version number
			Uint8 ver = CONVEX_VERSION;
			file << ver;

			// save shape
			Uint32 numShapes = m_shapes.Size();
			file << CCompressedNumSerializer(numShapes);

			// save planes
			for ( Uint32 i=0; i<numShapes; ++i )
			{
				CAreaConvex* shape = m_shapes[i];
				shape->Serialize( file );
			}
		}
		else if ( file.IsReader() )
		{
			// load version number
			Uint8 ver = 0;
			file << ver;

			// failed to load
			if (ver != CONVEX_VERSION)
			{
				return false;
			}

			// load plane count
			Uint32 numShapes = 0;
			file << CCompressedNumSerializer(numShapes);

			// Reserve array
			m_shapes.ClearPtr();
			m_shapes.Reserve(numShapes);

			// create shape list
			for ( Uint32 i=0; i<numShapes; ++i )
			{
				CAreaConvex* shape = new CAreaConvex();
				shape->Serialize( file );

				m_shapes.PushBack( shape );
			}
		}
	}

	// serialized
	return true;
}

Bool CAreaShape::PointOverlap(const Vector& point) const
{
	// test internal shapes
	const Uint32 numShapes = m_shapes.Size();
	for (Uint32 i=0; i<numShapes; ++i)
	{
		const CAreaConvex& shape = *m_shapes[i];
		const Vector localPoint(point - shape.GetOffset());
		if ( shape.PointOverlap( localPoint ) )
		{
			return true;
		}
	}

	// no overlap
	return false;
}

Bool CAreaShape::BoxOverlap( const Vector& center, const Vector& extents, const Vector& axisX, const Vector& axisY, const Vector& axisZ ) const
{
	// test internal shapes
	const Uint32 numShapes = m_shapes.Size();
	for (Uint32 i=0; i<numShapes; ++i)
	{
		const CAreaConvex& shape = *m_shapes[i];
		const Vector localCenter(center - shape.GetOffset());
		if ( shape.BoxOverlap( localCenter, extents, axisX, axisY, axisZ ) )
		{
			return true;
		}
	}

	// no overlap
	return false;
}

CAreaShapeClosestPointFinder* CAreaShape::CompileDistanceSearchData( const Matrix& localToWorld ) const
{
	return new CAreaShapeClosestPointFinder( localToWorld, const_cast< CAreaShape* >( this ) ); // we need non-const area for ref counting, blah
}

Bool CAreaShape::GetSurfacePoint( const Vector& localPoint, const Vector& localDir, Vector& outSurfacePoint ) const
{
	Bool result = false;
	Float bestFloorDistance = FLT_MAX;
	Vector bestLocalFloorPoint;

	// trace all convex pieces, get the minimum height at given location
	const Uint32 numShapes = m_shapes.Size();
	for (Uint32 i=0; i<numShapes; ++i)
	{
		const CAreaConvex& shape = *m_shapes[i];

		// convert position to convex space
		const Vector convexPoint = localPoint - shape.GetOffset();

		Vector shapeFloorPoint;
		if ( shape.GetSurfacePoint( convexPoint, localDir, shapeFloorPoint ) )
		{
			// Offset back to local space
			const Vector localFloorPoint = shapeFloorPoint + shape.GetOffset();
			const Float localFloorPointDist = Vector::Dot3( localFloorPoint, localDir );
			if ( localFloorPointDist < bestFloorDistance )
			{
				bestFloorDistance = localFloorPointDist;
				bestLocalFloorPoint = localFloorPoint;
				result = true;
			}
		}
	}

	// we got a result
	if ( result )
	{
		outSurfacePoint = bestLocalFloorPoint;
		return true;
	}

	// no intersection
	return false;
}

//---------------------------------------------------------------------------
