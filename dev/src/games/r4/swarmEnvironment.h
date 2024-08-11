#pragma once

#include "../../common/core/heap.h"
#include "../../common/engine/pathlibWorld.h"
#include "../../common/game/swarmsDensityMap.h"


struct SwarmSoundData
{
	SwarmSoundData()
		: m_soundCenter( 0, 0, 0 )
		, m_soundIntesity( 0.f )
		, m_radius( 0.f )
		, m_maxDistanceSq( 20.f * 20.f )		{}
	Vector				m_soundCenter;
	Float				m_soundIntesity;
	Float				m_radius;
	Float				m_maxDistanceSq;
};

enum ESwarmCellDataFlags
{
	CDF_BLOCKED					= FLAG(0),
	CDF_FLOOD_FILL_EXPLORED		= FLAG(1),
};
struct SwarmEnviromentCelData
{
	Vector2				m_wallPotential;
	Float				m_z;
	Uint16				m_flags;
	mutable Int16		m_guyzCounter;
};

typedef CSwarmDensityMap< SwarmEnviromentCelData > SwarmEnviromentData;




struct CCoord
{
	Int32 X, Y;
	CCoord( Int32 x = 0, Int32 y = 0 )
		: X(x)
		, Y(y)	{}
};

struct CSwarmHeadData
{
	CCoord	m_coord;
	Float	m_sqDistanceFromFloodSource;
	Vector2	m_floodSourcePosition2D;
	Float	m_lastValidHeight;

	CSwarmHeadData( const CCoord &coord, Float sqDistanceFromFloodSource, const Vector2 &floodSourcePosition2D, Float lastValidHeight )
		: m_coord( coord )
		, m_sqDistanceFromFloodSource( sqDistanceFromFloodSource )
		, m_floodSourcePosition2D( floodSourcePosition2D )
	, m_lastValidHeight( lastValidHeight )	{}

	// Enabling head sort 
	Bool operator<( const CSwarmHeadData& swarmHeadData ) const
	{
		return m_sqDistanceFromFloodSource < swarmHeadData.m_sqDistanceFromFloodSource;
	}

	Bool operator>( const CSwarmHeadData& swarmHeadData ) const
	{
		return m_sqDistanceFromFloodSource > swarmHeadData.m_sqDistanceFromFloodSource;
	}
};


class CSwarmFloodFill
{
public :
	CSwarmFloodFill( SwarmEnviromentData* enviroment, CPathLibWorld* pathLibWorld, Float cellRadius, CAreaComponent* areaBoundings = nullptr );

	void Process( );
	void AddFloodSource( const Vector3 &floodSourcePosition );
private :
	SwarmEnviromentCelData *const IsCellElligible( const CCoord & cellCoord  )const;
	void ProcessCell( Float height, const CCoord & cellCoord, SwarmEnviromentCelData& cellData, const Vector2 &floodSourcePosition2D, Float lastValidHeight );
	void ProcessNeighbourCell( const CCoord & coord, const Vector3 &lastExporedPosition, PathLib::AreaId &areaId, const Vector2 &floodSourcePosition2D, Float lastValidHeight );
private :
	 THeap< CSwarmHeadData >		m_priorityQueue;
	 SwarmEnviromentData*			m_enviroment;
	 CPathLibWorld*					m_pathLibWorld;
	 CAreaComponent*				m_areaBoundings;
	 Float							m_cellRadius;
};
