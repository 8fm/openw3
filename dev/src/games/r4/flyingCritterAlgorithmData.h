#pragma once

#include "../../common/game/swarmAlgorithmData.h"
#include "../../common/game/swarmsDensityMap.h"
#include "r4SwarmUtils.h"
#include "flyingCrittersLairEntity.h"
#include "flyingCrittersParams.h"
#include "swarmCellMap.h"

class CFlyingCrittersLairEntity;
class CFlyingCritterAI;
struct SSwarmMemberStateData;
class CPointOfInterestSpeciesConfig;
class CFlyingPoiConfig;
struct CFlyingGroupId;
class CCreateFlyingGroupRequest;
class CMoveBoidToGroupRequest;
class CSwarmCellMap;

class CFlyingCrittersAlgorithmData : public CSwarmAlgorithmData
{
	typedef CSwarmAlgorithmData Super;
	typedef CFlyingCrittersLairEntity Lair;
public:
	/*struct SoundData
	{
		SoundData()
			: m_soundCenter( 0, 0, 0 )
			, m_soundIntesity( 0.f )
			, m_radius( 0.f )
			, m_maxDistanceSq( 20.f * 20.f )		{}
		Vector				m_soundCenter;
		Float				m_soundIntesity;
		Float				m_radius;
		Float				m_maxDistanceSq;
	};

	enum ECellDataFlags
	{
		CDF_Blocked					1<<0,
		CDF_FLOOD_FILL_EXPLORED		1<<1,
	};
	struct EnviromentCelData
	{
		Vector2				m_wallPotential;
		Float				m_z;
		Uint32				m_flags;
		//Bool				m_isBlocked;
		mutable Int16		m_guyzCounter;
	};
	typedef CSwarmDensityMap< EnviromentCelData > EnviromentData;*/

	struct TemporaryPoi
	{
		Boids::PointOfInterestType				m_poiType;
		Boids::PointOfInterestId				m_poiId;
		Float									m_fadeoutTime;
	};

	CFlyingCrittersAlgorithmData( CFlyingCrittersLairEntity* lair, const CFlyingCritterLairParams & params, CFlyingSwarmScriptInput * scriptInput );
	~CFlyingCrittersAlgorithmData();

	void Initialize( CSwarmLairEntity& lair ) override;

	void UpdateMovementAndAnimation( const TSwarmStatesList currentState, TSwarmStatesList targetState )override;
	void UpdateSound(  const TSwarmStatesList currentStateList, TSwarmStatesList targetStateList )override;
	void CalculateSound( TSwarmStatesList crittersStates, CSwarmSoundJobData *const  soundJobData, CFlyingSwarmGroup & group );
	void PreUpdateSynchronization( Float deltaTime ) override;
	void PostUpdateSynchronization( ) override;
	void FillPoiJobDataFromDynamicPoi( CPoiJobData & poiJobData, const CName& filterTag, CEntity*const entity )override;

	void								SetCellMap( CSwarmCellMap *cellMap )			{ m_cellMap = cellMap; }
	const CPoiJobData_Array&			GetPlayerList() const							{ return *m_playerList; }
	const CFlyingCritterLairParams &	GetLairParams()const							{ return *(CFlyingCritterLairParams *)m_params; }

	void OnBoidActivated()										{ ++m_activeBoids; }
	void OnBoidDeactivated()									{ --m_activeBoids; }
	void OnBoidBorn()											{ ++m_aliveBoids; }
	void OnBoidDied()											{ --m_aliveBoids; }
	Int32 GetActiveBoids() const								{ return m_activeBoids; }

	Boids::PointOfInterestId AddTemporaryPoi( Boids::PointOfInterestType poiType, CPoiJobData& poiData, Float fadeoutDelay );
	void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );
protected:

	void CalculateEnviromentData( CFlyingCrittersLairEntity& lair );

	Uint32							m_breakCounter;

	Int32							m_activeBoids;
	Int32							m_aliveBoids;
	Int32							m_visibleBoidCound;

	THandle<CSwarmCellMap>			m_cellMap;

	CPoiJobData_Array*				m_playerList;

	Bool							m_isLairDefeated;
	Bool							m_despawningAll;
	Bool							m_despawnAllPending;
	Float							m_despawnAllTime;

	Boids::PointOfInterestId		m_fireInConePoiID_A;
	Boids::PointOfInterestId		m_fireInConePoiID_B;

	Box								m_lairBoundingBox;

	CFlyingSwarmScriptInput *		m_scriptInput;

	TStaticArray< TemporaryPoi, 4 >		m_temporaryPois;
	Uint32								m_nextPoiId;

	// Used for spawn to mark were we are in the dynamic spawn process
	Uint32							m_nextBoidIndexToSpawn;

	// idle target variables :
	Float			m_maxDist_IdleTarget;
	Float			m_minDist_IdleTarget;
	Float			m_groupForceMult_IdleTarget;
	Float			m_maxDistToAltitude_IdleTarget;
	Float			m_altitudeForceMult_IdleTarget;
	Float			m_collisionForceMult_IdleTarget;
	Float			m_maxVel_IdleTarget;
	Float			m_randomForceMult_IdleTarget;

	Float			m_jobTime;
private:
	/// Helper functions :
	void UpdateIdleTargets();
	void ComputeAltitudeForce( Vector3 & altitudeForce, const Vector3 &idleTargetPosition, Float randomAltitude );
	Boids::PointOfInterestId HandleFireInConePoi( CPoiJobData & poiJobData, CPoiJobData *const oldFirePointData );
	const CPoiJobData_Array *const GetPoiArrayFromType( CName poiType )const;
	void CreateGroup( const CCreateFlyingGroupRequest *const createFlyingGroupRequest );
	void RemoveGroup( const CFlyingGroupId *const groupID );
	void MoveBoidToGroup( const CMoveBoidToGroupRequest *const request );
};
