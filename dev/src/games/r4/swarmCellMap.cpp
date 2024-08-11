#include "build.h"
#include "swarmCellMap.h"
#include "../../common/game/boidAreaComponent.h"
#include "../../common/engine/freeCamera.h"
#include "../../common/core/factory.h"
#include "../../common/core/directory.h"
#include "../../common/physics/physicsWorldUtils.h"
#include "../../common/physics/physicsWorld.h"
#include "../../common/core/mathUtils.h"
#include "../../common/engine/renderFrame.h"

#define LINE_TEST_EPSILON 0.000001f

//////////////////////////////////////////////////////
// CSwarmCellMap
//
IMPLEMENT_ENGINE_CLASS( CSwarmCellMap );

CSwarmCellMap::CSwarmCellMap( )
	: m_cellSize( 1.0f )
	, m_dataSize( 0 )
	, m_dataSizeX( 0 )
	, m_dataSizeY( 0 )
	, m_dataSizeZ( 0 )
	, m_cornerPosition( 0.0f, 0.0f, 0.0f )
	, m_sizeInKbytes( 0.0f )
{
	
}

CSwarmCellMap::~CSwarmCellMap()
{
}

void CSwarmCellMap::Generate( CAreaComponent *const areaComponent, Float cellSize )
{
	if ( areaComponent == nullptr )
	{
		return;
	}
	m_cellSize					= cellSize;
	const Box cellmapBoundingBox	= ComputeCellmapArea( areaComponent->GetBoundingBox(), cellSize );

	// Creating a cell map with the first cell corner on areaBox.Min ( m_cornerPosition )
	// of size m_dataSizeX, m_dataSizeY, m_dataSizeZ with each cell of size m_cellSize
	const Vector3 areaSize = cellmapBoundingBox.CalcSize();
	m_dataSizeX		= ( Int32 ) ( areaSize.X / m_cellSize ) + 1; // +1 to make sure the cell map is bigger that the zone
	m_dataSizeY		= ( Int32 ) ( areaSize.Y / m_cellSize ) + 1;
	m_dataSizeZ		= ( Int32 ) ( areaSize.Z / m_cellSize ) + 1;
	const Int32 dataSizeXY		= m_dataSizeX * m_dataSizeY;
	m_dataSize					= dataSizeXY * m_dataSizeZ;
	m_cornerPosition			= cellmapBoundingBox.Min;

	m_data.Resize( m_dataSize );
	
	//const auto& areaWorldPoints = areaComponent->GetWorldPoints();


	const Float halfCellSize		= m_cellSize * 0.5f;

	CPhysicsWorld* physicsWorld	= nullptr;
	GGame->GetActiveWorld()->GetPhysicsWorld( physicsWorld );
	CPhysicsEngine::CollisionMask includeMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Water ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Foliage ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Door ) );
	
	SPhysicsOverlapInfo info;
	for ( Int32 i = 0; i < m_dataSize; ++i )
	{
		const Int32 index2d		= i % dataSizeXY; // index in the X/Y plane
		const Int32 indexX		= index2d % m_dataSizeX; 
		const Int32 indexY		= index2d / m_dataSizeX; 
		const Int32 indexZ		= i / dataSizeXY;
		Vector3 cellPosition;
		GetCellPositionFromIndex( indexX, indexY, indexZ, cellPosition );
		/*Matrix matrix;
		matrix.SetIdentity();
		matrix.SetTranslation( cellPosition );	*/
		
		// [Step] Set to 0 first   
		m_data.Set( i, 0 );

		// [Step] If cell is outside of area mark is as obstacle
		const Bool insideArea = areaComponent->TestBoxOverlap( Box( cellPosition, halfCellSize) );//MathUtils::GeometryUtils::IsPointInPolygon2D( areaWorldPoints, Vector2( cellPosition ) );
		if ( insideArea == false )
		{
			m_data.Set( i, 1 );
			continue;
		}

		// [Step] if cell is inside physics mark it as obstacle
		if ( physicsWorld->BoxOverlapWithAnyResult( cellPosition, Vector3( halfCellSize, halfCellSize, halfCellSize), Matrix::IDENTITY, includeMask, 0, info ) == TRV_Hit )
		{
			m_data.Set( i, 1 );
		}
	}
	const Float sizeInBytes		= (Float)m_data.DataSize();
	m_sizeInKbytes				= sizeInBytes / 1000.0f;
	const Float sizeInMbytes	= m_sizeInKbytes / 1000.0f;
}

Box CSwarmCellMap::ComputeCellmapArea( const Box& movingZoneArea, Float cellSize )
{
	Box result = movingZoneArea;
	const Float safeAreaMultiplier = cellSize * 2.0f;

	Vector3 areaDiagonal = (movingZoneArea.Max - movingZoneArea.Min);
	Vector3 cornerOffset = areaDiagonal.Normalized();
	cornerOffset.X = MSign( cornerOffset.X ) * safeAreaMultiplier;
	cornerOffset.Y = MSign( cornerOffset.Y ) * safeAreaMultiplier;
	cornerOffset.Z = MSign( cornerOffset.Z ) * safeAreaMultiplier;

	result.Min -= cornerOffset;
	result.Max += cornerOffset;

	return result;
}

