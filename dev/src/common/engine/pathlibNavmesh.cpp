#include "build.h"
#include "pathlibNavmesh.h"

#include "../core/longBitField.h"

#include "pathlibConst.h"
#include "pathlibSimpleBuffers.h"
#include "pathlibSpatialQuery.h"
#include "pathlibNavmeshArea.h"

namespace PathLib
{

const Uint16 CNavmesh::RES_VERSION = 1;

///////////////////////////////////////////////////////////////////////////////
// SNavmeshProblem
///////////////////////////////////////////////////////////////////////////////
const Vector SNavmeshProblem::LOCATION_UNSPECIFIED = Vector( 0.f, 0.f, 0.f, -1.f );

Bool SNavmeshProblem::IsLocationUnspecified() const
{
	return m_location.W < 0.f;
}
SNavmeshProblem::SNavmeshProblem()
{

}
SNavmeshProblem::SNavmeshProblem( String&& t )
	: m_text( Move( t ) )
	, m_location( LOCATION_UNSPECIFIED )
{

}
SNavmeshProblem::SNavmeshProblem( String&& t, const Vector3& v )
	: m_text( Move( t ) )
	, m_location( v.X, v.Y, v.Z, 1.f )
{

}


void SNavmeshProblem::WriteToBuffer( CSimpleBufferWriter& writer ) const
{
	writer.SmartPut( m_text );
	writer.Put( m_location );
}
Bool SNavmeshProblem::ReadFromBuffer( CSimpleBufferReader& reader )
{
	if ( !reader.SmartGet( m_text ) 
		|| !reader.Get( m_location ) )
	{
		return false;
	}

	return true;
}
///////////////////////////////////////////////////////////////////////////////
// CNavmesh
///////////////////////////////////////////////////////////////////////////////

CNavmesh::CNavmesh()
	: m_vertexCount( 0 )
	, m_triangleCount( 0 )
	, m_phantomEdgesCount( 0 )
	//, m_isEmpty( false )
	, m_vertexes( NULL )
	, m_triangleVertexes( NULL )
	, m_triangleAdjecent( NULL )
	, m_triangleNormals( NULL )
	, m_centralPoint( 0, 0, 0 )
	, m_radius( 1.f )
	, m_bbox( Vector(0,0,0,0), Vector(0,0,0,0) )
	, m_dataBuffer( NULL )
	, m_queryDataInUse( 0 )
{
}


CNavmesh::~CNavmesh()
{
	FreeDataBuffer();
	ClearQueryData();
}

void CNavmesh::FreeDataBuffer()
{
	if ( m_dataBuffer )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_PathLib, m_dataBuffer );
		m_dataBuffer = NULL;
		m_dataBufferSize = 0;
	}
}

void CNavmesh::AllocateDataBuffer( Uint32 size )
{
	FreeDataBuffer();
	m_dataBufferSize = size;
	m_dataBuffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_PathLib, m_dataBufferSize );
}


void CNavmesh::InitializeBuffers( Uint32 extraSpace )
{
	Uint32 initialBufferSize = m_vertexCount * sizeof( Vector3 ) + m_triangleCount * ( sizeof( CTriangleVertexes ) + sizeof( CTriangleBorder ) + sizeof( Vector3 ) ) + extraSpace;
	AllocateDataBuffer( initialBufferSize );
	Int8* buffer = (Int8*) m_dataBuffer;
	m_vertexes				= (Vector3*) buffer;
	buffer = buffer + m_vertexCount * sizeof( Vector3 );
	m_triangleVertexes		= (CTriangleVertexes*) buffer;
	buffer = buffer + m_triangleCount * sizeof( CTriangleVertexes );
	m_triangleAdjecent		= (CTriangleBorder*) buffer;
	buffer = buffer + m_triangleCount * sizeof( CTriangleBorder );
	m_triangleNormals		= (Vector3*) buffer;
	buffer = buffer + m_triangleCount * sizeof( Vector3 );
}

