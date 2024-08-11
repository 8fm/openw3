/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderProxySpeedTree.h"
#include "renderSpeedTreeResource.h"
#include "renderProxyTerrain.h"
#include "renderScene.h"

#include "../redMath/random/fastRand.h"
#include "../redSystem/numericalLimits.h"
#include "../engine/dynamicCollisionCollector.h"
#include "../engine/foliageInstance.h"
#include "../engine/renderFragment.h"
#include "../engine/renderSettings.h"
#include "../engine/grassCellMask.h"
#include "../core/configVar.h"
#include "../core/taskRunner.h"
#include "../core/taskDispatcher.h"

#ifdef USE_ANSEL
extern Bool isAnselSessionActive;
extern Bool isAnselCaptureActive;
#endif // USE_ANSEL

namespace Config
{
	TConfigVar< Bool >												cvEnableSpeedTreeUpdate(			"Rendering/SpeedTree", "UpdateEnabled",							true );

	TConfigVar< Float, Validation::FloatRange< 0, 6400, 100 > >		cvFoliageShadowDistanceScale(		"Rendering/SpeedTree", "FoliageShadowDistanceScale",			10.0f, eConsoleVarFlag_Save );
	TConfigVar< Float, Validation::FloatRange< 0, 20000, 100 > >	cvFoliageShadowFadeRangeDissolve(	"Rendering/SpeedTree", "FoliageShadowFadeRangeDissolve",		10.0f );
	TConfigVar< Float, Validation::FloatRange< 0, 20000, 100 > >	cvFoliageShadowFadeRangeErosion(	"Rendering/SpeedTree", "FoliageShadowFadeRangeErosion",			25.0f );
	TConfigVar< Float, Validation::FloatRange< 0, 100, 100 > >		cvShadowDistanceBillboardScale(		"Rendering/SpeedTree", "FoliageShadowBillboardDistanceScale",	0.5f ); // Defines min % of billboard distance from where the shadow blend will start
	
	TConfigVar< Float >												cvGrassGenerationEfficiency(		"Rendering/SpeedTree", "GrassGenerationEfficiency",				0.075f, eConsoleVarFlag_Save );
	TConfigVar< Int32 >												cvStaticRingSize(					"Rendering/SpeedTree", "StaticRingSize",						8 * 1024 * 1024, eConsoleVarFlag_Developer );
	TConfigVar< Int32 >												cvStaticPerCSRingSize(				"Rendering/SpeedTree", "StaticPerCSRingSize",					2 * 1024 * 1024,  eConsoleVarFlag_Developer );
	TConfigVar< Int32 >												cvDynamicRingSize(					"Rendering/SpeedTree", "DynamicRingSize",						64 * 1024, eConsoleVarFlag_Developer );
	TConfigVar< Int32 >												cvDynamicPerCSRingSize(				"Rendering/SpeedTree", "DynamicPerCSRingSize",					64 * 1024, eConsoleVarFlag_Developer );
	TConfigVar< Int32 >												cvGrassRingSize(					"Rendering/SpeedTree", "GrassRingSize",							4 * 1024 * 1024, eConsoleVarFlag_Save );

	extern TConfigVar< Int32, Validation::IntRange< 0, 63 > >		cvGrassLateAllocVSLimit;
	extern TConfigVar< Int32, Validation::IntRange< 0, 63 > >		cvTreesLateAllocVSLimit;
}

static Float GetSpeedTreeShadowFadeDistance( const CRenderFrameInfo *info )
{
	RED_FATAL_ASSERT( info, "Expects renderframe info" );

	const Float distErosion = Config::cvFoliageShadowFadeRangeErosion.Get();
	const Float distDissolve = Config::cvFoliageShadowFadeRangeDissolve.Get();
	const Float scenesFactor = info ? info->m_cameraLightsModifiersSetup.m_scenesSystemActiveFactor : 0.5f;
	return Lerp( scenesFactor, distDissolve, distErosion );
}

namespace Consts
{
	const Int32 defaultFoliageCellSize = 64;
}

#define PARALLEL_FOLIAGE_PROCESSING true

#ifdef USE_SPEED_TREE

// Resolution of density metrics cells
#define DENSITY_METRICS_WORLD_SIZE		16384.0f
#define DENSITY_METRICS_CELL_SIZE		8.0f		
#define DENSITY_METRICS_TEXTURE_SIZE	64

// Trees fading params
#define FADE_IN_TIME		0.2f;
#define FADE_OUT_TIME		0.25f;
#define MINIMUM_TREE_HEIGHT	2.0f;

#ifdef ENABLE_SPEEDTREE_DENSITY_VISUALISER
	#include "speedTreeDensityDebugger.h"
#include "../engine/renderFragment.h"
#endif

// Use this to enable log spamming when max cell instances is exceeded
//#define ENABLE_MAX_INSTANCES_LOGGING

#include "speedTreeLinkage.h"

class CSpeedTreeAllocator : public SpeedTree::CAllocator
{
public:

	void* Alloc( size_t BlockSize, SpeedTree::EAllocationType eType )
	{
		return RED_MEMORY_ALLOCATE_HYBRID( MemoryPool_SpeedTree, MC_SpeedTree, BlockSize );
	}

	void Free( void* pBlock )
	{
		RED_MEMORY_FREE_HYBRID( MemoryPool_SpeedTree, MC_SpeedTree, pBlock );
	}
};

static CSpeedTreeAllocator GSpeedTreeAllocator;
static SpeedTree::CAllocatorInterface GSpeedTreeAllocatorInterface(&GSpeedTreeAllocator);

#include "speedTreeDefines.h"

namespace Config
{
	TConfigVar< Float >	cvGrassInstancesCount( "Rendering", "GrassDensity", GRASS_MAX_INSTANCE_COUNT, eConsoleVarFlag_Save );
}

//dex++
Uint32 GGlobalSpeedTreeFrameIndex = 1;
//dex--

using namespace SpeedTree;

//////////////////////////////////////////////////////////////////////////
// Functors for instances-in-area tests
//////////////////////////////////////////////////////////////////////////

class InsideCircle
{
public:
	Vec3 m_center;
	Float m_radiusSqr;

	RED_INLINE Bool operator()( const Vec3& position ) const
	{
		return position.DistanceSquared( m_center ) <= m_radiusSqr;
	}
};

class InsideRectangle
{
public:
	Vec3 m_min;
	Vec3 m_max;

	RED_INLINE Bool operator()( const Vec3& position ) const
	{
		return position.x >= m_min.x && position.x <= m_max.x && position.y >= m_min.y && position.y <= m_max.y;
	}
};

//////////////////////////////////////////////////////////////////////////
// Collisions and pigment ATM
//////////////////////////////////////////////////////////////////////////
struct SGrassConstantBuffer
{
	SPigmentDataConstants		m_pigmentConstants;
	SCollisionsDataConstants	m_collisionsConstants;		
	Vector						m_terrainNormalsAreaParams;
	Vector						m_terrainNormalsParams;
	Vector						alphaScalarMulParams;
};

//////////////////////////////////////////////////////////////////////////
// Grid of the speed tree instances.
//////////////////////////////////////////////////////////////////////////

template < class CELL, class INST >
void TSpeedTreeGrid<CELL, INST>::Init( Float cellSize )
{
	m_perCellData.Clear();
	m_extents.Reset();
	m_cellSize = cellSize;
	m_numCols = 0;
	m_numRows = 0;
	m_firstCol = 0;
	m_firstRow = 0;
	m_lastCol = -1;
	m_lastRow = -1;
}

template < class CELL, class INST >
CELL* TSpeedTreeGrid<CELL, INST>::GetCellByKey( const SCellKey& key )
{
	CELL* data = NULL;
	m_perCellData.Find( SRowCol( key.m_nRow, key.m_nCol ), data );
	return data;
}

template < class CELL, class INST >
const CELL* TSpeedTreeGrid<CELL, INST>::GetCellByKey(const SpeedTree::SCellKey& key) const
{
	CELL* data = NULL;
	m_perCellData.Find( SRowCol( key.m_nRow, key.m_nCol ), data );
	return data;
}

template < class CELL, class INST >
void TSpeedTreeGrid<CELL, INST>::EraseCellByKey(const SpeedTree::SCellKey& key)
{
	auto iter = m_perCellData.Find( SRowCol( key.m_nRow, key.m_nCol ) );
	if ( iter != m_perCellData.End() )
	{
		CELL* cell = (*iter).m_second;
		RED_FATAL_ASSERT( cell != nullptr, "cell in the map is null!" );
		delete cell;
		m_perCellData.Erase( iter );
	}
}

template < class CELL, class INST >
CELL* TSpeedTreeGrid<CELL, INST>::GetOrCreateCellByKey(const SpeedTree::SCellKey& key)
{
	// This baby just creates it if necessary
	CELL*& cell = m_perCellData[ SRowCol( key.m_nRow, key.m_nCol ) ];
	if ( cell == nullptr )
	{
		cell = new CELL();
	}
	return cell;
}

template < class CELL, class INST >
Uint32 TSpeedTreeGrid<CELL, INST>::RemoveInstances( const SpeedTree::CTreeRender* baseTree, const Vector& center, Float radius, Int32 outRowCol[4] )
{
	const Vec3 stCenter( center.AsFloat() );

	const Float leftMostX = center.X - radius;
	const Float rightMostX = center.X + radius;
	const Float topMostY = center.Y + radius;
	const Float bottomMostY = center.Y - radius;

	const Vec3 minCorner( leftMostX, bottomMostY );
	const Vec3 maxCorner( rightMostX, topMostY );

	Int32 minRow, minCol, maxRow, maxCol;
	ComputeCellCoords( minCorner, m_cellSize, minRow, minCol );
	ComputeCellCoords( maxCorner, m_cellSize, maxRow, maxCol );
	OrderPair<st_int32>(minRow, maxRow);
	OrderPair<st_int32>(minCol, maxCol);

	outRowCol[0] = minRow; outRowCol[1] = maxRow;
	outRowCol[2] = minCol; outRowCol[3] = maxCol;

	const CExtents& baseExtents = baseTree->GetExtents();

	Uint32 numRemoved = 0;

	for ( Int32 i=minRow; i<=maxRow; ++i )
	{
		for ( Int32 j=minCol; j<=maxCol; ++j )
		{
			// Get cell data
			CELL* perCellData = GetCellByKey( SCellKey( i, j ) );

			if ( perCellData )
			{
				// Remove in-radius instances from this cell
				InsideCircle functor;
				functor.m_center.Set( center.AsFloat() );
				functor.m_radiusSqr = radius*radius;
				numRemoved += perCellData->RemoveInstances( baseTree, functor );

				// Watch out! Recompilation means that old instances pointers will be invalid!
				if ( perCellData->IsEmpty() )
				{
					EraseCellByKey( SCellKey( i, j ) );
				}
				else
				{
					perCellData->Recompile( &baseExtents );
				}
			}
		}
	}

	return numRemoved;
}

template < class CELL, class INST >
Uint32 TSpeedTreeGrid<CELL, INST>::RemoveInstances( const SpeedTree::CTreeRender* baseTree, const Vector& center, Float radius )
{
	const Vec3 stCenter( center.AsFloat() );

	const Float leftMostX = center.X - radius;
	const Float rightMostX = center.X + radius;
	const Float topMostY = center.Y + radius;
	const Float bottomMostY = center.Y - radius;

	const Vec3 minCorner( leftMostX, bottomMostY );
	const Vec3 maxCorner( rightMostX, topMostY );

	Int32 minRow, minCol, maxRow, maxCol;
	ComputeCellCoords( minCorner, m_cellSize, minRow, minCol );
	ComputeCellCoords( maxCorner, m_cellSize, maxRow, maxCol );
	OrderPair<st_int32>(minRow, maxRow);
	OrderPair<st_int32>(minCol, maxCol);

	const CExtents& baseExtents = baseTree->GetExtents();

	Uint32 numRemoved = 0;

	for ( Int32 i=minRow; i<=maxRow; ++i )
	{
		for ( Int32 j=minCol; j<=maxCol; ++j )
		{
			// Get cell data
			CELL* perCellData = GetCellByKey( SCellKey( i, j ) );

			if ( perCellData )
			{
				// Remove in-radius instances from this cell
				InsideCircle functor;
				functor.m_center.Set( center.AsFloat() );
				functor.m_radiusSqr = radius*radius;
				numRemoved += perCellData->RemoveInstances( baseTree, functor );

				// Watch out! Recompilation means that old instances pointers will be invalid!
				if ( perCellData->IsEmpty() )
				{
					EraseCellByKey( SCellKey( i, j ) );
				}
				else
				{
					perCellData->Recompile( &baseExtents );
				}
			}
		}
	}

	return numRemoved;
}

template < class CELL, class INST >
Uint32 TSpeedTreeGrid<CELL, INST>::RemoveInstances(  const SpeedTree::CTreeRender* baseTree, const Box& rect, Int32 outRowCol[4] )
{
	const Float leftMostX = rect.Min.X;
	const Float rightMostX = rect.Max.X;
	const Float topMostY = rect.Max.Y;
	const Float bottomMostY = rect.Min.Y;

	const Vec3 minCorner( leftMostX, bottomMostY );
	const Vec3 maxCorner( rightMostX, topMostY );

	Int32 minRow, minCol, maxRow, maxCol;
	ComputeCellCoords( minCorner, m_cellSize, minRow, minCol );
	ComputeCellCoords( maxCorner, m_cellSize, maxRow, maxCol );
	OrderPair<st_int32>(minRow, maxRow);
	OrderPair<st_int32>(minCol, maxCol);

	outRowCol[0] = minRow; outRowCol[1] = maxRow;
	outRowCol[2] = minCol; outRowCol[3] = maxCol;

	const CExtents& baseExtents = baseTree->GetExtents();

	Uint32 numRemoved = 0;

	for ( Int32 i=minRow; i<=maxRow; ++i )
	{
		for ( Int32 j=minCol; j<=maxCol; ++j )
		{
			// Get cell data
			CELL* perCellData = GetCellByKey( SCellKey( i, j ) );

			if ( perCellData )
			{
				// Remove rectangular are of instances from this cell
				InsideRectangle functor;
				functor.m_min.Set( rect.Min.AsFloat() );
				functor.m_max.Set( rect.Max.AsFloat() );
				numRemoved += perCellData->RemoveInstances( baseTree, functor );

				// Watch out! Recompilation means that old instances pointers will be invalid!
				if ( perCellData->IsEmpty() )
				{
					EraseCellByKey( SCellKey( i, j ) );
				}
				else
				{
					perCellData->Recompile( &baseExtents );
				}
			}
		}
	}

	return numRemoved;
}

template < class CELL, class INST >
Uint32 TSpeedTreeGrid<CELL, INST>::RemoveInstances(  const SpeedTree::CTreeRender* baseTree, const Box& rect )
{
	const Float leftMostX = rect.Min.X;
	const Float rightMostX = rect.Max.X;
	const Float topMostY = rect.Max.Y;
	const Float bottomMostY = rect.Min.Y;

	const Vec3 minCorner( leftMostX, bottomMostY );
	const Vec3 maxCorner( rightMostX, topMostY );

	Int32 minRow, minCol, maxRow, maxCol;
	ComputeCellCoords( minCorner, m_cellSize, minRow, minCol );
	ComputeCellCoords( maxCorner, m_cellSize, maxRow, maxCol );
	OrderPair<st_int32>(minRow, maxRow);
	OrderPair<st_int32>(minCol, maxCol);

	const CExtents& baseExtents = baseTree->GetExtents();

	Uint32 numRemoved = 0;

	for ( Int32 i=minRow; i<=maxRow; ++i )
	{
		for ( Int32 j=minCol; j<=maxCol; ++j )
		{
			// Get cell data
			CELL* perCellData = GetCellByKey( SCellKey( i, j ) );

			if ( perCellData )
			{
				// Remove rectangular are of instances from this cell
				InsideRectangle functor;
				functor.m_min.Set( rect.Min.AsFloat() );
				functor.m_max.Set( rect.Max.AsFloat() );
				numRemoved += perCellData->RemoveInstances( baseTree, functor );

				// Watch out! Recompilation means that old instances pointers will be invalid!
				if ( perCellData->IsEmpty() )
				{
					EraseCellByKey( SCellKey( i, j ) );
				}
				else
				{
					perCellData->Recompile( &baseExtents );
				}
			}
		}
	}

	return numRemoved;
}

template < class CELL, class INST >
void TSpeedTreeGrid<CELL, INST>::AddInstances( const TDynArray<INST, MC_SpeedTreeContainer>& instances, const SpeedTree::CExtents& sumExtents, const SpeedTree::CExtents& baseExtents, TDynArray< SCellKey >& updatedCells )
{
	Int32 row, col;
	TDynArray< CELL* >	cellsTouched;
	cellsTouched.Reserve( 8 );

	// Compute extents of the new instances, to see if we have enough cell data allocated
	for ( Uint32 i=0; i<instances.Size(); ++i ) 
	{
		const INST& instance = instances[i];

		// Compute cell coords for this instance
		ComputeCellCoords( instance.GetPos(), m_cellSize, row, col );
		SCellKey key = SCellKey( row, col );
		CELL* cellData = GetOrCreateCellByKey( key );
		ASSERT( cellData );

		if ( cellData )
		{
			// Add instance to the cell
			cellData->AddInstance( instance );

			if( cellsTouched.PushBackUnique( cellData ) )
			{
				updatedCells.PushBack(key);
			}
		}
	}

	for ( Uint32 c=0; c<cellsTouched.Size(); ++c )
	{
		cellsTouched[c]->Recompile( &baseExtents );
	}
}

template < class CELL, class INST >
void TSpeedTreeGrid<CELL, INST>::AddInstances( const TDynArray<INST, MC_SpeedTreeContainer>& instances, const SpeedTree::CExtents& sumExtents, const SpeedTree::CExtents& baseExtents )
{
	Int32 row, col;
	TDynArray< CELL* >	cellsTouched;
	cellsTouched.Reserve( 8 );

	// Compute extents of the new instances, to see if we have enough cell data allocated
	for ( Uint32 i=0; i<instances.Size(); ++i ) 
	{
		const INST& instance = instances[i];

		// Compute cell coords for this instance
		ComputeCellCoords( instance.GetPos(), m_cellSize, row, col );
		SCellKey key = SCellKey( row, col );
		CELL* cellData = GetOrCreateCellByKey( key );
		ASSERT( cellData );

		if ( cellData )
		{
			// Add instance to the cell
			cellData->AddInstance( instance );

			cellsTouched.PushBackUnique( cellData );
		}
	}

	for ( Uint32 c=0; c<cellsTouched.Size(); ++c )
	{
		cellsTouched[c]->Recompile( &baseExtents );
	}
}

//////////////////////////////////////////////////////////////////////////
// Per cell data, tree/grass specific
//////////////////////////////////////////////////////////////////////////

void CGrassLayer::SPerCellData::AddInstance( const SpeedTree::SGrassInstance& grassInstance )
{
	m_outGrassInstances.PushBack( grassInstance );
}

void CGrassLayer::SPerCellData::AddInstances( const GrassInstances& instances, const SpeedTree::CExtents& extents )
{
	m_outGrassInstances.Reserve( m_outGrassInstances.Size() + instances.Size() );
	for( auto iter = instances.Begin(), end = instances.End(); iter != end; ++iter )
	{
		m_outGrassInstances.PushBack( *iter );
	}
}

template< typename TESTFUNC >
Uint32 CGrassLayer::SPerCellData::RemoveInstances( const CTreeRender* baseTree, const TESTFUNC& testFunctor )
{
	ASSERT( baseTree );

	Uint32 numRemoved = 0;
	Int32 numInstances = (Int32)m_outGrassInstances.Size();

	for ( Int32 i=numInstances-1; i>=0; --i )
	{
		if ( testFunctor( m_outGrassInstances[i].GetPos() ) )
		{
			m_outGrassInstances.Erase( m_outGrassInstances.Begin() + i );
			++numRemoved;
		}
	}

	return numRemoved;
}

void CGrassLayer::SPerCellData::Recompile( const SpeedTree::CExtents* baseExtents )
{
	// reset extents
	m_extents.Reset();

	const Vec3 up( 0.0f, 0.0f, 1.0f );
	if ( !m_outGrassInstances.Empty() )
	{
		for ( Uint32 i=0; i<m_outGrassInstances.Size(); ++i )
		{
			// Update extents
			SGrassInstance& instance =  m_outGrassInstances[i];
			CExtents instExtents = *baseExtents;
			instExtents.Scale( instance.m_fScalar );
			//instExtents.Orient( up, instance.m_vRight ); <- this is too expensive and we don't pitch instances so should be fine
			instExtents.Translate( instance.m_vPos );
			m_extents.ExpandAround( instExtents );
		}
	}

	m_outGrassInstances.Shrink();
}

//////////////////////////////////////////////////////////////////////////