Int32 CSwarmCellMap::Get( const Vector3 &position )const
{
	// [ Step ] Computing local position in the axis aligned cell map
	const Vector3 localPosition	= position - m_cornerPosition;
	if ( localPosition.X < 0.0f || localPosition.Y < 0.0f || localPosition.Z < 0.0f )
	{
		// We are outside of the cell map !
		return 1;
	}
	
	// [ Step ] Compute cell map indexes on all axis
	Int32 indexX = (Int32)( localPosition.X / m_cellSize );
	Int32 indexY = (Int32)( localPosition.Y / m_cellSize );
	Int32 indexZ = (Int32)( localPosition.Z / m_cellSize );

	// We are outside of the cell map !
	if ( indexX >= m_dataSizeX || indexY >= m_dataSizeY || indexZ >= m_dataSizeZ )
	{
		return 1;
	}
	const Uint32 index = indexX + indexY * m_dataSizeX + indexZ * m_dataSizeX * m_dataSizeY;
	return m_data.Get( (Uint32)index );
}
void CSwarmCellMap::GetCellPositionFromIndex( Int32 indexX, Int32 indexY, Int32 indexZ, Vector3 &cellPosition )const
{
	// corner position + Index * m_cellSize + half cell size ( to get the center of the cell )
	cellPosition			= m_cornerPosition + Vector3 ( (Float)indexX, (Float)indexY, (Float)indexZ  ) * m_cellSize + Vector3( m_cellSize * 0.5f, m_cellSize * 0.5f, m_cellSize * 0.5f );
}

void CSwarmCellMap::GetCellIndexFromPosition( const Vector3 &cellPosition, Int32 &indexX, Int32 &indexY, Int32 &indexZ )const
{
	const Vector3 localPosition =  cellPosition - m_cornerPosition;
	const Float indexXFloat = localPosition.X / m_cellSize;
	const Float indexYFloat = localPosition.Y / m_cellSize;
	const Float indexZFloat = localPosition.Z / m_cellSize;

	indexX = indexXFloat >= 0.0f ? (Int32)indexXFloat : (Int32)( indexXFloat - 1.0f );
	indexY = indexYFloat >= 0.0f ? (Int32)indexYFloat : (Int32)( indexYFloat - 1.0f );
	indexZ = indexZFloat >= 0.0f ? (Int32)indexZFloat : (Int32)( indexZFloat - 1.0f );
}

void CSwarmCellMap::DebugDisplay( CRenderFrame* frame )const
{
	if ( GGame->IsFreeCameraEnabled() == false )
	{
		return;
	}

	const Float halfCellSize		= m_cellSize * 0.5f;
	const Int32 dataSizeXY			= m_dataSizeX * m_dataSizeY;
	const Vector3 &cameraPosition	= GGame->GetFreeCamera().GetPosition();
	const Box boxForGrid( Vector3( -halfCellSize, -halfCellSize, -halfCellSize ), Vector3( halfCellSize, halfCellSize, halfCellSize ) );

	for ( Int32 i = 0; i < m_dataSize; ++i )
	{
		const Int32 index2d	= i % dataSizeXY; // index in the X/Y plane
		const Int32 indexX	= index2d % m_dataSizeX; 
		const Int32 indexY	= index2d / m_dataSizeX; 
		const Int32 indexZ	= i / dataSizeXY;
		Vector3 cellPosition;
		GetCellPositionFromIndex( indexX, indexY, indexZ, cellPosition );

		if ( ( cellPosition - cameraPosition ).SquareMag() < SWARM_DEBUG_DISTANCE * SWARM_DEBUG_DISTANCE )
		{
			if ( m_data.Get( i ) == 1 )
			{
				Matrix matrix;
				matrix.SetIdentity();
				matrix.SetTranslation( cellPosition );
				frame->AddDebugBox ( boxForGrid, matrix, Color::WHITE );
			}
		}
	}
	const Box boxForTest( Vector3( -halfCellSize * 0.9f, -halfCellSize * 0.9f, -halfCellSize * 0.9f ), Vector3( halfCellSize * 0.9f, halfCellSize * 0.9f, halfCellSize * 0.9f ) );

#ifdef SWARM_ADVANCED_DEBUG
	for ( Uint32 i = 0; i < m_debugTestCells.Size(); ++i )
	{
		const CDebugTestCellData & data = m_debugTestCells[ i ];
		Matrix matrix;
		matrix.SetIdentity();
		matrix.SetTranslation( data.m_position );
		
		frame->AddDebugSolidBox ( boxForTest, matrix, data.m_color );
		
	}
#endif
}


void CSwarmCellMap::ClearDebug()
{
#ifdef SWARM_ADVANCED_DEBUG
	m_debugTestCells.ClearFast();
#endif
}