void CNavmesh::RestoreDataPostSerialization()
{
	Uint32 baseBufferSize = m_vertexCount * sizeof( Vector3 ) + m_triangleCount * ( sizeof( CTriangleVertexes ) + sizeof( CTriangleBorder ) + sizeof( Vector3 ) );
	Uint32 dataSize = m_dataBufferSize;
	if ( dataSize >= baseBufferSize )
	{
		Int8* buffer = (Int8*) m_dataBuffer;
		m_vertexes				= (Vector3*) buffer;
		buffer = buffer + m_vertexCount * sizeof( Vector3 );
		m_triangleVertexes		= (CTriangleVertexes*) buffer;
		buffer = buffer + m_triangleCount * sizeof( CTriangleVertexes );
		m_triangleAdjecent		= (CTriangleBorder*) buffer;
		buffer = buffer + m_triangleCount * sizeof( CTriangleBorder );
		m_triangleNormals		= (Vector3*) buffer;
		buffer = buffer + m_triangleCount * sizeof( Vector3 );
		if ( dataSize > baseBufferSize )
		{
			m_binTree.SetDataBuffer( buffer, dataSize - baseBufferSize );
		}
	}
}
void CNavmesh::InitializeQueryData()
{
	ClearQueryData();
	for ( Uint32 i = 0; i < QUERY_DATA_CACHE; ++i )
	{
		m_queryData[ i ].SetValue( new CNavmeshQueryData( this ) );
	}
}
void CNavmesh::ClearQueryData()
{
	ASSERT( m_queryDataInUse.GetValue() == 0 );
	for ( Uint32 i = 0; i < QUERY_DATA_CACHE; ++i )
	{
		CNavmeshQueryData* queryData = m_queryData[ i ].Exchange( nullptr );
		if ( queryData )
		{
			delete queryData;
		}
	}
}
void CNavmesh::PostDataInitializationProcess()
{
	// init: central point and bounding box
	m_bbox = Box( Box::RESET_STATE );
	Vector3 centralPoint( 0, 0, 0 );
	Double vertImportance( 1.0 / Double(m_vertexCount) );
	for (VertexIndex i = 0; i < m_vertexCount; ++i)
	{
		centralPoint += Vector3( Float( m_vertexes[ i ].X * vertImportance ), Float( m_vertexes[ i ].Y * vertImportance ), Float( m_vertexes[ i ].Z * vertImportance ) );
		m_bbox.AddPoint( m_vertexes[ i ] );
	}

	// stretch bbox vertically
	m_bbox.Min.Z -= PathLib::GEOMETRY_AND_NAVMESH_MAX_DISTANCE;
	m_bbox.Max.Z += PathLib::GEOMETRY_AND_NAVMESH_MAX_DISTANCE;

	m_centralPoint = centralPoint;

	// init: radius
	Float maxRadiusSq = 0.f;
	for (VertexIndex i = 0; i < m_vertexCount; ++i)
	{
		Float radiusSq = (m_centralPoint.AsVector3() - m_vertexes[ i ]).SquareMag();
		maxRadiusSq = Max( maxRadiusSq, radiusSq );
	}
	m_radius = MSqrt( maxRadiusSq );

	// compute binary tree
	m_binTree.CreateTree( this );

	// grow data buffer and put binary tree inside it
	//{
	//	Uint32 pureDataSize = static_cast< Uint32 >( m_dataBuffer.GetSize() );
	//	// temporary copy all the data
	//	DataBuffer temporaryDataBuffer( m_dataBuffer );
	//	// reinitialize buffers (clearing it) - this will also compact bin tree
	//	InitializeBuffers();
	//	// copy all pure data back (without overwriting binary tree thats inside)
	//	Red::System::MemoryCopy( m_dataBuffer.GetData(), temporaryDataBuffer.GetData(), pureDataSize );
	//}

	InitializeQueryData();
}
void CNavmesh::Clear()
{
	ClearQueryData();
	FreeDataBuffer();
	m_vertexes = NULL;
	m_triangleVertexes = NULL;
	m_triangleAdjecent = NULL;
	m_triangleCount = 0;
	m_triangleNormals = NULL;
	m_binTree.Clear();
}
void CNavmesh::InitializeMesh( const TDynArray< Vector3 >& vertexes, const TDynArray< Uint16 >& triangleVertexes )
{
	struct Local
	{
		static Uint32 hash(Uint16 v1,Uint16 v2)
		{
			// v1 must be smaller one - for hash consistency
			if (v1 > v2)
			{
				Uint16 tmp = v1; v1 = v2; v2 = tmp;
			}
			Uint32 hash = v1;
			hash = hash << 16;
			hash |= v2;
			return hash;
		}
	};

#ifndef NO_NAVMESH_GENERATION
	m_problems.Clear();
#endif

	m_triangleCount = TriangleIndex( triangleVertexes.Size() / 3 );
	m_vertexCount = VertexIndex( vertexes.Size() );
	
	Bool correctTris = (m_triangleCount == triangleVertexes.Size() / 3);

	ASSERT( correctTris );

	// Invalid values
	if ( !correctTris )
	{
		return;
	}

	m_binTree.Clear();
	InitializeBuffers();

	// init: vertex array
	for ( VertexIndex i = 0; i < m_vertexCount; ++i )
	{
		m_vertexes[ i ] = vertexes[ i ];
	}

	// init: triangle vertex indexes array
	for ( Uint32 i = 0; i < m_triangleCount; ++i )
	{
		m_triangleVertexes[ i ].Set(
			triangleVertexes[ i*3 + 0 ],
			triangleVertexes[ i*3 + 1 ],
			triangleVertexes[ i*3 + 2 ]
			);
	}

	// init: triangle adjecency
	{
		// dynamic structures
		typedef THashMap< Uint32 , TriangleIndex > tTrianglesAdjecencyMap;
		tTrianglesAdjecencyMap triangleEdgeMap;

		for (TriangleIndex triIndex = 0; triIndex < m_triangleCount; triIndex++)
		{
			for (Uint32 triVert = 0; triVert < 3; triVert++)
			{
				VertexIndex v1 = m_triangleVertexes[triIndex].m_vertex[triVert];
				VertexIndex v2 = m_triangleVertexes[triIndex].m_vertex[ triVert == 2 ? 0 : triVert+1 ];
				Uint32 hash = Local::hash(v1,v2);
				// check map for adjecent triangle that was already found.
				auto itFind = triangleEdgeMap.Find(hash);
				if ( itFind != triangleEdgeMap.End() )
				{
					// Adjecent triangle found
					TriangleIndex adjTriIndex = itFind->m_second;

					ASSERT( adjTriIndex < triIndex, TXT("Please inform Michal Slapa of this problem!") );

					// Let other triangle know, that we are his neighbour
					Uint32 otherVertInd;
					for ( otherVertInd = 0; otherVertInd < 3; otherVertInd++ )
					{
						VertexIndex nv1 = m_triangleVertexes[adjTriIndex].m_vertex[otherVertInd];
						VertexIndex nv2 = m_triangleVertexes[adjTriIndex].m_vertex[ otherVertInd == 2 ? 0 : otherVertInd+1 ];
						if ( (nv1 == v1 && nv2 == v2) || (nv1 == v2 && nv2 == v1) )
						{
							if ( m_triangleAdjecent[adjTriIndex].m_triangle[otherVertInd] == INVALID_INDEX )
							{
								m_triangleAdjecent[triIndex].m_triangle[triVert] = adjTriIndex;
								m_triangleAdjecent[adjTriIndex].m_triangle[otherVertInd] = triIndex;
							}
							else
							{
								Vector3 vertsOut[2];
								GetTriangleEdge( adjTriIndex, otherVertInd, vertsOut );
								Vector3 centralPoint = (vertsOut[ 0 ] + vertsOut[ 1 ]) * 0.5f;
								
								SNavmeshProblem error( String::Printf( TXT("Problem in navmesh generation. Triangle connection (at %f, %f, %f) will be unavailable.\n"), centralPoint.X, centralPoint.Y, centralPoint.Z ), centralPoint );
								PATHLIB_ERROR( error.m_text.AsChar() );
#ifndef NO_NAVMESH_GENERATION
								NoticeProblem( Move( error ) );
#endif
								m_triangleAdjecent[triIndex].m_triangle[triVert] = INVALID_INDEX;
								break;
							}
							
							break;
						}
					}
					ASSERT(otherVertInd != 3);
				}
				else
				{
					m_triangleAdjecent[triIndex].m_triangle[triVert] = INVALID_INDEX;
					triangleEdgeMap.Insert(hash,triIndex);
				}
			}
		}
	}

	//FixOverlappingTriangles();

	// init: precalculate triangle normals
	for (TriangleIndex i = 0; i < m_triangleCount; ++i)
	{
		Vector3 verts[3];
		GetTriangleVerts(i,verts);
		m_triangleNormals[i] = ( verts[1] - verts[0] ).Cross( verts[2] - verts[0] );
		m_triangleNormals[i].Normalize();
		if ( Abs(m_triangleNormals[i].Z) < NumericLimits< Float >::Epsilon() )
		{
			SNavmeshProblem error( 
				String::Printf( TXT("NAVMESH ERROR! NORMAL IS HORIZONTAL %d; %f, %f, %f; %f, %f, %f; %f, %f, %f\n"),
				i, verts[0].X, verts[0].Y, verts[0].Z, verts[1].X, verts[1].Y, verts[1].Z, verts[2].X, verts[2].Y, verts[2].Z ),
				(verts[ 0 ] + verts[ 1 ] + verts[ 2 ]) / 3.f );
			PATHLIB_ERROR( error.m_text.AsChar() );
#ifndef NO_NAVMESH_GENERATION
			NoticeProblem( Move( error ) );
#endif
		}
		if (m_triangleNormals[i].Z < 0.f)
		{
			SNavmeshProblem error( 
				String::Printf( TXT("NAVMESH ERROR! NORMAL IS DOWN %d; %f, %f, %f; %f, %f, %f; %f, %f, %f\n"),
				i, verts[0].X, verts[0].Y, verts[0].Z, verts[1].X, verts[1].Y, verts[1].Z, verts[2].X, verts[2].Y, verts[2].Z ),
				(verts[ 0 ] + verts[ 1 ] + verts[ 2 ]) / 3.f );
			PATHLIB_ERROR( error.m_text.AsChar() );
#ifndef NO_NAVMESH_GENERATION
			NoticeProblem( Move( error ) );
#endif
		}
	}

	PostDataInitializationProcess();
}

Bool CNavmesh::FixOverlappingTriangles()
{
	struct Local
	{
		static Bool HasTriangleMeaningfulEdge( CNavmesh* navi, TriangleIndex tri1, TriangleIndex ignoreNeighbour )
		{
			TriangleIndex neighbours[ 3 ];
			navi->GetTriangleNeighbours( tri1, neighbours );
			for ( Uint32 i = 0; i < 3; ++i )
			{
				TriangleIndex tri2 = neighbours[ i ];
				if ( tri2 == INVALID_INDEX || tri2 == ignoreNeighbour )
					continue;

				TriangleIndex neighbourNeighbours[ 3 ];
				navi->GetTriangleNeighbours( tri2, neighbourNeighbours );

				Bool isOverlappingEdge = false;
				for ( Uint32 j = 0; j < 3; ++j )
				{
					if ( neighbourNeighbours[ j ] == tri1 )
					{
						// check for overlapping triangles
						CNavmesh::VertexIndex edge1[2];
						CNavmesh::VertexIndex edge2[2];
						navi->GetTriangleEdgeIndex( tri1, i, edge1 );
						navi->GetTriangleEdgeIndex( tri2, j, edge2 );

						if ( edge1[ 0 ] == edge2[ 0 ] && edge1[ 1 ] == edge2[1] )
						{
							isOverlappingEdge = true;
							break;
						}
					}
				}
				if ( !isOverlappingEdge )
				{
					return true;
				}
			}

			return false;
		}
	};

	Bool dirty = false;
	Bool allProblemsAreResolved = true;

	TDynArray< TriangleIndex > trianglesForRemoval;

	// iterate over all triangles!
	for ( TriangleIndex tri1Index = 0; tri1Index < m_triangleCount; tri1Index++ )
	{
		// iterate over all triangle edges
		TriangleIndex neighbours[ 3 ];
		GetTriangleNeighbours( tri1Index, neighbours );
		for ( Uint32 i = 0; i < 3; ++i )
		{
			TriangleIndex tri2Index = neighbours[ i ];
			if ( tri2Index == INVALID_INDEX || tri2Index > tri1Index )
				continue;

			TriangleIndex neighbourNeighbours[ 3 ];
			GetTriangleNeighbours( tri2Index, neighbourNeighbours );

			for ( Uint32 j = 0; j < 3; ++j )
			{
				if ( neighbourNeighbours[ j ] == tri1Index )
				{
					// check for overlapping triangles
					CNavmesh::VertexIndex edge1[2];
					CNavmesh::VertexIndex edge2[2];
					GetTriangleEdgeIndex( tri1Index, i, edge1 );
					GetTriangleEdgeIndex( tri2Index, j, edge2 );

					if ( edge1[ 0 ] == edge2[ 0 ] && edge1[ 1 ] == edge2[1] )
					{
						// overlapping edge
						dirty = true;

						// firstly try easiest case
						if ( !Local::HasTriangleMeaningfulEdge( this, tri1Index, tri2Index ) )
						{
							trianglesForRemoval.PushBackUnique( tri1Index );
						}
						else if ( !Local::HasTriangleMeaningfulEdge( this, tri2Index, tri1Index ) )
						{
							trianglesForRemoval.PushBackUnique( tri2Index );
						}
						else
						{
							allProblemsAreResolved = false;
						}
					}
				}
			}
		}
	}
	if ( dirty )
	{
		::Sort( trianglesForRemoval.Begin(), trianglesForRemoval.End(), ::Less< TriangleIndex >() );
		for ( Uint32 i = 0, n = trianglesForRemoval.Size(); i < n; ++i )
		{
			Uint32 shift = i+1;

			TriangleIndex startInd = trianglesForRemoval[ i ];
			TriangleIndex endInd = (i == n-1) ? m_triangleCount : trianglesForRemoval[ i+1 ];

			for ( Uint32 ind = startInd+1; ind < endInd; ++ind )
			{
				m_triangleAdjecent[ ind - shift ] = m_triangleAdjecent[ ind ];
				m_triangleVertexes[ ind - shift ] = m_triangleVertexes[ ind ];
			}
		}
		m_triangleCount -= TriangleIndex( trianglesForRemoval.Size() );

		CTriangleBorder* triangleAdjecent = new CTriangleBorder [ m_triangleCount ];
		CTriangleVertexes* triangleVertexes = new CTriangleVertexes [ m_triangleCount ];
		Vector3* vertexes = new Vector3[ m_vertexCount ];

		Red::System::MemoryCopy( triangleAdjecent, m_triangleAdjecent, m_triangleCount*sizeof(CTriangleBorder) );
		Red::System::MemoryCopy( triangleVertexes, m_triangleVertexes, m_triangleCount*sizeof(CTriangleVertexes) );
		Red::System::MemoryCopy( vertexes, m_vertexes, m_vertexCount*sizeof(Vector3) );

		InitializeBuffers();

		Red::System::MemoryCopy( m_triangleAdjecent, triangleAdjecent, m_triangleCount*sizeof(CTriangleBorder) );
		Red::System::MemoryCopy( m_triangleVertexes, triangleVertexes, m_triangleCount*sizeof(CTriangleVertexes) );
		Red::System::MemoryCopy( m_vertexes, vertexes, m_vertexCount*sizeof(Vector3) );

		delete [] triangleAdjecent;
		delete [] triangleVertexes;
		delete [] vertexes;
	}
	if ( !allProblemsAreResolved )
	{
		PATHLIB_ERROR( TXT("There are problems with navmesh. Some connections might be unaccessible!\n") );
	}
	return dirty;
}

