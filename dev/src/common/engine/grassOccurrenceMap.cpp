/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "grassOccurrenceMap.h"
#include "clipMap.h"
#include "vegetationBrush.h"
#include "baseTree.h"
#include "terrainTile.h"
#include "../renderer/speedTreeDefines.h"
#include "../renderer/speedTreeLinkage.h"
#include <Core/Core.h>

IMPLEMENT_ENGINE_CLASS( CGrassOccurrenceMap );

#ifndef NO_RESOURCE_COOKING

namespace 
{

struct SRTInfo
{
	Bool	m_isGrass;
	Vector	m_diagonalExtents;
};

SpeedTree::CTree* ParseSpeedTreeObject( const CSRTBaseTree* baseTree )
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

}

class GrassOccurenceMapGenerator
{
private:
	struct GrassDesc
	{
		String		fileName;
		Float		cellSize;

		friend Bool operator==( const GrassDesc& lhs, const GrassDesc& rhs )
		{
			return lhs.cellSize == rhs.cellSize && lhs.fileName == rhs.fileName;
		}
	};

	struct BrushDesc
	{
		TDynArray< Uint32 >	grassTypesIndices;
		Bool IsValid() const { return grassTypesIndices.Size() > 0; }
	};

	TDynArray< BrushDesc >		m_brushes;
	TDynArray< GrassDesc >		m_grassTypes;
	TDynArray< CGrassCellMask >	m_cellMasks;

public:
	GrassOccurenceMapGenerator();

	//! For a clipmap, prepare a set of per-type grass Occurrence Maps.
	CGrassOccurrenceMap* CookGrassOccurenceMaps( const CClipMap* clipmap );

private:
	void CollectGrassTypesToProcess( const CClipMap* clipmap, TDynArray< GrassDesc >& outTypes, TDynArray< BrushDesc >& outBrushes );
	Bool ProcessTile( const CClipMap* clipmap, const SClipmapParameters& clipmapParams, CTerrainTile* tile, Uint32 row, Uint32 col );
	void ProcessPosition( const Vector& position, Uint32 texIndex );
};


GrassOccurenceMapGenerator::GrassOccurenceMapGenerator()
{
}

CGrassOccurrenceMap* GrassOccurenceMapGenerator::CookGrassOccurenceMaps( const CClipMap* clipmap )
{
	CollectGrassTypesToProcess( clipmap, m_grassTypes, m_brushes );

	const Uint32 numGrassTypes = m_grassTypes.Size();
	if ( numGrassTypes == 0 )
	{
		// No types but that's fine ...
		RED_LOG( RED_LOG_CHANNEL( Foliage ), TXT("No grass types to cook occurrence maps for.") );
		return nullptr;
	}

	SClipmapParameters terrainParams;
	clipmap->GetClipmapParameters( &terrainParams );

	const Float terrainLength = terrainParams.terrainSize;
	const Uint32 terrainResolution = terrainParams.clipmapSize;
	const Vector terrainCorner = clipmap->GetTerrainCorner();
	const Vector terrainOppositeCorner = terrainCorner + Vector( terrainLength, terrainLength, terrainCorner.Z );
	const Uint32 numTilesOnEdge = clipmap->GetNumTilesPerEdge();

	// Allocate an array of occurrence maps	
	m_cellMasks.Resize( numGrassTypes );
	for ( Uint32 g=0; g<numGrassTypes; ++g )
	{
		CGrassCellMask& cellMask = m_cellMasks[g];

		const Float cellSize = m_grassTypes[g].cellSize;
		cellMask.m_cellSize = cellSize;
		cellMask.m_srtFileName = m_grassTypes[g].fileName;

		cellMask.m_firstRow = (terrainCorner.Y < 0.0f) ? Int32((terrainCorner.Y - cellSize) / cellSize) : Int32(terrainCorner.Y / cellSize);
		cellMask.m_lastRow = (terrainOppositeCorner.Y < 0.0f) ? Int32((terrainOppositeCorner.Y - cellSize) / cellSize) : Int32(terrainOppositeCorner.Y / cellSize);

		cellMask.m_firstCol = (terrainCorner.X < 0.0f) ? Int32((terrainCorner.X - cellSize) / cellSize) : Int32(terrainCorner.X / cellSize);
		cellMask.m_lastCol = (terrainOppositeCorner.X < 0.0f) ? Int32((terrainOppositeCorner.X - cellSize) / cellSize) : Int32(terrainOppositeCorner.X / cellSize);

		cellMask.Allocate();
	}

	RED_LOG( RED_LOG_CHANNEL( Foliage ), TXT("Cooking automatic grass occurrence maps...") );

	const Uint32 tilesToProcess = numTilesOnEdge * numTilesOnEdge;
	Uint32 currentTile = 0;
	for ( Uint32 row=0; row<numTilesOnEdge; ++row )
	{
		for ( Uint32 col=0; col<numTilesOnEdge; ++col )
		{
			CTerrainTile* tile = clipmap->GetTile( col, row );
			if ( tile )
			{
				RED_LOG( RED_LOG_CHANNEL( Foliage ), TXT("Processing terrain tile %i out of %i"), currentTile++, tilesToProcess );
				ProcessTile( clipmap, terrainParams, tile, row, col );
			}
		}
	}

	CGrassOccurrenceMap* grassOccurrenceMap = CreateObject< CGrassOccurrenceMap >();
	grassOccurrenceMap->SetCellMasks( m_cellMasks );

	return grassOccurrenceMap;
}