Bool CSwarmCellMap::ComputeCollisionForceAtPosition( const Vector3 & position, const Vector3 & velocity, Float radius, Vector3 & collision, const Color& color )const
{
	// [ Step ] declaring return collision vector
	collision = Vector3( 0.0f, 0.0f, 0.0f );

	Int32 indexX, indexY, indexZ;
	GetCellIndexFromPosition( position, indexX, indexY, indexZ );

	const Int32 testIndex	= indexX + indexY * m_dataSizeX + indexZ * m_dataSizeX * m_dataSizeY;

	Bool currentCellInvalid = false;
	if (	( indexX < 0 || indexY < 0 || indexZ < 0 || indexX >= m_dataSizeX || indexY >= m_dataSizeY || indexZ >= m_dataSizeZ )
		||	m_data.Get( (Uint32)testIndex ) == 1 )
	{
		currentCellInvalid = true;
	}
	
	if ( currentCellInvalid )
	{
		// If outside the cell map returning a vector pointing tward the center of the cell map
		const Vector3 cellMapCenter = m_cornerPosition + Vector3( m_dataSizeX/2 * m_cellSize, m_dataSizeY/2 * m_cellSize, m_dataSizeZ/2 * m_cellSize);
		collision = (cellMapCenter - position).Normalized();
		return false;
	}

	if ( radius == 0.0f )
	{
		return true;
	}
	
	// [ Step ] Computing how deep the algo will reach out around the potition
	const Float halfCellSize	= m_cellSize * 0.5f;
	const Int32 depth			= radius > halfCellSize ? Max( (Int32)1, Ceil ( ( radius - halfCellSize ) / m_cellSize ) ) : 1;

	// [ Step ] Computing boundaries of the box the algo will be searching through
	const Int32 indexStartX	= indexX - depth;
	Int32 indexEndX			= indexX + depth;

	const Int32 indexStartY	= indexY - depth;
	Int32 indexEndY			= indexY + depth;

	const Int32 indexStartZ	= indexZ - depth;
	Int32 indexEndZ			= indexZ + depth;
	Float closestCellDist	= radius;
	Bool cellIsHit			= false;
	Vector3 closestCellDistPosition;
	for ( Int32 iX = indexStartX; iX <= indexEndX; ++iX )
	{
		for ( Int32 iY = indexStartY; iY <= indexEndY; ++iY )
		{
			for ( Int32 iZ = indexStartZ; iZ <= indexEndZ; ++iZ )
			{
				const Bool outsideOfMap = iX < 0 || iY < 0 || iZ < 0 || iX >= m_dataSizeX || iY >= m_dataSizeY || iZ >= m_dataSizeZ;
				Bool isObstacle = true;
			
				isObstacle = false;
				const Int32 testIndex	= iX + iY * m_dataSizeX + iZ * m_dataSizeX * m_dataSizeY;
				if ( outsideOfMap || m_data.Get( (Uint32)testIndex ) == 1 )
				{
					isObstacle = true;
					Vector3 cellPosition;
					GetCellPositionFromIndex( iX, iY, iZ, cellPosition ); // also works when outside of nav map
					Vector3 vectFromCell = position - cellPosition;
					const Float squareDistanceFromCell		= vectFromCell.SquareMag();
					if ( squareDistanceFromCell < closestCellDist * closestCellDist )
					{
						closestCellDist					= Red::Math::MSqrt( squareDistanceFromCell );
						cellIsHit						= true;
						closestCellDistPosition			= cellPosition;
					}
				}
#ifdef SWARM_ADVANCED_DEBUG
				if ( isObstacle && color.A != 0 )
				{
					Vector3 cellPosition;
					GetCellPositionFromIndex( iX, iY, iZ, cellPosition );
					const_cast< TDynArray< CDebugTestCellData > & >(m_debugTestCells).PushBack( CDebugTestCellData( cellPosition, color ) );
				}
#endif 
			}
		}
	}
	if ( cellIsHit && closestCellDist > NumericLimits< Float >::Epsilon() )   // sanity check divide by 0
	{
		const Vector3 vectFromCell				= position - closestCellDistPosition;
		const Vector3 normalizedVectFromCell	= vectFromCell / closestCellDist;
		const Float correctedDistanceFromCell	= closestCellDist - m_cellSize * 0.5f; // should be [ 0.0f, radius - m_cellSize * 0.5f ]
		ASSERT( radius > m_cellSize * 0.5f ); // sanity check
		Float lerpRatio							= radius > halfCellSize ? correctedDistanceFromCell / ( radius - halfCellSize ) : 0.0f;
		ASSERT( 0.0f <= lerpRatio && lerpRatio <= 1.0f );
		collision								= Lerp( lerpRatio, normalizedVectFromCell, Vector3( 0.0f, 0.0f, 0.0f ) );
	}
	
	return true;
}

