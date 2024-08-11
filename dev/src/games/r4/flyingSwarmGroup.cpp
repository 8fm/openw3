#include "build.h"
#include "flyingSwarmGroup.h"


IMPLEMENT_ENGINE_CLASS( CFlyingGroupId );
IMPLEMENT_ENGINE_CLASS( CFlyingSwarmGroup );
IMPLEMENT_ENGINE_CLASS( CFlyingSwarmScriptInput );

CFlyingGroupId::CFlyingGroupId( Uint32 id )
		: m_id( id )		
{
}
void funcFlyingGroupIdCompare( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CFlyingGroupId, groupIdA, CFlyingGroupId() );
	GET_PARAMETER( CFlyingGroupId, groupIdB, CFlyingGroupId() );
	FINISH_PARAMETERS;
	if ( groupIdA.m_id == groupIdB.m_id )
	{
		RETURN_BOOL( true );
		return;
	}
	RETURN_BOOL( false );
}

void funcFlyingGroupIdIsValid( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CFlyingGroupId, groupId, CFlyingGroupId() );
	FINISH_PARAMETERS;
	if ( groupId.m_id == (Uint32)-1 )
	{
		RETURN_BOOL( false );
		return;
	}
	RETURN_BOOL( true );
}

CFlyingGroupId	CFlyingSwarmGroup::m_groupIDCounter = CFlyingGroupId( 0 );

CFlyingSwarmGroup::CFlyingSwarmGroup()
		: m_groupState( NULL )
		, m_idleTarget()
		, m_currentGroupStateIndex( (Uint32)-1 )
		, m_despawnedInDeactivationCount( 0 )
		, m_groupId( )
		, m_groupCenter( 0.0f, 0.0f, 0.0f )
		, m_targetPosition( 0.0f, 0.0f, 0.0f )
		, m_currentGroupState( CName::NONE )
		, m_boidCount( 0 )
		, m_toSpawnCount( 0 )
		, m_spawnPoiType( CName::NONE )
		, m_toDespawnCount( 0 )
		, m_despawnPoiType( CName::NONE )
		, m_changeGroupState( CName::NONE )		 
		, m_waterLevel( 10.0f )
{
	m_groupId	= CFlyingSwarmGroup::m_groupIDCounter;
	CFlyingSwarmGroup::m_groupIDCounter.m_id++;
}

CFlyingSwarmScriptInput::CFlyingSwarmScriptInput()
{

}
CFlyingSwarmGroup *const CFlyingSwarmScriptInput::GetGroupFromId( const CFlyingGroupId & groupId )
{
	for ( Uint32 i = 0; i < m_groupList.Size(); ++i )
	{
		CFlyingSwarmGroup *const flyingSwarmGroup = &m_groupList[ i ];
		if ( flyingSwarmGroup->m_groupId == groupId )
		{
			return flyingSwarmGroup;
		}
	}
	return NULL;
}
void CFlyingSwarmScriptInput::CreateGroup( Int32 toSpawnCount, CName spawnPoiType, CName groupState, CFlyingGroupId fromOtherGroup_Id )
{
	m_createFlyingGroupRequestArray.PushBack( CCreateFlyingGroupRequest( toSpawnCount, spawnPoiType, groupState, fromOtherGroup_Id ) );
}
void CFlyingSwarmScriptInput::RemoveGroup(  const CFlyingGroupId &groupId )
{
	m_removeGroupRequestArray.PushBack( groupId );
}
void CFlyingSwarmScriptInput::MoveBoidToGroup( const CFlyingGroupId &groupdA, Int32 boidCount, const CFlyingGroupId &groupIdB )
{
	m_moveBoidToGroupRequestArray.PushBack( CMoveBoidToGroupRequest( groupdA, boidCount, groupIdB ) );
}

void CFlyingSwarmScriptInput::funcCreateGroup( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, toSpawnCount, 0 );
	GET_PARAMETER( CName, spawnPoiType, CName::NONE );
	GET_PARAMETER( CName, groupState, CName::NONE );
	GET_PARAMETER( CFlyingGroupId, fromOtherGroup_Id, CFlyingGroupId() );
	FINISH_PARAMETERS;

	CreateGroup( toSpawnCount, spawnPoiType, groupState, fromOtherGroup_Id );
}
void CFlyingSwarmScriptInput::funcRemoveGroup( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CFlyingGroupId, groupId, CFlyingGroupId() );
	FINISH_PARAMETERS;
	RemoveGroup( groupId );
}
void CFlyingSwarmScriptInput::funcMoveBoidToGroup( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CFlyingGroupId, groupIdA, CFlyingGroupId() );
	GET_PARAMETER( Int32, count, 0 );
	GET_PARAMETER( CFlyingGroupId, groupIdB, CFlyingGroupId() );
	FINISH_PARAMETERS;
	
	MoveBoidToGroup( groupIdA, count, groupIdB );
}