// Algorithm that cuts generated but unused parts of navmesh based on "root points" - points that indicates
// navmesh consistant parts.
void CNavmesh::SimplifyUsingRootPoints( const TDynArray< Vector3 >& rootPoints )
{
	// STEP 1: paint graph from root points and determine which triangles "stay"

	// root points processing
	LongBitField processedTriangles;
	processedTriangles.Resize( m_triangleCount );
	processedTriangles.Clear();
	TriangleIndex newTrianglesCount = 0;
	TDynArray< TriangleIndex > processingQueue;
	processingQueue.Reserve( ::Min( m_triangleCount, TriangleIndex(100) ) );
	for ( auto it = rootPoints.Begin(), end = rootPoints.End(); it != end; ++it )
	{
		TriangleIndex rootTriangle = GetTriangle( *it );
		if ( rootTriangle == INVALID_INDEX )
		{
			// root point out of mesh
			continue;
		}

		if ( processedTriangles.IsBitSet( rootTriangle ) )
		{
			// root point already processed
			continue;
		}
		processedTriangles.SetBit( rootTriangle, true );
		++newTrianglesCount;
		processingQueue.PushBack( rootTriangle );
		while ( !processingQueue.Empty() )
		{
			TriangleIndex triangle = processingQueue.PopBackFast();
			TriangleIndex neighbours[3];
			GetTriangleNeighbours( triangle, neighbours );
			for ( Uint32 i = 0; i < 3; ++i )
			{
				TriangleIndex neighbour = neighbours[ i ];
				if ( neighbour == INVALID_INDEX )
				{
					// no neighbour
					continue;
				}
				if ( processedTriangles.IsBitSet( neighbour ) )
				{
					// already processed
					continue;
				}
				processedTriangles.SetBit( neighbour,true );
				++newTrianglesCount;
				processingQueue.PushBack( neighbour );
			}
		}
	}
	if ( newTrianglesCount == 0 )
	{
		// No available root points. We don't want to clear all mesh then, to simplify artists debugging.
		PATHLIB_LOG( TXT("No available root points for navmesh generation!\n") );
		return;
	}
	if ( newTrianglesCount == m_triangleCount )
	{
		// no changes
		return;
	}

	// STEP 2: Determine unused vertexes
	LongBitField processedVertexes;
	processedVertexes.Resize( m_vertexCount );
	VertexIndex newVertexCount = 0;

	for ( TriangleIndex i = 0; i < m_triangleCount; ++i )
	{
		if ( processedTriangles.IsBitSet( i ) )
		{
			VertexIndex triVerts[3];
			GetTriangleVertsIndex( i, triVerts );
			for ( Uint32 j = 0; j < 3; ++j )
			{
				VertexIndex vert = triVerts[j];
				if ( !processedVertexes.IsBitSet( vert ) )
				{
					processedVertexes.SetBit( vert, true );
					++newVertexCount;
				}
			}
		}
	}

	// STEP 3: allocate and fill buffers for new data
	TDynArray< Vector3 > newVertexesArray( newVertexCount );
	TDynArray< CTriangleVertexes > newTriangleVertexesArray( newTrianglesCount );
	TDynArray< CTriangleBorder > newTriangleAdjecentArray( newTrianglesCount );
	TDynArray< Vector3 > newTriangleNormalsArray( newTrianglesCount );

	TDynArray< VertexIndex > vertexArrayShift( m_vertexCount );
	TDynArray< TriangleIndex > triangleArrayShift( m_triangleCount );
	// squeze vertex data
	{
		VertexIndex vertShift = 0;
		for ( VertexIndex v = 0; v < m_vertexCount; ++v )
		{
			if ( processedVertexes.IsBitSet( v ) )
			{
				VertexIndex shiftedIdx = v - vertShift;
				vertexArrayShift[ v ] = shiftedIdx;
				newVertexesArray[ shiftedIdx ] = m_vertexes[ v ];
			}
			else
			{
				++vertShift;
				vertexArrayShift[ v ] = INVALID_INDEX;
			}
		}
		ASSERT( vertShift + newVertexCount == m_vertexCount );
	}

	// squeze triangles data
	{
		TriangleIndex tri = 0;
		while ( processedTriangles.IsBitSet( tri ) )
		{
			CTriangleVertexes vertsIndexes = m_triangleVertexes[ tri ];
			newTriangleVertexesArray[ tri ].Set( vertexArrayShift[ vertsIndexes.m_vertex[0] ], vertexArrayShift[ vertsIndexes.m_vertex[1] ], vertexArrayShift[ vertsIndexes.m_vertex[2] ] );
			newTriangleAdjecentArray[ tri ] = m_triangleAdjecent[ tri ];
			newTriangleNormalsArray[ tri ] = m_triangleNormals[ tri ];
			ASSERT( tri < m_triangleCount );
			triangleArrayShift[ tri ] = tri;
			++tri;
		}
		TriangleIndex indexShift = 0;
		for ( ; tri < m_triangleCount; ++tri )
		{
			if ( processedTriangles.IsBitSet( tri ) )
			{
				TriangleIndex shiftedIndex = tri - indexShift;
				CTriangleVertexes vertsIndexes = m_triangleVertexes[ tri ];
				newTriangleVertexesArray[ shiftedIndex ].Set( vertexArrayShift[ vertsIndexes.m_vertex[0] ], vertexArrayShift[ vertsIndexes.m_vertex[1] ], vertexArrayShift[ vertsIndexes.m_vertex[2] ] );
				newTriangleAdjecentArray[ shiftedIndex ] = m_triangleAdjecent[ tri ];
				newTriangleNormalsArray[ shiftedIndex ] = m_triangleNormals[ tri ];
				triangleArrayShift[ tri ] = shiftedIndex;
			}
			else
			{
				triangleArrayShift[ tri ] = INVALID_INDEX;
				++indexShift;
			}
		}
		ASSERT( indexShift + newTrianglesCount == m_triangleCount );

		// fix triangle adjecency data
		for ( tri = 0; tri < m_triangleCount; ++tri )
		{
			if ( triangleArrayShift[ tri ] != INVALID_INDEX )
			{
				CTriangleBorder& border = newTriangleAdjecentArray[ triangleArrayShift[ tri ] ];
				for ( Uint32 i = 0; i < 3; ++i )
				{
					if ( (border.m_triangle[ i ] & MASK_EDGE) == 0 )
					{
						border.m_triangle[ i ] = triangleArrayShift[ border.m_triangle[ i ] ];
					}
				}
			}
		}
	}

	
	// STER 4: reinitialize structure with new data
	m_binTree.Clear();

	m_triangleCount = TriangleIndex( newTrianglesCount );
	m_vertexCount = VertexIndex( newVertexCount );

	InitializeBuffers();

	Red::System::MemoryCopy( m_vertexes, newVertexesArray.Data(), newVertexesArray.DataSize() );
	Red::System::MemoryCopy( m_triangleVertexes, newTriangleVertexesArray.Data(), newTriangleVertexesArray.DataSize() );
	Red::System::MemoryCopy( m_triangleAdjecent, newTriangleAdjecentArray.Data(), newTriangleAdjecentArray.DataSize() );
	Red::System::MemoryCopy( m_triangleNormals, newTriangleNormalsArray.Data(), newTriangleNormalsArray.DataSize() );

	PostDataInitializationProcess();
}

