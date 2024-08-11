#include "build.h"
#include "pathlibNavmeshBinTree.h"

#include "pathlibBinTree.h"
#include "pathlibNavmesh.h"



namespace PathLib
{

////////////////////////////////////////////////////////////////////////////
// CBinTree creation process
////////////////////////////////////////////////////////////////////////////
typedef Uint16 NavmeshBinTreeElementType;

typedef BinTree::TBinTreeNode< NavmeshBinTreeElementType > CNavmeshBinTreeNode;

namespace
{
	static CNavmeshBinTreeNode* RootNode( BinTree::BinTreeNodeBase* rootNode )		{ return static_cast< CNavmeshBinTreeNode* >( rootNode );  }
};

////////////////////////////////////////////////////////////////////////////
// CBinTree
////////////////////////////////////////////////////////////////////////////
CNavmeshBinTree::~CNavmeshBinTree()
{
}

void CNavmeshBinTree::CreateTree( CNavmesh* navmesh )
{
	struct TrianglesInput : Red::System::NonCopyable
	{
		typedef NavmeshBinTreeElementType ElementType;
		static_assert( sizeof( ElementType ) == sizeof( CNavmesh::TriangleIndex ), "Incompatible navmesh bin tree types!" );

		enum { MIN_NODE_ELEMENTS = 5 };

		struct InputData
		{
			ElementType								m_element;
			Bool									m_ending;
		};
		TDynArray< InputData >					m_data;
		TDynArray< InputData >::iterator		m_itBegin;
		TDynArray< InputData >::iterator		m_itEnd;
		CNavmesh*								m_navmesh;
		Box										m_inputArea;
		TDynArray< Uint8 >						m_dataBuffer;

		void SortInput( Uint32 dimm )
		{
			struct Comperator
			{
				Comperator( Uint32 dimm, CNavmesh* navmesh )
					: m_dimm( dimm ), m_navmesh( navmesh ) {}
				RED_INLINE Bool operator()( const InputData& n1, const InputData& n2 ) const
				{
					Vector3 v1[3];
					Vector3 v2[3];
					m_navmesh->GetTriangleVerts(n1.m_element,v1);
					m_navmesh->GetTriangleVerts(n2.m_element,v2);
					Float f1 = n1.m_ending ? Max( v1[0].A[m_dimm], v1[1].A[m_dimm], v1[2].A[m_dimm] ) : Min( v1[0].A[m_dimm], v1[1].A[m_dimm], v1[2].A[m_dimm] );
					Float f2 = n2.m_ending ? Max( v2[0].A[m_dimm], v2[1].A[m_dimm], v2[2].A[m_dimm] ) : Min( v2[0].A[m_dimm], v2[1].A[m_dimm], v2[2].A[m_dimm] );
					if ( f1 == f2 )
						return n1.m_ending > n2.m_ending;
					return f1 < f2;
				}
				Uint32		m_dimm;
				CNavmesh*	m_navmesh;
			};
		
			Sort( m_itBegin, m_itEnd, Comperator( dimm, m_navmesh ) );

		}

		Vector3 GetElementPosition( const InputData& d )
		{
			Vector3 v[3];
			m_navmesh->GetTriangleVerts( d.m_element, v );
			if ( d.m_ending )
			{
				return Vector3(
					Max( v[0].X, v[1].X, v[2].X ),
					Max( v[0].Y, v[1].Y, v[2].Y ),
					Max( v[0].Z, v[1].Z, v[2].Z ) );
			}
			else
			{
				return Vector3(
					Min( v[0].X, v[1].X, v[2].X ),
					Min( v[0].Y, v[1].Y, v[2].Y ),
					Min( v[0].Z, v[1].Z, v[2].Z ) );
			}
		}
	};


	TrianglesInput input;
	input.m_navmesh = navmesh;
	input.m_inputArea = navmesh->GetBoundingBox();
	input.m_data.Resize( navmesh->GetTrianglesCount() * 2 );
	for( CNavmesh::TriangleIndex i = 0; i < navmesh->GetTrianglesCount(); ++i )
	{
		input.m_data[ (Uint32(i) << 1U) + 0 ].m_element = i;
		input.m_data[ (Uint32(i) << 1U) + 0 ].m_ending = false;
		input.m_data[ (Uint32(i) << 1U) + 1 ].m_element = i;
		input.m_data[ (Uint32(i) << 1U) + 1 ].m_ending = true;

	}
	input.m_itBegin = input.m_data.Begin();
	input.m_itEnd = input.m_data.End();
	BinTree::ComputeBinTree( input );
	m_dataSize = Uint32( input.m_dataBuffer.DataSize() );
	if ( m_dataSize )
	{
		void*& dataBuffer = navmesh->m_dataBuffer;
		Uint32 pureNavmeshDataSize = navmesh->m_dataBufferSize;
		// temporary copy all the data
		void* temporaryDataBuffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_PathLib, pureNavmeshDataSize );
		Red::System::MemoryCopy( temporaryDataBuffer, dataBuffer, pureNavmeshDataSize );
		// reinitialize buffers (clearing it) - this will also compact bin tree
		navmesh->InitializeBuffers( m_dataSize );
		// copy all pure data back (without overwriting binary tree thats inside)
		void* binTreeBuffer = OffsetPtr( dataBuffer, pureNavmeshDataSize );
		Red::System::MemoryCopy( dataBuffer, temporaryDataBuffer, pureNavmeshDataSize );
		Red::System::MemoryCopy( binTreeBuffer, input.m_dataBuffer.Data(), m_dataSize );
		m_rootNode = static_cast< CNavmeshBinTreeNode* >( binTreeBuffer );
		// free temporary data buffer
		RED_MEMORY_FREE( MemoryPool_Default, MC_PathLib, temporaryDataBuffer );
	}
}

