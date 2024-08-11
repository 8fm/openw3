/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

namespace PathLib
{

class CRoughtTerrainMap
{
protected:
	struct RoughtTerrainZone
	{
		RoughtTerrainZone()																			{}
		RoughtTerrainZone( const RoughtTerrainZone& d )
			: m_verts( d.m_verts )
			, m_bbox( d.m_bbox )
			, m_isRoughtTerrain( d.m_isRoughtTerrain )												{}
		RoughtTerrainZone( RoughtTerrainZone&& d )
			: m_verts( Move( d.m_verts ) )
			, m_bbox( d.m_bbox )
			, m_isRoughtTerrain( d.m_isRoughtTerrain )												{}

		void operator=( RoughtTerrainZone&& d )
		{
			m_verts = Move( d.m_verts );
			m_bbox = d.m_bbox;
			m_isRoughtTerrain = d.m_isRoughtTerrain;
		}

		Box							m_bbox;
		TDynArray< Vector2 >		m_verts;
		Bool						m_isRoughtTerrain;
	};

	TDynArray< RoughtTerrainZone >	m_zones;

public:
	enum ERoughtOverride
	{
		NOPE,
		ROUGH,
		EASY
	};

	CRoughtTerrainMap();
	~CRoughtTerrainMap();

	void Collect( CPathLibRoughtTerrainComponent* component );

	ERoughtOverride IsLocationRought( const Vector& pos ) const;
	ERoughtOverride IsLineRought( const Vector& v0, const Vector& v1 ) const;
};

};			// namespace PathLib