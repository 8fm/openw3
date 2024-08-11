#pragma once

#include "r4SwarmUtils.h"
#include "../../common/game/poiJobData.h"


////////////////////////////////////////////////////
// CFlyingGroupID
// This is a wrapper around and int whose sole purpose it to 
// keep the scripter from doing mistakes
struct CFlyingGroupId
{
	DECLARE_RTTI_STRUCT( CFlyingGroupId );
public:
	Uint32 m_id;

	CFlyingGroupId( Uint32 id = (Uint32)-1 );

	Bool operator==( const CFlyingGroupId & groupID )const
	{
		if ( groupID.m_id == m_id )
		{
			return true;
		}
		return false;
	}
};
BEGIN_CLASS_RTTI( CFlyingGroupId );
END_CLASS_RTTI();

// function to compare CFlyingGroupId together
void funcFlyingGroupIdCompare( IScriptable* context, CScriptStackFrame& stack, void* result );
// function returning wether or not a group as been initialised or not
void funcFlyingGroupIdIsValid( IScriptable* context, CScriptStackFrame& stack, void* result );


typedef TDynArray< CFlyingGroupId > CFlyingGroupId_Array;

/////////////////////////////////////////////////////
// Idle target
class CIdleTarget
{
public:
	Vector3		m_velocity;
	Float		m_centerForceMult;
	Float		m_changeMultTimer;
	Vector3		m_randomForce;
	// Because the area is often very large groups don't really have to avoid each other
	// on the Z axis so we put a random force instead 
	Float		m_randomAltitude; 
	CPoiJobData	m_poiJobData;

	// here for debug
	Vector3		m_collisionForce;

	CIdleTarget()
		: m_velocity( 0.0f, 0.0f, 0.0f )
		, m_centerForceMult( 1.0f )
		, m_changeMultTimer( 0.0f )	
		, m_randomAltitude( 0.0f )
		, m_randomForce( 0.0f, 0.0f, 0.0f )
		, m_collisionForce( 0.0f, 0.0f, 0.0f )
	{
		m_poiJobData.m_cpntParams.m_gravityRangeMax	= -1.0f;
		m_poiJobData.m_cpntParams.m_type = CNAME( MovingIdleTarget );
	}
};

//////////////////////////////////////////////
//  CFlyingSwarmGroup
class CGroupState;
class CFlyingSwarmGroup
{
	DECLARE_RTTI_SIMPLE_CLASS( CFlyingSwarmGroup );
public:
	TDynArray<Uint32>	m_boidIndexArray;
	CGroupState	*		m_groupState;
	CIdleTarget			m_idleTarget;
	Uint32				m_currentGroupStateIndex;
	// used for reactivation of lair :
	Uint32				m_despawnedInDeactivationCount;	

	// used to generate m_groupID
	static CFlyingGroupId		m_groupIDCounter;

	// All that follows is accessible by script :

	// general info exposed to script
	CFlyingGroupId		m_groupId;	// unique ID to identify the group
	Vector				m_groupCenter;
	Vector				m_targetPosition;
	CName				m_currentGroupState;
	Int32				m_boidCount;

	// Spawn request parameters :
	Int32	m_toSpawnCount;
	CName	m_spawnPoiType;

	// Despawn request parameters :
	Int32	m_toDespawnCount;
	CName	m_despawnPoiType;

	// Change group State 
	CName				m_changeGroupState;

	// water level above the groups centre:
	Float				m_waterLevel;
	

	CFlyingSwarmGroup();
};
BEGIN_CLASS_RTTI( CFlyingSwarmGroup );
	PROPERTY( m_groupId );
	PROPERTY( m_groupCenter );
	PROPERTY( m_targetPosition );
	PROPERTY( m_currentGroupState );
	PROPERTY( m_boidCount );
	PROPERTY( m_toSpawnCount );
	PROPERTY( m_spawnPoiType );
	PROPERTY( m_toDespawnCount );
	PROPERTY( m_despawnPoiType );
	PROPERTY( m_changeGroupState );
END_CLASS_RTTI();

typedef TDynArray< CFlyingSwarmGroup >			CFlyingSwarmGroup_Array;


//////////////////////////////////////////////////////
/// CCreateFlyingGroupRequest
class CCreateFlyingGroupRequest
{
public:
	CName			m_groupState;
	Int32			m_toSpawnCount;
	CName			m_spawnPoiType;
	CFlyingGroupId	m_fromOtherGroup_Id;

	CCreateFlyingGroupRequest( Int32 toSpawnCount = 0, CName spawnPoiType = CName::NONE, CName groupState = CName::NONE, CFlyingGroupId fromOtherGroup_Id = CFlyingGroupId() )
		: m_groupState( groupState )
		, m_toSpawnCount( toSpawnCount )
		, m_spawnPoiType( spawnPoiType )
		, m_fromOtherGroup_Id( fromOtherGroup_Id )	{}
};


typedef TDynArray< CCreateFlyingGroupRequest >  CCreateFlyingGroupRequest_Array;


//////////////////////////////////////////////////////
/// CMoveBoidToGroupRequest
class CMoveBoidToGroupRequest
{
public:
	CFlyingGroupId	m_groupIdA;
	Int32			m_count;
	CFlyingGroupId	m_groupIdB;

	CMoveBoidToGroupRequest( CFlyingGroupId groupIdA = CFlyingGroupId(), Int32 count = 0, CFlyingGroupId groupIdB = CFlyingGroupId() )
		: m_groupIdA( groupIdA )
		, m_count( count )
		, m_groupIdB( groupIdB )	{}
};


typedef TDynArray< CMoveBoidToGroupRequest >  CMoveBoidToGroupRequest_Array;

//////////////////////////////////////////////////////
/// CFlyingSwarmScriptInput :
class CFlyingSwarmScriptInput : public CObject
{
	DECLARE_ENGINE_CLASS( CFlyingSwarmScriptInput, CObject, 0 );

	
public:
	CFlyingSwarmGroup_Array		m_groupList;

	void CreateGroup( Int32 toSpawnCount, CName spawnPoiType, CName groupState, CFlyingGroupId fromOtherGroup_Id );
	void RemoveGroup( const CFlyingGroupId &groupId );

	/// moves boidCount boids from groupIdA to groupIdB
	void MoveBoidToGroup( const CFlyingGroupId &groupdA, Int32 boidCount, const CFlyingGroupId &groupIdB );

	CFlyingSwarmGroup *const GetGroupFromId( const CFlyingGroupId & groupId );
	CFlyingSwarmScriptInput();

	CCreateFlyingGroupRequest_Array		m_createFlyingGroupRequestArray;
	CFlyingGroupId_Array				m_removeGroupRequestArray;
	CMoveBoidToGroupRequest_Array		m_moveBoidToGroupRequestArray;
private:
	// script
	void funcCreateGroup	( CScriptStackFrame& stack, void* result );
	void funcRemoveGroup	( CScriptStackFrame& stack, void* result );
	void funcMoveBoidToGroup( CScriptStackFrame& stack, void* result );
};


BEGIN_CLASS_RTTI( CFlyingSwarmScriptInput );
	PARENT_CLASS( CObject );
	PROPERTY( m_groupList );
	NATIVE_FUNCTION( "CreateGroup", funcCreateGroup );
	NATIVE_FUNCTION( "RemoveGroup", funcRemoveGroup );
	NATIVE_FUNCTION( "MoveBoidToGroup", funcMoveBoidToGroup );
END_CLASS_RTTI();
