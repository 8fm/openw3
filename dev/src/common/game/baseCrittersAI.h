#pragma once

#include "../../common/game/swarmsDensityMap.h"
#include "../../common/game/swarmAlgorithmData.h"
#include "../../common/game/poiJobData.h"

class CHumbleCrittersAlgorithmData;

class CBaseCritterAI
{
public :
	CBaseCritterAI();

	CPoiDataByType *const	GetPoiDataByType( CName pointOfInterestType );
	const CName &			GetCurrentEffectorType()const{ return m_currentEffectorType; }
	void					SetCurrentEffectorType( CName type ){ m_currentEffectorType = type; }
	const EBoidState &		GetBoidState()const { return m_currentBoidState; }
	const ECritterState	&	GetCritterState() const{ return m_critterState; }
protected:
	Uint32							m_randomSeed;
	CPoiDataByType_Map				m_poiDataByType_Map;	// data specific for a boid to each Poi type
	CName							m_currentEffectorType;	// Current effector the boid is in
	ECritterState					m_critterState;
	EBoidState						m_currentBoidState;
	
};