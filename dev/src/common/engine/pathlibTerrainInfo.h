#pragma once
#include "pathlib.h"

class CPathLibWorld;

namespace PathLib
{

//struct CTerrainQueryData;

////////////////////////////////////////////////////////////////////////////
class CTerrainInfo : public Red::System::NonCopyable
{
protected:
	CPathLibWorld&				m_pathlib;
	Float						m_tileSize;
	Uint32						m_tilesResolution;
	Uint32						m_tilesInRow;
	Vector2						m_terrainCorner;
	Float						m_quadSize;
	Bool						m_isInitialized;
public:
	CTerrainInfo( CPathLibWorld* pathlib );

	void Initialize( const CClipMap* terrain );
	void Clear();

	void GetTileCoordsAtPosition( const Vector2& pos, Int32& outX, Int32& outY ) const;
	AreaId GetTileIdAtPosition( const Vector2& pos ) const					{ Int32 x, y; GetTileCoordsAtPosition( pos, x, y ); return GetTileIdFromCoords( x, y ); }
	AreaId GetTileIdFromCoords( Int32 x, Int32 y ) const					{ return PathLib::AreaId(x * m_tilesInRow + y) | PathLib::AREA_ID_MASK_TERRAIN; }
	void GetTileCoordsFromId( AreaId id, Int32& x, Int32& y ) const;
	Vector2 GetTileCorner( Int32 x, Int32 y ) const;
	Float GetQuadSize() const												{ return m_quadSize; }
	Float GetTileSize() const												{ return m_tileSize; }
	Uint32 GetTilesResolution() const										{ return m_tilesResolution; }
	Uint32 GetQuadsCount() const											{ return m_tilesResolution*m_tilesResolution; }
	CPathLibWorld& GetPathLib() const										{ return m_pathlib; }
	Bool IsInitialized() const												{ return m_isInitialized; }
	const Vector2& GetTerrainCorner() const									{ return m_terrainCorner; }
	Uint32 GetTilesCount() const											{ return m_tilesInRow * m_tilesInRow; }
	Uint32 GetTilesInRow() const											{ return m_tilesInRow; }

	Uint32 ComputeHash() const;

	static Bool IsQuadTriangulatedNW2SE( Int32 x, Int32 y );
};

};		// namespace PathLib