void CNavmeshBinTree::SetDataBuffer( void* data, Uint32 dataSize )
{
	m_rootNode = static_cast< CNavmeshBinTreeNode* >( data );
	m_dataSize = dataSize;
}

void CNavmeshBinTree::Clear()
{
	m_rootNode = NULL;
	m_dataSize = 0;
}

CNavmeshBinTree::TriangleIndex CNavmeshBinTree::FindTriangle( const Vector2& pos, Float zMin, Float zMax, const CNavmesh* navmesh ) const
{
	if ( !m_rootNode )
		return CNavmesh::INVALID_INDEX;

	struct Acceptor : Red::System::NonCopyable
	{
		static TriangleIndex InvalidElement() { return CNavmesh::INVALID_INDEX; }

		Acceptor( const Vector2& pos, Float zMin, Float zMax, const CNavmesh* navmesh )
			: m_pos( pos )
			, m_zMin( zMin )
			, m_zMax( zMax )
			, m_navmesh( navmesh )
		{}
		RED_INLINE Bool operator()( Uint16 triangleIndex )
		{
			Vector3 verts[3];
			m_navmesh->GetTriangleVerts(triangleIndex,verts);

			// Does triangle intersects our segment in 2d space
			if ( !MathUtils::GeometryUtils::IsPointInsideTriangle2D( verts[0].AsVector2(), verts[1].AsVector2(), verts[2].AsVector2(), m_pos ) )
				return false;

			// In most cases triangles are inside 'Z' boundings
			Bool bTriangleInsideBounding = true;
			Bool bAllHigher = true;
			Bool bAllLower = true;
			for (Int32 verIndex = 0; verIndex < 3; ++verIndex)
			{
				const Float fZ = verts[verIndex].Z;
				if (fZ < m_zMin || fZ > m_zMax)
					bTriangleInsideBounding = false;

				if (fZ > m_zMin)
					bAllLower = false;

				if (fZ < m_zMax)
					bAllHigher = false;
			}
			if (bAllHigher || bAllLower)
			{
				return false;
			}
			if (!bTriangleInsideBounding)
			{
				// If not - compute triangle height in given point
				// Calculate normal of triangle
				const Vector3& triangleNormal = m_navmesh->GetTriangleNormal( triangleIndex );
				// Calculate distance
				Float fZ = - ( Vector3( m_pos.X, m_pos.Y, 0.f ) - verts[0] ).Dot( triangleNormal );
				fZ /= triangleNormal.Z;		// == Dot( Vec3(0,0,1), vNormal ) where this first one is ray direction
				// basically thats just it
				if (fZ < m_zMin || fZ > m_zMax)
				{
					return false;
				}
			}

			return true;
		}
		const Vector2&		m_pos;
		Float				m_zMin;
		Float				m_zMax;
		const CNavmesh*		m_navmesh;
	} acceptor( pos, zMin, zMax, navmesh );
	return FindBinTreeElement( PathLib::RootNode( m_rootNode ), pos, acceptor );
}

