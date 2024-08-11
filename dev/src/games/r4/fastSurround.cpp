#include "build.h"
#include "fastSurround.h"

#include "combatDataComponent.h"
#include "../../common/game/movementCommandBuffer.h"
#include "../../common/game/movementGoal.h"
#include "../../common/core/instanceDataLayoutCompiler.h"

IMPLEMENT_ENGINE_CLASS( CMoveSCFastSurround );
IMPLEMENT_ENGINE_CLASS( CMoveSCTargetsCount );

///////////////////////////////////////////////////////////////////////////////
// CMoveSCFastSurround
///////////////////////////////////////////////////////////////////////////////
CMoveSCFastSurround::CMoveSCFastSurround()
	: m_usageDelay( 6.f )
	, m_angleDistanceToActivate( 50.f )
	, m_speedMinToActivate( 0.5f )
	, m_angleDistanceToBreak( 15.0f )
	, m_speedMinLimitToBreak( 0.5f )	
{

}
Bool CMoveSCFastSurround::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	if( data[ i_isFastSurrounding ] )
	{
		const SMoveLocomotionGoal& goal = comm.GetGoal();
		Float angleDiff;
		Bool breakSurround = false;
		if ( !goal.GetFlag( CNAME( DesiredTargetYawDifference ), angleDiff ) || Abs( angleDiff ) < m_angleDistanceToBreak )
		{
			breakSurround = true;
		}
		else
		{
			// test if we should keep the current speed
			const Vector2& steeringHeading = comm.GetHeading();
			Float steeingSpeed = comm.GetSpeed();
			Float desiredSpeedSq = steeringHeading.SquareMag() * steeingSpeed * steeingSpeed;

			if ( desiredSpeedSq < m_speedMinLimitToBreak*m_speedMinLimitToBreak )
			{
				breakSurround = true;
			}
		}
		if ( breakSurround )
		{
			data[ i_isFastSurrounding ] = false;
			data[ i_nextActivationDelay ] = GGame->GetEngineTime() + m_usageDelay;
		}
	}
	else
	{
		EngineTime currentTime = GGame->GetEngineTime();
		if ( data[ i_nextActivationDelay ] < currentTime )
		{
			const SMoveLocomotionGoal& goal = comm.GetGoal();
			Float angleDiff;
			if ( goal.GetFlag( CNAME( DesiredTargetYawDifference ), angleDiff ) && Abs( angleDiff ) > m_angleDistanceToActivate )
			{
				const Vector2& steeringHeading = comm.GetHeading();
				Float steeingSpeed = comm.GetSpeed();
				Float desiredSpeedSq = steeringHeading.SquareMag() * steeingSpeed * steeingSpeed;

				if ( desiredSpeedSq > m_speedMinToActivate*m_speedMinToActivate )
				{
					data[ i_isFastSurrounding ] = true;
					data[ i_isFastSurroundingSince ] = currentTime;
				}
			}
		}
	}
	return data[ i_isFastSurrounding ];
}

String CMoveSCFastSurround::GetConditionName() const
{
	return TXT("Fast surround");
}

void CMoveSCFastSurround::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_isFastSurrounding;
	compiler << i_hasLowVelocity;
	compiler << i_nextActivationDelay;
	compiler << i_lowVelocityBreakDelay;
	compiler << i_isFastSurroundingSince;
}
void CMoveSCFastSurround::OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	TBaseClass::OnInitData( agent, data );

	data[ i_isFastSurrounding ] = false;
	data[ i_hasLowVelocity ] = false;
	data[ i_nextActivationDelay ] = EngineTime::ZERO;
	data[ i_lowVelocityBreakDelay ] = EngineTime::ZERO;
	data[ i_isFastSurroundingSince ] = EngineTime::ZERO;
}
void CMoveSCFastSurround::OnDeinitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	TBaseClass::OnDeinitData( agent, data );
}
///////////////////////////////////////////////////////////////////////////////
// CMoveSCTargetsCount
///////////////////////////////////////////////////////////////////////////////
Bool CMoveSCTargetsCount::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	const SMoveLocomotionGoal& goal = comm.GetGoal();
	CNode* targetNode = goal.GetGoalTargetNode();
	if ( !targetNode )
	{
		return false;
	}
	//////////////////
	// TODO: OPTIMIZE!
	CActor* targetActor = Cast< CActor >( targetNode );
	if ( !targetActor )
	{
		return false;
	}
	CCombatDataPtr targetDataPtr( targetActor );
	CCombatDataComponent* targetData = targetDataPtr.Get();
	if ( !targetData )
	{
		return false;
	}
	//////////////////
	Uint32 count = targetData->GetAttackersCount();
	return count > m_count;
}

String CMoveSCTargetsCount::GetConditionName() const
{
	return TXT("Targets count is greater");
}