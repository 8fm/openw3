/*
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "soundAmbientAreaComponent.h"

#ifndef NO_EDITOR

#include "../../common/core/mathUtils.h"
#include "../../common/engine/clipMap.h"
#include "../../common/engine/entityTemplate.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/streamingSectorData.h"
#include "../../common/engine/sectorDataStreaming.h"
#include "../../common/engine/soundAmbientAreaComponent.h"
#include "../../common/engine/foliageEditionController.h"
#include "../../common/engine/foliageCell.h"

void CSoundAmbientAreaComponent::GenerateWaterAreas( )
{
	const CWorld* world = GetWorld( );
	const CClipMap* clipMap = world->GetTerrain( );

	const Float worldDimension = world->GetWorldDimensions( );
	const Float gridCellDimension = worldDimension / static_cast< Float >( m_waterGridCellCount );

	const Matrix& transform = GetLocalToWorld( );
	const Vector& translation = transform.GetTranslationRef( );
	const Vector scale = transform.GetScale33( );

	Vector minWorldCoord( 99999.9f, 99999.9f, 99999.9f );
	Vector maxWorldCoord( -99999.9f, -99999.9f, -99999.9f );

	// Find current shape bounding box (it could be something else than a box).
	for( const Vector& p : m_localPoints )
	{
		if( p.X < minWorldCoord.X ) minWorldCoord.X = p.X;
		if( p.X > maxWorldCoord.X ) maxWorldCoord.X = p.X;
		if( p.Y < minWorldCoord.Y ) minWorldCoord.Y = p.Y;
		if( p.Y > maxWorldCoord.Y ) maxWorldCoord.Y = p.Y;
	}

	// Get transformed bounding box limits in world coordinates.
	minWorldCoord = translation + minWorldCoord * scale;
	maxWorldCoord = translation + maxWorldCoord * scale;

	// Make the region start from an exact point in the grid.
	minWorldCoord.X = Red::Math::MFloor( minWorldCoord.X / gridCellDimension ) * gridCellDimension;
	minWorldCoord.Y = Red::Math::MFloor( minWorldCoord.Y / gridCellDimension ) * gridCellDimension;

	SClipmapParameters clipParams;
	clipMap->GetClipmapParameters( &clipParams );

	struct SMapCell
	{
		Float	m_terrainLevel;
		Float	m_waterLevel;
		Uint8	m_isWater;
		Uint32	m_areaId;

		SMapCell( ) : m_terrainLevel( -30.0f ), m_waterLevel( -30.f ), m_isWater( 0 ), m_areaId( 0 ) { }
		SMapCell( Float terrainLevel, Float waterLevel, Bool isWater ) : m_terrainLevel( terrainLevel ), m_waterLevel( waterLevel ), m_isWater( isWater ), m_areaId( 0 ) { }
	};

	TDynArray< TDynArray< SMapCell > > waterMap;
	waterMap.Reserve( m_waterGridCellCount );

	// Create a sampled binary map (land/water) taking into account only the samples inside the area.
	for( Float y = minWorldCoord.Y; y < maxWorldCoord.Y; y += gridCellDimension )
	{
		waterMap.Resize( waterMap.Size() + 1 );
		TDynArray< SMapCell >& waterMapRow = waterMap.Back();
		waterMapRow.Reserve( m_waterGridCellCount );

		for( Float x = minWorldCoord.X; x < maxWorldCoord.X; x += gridCellDimension )
		{
			const Vector position = Vector( x, y, clipParams.highestElevation + 1 );

			Float height = -10.0f;
			if( clipMap->GetControlmapValueForWorldPosition( position ) > 0 )
				clipMap->GetHeightForWorldPosition( position, height );

			const Float waterLevel = world->GetWaterLevel( position, 0 ) + m_waterLevelOffset;
			const Bool isInsideArea = MathUtils::GeometryUtils::IsPointInPolygon2D( reinterpret_cast< Vector* >( m_localPoints.Data( ) ), m_localPoints.Size( ), ( position - translation ) / scale );
			waterMapRow.PushBack( SMapCell( height, waterLevel, height <= waterLevel && isInsideArea ) );
		}
	}

	// Set map borders to solid. Makes area finding easier.
	for( Uint32 x = 0; x < waterMap[ 0 ].Size(); ++x )
	{
		waterMap[ 0 ][ x ].m_isWater = 0;
		waterMap[ waterMap.Size()-1 ][ x ].m_isWater = 0;
	}
	for( Uint32 y = 0; y < waterMap.Size(); ++y )
	{
		waterMap[ y ][ 0 ].m_isWater = 0;
		waterMap[ y ][ waterMap[ 0 ].Size()-1 ].m_isWater = 0;
	}

	struct SMapCellCoord
	{
		Int32 m_x, m_y;
		Float m_z;
		SMapCellCoord( ) : m_x( 0 ), m_y( 0 ), m_z( 0.0f ) { }
		SMapCellCoord( Int32 x, Int32 y ) : m_x( x ), m_y( y ), m_z( 0.0f ) { }
		SMapCellCoord( Int32 x, Int32 y, Float z ) : m_x( x ), m_y( y ), m_z( z ) { }
	};

	// Lookup relative coordinates used to follow region boundaries.
	const Uint32 rotOffsetsCount = 8;
	const SMapCellCoord rotOffsets[ rotOffsetsCount ] =
	{
		SMapCellCoord( +1, +1 ), SMapCellCoord(  0, +1 ), SMapCellCoord( -1, +1 ), SMapCellCoord( -1,  0 ),
		SMapCellCoord( -1, -1 ), SMapCellCoord(  0, -1 ), SMapCellCoord( +1, -1 ), SMapCellCoord( +1,  0 )
	};

	typedef TDynArray< SMapCellCoord > TContour;

	struct SMapArea
	{
		TContour m_contour;
		SMapCellCoord m_minCoord, m_maxCoord;
		SMapArea( ) : m_minCoord( SMapCellCoord( 99999, 99999, 99999.9f ) ), m_maxCoord( SMapCellCoord( -99999, -99999, -99999.9f ) ) { }
		void AddCoord( const SMapCellCoord& coord )
		{
			if( coord.m_x < m_minCoord.m_x ) m_minCoord.m_x = coord.m_x;
			if( coord.m_y < m_minCoord.m_y ) m_minCoord.m_y = coord.m_y;
			if( coord.m_z < m_minCoord.m_z ) m_minCoord.m_z = coord.m_z;
			if( coord.m_x > m_maxCoord.m_x ) m_maxCoord.m_x = coord.m_x;
			if( coord.m_y > m_maxCoord.m_y ) m_maxCoord.m_y = coord.m_y;
			if( coord.m_z > m_maxCoord.m_z ) m_maxCoord.m_z = coord.m_z;
			m_contour.PushBack( coord );
		}
		void RemoveLastCoord( )
		{
			m_contour.Resize( m_contour.Size() - 1 );
		}
	};

	TDynArray< SMapArea > mapAreas;

	// Inspect all cells looking for all unique water areas in the sampled region.
	for( Uint32 y = 1; y < waterMap.Size() - 1; ++y )
	{
		for( Uint32 x = 1; x < waterMap[ y ].Size() - 1; ++x )
		{
			// Non-processed boundary cell.
			if( !waterMap[ y ][ x ].m_isWater && waterMap[ y ][ x+1 ].m_isWater && waterMap[ y ][ x ].m_areaId == 0 )
			{
				Int32 testCellIdx = -1;

				// Find first next cell.
				for( Uint32 i = 0; i < rotOffsetsCount; ++i )
				{
					const SMapCell& mapCell = waterMap[ y + rotOffsets[ i ].m_y ][ x + rotOffsets[ i ].m_x ];
					if( !mapCell.m_isWater )
					{
						testCellIdx = i;
						break;
					}
				}

				// Found a one-cell island.
				if( testCellIdx < 0 )
					continue;

				// Create a new area contour and add first vertex.
				mapAreas.Resize( mapAreas.Size() + 1 );
				SMapArea& currArea = mapAreas.Back( );
				currArea.AddCoord( SMapCellCoord( x, y, waterMap[ y ][ x ].m_terrainLevel ) );

				// Assign unique area id.
				const Uint32 currentAreaId = mapAreas.Size();
				waterMap[ y ][ x ].m_areaId = currentAreaId;

				// Next coordinates to be tested.
				Int32 testX = x + rotOffsets[ testCellIdx ].m_x;
				Int32 testY = y + rotOffsets[ testCellIdx ].m_y;

				// Curve orientation is determined by sum over the edges: (x2-x1)(y2+y1).
				// If result is + then clockwise (island), else counter-clockwise (lake).
				Int32 clockWiseFactor = ( testX - x ) * ( testY + y );
				Int32 prevX = testX, prevY = testY;

				Int32 prevTestCell = -1;

				while( !( testX == x && testY == y ) )
				{
					waterMap[ testY ][ testX ].m_areaId = currentAreaId;

					// Make one big chunk out of consecutive similar ones.
					if( testCellIdx == prevTestCell )
						currArea.RemoveLastCoord();
					prevTestCell = testCellIdx;

					currArea.AddCoord( SMapCellCoord( testX, testY, waterMap[ testY ][ testX ].m_terrainLevel ) );

					// Find first next cell (starting from the previous water match).
					testCellIdx = ( testCellIdx + 4 + 1 ) % rotOffsetsCount;
					for( Uint32 i = 0; i < rotOffsetsCount; ++i )
					{
						const SMapCell& nextMapCell = waterMap[ testY + rotOffsets[ testCellIdx ].m_y ][ testX + rotOffsets[ testCellIdx ].m_x ];
						if( !nextMapCell.m_isWater )
						{
							testX += rotOffsets[ testCellIdx ].m_x;
							testY += rotOffsets[ testCellIdx ].m_y;
							break;
						}
						testCellIdx = ( testCellIdx + 1 ) % rotOffsetsCount;
					}

					clockWiseFactor += ( testX - prevX ) * ( testY + prevY );
					prevX = testX; prevY = testY;
				}

				// Found an island.
				if( clockWiseFactor < 0 )
				{
					mapAreas.Resize( mapAreas.Size() - 1 );
				}
			}
		}
	}

	// Create the areas (we only want one here, this is a bad approach).
	for( Uint32 areaIdx = 0; areaIdx < mapAreas.Size(); ++areaIdx )
	{
		const SMapArea& currArea = mapAreas[ areaIdx ];
		const TContour& currContour = currArea.m_contour;

		CAreaComponent::TAreaPoints areaPoints;

		const Vector areaCellOffset = Vector( -static_cast< Float >( waterMap[0].Size() ) / 2.0f, -static_cast< Float >( waterMap.Size() ) / 2.0f, 0 );

		// This will probably slow down the process, but makes the code more readable.
		TDynArray< Vector > finalPoints;

		// Generate final points and determine center of mass.
		Vector spawnPosition( 0.0f, 0.0f, 0.0f );
		for( Uint32 pIdx = 0; pIdx < currContour.Size( ); ++pIdx )
		{
			Vector p( static_cast< Float >( currContour[ pIdx ].m_x ), static_cast< Float >( currContour[ pIdx ].m_y ), 0.0f );
			p += areaCellOffset;
			p = p * gridCellDimension / scale;
			p.SetZ( m_localPoints[ 0 ].Z );
			p.SetW( 1.0f );

			// We could also follow the height of each vertex like this.
			//p.SetZ( currContour[ pIdx ].m_z / scale.Z );
			//p.SetZ( currContour[ pIdx ].m_z > 0 ? currContour[ pIdx ].m_z / scale.Z : 0 );

			finalPoints.PushBack( p );
			spawnPosition += p / static_cast< Float >( currContour.Size( ) );
		}

		// Points are added in reverse direction to account for sidedness.
		for( Int32 pIdx = finalPoints.Size() - 1; pIdx >= 0; --pIdx )
			areaPoints.PushBack( finalPoints[ pIdx ] - spawnPosition );

		// Create a new entity for every new area.
		CEntity* entity = SafeCast< CEntity >( GetEntity( )->Clone( GetParent( ), false, false ) );
		if( entity == nullptr )
			continue;

		CSoundAmbientAreaComponent* component = entity->FindComponent< CSoundAmbientAreaComponent >( );
		if( component == nullptr )
		{
			entity->Discard( );
			continue;
		}

		// Parameters are inherited from current area, except for its name and position.
		entity->SetName( String::Printf( TXT( "%ls_auto_%d" ), entity->GetName( ).AsChar( ), areaIdx ) );
		entity->SetPosition( translation + spawnPosition * scale );

		GetLayer( )->AddEntity( entity );

		component->SetLocalPoints( areaPoints, true );

		// And recompute the outer shape.
		component->UnregisterAudioSource( );
		component->RecreateOuterShape( );
		component->RegisterAudioSource( );
	}

	// The flag is kept down in the source area.
	m_fitWaterShore = false;

	// Log into the console the ASCII map.
	/*
	RED_LOG( WaterFitter, TXT( "--- Water map start ---" ) );
	for( auto mapRow : waterMap )
	{
		String mapRowStr = String::EMPTY;
		for( auto mapCell : mapRow )
		{
			mapRowStr += mapCell.m_isWater ? TXT( " " ) : ( mapCell.m_areaId == 0 ? TXT( "X" ) : String::Printf( TXT( "%d" ), mapCell.m_areaId % 10 ) );
		}
		RED_LOG( WaterFitter, TXT( "%ls" ), mapRowStr.AsChar() );
	}
	RED_LOG( WaterFitter, TXT( "---- Water map end ----" ) );
	*/
}