void CRenderProxy_SpeedTree::DisableVisualisation()
{
#ifdef ENABLE_SPEEDTREE_DENSITY_VISUALISER
	m_densityDebugger->SetMode( CSpeedTreeDensityDebugger::EMODE_None );
#endif
}

void CRenderProxy_SpeedTree::EnableGrassInstanceVisualisation()
{
#ifdef ENABLE_SPEEDTREE_DENSITY_VISUALISER
	m_densityDebugger->SetMode( CSpeedTreeDensityDebugger::EMODE_GrassInstances );
#endif
}

void CRenderProxy_SpeedTree::EnableGrassLayerVisualisation()
{
#ifdef ENABLE_SPEEDTREE_DENSITY_VISUALISER
	m_densityDebugger->SetMode( CSpeedTreeDensityDebugger::EMODE_GrassLayers );
#endif
}

void CRenderProxy_SpeedTree::EnableTreeInstanceVisualisation()
{
#ifdef ENABLE_SPEEDTREE_DENSITY_VISUALISER
	m_densityDebugger->SetMode( CSpeedTreeDensityDebugger::EMODE_TreeInstances );
#endif
}

void CRenderProxy_SpeedTree::SetGrassDensityBudget( Float instancesPerSqMeter )
{
	RED_UNUSED( instancesPerSqMeter );

#ifdef ENABLE_SPEEDTREE_DENSITY_VISUALISER
	m_densityDebugger->SetGrassDensityBudget( instancesPerSqMeter );
#endif
}

void CRenderProxy_SpeedTree::SetTreeDensityBudget( Float instancesPerSqMeter )
{
	RED_UNUSED( instancesPerSqMeter );

#ifdef ENABLE_SPEEDTREE_DENSITY_VISUALISER
	m_densityDebugger->SetTreeDensityBudget( instancesPerSqMeter );
#endif
}

void CRenderProxy_SpeedTree::SetGrassLayerDensityBudget( Float instancesPerSqMeter )
{
	RED_UNUSED( instancesPerSqMeter );

#ifdef ENABLE_SPEEDTREE_DENSITY_VISUALISER
	m_densityDebugger->SetGrassLayerDensityBudget( instancesPerSqMeter );
#endif
}

void CRenderProxy_SpeedTree::SPerCellData::AddInstance( const SpeedTree::CTreeInstance& treeInstance )
{
	CTree* tree = const_cast< CTree* >( treeInstance.InstanceOf() );
	ASSERT( tree );
	Int32 treeSlot = -1;

	// Try base trees array first. If tree is there, we expand the proper instances array
	for ( Int32 i=0; i<(Int32)m_baseTrees.Size(); ++i )
	{
		if ( m_baseTrees[i] == tree )
		{
			// Found. Remember iterator for use in the instances table.
			treeSlot = i;
			break;
		}
	}

	if ( treeSlot == -1 )
	{
		// First use, append tree
		m_baseTrees.PushBack( tree );
		m_instancesPerTree.Grow();
		treeSlot = static_cast< Int32 >( m_baseTrees.Size() ) - 1;
	}

	ASSERT( treeSlot >= 0 && treeSlot < (Int32)m_baseTrees.Size() );

	InstanceContainer& instancesPerGroup = m_instancesPerTree[ treeSlot ];
	instancesPerGroup.PushBack( treeInstance );
}

void CRenderProxy_SpeedTree::SPerCellData::AddInstances( const InstanceContainer & instances, const SpeedTree::CExtents& extents )
{
	m_extents.ExpandAround( extents );

	CTree* tree = const_cast< CTree* >( instances.Back().InstanceOf() );
	ASSERT( tree );
	Int32 treeSlot = -1;

	// Try base trees array first. If tree is there, we expand the proper instances array
	for ( Int32 i=0; i<(Int32)m_baseTrees.Size(); ++i )
	{
		if ( m_baseTrees[i] == tree )
		{
			// Found. Remember iterator for use in the instances table.
			treeSlot = i;
			break;
		}
	}

	if ( treeSlot == -1 )
	{
		// First use, append tree
		m_baseTrees.PushBack( tree );
		m_instancesPerTree.Grow();
		treeSlot = static_cast< Int32 >( m_baseTrees.Size() ) - 1;
	}

	ASSERT( treeSlot >= 0 && treeSlot < (Int32)m_baseTrees.Size() );
	InstanceContainer& instancesPerGroup = m_instancesPerTree[ treeSlot ];
	instancesPerGroup.PushBack( instances );

	// Reset min/max fade values, so that we can make sure the new instance will fade properly.
	m_minFade = 0.0f;
	m_maxFade = 1.0f;
}

template< typename TESTFUNC >
Uint32 CRenderProxy_SpeedTree::SPerCellData::RemoveInstances( const CTreeRender* baseTree, const TESTFUNC& testFunctor )
{
	// Find instances slot
	Int32 treeSlot = -1;
	for ( Int32 i=0; i<(Int32)m_baseTrees.Size(); ++i )
	{
		if ( m_baseTrees[i] == baseTree )
		{
			// Found. Remember iterator for use in the instances table.
			treeSlot = i;
			break;
		}
	}

	if ( treeSlot == -1 )
	{
		// No instances of this tree, in the cell
		return 0;
	}

	InstanceContainer& instances = m_instancesPerTree[treeSlot];
	InstanceContainer instancesLeftAlive; // instances that survive removal

	for ( Uint32 i=0; i<instances.Size(); ++i ) 
	{
		if ( !testFunctor( instances[i].GetPos() ) )
		{
			// This instance survives
			CTreeInstance instance = instances[i];
			instancesLeftAlive.PushBack( instance );
		}
	}

	// Replace instances array with the new, shrinked (or not :)) instances array
	Uint32 numRemoved = instances.Size() - instancesLeftAlive.Size();
	instances = instancesLeftAlive;

	if ( instances.Empty() )
	{
		m_instancesPerTree.RemoveAt( treeSlot );
		m_baseTrees.Erase( m_baseTrees.Begin() + treeSlot );
	}

	return numRemoved;
}

void CRenderProxy_SpeedTree::SPerCellData::Recompile( const SpeedTree::CExtents* baseExtents )
{
	RED_UNUSED(baseExtents);

	m_instancesOut.ClearFast();
	for ( Int32 t = 0, baseTreeEnd = (Int32)m_baseTrees.Size(); t < baseTreeEnd; ++t )
	{
		InstanceContainer& instances = m_instancesPerTree[t];
		m_instancesOut.Reserve( m_instancesOut.Size() + instances.Size() );

		for ( Uint32 i=0, end = instances.Size(); i< end; ++i )
		{
			const CTreeInstance* instance = &(instances[i]);
			m_instancesOut.PushBack( instance );
		}
	}

	m_baseTrees.Shrink();
	m_instancesOut.Shrink();
}

void CRenderProxy_SpeedTree::SPerCellData::GetUsedBaseTrees( TreeRenderContainer& baseTrees )
{
	for ( Uint32 b=0; b<m_baseTrees.Size(); ++b )
	{
		baseTrees.PushBackUnique( static_cast< SpeedTree::CTreeRender* >( m_baseTrees[b] ) );
	}
}

void CRenderProxy_SpeedTree::SPerCellData::GetUsedBaseTrees( ConstTreeRenderContainer& baseTrees ) const
{
	for ( Uint32 b=0; b<m_baseTrees.Size(); ++b )
	{
		baseTrees.PushBackUnique( (const CTreeRender*)m_baseTrees[b] );
	}
}

void CRenderProxy_SpeedTree::SPerCellData::GetUsedBaseTrees( ConstTreeContainer& baseTrees ) const
{
	for ( Uint32 b=0; b<m_baseTrees.Size(); ++b )
	{
		baseTrees.PushBackUnique( m_baseTrees[b] );
	}
}

void CRenderProxy_SpeedTree::SPerCellData::GetUsedBaseTrees( TreeContainer& baseTrees )
{
	for ( Uint32 b=0; b<m_baseTrees.Size(); ++b )
	{
		baseTrees.PushBackUnique( (CTreeRender*)m_baseTrees[b] );
	}
}

//////////////////////////////////////////////////////////////////////////
// Layer of the grass (instances of single speed tree resource)
//////////////////////////////////////////////////////////////////////////

CGrassLayer::CGrassLayer()
	: m_visibleGrass( POPULATION_GRASS )
	, m_renderTree( NULL )
	, m_grassCullRadius( 0.0f )
	, m_numInstances( 0 )
	, m_numMaxInstances( (Int32)Config::cvGrassInstancesCount.Get() )
	, m_isAutopopulated( false )
	, m_grassPopulationUpdateScheduled( false )
	, m_needInstanceBufferUpdate(false)
{
}

void CGrassLayer::ReleaseGfxResources()
{
	m_visibleGrass.ReleaseGfxResources();
	if ( m_renderTree )
	{
		m_renderTree->Release();
		m_renderTree = NULL;
	}
}

CGrassLayer::~CGrassLayer()
{
	ReleaseGfxResources();
}

void CGrassLayer::AddInstances( const GrassInstances & instances, const SpeedTree::CExtents& extents )
{
	m_cellData.AddInstances( instances, extents, m_renderTree->GetRenderBaseTree()->GetExtents() );
	m_numInstances += static_cast< Uint32 >( instances.Size() );
}

void CGrassLayer::RemoveInstances( const Vector& center, Float radius )
{	
	Uint32 numRemoved = m_cellData.RemoveInstances( m_renderTree->GetRenderBaseTree(), center, radius );
	
	if( numRemoved > 0 )
	{
		// Clear instancing and visibility state of speed tree
		ScheduleGrassPopulationUpdate();
		m_numInstances -= numRemoved;
	}
}

void CGrassLayer::RemoveInstances( const Box& rect )
{
	Uint32 numRemoved = m_cellData.RemoveInstances( m_renderTree->GetRenderBaseTree(), rect );

	if( numRemoved > 0 )
	{
		// Clear instancing and visibility state of speed tree
		ScheduleGrassPopulationUpdate();
		m_numInstances -= numRemoved;
	}
}

void CGrassLayer::UpdatePopulation()
{
	m_visibleGrass.NotifyOfPopulationChange();
	m_grassPopulationUpdateScheduled = false;
}

CGrassLayer::SPerCellData* CGrassLayer::GetCellByKey( const SpeedTree::SCellKey& key )
{
	return m_cellData.GetCellByKey( key );
}

void CGrassLayer::SetBaseGrass( CRenderSpeedTreeResource* renderTree )
{
	ASSERT( renderTree );
	ASSERT( m_renderTree == NULL ); // Set it only once
	m_renderTree = renderTree;
	m_renderTree->AddRef();
	CTreeRender* baseTree = m_renderTree->GetRenderBaseTree();
	m_grassCullRadius = baseTree->GetExtents( ).ComputeRadiusFromCenter3D( );

	Vec3 diagonalExtents = baseTree->GetExtents( ).GetDiagonal();
	Float meshRadius = Max<Float>( diagonalExtents.x, diagonalExtents.y ) * ( 1.0f / MSqrt( 2.0f ) );

	// Estimate optimal cell size 
	// TODO: a lot of profiling can be done to make the cellsizes optimal for specific grass types. This should be done at some point prior to release.
	Float cellSize = MAX_GRASS_INSTANCES_ON_CELL_EDGE * meshRadius;
	cellSize = cellSize < GRASS_CELL_SIZE_MIN ? GRASS_CELL_SIZE_MIN : cellSize;
	cellSize = cellSize > GRASS_CELL_SIZE_MAX ? GRASS_CELL_SIZE_MAX : cellSize;

	m_cellData.Init( cellSize );
	m_visibleGrass.SetCellSize( cellSize );
	
	// Since we make a special build of speed tree with no default grass instance reserve size (so we can have a LOT more cells during the culling stages),
	// we manually set the grass instance reserve values to zero. This ensures that the instance buffer only gets allocated when we add instances after the fine cull
	SHeapReserves heapReserves;
	heapReserves.m_nMaxVisibleGrassCells = GRASS_MAX_VISIBLE_CELL_RESERVATION;
	heapReserves.m_nMaxPerBaseGrassInstancesInAnyCell = 0;
	m_visibleGrass.SetHeapReserves( heapReserves );
}

//////////////////////////////////////////////////////////////////////////
// Render speed tree proxy
//////////////////////////////////////////////////////////////////////////


Bool CRenderProxy_SpeedTree::m_treeFadingEnabled = false;
Float CRenderProxy_SpeedTree::m_treeFadingFadeInTime = 0.0f;
Float CRenderProxy_SpeedTree::m_treeFadingFadeOutTime = 0.0f;
Float CRenderProxy_SpeedTree::m_treeFadingMinTreeHeight = 0.0f;
Bool CRenderProxy_SpeedTree::m_UseMultiThreadedTrees = true;

