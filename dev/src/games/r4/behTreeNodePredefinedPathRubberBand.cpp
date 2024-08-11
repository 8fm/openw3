#include "build.h"
#include "behTreeNodePredefinedPathRubberBand.h"
#include "../../common/game/movementGoal.h"
#include "../../common/engine/pathComponent.h"


//////////////////////////////////////////////////////////////////////////
// CBehTreeNodePredefinedPathRuberBandDefinition
//////////////////////////////////////////////////////////////////////////
IBehTreeNodeInstance* CBehTreeNodePredefinedPathRuberBandDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}


//////////////////////////////////////////////////////////////////////////
// CBehTreeNodePredefinedPathRuberBandInstance
//////////////////////////////////////////////////////////////////////////
CBehTreeNodePredefinedPathRuberBandInstance::CBehTreeNodePredefinedPathRuberBandInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodePredefinedPathInstance( def, owner, context, parent )
{

}

// IMovementTargeter interface
void CBehTreeNodePredefinedPathRuberBandInstance::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{
	Super::UpdateChannels( agent, goal, timeDelta );

	CEntity *const target				= GGame->GetPlayerEntity();
	const Vector & targetPosition		= target->GetWorldPositionRef();

	CPathComponent* pathComponent = m_pathComponent.Get();
	ASSERT( pathComponent );
	Int32 edgeIdx;
	Float edgeAlpha;
	pathComponent->GetClosestPointOnPath( targetPosition, edgeIdx, edgeAlpha, 0.5f );
	const Float targetDistanceFromStart = pathComponent->CalculateDistanceFromStart( edgeIdx, edgeAlpha );
	const Float distanceToTarget		= m_distanceFromStart - targetDistanceFromStart;

	goal.SetFlag( CNAME( RubberBandDistance ), distanceToTarget );
	goal.SnapToMinimalSpeed( true );

}