void CNavmesh::CollectPhantomEdges( TDynArray< Vector3 >& outEdges )
{
	for ( TriangleIndex tri = 0; tri < m_triangleCount; ++tri )
	{
		TriangleIndex neighbours[ 3 ];
		GetTriangleNeighbours( tri, neighbours );
		for ( Uint32 i = 0; i < 3; ++i )
		{
			if ( IsPhantomEdge( neighbours[ i ] ) )
			{
				Vector3 verts[ 2 ];
				GetTriangleEdge( tri, i, verts );
				outEdges.PushBack( verts[ 0 ] );
				outEdges.PushBack( verts[ 1 ] );
			}
		}
	}
}
void CNavmesh::RemarkPhantomEdges( const TDynArray< Vector3 >& phantomEdges )
{
	struct Local
	{
		static Bool AcceptPoint( const Vector3& point, const Vector3& phantomEdge1, const Vector3& phantomEdge2 )
		{
			const Float DIFF = 0.1f;

			Vector3 bboxMin(
				Min( phantomEdge1.X, phantomEdge1.X ) - DIFF,
				Min( phantomEdge1.Y, phantomEdge1.Y ) - DIFF,
				Min( phantomEdge1.Z, phantomEdge1.Z ) - DIFF);
			Vector3 bboxMax(
				Max( phantomEdge1.X, phantomEdge1.X ) + DIFF,
				Max( phantomEdge1.Y, phantomEdge1.Y ) + DIFF,
				Max( phantomEdge1.Z, phantomEdge1.Z ) + DIFF);

			if ( point.X < bboxMin.X || point.X > bboxMax.X
				|| point.Y < bboxMin.Y || point.Y > bboxMax.Y 
				|| point.Z < bboxMin.Z || point.Z > bboxMax.Z )
			{
				return false;
			}
			return MathUtils::GeometryUtils::TestIntersectionCircleLine2D( point.AsVector2(), DIFF, phantomEdge1.AsVector2(), phantomEdge2.AsVector2() );
		}
	};

	Uint32 phantomVertsCount = phantomEdges.Size();
	if ( phantomVertsCount == 0 )
		return;

	m_phantomEdgesCount = 0;
	for ( TriangleIndex tri = 0; tri < m_triangleCount; ++tri )
	{
		TriangleIndex neighbours[ 3 ];
		GetTriangleNeighbours( tri, neighbours );
		for ( Uint32 edge = 0; edge < 3; ++edge )
		{
			if ( neighbours[ edge ] == INVALID_INDEX )
			{
				Vector3 verts[ 2 ];
				GetTriangleEdge( tri, edge, verts );
				
				for ( Uint32 i = 0; i < phantomVertsCount; i += 2 )
				{
					if ( !Local::AcceptPoint( verts[ 0 ], phantomEdges[ i ], phantomEdges[ i+1 ] ) )
						continue;

					Bool isPhantom = false;
					for ( Uint32 j = 0; j < phantomVertsCount; j += 2 )
					{
						// looks like square root performance cost, but it should be very rare case
						if ( Local::AcceptPoint( verts[ 0 ], phantomEdges[ j ], phantomEdges[ j+1 ] ) )
						{
							isPhantom = true;
							break;
						}
					}
					if ( isPhantom )
					{
						m_triangleAdjecent[ tri ].m_triangle[ edge ] = MASK_EDGE | (m_phantomEdgesCount++);
						break;
					}
				}

			}
		}
	}
}

void CNavmesh::MarkPhantomEdge( TriangleIndex tri, Uint32 n )
{
	if ( m_triangleAdjecent[tri].m_triangle[n] == INVALID_INDEX )
	{
		m_triangleAdjecent[tri].m_triangle[n] = MASK_EDGE | (m_phantomEdgesCount++);
	}
}
void CNavmesh::ClearPhantomEdge( TriangleIndex tri, Uint32 n )
{
	if ( IsPhantomEdge( m_triangleAdjecent[tri].m_triangle[n] ) )
	{
		TriangleIndex index = m_triangleAdjecent[tri].m_triangle[n] & (~MASK_EDGE);
		m_triangleAdjecent[tri].m_triangle[n] = INVALID_INDEX;
		--m_phantomEdgesCount;

		// correct all existing phantom edges
		for ( TriangleIndex tri = 0; tri < m_triangleCount; ++tri )
		{
			TriangleIndex neighbours[ 3 ];
			GetTriangleNeighbours( tri, neighbours );
			for ( Uint32 i = 0; i < 3; ++i )
			{
				if ( IsPhantomEdge( neighbours[ i ] ) )
				{
					TriangleIndex neighbourIndex = neighbours[ i ] & (~MASK_EDGE);
					if ( neighbourIndex > index )
					{
						// shift neighbour index 1 down
						neighbours[ i ] = MASK_EDGE | (neighbourIndex-1);
					}
				}
			}
		}
	}
}

template < class Predicate >
RED_INLINE Bool CNavmesh::AreaTest( const Vector3& basePos, const PathLib::CNavmeshAreaDescription* owner, CNavmeshQueryData& queryData, Predicate& c ) const
{

	struct Local : public Red::System::NonCopyable
	{
		CNavmeshQueryData::tStackToVisit& m_cStack;
		Local(CNavmeshQueryData::tStackToVisit& cStack) : m_cStack(cStack) {}
		~Local() { m_cStack.ClearFast(); }
	};
	
	TriangleIndex startingIndex = GetTriangle( basePos );
	if ( startingIndex == INVALID_INDEX )
		return false;

	Local cGuard( queryData.m_activeTriangles );
	CNavmeshQueryData::tTestMarker testMarker = ++queryData.m_currentTestMarker;

	queryData.m_testTriangles[startingIndex] = testMarker;
	queryData.m_activeTriangles.PushBack(startingIndex);
	while ( !queryData.m_activeTriangles.Empty() )
	{
		TriangleIndex currentTriangle = queryData.m_activeTriangles.Back();
		c.StartTriangle( currentTriangle );
		queryData.m_activeTriangles.PopBackFast();
		Vector3 triangleVerts[3];
		GetTriangleVerts( currentTriangle,triangleVerts );
		TriangleIndex neighbourList[3];
		GetTriangleNeighbours( currentTriangle, neighbourList );
		// Determine which neighbour triangles intersect test area
		for (Uint32 i = 0; i < 3; ++i)
		{
			Uint32 v1 = i;
			Uint32 v2 = i == 2 ? 0 : i + 1;

			TriangleIndex neighbourIndex = neighbourList[i];
			// Thats not the optimal check - TODO: Optimize
			if ( (neighbourIndex & MASK_EDGE) == 0 && queryData.m_testTriangles[neighbourIndex] == testMarker )
			{
				continue;
			}

			if ( c.Intersect( triangleVerts[v1].AsVector2(), triangleVerts[v2].AsVector2() ) )
			{
				if ( neighbourIndex & MASK_EDGE )
				{
					Uint32 flags;
					if ( neighbourIndex == INVALID_INDEX || ((flags = c.GetQueryFlags()) & (PathLib::CT_MULTIAREA | PathLib::CT_IGNORE_OTHER_AREAS)) == 0 )
					{
						if ( Predicate::AUTOFAIL )
						{
							return false;
						}
						else	// this code shouldn't be assembled if AUTOFAIL is set.
						{
							// signal failure
							c.OnFail( triangleVerts[v1], triangleVerts[v2] );
						}	
					}
					else
					{
						// phantom edge, and we are in multiarea test or we ignore areas
						if ( (flags & PathLib::CT_IGNORE_OTHER_AREAS) == 0 )
						{
							Uint32 neighbourId = PhantomEdgeNeighbourIndex( neighbourIndex );
							PathLib::AreaId areaId = owner->GetNeighbourAreaId( neighbourId );
							if ( areaId == INVALID_INDEX )
							{
								// fail test! Kopi&pejst from above
								if ( Predicate::AUTOFAIL )
								{
									return false;
								}
								else
								{
									c.OnFail( triangleVerts[v1], triangleVerts[v2] );
								}	
							}
							else
							{
								// push multiarea test
								auto& multiAreaData = c.GetMultiareaData();
								Vector3 edgePos = (triangleVerts[v1] + triangleVerts[v2]) * 0.5f;
								multiAreaData.PushAreaUnique( areaId, edgePos );
								multiAreaData.NoticeAreaEdgeHit();
							}
						}
					}
				}
				else
				{
					queryData.m_testTriangles[neighbourIndex] = testMarker;
					queryData.m_activeTriangles.PushBack(neighbourList[i]);
				}
			}
		}
		if ( c.EndTriangle() )
			break;
	}

	return c.FinalCheck(*this);
}

