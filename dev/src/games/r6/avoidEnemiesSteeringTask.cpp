#include "build.h"

#include "avoidEnemiesSteeringTask.h"

#include "enemyAwareComponent.h"
#include "../../common/game/movementCommandBuffer.h"
#include "../../common/game/movementGoal.h"

IMPLEMENT_ENGINE_CLASS( CMoveSTAvoidEnemies );

CMoveSTAvoidEnemies::CMoveSTAvoidEnemies()
	: m_headingImportance( 0.9f )
	, m_minDistanceFromEnemies( 3 )
	, m_overrideSteering( true )
{

}


void CMoveSTAvoidEnemies::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	const Float TIGHTSPOT_SIZE = 0.25f;

	const SMoveLocomotionGoal& goal = comm.GetGoal();

	if ( !goal.IsHeadingGoalSet() )
	{
		// don't create some special additional movement when no heading is set by the goal
		return;
	}

	CMovingAgentComponent& moveAgent = comm.GetAgent();
	CEntity* entity = moveAgent.GetEntity();
	CEnemyAwareComponent* enemyAwareComponent = entity->FindComponent< CEnemyAwareComponent >( );

	Vector enemyPosition;
	if( !enemyAwareComponent || !enemyAwareComponent->GetNearestKnownPosition( enemyPosition ) )
		return;

	Vector enemyDirection = enemyPosition - entity->GetWorldPosition();
	if( enemyDirection.SquareMag3() > m_minDistanceFromEnemies*m_minDistanceFromEnemies )
		return;

	enemyDirection.Normalize3();

	Vector newHeading( -enemyDirection.Y, enemyDirection.X, enemyDirection.Z);
	
	if( m_overrideSteering )
	{
		comm.OverrideHeading( newHeading, m_headingImportance );
	}
	else
	{
		comm.AddHeading( newHeading, m_headingImportance );
	}
}

String CMoveSTAvoidEnemies::GetTaskName() const
{
	static const String TASKNAME( TXT( "Keep away enemies" ) );
	return TASKNAME;
}