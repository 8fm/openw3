#include "build.h"
#include "baseCrittersAI.h"
#include "boidCone.h"

CBaseCritterAI::CBaseCritterAI()
	: m_poiDataByType_Map()
	, m_randomSeed( GEngine->GetRandomNumberGenerator().Get< Uint32 >() )
	, m_currentEffectorType( CName::NONE )
	, m_critterState( CRITTER_STATE_NOT_SPAWNED )
	, m_currentBoidState( BOID_STATE_NOT_SPAWNED )
{
}

CPoiDataByType *const CBaseCritterAI::GetPoiDataByType( CName pointOfInterestType )
{
	CPoiDataByType_Map::iterator it = m_poiDataByType_Map.Find( pointOfInterestType );
	if ( it != m_poiDataByType_Map.End() )
	{
		return &it->m_second;
	}
	m_poiDataByType_Map.Insert( pointOfInterestType, CPoiDataByType() );
	return NULL;
}