template < class Functor >
RED_INLINE void CNavmeshBinTree::IterateTrianglesInBBox( const Vector3& bboxMin, const Vector3& bboxMax, const CNavmesh* navmesh, Functor& functor )
{
	if ( !m_rootNode )
		return;

	struct LocalFunctor : Red::System::NonCopyable
	{
		Vector3							m_bboxMin;
		Vector3							m_bboxMax;
		const CNavmesh*					m_navmesh;
		Functor&						m_functor;

		LocalFunctor( const Vector3& bboxMin, const Vector3& bboxMax, const CNavmesh* navmesh, Functor& functor )
			: m_bboxMin( bboxMin )
			, m_bboxMax( bboxMax )
			, m_navmesh( navmesh )
			, m_functor( functor ) {}

		void operator()( CNavmesh::TriangleIndex triangleIndex )
		{
			RED_MESSAGE("FIXME>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>WTF????????????????")
			//FIXME>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#ifndef RED_COMPILER_CLANG
			Vector3 verts[3];
			m_navmesh->GetTriangleVerts(triangleIndex,verts);

			Vector3 triMin(
				Min( Min( verts[0].X, verts[1].X ), verts[2].X ),
				Min( Min( verts[0].Y, verts[1].Y ), verts[2].Y ),
				Min( Min( verts[0].Z, verts[1].Z ), verts[2].Z ) );

			Vector3 triMax(
				Max( Max( verts[0].X, verts[1].X ), verts[2].X ),
				Max( Max( verts[0].Y, verts[1].Y ), verts[2].Y ),
				Max( Max( verts[0].Z, verts[1].Z ), verts[2].Z ) );

			if (
				!MathUtils::GeometryUtils::RangeOverlap1D( triMin.X, triMax.X, m_bboxMin.X, m_bboxMax.X ) ||
				!MathUtils::GeometryUtils::RangeOverlap1D( triMin.Y, triMax.Y, m_bboxMin.Y, m_bboxMax.Y ) ||
				!MathUtils::GeometryUtils::RangeOverlap1D( triMin.Z, triMax.Z, m_bboxMin.Z, m_bboxMax.Z ) )
				return;

			// TODO Detailed test

			m_functor( triangleIndex, verts );
#endif // RED_COMPILER_CLANG
		}

		
	} localFunctor( bboxMin, bboxMax, navmesh, functor );
	IterateElementsInBoundings( PathLib::RootNode( m_rootNode ), bboxMin.AsVector2(), bboxMax.AsVector2(), localFunctor );
}

