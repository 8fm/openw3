#include "build.h"
#include "behTreeDecoratorHorseSpeedManager.h"
#include "../../common/game/moveSteeringLocomotionSegment.h"
#include "../../common/game/movementGoal.h"
#include "../../common/game/behTreeInstance.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorHorseSpeedManagerDefinition
IBehTreeNodeDecoratorInstance* CBehTreeDecoratorHorseSpeedManagerDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const 
{
	return new Instance( *this, owner, context, parent );
}

String CBehTreeDecoratorHorseSpeedManagerDefinition::GetNodeCaption() const
{
	return String::Printf( TXT("HorseSpeedManager ( %s )"), CEnum::ToString< EHorseMoveType >( m_moveType.GetValue() ).AsChar() );
}

///////////////////////////////////////////////////
// CBehTreeDecoratorHorseSpeedManagerInstance
CBehTreeDecoratorHorseSpeedManagerInstance::CBehTreeDecoratorHorseSpeedManagerInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent ) 
	, m_moveType( def.m_moveType.GetVal( context ) )
{
	
}

// IMovementTargeter interface
void CBehTreeDecoratorHorseSpeedManagerInstance::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{
	goal.SetFlag( CNAME( HorseMoveType ), m_moveType );
	goal.SnapToMinimalSpeed( true );
}