Bool CSwarmCellMap::GetNextFreeCellPosition_AxisAligned( const Vector3 & position, Vector3 & cellPosition, const Vector3 & direction, const Color& color )const
{
	Int32 indexX, indexY, indexZ;
	GetCellIndexFromPosition( position, indexX, indexY, indexZ );

	if ( indexX < 0 || indexY < 0 || indexZ < 0 || indexX >= m_dataSizeX || indexY >= m_dataSizeY || indexZ >= m_dataSizeZ )
	{
		return false;
	}

	if ( direction.X > 0.0f )
	{
		ASSERT( direction.Y == 0.0f, TXT("Should be 0 because this is an axis align test") );
		ASSERT( direction.Z == 0.0f, TXT("Should be 0 because this is an axis align test")  );
		for ( Int32 i = indexX; i < m_dataSizeX; ++i )
		{
			const Int32 index = i + indexY * m_dataSizeX + indexZ * m_dataSizeX * m_dataSizeY;
			if ( m_data.Get( (Uint32)index ) == 0 )
			{
				GetCellPositionFromIndex( i, indexY, indexZ, cellPosition );
				return true;
			}
		}
	}
	else if ( direction.X < 0.0f )
	{
		ASSERT( direction.Y == 0.0f, TXT("Should be 0 because this is an axis align test") );
		ASSERT( direction.Z == 0.0f, TXT("Should be 0 because this is an axis align test")  );
		for ( Int32 i = indexX; i >= 0; --i )
		{
			const Int32 index = i + indexY * m_dataSizeX + indexZ * m_dataSizeX * m_dataSizeY;
			if ( m_data.Get( (Uint32)index ) == 0  )
			{
				GetCellPositionFromIndex( i, indexY, indexZ, cellPosition );
				return true;
			}
		}
	}
	else if ( direction.Y > 0.0f )
	{
		ASSERT( direction.X == 0.0f, TXT("Should be 0 because this is an axis align test") );
		ASSERT( direction.Z == 0.0f, TXT("Should be 0 because this is an axis align test")  );
		for ( Int32 i = indexY; i < m_dataSizeY; ++i )
		{
			const Int32 index = indexX + i * m_dataSizeX + indexZ * m_dataSizeX * m_dataSizeY;
			if ( m_data.Get( (Uint32)index ) == 0  )
			{
				GetCellPositionFromIndex( indexX, i, indexZ, cellPosition );
				return true;
			}
		}
	}
	else if ( direction.Y < 0.0f )
	{
		ASSERT( direction.X == 0.0f, TXT("Should be 0 because this is an axis align test") );
		ASSERT( direction.Z == 0.0f, TXT("Should be 0 because this is an axis align test")  );
		for ( Int32 i = indexY; i >= 0; --i )
		{
			const Int32 index = indexX + i * m_dataSizeX + indexZ * m_dataSizeX * m_dataSizeY;
			if ( m_data.Get( (Uint32)index ) == 0  )
			{
				GetCellPositionFromIndex( indexX, i, indexZ, cellPosition );
				return true;
			}
		}
	}
	else if ( direction.Z > 0.0f )
	{
		ASSERT( direction.X == 0.0f, TXT("Should be 0 because this is an axis align test") );
		ASSERT( direction.Y == 0.0f, TXT("Should be 0 because this is an axis align test")  );
		for ( Int32 i = indexZ; i < m_dataSizeZ; ++i )
		{
			const Int32 index = indexX + indexY * m_dataSizeX + i * m_dataSizeX * m_dataSizeY;

			if ( m_data.Get( (Uint32)index ) == 0  )
			{
				GetCellPositionFromIndex( indexX, indexY, i, cellPosition );
				return true;
			}
			
		}
	}
	else if ( direction.Z < 0.0f )
	{
		ASSERT( direction.X == 0.0f, TXT("Should be 0 because this is an axis align test") );
		ASSERT( direction.Y == 0.0f, TXT("Should be 0 because this is an axis align test")  );
		for ( Int32 i = indexZ; i >= 0; --i )
		{
			const Int32 index = indexX + indexY * m_dataSizeX + i * m_dataSizeX * m_dataSizeY;
			if ( m_data.Get( (Uint32)index ) == 0  )
			{
				GetCellPositionFromIndex( indexX, indexY, i, cellPosition );
				return true;
			}
		}
	}
	return false;
}


