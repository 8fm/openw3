#pragma once

#include "r4BoidSpecies.h"
#include "r4SwarmUtils.h"
#include "../../common/game/baseCrittersAI.h"
#include "../../common/game/swarmsDensityMap.h"
#include "../../common/game/swarmAlgorithmData.h"

class CFlyingCrittersAlgorithmData;
class CPoiConfigByGroup;
class CFlyingCritterLairParams;
class CFlyingCrittersLairEntity;
class CGroupState;
class CFlyingSwarmGroup;

#define MAX_SWARM_NEIGHBOUR_COUNT	(Uint32)100
typedef TStaticArray< Uint32, MAX_SWARM_NEIGHBOUR_COUNT > CNeighbourId_Array;


class CFlyingCritterAI : public CBaseCritterAI 
{
	typedef CFlyingCrittersLairEntity Lair;
	typedef CFlyingCrittersAlgorithmData AlgorithmData;
public:
	typedef Float Time;

	
	CFlyingCritterAI();

	void							SetWorld( CFlyingCrittersAlgorithmData* algorithmData )		{ m_algorithmData = algorithmData; }
	Bool							SeeActor() const											{ return m_targetActor != INVALID_POI_ID; }

	void			UpdateFlying( const SSwarmMemberStateData& currState, SSwarmMemberStateData& destState, const CFlyingSwarmGroup *const flyingGroup, Bool poiStateChanged, const Vector3 &tooCloseForce, const Vector3 &tooFarForce, const Vector3 &centerOfCloud, CFlyingCrittersPoi_Map & flyingCrittersPoi_Map, const CPoiJobData_Array *const despawnPoiArray, const CSwarmCellMap *const cellMap ); 
	void			Spawn( const Vector3& position );
	Bool			ApplyFire(Boids::PointOfInterestId poiID);
	void			ApplyAction(Boids::PointOfInterestId poiID);
	void			GoDespawn();
	void			CancelDespawn();
	void			Kill();

	const Float&			GetEffectorRandom()const				{ return m_effectorRandom; }
	CNeighbourId_Array &	GetNeighbourIdArray()					{ return m_neighbourArray; }
	const Bool &			GetHasDespawned()const					{ return m_hasDespawned; }

protected:
	void		ChangeState( EFlyingCritterState state );

	CFlyingCrittersAlgorithmData*	m_algorithmData;
	Bool							m_stateChanged;
	Boids::PointOfInterestId		m_targetActor;
	Time							m_stateTimeout;
	Uint32							m_randomSeed;
	Bool							m_isAlive;
	Float							m_effectorRandom;

	Vector3							m_velocity;
	Vector3							m_randomForce;
	Float							m_randomDirectionTimeOut;
	Bool							m_hasRandomDir;
	Float							m_groundZ;

	/// previous boid pos, stored here because it cannot be accessed via the state list
	Vector3							m_previousPosition;
	/// True if the boid has despawned without dying during the frame 
	Bool							m_hasDespawned;
public :
	// Data both set and accessed by CFlyingCritterAlgorithmData 
	CNeighbourId_Array				m_neighbourArray;
	Vector3							m_lastTooCloseDistanceVect;

private:

	// Helper functions 
	const CPoiJobData *const	FindClosestPoi( const CPoiJobData_Array *const poiJobDataArray, const Vector3& currentPosition, Float & closestSpawnPointSq )const;
	Vector3						UpdateCirclePoi			( const CPoiConfigByGroup *const poiConfig, const Vector &boidPosition, CPoiJobData &poiJobData, Float randomisation );
	void						ApplyEffector			( const CPoiJobData &poiJobData, const CPoiConfigByGroup*const poiConfig );
};

