/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "cookerSpeedTreeSupport.h"

#include "../../common/engine/baseTree.h"

#include <Core/Types.h>
#include <Core/Core.h>

#include "../../common/renderer/speedTreeLinkage.h"

// Let's set our allocator just in case.
class CSpeedTreeAllocator : public SpeedTree::CAllocator
{
public:

	void* Alloc( size_t BlockSize, SpeedTree::EAllocationType eType )
	{
		return RED_MEMORY_ALLOCATE( MemoryPool_SpeedTree, MC_SpeedTree, BlockSize );
	}

	void Free( void* pBlock )
	{
		RED_MEMORY_FREE( MemoryPool_SpeedTree, MC_SpeedTree, pBlock );
	}
};

static CSpeedTreeAllocator GSpeedTreeAllocator;
static SpeedTree::CAllocatorInterface GSpeedTreeAllocatorInterface(&GSpeedTreeAllocator);

namespace CookerSpeedTreeSupport
{

	static SpeedTree::CTree* ParseSpeedTreeObject( const CSRTBaseTree* baseTree )
	{
		SpeedTree::CTree* sptTree = new SpeedTree::CTree();

		if ( sptTree->LoadTree( (SpeedTree::st_byte*)baseTree->GetSRTData(), baseTree->GetSRTDataSize(), true ) )
		{
			return sptTree;
		}
		else
		{
			// failed to load
			delete sptTree;
			return nullptr;
		}
	}


	void GetTreeCollisionShapes( const CSRTBaseTree* baseTree, Bool& grass, TDynArray< Sphere >& outShapes )
	{
		SpeedTree::CTree* sptTree = ParseSpeedTreeObject( baseTree );

		grass = sptTree->IsCompiledAsGrass();

		SpeedTree::st_int32 nNumObjects = 0;
		const SpeedTree::SCollisionObject* pObjects = sptTree->GetCollisionObjects( nNumObjects );
		outShapes.Resize( nNumObjects * 2 );
		for( Int32 i = 0; i != nNumObjects; ++i )
		{
			outShapes[ i*2+0 ] = Sphere( Vector( pObjects[ i ].m_vCenter1.x, pObjects[ i ].m_vCenter1.y, pObjects[ i ].m_vCenter1.z ), pObjects[ i ].m_fRadius );
			outShapes[ i*2+1 ] = Sphere( Vector( pObjects[ i ].m_vCenter2.x, pObjects[ i ].m_vCenter2.y, pObjects[ i ].m_vCenter2.z ), pObjects[ i ].m_fRadius );
		}

		delete sptTree;
	}

	Bool GetTreeInfo( const CSRTBaseTree* tree, SRTInfo& outInfo )
	{
		SpeedTree::CTree* sptTree = ParseSpeedTreeObject( tree );
		if ( !sptTree )
		{
			// Can't parse
			return false;
		}

		SpeedTree::Vec3 dext = sptTree->GetExtents( ).GetDiagonal();
		
		outInfo.m_isGrass = sptTree->IsCompiledAsGrass();
		outInfo.m_diagonalExtents.Set3( dext.x, dext.y, dext.z );

		delete sptTree;

		return true;
	}

};