void CSoundAmbientAreaComponent::GenerateFoliageAreas( )
{
	CWorld* world = GetWorld( );

	CFoliageEditionController& controller = world->GetFoliageEditionController( );
	controller.WaitUntilAllFoliageResourceLoaded( );

	const Matrix& transform = GetLocalToWorld( );
	const Vector& translation = transform.GetTranslationRef( );
	const Vector scale = transform.GetScale33( );

	Box boundingBox;
	boundingBox.Clear( );
	for( const Vector& v : m_localPoints )
		boundingBox.AddPoint( translation + v * scale );

	CellHandleContainer foliageHandle;
	controller.AcquireAndLoadFoliageAtArea( boundingBox, foliageHandle );
	if( !foliageHandle.Empty( ) )
	{
		const Float maxSquaredDistanceFromTreeToTree = m_foliageMaxDistance * m_foliageMaxDistance;

		typedef TDynArray< Vector > TTreeArea;
		TDynArray< TTreeArea > treeAreas;

		// Build groups of trees based on mutual distances.
		for( auto it = foliageHandle.Begin( ); it != foliageHandle.End( ); ++it )
		{
			const CFoliageCell* foliageCel = it->Get( );
			if( !foliageCel->IsResourceValid( ) )
				continue;

			const CFoliageResource* foliageResource = foliageCel->GetFoliageResource( );
			if( foliageResource == nullptr )
				continue;

			const CFoliageResource::InstanceGroupContainer& treeGroup = foliageResource->GetAllTreeInstances( );
			for( auto group = treeGroup.Begin( ); group != treeGroup.End( ); ++group )
			{
				const FoliageInstanceContainer& trees = group->instances;
				for( auto tree = trees.Begin( ); tree != trees.End( ); ++tree )
				{
					const SFoliageInstance& instance = *tree;
					//const Vector treePosition = instance.GetPosition( );
					const Vector treePosition = Vector( instance.GetPosition( ).X, instance.GetPosition( ).Y, 0.0f );
					if( !MathUtils::GeometryUtils::IsPointInPolygon2D( reinterpret_cast< Vector* >( m_localPoints.Data( ) ), m_localPoints.Size( ), ( treePosition - translation ) / scale ) )
						continue;

					Int32 bestAreaIdx = -1;
					for( Uint32 areaIdx = 0; areaIdx < treeAreas.Size( ); ++areaIdx )
					{
						TTreeArea& area = treeAreas[ areaIdx ];
						for( Uint32 treeIdx = 0; treeIdx < area.Size( ); ++treeIdx )
						{
							// The new tree is inside this group.
							if( area[ treeIdx ].DistanceSquaredTo( treePosition ) <= maxSquaredDistanceFromTreeToTree )
							{
								// We didn't have a best group yet, take note of it.
								if( bestAreaIdx < 0 )
								{
									bestAreaIdx = areaIdx;
									break;
								}

								// We already found a group for this tree, but it is also close to this other area.
								// This means these two areas are connected by this new tree, so we have to merge them.
								else if( bestAreaIdx != areaIdx )
								{
									treeAreas[ bestAreaIdx ].PushBack( treeAreas[ areaIdx ] );
									treeAreas[ areaIdx ].ClearFast( );
									break;
								}
							}
						}
					}

					// There was no area close enough to the tree. Create a new one.
					if( bestAreaIdx < 0 )
					{
						treeAreas.Resize( treeAreas.Size( ) + 1 );
						treeAreas.Back( ).PushBack( treePosition );
					}
					else
						treeAreas[ bestAreaIdx ].PushBack( treePosition );
				}
			}
		}

		// Apply vital area beveling if specified.
		if( m_foliageVitalAreaRadius > 0.0f && m_foliageVitalAreaPoints > 1 )
		{
			const Float treeVitalAreaRotStep = ( M_PI * 2.0f ) / static_cast< Float >( m_foliageVitalAreaPoints );
			TDynArray< Vector > treeVitalAreaSteps;
			Vector treeVitalAreaStep( m_foliageVitalAreaRadius, 0, 0 );

			// Precompute directions.
			for( Uint32 step = 0; step < m_foliageVitalAreaPoints; ++step )
			{
				treeVitalAreaSteps.PushBack( treeVitalAreaStep );
				treeVitalAreaStep = MathUtils::GeometryUtils::Rotate2D( treeVitalAreaStep, treeVitalAreaRotStep );
			}

			// And replace all points by the surrounding ones.
			for( Uint32 areaIdx = 0; areaIdx < treeAreas.Size( ); ++areaIdx )
			{
				TDynArray< Vector > newArea;
				for( const Vector& p : treeAreas[ areaIdx ] )
				{
					for( const Vector& vitalAreaStep : treeVitalAreaSteps )
					{
						newArea.PushBack( Vector2( p + vitalAreaStep ) );
					}
				}
				treeAreas[ areaIdx ].SwapWith( newArea );
			}
		}

		Uint32 generatedAreaIdx = 0;

		// Now build an ambient area for each group of trees.
		for( Uint32 areaIdx = 0; areaIdx < treeAreas.Size( ); ++areaIdx )
		{
			RED_LOG( FoliageFitter, TXT( "AREA #%d has %d trees." ), areaIdx, treeAreas[ areaIdx ].Size( ) );

			TDynArray< Vector2 > currArea;
			for( const Vector2 v : treeAreas[ areaIdx ] )
				currArea.PushBack( v );

			TDynArray< Vector2 > areaHull;

			if( MathUtils::GeometryUtils::ComputeConcaveHull2D( currArea, areaHull, m_foliageStepNeighbors ) )
			{
				TDynArray< Vector > areaPoints;

				// Determine center of mass.
				Vector spawnPosition( 0.0f, 0.0f, 0.0f );
				for( Uint32 i = 0; i < areaHull.Size( ); ++i )
					spawnPosition += areaHull[ i ] / static_cast< Float >( areaHull.Size( ) );

				// Scale all points.
				for( Uint32 i = 0; i < areaHull.Size( ); ++i )
					areaPoints.PushBack( ( Vector( areaHull[ i ] ) - Vector( spawnPosition.X, spawnPosition.Y, 0 ) ) / scale );

				// Create a new entity for every new area.
				CEntity* entity = SafeCast< CEntity >( GetEntity( )->Clone( GetParent( ), false, false ) );
				if( entity == nullptr )
					continue;

				CSoundAmbientAreaComponent* component = entity->FindComponent< CSoundAmbientAreaComponent >( );
				if( component == nullptr )
				{
					entity->Discard( );
					continue;
				}

				// Parameters are inherited from current area, except for its name and position.
				entity->SetName( String::Printf( TXT( "%ls_auto_%d" ), entity->GetName( ).AsChar( ), generatedAreaIdx ) );
				entity->SetPosition( spawnPosition );

				GetLayer( )->AddEntity( entity );

				component->SetLocalPoints( areaPoints, true );

				// And recompute the outer shape.
				component->UnregisterAudioSource( );
				component->RecreateOuterShape( );
				component->RegisterAudioSource( );

				++generatedAreaIdx;
			}
			else
			{
				RED_LOG( FoliageFitter, TXT( "Failed to generate a valid concave hull. :(" ) );
			}
		}
	}

	// The flag is kept down in the source area.
	m_fitFoliage = false;
}