CRenderProxy_SpeedTree::CRenderProxy_SpeedTree()
	: m_longestCellOverhang( 0.0f )
	, m_staticFoliageBalance( 0 )
	, m_dynamicFoliageBalance( 0 )
	, m_cachedTerrainProxy( NULL )
	, m_genericGrassOn( true )
	, m_visible( true )
	, m_densityDebugger( nullptr )
	, m_terrainInterVertexSpace( 0.0f )
	, m_grassProcessing( nullptr )
	, m_updateInstancesProcessing( nullptr )
	, m_currentFoliageDistanceScaleParam( 1.0f )
	, m_currentGrassDistanceScaleParam( 1.0f )
{	
	m_cellData[STATIC_FOLIAGE_IDX].Init(STATIC_FOREST_CELL_SIZE);
	m_cellData[DYNAMIC_FOLIAGE_IDX].Init(DYNAMIC_FOREST_CELL_SIZE);
	m_visibleTreesFromCamera[STATIC_FOLIAGE_IDX] = SpeedTree::CVisibleInstancesRender( SpeedTree::POPULATION_TREES, true, Config::cvStaticRingSize.Get() );	// true - track nearest instances for purposes of texture streaming
	m_visibleTreesFromCamera[DYNAMIC_FOLIAGE_IDX] = SpeedTree::CVisibleInstancesRender( SpeedTree::POPULATION_TREES, true, Config::cvDynamicRingSize.Get() );	// true - track nearest instances for purposes of texture streaming

	// TODO: There is no point to do this on each render proxy creation, so move it somewhere else
	CCoordSys::SetCoordSys( CCoordSys::COORD_SYS_RIGHT_HANDED_Z_UP );
	//CCore::SetClipSpaceDepthRange( 0.0f, 1.0f );

	SForestRenderInfo sRenderInfo;
	// app-level
	sRenderInfo.m_sAppState.m_bMultisampling = false;
	sRenderInfo.m_sAppState.m_bAlphaToCoverage = false;
	sRenderInfo.m_sAppState.m_bDepthPrepass = false;
	sRenderInfo.m_sAppState.m_bDeferred = true;
	// The below one doesn't really matter for us as long as it is not SHADOW_CONFIG_OFF.
	sRenderInfo.m_sAppState.m_eShadowConfig = SRenderState::SHADOW_CONFIG_2_MAPS;

	// general rendering
	sRenderInfo.m_nMaxAnisotropy = 8;
	sRenderInfo.m_bDepthOnlyPrepass = false;
	sRenderInfo.m_fNearClip = 0.5f;
	sRenderInfo.m_fFarClip = 800.0f;

	// lighting
	SSimpleMaterial sLightMaterial;
	sLightMaterial.m_vAmbient = Vec3( 1.0f, 1.0f, 1.0f );
	sLightMaterial.m_vDiffuse = Vec3( 1.0f, 1.0f, 1.0f );
	sLightMaterial.m_vSpecular = Vec3( 1.0f, 1.0f, 1.0f );
	sLightMaterial.m_vTransmission = Vec3( 1.0f, 1.0f, 1.0f );
	sRenderInfo.m_sLightMaterial = sLightMaterial;
	// fog
	//sRenderInfo.m_vFogColor = Vec3( 0.9f, 0.95f, 1.0f );
	//sRenderInfo.m_fFogStartDistance = 200.0f;
	//sRenderInfo.m_fFogEndDistance = 2000.0f;
	// sky (leave defaults)
	//sRenderInfo.m_vSkyColor = m_cConfigFile.m_sSky.m_vColor;
	//sRenderInfo.m_fSkyFogMin = m_cConfigFile.m_sSky.m_afFogRange[0];
	//sRenderInfo.m_fSkyFogMax = m_cConfigFile.m_sSky.m_afFogRange[1];
	// sun (leave defaults)
	//sRenderInfo.m_vSunColor = m_cConfigFile.m_sSky.m_vSunColor;
	//sRenderInfo.m_fSunSize = m_cConfigFile.m_sSky.m_fSunSize;
	//sRenderInfo.m_fSunSpreadExponent = m_cConfigFile.m_sSky.m_fSunSpreadExponent;
	// shadows (leave defaults)
	sRenderInfo.m_bShadowsEnabled = true;
	sRenderInfo.m_nShadowsNumMaps = 2;
	sRenderInfo.m_nShadowsResolution = 512;
	//sRenderInfo.m_afShadowMapRanges
	//sRenderInfo.m_fShadowFadePercent ;
	m_forestRender.SetRenderInfo(sRenderInfo);
	m_forestRender.SetLightDir( Vec3( 1.0f, 0.0f, -1.0f ).Normalize() );

	m_visibleTreesFromCamera[STATIC_FOLIAGE_IDX].SetCellSize( STATIC_FOREST_CELL_SIZE );
	m_visibleTreesFromCamera[DYNAMIC_FOLIAGE_IDX].SetCellSize( DYNAMIC_FOREST_CELL_SIZE );

	// Set up tree visibility heap reserves
	SpeedTree::SHeapReserves treeHeapReserves;
	treeHeapReserves.m_nMaxVisibleTreeCells = TREE_MAX_VISIBLE_CELL_RESERVATION;
	treeHeapReserves.m_nMaxTreeInstancesInAnyCell = TREE_MAX_VISIBLE_INSTANCE_RESERVATION;
	m_visibleTreesFromCamera[STATIC_FOLIAGE_IDX].SetHeapReserves( treeHeapReserves );
	m_visibleTreesFromCamera[DYNAMIC_FOLIAGE_IDX].SetHeapReserves( treeHeapReserves );
	m_cascadeThreadData.Grow(MAX_CASCADES);
	//dex++
	for ( Uint32 i=0; i<MAX_CASCADES;++i )
	{
		m_cascadeThreadData[i] = new CShadowCascade;
		m_cascadeThreadData[i]->m_cascadeIndex = i;
		m_cascadeThreadData[i]->m_foliageType = STATIC_FOLIAGE_IDX;

		m_visibleTreesFromCascades[STATIC_FOLIAGE_IDX][i] = SpeedTree::CVisibleInstancesRender( SpeedTree::POPULATION_TREES, false, Config::cvStaticPerCSRingSize.Get() );
		m_visibleTreesFromCascades[STATIC_FOLIAGE_IDX][i].SetCellSize( STATIC_FOREST_CELL_SIZE );
		m_visibleTreesFromCascades[STATIC_FOLIAGE_IDX][i].SetHeapReserves( treeHeapReserves );

		m_visibleTreesFromCascades[DYNAMIC_FOLIAGE_IDX][i] = SpeedTree::CVisibleInstancesRender( SpeedTree::POPULATION_TREES, false, Config::cvDynamicPerCSRingSize.Get() );
		m_visibleTreesFromCascades[DYNAMIC_FOLIAGE_IDX][i].SetCellSize( DYNAMIC_FOREST_CELL_SIZE );
		m_visibleTreesFromCascades[DYNAMIC_FOLIAGE_IDX][i].SetHeapReserves( treeHeapReserves );
	}
	//dex--

	m_lastUpdateTreesPresent[STATIC_FOLIAGE_IDX] = m_lastUpdateTreesPresent[DYNAMIC_FOLIAGE_IDX] = false;
	m_lastUpdateGrassPresent = false;

	m_grassConstantBuffer = new SGrassConstantBuffer;
	
	// Create additional grass constants buffer
	m_grassConstantBufferRef = GpuApi::CreateBuffer( sizeof( SGrassConstantBuffer ), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
	ASSERT( m_grassConstantBufferRef );
	GpuApi::SetBufferDebugPath( m_grassConstantBufferRef, "grass cbuffer" );
	
#ifdef ENABLE_SPEEDTREE_DENSITY_VISUALISER
	// This may be the first time we get the world rectangle; initialise the density debugger if required
	m_densityDebugger = new CSpeedTreeDensityDebugger();
	m_densityDebugger->Initialise( DENSITY_METRICS_WORLD_SIZE, DENSITY_METRICS_CELL_SIZE, DENSITY_METRICS_TEXTURE_SIZE );
#endif

	m_grassInstancesRingBuffer.SetRingBufferSize( Config::cvGrassRingSize.Get() );
}

CRenderProxy_SpeedTree::~CRenderProxy_SpeedTree()
{
#ifdef ENABLE_SPEEDTREE_DENSITY_VISUALISER
	delete m_densityDebugger;
#endif

	// Make sure no grass processing happens in parallel
	if ( m_grassProcessing )
	{
		m_grassProcessing->FinalizeGrassInstanceUpdates();
		m_grassProcessing->Release();
		m_grassProcessing = nullptr;
	}

	if( m_updateInstancesProcessing )
	{
		m_updateInstancesProcessing->FinalizeTreeInstanceUpdates();
		m_updateInstancesProcessing->Release();
		m_updateInstancesProcessing = nullptr;
	}

	if( m_parallelShadowsProcessingParams )
	{
		m_parallelShadowsProcessingParams->FinalizeProcessing();
		m_parallelShadowsProcessingParams->Release();
		m_parallelShadowsProcessingParams = nullptr;
		m_parallelShadowsProcessingElements.ClearFast();
	}

	if( m_UseMultiThreadedTrees )
	{
		if(m_TreeProcessing[STATIC_FOLIAGE_IDX])
		{
			m_TreeProcessing[STATIC_FOLIAGE_IDX]->FinalizeTreeInstanceProcessing();
			m_TreeProcessing[STATIC_FOLIAGE_IDX]->Release();
			m_TreeProcessing[STATIC_FOLIAGE_IDX] = nullptr;
		}

		if(m_TreeProcessing[DYNAMIC_FOLIAGE_IDX])
		{
			m_TreeProcessing[DYNAMIC_FOLIAGE_IDX]->FinalizeTreeInstanceProcessing();
			m_TreeProcessing[DYNAMIC_FOLIAGE_IDX]->Release();
			m_TreeProcessing[DYNAMIC_FOLIAGE_IDX] = nullptr;
		}
	}

	for(Uint32 c = 0; c < 2; ++c )
	{
		m_visibleTreesFromCamera[c].ReleaseGfxResources();
		for ( Uint32 i=0; i<MAX_CASCADES;++i )
		{
			m_visibleTreesFromCascades[c][i].ReleaseGfxResources();
		}
	}

	for ( Uint32 i=0; i<MAX_CASCADES;++i )
	{
		delete m_cascadeThreadData[i];
	}

	for ( Uint32 i=0; i<m_grassLayers.Size(); ++i )
	{
		delete m_grassLayers[i];
	}

	for( auto res : m_usedSpeedTreeResources )
	{
		res.m_first->Release();
	}
	m_usedSpeedTreeResources.Clear();

	delete m_grassConstantBuffer;
	m_grassConstantBuffer = nullptr;

	GpuApi::SafeRelease( m_grassConstantBufferRef );

	m_grassInstancesRingBuffer.ReleaseBuffer();

	SAFE_RELEASE( m_cachedTerrainProxy );
}


void CRenderProxy_SpeedTree::AttachToScene()
{
	m_forestRender.InitGfx();
	SpeedTree::CRenderState::InitFallbackTextures( );
}

void CRenderProxy_SpeedTree::DetachFromScene()
{
	m_forestRender.ReleaseGfxResources();
	SpeedTree::CRenderState::ReleaseFallbackTextures();
}

void CRenderProxy_SpeedTree::AddStaticInstances( IRenderObject* baseTree, const FoliageInstanceContainer & instancesData, const Box & box )
{
	CRenderSpeedTreeResource* baseTreeRenderResource = static_cast< CRenderSpeedTreeResource* >( baseTree );
	ASSERT( baseTreeRenderResource );
	
	if ( InitializeBaseTree( baseTreeRenderResource ) )
	{
		CTreeRender* treeRender = baseTreeRenderResource->GetRenderBaseTree();
		ASSERT( treeRender );

		if ( treeRender->IsCompiledAsGrass() )
		{
			AddGrassInstances(baseTreeRenderResource, instancesData, treeRender, box );

		}
		else
		{
			AddTreeStaticInstances(baseTreeRenderResource, instancesData, treeRender, box );
		}

		treeRender->DeleteGeometry( );
	}
}

void CRenderProxy_SpeedTree::QueueAddDynamicInstances(RenderObjectHandle baseTreeHandle, const FoliageInstanceContainer & instancesData, const Box & box)
{
	FoliageTreeInstances inst;
	inst.tree = baseTreeHandle;
	inst.instances = instancesData;
	inst.box = box;
	m_dynamicsToAddQueue.PushBack( std::move( inst ) );
}

void CRenderProxy_SpeedTree::QueueRemoveDynamicInstances(RenderObjectHandle baseTreeHandle, const Vector& position, Float radius)
{
	DynamicFoliageRemoveInstance inst;
	inst.m_baseTree = baseTreeHandle;
	inst.m_position = position;
	inst.m_radius = radius;
	m_dynamicsToRemoveQueue.PushBack( std::move( inst ) );
}

void CRenderProxy_SpeedTree::ProcessQueuedInstances()
{
	for( auto iter = m_dynamicsToRemoveQueue.Begin(), end = m_dynamicsToRemoveQueue.End(); iter != end; ++iter )
	{
		RemoveDynamicInstances( iter->m_baseTree.Get(), iter->m_position, iter->m_radius );
	}

	for( auto iter = m_dynamicsToAddQueue.Begin(), end = m_dynamicsToAddQueue.End(); iter != end; ++iter )
	{
		AddDynamicInstances( iter->tree.Get(), iter->instances, iter->box );
	}

	m_dynamicsToRemoveQueue.ClearFast();
	m_dynamicsToAddQueue.ClearFast();
}

void CRenderProxy_SpeedTree::AddDynamicInstances( IRenderObject* baseTree, const FoliageInstanceContainer & instancesData, const Box & box )
{
	CRenderSpeedTreeResource* baseTreeRenderResource = static_cast< CRenderSpeedTreeResource* >( baseTree );
	ASSERT( baseTreeRenderResource );

	if ( InitializeBaseTree( baseTreeRenderResource, DYNAMIC_FOLIAGE_IDX ) )
	{
		CTreeRender* treeRender = baseTreeRenderResource->GetRenderBaseTree();
		ASSERT( treeRender );

		AddTreeDynamicInstances(baseTreeRenderResource, instancesData, treeRender, box );

		treeRender->DeleteGeometry( );
	}
}

void CRenderProxy_SpeedTree::RemoveStaticInstances( IRenderObject* baseTree, const Vector& position, Float radius )
{
	PC_CHECKER_SCOPE( 0.001f, TXT("FOLIAGE"), TXT("Slow RemoveInstances in radius") );

	ASSERT( baseTree );
	CRenderSpeedTreeResource* baseTreeRenderResource = static_cast< CRenderSpeedTreeResource* >( baseTree );
	ASSERT( baseTreeRenderResource );

	CTreeRender* treeRender = baseTreeRenderResource->GetRenderBaseTree();
	ASSERT( treeRender );

	if ( treeRender->IsCompiledAsGrass() )
	{
		// Find layer with instances of this base tree (base grass actually)
		CGrassLayer* grassLayer = GetGrassLayer( baseTreeRenderResource );
		if ( grassLayer )
		{
			RemoveGrassInstances( grassLayer, position, radius );
		}
	}
	else
	{
		RemoveTreeStaticInstances( baseTreeRenderResource, position, radius );
	}
}

void CRenderProxy_SpeedTree::RemoveDynamicInstances( IRenderObject* baseTree, const Vector& position, Float radius )
{
	PC_CHECKER_SCOPE( 0.001f, TXT("FOLIAGE"), TXT("Slow RemoveDynamicInstances in radius") );

	ASSERT( baseTree );
	CRenderSpeedTreeResource* baseTreeRenderResource = static_cast< CRenderSpeedTreeResource* >( baseTree );
	ASSERT( baseTreeRenderResource );

	CTreeRender* treeRender = baseTreeRenderResource->GetRenderBaseTree();
	ASSERT( treeRender );

	RemoveTreeDynamicInstances( baseTreeRenderResource, position, radius );
}

void CRenderProxy_SpeedTree::RemoveStaticInstances( IRenderObject* baseTree, const Box& rect )
{
	PC_CHECKER_SCOPE( 0.001f, TXT("FOLIAGE"), TXT("Slow RemoveInstances in rect") );

	ASSERT( baseTree );
	CRenderSpeedTreeResource* baseTreeRenderResource = static_cast< CRenderSpeedTreeResource* >( baseTree );
	ASSERT( baseTreeRenderResource );

	CTreeRender* treeRender = baseTreeRenderResource->GetRenderBaseTree();
	ASSERT( treeRender );

	if ( treeRender->IsCompiledAsGrass() )
	{
		// Find layer with instances of this base tree (base grass actually)
		CGrassLayer* grassLayer = GetGrassLayer( baseTreeRenderResource );
		if ( grassLayer )
		{
			RemoveGrassInstances( grassLayer, rect );
		}
	}
	else
	{
		RemoveTreeStaticInstances( baseTreeRenderResource, rect );
	}
}

void CRenderProxy_SpeedTree::RefreshGenericGrass()
{
	FinalizeParallelGrassProcessing();

	if( (Int32)m_grassInstancesRingBuffer.GetRingBufferSize() != Config::cvGrassRingSize.Get() )
	{
		ReinitializeRingBuffer();
	}

	for ( Uint32 l=0; l<m_grassLayers.Size(); ++l )
	{
		// Clear instancing and visibility state of speed tree
		if ( m_grassLayers[l]->IsAutopopulated() )
		{
			m_grassLayers[l]->GetVisible().NotifyOfPopulationChange();
		}
	}
}

void CRenderProxy_SpeedTree::FrameUpdate( const CRenderFrameInfo& frameInfo, CRenderSceneEx* scene )
{
	if ( !Config::cvEnableSpeedTreeUpdate.Get() )
		return;

	m_lastUpdateTreesPresent[STATIC_FOLIAGE_IDX] = m_lastUpdateTreesPresent[DYNAMIC_FOLIAGE_IDX] = false;
	m_lastUpdateGrassPresent = false;

	Bool genericGrassOn = frameInfo.IsShowFlagOn( SHOW_GenericGrass );
	Bool grassSettingsChanged = (Int32)m_grassInstancesRingBuffer.GetRingBufferSize() != Config::cvGrassRingSize.Get();
	if ( m_genericGrassOn != genericGrassOn || grassSettingsChanged )
	{
		RefreshGenericGrass();
		m_genericGrassOn = genericGrassOn;
	}

	// Cache terrain proxy
	SAFE_COPY( m_cachedTerrainProxy, scene->GetTerrain() );

	if ( m_cachedTerrainProxy && m_grassOccurrenceMasks.Size()>0 )
	{
		//m_cachedTerrainProxy->SetCustomBitmaskOverlay( m_grassOccurrenceMasks[0].GetNumCols(), m_grassOccurrenceMasks[0].GetNumRows(), m_grassOccurrenceMasks[0].m_bitmap.m_mem.TypedData() );
	}
	
	// Finish processing tree instance updates render command 
	FinalizeParallelInstanceUpdates();

	// Add / Remove dynamic instances that were queued during processing of commands
	ProcessQueuedInstances();


	// Get camera (in game it's the same camera as render camera, in editor the occlusion camera can be detached)
	const CRenderCamera& camera = frameInfo.m_occlusionCamera;

	// Collect all trees into one array
	m_cachedBaseTrees.ClearFast();
	m_cachedBaseGrass.ClearFast();

	CollectAllBaseTreesUsed( m_cachedBaseTrees );

	if(m_TreeProcessing[STATIC_FOLIAGE_IDX] != nullptr || m_TreeProcessing[DYNAMIC_FOLIAGE_IDX] != nullptr)
		FinishProcessTreeBuffers();


	// Collect all trees and grass into another array
	for ( Uint32 i=0; i<m_grassLayers.Size(); ++i )
	{
		ASSERT( m_grassLayers[i] );
		ASSERT( m_grassLayers[i]->GetBaseGrass() );
		ASSERT( m_grassLayers[i]->GetBaseGrass()->GetRenderBaseTree() );
		m_cachedBaseGrass.PushBackUnique( m_grassLayers[i]->GetBaseGrass()->GetRenderBaseTree() );
	}

	CheckUpdateFoliageRenderParams();

	// Update last update grass/trees presence
	m_lastUpdateTreesPresent[STATIC_FOLIAGE_IDX] = m_staticFoliageBalance > 0;
	m_lastUpdateTreesPresent[DYNAMIC_FOLIAGE_IDX] = m_dynamicFoliageBalance > 0;
	m_lastUpdateGrassPresent = !m_cachedBaseGrass.Empty();

	// Update wind settings
	m_forestRender.WindEnable( frameInfo.m_envParametersDayPoint.m_windParameters.m_speedTreeEnabled );		
	const bool windEnabled = m_forestRender.WindIsEnabled();
	Vector windDirection = frameInfo.m_envParametersDayPoint.m_windParameters.GetWindDirection();

	for ( Uint32 i=0; i<m_cachedBaseTrees.Size(); ++i )
	{

		CWind& wind = m_cachedBaseTrees[i]->GetWind();
		wind.SetStrength(frameInfo.m_envParametersDayPoint.m_windParameters.GetWindScale() );
		wind.SetDirection(Vec3( windDirection.X, windDirection.Y, windDirection.Z ) );
		wind.Advance( windEnabled, frameInfo.m_engineTime );
		m_cachedBaseTrees[i]->UpdateWindConstantBuffer();
	}

	for ( Uint32 i=0; i<m_cachedBaseGrass.Size(); ++i )
	{
		CWind& wind = m_cachedBaseGrass[i]->GetWind();
		wind.SetStrength( frameInfo.m_envParametersDayPoint.m_windParameters.GetWindScale() );
		wind.SetDirection( Vec3( windDirection.X, windDirection.Y, windDirection.Z ) );
		wind.Advance( windEnabled, frameInfo.m_engineTime );
		m_cachedBaseGrass[i]->UpdateWindConstantBuffer();
	}

	// Update view
	{
		Vec3 camPos( camera.GetPosition().AsFloat() );
		Mat4x4 projMatrix ( camera.GetViewToScreen().AsFloat() );
		Mat4x4 projMatrixReversedProjection ( camera.GetViewToScreenRevProjAware().AsFloat() );
		Mat4x4 viewMatrix( camera.GetWorldToView().AsFloat() );

		Float treesCullingFarPlane = (Float)(Config::cvFoliageMaxVisibilityDepth.Get() * Consts::defaultFoliageCellSize);

		m_forestView.Set( camPos, projMatrix, viewMatrix, camera.GetNearPlane(), treesCullingFarPlane );
		m_forestViewReversedProjection.Set( camPos, projMatrixReversedProjection, viewMatrix, camera.GetNearPlane(), treesCullingFarPlane );
	}
}


void CRenderProxy_SpeedTree::PreRenderUpdate( const CRenderFrameInfo& frameInfo, CRenderSceneEx* scene
#ifdef USE_UMBRA
											 , const CRenderOcclusionData* occlusionData
#endif // USE_UMBRA
											 , CRenderFrame* frame
											 )
{
	if ( !Config::cvEnableSpeedTreeUpdate.Get() )
		return;

#ifdef ENABLE_SPEEDTREE_DENSITY_VISUALISER
	m_densityDebugger->ResetAll();
#endif

	// Get camera (in game it's the same camera as render camera, in editor the occlusion camera can be detached)
	const CRenderCamera& camera = frameInfo.m_occlusionCamera;

	const Bool cullFoliageWithUmbra = occlusionData != nullptr && CUmbraScene::IsUsingOcclusionCulling() && frameInfo.IsShowFlagOn( SHOW_UmbraCullFoliage );

	if ( m_cachedBaseTrees.Size() > 0 )
	{
		// Process trees culling and loding
		ProcessTreesInstances( m_cachedBaseTrees
#ifdef USE_UMBRA
							   , cullFoliageWithUmbra, occlusionData
#endif // USE_UMBRA
							   , frame
							 );
	}

	if ( m_cachedBaseGrass.Size() > 0 )
	{
		static Int32 framesSpentWaitingForDataToBeReady = 0;
		if ( m_cachedTerrainProxy && m_cachedTerrainProxy->IsDataReadyForGrassGeneration( scene->GetLastAllocatedFrame() ) )	// wait until ready
		{
			m_cachedTerrainProxy->GetClipmap0Params( m_clipmap0worldRect, m_clipWindowResolution, m_minElevation, m_maxElevation, m_terrainInterVertexSpace );
			m_clipmap0worldRect.Min.Z = -1.0f;
			m_clipmap0worldRect.Max.Z = 1.0f;

			// Process grass culling and loding
			ProcessGrassInstances( camera
#ifdef USE_UMBRA
								   , cullFoliageWithUmbra, occlusionData 
#endif // USE_UMBRA
								);

			if ( framesSpentWaitingForDataToBeReady > 3 )
			{
				RED_LOG( RED_LOG_CHANNEL( Foliage ), TXT("Grass processing was postponed %i (>3) frames due to grass map preparation."), framesSpentWaitingForDataToBeReady );
			}
			framesSpentWaitingForDataToBeReady = 0;
		}
		else
		{
			++framesSpentWaitingForDataToBeReady;
		}
	}

#ifdef ENABLE_SPEEDTREE_DENSITY_VISUALISER
	// Update the texture
	m_densityDebugger->UpdateTexture( camera.GetPosition(), camera.GetCameraForward() );
	if ( m_cachedTerrainProxy )
	{
		const Vector worldSpaceRect = m_densityDebugger->GetWorldSpaceRect();	
		m_cachedTerrainProxy->SetGrassDensityVisualisationTexture( m_densityDebugger->GetGrassDensityTexture(), worldSpaceRect, m_densityDebugger->GetVisualisationMode() != CSpeedTreeDensityDebugger::EMODE_None );
	}
#endif

	// Clear cached containers, just in case
	m_cachedBaseTrees.ClearFast();
	m_cachedBaseGrass.ClearFast();
}

void CRenderProxy_SpeedTree::OnGrassSetupUpdated( const TDynArray< IRenderObject* >& baseObjectsInUse )
{
	// Remove any empty grass layers.
	for ( Int32 i=m_grassLayers.Size()-1; i>=0; --i )
	{
		CGrassLayer* layer = m_grassLayers[i];
		if ( layer->GetNumInstances() == 0 )
		{
			m_grassLayers.RemoveAt( i );
			delete layer;
		}
		else
		{
			layer->GetVisible().NotifyOfPopulationChange();
		}
	}

	for ( Uint32 i=0; i<baseObjectsInUse.Size(); ++i )
	{
		CRenderSpeedTreeResource* baseTreeRenderResource = static_cast< CRenderSpeedTreeResource* >( baseObjectsInUse[i] );
		ASSERT( baseTreeRenderResource );
		CTreeRender* treeRender = baseTreeRenderResource->GetRenderBaseTree();
		ASSERT( treeRender );		

		if ( treeRender->IsCompiledAsGrass() && InitializeBaseTree( baseTreeRenderResource ) )
		{
			if ( treeRender->IsCompiledAsGrass() )
			{
				CGrassLayer* grassLayer = GetOrCreateGrassLayer( baseTreeRenderResource );
				ASSERT( grassLayer );
				grassLayer->SetAutoPopulatedFlag( true );
			}
		}
	}
}

inline void UpdateSpeedTreeResourceTextureAlphaScalars( CRenderSpeedTreeResource *res, const CForestRender &forestRender )
{
	if ( res && res->GetRenderBaseTree() )
	{
		res->GetRenderBaseTree()->SetTextureAlphaScalars( forestRender.GetRenderInfo().m_fTextureAlphaScalar3d, forestRender.GetRenderInfo().m_fTextureAlphaScalarGrass, forestRender.GetRenderInfo().m_fTextureAlphaScalarBillboards );
	}
}

void CRenderProxy_SpeedTree::Render( const RenderingContext& context, const CRenderFrameInfo& frameInfo )
 {
	PC_SCOPE( RenderSpeedTreeProxy );

	FinishProcessTreeBuffers();

	if( !IsVisible() || !frameInfo.IsShowFlagOn( SHOW_Foliage ) )
	{
		FinalizeParallelGrassProcessing();
		return;
	}

	Vec3 lightDir( frameInfo.m_envParametersDayPoint.m_globalLightDirection.AsFloat() );
	m_forestRender.SetLightDir( -lightDir );

	SForestRenderInfo renderInfo = m_forestRender.GetRenderInfo(); 

	// Material parameters
	{
		const CEnvSpeedTreeParametersAtPoint &speedTreeParams = frameInfo.m_envParametersArea.m_speedTree;

		const Float specular = speedTreeParams.m_specularScale.GetScalar();
		const Float translucency = speedTreeParams.m_translucencyScale.GetScalar();

		//renderInfo.m_sLightMaterial.m_vAmbient = Vec3 ( 1.f, 1.f, 1.f );
 		renderInfo.m_sLightMaterial.m_vDiffuse = Vec3 ( speedTreeParams.m_diffuse.GetColorScaled( true ).AsFloat() );
 		renderInfo.m_sLightMaterial.m_vSpecular.Set( specular, specular, specular );
 		renderInfo.m_sLightMaterial.m_vTransmission.Set( translucency, translucency, translucency );
	}

	// Setup weak alpha scalars
	renderInfo.m_fTextureAlphaScalar3d = frameInfo.m_speedTreeParameters.m_alphaScalar3d;
	renderInfo.m_fTextureAlphaScalarGrass = frameInfo.m_speedTreeParameters.m_alphaScalarGrass;
	renderInfo.m_fTextureAlphaScalarBillboards = frameInfo.m_speedTreeParameters.m_alphaScalarBillboards;

	// Test whether we need alphaScalars update
	const Bool needsAlphaScalarsUpdate = 
		m_forestRender.GetRenderInfo().m_fTextureAlphaScalar3d != frameInfo.m_speedTreeParameters.m_alphaScalar3d ||
		m_forestRender.GetRenderInfo().m_fTextureAlphaScalarGrass != frameInfo.m_speedTreeParameters.m_alphaScalarGrass ||
		m_forestRender.GetRenderInfo().m_fTextureAlphaScalarBillboards != frameInfo.m_speedTreeParameters.m_alphaScalarBillboards;

	// Set new forestRender
	m_forestRender.SetRenderInfo( renderInfo );

	// Update alpha scalars if needed
	if ( needsAlphaScalarsUpdate )
	{
		// Update trees
		for ( auto res : m_usedSpeedTreeResources )
		{
			UpdateSpeedTreeResourceTextureAlphaScalars( res.m_first, m_forestRender );
		}

		// Update grass
		for ( Uint32 grass_i=0; grass_i<m_grassLayers.Size(); ++grass_i )
		{
			if ( m_grassLayers[grass_i] )
			{
				UpdateSpeedTreeResourceTextureAlphaScalars( m_grassLayers[grass_i]->GetBaseGrass(), m_forestRender );
			}
		}
	}

	// Render stuff
	if ( m_lastUpdateTreesPresent[STATIC_FOLIAGE_IDX] || m_lastUpdateTreesPresent[DYNAMIC_FOLIAGE_IDX] || m_lastUpdateGrassPresent )
	{
		PC_SCOPE( ForestRendering );
		{
			const Uint32 PigmentSampleIndex = 3;
			const Uint32 TerrainNormalSampleIndex = 4;
			const Uint32 GrassConstantsBufferRegIndex = 11;
			const GpuApi::eShaderType PigmentShaderType = GpuApi::VertexShader;
			const GpuApi::eShaderType TerrainNormalShaderType = GpuApi::VertexShader;
			
			// Bind pigment texture and constants
			{
				ASSERT( m_cachedTerrainProxy );
				const GpuApi::TextureRef pigmentTexture = m_cachedTerrainProxy ? m_cachedTerrainProxy->GetPigmentData().m_texture : GpuApi::TextureRef::Null();
				const GpuApi::TextureRef terrainNormalTexture = m_cachedTerrainProxy ? m_cachedTerrainProxy->GetNormalMapsArray() : GpuApi::TextureRef::Null();
								
				// update dynamic collision and pigments data buffers				
				UpdateGrassConstants( frameInfo );
				
				// Bind our own dissolve pattern for tree hiding
				{
					GpuApi::TextureRef dissolvePattern = GpuApi::GetInternalTexture( GpuApi::INTERTEX_DissolvePattern);
					GpuApi::BindTextures( 11, 1, &dissolvePattern, GpuApi::PixelShader );
				}

				GpuApi::BindTextures( PigmentSampleIndex, 1, &pigmentTexture, PigmentShaderType );
				GpuApi::SetSamplerStatePreset( PigmentSampleIndex, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, PigmentShaderType );
				GpuApi::BindTextures( TerrainNormalSampleIndex, 1, &terrainNormalTexture, TerrainNormalShaderType );
				GpuApi::SetSamplerStatePreset( TerrainNormalSampleIndex, GpuApi::SAMPSTATEPRESET_ClampLinearMip, TerrainNormalShaderType );
				GpuApi::BindConstantBuffer( GrassConstantsBufferRegIndex, m_grassConstantBufferRef, PigmentShaderType );
			}

			{
				PC_SCOPE( StartForestRendering );
				m_forestRender.StartRender( );
			}

			{
				// set the shader view parameters for the main view
				{
					PC_SCOPE( UploadingViewShaderParameters );
					
					// using viewReversedProjection only if enabled in camera to have a perfect fallback scenario with non reversedProjection view for comparison.
					const SpeedTree::CView &shadersForestView = context.GetCamera().IsReversedProjection() ? m_forestViewReversedProjection : m_forestView; 
					m_forestRender.UpdateFrameConstantBuffer( shadersForestView, frameInfo.m_width, frameInfo.m_height );
				}

				// Trees
				if ( m_lastUpdateTreesPresent[STATIC_FOLIAGE_IDX] || m_lastUpdateTreesPresent[DYNAMIC_FOLIAGE_IDX] )
				{
					// Foliage doesn't directly have light channels, but trees write out LC_OutlineOccluder, so we'll pretend that's it.
					{
						// 3d trees
						{
							GpuApi::SetVsWaveLimits( 0, Config::cvTreesLateAllocVSLimit.Get() );

							PC_SCOPE_RENDER_LVL0( Render3DTrees );
							if( m_lastUpdateTreesPresent[STATIC_FOLIAGE_IDX] )
							{
								m_forestRender.Render3dTrees( RENDER_PASS_MAIN, m_visibleTreesFromCamera[STATIC_FOLIAGE_IDX] );
							}
							if( m_lastUpdateTreesPresent[DYNAMIC_FOLIAGE_IDX] )
							{
								m_forestRender.Render3dTrees( RENDER_PASS_MAIN, m_visibleTreesFromCamera[DYNAMIC_FOLIAGE_IDX] );
							}
						}

						// Billboards
						{
							const Uint32 noiseTexRegisterIndex = 12;
							GpuApi::TextureRef texMipNoise = GpuApi::GetInternalTexture( GpuApi::INTERTEX_MipNoise );
							GPUAPI_ASSERT( texMipNoise );

							// Bind the mip noise map. It will reuse the samplers already set by SpT
							GpuApi::BindTextures( noiseTexRegisterIndex, 1, &texMipNoise, GpuApi::PixelShader );
							
							PC_SCOPE_RENDER_LVL0( RenderTreeBillboards );
							m_forestRender.RenderBillboards( RENDER_PASS_MAIN, m_visibleTreesFromCamera[STATIC_FOLIAGE_IDX] );
							if( m_dynamicFoliageBalance > 0 )
							{
								m_forestRender.RenderBillboards( RENDER_PASS_MAIN, m_visibleTreesFromCamera[DYNAMIC_FOLIAGE_IDX] );
							}

							GpuApi::BindTextures( noiseTexRegisterIndex, 1, nullptr, GpuApi::PixelShader );
						}
					}
				}

				// Grass
				if ( m_lastUpdateGrassPresent )
				{
					if ( PARALLEL_FOLIAGE_PROCESSING )
					{
						// Make sure grass update is finished
						PC_SCOPE( GrassProcessingFinalization );
						
						{
							// If worker threads didn't finish yet - help them now
							PC_CHECKER_SCOPE( 0.0001f/*0.1ms*/, TXT("FOLIAGE"), TXT("Grass workers still working!") );
							FinalizeParallelGrassProcessing();
						}
					}

					// Foliage doesn't directly have light channels, and grass doesn't write any, so we can just count it as 0.
					if ( context.CheckLightChannels( 0 ) )
					{
						GpuApi::SetVsWaveLimits( 0, Config::cvGrassLateAllocVSLimit.Get() );

						PC_SCOPE_RENDER_LVL0( RenderGrass );
						for ( Uint32 g = 0; g<m_grassLayers.Size(); ++g )
						{
							m_forestRender.RenderGrass( RENDER_PASS_MAIN, m_grassLayers[g]->GetBaseGrass()->GetRenderBaseTree(), m_grassLayers[g]->GetVisible(), m_grassInstancesRingBuffer );
						}
					}
				}
			}
			{
				PC_SCOPE( EndForestRendering );
				m_forestRender.EndRender( );
			}

			// Unbind pigment data
			GpuApi::BindTextures( PigmentSampleIndex, 1, nullptr, PigmentShaderType );
			GpuApi::BindTextures( TerrainNormalSampleIndex, 1, nullptr, TerrainNormalShaderType );
			GpuApi::BindConstantBuffer( GrassConstantsBufferRegIndex, GpuApi::BufferRef::Null(), PigmentShaderType );

			GpuApi::ResetVsWaveLimits();
		}
	}

	const char* pError = CCore::GetError();
	while (pError)
	{
		ERR_RENDERER( TXT( "%" ) RED_PRIWas, pError );
		pError = CCore::GetError();
	}
}

void CRenderProxy_SpeedTree::NoRender_FinalizeProcessing()
{
	FinishProcessTreeBuffers();
	FinalizeParallelGrassProcessing();
}

//dex++
void CRenderProxy_SpeedTree::PrepareCascadeShadows( const CRenderCollector& collector )
{
	PC_SCOPE( SpeedtreePrepareCascadeShadows );

	Uint16 numCascades = collector.m_cascades.m_numCascades;
	Int16 fullDetailsCascades = collector.m_cascades.MAX_NORMAL_CASCADES;
	m_hasBaseTrees[STATIC_FOLIAGE_IDX] = m_staticFoliageBalance > 0;
	m_hasBaseTrees[DYNAMIC_FOLIAGE_IDX] = m_dynamicFoliageBalance > 0;
	m_cameraPosition_cascades = Vector3(collector.GetRenderCamera().GetPosition());
	m_cullShadowsWithUmbra = collector.m_frame->GetFrameInfo().IsShowFlagOn( SHOW_UmbraCullSpeedTreeShadows );

	FinalizeParallelCascadesProcessing();

	if( !IsVisible() || !collector.m_renderFoliage )
	{
		return;
	}

	if( IsStaticTreesPopulationChangedInCascadesScheduled() )
	{
		NotifyStaticTreesPopulationChangedInCascades();
	}

	if( IsDynamicTreesPopulationChangedInCascadesScheduled() )
	{
		NotifyDynamicTreesPopulationChangedInCascades();
	}

	for(Uint16 i = 0; i < numCascades; ++i)
	{
		const SShadowCascade& cascade = collector.m_cascades.m_cascades[i];
		const CRenderCamera& camera = cascade.m_camera;
		CShadowCascade* threadData = m_cascadeThreadData[i];

		m_visibleTreesFromCascades[STATIC_FOLIAGE_IDX][i].PreUpdate3dTreeInstanceBuffers();

		threadData->m_camPos = camera.GetPosition();
		threadData->m_camForward = camera.GetCameraForward();
		threadData->m_zoom = camera.GetZoom();
		threadData->m_mtxWorldToView = camera.GetWorldToView();
		threadData->m_mtxViewToScreen = camera.GetViewToScreen();
		threadData->m_camFarPlane = camera.GetFarPlane();
		threadData->m_camNearPlane = camera.GetNearPlane();
		threadData->m_cullShadows = m_cullShadowsWithUmbra && i < fullDetailsCascades;
#ifdef USE_UMBRA
		threadData->m_occlusionData = &(collector.GetOcclusionData());
#endif // USE_UMBRA
	}

	if ( PARALLEL_FOLIAGE_PROCESSING && numCascades > 0 )
	{
		// Create elements
		{
			RED_FATAL_ASSERT( m_parallelShadowsProcessingElements.Empty(), "Didn't the last processing end?" );
			m_parallelShadowsProcessingElements.ClearFast();
			m_parallelShadowsProcessingElements.Reserve( numCascades );

			const Float fadeDistance = GetSpeedTreeShadowFadeDistance( collector.m_info );
			for ( Uint32 cascade_i=0; cascade_i<numCascades; ++cascade_i )
			{
				m_parallelShadowsProcessingElements.PushBack( SSpeedTreeParallelProcessingElement ( m_cascadeThreadData[cascade_i], fadeDistance ) );
			}
		}

		// Build processing params and start processing
		m_parallelShadowsProcessingParams = TParallelShadowsProcess::SParams::Create();
		{
			m_parallelShadowsProcessingParams->m_array				= &m_parallelShadowsProcessingElements[0];
			m_parallelShadowsProcessingParams->m_numElements		= (Int32)m_parallelShadowsProcessingElements.Size();
			m_parallelShadowsProcessingParams->m_processFunc		= &CRenderProxy_SpeedTree::PrepareSpecificCascadeShadowsElement;
			m_parallelShadowsProcessingParams->m_processFuncOwner	= this;
			m_parallelShadowsProcessingParams->m_priority			= TSP_High;
			m_parallelShadowsProcessingParams->SetDebugName			( TXT("ShadowsProcess") );

			m_parallelShadowsProcessingParams->StartProcessing();
		}
	}
	else
	{
		for(Uint16 i = 0; i < numCascades; ++i)
		{
			PrepareSpecificCascadeShadows( m_cascadeThreadData[i], GetSpeedTreeShadowFadeDistance(collector.m_info) );

			m_visibleTreesFromCascades[STATIC_FOLIAGE_IDX][i].PostUpdate3dTreeInstanceBuffers();
		}
	}

	if( m_dynamicFoliageBalance > 0 )
	{
		Uint32 numDynamicCascades = Min( numCascades, MAX_DYNAMIC_FOLIAGE_CASCADES );
		for( Uint16 i = 0; i < numDynamicCascades; ++i )
		{
			const SShadowCascade& cascade = collector.m_cascades.m_cascades[i];
			const CRenderCamera& camera = cascade.m_camera;
			CShadowCascade threadData;
			threadData.m_cascadeIndex = i;
			threadData.m_foliageType = DYNAMIC_FOLIAGE_IDX;
			threadData.m_camPos = camera.GetPosition();
			threadData.m_camForward = camera.GetCameraForward();
			threadData.m_zoom = camera.GetZoom();
			threadData.m_mtxWorldToView = camera.GetWorldToView();
			threadData.m_mtxViewToScreen = camera.GetViewToScreen();
			threadData.m_mtxWorldToScreen = camera.GetWorldToScreen();
			threadData.m_camFarPlane = camera.GetFarPlane();
			threadData.m_camNearPlane = camera.GetNearPlane();
			threadData.m_cullShadows = m_cullShadowsWithUmbra && i < fullDetailsCascades;
			CShadowCascade* threadDataPtr = &threadData;

			m_visibleTreesFromCascades[DYNAMIC_FOLIAGE_IDX][i].PreUpdate3dTreeInstanceBuffers();

			PrepareSpecificCascadeShadows( threadDataPtr, GetSpeedTreeShadowFadeDistance(collector.m_info) );

			m_visibleTreesFromCascades[DYNAMIC_FOLIAGE_IDX][i].PostUpdate3dTreeInstanceBuffers();
		}
	}

	const char* pError = CCore::GetError();
	while (pError)
	{
		ERR_RENDERER( TXT( "%" ) RED_PRIWas, pError );
		pError = CCore::GetError();
	}
}
//dex--

void CRenderProxy_SpeedTree::PrepareSpecificCascadeShadowsElement( SSpeedTreeParallelProcessingElement &element )
{
	PrepareSpecificCascadeShadows( element.m_cascade, element.m_fadeDistance );
}

void CRenderProxy_SpeedTree::PrepareSpecificCascadeShadows( CShadowCascade* const cascade, Float fadeDistance )
{
	Uint16 cascadeIdx = cascade->m_cascadeIndex;
	Uint16 foliageType = cascade->m_foliageType;

	// Update view
	SpeedTree::CView cascadeView;
	{
		// get camera position
		Vec3 camPos( cascade->m_camPos.AsFloat() );
		Vec3 camDir( cascade->m_camForward.AsFloat() );

		// setup projection matrix
		const Float zoom = cascade->m_zoom / 2.0f;
		Mat4x4 projectionMatrix;//( cascade.m_camera.GetViewToScreen().AsFloat() );
		projectionMatrix.Ortho( 
			-zoom, zoom, 
			-zoom, zoom, 
			0.0f,
			200.0f );
		projectionMatrix.Scale( 1.0f, 1.0f, -1.0f );
		Mat4x4 viewMatrix( cascade->m_mtxWorldToView.AsFloat() );

		// setup view
		cascadeView.Set( 
			camPos - camDir*100.0f, 
			projectionMatrix, 
			viewMatrix, 
			cascade->m_camNearPlane,
			cascade->m_camFarPlane );

		// setup LOD reference point
		cascadeView.SetLodRefPoint( Vec3( m_cameraPosition_cascades.X, m_cameraPosition_cascades.Y, m_cameraPosition_cascades.Z ) );

		// save for latter update of buffers
		m_cascadeViews[cascadeIdx] = cascadeView;

		// calculate rendering view
		SpeedTree::CView renderCascadeView;
		Mat4x4 projectionMatrixForRender( cascade->m_mtxViewToScreen.AsFloat() );			
		renderCascadeView.Set( 
			camPos,  // use original, unbiased position
			projectionMatrixForRender, 
			cascadeView.GetModelview(), 
			cascade->m_camNearPlane,
			cascade->m_camFarPlane );

		// setup LOD reference point
		renderCascadeView.SetLodRefPoint( Vec3( m_cameraPosition_cascades.X, m_cameraPosition_cascades.Y, m_cameraPosition_cascades.Z ) );

		// save for later
		m_renderCascadeViews[cascadeIdx] = renderCascadeView;
	}

	// Process trees culling and loding for this cascade
	if ( m_hasBaseTrees[foliageType] )
	{
		SpeedTree::CVisibleInstancesRender& visInstances = m_visibleTreesFromCascades[foliageType][cascadeIdx];

		// Rough cull
		{
			PC_SCOPE( RoughCull );
			visInstances.RoughCullCells( 
				cascadeView, 
				GGlobalSpeedTreeFrameIndex + cascadeIdx, 
				m_longestCellOverhang );
		}

		// Process trees culling and loding for this cascade
		{
			PC_SCOPE( UpdateCellExtents );
			for ( Int32 c = 0; c < (Int32)visInstances.RoughCells().size(); ++c )
			{
				// get SDK-side rough cell
				CCell& roughCell = visInstances.RoughCells()[c];

				// lookup app-side instance cell to get extents
				const SPerCellData* cellData = m_cellData[foliageType].GetCellByKey( SCellKey( roughCell.Row(), roughCell.Col() ) );
				if (cellData)
				{
					/*SpeedTree::CExtents ext = cellData->m_extents;
					SpeedTree::Vec3 extMin( ext.Min().x, ext.Min().y, -200.0f );
					SpeedTree::Vec3 extMax( ext.Max().x, ext.Max().y, -200.0f );
					SpeedTree::CExtents newExt( extMin, extMax );						*/
					roughCell.SetExtents( cellData->m_extents );
				}
			}
		}

		// Fine cull
		{
			PC_SCOPE( FineCull );
			visInstances.FineCullTreeCells( cascadeView, GGlobalSpeedTreeFrameIndex + cascadeIdx );
		}

		// Append instances to newly visible cells
		{
			PC_SCOPE( AppendingInstances );
			for ( size_t c = 0; c < visInstances.NewlyVisibleCells().size(); ++c )
			{
				CCell* newCell = visInstances.NewlyVisibleCells( )[c];
				ASSERT( newCell );

				const SPerCellData* cellData = m_cellData[foliageType].GetCellByKey(SCellKey( newCell->Row(), newCell->Col() ) );
				if ( cellData )
				{
					const CompiledInstanceContainer& appSideInstances = cellData->m_instancesOut;
					const TreeContainer& appSideBaseTrees = cellData->m_baseTrees;
					if ( !appSideInstances.Empty( ) && !appSideBaseTrees.Empty( ) )
					{
						const CTreeInstance** packedListOfInstances =		(const CTreeInstance**) &appSideInstances[0];
						const st_int32 totalInstancesForAllBaseTrees =	(st_int32)appSideInstances.Size( );
						const CTree** listOfBaseTrees =				(const CTree**) &appSideBaseTrees[0];
						const st_int32 numBaseTrees =					(st_int32)appSideBaseTrees.Size( );
						newCell->AppendTreeInstances( listOfBaseTrees, numBaseTrees, packedListOfInstances, totalInstancesForAllBaseTrees );
					}
				}
			}
		}
				
		CFrustum frustum( cascade->m_mtxWorldToScreen );

#ifdef USE_UMBRA
		if ( cascade->m_cullShadows && cascade->m_occlusionData )
		{
			RedShadowVisibilityHelper vh( cascade->m_occlusionData, frustum, cascadeIdx, Vec3( m_cameraPosition_cascades.X, m_cameraPosition_cascades.Y, m_cameraPosition_cascades.Z ), Config::cvFoliageShadowDistanceScale.Get(), Config::cvShadowDistanceBillboardScale.Get(), fadeDistance );			
			vh.m_useLegacyVisibilityTest = false;			
			visInstances.Update3dTreeLists( m_cascadeViews[cascadeIdx], &vh );			
		}
		else
#endif // USE_UMBRA
		{
			// by default we use legacy visibility test here
			RedShadowVisibilityHelper vh( 
#ifdef USE_UMBRA
				nullptr, 
#endif // USE_UMBRA
				frustum, cascadeIdx, Vec3( m_cameraPosition_cascades.X, m_cameraPosition_cascades.Y, m_cameraPosition_cascades.Z ), Config::cvFoliageShadowDistanceScale.Get(), Config::cvShadowDistanceBillboardScale.Get(), fadeDistance );
			visInstances.Update3dTreeLists( m_cascadeViews[cascadeIdx], &vh );			
		}

		{
			PC_SCOPE( Updating3DTreeInstanceBuffers );
			if ( !visInstances.Update3dTreeInstanceBuffers( m_cascadeViews[cascadeIdx] ) )
			{
				ERR_RENDERER( TXT("m_visibleTreesFromCascades[STATIC_FOLIAGE_IDX][index].Update3dTreeInstanceBuffers() failed!\n") );
			}
		}
	}
}

//dex++
void CRenderProxy_SpeedTree::RenderCascadeShadows( const RenderingContext& context, const CRenderFrameInfo& frameInfo, const SShadowCascade* cascade )
{
	const Uint32 index = cascade->m_cascadeIndex;

	FinalizeParallelCascadesProcessing();

	if( !IsVisible() || !frameInfo.IsShowFlagOn( SHOW_Foliage ) )
	{
		return;
	}

	{
		PC_SCOPE( RenderSpeedTreeShadows );

		// Grab some states
		const CGpuApiScopedDrawContext drawContextOriginalRestore;

		// Start render
		m_forestRender.StartRender();

		{
			// draw trees
			if ( frameInfo.IsShowFlagOn( SHOW_SpeedTreeShadows ) )
			{
				const Uint32 viewWidth = GpuApi::GetViewport().width;
				const Uint32 viewHeight = GpuApi::GetViewport().height;

				m_forestRender.GetFrameConstantBufferContents().m_sShadows.m_fShadowMapWritingActive = 1.0f;
				m_forestRender.UpdateFrameConstantBuffer( m_renderCascadeViews[ index ], viewWidth, viewHeight );
				m_forestRender.Render3dTrees( RENDER_PASS_SHADOW_CAST, m_visibleTreesFromCascades[STATIC_FOLIAGE_IDX][index], true );
				if( index < MAX_DYNAMIC_FOLIAGE_CASCADES && m_dynamicFoliageBalance > 0 )
				{
					m_forestRender.Render3dTrees( RENDER_PASS_SHADOW_CAST, m_visibleTreesFromCascades[DYNAMIC_FOLIAGE_IDX][index], true );
				}
			}
		}

		// End render
		m_forestRender.EndRender();
	}

	const char* pError = CCore::GetError();
	while (pError)
	{
		ERR_RENDERER( TXT( "%" ) RED_PRIWas, pError );
		pError = CCore::GetError();
	}
}
//dex--

void CRenderProxy_SpeedTree::SetupTreeFading( Bool enable )
{
	m_treeFadingEnabled = enable;
	m_treeFadingFadeInTime = FADE_IN_TIME;
	m_treeFadingFadeOutTime = FADE_OUT_TIME;
	m_treeFadingMinTreeHeight = MINIMUM_TREE_HEIGHT;
}

void CRenderProxy_SpeedTree::UpdateCellFading( const SpeedTree::CCell* const& cell )
{
	PC_SCOPE_PIX( UpdateCellFading );

#ifdef USE_ANSEL
	if ( isAnselCaptureActive )
	{
		// to avoid dithering during multipart captures
		return;
	}
#endif // USE_ANSEL

	// Even if fading is disabled, we may have to do some work, to show any potentially hidden trees.
	const Bool forceFadeIn = !m_treeFadingEnabled;

	// Two planes creating a triangle with the bottom of the screen. Any trees with position inside both planes will be faded out.
	const Vector& playerPos = m_treeFadingCenterReference;
	const Vector playerPosFar = playerPos + Vector( 0, 0, 1 );

	const Plane leftPlane( playerPos, playerPosFar, m_treeFadingLeftReference );
	const Plane rightPlane( playerPosFar, playerPos, m_treeFadingRightReference );

	const Float tickDelta = GetRenderer()->GetLastTickDelta();

	const Float treeFadeHideAmount = tickDelta / m_treeFadingFadeOutTime;
	const Float treeFadeShowAmount = tickDelta / m_treeFadingFadeInTime;


	// Find max distance between our reference points. We won't fade out anything that's farther from the camera than them.
	const Vec3& camPosST = m_forestView.GetCameraPos();
	Vector camPos( camPosST.x, camPosST.y, camPosST.z );

	Float maxDistSquared = 0.0f;
	maxDistSquared = Max( maxDistSquared, camPos.DistanceSquaredTo2D( m_treeFadingCenterReference ) );
	maxDistSquared = Max( maxDistSquared, camPos.DistanceSquaredTo2D( m_treeFadingLeftReference ) );
	maxDistSquared = Max( maxDistSquared, camPos.DistanceSquaredTo2D( m_treeFadingRightReference ) );

	Uint8 foliageTypeStart = STATIC_FOLIAGE_IDX;
	Uint8 foliageTypeEnd = m_dynamicFoliageBalance > 0 ? DYNAMIC_FOLIAGE_IDX : STATIC_FOLIAGE_IDX;
	for(Uint8 foliageType = foliageTypeStart; foliageType <= foliageTypeEnd; ++foliageType )
	{
		SPerCellData* cellData = m_cellData[foliageType].GetCellByKey(SCellKey( cell->Row(), cell->Col() ) );
		if ( cellData == nullptr )
		{
			continue;
		}

		const CompiledInstanceContainer& appSideInstances = cellData->m_instancesOut;

		// If we have no instances, nothing to do.
		if ( appSideInstances.Empty( ) )
		{
			continue;
		}

		const CExtents& cellExtents = cell->GetExtents();
		const Vec3& cellMin = cellExtents.Min();
		const Vec3& cellMax = cellExtents.Max();
		const Box cellBox( Vector( cellMin.x, cellMin.y, cellMin.z ), Vector( cellMax.x, cellMax.y, cellMax.z ) );

		const Plane::ESide cellBoundsLeft = leftPlane.GetSide( cellBox );
		const Plane::ESide cellBoundsRight = rightPlane.GetSide( cellBox );
		const Float cellDistSquared = cellBox.SquaredDistance2D( camPos );

		const Bool fadeOutWholeCell = cellBoundsLeft == Plane::PS_Back && cellBoundsRight == Plane::PS_Back && cellDistSquared < maxDistSquared;
		const Bool fadeInWholeCell = cellBoundsLeft == Plane::PS_Front || cellBoundsRight == Plane::PS_Front || cellDistSquared > maxDistSquared;

		// If this foliage cell is entirely inside the fading area, and everything is already faded out, we can skip it.
		if ( fadeOutWholeCell && !forceFadeIn )
		{
			if ( cellData->m_minFade >= 1.0f )
			{
				continue;;
			}
		}
		// If this cell is entirely outside the fading area, and everything is already faded in, we can skip it.
		else if ( fadeInWholeCell || forceFadeIn )
		{
			if ( cellData->m_maxFade <= 0.0f )
			{
				continue;
			}
		}

		Float cellMinFade = 1.0f;
		Float cellMaxFade = 0.0f;

		const CTreeInstance* const* packedListOfInstances	= &appSideInstances[0];
		const size_t totalInstancesForAllBaseTrees			= appSideInstances.Size( );

		for ( size_t i = 0; i < totalInstancesForAllBaseTrees; ++i )
		{
			const CTreeInstance* inst = packedListOfInstances[i];

			const CTree* tree = inst->InstanceOf();
			CExtents treeExtents = tree->GetExtents();
			treeExtents.Scale( inst->GetScalar() );

			// Skip anything small.
			if ( treeExtents.GetHeight() < m_treeFadingMinTreeHeight )
			{
				continue;
			}

			Bool shouldHide = false;

			if ( !forceFadeIn )
			{
				// If this foliage cell is entirely inside the fading area, we fade all trees in the cell.
				if ( fadeOutWholeCell )
				{
					shouldHide = true;
				}
				// If this cell is entirely outside the fading area, unfade all trees.
				else if ( fadeInWholeCell )
				{
					shouldHide = false;
				}
				// If the cell overlaps the area, then we need to check each tree separately.
				else
				{
					const Vec3& treePos = inst->GetPos();
					Vector pos( treePos.x, treePos.y, treePos.z, 1 );
					shouldHide = ( leftPlane.GetSide( pos ) == Plane::PS_Back && rightPlane.GetSide( pos ) == Plane::PS_Back && pos.DistanceSquaredTo2D( camPos ) < maxDistSquared );
				}
			}

			// HACK : Store instance fade data in the UserData... reinterpret_cast the pointer itself, instead of just pointing it to some value,
			// so we can avoid cache misses (the fading is stored right with the other instance data).
			size_t userData = reinterpret_cast< size_t >( inst->GetUserData() );

			// Fade is stored as a Float, so we need to converts the bits in userData. We could store it as an integer value, and scale it
			// appropriately, but we might get odd results at high or low frame times -- not enough precision for short frames, or long frames
			// result in inaccurate deltas; either way, we could end up with a less smooth transition, or maybe even get stuck.
			Uint32 fadeBits = ( Uint32 )( userData & 0xffffffff );
			Float fade = *( Float* )( &fadeBits );
			if ( shouldHide && fade < 1.0f )
			{
				fade = Min( fade + treeFadeHideAmount, 1.0f );
			}
			else if ( !shouldHide && fade > 0.0f )
			{
				fade = Max( fade - treeFadeShowAmount, 0.0f );
			}

			cellMinFade = Min( cellMinFade, fade );
			cellMaxFade = Max( cellMaxFade, fade );

			// Pack the fade bits back into userData.
			userData = *( Uint32* )( &fade );

			// Write back into the userdata.
			const_cast< CTreeInstance* >( inst )->SetUserData( reinterpret_cast< void* >( userData ) );
		}

		// If min/max fade are still out of order, we didn't have any fade-able instances. In this case, set the cell's min/max so that next time
		// we can early out.
		if ( cellMaxFade < cellMinFade )
		{
			if ( cellBoundsLeft == Plane::PS_Back && cellBoundsRight == Plane::PS_Back && !forceFadeIn )
			{
				cellMinFade = cellMaxFade = 1.0f;
			}
			else if ( cellBoundsLeft == Plane::PS_Front || cellBoundsRight == Plane::PS_Front || forceFadeIn )
			{
				cellMinFade = cellMaxFade = 0.0f;
			}
		}

		cellData->m_maxFade = cellMaxFade;
		cellData->m_minFade = cellMinFade;
	}
}

void CRenderProxy_SpeedTree::ProcessTreesInstances( const TreeRenderContainer& trees
#ifdef USE_UMBRA
												   , Bool cullWithUmbra, const CRenderOcclusionData* occlusionData
#endif // USE_UMBRA
												   , CRenderFrame* frame
												   )
{
	PC_SCOPE_PIX( ProcessTreeInstances );

#ifndef RED_FINAL_BUILD
	// Update metrics from previous frame
	SSpeedTreeRenderStats::s_visibleTreeCellCount[ SSpeedTreeRenderStats::s_bufferIndex ] += m_visibleTreesFromCamera[STATIC_FOLIAGE_IDX].VisibleCells().size();
	SSpeedTreeRenderStats::s_visibleTreeCellArrayCapacity[ SSpeedTreeRenderStats::s_bufferIndex ] += m_visibleTreesFromCamera[STATIC_FOLIAGE_IDX].VisibleCells().capacity() * sizeof( CCell* );
	SSpeedTreeRenderStats::s_visibleTreeCellArraySize[ SSpeedTreeRenderStats::s_bufferIndex ] += m_visibleTreesFromCamera[STATIC_FOLIAGE_IDX].VisibleCells().size() * sizeof( CCell* );
	for (TRowColCellPtrMap::const_iterator iCell = m_visibleTreesFromCamera[STATIC_FOLIAGE_IDX].VisibleCells().begin( ); iCell != m_visibleTreesFromCamera[STATIC_FOLIAGE_IDX].VisibleCells().end( ); ++iCell )
	{
		CCell* cell = iCell->second;
		SSpeedTreeRenderStats::s_visibleTreeInstanceCount[ SSpeedTreeRenderStats::s_bufferIndex ] += cell->GetTreeInstances().size();
		SSpeedTreeRenderStats::s_visibleTreeInstanceArrayCapacity[ SSpeedTreeRenderStats::s_bufferIndex ] += cell->GetTreeInstances().capacity() * sizeof( SpeedTree::CTreeInstance );
		SSpeedTreeRenderStats::s_visibleTreeInstanceArraySize[ SSpeedTreeRenderStats::s_bufferIndex ] += cell->GetTreeInstances().size() * sizeof( SpeedTree::CTreeInstance );
	}
#endif

#ifdef USE_UMBRA
	m_cullFoliageWithUmbra = cullWithUmbra;
	m_vHelper = RedVisibilityHelper( occlusionData, frame, frame->GetFrameInfo().IsShowFlagOn( SHOW_UmbraShowFoliageCells ) );
#else
	m_vHelper = RedVisibilityHelper( frame, frame->GetFrameInfo().IsShowFlagOn( SHOW_UmbraShowFoliageCells ) );
#endif // USE_UMBRA

	Uint32 foliageTypeStart = STATIC_FOLIAGE_IDX;
	Uint32 foliageTypeEnd = m_dynamicFoliageBalance > 0 ? DYNAMIC_FOLIAGE_IDX : STATIC_FOLIAGE_IDX;

#ifdef ENABLE_SPEEDTREE_DENSITY_VISUALISER
	for( Uint32 foliageType = foliageTypeStart; foliageType <= foliageTypeEnd; ++foliageType )
	{
		if( m_densityDebugger->GetVisualisationMode() != CSpeedTreeDensityDebugger::EMODE_None )
		{
			SpeedTree::CVisibleInstancesRender& visibleInstances = m_visibleTreesFromCamera[foliageType];
			for ( TRowColCellPtrMap::const_iterator iCell = visibleInstances.VisibleCells().begin( ); iCell != visibleInstances.VisibleCells().end( ); ++iCell )
			{
				CCell* cell = iCell->second;

				st_int32 numInstances = static_cast< st_int32 >( cell->GetTreeInstances().size() );
				for( st_int32 treeInstanceIndex = 0; treeInstanceIndex < numInstances; ++treeInstanceIndex )
				{
					m_densityDebugger->AddTreeInstance( cell->GetTreeInstances()[ treeInstanceIndex ]->GetPos() );
				}
			}
		}
	}
#endif

	if( IsStaticTreesPopulationChangedScheduled() )
	{
		NotifyStaticTreesPopulationChanged();
	}

	if( IsDynamicTreesPopulationChangedScheduled() )
	{
		NotifyDynamicTreesPopulationChanged();
	}

	RED_FATAL_ASSERT( GTaskManager, "Task manager has not been initialised yet" );
	//Lock Tree Instance Buffers
	m_visibleTreesFromCamera[STATIC_FOLIAGE_IDX].PreUpdate3dTreeInstanceBuffers();
	m_TreeProcessing[STATIC_FOLIAGE_IDX] = new (CTask::Root) CTreeInstanceProcessing(this, STATIC_FOLIAGE_IDX, trees);
	GTaskManager->Issue( *m_TreeProcessing[STATIC_FOLIAGE_IDX] );

	if ( m_dynamicFoliageBalance > 0 )
	{
		m_visibleTreesFromCamera[DYNAMIC_FOLIAGE_IDX].PreUpdate3dTreeInstanceBuffers();
		m_TreeProcessing[DYNAMIC_FOLIAGE_IDX] = new (CTask::Root) CTreeInstanceProcessing(this, DYNAMIC_FOLIAGE_IDX, trees);
		GTaskManager->Issue( *m_TreeProcessing[DYNAMIC_FOLIAGE_IDX] );
	}
}

void CRenderProxy_SpeedTree::ProcessTreeBuffers(TreeRenderContainer& trees, Uint32 foliageType)
{
	SpeedTree::CVisibleInstancesRender& visibleInstances = m_visibleTreesFromCamera[foliageType];
	TSpeedTreeGrid< SPerCellData, SpeedTree::CTreeInstance >& cellData = m_cellData[foliageType];

	// Rough cull
	{
		PC_SCOPE_PIX( RoughCull );
		visibleInstances.RoughCullCells( m_forestView, GGlobalSpeedTreeFrameIndex, m_longestCellOverhang );
	}

	// Extents update
	{
		PC_SCOPE_PIX( UpdateCellExtents );
		CExtents EMPTY_EXTENTS_BUGFIX; 
		for (Int32 c = 0; c < (Int32)visibleInstances.RoughCells().size(); ++c )
		{
			// get SDK-side rough cell
			CCell& roughCell = visibleInstances.RoughCells()[c];

			roughCell.SetExtents( EMPTY_EXTENTS_BUGFIX ); // <- This is a nasty bugfix for SpeedTree. Basically, the rough cells are not cleared between frames!!

			// lookup app-side instance cell to get extents
			const SPerCellData* data = cellData.GetCellByKey( SCellKey( roughCell.Row(), roughCell.Col() ) );
			if (data && !data->m_instancesOut.Empty() && !data->m_baseTrees.Empty())
			{
				roughCell.SetExtents( data->m_extents );
			}
		}
	}

	// Fine cull
	{
		PC_SCOPE_PIX( FineCull );
#ifdef USE_UMBRA
		if ( m_cullFoliageWithUmbra )
		{
			visibleInstances.FineCullTreeCells( m_forestView, GGlobalSpeedTreeFrameIndex, &m_vHelper );
		}
		else
#endif // USE_UMBRA
		{
			visibleInstances.FineCullTreeCells( m_forestView, GGlobalSpeedTreeFrameIndex );
		}
	}

	// Append instances to newly visible cells
	{
		PC_SCOPE_PIX( AppendingInstances );
		for ( size_t c = 0; c < visibleInstances.NewlyVisibleCells().size(); ++c )
		{
			CCell* newCell = visibleInstances.NewlyVisibleCells( )[c];
			ASSERT( newCell );

			const SPerCellData* data = cellData.GetCellByKey(SCellKey( newCell->Row(), newCell->Col() ) );
			if ( data )
			{
				const CompiledInstanceContainer& appSideInstances = data->m_instancesOut;
				const TreeContainer& appSideBaseTrees = data->m_baseTrees;
				if ( !appSideInstances.Empty( ) && !appSideBaseTrees.Empty( ) )
				{
					const CTreeInstance** packedListOfInstances =		(const CTreeInstance**) &appSideInstances[0];
					const st_int32 totalInstancesForAllBaseTrees =	(st_int32)appSideInstances.Size( );
					const CTree** listOfBaseTrees =				(const CTree**) &appSideBaseTrees[0];
					const st_int32 numBaseTrees =					(st_int32)appSideBaseTrees.Size( );

					newCell->AppendTreeInstances( listOfBaseTrees, numBaseTrees, packedListOfInstances, totalInstancesForAllBaseTrees );
				}
			}
		}
	}

	// Update tree fading for static foliage
	{
		PC_SCOPE_PIX( FadingTrees );
		for (TRowColCellPtrMap::const_iterator iCell = visibleInstances.VisibleCells().begin( ); iCell != visibleInstances.VisibleCells().end( ); ++iCell )
		{
			UpdateCellFading( iCell->second );
		}
	}

	//Update Tree Lists
	{
		PC_SCOPE_PIX( Updating3dTreeLists );
#ifdef USE_UMBRA
		if ( m_cullFoliageWithUmbra )
		{
			visibleInstances.Update3dTreeLists( m_forestView, &m_vHelper );
		}
		else
#endif
		{
			visibleInstances.Update3dTreeLists( m_forestView );
		}
	}

	// Update instance buffers
	{
		PC_SCOPE_PIX( Updating3DTreeInstanceBuffers );

		if ( !visibleInstances.Update3dTreeInstanceBuffers( m_forestView ) )
		{
			ERR_RENDERER( TXT("visibleInstances.Update3dTreeInstanceBuffers() failed!\n") );
		}
	}

	// Update Billboard Vertex Instance Data
	{
		for ( Int32 b = 0; b < (Int32)trees.Size(); ++b )
		{
			const CTreeRender* baseTree = trees[b];
			if (!baseTree->IsGrassModel( ))
			{
				visibleInstances.LAVACopyBillboardInstanceDataToInstanceBufferDoneRight( baseTree, m_forestView.GetFrustumPlanes(), 22.0f );
			}
		}
	}
}

void CRenderProxy_SpeedTree::FinishProcessTreeBuffers()
{
	Uint32 foliageTypeStart = STATIC_FOLIAGE_IDX;
	Uint32 foliageTypeEnd = m_dynamicFoliageBalance > 0 ? DYNAMIC_FOLIAGE_IDX : STATIC_FOLIAGE_IDX;
	for( Uint32 foliageType = foliageTypeStart; foliageType <= foliageTypeEnd; ++foliageType )
	{
		SpeedTree::CVisibleInstancesRender& visibleInstances = m_visibleTreesFromCamera[foliageType];

		if(!m_TreeProcessing[foliageType])
			continue;

		if(!m_UseMultiThreadedTrees)
			m_TreeProcessing[foliageType]->Run( );

		m_TreeProcessing[foliageType]->FinalizeTreeInstanceProcessing( );

		m_visibleTreesFromCamera[foliageType].PostUpdate3dTreeInstanceBuffers();

		m_TreeProcessing[foliageType]->Release( );
		m_TreeProcessing[foliageType] = nullptr;

	}
}


void CRenderProxy_SpeedTree::UpdateGrassInstanceMetrics( const SpeedTree::CVisibleInstancesRender& visibleGrassFromCamera )
{
	PC_SCOPE_PIX( UpdateGrassInstanceMetrics );

	// Update metrics
	SSpeedTreeRenderStats::s_visibleGrassCellCount[ SSpeedTreeRenderStats::s_bufferIndex ] += visibleGrassFromCamera.VisibleCells().size();
	SSpeedTreeRenderStats::s_visibleGrassCellArrayCapacity[ SSpeedTreeRenderStats::s_bufferIndex ] += visibleGrassFromCamera.VisibleCells().capacity() * sizeof( CCell* );
	SSpeedTreeRenderStats::s_visibleGrassCellArraySize[ SSpeedTreeRenderStats::s_bufferIndex ] += visibleGrassFromCamera.VisibleCells().size() * sizeof( CCell* );
	for ( TRowColCellPtrMap::const_iterator iCell = visibleGrassFromCamera.VisibleCells().begin( ); iCell != visibleGrassFromCamera.VisibleCells().end( ); ++iCell )
	{
		CCell* cell = iCell->second;
		SSpeedTreeRenderStats::s_visibleGrassInstanceCount[ SSpeedTreeRenderStats::s_bufferIndex ] += cell->GetGrassInstances().size();
		SSpeedTreeRenderStats::s_visibleGrassInstanceArrayCapacity[ SSpeedTreeRenderStats::s_bufferIndex ] += cell->GetGrassInstances().capacity() * sizeof( SpeedTree::SGrassInstance );
		SSpeedTreeRenderStats::s_visibleGrassInstanceArraySize[ SSpeedTreeRenderStats::s_bufferIndex ] += cell->GetGrassInstances().size() * sizeof( SpeedTree::SGrassInstance );
	}
}

void CRenderProxy_SpeedTree::BuildGrassInstanceSpeedtreeView( SpeedTree::CTreeRender* baseTree, SpeedTree::CView& targetView )
{
	// adjust frustum to closer far clip
	const Float grassFarClip = baseTree->GetLodProfile( ).m_fLowDetail3dDistance;

	// setup projection matrix
	Mat4x4 projectionMatrix;
	projectionMatrix.Perspective( m_cachedFov, m_cachedAspect, m_cachedNearPlane, grassFarClip );
	projectionMatrix.Scale( 1.0f, 1.0f, -1.0f );

	targetView.Set( m_cachedCamPos_grass, projectionMatrix, m_cachedViewMatrix, m_cachedNearPlane, grassFarClip );
}

void CRenderProxy_SpeedTree::UpdateGrassCellExtents( CGrassLayer* grassLayer, SpeedTree::CVisibleInstancesRender& visibleGrassFromCamera, Bool autoPopulate, const Vector2& areaSize )
{
	PC_SCOPE( UpdateCellExtents );

	if ( grassLayer->IsAutopopulated() && autoPopulate && m_cachedTerrainProxy )
	{
		// For automatically populated grass layers, we cover both the manually placed and procedurally generated grass extents
		// by setting extents based on the terrain min/max at the cell corners and the cell size
		const Float cellSize = grassLayer->GetCellSize();
		const Float extentsHeight = cellSize * GENERATED_GRASS_EXTENTS_MULTIPLIER;
		const Float halfCellSize = cellSize * 0.5f;
		const Vector2 cellCenterOffset( halfCellSize, halfCellSize );
		TCellArray& roughCells = visibleGrassFromCamera.RoughCells();
		if ( !roughCells.empty() )
		{
			// get frustum extents
			Int32 startRow, startCol, endRow, endCol;
			visibleGrassFromCamera.GetExtentsAsRowCols( startRow, startCol, endRow, endCol );
			const Int32 widthInCells = (endCol - startCol) + 2; // +2 to include samples on both ends
			const Int32 heightInCells = (endRow - startRow) + 2;

			// compute the extents for each cell
			CCell* roughCell = &roughCells[0];
			Float y = startRow * cellSize;
			for ( Int32 row = 0; row < heightInCells - 1; ++row )
			{
				st_float32 x = startCol * cellSize;
				for ( Int32 col = 0; col < widthInCells - 1; ++col )
				{
					// If this visible cell is NOT in the world / terrain bounds, we ignore it
					Vec3 vMin, vMax;
					if( m_clipmap0worldRect.Contains( Vector( x + cellCenterOffset.X, y + cellCenterOffset.Y, 0.0f ) ) )
					{
						Vector2 UV = Vector2( x, y ) - m_clipmap0worldRect.Min.AsVector2() + cellCenterOffset;
						UV.X /= areaSize.X;
						UV.Y /= areaSize.Y;
						Float terrainHeight = m_cachedTerrainProxy->GetUnfilteredHeightAtPos( UV );
						vMin = Vec3(x, y, terrainHeight - extentsHeight );
						vMax = Vec3(x + cellSize, y + cellSize, terrainHeight + extentsHeight );

						// adjust for grass height
						roughCell->SetExtents( CExtents( vMin, vMax ) );
					}

					++roughCell;
					x += cellSize;
				}

				y += cellSize;
			}
		}
	}
	else
	{
		// This layer doesn't have automatic population, so we can use the extents from our pregenerated client-side grid.
		for (Int32 c = 0; c < (Int32)visibleGrassFromCamera.RoughCells().size(); ++c )
		{
			// get SDK-side rough cell
			CCell& roughCell = visibleGrassFromCamera.RoughCells()[c];

			// lookup app-side instance cell to get extents
			const CGrassLayer::SPerCellData* cellData = grassLayer->GetCellByKey( SCellKey( roughCell.Row(), roughCell.Col() ) );
			if (cellData && cellData->HasGrassInstances())
			{
				roughCell.SetExtents( cellData->m_extents );
			}
		}
	}
}

void CRenderProxy_SpeedTree::PopulateVisibleGrassCells( CGrassLayer* grassLayer, SpeedTree::CVisibleInstancesRender& visibleGrassFromCamera, Bool autoPopulate )
{
	PC_SCOPE_PIX( AppendingInstances );
	const CGrassCellMask* grassCellMask = GetGrassCellMaskForLayer( grassLayer );
	for ( size_t c = 0; c < visibleGrassFromCamera.NewlyVisibleCells().size(); ++c )
	{
		CCell* newCell = visibleGrassFromCamera.NewlyVisibleCells( )[c];
		ASSERT( newCell );

		const Int32 row = newCell->Row();
		const Int32 col = newCell->Col();

		const CGrassLayer::SPerCellData* cellData = grassLayer->GetCellByKey(SCellKey( row, col ) );
		if ( cellData )
		{
			// We have app side cached instances
			const CGrassLayer::GrassInstances& appSideInstances = cellData->m_outGrassInstances;
			if ( !appSideInstances.Empty( ) )
			{
				PC_SCOPE_PIX(SetGrassInstances);

				const SGrassInstance* packedListOfInstances =		(const SGrassInstance*) &appSideInstances[0];
				const st_int32 numInstances =	(st_int32)appSideInstances.Size( );
#ifdef ENABLE_MAX_INSTANCES_LOGGING
				if( numInstances > GRASS_MAX_INSTANCE_COUNT )
				{
					RED_LOG_SPAM( RED_LOG_CHANNEL( Foliage ), TXT("Exceeded max grass instances when adding authored grass.") );
				}
#endif
				newCell->SetGrassInstances( packedListOfInstances, numInstances );
			}
		}

		if ( autoPopulate && m_cachedTerrainProxy && grassLayer->IsAutopopulated() && ( !grassCellMask || grassCellMask->IsBitSet( row, col ) ) )
		{
			// Procedurally add instances based on the terrain material
			TGrassInstArray& array = const_cast< TGrassInstArray& >( newCell->GetGrassInstances() );
			GenerateGrass( newCell, array, grassLayer );
		}
	}
}

void CRenderProxy_SpeedTree::ProcessGrassInstances( const CRenderCamera& renderCamera
#ifdef USE_UMBRA
												   , Bool cullWithUmbra, const CRenderOcclusionData* occlusionData
#endif // USE_UMBRA
												   )
{
	PC_SCOPE_PIX( ProcessGrassInstances );

	if ( PARALLEL_FOLIAGE_PROCESSING )
	{
		// Need to be sure about the work from previous frame being completed already. This shouldn't happen unless we kick off the jobs and then don't render foliage.
		PC_CHECKER_SCOPE( 0.0001f/*0.1ms*/, TXT("FOLIAGE"), TXT("Grass worker didn't finish last frame!") );
		FinalizeParallelGrassProcessing();
	}

	Float maxGrassLayerDrawDistance = 0.0f;
	Float minGrassCellSize = Red::System::NumericLimits< Float >::Max();
	Float maxGrassCellSize = 0.0f;

#ifndef RED_FINAL_BUILD
	// Update metrics
	SSpeedTreeRenderStats::s_grassLayerCount[ SSpeedTreeRenderStats::s_bufferIndex ] = m_grassLayers.Size();
#endif

	// Cache view
	m_cachedCamPos_grass.Set( renderCamera.GetPosition().AsFloat() );
	m_cachedViewMatrix.Set( renderCamera.GetWorldToView().AsFloat() );
	Float fov = renderCamera.GetFOV();
#ifdef USE_ANSEL
	if ( isAnselSessionActive )
	{
		// clamp camera fov
		fov = Max( renderCamera.GetFOV(), 60.0f );
	}
#endif // USE_ANSEL
	m_cachedFov = DegToRad( fov );
	m_cachedAspect = renderCamera.GetAspect();
	m_cachedNearPlane = renderCamera.GetNearPlane();

	{
#ifndef RED_FINAL_BUILD
		PC_SCOPE_PIX( UpdateGrassMetrics );
		for ( Uint32 gl = 0; gl<m_grassLayers.Size(); ++gl )
		{
			CGrassLayer* grassLayer = m_grassLayers[gl];
			CVisibleInstancesRender& visibleGrassFromCamera = grassLayer->GetVisible();
			ASSERT( grassLayer->GetBaseGrass() );

			const Float grassFarClip = grassLayer->GetBaseGrass()->GetRenderBaseTree()->GetLodProfile( ).m_fLowDetail3dDistance;
			maxGrassLayerDrawDistance = grassFarClip > maxGrassLayerDrawDistance ? grassFarClip : maxGrassLayerDrawDistance;

			const Float& cellSize = grassLayer->GetCellSize();
			if ( cellSize > maxGrassCellSize ) maxGrassCellSize = cellSize;
			else if ( cellSize < minGrassCellSize ) minGrassCellSize = cellSize;

			UpdateGrassInstanceMetrics( visibleGrassFromCamera );

#ifdef ENABLE_SPEEDTREE_DENSITY_VISUALISER
			// In order to accurately count layers per cell, we populate a seperate hash table to check which cells this layer has touched
			if( m_densityDebugger->GetVisualisationMode() != CSpeedTreeDensityDebugger::EMODE_None )
			{
				m_densityDebugger->ClearLayerToucher();
				for ( TRowColCellPtrMap::const_iterator iCell = visibleGrassFromCamera.VisibleCells().begin( ); iCell != visibleGrassFromCamera.VisibleCells().end( ); ++iCell )
				{
					CCell* cell = iCell->second;
					// Append each visible instance to the density metrics
					st_int32 numInstances = static_cast< st_int32 >( cell->GetGrassInstances().size() );
					for( st_int32 instanceIndex = 0; instanceIndex < numInstances; ++instanceIndex )
					{
						SpeedTree::Vec3 position = cell->GetGrassInstances()[ instanceIndex ].GetPos();
						m_densityDebugger->AddGrassInstance( cell->GetGrassInstances()[ instanceIndex ].GetPos() );
						m_densityDebugger->TouchLayer( position );
					}
				}
			}
#endif
		}
#endif
		
	}
	

#ifndef RED_FINAL_BUILD
	SSpeedTreeRenderStats::s_maxGrassLayerCullDistance[ SSpeedTreeRenderStats::s_bufferIndex ] = maxGrassLayerDrawDistance;
	SSpeedTreeRenderStats::s_minGrassCellSize[ SSpeedTreeRenderStats::s_bufferIndex ] = minGrassCellSize;
	SSpeedTreeRenderStats::s_maxGrassCellSize[ SSpeedTreeRenderStats::s_bufferIndex ] = maxGrassCellSize;
#endif

	if ( PARALLEL_FOLIAGE_PROCESSING )
	{
		PC_SCOPE_PIX( GrassProcessingKickOff )
		//PC_CHECKER_SCOPE( 0.0002f/*0.2ms*/, TXT( "FOLIAGE" ), TXT("Slow parallel for kickoff") );

		m_grassInstancesRingBuffer.Lock();

		m_grassProcessing = new (CTask::Root) CGrassInstanceProcessing(this);
		GTaskManager->Issue( *m_grassProcessing );
	}
	else
	{
		// NOTE: It causes iterating the layers once more, but it is just a fallback path. For debugging, profiling, etc. It is intended for the game to have all such stuff happen through the parallel path.
		for ( Uint32 gl = 0; gl<m_grassLayers.Size(); ++gl )
		{
			ProcessGrassLayer( m_grassLayers[gl] );
		}
	}
}

void CRenderProxy_SpeedTree::ProcessGrassLayer( CGrassLayer*& grassLayer )
{
	PC_SCOPE_PIX( ProcessGrassLayer ); 

	ASSERT( grassLayer );
	CVisibleInstancesRender& visibleGrassFromCamera = grassLayer->GetVisible();
	ASSERT( grassLayer->GetBaseGrass() );

	// Update view
	BuildGrassInstanceSpeedtreeView( grassLayer->GetBaseGrass()->GetRenderBaseTree(), grassLayer->GetView() );

	if( grassLayer->GetNumMaxInstances() != Config::cvGrassInstancesCount.Get() )
	{
		grassLayer->ScheduleGrassPopulationUpdate();
	}

	{
		PC_SCOPE_PIX( NotifyPopulationChange );
		if ( grassLayer->IsGrassPopulationUpdateScheduled() )
		{
			// Update and unschedule
			grassLayer->UpdatePopulation(); 
		}
	}

	// Cache the world sizes for terrain sampling
	const Vector2 areaSize = m_clipmap0worldRect.Max.AsVector2() - m_clipmap0worldRect.Min.AsVector2();
	Vector2 worldMinVec2 = m_clipmap0worldRect.Min.AsVector2();

	const CView& currentView = grassLayer->GetView();

	// Rough cull
	Bool doFineCull = true;
	{
		PC_SCOPE_PIX( RoughCull );
		if ( !visibleGrassFromCamera.RoughCullCells( currentView, GGlobalSpeedTreeFrameIndex, grassLayer->GetGrassCullRadius() ) )
		{
			// Nothing changed
			doFineCull = false;
		}
	}

	if( doFineCull )
	{
		{
			PC_SCOPE_PIX( UpdateGrassCellExtents );
			UpdateGrassCellExtents( grassLayer, visibleGrassFromCamera, m_genericGrassOn, areaSize );
		}

		// Fine cull
		{
			PC_SCOPE_PIX( FineCull );
			visibleGrassFromCamera.FineCullGrassCells( currentView, GGlobalSpeedTreeFrameIndex, grassLayer->GetGrassCullRadius() );
		}

		{
			PC_SCOPE_PIX( PopulateVisibleGrassCells );
			// Append instances to newly visible cells
			PopulateVisibleGrassCells( grassLayer, visibleGrassFromCamera, m_genericGrassOn );
		}

		// Flag that we need to update the instance buffers
		{
			if ( !visibleGrassFromCamera.NewlyVisibleCells().empty() )
			{
				grassLayer->SetNeedUpdateInstanceBuffer( true );
			}
		}
	}

	visibleGrassFromCamera.UpdateGrassInstanceBuffers( grassLayer->GetBaseGrass()->GetRenderBaseTree(), currentView.GetFrustumPlanes(), grassLayer->GetGrassCullRadius(), m_grassInstancesRingBuffer );
}

void CRenderProxy_SpeedTree::ProcessAllGrassLayers()
{
	for ( Uint32 gl = 0; gl<m_grassLayers.Size(); ++gl )
	{
		ProcessGrassLayer( m_grassLayers[gl] );
	}
}

CGrassLayer* CRenderProxy_SpeedTree::GetOrCreateGrassLayer( CRenderSpeedTreeResource* renderTree )
{
	ASSERT( renderTree->GetRenderBaseTree()->IsCompiledAsGrass() );
	for ( Uint32 l=0; l<m_grassLayers.Size(); ++l )
	{
		if ( m_grassLayers[l]->GetBaseGrass() == renderTree )
		{
			return m_grassLayers[l];
		}
	}

	// Not found, have to create
	m_grassLayers.Grow();
	CGrassLayer*& newLayer = m_grassLayers.Back();
	newLayer = new CGrassLayer();
	newLayer->SetBaseGrass( renderTree );

	// Update texture alpha scalar
	UpdateSpeedTreeResourceTextureAlphaScalars( renderTree, m_forestRender );

	//
	return newLayer;
}

CGrassLayer* CRenderProxy_SpeedTree::GetGrassLayer( CRenderSpeedTreeResource* renderTree )
{
	ASSERT( renderTree->GetRenderBaseTree()->IsCompiledAsGrass() );
	for ( Uint32 l=0; l<m_grassLayers.Size(); ++l )
	{
		if ( m_grassLayers[l]->GetBaseGrass() == renderTree )
		{
			return m_grassLayers[l];
		}
	}

	return NULL;
}

void CRenderProxy_SpeedTree::RemoveGrassInstances( CGrassLayer* layer, const Vector& position, Float radius )
{
	ASSERT( layer );

	PreGrassLayerChange( layer );

	// Remove instances in a circular area
	layer->RemoveInstances( position, radius );

	// Cleanup
	PostGrassInstancesRemoval( layer );
}

void CRenderProxy_SpeedTree::RemoveGrassInstances( CGrassLayer* layer, const Box& rect )
{
	ASSERT( layer );

	PreGrassLayerChange( layer );

	// Remove instances in a rectangular area
	layer->RemoveInstances( rect );

	// Cleanup
	PostGrassInstancesRemoval( layer );
}

void CRenderProxy_SpeedTree::RemoveTreeStaticInstances( CRenderSpeedTreeResource* renderTreeResource, const Vector& position, Float radius )
{
	//Make sure worker threads are finished
	PreTreeLayerChange();

	// Make sure we do operations on the speed tree structures only after parallel cascades processing is completed
	FinalizeParallelCascadesProcessing();

	// Remove instances
	Int32 outRowCol[4];
	Uint32 numRemoved = m_cellData[STATIC_FOLIAGE_IDX].RemoveInstances( renderTreeResource->GetRenderBaseTree(), position, radius, outRowCol );

	if ( numRemoved > 0 )
	{
		ScheduleStaticTreesPopulationChange();
		NotifyTreesPopulationRemoval( STATIC_FOLIAGE_IDX, outRowCol, renderTreeResource->GetRenderBaseTree() ); 
		m_updatedTrees.PushBackUnique(renderTreeResource->GetRenderBaseTree());

		ASSERT( m_usedSpeedTreeResources.KeyExist( renderTreeResource ) );

		// Cleanup
		m_staticFoliageBalance -= numRemoved;
		m_usedSpeedTreeResources[renderTreeResource] -= numRemoved;
		PostTreeInstancesRemoval( renderTreeResource );
	}
}

void CRenderProxy_SpeedTree::RemoveTreeStaticInstances( CRenderSpeedTreeResource* renderTreeResource, const Box& rect )
{
	// Remove instances
	Int32 outRowCol[4];
	Uint32 numRemoved = m_cellData[STATIC_FOLIAGE_IDX].RemoveInstances( renderTreeResource->GetRenderBaseTree(), rect, outRowCol );

	if ( numRemoved > 0 )
	{
		NotifyTreesPopulationRemoval( STATIC_FOLIAGE_IDX, outRowCol, renderTreeResource->GetRenderBaseTree() );
		m_updatedTrees.PushBackUnique(renderTreeResource->GetRenderBaseTree());
		ScheduleStaticTreesPopulationChange();

		ASSERT( m_usedSpeedTreeResources.KeyExist( renderTreeResource ) );

		// Cleanup
		m_staticFoliageBalance -= numRemoved;
		m_usedSpeedTreeResources[renderTreeResource] -= numRemoved;
		PostTreeInstancesRemoval( renderTreeResource );
	}
}

void CRenderProxy_SpeedTree::RemoveTreeDynamicInstances( CRenderSpeedTreeResource* renderTreeResource, const Vector& position, Float radius )
{
	//Make sure worker threads are finished
	PreTreeLayerChange();


	// Remove instances
	Int32 outRowCol[4];
	Uint32 numRemoved = m_cellData[DYNAMIC_FOLIAGE_IDX].RemoveInstances( renderTreeResource->GetRenderBaseTree(), position, radius, outRowCol );

	if ( numRemoved > 0 )
	{
		ScheduleDynamicTreesPopulationChange();
		NotifyTreesPopulationRemoval( DYNAMIC_FOLIAGE_IDX, outRowCol, renderTreeResource->GetRenderBaseTree() ); 

		ASSERT( m_usedSpeedTreeResources.KeyExist( renderTreeResource ) );

		// Cleanup
		m_dynamicFoliageBalance -= numRemoved;
		m_usedSpeedTreeResources[renderTreeResource] -= numRemoved;
		PostTreeInstancesRemoval( renderTreeResource );
	}
}

void CRenderProxy_SpeedTree::PreGrassLayerChange( CGrassLayer* layer )
{
	if ( PARALLEL_FOLIAGE_PROCESSING )
	{
		// Make sure we don't work on grass layers in parallel anymore.
		PC_CHECKER_SCOPE( 0.0001f/*0.1ms*/, TXT("FOLIAGE"), TXT("Stalling grass layer change because of workers!") );
		FinalizeParallelGrassProcessing();
	}
}

void CRenderProxy_SpeedTree::PreTreeLayerChange()
{
	if(m_UseMultiThreadedTrees)
	{
		FinishProcessTreeBuffers();
	}
}

void CRenderProxy_SpeedTree::PostGrassInstancesRemoval( CGrassLayer* layer )
{
	ASSERT( layer );

	// Is this layer empty now? ...
	if ( !layer->IsAutopopulated() && layer->GetNumInstances() == 0 )
	{
		// ... then get rid of it. 
		// The refcount of the base tree, will be decreased on grass layer destructor.
		m_grassLayers.Remove( layer );
		delete layer;
	}
}

void CRenderProxy_SpeedTree::PostTreeInstancesRemoval( CRenderSpeedTreeResource* renderTreeResource )
{
	auto iter = m_usedSpeedTreeResources.Find( renderTreeResource );
	if ( iter != m_usedSpeedTreeResources.End() && iter.Value() == 0 )
	{
		// Release base tree, and forget it.
		m_usedSpeedTreeResources.Erase(renderTreeResource);
		renderTreeResource->Release();
	}
}

void CRenderProxy_SpeedTree::CollectAllBaseTreesUsed(TreeContainer & baseTrees)
{
	for( auto res : m_usedSpeedTreeResources )
	{
		baseTrees.PushBack( static_cast< SpeedTree::CTree* >( res.m_first->GetRenderBaseTree() ) );
	}
}

void CRenderProxy_SpeedTree::CollectAllBaseTreesUsed(TreeRenderContainer & baseTrees)
{
	for( auto res : m_usedSpeedTreeResources )
	{
		baseTrees.PushBack( res.m_first->GetRenderBaseTree() );
	}
}

void CRenderProxy_SpeedTree::CollectAllBaseTreesUsed(ConstTreeContainer & baseTrees)
{
	for( auto res : m_usedSpeedTreeResources )
	{
		baseTrees.PushBack( static_cast< const SpeedTree::CTree* >( res.m_first->GetRenderBaseTree() ) );
	}
}

void CRenderProxy_SpeedTree::NotifyTreesPopulationRemoval( Uint16 foliageType, Int32 rowColRange[4], const SpeedTree::CTree* tree )
{
	// Clear instancing and visibility state of speed tree
	m_visibleTreesFromCamera[foliageType].NotifyOfPopulationRemoval( rowColRange, tree );
	for ( Uint32 i=0; i<MAX_CASCADES;++i )
	{
		m_visibleTreesFromCascades[foliageType][i].NotifyOfPopulationRemoval( rowColRange, tree );
	}
}

void CRenderProxy_SpeedTree::NotifyTreesPopulationAddition(Uint16 foliageType, TDynArray< SCellKey >& updatedCellsKeys, const SpeedTree::CTree* tree )
{
	// Set instancing and visibility state of speed tree
	for( auto key : updatedCellsKeys )
	{
		m_visibleTreesFromCamera[foliageType].NotifyOfPopulationAddition( key.m_nRow, key.m_nCol, tree );
		for ( Uint32 i=0; i<MAX_CASCADES;++i )
		{
			m_visibleTreesFromCascades[foliageType][i].NotifyOfPopulationAddition( key.m_nRow, key.m_nCol, tree );
		}
	}
}

void CRenderProxy_SpeedTree::NotifyStaticTreesPopulationChanged()
{
	// Clear instancing and visibility state of speed tree
	m_visibleTreesFromCamera[STATIC_FOLIAGE_IDX].NotifyOfFrustumReset();
	m_staticPopulatioChangeScheduled = false;
}

void CRenderProxy_SpeedTree::NotifyDynamicTreesPopulationChanged()
{
	// Clear instancing and visibility state of speed tree
	m_visibleTreesFromCamera[DYNAMIC_FOLIAGE_IDX].NotifyOfFrustumReset();
	m_dynamicPopulatioChangeScheduled = false;
}

void CRenderProxy_SpeedTree::NotifyStaticTreesPopulationChangedInCascades()
{
	// Clear instancing and visibility state of speed tree for cascades
	for ( Uint32 i=0; i<MAX_CASCADES;++i )
	{
		m_visibleTreesFromCascades[STATIC_FOLIAGE_IDX][i].NotifyOfFrustumReset();
	}
	m_staticPopulatioChangeInCascadesScheduled = false;
}

void CRenderProxy_SpeedTree::NotifyDynamicTreesPopulationChangedInCascades()
{
	// Clear instancing and visibility state of speed tree for cascades
	for ( Uint32 i=0; i<MAX_CASCADES;++i )
	{
		m_visibleTreesFromCascades[DYNAMIC_FOLIAGE_IDX][i].NotifyOfFrustumReset();
	}
	m_dynamicPopulatioChangeInCascadesScheduled = false;
}

Bool CRenderProxy_SpeedTree::InitializeBaseTree( CRenderSpeedTreeResource* baseTreeRenderResource, Uint32 foliageType /* = STATIC_FOLIAGE_IDX */ )
{
	CTreeRender* treeRender = baseTreeRenderResource->GetRenderBaseTree();
	ASSERT( treeRender );

	if( !baseTreeRenderResource->IsInitialized() && treeRender->GraphicsAreInitialized() )
	{
		CTreeRender* treeRender = baseTreeRenderResource->GetRenderBaseTree();
		ASSERT( treeRender );
		
		// Post-init stuff
		Float lodDistanceScale = 1.0f; 
		if ( treeRender->IsCompiledAsGrass() )
		{
			lodDistanceScale = m_currentGrassDistanceScaleParam;
		}
		else
		{
			lodDistanceScale = m_currentFoliageDistanceScaleParam;
		}

		SLodProfile lodProfile = treeRender->GetLodProfile();
		lodProfile.Scale( lodDistanceScale );
		treeRender->SetLodProfile( lodProfile );

		CWind& wind = treeRender->GetWind();
		wind.SetInitDirection( Vec3( 1.0f, 0.0f, 0.0f ) );
		wind.EnableGusting( true );

		baseTreeRenderResource->SetHasInitialized();
	}

	return treeRender->GraphicsAreInitialized();
}

Uint32 CRenderProxy_SpeedTree::GenerateGrass( SpeedTree::CCell* targetCell, TGrassInstArray& outInstances, CGrassLayer* grassLayer )
{
	PC_SCOPE_PIX(GenerateGrass);
	// vv Don't want this now, as it gets confusing. Checker scopes have been placed in other areas hinting that grass is a problem, so let's rely on the PC_SCOPE here.
	//PC_CHECKER_SCOPE( 0.0005f, TXT("FOLIAGE"), TXT("Slow Generate Grass") );

	CTreeRender* treeRender = grassLayer->GetBaseGrass()->GetRenderBaseTree();
	IRenderObject* grassRO = static_cast< IRenderObject* >( grassLayer->GetBaseGrass() );

	const Int32 grassIndex = m_cachedTerrainProxy->GetIndexForGrassType( grassRO );
	if ( grassIndex < 0 )
	{
		RED_ASSERT( "Automatic grass error. Please report!!!" );
		return 0;
	}

	Float cellSize = grassLayer->GetCellSize();

	// Compute extents of the area in world space
	const Int32 col = targetCell->Col();
	const Int32 row = targetCell->Row();
	Float minX = col * cellSize;
	Float maxX = minX + cellSize;
	Float minY = row * cellSize;
	Float maxY = minY + cellSize;

	// Generate base distribution
	Int32 grassMaxInstancesCount = (Int32)Config::cvGrassInstancesCount.Get();
	Int32 numAttempts = grassMaxInstancesCount;
	const Vector2 areaSize = m_clipmap0worldRect.Max.AsVector2() - m_clipmap0worldRect.Min.AsVector2();
	const Vector2 halfTexelSize( m_terrainInterVertexSpace * 0.5f );
	const Float elevationRange = m_maxElevation - m_minElevation;

	// Compute size of the array to fit the manual and automatically generated instances
	// Since manually placed grass can apparently go over the limit, we need to clamp the array size
	Int32 baseArraySize = Red::Math::NumericalUtils::Min<Int32>( (Int32)outInstances.size(), grassMaxInstancesCount );
	Int32 requiredArraySize = baseArraySize + (size_t)numAttempts;
	Int32 heapExceededAmount = requiredArraySize - grassMaxInstancesCount;

	// Adjust in case we exceeded the heap reserve
	if ( heapExceededAmount > 0 )
	{
#ifdef ENABLE_MAX_INSTANCES_LOGGING
		RED_LOG_SPAM( RED_LOG_CHANNEL( Foliage ), TXT("Exceeded max instance count (%i) when adding automatic grass."), heapExceededAmount );
#endif
		numAttempts -= (Uint32)heapExceededAmount;
	}

	CGrassLayer::GrassInstances& grassGenerationInstanceBuffer = grassLayer->GetGrassGenerationInstanceBuffer();

	// Ensure the write array is always big enough for the number of instances we will attempt to generate
	grassGenerationInstanceBuffer.Reserve( numAttempts );
	grassGenerationInstanceBuffer.ResizeFast( 0 );

	const Vec3 up = Vec3( 0.0f, 0.0f, 1.0f );
	Float attemptsConsumed = 0;
	Uint32 numGenerated = 0;
	Uint32 failures = 0;
	Float numAttemptsF = (Float)numAttempts;
	Float attemptsConsumeMultiplier = 1.0f;
	Float rangeX = (maxX - minX );
	Float rangeY = (maxY - minY );

	if ( numAttempts > 0 )
	{
		CStandardRand randGen;
		const Int32 seed = col * row * (Int32)( cellSize * 10.0f );

		Red::Math::Random::FastRand rndPosGen;
		rndPosGen.Seed( seed );

		Float height = 0.0f;
		Float scale = 1.0f;
		Vector2 UV;
		Float efficiency = 1.0f;

		while ( attemptsConsumed < numAttemptsF && efficiency > Config::cvGrassGenerationEfficiency.Get() )
		{
			Int32 instanceBufferSize = (Int32)grassGenerationInstanceBuffer.Size();
			const Float randX = rndPosGen.Get<Float>();
			const Float randY = rndPosGen.Get<Float>();
			const Float realX = randX * rangeX + minX;
			const Float realY = randY * rangeY + minY;

			if ( m_clipmap0worldRect.Contains( Vector( realX, realY, 0.0f ) ) && !m_cachedTerrainProxy->IsGrassMaskedOut( realX, realY ) )
			{
				const Vector2 pos( realX, realY );
				UV = ( pos - m_clipmap0worldRect.Min.AsVector2() );
				UV.X /= areaSize.X;
				UV.Y /= areaSize.Y;

				if ( m_cachedTerrainProxy->GetHeightParamsAtPos( UV, grassIndex, height, scale, attemptsConsumed ) )
				{
					grassGenerationInstanceBuffer.ResizeFast( instanceBufferSize + 1 );

					SGrassInstance* instanceWritePtr = &grassGenerationInstanceBuffer[ instanceBufferSize ];
					instanceWritePtr->SetPos( Vec3( pos.X - halfTexelSize.X, pos.Y - halfTexelSize.Y, height ) );
					instanceWritePtr->SetScalar( scale );

					// reuse position as rotation. It will be normalized
					Vec3 right( rndPosGen.Get<Float>() * 2.0f - 1.0f, rndPosGen.Get<Float>() * 2.0f - 1.0f, 0.0f );

					instanceWritePtr->SetOrientation(up, right);

					++numGenerated;
				}
				else
				{
					++failures;
					// we need at least 100 failure samples before estimating efficiency: 7.5% is the border condition
					if( failures > 100 )
					{
						efficiency = (Float)numGenerated / failures;
					}
				}
			}
			else
			{
				attemptsConsumed += 1.0f;
			}
		}

		numGenerated = grassGenerationInstanceBuffer.Size();

		if( numGenerated > 0 )
		{
			// Now, we can copy the generated instances into the output buffer
			Int32 sizeBefore = (Int32)outInstances.size();
			outInstances.resize( numGenerated + baseArraySize );
			Int32 sizeAfter = (Int32)outInstances.size();

			// This can probably be quicker
			Red::System::MemoryCopy( &outInstances[ baseArraySize ], &grassGenerationInstanceBuffer[0], numGenerated * sizeof( SGrassInstance ) );
		}
	}

	grassLayer->SetNumMaxInstances( grassMaxInstancesCount );

	return numGenerated;
}

void CRenderProxy_SpeedTree::UpdateGrassConstants( const CRenderFrameInfo& frameInfo )
{
	ASSERT( m_grassConstantBufferRef );
	ASSERT( m_grassConstantBuffer );

	void* constantData = GpuApi::LockBuffer( m_grassConstantBufferRef, GpuApi::BLF_Discard, 0, sizeof( SGrassConstantBuffer ) );
	if ( constantData )
	{
		SGrassConstantBuffer *buf = static_cast<SGrassConstantBuffer*>( constantData );

		// Fill pigment constants
		if ( m_cachedTerrainProxy )
		{
			buf->m_pigmentConstants = m_cachedTerrainProxy->GetPigmentData().m_constants;
		}

		// Fill terrain normal constants
		buf->m_terrainNormalsAreaParams = Vector::ZEROS;
		buf->m_terrainNormalsParams.Set4( Max( 0.f, frameInfo.m_speedTreeParameters.m_grassNormalsVariation ), 0.f, 0.f, 0.f );
		if ( m_cachedTerrainProxy )
		{
			const Box &clipWindowBox = m_cachedTerrainProxy->GetPigmentMapNormalsWindow();
			const GpuApi::TextureLevelDesc desc = GpuApi::GetTextureLevelDesc( m_cachedTerrainProxy->GetNormalMapsArray(), 0 );			

			// Disabled assert since it doesn't break anything (intermediate maps will be regenerated anyway after clipWindow update),
			// and I don't think there is any reason to spend time making a full blown solution.
			// ASSERT( Abs( (clipWindowBox.Max.X - clipWindowBox.Min.X) - (clipWindowBox.Max.Y - clipWindowBox.Min.Y) ) <= 0.001f );

			ASSERT( desc.width == desc.height );

			Float regionSize = clipWindowBox.Max.X - clipWindowBox.Min.X;
			Float regionInvSize = 1.f / regionSize;
			Float regionBiasX = -clipWindowBox.Min.X * regionInvSize + 0.5f / desc.width;
			Float regionBiasY = -clipWindowBox.Min.Y * regionInvSize + 0.5f / desc.height;

			buf->m_terrainNormalsAreaParams.Set4( regionInvSize, regionBiasX, regionBiasY, (Float)m_cachedTerrainProxy->GetPigmentMapNormalsClipmapIndex() );
		}

		// Fill alphaScalarMulParams
		{
			const Float grassAlphaScalarScale = Max( 0.001f, frameInfo.m_speedTreeParameters.m_alphaScalarGrassNear ) / Max( 0.001f, frameInfo.m_speedTreeParameters.m_alphaScalarGrass );
			const Float grassAlphaScalarDistNear = frameInfo.m_speedTreeParameters.m_alphaScalarGrassDistNear;
			const Float grassAlphaScalarDistFar = Max( frameInfo.m_speedTreeParameters.m_alphaScalarGrassDistNear + 0.001f, frameInfo.m_speedTreeParameters.m_alphaScalarGrassDistFar );
			buf->alphaScalarMulParams.Set4( grassAlphaScalarScale - 1, -1.f / (grassAlphaScalarDistFar - grassAlphaScalarDistNear), 1 + grassAlphaScalarDistNear / (grassAlphaScalarDistFar - grassAlphaScalarDistNear), 0.f );
		}
				
		Uint32 activeCollisions = m_grassConstantBuffer->m_collisionsConstants.m_activeCollisionsNum;

		// Fill collision constants
		for(Uint32 i=0; i<activeCollisions; i++)
		{			
			buf->m_collisionsConstants.m_transformMatrices[i] = m_grassConstantBuffer->m_collisionsConstants.m_transformMatrices[i];
		}		
		buf->m_collisionsConstants.m_activeCollisionsNum = activeCollisions;		

		GpuApi::UnlockBuffer( m_grassConstantBufferRef );
	}
}

void CRenderProxy_SpeedTree::UpdateDynamicGrassColisions( const TDynArray< SDynamicCollider >& collisions )
{	
	ASSERT( m_grassConstantBuffer );

	Uint32 activeCollisions = 0;
	Uint32 maxCollisions = ( collisions.Size() > DYNAMIC_COLLISION_LIMIT ) ? DYNAMIC_COLLISION_LIMIT : collisions.Size();

	for( Uint32 i=0; i<maxCollisions; i++ )
	{
		if( collisions[i].m_intensity > 0.001f ) 
		{	
			m_grassConstantBuffer->m_collisionsConstants.m_transformMatrices[ activeCollisions ] = collisions[i].m_transformMatrix;

			// Set hide factor, which changes sign of intensity (so there is no need to create another variable in constant buffer)
			// When sign of intensity is minus, then the hide factor is applied to final result in shader - all it does is transforming vertices
			// so they are hidden under visible terrain.
			Float intensity = collisions[ i ].m_intensity;
			Vector currentTranslation = collisions[ i ].m_transformMatrix.GetRow(3);
			currentTranslation.W = collisions[ i ].m_useHideFactor ? -intensity : intensity;
			m_grassConstantBuffer->m_collisionsConstants.m_transformMatrices[ activeCollisions ].SetRow( 3, currentTranslation );

			activeCollisions++;
		}
	}
	m_grassConstantBuffer->m_collisionsConstants.m_activeCollisionsNum = activeCollisions;
}

void CRenderProxy_SpeedTree::SetOccurrenceMasks( TDynArray< CGrassCellMask >& occurrenceMasks )
{
	m_grassOccurrenceMasks = Move( occurrenceMasks );
}

void CRenderProxy_SpeedTree::UpdateFoliageRenderParams( const SFoliageRenderParams &params )
{
	// Collect all trees into one array
	TreeRenderContainer baseTrees;
	CollectAllBaseTreesUsed( baseTrees );

	for ( Uint32 g=0; g<m_grassLayers.Size(); ++g )
	{
		baseTrees.PushBackUnique( m_grassLayers[g]->GetBaseGrass()->GetRenderBaseTree() );
	}

	for( auto iter = baseTrees.Begin(), end = baseTrees.End(); iter != end; ++iter)
	{
		CTree * tree = *iter;
		SLodProfile lodProfile = tree->GetLodProfile();
		if( tree->IsCompiledAsGrass() )
		{
			lodProfile.Scale( params.m_grassDistScale  );
		}
		else
		{
			lodProfile.Scale(  params.m_foliageDistShift );
		}
		
		tree->SetLodProfile( lodProfile );
	}
}

CExtents ComputeExtents( const FoliageInstanceContainer & instances, const Box & box )
{
	if( box == Box::EMPTY )
	{
		CExtents extents;

		for( auto iter = instances.Begin(), end = instances.End(); iter != end; ++iter )
		{
			extents.ExpandAround( iter->GetPosition().A );
		}

		return extents;
	}
	else
	{
		return CExtents(box.Min.A, box.Max.A );
	}
}

void CRenderProxy_SpeedTree::AddGrassInstances( CRenderSpeedTreeResource* baseTreeRenderResource, const FoliageInstanceContainer & renderFoliageInstancesBuffer, CTreeRender* treeRender, const Box & box )
{
	PC_CHECKER_SCOPE( 0.001f, TXT("FOLIAGE"), TXT("Slow AddGrassInstances") );

	CGrassLayer* grassLayer = GetOrCreateGrassLayer( baseTreeRenderResource );
	ASSERT( grassLayer );

	PreGrassLayerChange( grassLayer );

	// Generate render side instances
	CGrassLayer::GrassInstances instancesToAdd;
	{
		//PC_CHECKER_SCOPE( 0.001f, TXT("FOLIAGE"), TXT("Slow render side instances array resizing") );
		instancesToAdd.Resize( renderFoliageInstancesBuffer.Size() );
	}

	CExtents sumExtents = ComputeExtents( renderFoliageInstancesBuffer, box );
	CExtents baseTreeExtents = treeRender->GetExtents();

	{
		//PC_CHECKER_SCOPE( 0.001f, TXT("FOLIAGE"), TXT("Slow render side instances iteration") );
		for ( Uint32 i=0; i<instancesToAdd.Size(); ++i )
		{
			SGrassInstance& stInstance = instancesToAdd[i];
			const SFoliageInstance& instanceInfo = renderFoliageInstancesBuffer[i];
			const Vector3 up = instanceInfo.GetUpVector();
			const Vector3 right = instanceInfo.GetRightVector();
			stInstance.SetOrientation( up.A, right.A );
			stInstance.SetPos( (Float*)&(instanceInfo.GetPosition()) );
			stInstance.SetScalar( instanceInfo.GetScale() );
		}
	}

	{
		//PC_CHECKER_SCOPE( 0.001f, TXT("FOLIAGE"), TXT("Slow grassLayer->AddInstances") );
		grassLayer->AddInstances( instancesToAdd, sumExtents );
	}

	grassLayer->ScheduleGrassPopulationUpdate();
}

void CRenderProxy_SpeedTree::AddTreeStaticInstances( CRenderSpeedTreeResource* baseTreeRenderResource, const FoliageInstanceContainer & renderFoliageInstancesBuffer, CTreeRender* treeRender, const Box & box )
{
	PC_CHECKER_SCOPE( 0.001f, TXT("FOLIAGE"), TXT("Slow AddTreeInstances") );

	{
		if ( !m_usedSpeedTreeResources.KeyExist( baseTreeRenderResource ) )
		{
			// From this moment, this base tree resource, will be un use by this proxy. Addref it, and remember it.
			m_usedSpeedTreeResources.Insert( baseTreeRenderResource, 0 );
			baseTreeRenderResource->AddRef();

			// Update texture alpha scalars
			UpdateSpeedTreeResourceTextureAlphaScalars( baseTreeRenderResource, m_forestRender );
		}

		// Generate render side instances
		InstanceContainer instancesToAdd;
		instancesToAdd.Resize( renderFoliageInstancesBuffer.Size() );

		CExtents sumExtents = ComputeExtents( renderFoliageInstancesBuffer, box );
		CExtents baseTreeExtents = treeRender->GetExtents();
	
		for ( Uint32 i=0; i<instancesToAdd.Size(); ++i )
		{
			CTreeInstance& stInstance = instancesToAdd[i];
			const SFoliageInstance& instanceInfo = renderFoliageInstancesBuffer[i];
			const Vector3 up = instanceInfo.GetUpVector();
			const Vector3 right = instanceInfo.GetRightVector();
			stInstance.SetInstanceOf( treeRender );	
			stInstance.SetOrientation( up.A, right.A );
			stInstance.SetPos( instanceInfo.GetPosition().A );
			stInstance.SetScalar( instanceInfo.GetScale() );
			stInstance.ComputeCullParameters();
			m_longestCellOverhang = ::Max( m_longestCellOverhang, stInstance.GetCullingRadius( ));
		}

		m_updatedCellsKeys.ClearFast();
		if( box == Box::EMPTY )
		{
			m_cellData[STATIC_FOLIAGE_IDX].AddInstances( instancesToAdd, sumExtents, baseTreeExtents, m_updatedCellsKeys );
		}
		else
		{
			Int32 row;
			Int32 col;
			ComputeCellCoords( sumExtents.GetCenter(), m_cellData[STATIC_FOLIAGE_IDX].GetCellSize(), row, col );
			SCellKey key = SCellKey( row, col );
			m_updatedCellsKeys.PushBack( key );
			SPerCellData * cellData = m_cellData[STATIC_FOLIAGE_IDX].GetOrCreateCellByKey( key );
			cellData->AddInstances( instancesToAdd, sumExtents );
			cellData->Recompile();
		}
		NotifyTreesPopulationAddition( STATIC_FOLIAGE_IDX, m_updatedCellsKeys, treeRender ); 
		m_updatedTrees.PushBackUnique( treeRender );
		ScheduleStaticTreesPopulationChange();

		m_staticFoliageBalance += instancesToAdd.Size();
		m_usedSpeedTreeResources[baseTreeRenderResource] += instancesToAdd.Size();
	}
}

void CRenderProxy_SpeedTree::AddTreeDynamicInstances( CRenderSpeedTreeResource* baseTreeRenderResource, const FoliageInstanceContainer & renderFoliageInstancesBuffer, CTreeRender* treeRender, const Box & box )
{
	PC_CHECKER_SCOPE( 0.001f, TXT("FOLIAGE"), TXT("Slow AddTreeDynamicInstances") );

	PreTreeLayerChange();

	{
		if ( !m_usedSpeedTreeResources.KeyExist( baseTreeRenderResource ) )
		{
			// From this moment, this base tree resource, will be un use by this proxy. Addref it, and remember it.
			m_usedSpeedTreeResources.Insert( baseTreeRenderResource, 0 );
			baseTreeRenderResource->AddRef();

			// Update texture alpha scalars
			UpdateSpeedTreeResourceTextureAlphaScalars( baseTreeRenderResource, m_forestRender );
		}

		// Generate render side instances
		ASSERT( renderFoliageInstancesBuffer.Size() == 1 );
		InstanceContainer instancesToAdd;
		instancesToAdd.Resize( 1 );

		CExtents sumExtents = ComputeExtents( renderFoliageInstancesBuffer, box );
		CExtents baseTreeExtents = treeRender->GetExtents();

		CTreeInstance& stInstance = instancesToAdd[0];
		const SFoliageInstance& instanceInfo = renderFoliageInstancesBuffer[0];
		const Vector3 up = instanceInfo.GetUpVector();
		const Vector3 right = instanceInfo.GetRightVector();
		stInstance.SetInstanceOf( treeRender );	
		stInstance.SetOrientation( up.A, right.A );
		stInstance.SetPos( instanceInfo.GetPosition().A );
		stInstance.SetScalar( instanceInfo.GetScale() );
		stInstance.ComputeCullParameters();
		m_longestCellOverhang = ::Max( m_longestCellOverhang, stInstance.GetCullingRadius( ));

		m_updatedCellsKeys.ClearFast();
		if( box == Box::EMPTY )
		{
			m_cellData[DYNAMIC_FOLIAGE_IDX].AddInstances( instancesToAdd, sumExtents, baseTreeExtents, m_updatedCellsKeys );
		}
		else
		{
			Int32 row;
			Int32 col;
			ComputeCellCoords( sumExtents.GetCenter(), m_cellData[DYNAMIC_FOLIAGE_IDX].GetCellSize(), row, col );
			SCellKey key = SCellKey( row, col );
			m_updatedCellsKeys.PushBack( key );
			SPerCellData * cellData = m_cellData[DYNAMIC_FOLIAGE_IDX].GetOrCreateCellByKey( key );
			cellData->AddInstances( instancesToAdd, sumExtents );
			cellData->Recompile();
		}
		NotifyTreesPopulationAddition( DYNAMIC_FOLIAGE_IDX, m_updatedCellsKeys, treeRender ); 
		ScheduleDynamicTreesPopulationChange();

		// Cache instances count of the tree
		m_dynamicFoliageBalance += instancesToAdd.Size();
		m_usedSpeedTreeResources[baseTreeRenderResource] += instancesToAdd.Size();
	}
}

const CGrassCellMask* CRenderProxy_SpeedTree::GetGrassCellMaskForLayer( const CGrassLayer* grassLayer ) const
{
	const CRenderSpeedTreeResource* res = grassLayer->GetBaseGrass();
	if ( res )
	{
		// Find a grass cell mask for this layer
		const String& srtFileName = res->GetSRTFileName();
		Uint32 numCellMasks = m_grassOccurrenceMasks.Size();
		for ( Uint32 i=0; i<numCellMasks; ++i )
		{
			if ( m_grassOccurrenceMasks[i].m_srtFileName == srtFileName )
			{
				// found
				return &m_grassOccurrenceMasks[i];
			}
		}
	}
	return nullptr;
}

void CRenderProxy_SpeedTree::FinalizeParallelGrassProcessing()
{
	if ( m_grassProcessing )
	{
		m_grassProcessing->FinalizeGrassInstanceUpdates( );
		m_grassProcessing->Release();
		m_grassProcessing = nullptr;

		m_grassInstancesRingBuffer.Unlock();
	}
}

void CRenderProxy_SpeedTree::FinalizeParallelCascadesProcessing()
{
	if ( m_parallelShadowsProcessingParams )
	{
		PC_SCOPE( WaitForShadowsCreationWorkers );

		const Uint32 numCascades = m_parallelShadowsProcessingParams->m_numElements;

		m_parallelShadowsProcessingParams->FinalizeProcessing();
		m_parallelShadowsProcessingParams->Release();
		m_parallelShadowsProcessingParams = nullptr;

		for ( Uint32 i=0; i<numCascades; ++i )
		{
			m_visibleTreesFromCascades[STATIC_FOLIAGE_IDX][i].PostUpdate3dTreeInstanceBuffers();
		}

		m_parallelShadowsProcessingElements.ClearFast();
	}
}

void CRenderProxy_SpeedTree::Prefetch( CRenderFramePrefetch* prefetch ) const
{
	const Vec3 refPointVec( prefetch->GetCameraPosition().AsFloat() );

	// Static and dynamic trees
	{
		PrefetchHelper( prefetch, m_cellData[ STATIC_FOLIAGE_IDX ], refPointVec );
		PrefetchHelper( prefetch, m_cellData[ DYNAMIC_FOLIAGE_IDX ], refPointVec );
	}

	// Grass
	{
		ConstTreeRenderContainer grassTrees;
		for ( CGrassLayer* grassLayer : m_grassLayers )
		{
			if ( grassLayer->GetBaseGrass() && grassLayer->GetBaseGrass()->GetRenderBaseTree() )
			{
				grassTrees.PushBack( grassLayer->GetBaseGrass()->GetRenderBaseTree() );
			}
		}
		PrefetchHelper( prefetch, grassTrees, GRASS_CELL_SIZE_MAX / 2.0f );
	}
}

void CRenderProxy_SpeedTree::PrefetchHelper( CRenderFramePrefetch* prefetch, const TSpeedTreeGrid< SPerCellData, CTreeInstance >& grid, const Vec3& refPointVec ) const
{
	Int32 row;
	Int32 col;
	ComputeCellCoords( refPointVec, grid.GetCellSize(), row, col );
	const Int32 startRow = row-1;
	const Int32 endRow = row+1;
	const Int32 startCol = col-1;
	const Int32 endCol = col+1;

	for ( Int32 r=startRow; r<=endRow; ++r )
	{
		for ( Int32 c=startCol; c<=endCol; ++c )
		{
			SCellKey key = SCellKey( r, c );
			const SPerCellData * cellData = grid.GetCellByKey( key );
			if ( cellData )
			{
				ConstTreeRenderContainer trees;
				cellData->GetUsedBaseTrees( trees );

				const Float squaredDistance = cellData->m_extents.GetCenter().DistanceSquared( refPointVec );
				PrefetchHelper( prefetch, trees, squaredDistance );
			}
		}
	}
}

void CRenderProxy_SpeedTree::PrefetchHelper( CRenderFramePrefetch* prefetch, const ConstTreeRenderContainer& trees, Float squareDistance ) const
{
	for ( const CTreeRender* tree : trees )
	{
		const CArray< SpeedTree::CRenderState >& renderStates3D = tree->Get3dRenderStates( SpeedTree::RENDER_PASS_MAIN );
		const SpeedTree::CRenderState& billboardRenderState = tree->GetBillboardRenderState( SpeedTree::RENDER_PASS_MAIN );
		for ( const CRenderState& renderState : renderStates3D )
		{
			PrefetchHelperAddTextureBinds( prefetch, renderState, squareDistance );
		}
		PrefetchHelperAddTextureBinds( prefetch, billboardRenderState, squareDistance );
	}
}

void CRenderProxy_SpeedTree::PrefetchHelperAddTextureBinds( CRenderFramePrefetch* prefetch, const SpeedTree::CRenderState& renderState, Float squareDistance ) const
{
	for ( Uint32 i = 0; i < TL_NUM_TEX_LAYERS; ++i )
	{
		CRenderTexture* texture = renderState.GetTextureClass( i ).m_tTexturePolicy.GetRenderTexture();
		prefetch->AddTextureBind( texture, squareDistance );
	}
}

void CRenderProxy_SpeedTree::ProcessAllUpdateRequest( SFoliageUpdateRequest updates )
{
	if( !updates.addRequestContainer.Empty() || !updates.removeRequestContainer.Empty() )
	{
		//Make sure worker threads are finished
		PreTreeLayerChange();

		// Make sure we do operations on the speed tree structures only after parallel cascades processing is completed
		FinalizeParallelCascadesProcessing();

		// Make sure previous prefetch is already finished
		FinalizeParallelInstanceUpdates();

		m_updateInstancesProcessing = new (CTask::Root) CUpdateTreeInstancesProcessing( this, std::move(updates) );
		GTaskManager->Issue( *m_updateInstancesProcessing );
	}
}

void CRenderProxy_SpeedTree::FinalizeParallelInstanceUpdates()
{
	if( m_updateInstancesProcessing )
	{
		PC_SCOPE( WaitOnParallelInstanceUpdates );

		m_updateInstancesProcessing->FinalizeTreeInstanceUpdates();
		m_updateInstancesProcessing->Release();
		m_updateInstancesProcessing = nullptr;
	}
}

void CRenderProxy_SpeedTree::CheckUpdateFoliageRenderParams()
{
	Bool performUpdate = false;		// Don't update render params if there's no need to

	if( Config::cvFoliageDistanceScale.Get() != m_currentFoliageDistanceScaleParam )
	{
		performUpdate = true;
	}

	if( Config::cvGrassDistanceScale.Get() != m_currentGrassDistanceScaleParam )
	{
		performUpdate = true;
	}

	if( performUpdate == true )
	{
		if( m_currentGrassDistanceScaleParam > 0.0f && m_currentFoliageDistanceScaleParam > 0.0f )
		{
			SFoliageRenderParams params;
			params.m_foliageDistShift = 1.0f / m_currentFoliageDistanceScaleParam;
			params.m_grassDistScale = 1.0f / m_currentGrassDistanceScaleParam;
			UpdateFoliageRenderParams( params );

			m_currentFoliageDistanceScaleParam = Config::cvFoliageDistanceScale.Get();
			m_currentGrassDistanceScaleParam = Config::cvGrassDistanceScale.Get();

			params.m_foliageDistShift = m_currentFoliageDistanceScaleParam;
			params.m_grassDistScale = m_currentGrassDistanceScaleParam;
			UpdateFoliageRenderParams( params );
		}

		for ( CTreeRender* treeRender : m_cachedBaseTrees )
		{
			if ( treeRender )
			{
				treeRender->UpdateBaseTreeCBDataBasedOnLodProfile();
			}
		}

		for( CTreeRender* grassTreeRender : m_cachedBaseGrass )
		{
			if ( grassTreeRender )
			{
				grassTreeRender->UpdateBaseTreeCBDataBasedOnLodProfile();
			}
		}
	}
}

void CRenderProxy_SpeedTree::ReinitializeRingBuffer()
{
	m_grassInstancesRingBuffer.ReleaseBuffer();
	m_grassInstancesRingBuffer.SetRingBufferSize( Config::cvGrassRingSize.Get() );
}

CUpdateTreeInstancesProcessing::CUpdateTreeInstancesProcessing(CRenderProxy_SpeedTree* renderProxy, SFoliageUpdateRequest updates)
	: m_renderProxy( renderProxy )
	, m_updates( std::move(updates) )
{

}

CUpdateTreeInstancesProcessing::~CUpdateTreeInstancesProcessing()
{

}

#ifndef NO_DEBUG_PAGES
const Char* CUpdateTreeInstancesProcessing::GetDebugName() const
{
	return TXT( "CUpdateTreeInstancesTask" );
}

Uint32 CUpdateTreeInstancesProcessing::GetDebugColor() const
{
	return Color::DARK_BLUE.ToUint32();
}

#endif // NO_DEBUG_PAGES

void CUpdateTreeInstancesProcessing::Run()  
{
	PC_SCOPE( AddRemoveSptInstances );
	const FoliageRemoveInstancesContainer & removeInstanceContainer = m_updates.removeRequestContainer; 
	CRenderProxy_SpeedTree& renderProxy = (*m_renderProxy);

	for( auto iter = removeInstanceContainer.Begin(), end = removeInstanceContainer.End(); iter != end; ++iter )
	{
		RenderObjectHandle tree = iter->m_first;
		const Box & box = iter->m_second;
		renderProxy.RemoveStaticInstances( tree.Get(), box );
	}

	const FoliageAddInstancesContainer & addInstanceContainer = m_updates.addRequestContainer;
	for( auto iter = addInstanceContainer.Begin(), end = addInstanceContainer.End(); iter != end; ++iter )
	{
		RenderObjectHandle tree = iter->tree;
		const FoliageInstanceContainer & instanceContainer = iter->instances;
		const Box & box = iter->box; 
		renderProxy.AddStaticInstances( tree.Get(), instanceContainer, box );
	}
}

void CUpdateTreeInstancesProcessing::FinalizeTreeInstanceUpdates()
{
	if(m_renderProxy->m_UseMultiThreadedTrees)
	{
		CTaskDispatcher taskDispatcher( *GTaskManager );
		CTaskRunner taskRunner;
		taskRunner.RunTask( *this, taskDispatcher );
		
		while( !this->IsFinished() ){RED_BUSY_WAIT();}
	}
}

CTreeInstanceProcessing::CTreeInstanceProcessing(CRenderProxy_SpeedTree* renderProxy, Uint32 foliageType, const TreeRenderContainer& trees)
	: m_pBillboardBufferBase( nullptr )
	, m_pUpdatedBillboardBufferBase( nullptr )
{
	m_foliageType = foliageType;
	m_renderProxy = renderProxy;
	m_trees = trees;
	m_nBillboardBlockHandle = -1;
	m_nUpdatedBillboardBlockHandle = -1;
}

CTreeInstanceProcessing::~CTreeInstanceProcessing()
{

}

TreeRenderContainer& CTreeInstanceProcessing::GetTrees()
{
	return m_trees;
}

void CTreeInstanceProcessing::Run()
{
	PC_SCOPE_PIX( CTreeInstanceProcessing );

	m_renderProxy->ProcessTreeBuffers(m_trees, m_foliageType);
}


void CTreeInstanceProcessing::FinalizeTreeInstanceProcessing()
{
	if(m_renderProxy->m_UseMultiThreadedTrees)
	{
		CTaskDispatcher taskDispatcher( *GTaskManager );
		CTaskRunner taskRunner;
		taskRunner.RunTask( *this, taskDispatcher );

		while( !this->IsFinished() ){RED_BUSY_WAIT();}
	}
}


#ifndef NO_DEBUG_PAGES
const Char* CTreeInstanceProcessing::GetDebugName() const
{
	return TXT( "CTreeInstanceProcessingTask" );
}

Uint32 CTreeInstanceProcessing::GetDebugColor() const
{
	return Color::LIGHT_BLUE.ToUint32();
}

#endif // NO_DEBUG_PAGES

CGrassInstanceProcessing::CGrassInstanceProcessing(CRenderProxy_SpeedTree* renderProxy)
{
	m_renderProxy = renderProxy;
}

CGrassInstanceProcessing::~CGrassInstanceProcessing()
{

}

void CGrassInstanceProcessing::Run()
{
	m_renderProxy->ProcessAllGrassLayers();
}


void CGrassInstanceProcessing::FinalizeGrassInstanceUpdates()
{
	if( PARALLEL_FOLIAGE_PROCESSING )
	{
		CTaskDispatcher taskDispatcher( *GTaskManager );
		CTaskRunner taskRunner;
		taskRunner.RunTask( *this, taskDispatcher );

		while( !this->IsFinished() ){RED_BUSY_WAIT();}
	}
}


#ifndef NO_DEBUG_PAGES
const Char* CGrassInstanceProcessing::GetDebugName() const
{
	return TXT( "CTreeInstanceProcessingTask" );
}

Uint32 CGrassInstanceProcessing::GetDebugColor() const
{
	return Color::LIGHT_BLUE.ToUint32();
}

#endif // NO_DEBUG_PAGES

#endif
