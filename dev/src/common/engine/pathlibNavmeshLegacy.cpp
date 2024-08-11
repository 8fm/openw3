/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibNavmeshLegacy.h"

#include "../core/factory.h"

#include "pathlibConst.h"
#include "pathlibSpatialQuery.h"
#include "pathlibNavmeshArea.h"
#include "pathlibSimpleBuffers.h"


const Uint16 CNavmesh::CURRENT_BIN_VERSION = 1;

///////////////////////////////////////////////////////////////////////////////
// CNavmeshFactory
///////////////////////////////////////////////////////////////////////////////
class CNavmeshFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CNavmeshFactory, IFactory, 0 );

public:
	CNavmeshFactory()
	{
		m_resourceClass = ClassID< CNavmesh >();
	}

	CResource* DoCreate( const FactoryOptions& options ) override
	{
		return ::CreateObject< CNavmesh >( options.m_parentObject );		
	}
};

BEGIN_CLASS_RTTI( CNavmeshFactory )
	PARENT_CLASS( IFactory )
	END_CLASS_RTTI()

	IMPLEMENT_ENGINE_CLASS( CNavmeshFactory );


IMPLEMENT_ENGINE_CLASS( CNavmesh );

///////////////////////////////////////////////////////////////////////////////
// CNavmesh
///////////////////////////////////////////////////////////////////////////////

CNavmesh::CNavmesh()
	: m_vertexCount( 0 )
	, m_triangleCount( 0 )
	, m_phantomEdgesCount( 0 )
	, m_binariesVersion( 0xffff )
	, m_vertexes( NULL )
	, m_triangleVertexes( NULL )
	, m_triangleAdjecent( NULL )
	, m_triangleNormals( NULL )
	, m_centralPoint( 0, 0, 0 )
	, m_radius( 1.f )
	, m_bbox( Vector(0,0,0,0), Vector(0,0,0,0) )
	, m_dataBuffer( TDataBufferAllocator< MC_PathLib >::GetInstance() )
{
}


CNavmesh::~CNavmesh()
{
	// all dynamic data are freed by destruction of DataBuffer
}

void CNavmesh::InitializeBuffers( Uint32 extraSpace )
{
	Uint32 initialBufferSize = m_vertexCount * sizeof( Vector3 ) + m_triangleCount * ( sizeof( CTriangleVertexes ) + sizeof( CTriangleBorder ) + sizeof( Vector3 ) ) + extraSpace;
	m_dataBuffer.Allocate( initialBufferSize );
	Int8* buffer = (Int8*) m_dataBuffer.GetData();
	m_vertexes				= (Vector3*) buffer;
	buffer = buffer + m_vertexCount * sizeof( Vector3 );
	m_triangleVertexes		= (CTriangleVertexes*) buffer;
	buffer = buffer + m_triangleCount * sizeof( CTriangleVertexes );
	m_triangleAdjecent		= (CTriangleBorder*) buffer;
	buffer = buffer + m_triangleCount * sizeof( CTriangleBorder );
	m_triangleNormals		= (Vector3*) buffer;
	buffer = buffer + m_triangleCount * sizeof( Vector3 );
}