void CSoundAmbientAreaComponent::RestoreAreaToDefaultBox( )
{
	// Computes bounding box boundaries.
	Vector minCoord( 99999.9f, 99999.9f, 99999.9f );
	Vector maxCoord( -99999.9f, -99999.9f, -99999.9f );
	for( const Vector& v : m_localPoints )
	{
		if( v.X < minCoord.X ) minCoord.X = v.X;
		if( v.Y < minCoord.Y ) minCoord.Y = v.Y;
		if( v.Z < minCoord.Z ) minCoord.Z = v.Z;
		if( v.X > maxCoord.X ) maxCoord.X = v.X;
		if( v.Y > maxCoord.Y ) maxCoord.Y = v.Y;
		if( v.Z > maxCoord.Z ) maxCoord.Z = v.Z;
	}

	// Adds extra space to compensate sampling.
	if( const CWorld* world = GetWorld( ) )
	{
		const Float worldDimension = world->GetWorldDimensions( );
		const Float gridCellDimension = worldDimension / 256.0f;

		const Matrix& transform = GetLocalToWorld( );
		const Vector invScale = transform.GetScale33( );

		const Vector extraOffset = Vector( gridCellDimension, gridCellDimension, 0.0f ) * 0.5f / invScale;

		// Make sure to include bordering points.
		minCoord -= extraOffset;
		maxCoord += extraOffset;
	}

	// Build a squared bounding box area that includes all current points.
	CAreaComponent::TAreaPoints areaPoints;
	areaPoints.PushBack( Vector( minCoord.X, minCoord.Y, maxCoord.Z ) );
	areaPoints.PushBack( Vector( maxCoord.X, minCoord.Y, maxCoord.Z ) );
	areaPoints.PushBack( Vector( maxCoord.X, maxCoord.Y, maxCoord.Z ) );
	areaPoints.PushBack( Vector( minCoord.X, maxCoord.Y, maxCoord.Z ) );

	SetLocalPoints( areaPoints, true );

	// And recompute the outer shape.
	UnregisterAudioSource( );
	RecreateOuterShape( );
	RegisterAudioSource( );
}

#endif