Bool CSwarmCellMap::GetNextBlockedCellPosition_AxisAligned( const Vector3 & position, Vector3 & blockedCellPosition, const Vector3 & direction, Float radius, Vector3 *lastFreeCellPosition, const Color& color )const
{
	Int32 indexX, indexY, indexZ;
	GetCellIndexFromPosition( position, indexX, indexY, indexZ );

	if ( indexX < 0 || indexY < 0 || indexZ < 0 || indexX >= m_dataSizeX || indexY >= m_dataSizeY || indexZ >= m_dataSizeZ )
	{
		blockedCellPosition = position;
		if ( lastFreeCellPosition )
		{
			*lastFreeCellPosition = position;
		}
		return true;
	}

	const Int32 depth = radius == -1.0f ? -1 : Ceil( radius / m_cellSize );

	if ( direction.X > 0.0f )
	{
		ASSERT( direction.Y == 0.0f, TXT("Should be 0 because this is an axis align test") );
		ASSERT( direction.Z == 0.0f, TXT("Should be 0 because this is an axis align test")  );
		for ( Int32 i = indexX; i < m_dataSizeX; ++i )
		{
			const Int32 index = i + indexY * m_dataSizeX + indexZ * m_dataSizeX * m_dataSizeY;
			if ( m_data.Get( (Uint32)index ) == 1 )
			{
				GetCellPositionFromIndex( i, indexY, indexZ, blockedCellPosition );
				if ( lastFreeCellPosition )
				{
					*lastFreeCellPosition = blockedCellPosition;
					if ( i > 0 )
					{
						*lastFreeCellPosition += Vector3( -m_cellSize, 0.0f, 0.0f );
					}
				}
				return true;
			}
#ifdef SWARM_ADVANCED_DEBUG
			Vector3 debugCellPosition;
			GetCellPositionFromIndex( i, indexY, indexZ, debugCellPosition );
			const_cast< TDynArray< CDebugTestCellData > & >(m_debugTestCells).PushBack( CDebugTestCellData( debugCellPosition, color ) );
#endif 
			if ( i - indexX + 1  == depth  )
			{
				return false;
			}
		}
	}
	else if ( direction.X < 0.0f )
	{
		ASSERT( direction.Y == 0.0f, TXT("Should be 0 because this is an axis align test") );
		ASSERT( direction.Z == 0.0f, TXT("Should be 0 because this is an axis align test")  );
		for ( Int32 i = indexX; i >= 0; --i )
		{
			const Int32 index = i + indexY * m_dataSizeX + indexZ * m_dataSizeX * m_dataSizeY;
			if ( m_data.Get( (Uint32)index ) == 1 )
			{
				GetCellPositionFromIndex( i, indexY, indexZ, blockedCellPosition );
				if ( lastFreeCellPosition )
				{
					*lastFreeCellPosition = blockedCellPosition;
					if ( i < m_dataSizeX - 1 )
					{
						*lastFreeCellPosition += Vector3( m_cellSize, 0.0f, 0.0f );
					}
				}
				return true;
			}
#ifdef SWARM_ADVANCED_DEBUG
			Vector3 debugCellPosition;
			GetCellPositionFromIndex( i, indexY, indexZ, debugCellPosition );
			const_cast< TDynArray< CDebugTestCellData > & >(m_debugTestCells).PushBack( CDebugTestCellData( debugCellPosition, color ) );
#endif 
			if ( indexX - i + 1  == depth  )
			{
				return false;
			}
		}
	}
	else if ( direction.Y > 0.0f )
	{
		ASSERT( direction.X == 0.0f, TXT("Should be 0 because this is an axis align test") );
		ASSERT( direction.Z == 0.0f, TXT("Should be 0 because this is an axis align test")  );
		for ( Int32 i = indexY; i < m_dataSizeY; ++i )
		{
			const Int32 index = indexX + i * m_dataSizeX + indexZ * m_dataSizeX * m_dataSizeY;
			if ( m_data.Get( (Uint32)index ) == 1 )
			{
				GetCellPositionFromIndex( indexX, i, indexZ, blockedCellPosition );
				if ( lastFreeCellPosition )
				{
					*lastFreeCellPosition = blockedCellPosition;
					if ( i > 0 )
					{
						*lastFreeCellPosition += Vector3( 0.0f, -m_cellSize, 0.0f );
					}
				}
				return true;
			}
#ifdef SWARM_ADVANCED_DEBUG
			Vector3 debugCellPosition;
			GetCellPositionFromIndex( indexX, i, indexZ, debugCellPosition );
			const_cast< TDynArray< CDebugTestCellData > & >(m_debugTestCells).PushBack( CDebugTestCellData( debugCellPosition, color ) );
#endif 
			if ( i - indexY + 1  == depth  )
			{
				return false;
			}
		}
	}
	else if ( direction.Y < 0.0f )
	{
		ASSERT( direction.X == 0.0f, TXT("Should be 0 because this is an axis align test") );
		ASSERT( direction.Z == 0.0f, TXT("Should be 0 because this is an axis align test")  );
		for ( Int32 i = indexY; i >= 0; --i )
		{
			const Int32 index = indexX + i * m_dataSizeX + indexZ * m_dataSizeX * m_dataSizeY;
			if ( m_data.Get( (Uint32)index ) == 1 )
			{
				GetCellPositionFromIndex( indexX, i, indexZ, blockedCellPosition );
				if ( lastFreeCellPosition )
				{
					*lastFreeCellPosition = blockedCellPosition;
					if ( i < m_dataSizeY - 1 )
					{
						*lastFreeCellPosition += Vector3( 0.0f, m_cellSize, 0.0f );
					}
				}
				return true;
			}
#ifdef SWARM_ADVANCED_DEBUG
			Vector3 debugCellPosition;
			GetCellPositionFromIndex( indexX, i, indexZ, debugCellPosition );
			const_cast< TDynArray< CDebugTestCellData > & >(m_debugTestCells).PushBack( CDebugTestCellData( debugCellPosition, color ) );
#endif 
			if ( indexY - i + 1  == depth  )
			{
				return false;
			}
		}
	}
	else if ( direction.Z > 0.0f )
	{
		ASSERT( direction.X == 0.0f, TXT("Should be 0 because this is an axis align test") );
		ASSERT( direction.Y == 0.0f, TXT("Should be 0 because this is an axis align test")  );
		for ( Int32 i = indexZ; i < m_dataSizeZ; ++i )
		{
			const Int32 index = indexX + indexY * m_dataSizeX + i * m_dataSizeX * m_dataSizeY;
			if ( m_data.Get( (Uint32)index ) == 1 )
			{
				GetCellPositionFromIndex( indexX, indexY, i, blockedCellPosition );
				if ( lastFreeCellPosition )
				{
					*lastFreeCellPosition = blockedCellPosition;
					if ( i > 0 )
					{
						*lastFreeCellPosition += Vector3( 0.0f, 0.0f, -m_cellSize );
					}
				}
				return true;
			}
#ifdef SWARM_ADVANCED_DEBUG
			Vector3 debugCellPosition;
			GetCellPositionFromIndex( indexX, indexY, i, debugCellPosition );
			const_cast< TDynArray< CDebugTestCellData > & >(m_debugTestCells).PushBack( CDebugTestCellData( debugCellPosition, color ) );
#endif 
			if ( i - indexZ + 1  == depth  )
			{
				return false;
			}
		}
	}
	else if ( direction.Z < 0.0f )
	{
		ASSERT( direction.X == 0.0f, TXT("Should be 0 because this is an axis align test") );
		ASSERT( direction.Y == 0.0f, TXT("Should be 0 because this is an axis align test")  );
		for ( Int32 i = indexZ; i >= 0; --i )
		{
			const Int32 index = indexX + indexY * m_dataSizeX + i * m_dataSizeX * m_dataSizeY;
			if ( m_data.Get( (Uint32)index ) == 1 )
			{
				GetCellPositionFromIndex( indexX, indexY, i, blockedCellPosition );
				if ( lastFreeCellPosition )
				{
					*lastFreeCellPosition = blockedCellPosition;
					if ( i < m_dataSizeZ - 1 )
					{
						*lastFreeCellPosition += Vector3( 0.0f, 0.0f, m_cellSize );
					}
				}
				return true;
			}
#ifdef SWARM_ADVANCED_DEBUG
			Vector3 debugCellPosition;
			GetCellPositionFromIndex( indexX, indexY, i, debugCellPosition );
			const_cast< TDynArray< CDebugTestCellData > & >(m_debugTestCells).PushBack( CDebugTestCellData( debugCellPosition, color ) );
#endif 
			if ( indexZ - i + 1  == depth  )
			{
				return false;
			}
		}
	}
	return false;
}