void GrassOccurenceMapGenerator::CollectGrassTypesToProcess( const CClipMap* clipmap, TDynArray< GrassDesc >& outTypes, TDynArray< BrushDesc >& outBrushes )
{
	outTypes.Clear();
	outBrushes.Clear();
	outBrushes.Resize( NUM_TERRAIN_TEXTURES_AVAILABLE );
	for ( Uint32 t=0; t<NUM_TERRAIN_TEXTURES_AVAILABLE; ++t )
	{
		const CVegetationBrush* brush = clipmap->GetGrassBrush( t );
		if ( brush )
		{
			TDynArray< CVegetationBrushEntry* > brushEntries;
			brush->GetEntries( brushEntries );

			for ( Uint32 be=0; be<brushEntries.Size(); ++be )
			{
				const CVegetationBrushEntry* entry = brushEntries[be];
				if ( entry )
				{
					const CSRTBaseTree* baseTree = entry->GetBaseTree();
					ASSERT( baseTree );
					ASSERT( baseTree->GetSRTData() );
					ASSERT( baseTree->GetSRTDataSize() > 0 );

					const String& fileName = baseTree->GetFile()->GetFileName();
					SRTInfo srtInfo;

					if ( GetTreeInfo( baseTree, srtInfo ) && srtInfo.m_isGrass )
					{
						// MIMIC THE CELL SIZE COMPUTATION MADE BY RENDERER
						Float meshRadius = Max<Float>( srtInfo.m_diagonalExtents.X, srtInfo.m_diagonalExtents.Y ) * ( 1.0f / MSqrt( 2.0f ) );

						Float cellSize = MAX_GRASS_INSTANCES_ON_CELL_EDGE * meshRadius;
						cellSize = cellSize < GRASS_CELL_SIZE_MIN ? GRASS_CELL_SIZE_MIN : cellSize;
						cellSize = cellSize > GRASS_CELL_SIZE_MAX ? GRASS_CELL_SIZE_MAX : cellSize;

						GrassDesc desc;
						desc.cellSize = cellSize;
						desc.fileName = fileName;

						ptrdiff_t index = outTypes.GetIndex( desc );
						if ( index == -1 )
						{
							outTypes.PushBack( desc );
							index = outTypes.GetIndex( desc );
						}
						RED_ASSERT( index != -1 );

						outBrushes[t].grassTypesIndices.PushBackUnique( (Uint32)index );
					}
				}
			}
		}
	}
}