Bool CNavmesh::ComputeHeight(const Vector3& pos, Float& outHeight)
{
	// Notice that currently this calculation implies that there are no triangles with horizontal normal in navmesh.
	TriangleIndex nTriangle = GetTriangle( pos );
	if (nTriangle == INVALID_INDEX)
	{
		return false;
	}
	Vector3 verts[3];
	GetTriangleVerts( nTriangle, verts );

	ASSERT( MathUtils::GeometryUtils::IsPointInsideTriangle2D( verts[0].AsVector2(), verts[1].AsVector2(), verts[2].AsVector2(), pos.AsVector2() ) );
	// Calculate normal of triangle
	const Vector3& vNormal = GetTriangleNormal( nTriangle );
	// Calculate distance

	outHeight = - ( Vector3(pos.X,pos.Y,0.f) - verts[0] ).Dot( vNormal );
	outHeight /= vNormal.Z;		// == Dot( Vec3(0,0,1), vNormal ) where this first one is ray direction
	// basically thats just it
	return true;
}
Bool CNavmesh::ComputeHeight(const Vector2& pos, Float minZ, Float maxZ, Float& outHeight)
{
	TriangleIndex nTriangle = m_binTree.FindTriangle( pos, minZ, maxZ, this );
	if ( nTriangle == INVALID_INDEX )
	{
		return false;
	}
	outHeight = ComputeHeight( nTriangle, pos );
	return true;
}
Float CNavmesh::ComputeHeight( TriangleIndex triIndex, const Vector2& pos )
{
	Vector3 triangleVerts[3];
	GetTriangleVerts( triIndex, triangleVerts );

	//ASSERT( Math::IsPointInsideTriangle2D( triangleVerts[0], triangleVerts[1], triangleVerts[2], vPos ) );
	// Calculate normal of triangle
	const Vector3& triangleNormal = GetTriangleNormal( triIndex );
	// Calculate distance
	Float outHeight = - ( Vector3(pos.X, pos.Y, 0) - triangleVerts[0]).Dot( triangleNormal );
	outHeight /= triangleNormal.Z;		// == Dot( Vec3(0,0,1), vNormal ) where this first one is ray direction
	// basically thats just it
	return outHeight;
}
Bool CNavmesh::ComputeAverageHeight( const Box& bbox, Float& zAverage, Float& zMin, Float& zMax )
{
	return m_binTree.ComputeAverageHeight( bbox.Min.AsVector3(), bbox.Max.AsVector3(), this, zAverage, zMin, zMax );
}
CNavmesh::TriangleIndex CNavmesh::GetTriangle( const Vector3& pos ) const
{
	return m_binTree.FindTriangle( pos.AsVector2(), pos.Z - PathLib::GEOMETRY_AND_NAVMESH_MAX_DISTANCE, pos.Z + PathLib::GEOMETRY_AND_NAVMESH_MAX_DISTANCE, this );
}
CNavmesh::TriangleIndex CNavmesh::GetTriangleBelow( const Vector3& pos ) const
{
	return m_binTree.FindTriangle( pos.AsVector2(), NumericLimits< Float >::Min(), pos.Z + PathLib::GEOMETRY_AND_NAVMESH_MAX_DISTANCE, this );
}

template <>
Bool CNavmesh::SpatialQuery< PathLib::CCircleQueryData >( PathLib::CCircleQueryData& query, const PathLib::CNavmeshAreaDescription* owner, CNavmeshQueryData& data ) const
{
	struct Predicate : public QueryPredicate< PathLib::CCircleQueryData >
	{
		Predicate( PathLib::CCircleQueryData& query ) : QueryPredicate( query ) {}
		RED_INLINE Bool Intersect( const Vector2& p0, const Vector2& p1 ) const
		{
			return MathUtils::GeometryUtils::TestIntersectionCircleLine2D( m_query.m_circleCenter.AsVector2(), m_query.m_radius, p0, p1 );
		}
	};
	Predicate predicate(query);
	return AreaTest( query.m_basePos, owner, data, predicate );
}
template <>
Bool CNavmesh::SpatialQuery< PathLib::CClosestObstacleCircleQueryData >( PathLib::CClosestObstacleCircleQueryData& query, const PathLib::CNavmeshAreaDescription* owner, CNavmeshQueryData& data ) const
{
	struct Predicate : public QueryPredicate< PathLib::CClosestObstacleCircleQueryData >
	{
		Predicate( PathLib::CClosestObstacleCircleQueryData& query ) : QueryPredicate( query ) {}

		Float m_lastDistSq;
		Vector2 m_lastClosestPos;

		RED_INLINE Bool Intersect( const Vector2& v1, const Vector2& v2 )
		{
			Vector2 v;
			const Vector2& circleCenter = m_query.m_circleCenter.AsVector2();
			MathUtils::GeometryUtils::TestClosestPointOnLine2D( circleCenter, v1, v2, v );
			Float distSq = ( v - circleCenter).SquareMag();
			if ( distSq <= m_query.m_closestDistSq )
			{
				m_lastDistSq = distSq;
				m_lastClosestPos = v;

				return true;
			}
			return false;
		}
		enum e { AUTOFAIL = false };
		RED_INLINE void OnFail( const Vector3& v1, const Vector3& v2 )
		{
			m_query.m_closestDistSq = m_lastDistSq;
			m_query.m_pointOut.AsVector2() = m_lastClosestPos;
			// TODO: calculate Z
			m_query.m_pointOut.Z = m_query.m_circleCenter.Z;
			m_query.m_obstacleHit = true;
		}
	} predicate( query );
	if ( !AreaTest( query.m_basePos, owner, data, predicate ) )
	{
		// starting point test failed
		query.m_pointOut = query.m_basePos;
		query.m_closestDistSq = 0.f;
		query.m_obstacleHit = true;
		return false;
	}
	return true;
}

template <>
Bool CNavmesh::SpatialQuery< PathLib::CCollectCollisionPointsInCircleQueryData >( PathLib::CCollectCollisionPointsInCircleQueryData& query, const PathLib::CNavmeshAreaDescription* owner, CNavmeshQueryData& data ) const
{
	struct Predicate : public QueryPredicate< PathLib::CCollectCollisionPointsInCircleQueryData >
	{
		Predicate( PathLib::CCollectCollisionPointsInCircleQueryData& query ) : QueryPredicate( query ) {}

		enum e { AUTOFAIL = false };
		RED_INLINE void OnFail( const Vector3& v1, const Vector3& v2 )
		{
			Vector2 closestSpot;
			m_query.ClosestSpotSegment( v1.AsVector2(), v2.AsVector2(), closestSpot );
			m_query.NoticeSpot( closestSpot );
		}
	} predicate( query );

	AreaTest( query.m_basePos, owner, data, predicate );

	return true;
}


template <>
Bool CNavmesh::SpatialQuery< PathLib::CLineQueryData >( PathLib::CLineQueryData& query, const PathLib::CNavmeshAreaDescription* owner, CNavmeshQueryData& data ) const
{
	struct Predicate : public QueryPredicate< PathLib::CLineQueryData >
	{
		Predicate( PathLib::CLineQueryData& query )
			: QueryPredicate( query ) {}

		RED_INLINE Bool Intersect(const Vector2& point0, const Vector2& point1)
		{
			if ( MathUtils::GeometryUtils::PointIsLeftOfSegment2D( point0, point1, m_query.m_v2.AsVector2() ) <= 0.f )
			{
				return MathUtils::GeometryUtils::TestIntersectionLineLine2D( point0, point1, m_query.m_v1.AsVector2(), m_query.m_v2.AsVector2() );
			}
			return false;
		}
	};

	struct FinalCheckPredicate : public Predicate
	{
		const CNavmesh* m_this;
		Bool m_isInside;
		Bool m_multipleFinalTriangles;
		TriangleIndex m_currentTriangle;
		TriangleIndex m_finalTriangle;
		CNavmeshQueryData& m_data;
		FinalCheckPredicate( PathLib::CLineQueryData& query, const CNavmesh* me, CNavmeshQueryData& data )
			: Predicate( query )
			, m_this( me )
			, m_multipleFinalTriangles( false )
			, m_finalTriangle(INVALID_INDEX)
			, m_data( data ) {}

		RED_INLINE void StartTriangle( TriangleIndex n )
		{
			m_isInside = true;
			m_currentTriangle = n;
		}

		RED_INLINE Bool EndTriangle()
		{
			if ( m_isInside )
			{
				if ( m_finalTriangle != INVALID_INDEX )
					m_multipleFinalTriangles = true;
				m_finalTriangle = m_currentTriangle;
			}
			return false;
		}

		RED_INLINE Bool Intersect( const Vector2& vP0, const Vector2& vP1 )
		{
			if ( MathUtils::GeometryUtils::PointIsLeftOfSegment2D( vP0, vP1, m_query.m_v2.AsVector2() ) <= 0.f )
			{
				m_isInside = false;
				return MathUtils::GeometryUtils::TestIntersectionLineLine2D( vP0, vP1, m_query.m_v1.AsVector2(), m_query.m_v2.AsVector2() );
			}
			return false;
		}

		RED_INLINE Bool FinalCheck( const CNavmesh& cArea ) const
		{
			//ASSERT( m_finalTriangle != INVALID_INDEX );			sorry but because of some floating point arithmetics - this could happen (but very seldomly)
			if ( m_finalTriangle != INVALID_INDEX && !m_multipleFinalTriangles && (m_query.m_flags & PathLib::CT_NO_ENDPOINT_TEST) == 0 )
			{
				// (CTRL+C * CTRL+V) == EVIL
				Float minZ = m_query.m_v2.Z - PathLib::GEOMETRY_AND_NAVMESH_MAX_DISTANCE;
				Float maxZ = m_query.m_v2.Z + PathLib::GEOMETRY_AND_NAVMESH_MAX_DISTANCE;
				Vector3 triangleVerts[3];
				m_this->GetTriangleVerts( m_currentTriangle, triangleVerts );
				// In most cases triangles are inside 'Z' boundings
				Bool bTriangleInsideBounding = true;
				Bool bAllHigher = true;
				Bool bAllLower = true;
				for ( Int32 nVert = 0; nVert < 3; ++nVert )
				{
					const Float fZ = triangleVerts[nVert].Z;
					if (fZ < minZ || fZ > maxZ)
						bTriangleInsideBounding = false;

					if (fZ > minZ)
						bAllLower = false;

					if (fZ < maxZ)
						bAllHigher = false;
				}
				if ( bAllHigher || bAllLower )
				{
					return false;
				}
				if ( !bTriangleInsideBounding )
				{
					// If not - compute triangle height in given point
					// Calculate normal of triangle
					const Vector3& triangleNormal = m_this->GetTriangleNormal( m_currentTriangle );
					// Calculate distance
					Float fZ = -( Vector3( m_query.m_v2.X, m_query.m_v2.Y, 0.f ) - triangleVerts[0] ).Dot( triangleNormal );
					fZ /= triangleNormal.Z;		// == Dot( Vec3(0,0,1), triangleNormal ) where this first one is ray direction
					// basically thats just it
					if (fZ < minZ || fZ > maxZ)
					{
						return false;
					}
				}					
			}
			else
			{
				// old style check
				TriangleIndex triIndex = cArea.GetTriangle(m_query.m_v2);
				if ( triIndex == INVALID_INDEX || m_data.m_testTriangles[triIndex] != m_data.m_currentTestMarker )
					return false;
			}

			return true;
		}
	};

	if ( !(query.m_flags & PathLib::CT_NO_ENDPOINT_TEST) )
	{
		FinalCheckPredicate predicate( query, this, data );
		return AreaTest( query.m_basePos, owner, data, predicate );
	}
	else
	{
		Predicate predicate( query );
		return AreaTest( query.m_basePos, owner, data, predicate );
	}
}