void CNavmesh::RestoreDataPostSerialization( Bool scrapBinTree )
{
	Uint32 baseBufferSize = m_vertexCount * sizeof( Vector3 ) + m_triangleCount * ( sizeof( CTriangleVertexes ) + sizeof( CTriangleBorder ) + sizeof( Vector3 ) );
	Uint32 dataSize = static_cast< Uint32 >( m_dataBuffer.GetSize() );
	if ( dataSize >= baseBufferSize )
	{
		Int8* buffer = (Int8*) m_dataBuffer.GetData();
		m_vertexes				= (Vector3*) buffer;
		buffer = buffer + m_vertexCount * sizeof( Vector3 );
		m_triangleVertexes		= (CTriangleVertexes*) buffer;
		buffer = buffer + m_triangleCount * sizeof( CTriangleVertexes );
		m_triangleAdjecent		= (CTriangleBorder*) buffer;
		buffer = buffer + m_triangleCount * sizeof( CTriangleBorder );
		m_triangleNormals		= (Vector3*) buffer;
		buffer = buffer + m_triangleCount * sizeof( Vector3 );
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

}
void CNavmesh::Clear()
{
	m_dataBuffer.Clear();
	m_vertexes = NULL;
	m_triangleVertexes = NULL;
	m_triangleAdjecent = NULL;
	m_triangleCount = 0;
	m_triangleNormals = NULL;
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

	m_triangleCount = TriangleIndex( triangleVertexes.Size() / 3 );
	m_vertexCount = VertexIndex( vertexes.Size() );

	Bool correctTris = (m_triangleCount == triangleVertexes.Size() / 3);

	ASSERT( correctTris );

	// Invalid values
	if ( !correctTris )
	{
		return;
	}

	InitializeBuffers();

	// init: vertex array
	for ( VertexIndex i = 0; i < m_vertexCount; ++i )
	{
		m_vertexes[ i ] = vertexes[ i ];
	}

	// init: triangle vertex indexes array
	for ( TriangleIndex i = 0; i < m_triangleCount; ++i )
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

					ASSERT( adjTriIndex != triIndex, TXT("Please inform Michal Slapa of this problem!") );

					// Let other triangle know, that we are his neighbour
					Uint32 otherVertInd;
					for ( otherVertInd = 0; otherVertInd < 3; otherVertInd++ )
					{
						VertexIndex nv1 = m_triangleVertexes[adjTriIndex].m_vertex[otherVertInd];
						VertexIndex nv2 = m_triangleVertexes[adjTriIndex].m_vertex[ otherVertInd == 2 ? 0 : otherVertInd+1 ];
						if ( (nv1 == v1 && nv2 == v2) || (nv1 == v2 && nv2 == v1) )
						{
							if ( m_triangleAdjecent[adjTriIndex].m_triangle[otherVertInd] != INVALID_INDEX )
							{
								Vector3 vertsOut[2];
								GetTriangleEdge( adjTriIndex, otherVertInd, vertsOut );
								Vector3 centralPoint = (vertsOut[ 0 ] + vertsOut[ 1 ]) * 0.5f;

								m_triangleAdjecent[triIndex].m_triangle[triVert] = INVALID_INDEX;
								break;
							}
							m_triangleAdjecent[triIndex].m_triangle[triVert] = adjTriIndex;
							m_triangleAdjecent[adjTriIndex].m_triangle[otherVertInd] = triIndex;
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
	}

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

void CNavmesh::CopyFrom( CNavmesh* navmesh )
{
	m_vertexCount = navmesh->m_vertexCount;
	m_triangleCount = navmesh->m_triangleCount;
	m_centralPoint = navmesh->m_centralPoint;
	m_radius = navmesh->m_radius;
	m_bbox = navmesh->m_bbox;
	m_phantomEdgesCount = navmesh->m_phantomEdgesCount;
	m_dataBuffer = navmesh->m_dataBuffer;

	RestoreDataPostSerialization();

#ifdef DEBUG_NAVMESH_COLORS
	{
		m_triangleColours = navmesh->m_triangleColours;
	}
#endif
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

void CNavmesh::OnSerialize( IFile& file )
{
	if ( file.IsWriter() )
	{
		m_binariesVersion = CURRENT_BIN_VERSION;
	}
	TBaseClass::OnSerialize( file );
	m_dataBuffer.Serialize( file );

	if ( !file.IsGarbageCollector() && file.IsReader() )
	{
		RestoreDataPostSerialization( m_binariesVersion != 0xffff );
	}
}
Bool CNavmesh::IsValid()
{
	if ( m_vertexCount == 0 || m_triangleCount == 0 )
	{
		return false;
	}
	Uint32 baseBufferSize = m_vertexCount * sizeof( Vector3 ) + m_triangleCount * ( sizeof( CTriangleVertexes ) + sizeof( CTriangleBorder ) + sizeof( Vector3 ) );
	Uint32 dataSize = static_cast< Uint32 >( m_dataBuffer.GetSize() );
	if ( dataSize < baseBufferSize )
	{
		return false;
	}

	return true;
}

Bool CNavmesh::IsEmptyMesh() const
{
	return false;
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

void PathLib_TEMP_Convert( ::CNavmesh* legacyNavmesh, PathLib::CNavmesh* navmesh )
{
	TDynArray< Int8 > buffer;

	CSimpleBufferWriter writer( buffer, PathLib::CNavmesh::RES_VERSION );

	writer.Put( legacyNavmesh->m_vertexCount );
	writer.Put( legacyNavmesh->m_triangleCount );
	writer.Put( legacyNavmesh->m_phantomEdgesCount );
	writer.Put( legacyNavmesh->m_centralPoint );
	writer.Put( legacyNavmesh->m_radius );
	writer.Put( legacyNavmesh->m_bbox );

	Uint32 dataBufferSize = legacyNavmesh->m_dataBuffer.GetSize();

	writer.Put( dataBufferSize );
	writer.Write( legacyNavmesh->m_dataBuffer.GetData(), dataBufferSize );

	Uint16 problemsCount = 0;
	writer.Put( problemsCount );

	CSimpleBufferReader reader( (const Int8*) buffer.Data(), Uint32( buffer.DataSize() ) );

	navmesh->ReadFromBuffer( reader );
}