Bool GrassOccurenceMapGenerator::ProcessTile( const CClipMap* clipmap, const SClipmapParameters& clipmapParams, CTerrainTile* tile, Uint32 row, Uint32 col )
{
	Box tile2DBB = clipmap->GetBoxForTile( col, row, 0 );

	const Float terrainLength = clipmapParams.terrainSize;
	const Uint32 terrainResolution = clipmapParams.clipmapSize;
	const Vector terrainCorner = clipmap->GetTerrainCorner();
	const Vector terrainOppositeCorner = terrainCorner + Vector( terrainLength, terrainLength, terrainCorner.Z );
	const Uint32 numTilesOnEdge = clipmap->GetNumTilesPerEdge();
	const Uint32 tileRes = clipmapParams.tileRes;

	const TControlMapType* cmData = tile->GetLevelSyncCM( 0 );

	const Float terrainStep = terrainLength / terrainResolution;
	Vector position = tile2DBB.Min;

	for ( Uint32 y=0; y<tileRes; ++y, position.Y += terrainStep )
	{
		for ( Uint32 x=0; x<tileRes; ++x, position.X += terrainStep )
		{
			const TControlMapType& val = cmData[ y * tileRes + x ];

			const Uint16 actualHorizontalTexIndex = ( val & 31 ) - 1;
			const Uint16 actualVerticalTexIndex = ( ( val >> 5 ) & 31 ) - 1;

			if ( actualHorizontalTexIndex > NUM_TERRAIN_TEXTURES_AVAILABLE ) continue;
			if ( actualVerticalTexIndex > NUM_TERRAIN_TEXTURES_AVAILABLE ) continue;

			RED_ASSERT( actualHorizontalTexIndex < m_brushes.Size() );
			RED_ASSERT( actualVerticalTexIndex < m_brushes.Size() );

			ProcessPosition( position, actualHorizontalTexIndex );
			ProcessPosition( position, actualVerticalTexIndex );
		}

		position.X = tile2DBB.Min.X;
	}

	return true;
}

void GrassOccurenceMapGenerator::ProcessPosition( const Vector& position, Uint32 texIndex )
{
	BrushDesc& brushDesc = m_brushes[texIndex];
	TDynArray< Uint32 >& grassTypesIndices = brushDesc.grassTypesIndices;
	const Uint32 numGrassTypesInBrush = grassTypesIndices.Size();
	for ( Uint32 gi=0; gi<numGrassTypesInBrush; ++gi )
	{
		CGrassCellMask& occurrenceMap = m_cellMasks[ grassTypesIndices[gi] ];

		const Float& cellSize = occurrenceMap.m_cellSize;

		const Int32 row = (position.Y < 0.0f) ? Int32((position.Y - cellSize) / cellSize) : Int32(position.Y / cellSize);
		const Int32 col = (position.X < 0.0f) ? Int32((position.X - cellSize) / cellSize) : Int32(position.X / cellSize);

		occurrenceMap.SetBit( row, col );
	}
}

Red::TUniquePtr< CGrassOccurrenceMap > CreateGrassOccurenceMap( const CClipMap * clipMap )
{
	Red::TUniquePtr< CGrassOccurrenceMap > grassOccurenceMap;
	GrassOccurenceMapGenerator generator;
	grassOccurenceMap.Reset( generator.CookGrassOccurenceMaps( clipMap ) );
	return grassOccurenceMap;
}


#endif

CGrassOccurrenceMap::CGrassOccurrenceMap()
{
}

void CGrassOccurrenceMap::SetCellMasks( const TDynArray< CGrassCellMask >& cellMasks )
{
	m_cellMasks = cellMasks;
}

