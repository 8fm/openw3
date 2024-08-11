#pragma once

#include "../../common/game/swarmAlgorithmData.h"
#include "r4SwarmUtils.h"
#include "humbleCrittersLairEntity.h"
#include "swarmEnvironment.h"

class CHumbleCrittersLairEntity;
class CHumbleCritterAI;
struct SSwarmMemberStateData;
class CPointOfInterestSpeciesConfig;


class CHumbleCrittersAlgorithmData : public CSwarmAlgorithmData
{
	typedef CSwarmAlgorithmData Super;
	typedef CHumbleCrittersLairEntity Lair;
public:

	CHumbleCrittersAlgorithmData( CHumbleCrittersLairEntity* lair, const CHumbleCritterLairParams & params);
	~CHumbleCrittersAlgorithmData();

	void Initialize( CSwarmLairEntity& lair ) override;

	void UpdateMovementAndAnimation( const TSwarmStatesList currentState, TSwarmStatesList targetState )override;

	void PreUpdateSynchronization( Float deltaTime ) override;
	void PostUpdateSynchronization( ) override;

	const SwarmEnviromentData*				GetEnviroment() const							{ return m_enviroment; }
	
	const CPoiJobData_Array&			GetSpawnpoints() const							{ return *m_spawnPoints; }
	const CPoiJobData_Array&			GetActors() const								{ return *m_actorsList; }
	const CHumbleCritterLairParams &	GetHumbleCritterLairParams()const				{ return *static_cast<const CHumbleCritterLairParams *>(m_params); }

	void OnBoidActivated()										{ ++m_activeBoids; }
	void OnBoidDeactivated()									{ --m_activeBoids; }
	void OnBoidBorn()											{ ++m_aliveBoids; }
	void OnBoidDied()											{ --m_aliveBoids; }
	Int32 GetActiveBoids() const								{ return m_activeBoids; }

	void BoidIsAttacking( Boids::PointOfInterestId target );

	Boids::PointOfInterestId AddTemporaryPoi( Boids::PointOfInterestType poiType, CPoiJobData& poiData, Float fadeoutDelay );
protected:
	template < class Acceptor >
	void CalculateSound( TSwarmStatesList crittersStates, SwarmSoundData& sound, Acceptor& acceptor );

	void CalculateEnviromentData( CHumbleCrittersLairEntity& lair );
	Bool TestCellMaxZ( Int32 x, Int32 y, Float currentZ )const;

	Uint32							m_breakCounter;

	Int32							m_activeBoids;
	Int32							m_aliveBoids;

	SwarmEnviromentData*			m_enviroment;

	CPoiJobData_Array*				m_actorsList;
	CPoiJobData_Array*				m_spawnPoints;

	Bool							m_isLairDefeated;
	Bool							m_isDeactivationTimeUp;
	Bool							m_isDeactivationPending;
	Float							m_deactivationTime;

	TArrayMap< Boids::PointOfInterestId, Uint32 >					m_attackingCritters;

	Boids::PointOfInterestId		m_fireInConePoiID;

	struct TemporaryPoi
	{
		Boids::PointOfInterestType				m_poiType;
		Boids::PointOfInterestId				m_poiId;
		Float									m_fadeoutTime;
	};
	TStaticArray< TemporaryPoi, 4 >		m_temporaryPois;

	Uint32								m_nextPoiId;
};
