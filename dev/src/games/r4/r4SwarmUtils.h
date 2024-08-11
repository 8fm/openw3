#ifndef r4SwarmUtils
#define r4SwarmUtils

#include "../../common/game/swarmUtils.h"
#include "../../Common/Game/scriptRegistrationManager.h"

#define SWARM_COLLISION_TEST_RADIUS 5.0f

// Birds have got some bird specific boid states
enum EFlyingBoidState
{
	FLYING_BOID_STATE_GLIDE		= BOID_STATE_LAST,
	FLYING_BOID_STATE_LAST		= FLYING_BOID_STATE_GLIDE,	
};


enum EHumbleCritterState
{
	//HUMBLE_STATE_NOT_SPAWNED,
	HUMBLE_STATE_IDLE		= CRITTER_STATE_COUNT,
	HUMBLE_STATE_WANDER,
	HUMBLE_STATE_ACTION,
	HUMBLE_STATE_PANIC,
	HUMBLE_STATE_DIE_BURN_SHAKE,
	HUMBLE_STATE_FADEOUT,
	HUMBLE_STATE_GO_DESPAWN,

	HUMBLE_STATES_COUNT
};


enum EFlyingCritterState
{
	//FLYING_STATE_NOT_SPAWNED,
	FLYING_STATE_IDLE		= CRITTER_STATE_COUNT,
	FLYING_STATE_BURN,
	FLYING_STATE_GO_DESPAWN,
	FLYING_STATES_COUNT
};

enum ESwarmSounds
{
	SWARM_SOUND_MOVE_IDLE,
	SWARM_SOUND_MOVE_ATTACK,
	SWARM_SOUND_DIE,
	SWARM_SOUND_BEING_TOASTED,
	SWARM_SOUND_COUNT
};


class CGroupState;
typedef TDynArray<CGroupState*>	CGroupState_Array;
class CPoiConfigByGroup;
typedef  TDynArray< const CPoiConfigByGroup * > CPoiConfigByGroup_CPointerArray;;


class CR4SwarmScriptRegistration : public CScriptRegistration
{
public:
	CR4SwarmScriptRegistration();
	void RegisterScriptFunctions()const override;

	static CR4SwarmScriptRegistration s_r4SwarmScriptRegistration;

	void PreventOptimisedAway(){ m_unused = true; }
private:
	Bool m_unused;
};

class CSwarmCellMap;
class R4SwarmUtils
{
public:
	static Bool FindCellToClearPath( const Vector3 & position, const Vector & displacementVector, const CSwarmCellMap *cellMap, Vector3 & freeCellPosition );
	static CDirectory*const GetDataDirectory();
};

#endif