template <>
Bool CNavmesh::SpatialQuery< PathLib::CWideLineQueryData >( PathLib::CWideLineQueryData& query, const PathLib::CNavmeshAreaDescription* owner, CNavmeshQueryData& data ) const
{
	struct Predicate : public QueryPredicate< PathLib::CWideLineQueryData >
	{
		CNavmeshQueryData& m_data;
		Float m_rSq;
		Predicate( PathLib::CWideLineQueryData& query, CNavmeshQueryData& data )
			: QueryPredicate( query ), m_data( data ), m_rSq( query.m_radius*query.m_radius ) {}
		RED_INLINE Bool Intersect(const Vector2& vP0, const Vector2& vP1) const
		{
			return MathUtils::GeometryUtils::TestDistanceSqrLineLine2D( vP0, vP1, m_query.m_v1.AsVector2(), m_query.m_v2.AsVector2() ) <= m_rSq;
		}
		RED_INLINE Bool FinalCheck( const CNavmesh& cArea ) const
		{
			if ( !(m_query.m_flags & PathLib::CT_NO_ENDPOINT_TEST) )
			{
				TriangleIndex nTri = cArea.GetTriangle( m_query.m_v2 );
				if ( nTri == INVALID_INDEX || m_data.m_testTriangles[nTri] != m_data.m_currentTestMarker )
					return false;
			}

			return true;
		}

	};
	ASSERT( query.m_radius > 0.f );
	Predicate predicate( query, data );
	return AreaTest( query.m_basePos, owner, data, predicate );
}

template <>
Bool CNavmesh::SpatialQuery< PathLib::CClosestObstacleWideLineQueryData >( PathLib::CClosestObstacleWideLineQueryData& query, const PathLib::CNavmeshAreaDescription* owner, CNavmeshQueryData& data ) const
{
	struct Predicate : public QueryPredicate< PathLib::CClosestObstacleWideLineQueryData >
	{
		Predicate( PathLib::CClosestObstacleWideLineQueryData& query )
			: QueryPredicate( query ) {}

		Vector2		m_vG;
		Vector2		m_vL;
		Float		m_dist2DSq;

		RED_INLINE Bool Intersect( const Vector2& vP0, const Vector2& vP1 )
		{
			Vector2 vG, vL;
			MathUtils::GeometryUtils::ClosestPointsLineLine2D( vP0.AsVector2(), vP1.AsVector2(), m_query.m_v1.AsVector2(), m_query.m_v2.AsVector2(), vG, vL );
			Float distSq = ( vG - vL ).SquareMag();
			if ( distSq <= m_query.m_closestDistSq )
			{
				m_vG = vG;
				m_vL = vL;
				m_dist2DSq = distSq;
				return true;
			}
			return false;
		}
		enum { AUTOFAIL = false };
		RED_INLINE void OnFail( const Vector3& vP0, const Vector3& vP1 )
		{
			m_query.m_closestDistSq = m_dist2DSq;
			m_query.m_closestPointOnSegment = m_vL;
			m_query.m_closestGeometryPoint = m_vG;
			// TODO: calculate Z
			m_query.m_closestPointOnSegment.Z = m_query.m_v1.Z;
			m_query.m_closestGeometryPoint.Z = m_query.m_v1.Z;
			m_query.m_obstacleHit = true;
		}
	} cPredicate( query );
	if ( !AreaTest( query.m_basePos, owner, data, cPredicate ) )
	{
		// initial point wasn't found!
		query.m_closestPointOnSegment = query.m_basePos;
		query.m_closestGeometryPoint = query.m_basePos;
		query.m_closestDistSq = 0.f;
		query.m_obstacleHit = true;
		return false;
	}
	return true;
}

template <>
Bool CNavmesh::SpatialQuery< PathLib::CClearWideLineInDirectionQueryData >( PathLib::CClearWideLineInDirectionQueryData& query, const PathLib::CNavmeshAreaDescription* owner, CNavmeshQueryData& data ) const
{
	struct Predicate : public QueryPredicate< PathLib::CClearWideLineInDirectionQueryData >
	{
		Float		m_dist2DSq;

		Predicate( PathLib::CClearWideLineInDirectionQueryData& query )
			: QueryPredicate( query )
			, m_dist2DSq( query.m_radius * query.m_radius ) {}

		RED_INLINE Bool Intersect( const Vector2& vP0, const Vector2& vP1 )
		{
			return m_query.Intersect( vP0, vP1 );
		}
		enum { AUTOFAIL = false };
		RED_INLINE void OnFail( const Vector3& vP0, const Vector3& vP1 )
		{
			struct Fun
			{
				Vector2			m_p0;
				Vector2			m_p1;

				Fun( const Vector2& p0, const Vector2& p1 )
					: m_p0( p0 )
					, m_p1( p1 ) {}

				RED_INLINE Bool operator()( const Vector2 v0, const Vector2& v1, Float ps ) const
				{
					return MathUtils::GeometryUtils::TestDistanceSqrLineLine2D( v0, v1, m_p0, m_p1 ) > (ps*ps);
				}
				RED_INLINE Bool operator()( const Vector2 v, Float ps ) const
				{
					return !MathUtils::GeometryUtils::TestIntersectionCircleLine2D( v, ps, m_p0, m_p1 );
				}
			} fun( vP0.AsVector2(), vP1.AsVector2() );

			m_query.OnIntersection( fun );
		}
	} cPredicate( query );
	if ( !AreaTest( query.m_basePos, owner, data, cPredicate ) )
	{
		query.SetFailedAtBasePos();
		return false;
	}
	return !query.m_isFailedAtBasePos;
}


template <>
Bool CNavmesh::SpatialQuery< PathLib::CCollectGeometryInCirceQueryData >( PathLib::CCollectGeometryInCirceQueryData& query, const PathLib::CNavmeshAreaDescription* owner, CNavmeshQueryData& data ) const
{
	struct Predicate : public QueryPredicate< PathLib::CCollectGeometryInCirceQueryData >
	{
		Predicate( PathLib::CCollectGeometryInCirceQueryData& query )
			: QueryPredicate( query ) {}
		enum { AUTOFAIL = false };
		RED_INLINE void OnFail( const Vector3& vP0, const Vector3& vP1 )
		{
			Uint32 i = m_query.m_output.Size();
			m_query.m_output.Grow( 2 );
			m_query.m_output[ i+0 ] = vP0;
			m_query.m_output[ i+1 ] = vP1;
		}
	} cPredicate( query );
	return AreaTest( query.m_basePos, owner, data, cPredicate );
}


template <>
Bool CNavmesh::SpatialQuery< PathLib::CRectangleQueryData >( PathLib::CRectangleQueryData& query, const PathLib::CNavmeshAreaDescription* owner, CNavmeshQueryData& data ) const
{
	struct Predicate : public QueryPredicate< PathLib::CRectangleQueryData >
	{
		Predicate( PathLib::CRectangleQueryData& query  )
			: QueryPredicate( query ) {}
	} predicate( query );
	return AreaTest( query.m_basePos, owner, data, predicate );
}

