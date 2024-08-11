#pragma once

#include "pathlibAreaRes.h"
#include "pathlibNavmeshBinTree.h"
#include "pathlibResPtr.h"
#include "pathlibSpatialQuery.h"


class CNavmeshQueryData;
namespace PathLib
{

class CNavmeshAreaDescription;
class CNavmeshBinTree;
class CNavmeshQueryData;
class CNavmeshFactory;


struct SNavmeshProblem
{
	static const Vector LOCATION_UNSPECIFIED;
	Bool IsLocationUnspecified() const;

	SNavmeshProblem();
	SNavmeshProblem( String&& t );
	SNavmeshProblem( String&& t, const Vector3& v );

	void					WriteToBuffer( CSimpleBufferWriter& writer ) const;
	Bool					ReadFromBuffer( CSimpleBufferReader& reader );

	String					m_text;
	Vector					m_location;
};



///////////////////////////////////////////////////////////////////////////////
// Navmesh and its collision algorithms implementation
///////////////////////////////////////////////////////////////////////////////
class CNavmesh
{
	friend class PathLib::CNavmeshBinTree;										// direct access to data buffer
public:
	typedef Uint16 VertexIndex;
	typedef Uint16 TriangleIndex;
	static const TriangleIndex INVALID_INDEX = 0xffff;
	static const TriangleIndex MASK_EDGE = 0x8000;
	static const Uint32 TRIANGLE_LIMIT = 0x7fff;
	static const Uint32 VERTEX_LIMIT = 0xffff;
	static const Uint16 RES_VERSION;
protected:
	void AllocateDataBuffer( Uint32 size );
	void FreeDataBuffer();

	void InitializeBuffers( Uint32 extraSpace = 0 );
	void RestoreDataPostSerialization();
	void InitializeQueryData();
	void ClearQueryData();
	void PostDataInitializationProcess();

	////////////////////////////////////////////////////////////////////////
	struct CQueryDataGuard : public Red::System::NonCopyable
	{
		RED_INLINE CQueryDataGuard( const CNavmesh* mesh )
			: m_mesh( mesh )
			, m_data( *mesh->GetQueryData() ) {}
		RED_INLINE ~CQueryDataGuard()
		{
			m_mesh->ReturnBackQueryData( &m_data );
		}
		const CNavmesh*		m_mesh;
		CNavmeshQueryData&	m_data;
	};
	////////////////////////////////////////////////////////////////////////
	// Walkable test
	struct DefaultPredicate : public Red::System::NonCopyable
	{
		PathLib::CDummyMultiAreaQueryInterface& GetMultiareaData()			{ static PathLib::CDummyMultiAreaQueryInterface s_shit; return s_shit; }
		enum e { AUTOFAIL = true };
		RED_INLINE Uint32	GetQueryFlags() const							{ return PathLib::CT_DEFAULT; }
		RED_INLINE void OnFail( const Vector3& v1, const Vector3& v2 ) const	{}
		RED_INLINE Bool FinalCheck( const CNavmesh& cArea ) const			{ return true; }			// no need for virtuals - methods can be just overrided
		RED_INLINE void StartTriangle( TriangleIndex n ) const			{}
		RED_INLINE Bool EndTriangle() const								{ return false; }
		RED_INLINE Bool Intersect( const Vector2& v1,const Vector2& v2 )	{ return false; }
	};
	template < class TQuery >
	struct QueryPredicate : public DefaultPredicate
	{
		TQuery&					m_query;
		QueryPredicate( TQuery& query )
			: m_query( query )												{}
		RED_INLINE Bool Intersect( const Vector2& v1,const Vector2& v2 )	{ return m_query.Intersect( v1, v2 ); }
		RED_INLINE Uint32	GetQueryFlags() const							{ return m_query.m_flags; }
		typename TQuery::MultiArea& GetMultiareaData()						{ return *TQuery::MultiArea::GetMultiAreaData( m_query ); }
	};
	template < class Predicate >
	RED_INLINE Bool AreaTest( const Vector3& basePos, const PathLib::CNavmeshAreaDescription* owner, CNavmeshQueryData& data, Predicate& c ) const;
	////////////////////////////////////////////////////////////////////////
	struct CTriangleVertexes
	{
		VertexIndex					m_vertex[3];
		void Set( VertexIndex v0, VertexIndex v1, VertexIndex v2 )
		{
			m_vertex[ 0 ] = v0;
			m_vertex[ 1 ] = v1;
			m_vertex[ 2 ] = v2;
		}
	};
	struct CTriangleBorder
	{
		TriangleIndex				m_triangle[3];
	};
	////////////////////////////////////////////////////////////////////////
	// Navigation mesh
	VertexIndex								m_vertexCount;
	TriangleIndex							m_triangleCount;
	TriangleIndex							m_phantomEdgesCount;
	//Bool									m_isEmpty;
	Vector3*								m_vertexes;
	CTriangleVertexes*						m_triangleVertexes;
	CTriangleBorder*						m_triangleAdjecent;
	Vector3*								m_triangleNormals;
	void*									m_dataBuffer;
	Uint32									m_dataBufferSize;
	Vector									m_centralPoint;
	Float									m_radius;
	Box										m_bbox;
	PathLib::CNavmeshBinTree				m_binTree;
#ifndef NO_NAVMESH_GENERATION
	TDynArray< SNavmeshProblem >			m_problems;						// saved generation errors
#endif