Bool CNavmeshBinTree::ComputeAverageHeight(const Vector3& bboxMin, const Vector3& bboxMax,  const CNavmesh* navmesh, Float& zAverage, Float& zMin, Float& zMax)
{
	struct Functor
	{
		Functor()
			: m_zoneArea( 0.f )
			, m_averageZ( 0.f )
			, m_minZ( FLT_MAX )
			, m_maxZ( -FLT_MAX ) {}
		void operator()( CNavmesh::TriangleIndex tri, Vector3* verts )
		{
			// calculate triangle field
			Float triArea = Abs( MathUtils::GeometryUtils::TriangleArea2D( verts[ 0 ].AsVector2(), verts[ 1 ].AsVector2(), verts[ 2 ].AsVector2() ) );
			if ( triArea > NumericLimits< Float >::Epsilon() )
			{
				Float triZ = (verts[ 0 ].Z + verts[ 1 ].Z + verts[ 2 ].Z) / 3.f;
				Float triMin = Min( verts[ 0 ].Z, verts[ 1 ].Z, verts[ 2 ].Z );
				Float triMax = Max( verts[ 0 ].Z, verts[ 1 ].Z, verts[ 2 ].Z );

				m_zoneArea += triArea;

				Float ratio = triArea / m_zoneArea;

				m_averageZ = m_averageZ + (triZ - m_averageZ) * ratio;
				m_minZ = Min( m_minZ, triMin );
				m_maxZ = Max( m_maxZ, triMax );
			}

		}
		Float							m_zoneArea;
		Float							m_averageZ;
		Float							m_minZ;
		Float							m_maxZ;
	} functor;
	IterateTrianglesInBBox( bboxMin, bboxMax, navmesh, functor );
	zAverage = functor.m_averageZ;
	zMin = functor.m_minZ;
	zMax = functor.m_maxZ;
	return functor.m_zoneArea > 0.f;
}

Bool CNavmeshBinTree::ContainsTriangle(TriangleIndex nTri)
{
	if ( !m_rootNode )
	{
		return false;
	}
	return PathLib::RootNode( m_rootNode )->ContainsElement( nTri );
}

CNavmeshBinTree::TriangleIndex CNavmeshBinTree::GetClosestTriangleInside(const Box& bbox, CNavmesh* navmesh)
{
	struct Acceptor : public Red::System::NonCopyable
	{
		static TriangleIndex InvalidElement() { return CNavmesh::INVALID_INDEX; }

		Acceptor( CNavmesh* navi, const Box& bbox )
			: m_navmesh( navi )
			, m_bbox( bbox ) {}

		Float operator()( TriangleIndex triId )
		{
			Vector3 verts[3];
			m_navmesh->GetTriangleVerts( triId, verts );
			Box triBox(
				Vector(
					Min( verts[ 0 ].X, verts[ 1 ].X, verts[ 2 ].X ),
					Min( verts[ 0 ].Y, verts[ 1 ].Y, verts[ 2 ].Y ),
					Min( verts[ 0 ].Z, verts[ 1 ].Z, verts[ 2 ].Z )
				),
				Vector(
					Max( verts[ 0 ].X, verts[ 1 ].X, verts[ 2 ].X ),
					Max( verts[ 0 ].Y, verts[ 1 ].Y, verts[ 2 ].Y ),
					Max( verts[ 0 ].Z, verts[ 1 ].Z, verts[ 2 ].Z )
				)
			);
			if ( m_bbox.Touches( triBox ) )
			{
				return (triBox.CalcCenter() - m_bbox.CalcCenter()).SquareMag3();
			}
			return FLT_MAX;
		}

		CNavmesh*		m_navmesh;
		const Box&		m_bbox;
	};

	if ( !m_rootNode )
	{
		return false;
	}

	Vector2 centerPos = (bbox.Max.AsVector2() + bbox.Min.AsVector2()) * 0.5f;
	Vector2 extends = (bbox.Max.AsVector2() - bbox.Min.AsVector2()) * 0.5f;
	Float maxDist = Max( extends.X, extends.Y );
	Float maxDistSq = maxDist*maxDist;

	Acceptor acceptor( navmesh, bbox );

	return FindBinTreeClosestElement( PathLib::RootNode( m_rootNode ), centerPos, acceptor, maxDistSq );
}

};			// namespace PathLib