template <>
Bool CNavmesh::SpatialQuery< PathLib::CCustomTestQueryData >( PathLib::CCustomTestQueryData& query, const PathLib::CNavmeshAreaDescription* owner, CNavmeshQueryData& data ) const
{
	struct Predicate : public QueryPredicate< PathLib::CCustomTestQueryData >
	{
		Predicate( PathLib::CCustomTestQueryData& query  )
			: QueryPredicate( query ) {}
		RED_INLINE Bool Intersect(const Vector2& p0, const Vector2& p1) const
		{
			return m_query.m_customTester->IntersectLine( p0, p1 );
		}
	} predicate( query );
	return AreaTest( query.m_basePos, owner, data, predicate );
}

template Bool CNavmesh::SpatialQuery< PathLib::CCircleQueryData >( PathLib::CCircleQueryData& query, const PathLib::CNavmeshAreaDescription* owner ) const;
template Bool CNavmesh::SpatialQuery< PathLib::CClosestObstacleCircleQueryData >( PathLib::CClosestObstacleCircleQueryData& query, const PathLib::CNavmeshAreaDescription* owner ) const;
template Bool CNavmesh::SpatialQuery< PathLib::CCollectCollisionPointsInCircleQueryData >( PathLib::CCollectCollisionPointsInCircleQueryData& query, const PathLib::CNavmeshAreaDescription* owner ) const;
template Bool CNavmesh::SpatialQuery< PathLib::CLineQueryData >( PathLib::CLineQueryData& query, const PathLib::CNavmeshAreaDescription* owner ) const;
template Bool CNavmesh::SpatialQuery< PathLib::CWideLineQueryData >( PathLib::CWideLineQueryData& query, const PathLib::CNavmeshAreaDescription* owner ) const;
template Bool CNavmesh::SpatialQuery< PathLib::CClosestObstacleWideLineQueryData >( PathLib::CClosestObstacleWideLineQueryData& query, const PathLib::CNavmeshAreaDescription* owner ) const;
template Bool CNavmesh::SpatialQuery< PathLib::CClearWideLineInDirectionQueryData >( PathLib::CClearWideLineInDirectionQueryData& query, const PathLib::CNavmeshAreaDescription* owner ) const;
template Bool CNavmesh::SpatialQuery< PathLib::CCollectGeometryInCirceQueryData >( PathLib::CCollectGeometryInCirceQueryData& query, const PathLib::CNavmeshAreaDescription* owner ) const;
template Bool CNavmesh::SpatialQuery< PathLib::CRectangleQueryData >( PathLib::CRectangleQueryData& query, const PathLib::CNavmeshAreaDescription* owner ) const;
template Bool CNavmesh::SpatialQuery< PathLib::CCustomTestQueryData >( PathLib::CCustomTestQueryData& query, const PathLib::CNavmeshAreaDescription* owner ) const;

//Float CNavmesh::GetClosestFloorInRange( const Vector3& pos, Float radius, Vector3& pointOut )
//{
//	CQueryDataGuard guard( this );
//	return GetClosestFloorInRange( guard.m_data, pos, radius, pointOut );
//}

Bool CNavmesh::TestLocation(const Vector3& v1, Float radius)
{
	PathLib::CCircleQueryData query( PathLib::CT_DEFAULT, v1, radius );
	return SpatialQuery( query, NULL );
}
Bool CNavmesh::TestLine(const Vector3& pos1, const Vector3& pos2, Uint32 flags )
{
	PathLib::CLineQueryData query( flags, pos1, pos2 );
	return SpatialQuery( query, NULL );
}
Bool CNavmesh::TestLine(const Vector3& pos1, const Vector3& pos2, Float radius, Uint32 flags )
{
	PathLib::CWideLineQueryData query( flags, pos1, pos2, radius );
	return SpatialQuery( query, NULL );
}

CNavmesh::TriangleIndex CNavmesh::GetTriangleVisibleFrom( const Vector2& posAt, const Vector3& pointFrom)
{
	CQueryDataGuard guard( this );
	return GetTriangleVisibleFrom( guard.m_data, posAt, pointFrom );
}
Bool CNavmesh::ComputeHeightFrom( const Vector2& pos, const Vector3& posFrom, Float& outHeight )
{
	CQueryDataGuard guard( this );
	return ComputeHeightFrom( guard.m_data, pos, posFrom, outHeight );
}

//Float CNavmesh::GetClosestFloorInRange(const Vector3& vPos, Float fRadius, Vector3& vPointOut);
CNavmesh::TriangleIndex CNavmesh::GetTriangleVisibleFrom(CNavmeshQueryData& data, const Vector2& posAt, const Vector3& pointFrom)
{
	
	struct Predicate : public DefaultPredicate
	{
		CNavmesh* m_this;
		CNavmeshQueryData& m_queryData;
		Vector2 m_v1;
		Vector2 m_v2;
		Bool m_isInside;
		TriangleIndex m_finalTriangle;
		Predicate(CNavmesh* me, CNavmeshQueryData& queryData,const Vector2& v1, const Vector2& v2)
			: m_this(me), m_queryData(queryData), m_v1(v1), m_v2(v2) {}
		RED_INLINE void StartTriangle(TriangleIndex n)
		{
			m_isInside = true;
			m_finalTriangle = n;
		}
		RED_INLINE Bool EndTriangle()
		{
			return m_isInside;
		}
		RED_INLINE Bool Intersect(const Vector2& vP0, const Vector2& vP1)
		{
			if ( MathUtils::GeometryUtils::PointIsLeftOfSegment2D( vP0, vP1, m_v2 ) < 0.f )
			{
				m_isInside = false;
				return MathUtils::GeometryUtils::TestIntersectionLineLine2D(vP0,vP1,m_v1,m_v2);
			}
			return false;
		}
		TriangleIndex FindPoint( TriangleIndex n ) const
		{
			{
				Vector3 v[3];
				m_this->GetTriangleVerts( n, v );
				if ( MathUtils::GeometryUtils::IsPointInsideTriangle2D( v[0].AsVector2(), v[1].AsVector2(), v[2].AsVector2(), m_v2 ) )
				{
					return n;
				}
			}
			m_queryData.m_testTriangles[ n ] = m_queryData.m_currentTestMarker;
			TriangleIndex neig[3];
			m_this->GetTriangleNeighbours( n, neig );
			for( Uint32 i = 0; i < 3; ++i )
			{
				if ( (neig[i] & MASK_EDGE) == 0 && m_queryData.m_testTriangles[ neig[i] ] == m_queryData.m_currentTestMarker - 1 )
				{
					TriangleIndex nRet = FindPoint( neig[i] );
					if ( nRet != INVALID_INDEX )
						return nRet;
				}
			}
			return INVALID_INDEX;
		}
		RED_INLINE Bool FinalCheck( const CNavmesh& cArea )
		{
			if ( !m_isInside )
			{
				++m_queryData.m_currentTestMarker;
				// this shouldn't happen, but because of some floating point arithmetics this would happen seldomly
				m_finalTriangle = FindPoint( m_finalTriangle );
				ASSERT( m_finalTriangle != INVALID_INDEX );
				return m_finalTriangle != INVALID_INDEX;
			}

			return true;
		}
	} c(this, data, pointFrom.AsVector2(), posAt);
	if ( AreaTest( pointFrom, NULL, data, c ) )
	{
		return c.m_finalTriangle;
	}
	return INVALID_INDEX;
}
Bool CNavmesh::ComputeHeightFrom(CNavmeshQueryData& data, const Vector2& pos, const Vector3& posFrom, Float& outHeight)
{
	TriangleIndex nTriangle = GetTriangleVisibleFrom( data, pos, posFrom );
	if ( nTriangle == INVALID_INDEX )
		return false;
	
	outHeight = ComputeHeight( nTriangle, pos );
	return true;
}
void CNavmesh::CopyFrom( CNavmesh* navmesh )
{
	ClearQueryData();
	m_vertexCount = navmesh->m_vertexCount;
	m_triangleCount = navmesh->m_triangleCount;
	m_centralPoint = navmesh->m_centralPoint;
	m_radius = navmesh->m_radius;
	m_bbox = navmesh->m_bbox;
	m_phantomEdgesCount = navmesh->m_phantomEdgesCount;
	m_binTree.Clear();
	AllocateDataBuffer( navmesh->m_dataBufferSize );
	Red::System::MemoryCopy( m_dataBuffer, navmesh->m_dataBuffer, m_dataBufferSize );

	RestoreDataPostSerialization();
	InitializeQueryData();

#ifndef NO_NAVMESH_GENERATION
	m_problems = navmesh->m_problems;
#endif

#ifdef DEBUG_NAVMESH_COLORS
	{
		m_triangleColours = navmesh->m_triangleColours;
	}
#endif
}
CNavmesh::TriangleIndex CNavmesh::GetClosestTriangle( const Box& bbox )
{
	return m_binTree.GetClosestTriangleInside( bbox, this );
}