	typedef Red::Threads::CAtomic< CNavmeshQueryData* > NavmeshQueryPtr;
	typedef Red::Threads::CAtomic< Int32 > AtomicInt;

	static const Uint32						QUERY_DATA_CACHE = 2;
	mutable NavmeshQueryPtr					m_queryData[ QUERY_DATA_CACHE ];
	mutable AtomicInt						m_queryDataInUse;
public:

	CNavmesh();
	~CNavmesh();

	////////////////////////////////////////////////////////////////////////
	// Navigation mesh handling
	RED_INLINE TriangleIndex GetTrianglesCount() const					{ return m_triangleCount; }
	void GetTriangleVerts( TriangleIndex nTri, Vector3* vOut ) const
	{
		vOut[0] = m_vertexes[m_triangleVertexes[nTri].m_vertex[0]];
		vOut[1] = m_vertexes[m_triangleVertexes[nTri].m_vertex[1]];
		vOut[2] = m_vertexes[m_triangleVertexes[nTri].m_vertex[2]];
	}
	void GetTriangleVertsIndex( TriangleIndex nTri, VertexIndex* indOut ) const
	{
		indOut[0] = m_triangleVertexes[nTri].m_vertex[0];
		indOut[1] = m_triangleVertexes[nTri].m_vertex[1];
		indOut[2] = m_triangleVertexes[nTri].m_vertex[2];
	}
	void GetTriangleEdge( TriangleIndex nTri, Uint32 n, Vector3* vOut ) const
	{
		vOut[0] = m_vertexes[m_triangleVertexes[nTri].m_vertex[n]];
		vOut[1] = m_vertexes[m_triangleVertexes[nTri].m_vertex[n == 2 ? 0 : n + 1]];
	}
	void GetTriangleEdgeIndex( TriangleIndex nTri, Uint32 n, VertexIndex* indOut ) const
	{
		indOut[0] = m_triangleVertexes[nTri].m_vertex[n];
		indOut[1] = m_triangleVertexes[nTri].m_vertex[n == 2 ? 0 : n + 1];
	}
	void GetTriangleNeighbours( TriangleIndex nTri, TriangleIndex* aOut ) const
	{
		aOut[0] = m_triangleAdjecent[nTri].m_triangle[0];
		aOut[1] = m_triangleAdjecent[nTri].m_triangle[1];
		aOut[2] = m_triangleAdjecent[nTri].m_triangle[2];
	}
	
	RED_INLINE const Vector3& GetVertex(VertexIndex vert)					{ return m_vertexes[vert]; }
	RED_INLINE VertexIndex GetVertexesCount() const						{ return m_vertexCount; }
	RED_INLINE const Vector3& GetTriangleNormal(TriangleIndex nTri) const	{ return m_triangleNormals[nTri]; }
	RED_INLINE const Box& GetBoundingBox() const							{ return m_bbox; }
	RED_INLINE TriangleIndex GetPhantomEdgesCount() const					{ return m_phantomEdgesCount; }

	////////////////////////////////////////////////////////////////////////
	// Navigation mesh queries
	Bool ComputeHeight(const Vector3& pos, Float& outHeight);
	Bool ComputeHeight(const Vector2& pos, Float minZ, Float maxZ, Float& outHeight);
	Float ComputeHeight(TriangleIndex triIndex, const Vector2& pos);
	Bool ComputeAverageHeight(const Box& bbox, Float& zAverage, Float& zMin, Float& zMax);
	TriangleIndex GetTriangle(const Vector3& pos) const;
	TriangleIndex GetTriangleBelow(const Vector3& pos) const;

	template < class TQuery >
	RED_INLINE Bool SpatialQuery( TQuery& query, const PathLib::CNavmeshAreaDescription* owner = NULL ) const
	{
		CQueryDataGuard guard( this );
		return SpatialQuery( query, owner, guard.m_data );
	}

	template < class TQuery >
	Bool SpatialQuery( TQuery& query, const PathLib::CNavmeshAreaDescription* owner, CNavmeshQueryData& data ) const;

	Bool TestLocation(const Vector3& v1s, Float radius);														// check if given position is walkable (on navmesh)
	Bool TestLine(const Vector3& pos1, const Vector3& pos2, Uint32 flags );																	// check if given line segment is walkable
	Bool TestLine(const Vector3& pos1, const Vector3& pos2, Float radius, Uint32 flags );													// check if given line is walkable
	//Float GetClosestFloorInRange(const Vector3& pos, Float fRadius, Vector3& pointOut);
	TriangleIndex GetTriangleVisibleFrom( const Vector2& posAt, const Vector3& pointFrom);													// Function to find triangle at given 2D location at given level
	Bool ComputeHeightFrom(const Vector2& pos, const Vector3& posFrom, Float& outHeight);													// simplified shortcut to above function

