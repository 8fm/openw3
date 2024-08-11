#include "build.h"

#include "pathlibTerrainInfo.h"

#include "pathlibTerrain.h"
#include "clipMap.h"
#include "terrainTile.h"
#include "world.h"

namespace PathLib
{
////////////////////////////////////////////////////////////////////////////
// CTerrainManager
////////////////////////////////////////////////////////////////////////////

CTerrainInfo::CTerrainInfo( CPathLibWorld* pathlib )
	: m_pathlib( *pathlib )
	, m_tileSize( 1024.f )
	, m_tilesResolution( 1024 )
	, m_tilesInRow( 0 )
	, m_terrainCorner( 0, 0 )
	, m_quadSize( 1.f )
	, m_isInitialized( false )
{
}
void CTerrainInfo::Initialize( const CClipMap* map )
{
	Clear();
	m_isInitialized = true;
	m_tileSize = map->GetTileSize();
	m_tilesResolution = map->GetTilesMaxResolution();
	m_tilesInRow = map->GetNumTilesPerEdge();
	m_terrainCorner = map->GetTerrainCorner();
	m_quadSize = m_tileSize / Float(m_tilesResolution);
}
void CTerrainInfo::Clear()
{
	m_isInitialized = false;
}

//CTerrainQueryData* CTerrainInfo::GetQueryData() const
//{
//	// enter critical section
//	do
//	{
//		Int32 inUse = m_queryDataInUse.Increment();
//		if ( inUse <= s_queryDataCache )
//		{
//			break;
//		}
//		// NOTICE: active wait - basically it shouldn't happen if s_queryDataCache is high enough
//		m_queryDataInUse.Decrement();
//		Sleep(0);
//	}
//	while ( true );
//
//	// get yourself Ur own query data
//	Uint32 index = ( m_queryDataCounter.Increment() & (s_queryDataCache-1) );
//	return m_queryData[ index ];
//}
//void CTerrainInfo::ReturnBackQueryData( CTerrainQueryData* data ) const
//{
//	m_queryDataInUse.Decrement();
//}
void CTerrainInfo::GetTileCoordsAtPosition( const Vector2& pos, Int32& outX, Int32& outY ) const
{
	Vector2 diff = pos - m_terrainCorner;
	diff /= m_tileSize;
	outX = Clamp( Int32(diff.X), 0, Int32(m_tilesInRow-1) );
	outY = Clamp( Int32(diff.Y), 0, Int32(m_tilesInRow-1) );
}
void CTerrainInfo::GetTileCoordsFromId( AreaId id, Int32& x, Int32& y ) const
{
	id = id & ~PathLib::CAreaDescription::ID_MASK_TERRAIN;
	x = id / m_tilesInRow;
	y = id % m_tilesInRow;
}
Vector2 CTerrainInfo::GetTileCorner( Int32 x, Int32 y ) const
{
	return Vector2(
		m_terrainCorner.X + Float(x) * m_tileSize,
		m_terrainCorner.Y + Float(y) * m_tileSize
		);
}

Uint32 CTerrainInfo::ComputeHash() const
{
	Uint32 h[5];

	h[0] = GetHash( Int32( m_tileSize * 512.f ) );
	h[1] = GetHash( m_tilesResolution );
	h[2] = GetHash( m_tilesInRow );
	h[3] = GetHash( Int32( m_terrainCorner.X * 512.f ) );
	h[4] = GetHash( Int32( m_terrainCorner.Y * 512.f ) );

	return GetArrayHash( h, 5 );
}

Bool CTerrainInfo::IsQuadTriangulatedNW2SE( Int32 x, Int32 y )
{
	return CTerrainTile::IsQuadTriangulatedNW2SE( x, y );
}

};		// namespace PathLib