Bool CSwarmCellMap::GetFreeCellAtDistance_AxisAligned( const Vector3 & position, Vector3 & freeCellPosition, const Vector3 & direction, Float distance, const Color& color )const 
{
	Int32 indexX, indexY, indexZ;
	GetCellIndexFromPosition( position, indexX, indexY, indexZ );

	if ( indexX < 0 || indexY < 0 || indexZ < 0 || indexX >= m_dataSizeX || indexY >= m_dataSizeY || indexZ >= m_dataSizeZ )
	{
		return false;
	}

	if ( direction.X > 0.0f )
	{
		ASSERT( direction.Y == 0.0f, TXT("Should be 0 because this is an axis align test") );
		ASSERT( direction.Z == 0.0f, TXT("Should be 0 because this is an axis align test")  );
		for ( Int32 i = indexX + 1; i < m_dataSizeX; ++i ) // Do not check current cell
		{
			const Int32 index = i + indexY * m_dataSizeX + indexZ * m_dataSizeX * m_dataSizeY;
			if ( m_data.Get( (Uint32)index ) == 1 ) // bails out if blocked
			{
				return false;
			}
			GetCellPositionFromIndex( i, indexY, indexZ, freeCellPosition );
#ifdef SWARM_ADVANCED_DEBUG
			if ( color.A != 0 )
			{
				const_cast< TDynArray< CDebugTestCellData > & >(m_debugTestCells).PushBack( CDebugTestCellData( freeCellPosition, color ) );
			}
#endif 
			if ( freeCellPosition.X - position.X > distance )
			{
				return true;
			}
		}
	}
	else if ( direction.X < 0.0f )
	{
		ASSERT( direction.Y == 0.0f, TXT("Should be 0 because this is an axis align test") );
		ASSERT( direction.Z == 0.0f, TXT("Should be 0 because this is an axis align test")  );
		for ( Int32 i = indexX - 1; i >= 0; --i )  // Do not check current cell
		{
			const Int32 index = i + indexY * m_dataSizeX + indexZ * m_dataSizeX * m_dataSizeY;
			if ( m_data.Get( (Uint32)index ) == 1 ) // bails out if blocked
			{
				return false;
			}
			GetCellPositionFromIndex( i, indexY, indexZ, freeCellPosition );
#ifdef SWARM_ADVANCED_DEBUG
			if ( color.A != 0 )
			{
				const_cast< TDynArray< CDebugTestCellData > & >(m_debugTestCells).PushBack( CDebugTestCellData( freeCellPosition, color ) );
			}
#endif 
			if ( position.X - freeCellPosition.X  > distance )
			{
				return true;
			}
		}
	}
	else if ( direction.Y > 0.0f )
	{
		ASSERT( direction.X == 0.0f, TXT("Should be 0 because this is an axis align test") );
		ASSERT( direction.Z == 0.0f, TXT("Should be 0 because this is an axis align test")  );
		for ( Int32 i = indexY + 1; i < m_dataSizeY; ++i )  // Do not check current cell
		{
			const Int32 index = indexX + i * m_dataSizeX + indexZ * m_dataSizeX * m_dataSizeY;
			if ( m_data.Get( (Uint32)index ) == 1 ) // bails out if blocked
			{
				return false;
			}
			GetCellPositionFromIndex( indexX, i, indexZ, freeCellPosition );
#ifdef SWARM_ADVANCED_DEBUG
			if ( color.A != 0 )
			{
				const_cast< TDynArray< CDebugTestCellData > & >(m_debugTestCells).PushBack( CDebugTestCellData( freeCellPosition, color ) );
			}
#endif 
			if ( freeCellPosition.Y - position.Y > distance )
			{
				return true;
			}
		}
	}
	else if ( direction.Y < 0.0f )
	{
		ASSERT( direction.X == 0.0f, TXT("Should be 0 because this is an axis align test") );
		ASSERT( direction.Z == 0.0f, TXT("Should be 0 because this is an axis align test")  );
		for ( Int32 i = indexY - 1; i >= 0; --i )  // Do not check current cell
		{
			const Int32 index = indexX + i * m_dataSizeX + indexZ * m_dataSizeX * m_dataSizeY;
			if ( m_data.Get( (Uint32)index ) == 1 ) // bails out if blocked
			{
				return false;
			}
			GetCellPositionFromIndex( indexX, i, indexZ, freeCellPosition );
#ifdef SWARM_ADVANCED_DEBUG
			if ( color.A != 0 )
			{
				const_cast< TDynArray< CDebugTestCellData > & >(m_debugTestCells).PushBack( CDebugTestCellData( freeCellPosition, color ) );
			}
#endif 
			if ( position.Y - freeCellPosition.Y > distance )
			{
				return true;
			}
		}
	}
	else if ( direction.Z > 0.0f )
	{
		ASSERT( direction.X == 0.0f, TXT("Should be 0 because this is an axis align test") );
		ASSERT( direction.Y == 0.0f, TXT("Should be 0 because this is an axis align test")  );
		for ( Int32 i = indexZ + 1; i < m_dataSizeZ; ++i )  // Do not check current cell
		{
			const Int32 index = indexX + indexY * m_dataSizeX + i * m_dataSizeX * m_dataSizeY;
			if ( m_data.Get( (Uint32)index ) == 1 ) // bails out if blocked
			{
				return false;
			}
			GetCellPositionFromIndex( indexX, indexY, i, freeCellPosition );
#ifdef SWARM_ADVANCED_DEBUG
			if ( color.A != 0 )
			{
				const_cast< TDynArray< CDebugTestCellData > & >(m_debugTestCells).PushBack( CDebugTestCellData( freeCellPosition, color ) );
			}
#endif 
			if ( freeCellPosition.Z - position.Z > distance )
			{
				return true;
			}
		}
	}
	else if ( direction.Z < 0.0f )
	{
		ASSERT( direction.X == 0.0f, TXT("Should be 0 because this is an axis align test") );
		ASSERT( direction.Y == 0.0f, TXT("Should be 0 because this is an axis align test")  );
		for ( Int32 i = indexZ - 1; i >= 0; --i )  // Do not check current cell
		{
			const Int32 index = indexX + indexY * m_dataSizeX + i * m_dataSizeX * m_dataSizeY;
			if ( m_data.Get( (Uint32)index ) == 1 ) // bails out if blocked
			{
				return false;
			}
			GetCellPositionFromIndex( indexX, indexY, i, freeCellPosition );
#ifdef SWARM_ADVANCED_DEBUG
			if ( color.A != 0 )
			{
				const_cast< TDynArray< CDebugTestCellData > & >(m_debugTestCells).PushBack( CDebugTestCellData( freeCellPosition, color ) );
			}
#endif 
			if ( position.Z - freeCellPosition.Z > distance )
			{
				return true;
			}
		}
	}
	return false;
}