	//Float GetClosestFloorInRange(CNavmeshQueryData& data, const Vector3& pos, Float fRadius, Vector3& pointOut);
	TriangleIndex GetTriangleVisibleFrom(CNavmeshQueryData& data, const Vector2& posAt, const Vector3& pointFrom);							// Function to find triangle at given 2D location at given level
	Bool ComputeHeightFrom(CNavmeshQueryData& data, const Vector2& pos, const Vector3& posFrom, Float& outHeight);							// simplified shortcut to above function

	////////////////////////////////////////////////////////////////////////
	// Initialization
	void				InitializeMesh( const TDynArray< Vector3 >& vertexes, const TDynArray< Uint16 >& triangleVertexes );
	Bool				FixOverlappingTriangles();
	void				SimplifyUsingRootPoints( const TDynArray< Vector3 >& rootPoints );
	void				Clear();
	void				CopyFrom( CNavmesh* navmesh );
	TriangleIndex		GetClosestTriangle( const Box& bbox );

	// Errors reporting
#ifndef NO_NAVMESH_GENERATION
	void				NoticeProblems( const TDynArray< SNavmeshProblem >& problems )	{ m_problems.PushBack( problems ); }
	void				NoticeProblem( SNavmeshProblem&& problem )						{ m_problems.PushBack( Move( problem ) ); }
	Bool				HadProblems() const												{ return !m_problems.Empty(); }
	const TDynArray< SNavmeshProblem >& GetProblems() const								{ return m_problems; }
#endif

	void				ComputeConvexBoundings( Box& outBBox, TDynArray< Vector2 >& outBoundings ) const;

	void				CollectPhantomEdges( TDynArray< Vector3 >& outEdges );			// generation sub-procedures to collect phantom edges coordinates for future re-addition
	void				RemarkPhantomEdges( const TDynArray< Vector3 >& outEdges );		// generation sub-procedure that re-adds phantom edges from previous navmesh version
	void				MarkPhantomEdge( TriangleIndex tri, Uint32 n );					// set given triangle border as phantom edge
	void				ClearPhantomEdge( TriangleIndex tri, Uint32 n );				// clear given triangle border from phantom edge

	CNavmeshQueryData*	GetQueryData() const;
	void				ReturnBackQueryData( CNavmeshQueryData* data ) const;

	void				WriteToBuffer( CSimpleBufferWriter& writer ) const;
	Bool				ReadFromBuffer( CSimpleBufferReader& reader );
	void				OnPostLoad( CAreaDescription* area );
	Bool				IsValid();

	// resource interface
	Bool				Save( const String& depotPath ) const;

	TriangleIndex Debug_GetTriangleIntersectingRay( const Vector3& rayOrigin, const Vector3& rayDir, Vector3& collisionPoint );				// This function might be used in offline editing tools. Its veeery costly!
	static RED_INLINE Bool IsEdge( TriangleIndex tri )						{ return (tri & MASK_EDGE) != 0; }
	static RED_INLINE Bool IsPhantomEdge( TriangleIndex tri )					{ return (tri & MASK_EDGE) && tri != INVALID_INDEX; }
	static RED_INLINE TriangleIndex PhantomEdgeNeighbourIndex( TriangleIndex tri ) { ASSERT( IsPhantomEdge( tri ) ); return tri & (~MASK_EDGE); }

	//static CNavmesh*	GetEmptyMesh();
	//Bool				IsEmptyMesh() const										{ return m_isEmpty; }
	//void				CopyFromEmpty();

#ifdef DEBUG_NAVMESH_COLORS
	TDynArray< Uint32 >			m_triangleColours;
#endif

};


class CNavmeshRes : public CNavmesh, public CAreaRes, public CVersionTracking
{
	friend class CNavmeshFactory;
public:
	CNavmeshRes()															{}
	~CNavmeshRes()															{}

	void				CopyFrom( CNavmesh* navmesh );

	using				CNavmesh::Save;
	Bool				VHasChanged() const override;
	Bool				VSave( const String& depotPath ) const override;
	Bool				VLoad( const String& depotPath, CAreaDescription* area ) override; ///
	void				VOnPostLoad( CAreaDescription* area ) override;
	const Char*			VGetFileExtension() const override;
	ENavResType			VGetResType() const override;


	static const Char*			GetFileExtension()							{ return TXT("navmesh"); }
	static ENavResType			GetResType()								{ return NavRes_Navmesh; }
};


///////////////////////////////////////////////////////////////////////////////
// Data that are required to speed up navmesh queries. They should be
// instantiated per-navmesh per-thread.
///////////////////////////////////////////////////////////////////////////////
class CNavmeshQueryData
{
	friend class CNavmesh;
public:
	typedef TDynArray< CNavmesh::TriangleIndex, MC_PathLib > tStackToVisit;
	typedef Uint16 tTestMarker;
	tStackToVisit					m_activeTriangles;
	tTestMarker						m_currentTestMarker;
	tTestMarker*					m_testTriangles;
	//Red::Threads::CAtomic< Int32 >	m_users;

public:
	CNavmeshQueryData( const CNavmesh* navmesh );
	~CNavmeshQueryData();
};


};			// namespace PathLib
