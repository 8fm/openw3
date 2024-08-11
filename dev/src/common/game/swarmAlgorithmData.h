#pragma once

#include "swarmLairEntity.h"
#include "swarmUtils.h"
#include "pointOfInterestParams.h"
#include "poiJobData.h"
#include "swarmSound.h"


///////////////////////////////////////////////////////////////
// Custom processing data
// Only stuff that is modified during 
class CSwarmAlgorithmData : public Red::System::NonCopyable
{
protected:
	volatile CSwarmLairEntity*					m_lair;

	// Hard copy of data from xml config
	const CBoidLairParams	*					m_params;

	// List of boid AI's of size m_boidsLimit
	CBaseCritterAI **							m_critterAiArray;	
	const Uint32								m_boidsLimit;

	CPoiJobData_Map								m_dynamicPoiJobDataMap;
	CPoiJobData_Map								m_staticPoiJobData_Map;

	Float										m_spawnFrequency;
	Int32										m_totalSpawnLimit;
	Uint32										m_dynamicPoiCount;
	Boids::PointOfInterestId					m_poiUid;
	Float										m_localTime;
	Float										m_currentDelta;
	Float										m_spawnAccumulator;

	Vector										m_cameraPosition;
	Vector										m_cameraForward;
	CSwarmSoundJobDataCollection_Array			m_soundJobDataCollectionArray;

	CUint_Array									m_boidStateCountArray;

	// Number of boids inside an effector ( whatever effector ) ( to optimize sound )
	Uint32										m_boidCountInEffector;

	Bool										m_lairDisabledFromScript;

public:
	CSwarmAlgorithmData( CSwarmLairEntity* lair );
	virtual ~CSwarmAlgorithmData();

	virtual void		Initialize( CSwarmLairEntity& lair );
	void				Update( const TSwarmStatesList currentStateList, TSwarmStatesList targetStateList );
	virtual void		PreUpdateSynchronization( Float deltaTime );
	virtual void		PostUpdateSynchronization( );

	virtual void		UpdateMovementAndAnimation( const TSwarmStatesList currentStateList, TSwarmStatesList targetStateList );
	virtual void		UpdateSound( const TSwarmStatesList currentStateList, TSwarmStatesList targetStateList );

	virtual void CalculateSound( TSwarmStatesList crittersStates, CSwarmSoundJobData *const  soundJobData );

	virtual void FillPoiJobDataFromDynamicPoi( CPoiJobData & poiJobData, const CName& filterTag, CEntity*const entity );

	void						AddDynamicPoi( CEntity* entity, const CName& filterTag );
	void						RemoveDynamicPoi( CEntity* entity );
	Bool						HasDynamicPois() { return m_dynamicPoiCount > 0; }
	void						AddStaticPoi( const CPoiJobData& staticPoint );
	void						RemoveStaticPoi( Boids::PointOfInterestType poiType, Boids::PointOfInterestId uid );
	CPoiJobData *const			GetStaticPoi( Boids::PointOfInterestType poiType, Boids::PointOfInterestId uid );
	const Uint32 &				GetBoidStateCount( EBoidState boidState )const{ return m_boidStateCountArray[ boidState ]; } 

	Float								GetLocalTime() const					{ return m_localTime; }
	Float								GetCurrentDelta() const					{ return m_currentDelta; }
	Uint32								GetBoidsLimit() const					{ return m_boidsLimit; }
	Bool								IsSpawnLimitHit() const					{ return m_totalSpawnLimit == 0; }
	CPoiJobData_Map&					GetStaticPoiDataByType_Map()			{ return m_staticPoiJobData_Map; }
	const CPoiJobData_Map&				GetStaticPoiDataByType_Map()const 		{ return m_staticPoiJobData_Map; }
	const CPoiJobData_Map&				GetDynamicPoiDataByType_Map()const 		{ return m_dynamicPoiJobDataMap; }
	const CBaseCritterAI*const*			GetCritterAiArray() const				{ return m_critterAiArray; }
	const Uint32 &						GetBoidCountInEffector()const			{ return m_boidCountInEffector; }
};