Bool CSwarmCellMap::LineTest( const Vector3 &position, const Vector3 &direction, const Color& color )const
{
	Int32 indexX, indexY, indexZ;
	GetCellIndexFromPosition( position, indexX, indexY, indexZ );

	if ( indexX < 0 || indexY < 0 || indexZ < 0 || indexX >= m_dataSizeX || indexY >= m_dataSizeY || indexZ >= m_dataSizeZ )
	{
		return true;
	}

	const Vector3 endPoint			= position + direction;
	const Float directionSquareMag	= direction.SquareMag();

	Vector enterPoint( 0.0f, 0.0f, 0.0f );
	Vector exitPoint( 0.0f, 0.0f, 0.0f );
	do
	{
		Vector3 cellPosition;
		GetCellPositionFromIndex( indexX, indexY, indexZ, cellPosition );
		const Uint32 index = indexX + indexY * m_dataSizeX + indexZ * m_dataSizeX * m_dataSizeY;
		if ( m_data.Get( index ) == 1 )
		{
			return true;
		}
		const Float halfCellSize = m_cellSize * 0.5f;
		Box box( cellPosition - Vector3( halfCellSize, halfCellSize, halfCellSize ), cellPosition + Vector3( halfCellSize, halfCellSize, halfCellSize ) );
#ifdef SWARM_ADVANCED_DEBUG
		if ( color.A != 0 )
		{
			const_cast< TDynArray< CDebugTestCellData > & >(m_debugTestCells).PushBack( CDebugTestCellData( cellPosition, color ) );
		}
#endif 	
		const Bool result = box.IntersectSegment( Segment( position, direction ), enterPoint, exitPoint );

		// If both points are the same 
		// that means the vector doesn't exit the box  ( fact inherent to Box::IntersectSegment  )
		// as the box is not an obstacle return false
		if ( enterPoint == exitPoint )
		{
			return false;
		}

		if ( result == false )
		{
			// This does happen because of float imprecision, we may be able to fix this one day 
			// but for now this happens so little it is acceptable
			// failing returns true just in case
			return true;
		}
		const Float diffToXPlus		= Abs( exitPoint.X - ( cellPosition.X + halfCellSize ) );
		const Float diffToXMinus	= Abs( exitPoint.X - ( cellPosition.X - halfCellSize ) );
		const Float diffToYPlus		= Abs( exitPoint.Y - ( cellPosition.Y + halfCellSize ) );
		const Float diffToYMinus	= Abs( exitPoint.Y - ( cellPosition.Y - halfCellSize ) );
		const Float diffToZPlus		= Abs( exitPoint.Z - ( cellPosition.Z + halfCellSize ) );
		const Float diffToZMinus	= Abs( exitPoint.Z - ( cellPosition.Z - halfCellSize ) );
		if ( (	   diffToXPlus > LINE_TEST_EPSILON || diffToXMinus > LINE_TEST_EPSILON 
				|| diffToYPlus > LINE_TEST_EPSILON || diffToYMinus > LINE_TEST_EPSILON
				|| diffToZPlus > LINE_TEST_EPSILON || diffToZMinus > LINE_TEST_EPSILON ) == false )
		{
			ASSERT( false, TXT("Swarm Collision failed") );
			// failing returns true just in case
			return true;
		}
		if ( diffToXPlus <= LINE_TEST_EPSILON )
		{
			ASSERT( diffToXMinus > LINE_TEST_EPSILON );
			if ( indexX == m_dataSizeX - 1 )
			{
				return true;
			}
			indexX++;
		}
		else if ( diffToXMinus <= LINE_TEST_EPSILON )
		{
			ASSERT( diffToXPlus > LINE_TEST_EPSILON );
			if ( indexX == 0 )
			{
				return true;
			}
			indexX--;
		}

		if ( diffToYPlus <= LINE_TEST_EPSILON )
		{
			ASSERT( diffToYMinus > LINE_TEST_EPSILON );
			if ( indexY == m_dataSizeY - 1 )
			{
				return true;
			}
			indexY++;
		}
		else if ( diffToYMinus <= LINE_TEST_EPSILON )
		{
			ASSERT( diffToYPlus > LINE_TEST_EPSILON );
			if ( indexY == 0 )
			{
				return true;
			}
			indexY--;
		}
		
		if ( diffToZPlus <= LINE_TEST_EPSILON )
		{
			ASSERT( diffToZMinus > LINE_TEST_EPSILON );
			if ( indexZ == m_dataSizeZ - 1 )
			{
				return true;
			}
			indexZ++;
		}
		else if ( diffToZMinus <= LINE_TEST_EPSILON )
		{
			ASSERT( diffToZPlus > LINE_TEST_EPSILON );
			if ( indexZ == 0 )
			{
				return true;
			}
			indexZ--;
		}		
	}while ( directionSquareMag > ( position - exitPoint ).SquareMag() );

	// No collision found
	return false;
}

