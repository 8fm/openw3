/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "collisionMemToolModel.h"
#include "collisionObjSizeCalc.h"
#include "../../common/engine/staticMeshComponent.h"

using namespace CollisionMem;

namespace
{
	//** *******************************
	//
	struct VecCmp
	{
		//! Sorter
		RED_INLINE Bool operator()( const SingleScaleData & first, const SingleScaleData & second ) const
		{
			const Vector & v1 = first.scale;
			const Vector & v2 = second.scale;

			return v1.Mag3() < v2.Mag3();
		}
	};

	//** *******************************
	//
	struct UniqueMeshCmp
	{
	public:

		//! Sorter
		RED_INLINE Bool operator()( const UniqueMeshData & first, const UniqueMeshData & second ) const
		{
			return first.collisionMemSize < second.collisionMemSize;
		}		
	};

	//** *******************************
	//
	Float GVectorGroupingEpsilon = 2.f;
}


//** *******************************
//
void	CCollisionMemToolModel::ReadWorldNodeData		( CStaticMeshComponent * pStaticMeshComponent, TMeshMap & meshMap )
{
	if( pStaticMeshComponent )
	{
		CMesh * pMesh = pStaticMeshComponent->GetMeshNow();

		if ( CollisionObjMemCalc::CollisionSizeOf( pMesh ) > 0 )
		{
			TStaticMeshComponentArray* meshArray = meshMap.FindPtr( pMesh );
			if ( meshArray )
			{
				meshArray->PushBack( pStaticMeshComponent );
			}
		}
	}
}

//** *******************************
//
void	CCollisionMemToolModel::PrepareUniqueMeshData	( const TMeshMap & meshMap )
{
	m_UniqueMeshArray.Clear();

	for( TMeshMapConstIterator it = meshMap.Begin(); it != meshMap.End(); ++it )
	{
		UniqueMeshData umd;

		umd.pMesh = it->m_first;

		umd.collisionMemSize = CollisionObjMemCalc::CollisionSizeOf( umd.pMesh );

		ProcessScales( it->m_second, umd.arr, umd.collisionMemSize );

		m_UniqueMeshArray.PushBack( umd );
	}

	UniqueMeshCmp umComparator;
	Sort( m_UniqueMeshArray.Begin(), m_UniqueMeshArray.End(), umComparator );
}

//** *******************************
//
Uint32	CCollisionMemToolModel::GetTotalCollisionMemory						() const
{
	Uint32 totalMemory = 0;

	for( Uint32 i = 0; i < m_UniqueMeshArray.Size(); ++i )
	{
		totalMemory += m_UniqueMeshArray[ i ].collisionMemSize;
	}

	return totalMemory;
}

//** *******************************
//
const TUniqueMeshInfoArray &	CCollisionMemToolModel::GetUniqueMeshArray	() const
{
	return m_UniqueMeshArray;
}

//** *******************************
//
void	CCollisionMemToolModel::ProcessScales			( const TStaticMeshComponentArray & inArr, TSingleScaleDataArray & scalesArray, Uint32 & totalMemSize ) const
{
	// Append unique scales
	for( Uint32 i = 0; i < inArr.Size(); ++i )
	{
		CStaticMeshComponent * pSMC = inArr[ i ];

		Vector scale = GetCollsionScale( inArr[ i ] );

		Uint32 k = 0;

		// Try to find stored scale
		for( ; k < scalesArray.Size(); ++k )
		{
			if( scalesArray[ k ].scale == scale )
			{
				break;
			}
		}

		// Add new entry (if not present)
		if( k == scalesArray.Size()) 
		{
			SingleScaleData ssd;
			
			ssd.scale				= scale;
			ssd.collisionMemSize	= CollisionObjMemCalc::CollisionSizeOf( pSMC, scale );
			scalesArray.PushBack( ssd );

			totalMemSize += ssd.collisionMemSize;
		}

		// Add StaticMeshComponent to this scale's entries
		scalesArray[ k ].arr.PushBack( pSMC );
	}

	GroupScales( scalesArray );
}

//** *******************************
//
void	CCollisionMemToolModel::GroupScales				( TSingleScaleDataArray & arr ) const
{
	TSingleScaleDataArray out;

	// While there are still entries left
	while( arr.Size() > 0 )
	{
		TSingleScaleDataArray locOut;

		TSingleScaleDataArrayIterator maxIt	= arr.Begin();
		TSingleScaleDataArrayIterator it	= arr.Begin();
		++it;

		// Begin group
		Vector scale = it->scale;

		// Find the longest vector - any other criterion may be used as well
		for( ; it != arr.End(); ++it )
		{
			if( scale.Mag3() < it->scale.Mag3() )
			{
				scale	= it->scale;
				maxIt	= it;
			}
		}

		// Fill group with proper vectors (close enough to current vector representative)
		locOut.PushBack( *maxIt );
		arr.EraseFast( maxIt );

		for( TSingleScaleDataArrayIterator nit = arr.Begin(); nit != arr.End(); )
		{
			Float dist = sqrtf( ( scale - nit->scale ).Mag3() );

			if( dist < GVectorGroupingEpsilon )
			{
				locOut.PushBack( *nit );
				arr.EraseFast( nit );
			}
			else
			{
				++nit;
			}
		}

		// Sort current group
		VecCmp vComparator;
		Sort( locOut.Begin(), locOut.End(), vComparator );

		// Append to global out
		for( Uint32 i = 0; i < locOut.Size(); ++i )
		{
			out.PushBack( locOut[ i ] );
		}
	}

	arr = out;
}

//** *******************************
//
Vector	CCollisionMemToolModel::GetCollsionScale		( const CStaticMeshComponent * pSMC ) const
{
	ASSERT( pSMC );

	Matrix dummyMatrix;
	Vector scale;

	pSMC->GetLocalToWorld().ExtractScale( dummyMatrix, scale );

	return scale;
}