void CNavmesh::ComputeConvexBoundings( Box& outBBox, TDynArray< Vector2 >& outBoundings ) const
{
	outBBox.Clear();
	Float zMin = FLT_MAX;
	Float zMax = -FLT_MAX;
	TDynArray< Vector2 > pointsInput( m_vertexCount );
	for ( VertexIndex i = 0; i < m_vertexCount; ++i )
	{
		const Vector3& v = m_vertexes[ i ];
		zMin = Min( zMin, v.Z );
		zMax = Max( zMax, v.Z );
		pointsInput[ i ] = v.AsVector2();
	}
	outBBox.Min.Z = zMin;
	outBBox.Max.Z = zMax;
	MathUtils::GeometryUtils::ComputeConvexHull2D( pointsInput, outBoundings );
	for ( Uint32 i = 0, n = outBoundings.Size(); i != n; ++i )
	{
		const Vector2& v = outBoundings[ i ];
		outBBox.Min.X = Min( outBBox.Min.X, v.X );
		outBBox.Max.X = Max( outBBox.Max.X, v.X );
		outBBox.Min.Y = Min( outBBox.Min.Y, v.Y );
		outBBox.Max.Y = Max( outBBox.Max.Y, v.Y );
	}
}

Bool CNavmesh::IsValid()
{
	if ( m_vertexCount == 0 || m_triangleCount == 0 )
	{
		return false;
	}
	Uint32 baseBufferSize = m_vertexCount * sizeof( Vector3 ) + m_triangleCount * ( sizeof( CTriangleVertexes ) + sizeof( CTriangleBorder ) + sizeof( Vector3 ) );
	if ( m_dataBufferSize < baseBufferSize )
	{
		return false;
	}

	return true;
}
Bool CNavmesh::Save( const String& depotPath ) const
{
	TDynArray< Int8 > buffer;
	CSimpleBufferWriter writer( buffer, RES_VERSION );

	// serialize to buffer
	WriteToBuffer( writer );

	// save to file
	IFile* fileWriter = GFileManager->CreateFileWriter( depotPath );
	if( !fileWriter )
	{
		return false;
	}

	fileWriter->Serialize( buffer.Data(), buffer.DataSize() );
	delete fileWriter;
	
	return true;
}
//CNavmesh* CNavmesh::GetEmptyMesh()
//{
//	if ( !s_emptyNavmesh )
//	{
//		static CNavmesh emptyMesh;
//
//		s_emptyNavmesh = &emptyMesh;
//
//		TDynArray< Vector3 > verts( 3 );
//		TDynArray< Uint16 > indices( 3 );
//		indices[ 0 ] = 0;
//		indices[ 1 ] = 1;
//		indices[ 2 ] = 2;
//		verts[ 0 ].Set( 1.f, 0.f, 0.f );
//		verts[ 1 ].Set( 0.f, 1.f, 0.f );
//		verts[ 2 ].Set( -1.f, 0.f, 0.f );
//		s_emptyNavmesh->InitializeMesh( verts, indices );
//		s_emptyNavmesh->m_isEmpty = true;
//	}
//	return s_emptyNavmesh;
//}
//
//void CNavmesh::CopyFromEmpty()
//{
//	CNavmesh* emptyMesh = GetEmptyMesh();
//	CopyFrom( emptyMesh );
//	m_isEmpty = true;
//}

CNavmeshQueryData* CNavmesh::GetQueryData() const
{
	// enter critical section
	do
	{
		Int32 inUse = m_queryDataInUse.Increment();
		if ( inUse <= QUERY_DATA_CACHE )
		{
			break;
		}
		// NOTICE: active wait - basically it shouldn't happen if QUERY_DATA_CACHE is high enough
		m_queryDataInUse.Decrement();
		Red::Threads::SleepOnCurrentThread(0);
	}
	while ( true );

	CNavmeshQueryData* queryData;
	// for sure there is available
	for ( Uint32 index = 0; ; ++index )
	{
		ASSERT( index < QUERY_DATA_CACHE );
		queryData = m_queryData[ index ].Exchange( nullptr );
		if ( queryData )
		{
			break;
		}
	}

	return queryData;
}
void CNavmesh::ReturnBackQueryData( CNavmeshQueryData* queryData ) const
{
	for ( Uint32 index = 0; ; ++index )
	{
		if( m_queryData[ index ].CompareExchange( queryData, nullptr ) == nullptr )
		{
			break;
		}
	}
	m_queryDataInUse.Decrement();
}

void CNavmesh::WriteToBuffer( CSimpleBufferWriter& writer ) const
{
	writer.Put( m_vertexCount );
	writer.Put( m_triangleCount );
	writer.Put( m_phantomEdgesCount );
	writer.Put( m_centralPoint );
	writer.Put( m_radius );
	writer.Put( m_bbox );

	writer.Put( m_dataBufferSize );
	writer.Write( m_dataBuffer, m_dataBufferSize );

#ifndef NO_NAVMESH_GENERATION
	Uint16 problemsCount = Uint16( m_problems.Size() );
	writer.Put( problemsCount );
	for ( Uint16 i = 0; i < problemsCount; ++i )
	{
		m_problems[ i ].WriteToBuffer( writer );
	}
#else
	Uint16 problemsCount = 0;
	writer.Put( problemsCount );
#endif
}

Bool CNavmesh::ReadFromBuffer( CSimpleBufferReader& reader )
{
	if ( reader.GetVersion() != RES_VERSION )
	{
		return false;
	}

	if ( !reader.Get( m_vertexCount )
		|| !reader.Get( m_triangleCount )
		|| !reader.Get( m_phantomEdgesCount )
		|| !reader.Get( m_centralPoint )
		|| !reader.Get( m_radius )
		|| !reader.Get( m_bbox ) )
	{
		return false;
	}

	if ( !reader.Get( m_dataBufferSize ) )
	{
		return false;
	}

	AllocateDataBuffer( m_dataBufferSize );

	if ( !reader.Read( m_dataBuffer, m_dataBufferSize ) )
	{
		return false;
	}

	Uint16 problemsCount;
	if ( !reader.Get( problemsCount ) )
	{
		return false;
	}

#ifndef NO_NAVMESH_GENERATION
	m_problems.Resize( problemsCount );

	for ( Uint16 i = 0; i < problemsCount; ++i )
	{
		if ( !m_problems[ i ].ReadFromBuffer( reader ) )
		{
			return false;
		}
	}
#else
	for ( Uint16 i = 0; i < problemsCount; ++i )
	{
		SNavmeshProblem p;
		if ( !p.ReadFromBuffer( reader ) )
		{
			return false;
		}
	}
#endif


	RestoreDataPostSerialization();
	InitializeQueryData();

	return true;
}

void CNavmesh::OnPostLoad( CAreaDescription* area )
{

}

CNavmesh::TriangleIndex CNavmesh::Debug_GetTriangleIntersectingRay( const Vector3& rayOrigin, const Vector3& rayDir, Vector3& collisionPoint )
{
	Float closestSq = FLT_MAX;
	TriangleIndex bestTriangle = INVALID_INDEX;
	Vector3 tmpCollisionPoint;
	for( TriangleIndex tri = 0; tri < m_triangleCount; ++tri )
	{
		Vector3 triVerts[ 3 ];
		GetTriangleVerts( tri, triVerts );
		if ( MathUtils::GeometryUtils::TestRayTriangleIntersection3D( rayOrigin, rayDir, triVerts[ 0 ], triVerts[ 1 ], triVerts[ 2 ], tmpCollisionPoint ) )
		{
			Float distSq = ( tmpCollisionPoint - rayOrigin).SquareMag();
			if ( distSq < closestSq )
			{
				closestSq = distSq;
				bestTriangle = tri;
				collisionPoint = tmpCollisionPoint;
			}
		}
	}
	return bestTriangle;
}


///////////////////////////////////////////////////////////////////////////////
// CNavmeshRes
///////////////////////////////////////////////////////////////////////////////
void CNavmeshRes::CopyFrom( CNavmesh* navmesh )
{
	CNavmesh::CopyFrom( navmesh );
	MarkVersionDirty();
}

Bool CNavmeshRes::VHasChanged() const
{
	return !IsInitialVersion();
}
Bool CNavmeshRes::VSave( const String& depotPath ) const
{
	return CNavmesh::Save( depotPath );
}
Bool CNavmeshRes::VLoad( const String& depotPath, CAreaDescription* area )
{
	return CAreaRes::Load( this, depotPath );
}
void CNavmeshRes::VOnPostLoad( CAreaDescription* area )
{
	OnPostLoad( area );
}
const Char* CNavmeshRes::VGetFileExtension() const
{
	return GetFileExtension();
}
ENavResType CNavmeshRes::VGetResType() const
{
	return GetResType();
}

///////////////////////////////////////////////////////////////////////////////
// CNavmeshQueryData
///////////////////////////////////////////////////////////////////////////////
CNavmeshQueryData::CNavmeshQueryData( const CNavmesh* navmesh )
	: m_currentTestMarker( 1 )
{
	m_testTriangles = new tTestMarker[ navmesh->GetTrianglesCount() ];
	Red::System::MemorySet( m_testTriangles, 0, sizeof( tTestMarker ) * navmesh->GetTrianglesCount() );
}
CNavmeshQueryData::~CNavmeshQueryData()
{
	delete [] m_testTriangles;
}


};			// namespace PathLib