CDiskFile *const CSwarmCellMap::FindFileInDepot( const String &m_resourceName )
{
	CDirectory* dir = R4SwarmUtils::GetDataDirectory();
	if ( dir )
	{
		const String fullFileName = String::Printf( TXT( "%s.%s"), m_resourceName.AsChar(), CSwarmCellMap::GetFileExtension() );
		return dir->FindLocalFile( fullFileName );
	}
	return nullptr;
}

CSwarmCellMap* CSwarmCellMap::Create()
{
	IFactory* factory = IFactory::FindFactory( CSwarmCellMap::GetStaticClass() );
	if ( factory == nullptr )
		return nullptr;
		
	// Create resource
	IFactory::FactoryOptions options;
	options.m_parentObject = nullptr;
	CResource* created = factory->DoCreate( options );
	CSwarmCellMap *const swarmCellMap = static_cast< CSwarmCellMap* >( created );

	return swarmCellMap;
}

void CSwarmCellMap::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if( !file.IsGarbageCollector() )
	{
		file<<m_cellSize;

		m_data.OnSerialize( file );

		// Cached useful variables 
		file<<m_cornerPosition;

		file<<m_dataSizeX;
		file<<m_dataSizeY;
		file<<m_dataSizeZ;
		file<<m_dataSize;
		file<<m_sizeInKbytes;
	}
}

///////////////////////////////////////////////////////////////////////////////
// CSwarmCellMapFactory
///////////////////////////////////////////////////////////////////////////////
class CSwarmCellMapFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CSwarmCellMapFactory, IFactory, 0 );

public:
	CSwarmCellMapFactory()
	{
		m_resourceClass = ClassID< CSwarmCellMap >();
	}

	CResource* DoCreate( const FactoryOptions& options ) override
	{
		return ::CreateObject< CSwarmCellMap >( options.m_parentObject );		
	}
};

BEGIN_CLASS_RTTI( CSwarmCellMapFactory )
	PARENT_CLASS( IFactory )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CSwarmCellMapFactory );