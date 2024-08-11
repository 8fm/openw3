#include "build.h"
#include "moveSteeringMaintainTargetSpeed.h"
#include "movementCommandBuffer.h"
#include "movingAgentComponent.h"
#include "player.h"

IMPLEMENT_ENGINE_CLASS( CMoveSTMaintainTargetSpeed )
IMPLEMENT_ENGINE_CLASS( CMoveSTSaneMaintainTargetSpeed )

///////////////////////////////////////////////////////////////////////////////
// CMoveSTSaneMaintainTargetSpeed
///////////////////////////////////////////////////////////////////////////////
void CMoveSTSaneMaintainTargetSpeed::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	if ( CActor* target = Cast< CActor >( comm.GetGoal().GetGoalTargetNode() ) )
	{
		if ( CMovingAgentComponent* targetMac = target->GetMovingAgentComponent() )
		{
			const CMovingAgentComponent& agent = comm.GetAgent();
			// Getting target abs speed
			const Float targetAbsoluteSpeed = targetMac->GetAbsoluteMoveSpeed();

			// Mapping target abs speed into my own relative speed and then into steering graph speed
			const Float speed = Min( agent.ConvertSpeedAbsToRel( targetAbsoluteSpeed ) / agent.GetSpeedRelSpan(), 1.0f );

			comm.AddSpeed( speed, m_speedImportance );
		}
	}
}

String CMoveSTSaneMaintainTargetSpeed::GetTaskName() const
{
	return TXT( "MaintainTargetSpeed" );
}
///////////////////////////////////////////////////////////////////////////////
// CMoveSTMaintainTargetSpeed
///////////////////////////////////////////////////////////////////////////////
Bool CMoveSTMaintainTargetSpeed::ShouldMaintainTargetSpeed( IMovementCommandBuffer& comm )const
{
	Bool maintainTargetSpeed = false;
	comm.GetGoal().GetFlag( CNAME( MaintainTargetSpeed ), maintainTargetSpeed );

	return maintainTargetSpeed;
}
void CMoveSTMaintainTargetSpeed::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	Float newSpeed = 1.0f;
	if ( m_speedImportance > 0.f )
	{
		if ( ShouldMaintainTargetSpeed( comm ) )
		{
			CNode* companionNode				= comm.GetGoal().GetGoalTargetNode();
			CActor* companionActor				= Cast< CActor >( companionNode );
			CMovingAgentComponent* companionMAC	= companionActor ? companionActor->GetMovingAgentComponent() : nullptr;
			if ( !companionMAC )
			{
				return;
			}

			const CMovingAgentComponent& agent = comm.GetAgent();

			// getting potential offset from decision logic
			Float companionOffset = 0.0f;
			comm.GetGoal().GetFlag( CNAME( CompanionOffset ), companionOffset );

			const Vector2 agentHeading = agent.GetHeading().AsVector2().Normalized();

			const Vector agentPositionWithOffset = agent.GetWorldPositionRef() + agentHeading * companionOffset;
			// Getting target abs speed
			const Float companionAbsoluteSpeed	= companionMAC->GetAbsoluteMoveSpeed();
			const Float agentAbsoluteSpeed = agent.GetAbsoluteMoveSpeed();

			const Vector2 companionVelocity = companionMAC->GetHeading().AsVector2().Normalized() * companionAbsoluteSpeed;
			const Vector2 agentVelocity = agentHeading * agentAbsoluteSpeed;

			Float ratio = ( agentVelocity.Dot( companionVelocity ) / agentAbsoluteSpeed) / agentAbsoluteSpeed;
			ratio = Clamp( ratio, 0.0f, 2.0f);

			const Float distance = agentPositionWithOffset.DistanceTo( companionMAC->GetWorldPositionRef() );
			const Float distMultiplier = Clamp( distance * m_distanceCoefficient, 0.0f, 1.0f );

			Float sign = MSign( MathUtils::GeometryUtils::PerpendicularR( agent.GetHeading() ).CrossZ( agentPositionWithOffset - companionMAC->GetWorldPositionRef() ) );
			Float targetAbsoluteSpeedWithOffset	= agentAbsoluteSpeed * ratio * ( 1.0f - sign * distMultiplier );

			if ( sign < 0.0f && targetAbsoluteSpeedWithOffset <= NumericLimits< Float >::Epsilon() )
			{
				targetAbsoluteSpeedWithOffset = agent.GetMaxSpeed() * distMultiplier;
			}

			// Mapping target abs speed into my own relative speed and then into steering graph speed
			const Float targetSpeedExpressedInMySteerGraphSpeed	= Min( agent.ConvertSpeedAbsToRel( targetAbsoluteSpeedWithOffset ) / agent.GetSpeedRelSpan(), 1.0f );

			// Damping this a bit because Npc should not react to my speed changes too fast
			const Float & previousVelocity		= data[ i_previousVelocity ];
			const Float diff					= targetSpeedExpressedInMySteerGraphSpeed - previousVelocity;

			const Float diffPerSecond			= Abs( diff ) / timeDelta;
			newSpeed = targetSpeedExpressedInMySteerGraphSpeed;

			if ( diffPerSecond > m_allowedDiffPerSecond )
			{
				newSpeed = previousVelocity + Sgn( diff ) * m_allowedDiffPerSecond * timeDelta;
			}

			// If actor speed is below threshold and speed is decreasing then stop actor
			// m_offsetSpeed needs to be removed here because the threshold would never trigger
			if ( newSpeed < m_stopSpeedThreshold && diff <= 0.0f )
			{
				newSpeed = 0.0f;
			}
		}
		else
		{
			const SMoveLocomotionGoal& goal = comm.GetGoal();
			const CMovingAgentComponent& agent = comm.GetAgent();
			newSpeed = goal.IsSpeedGoalSet() ? ( goal.GetDesiredSpeed() / agent.GetMaxSpeed() ) : 1.f;
		}

		ASSERT( newSpeed <= 1.0f, TXT("Speed should not be above 1 here !") );
		comm.AddSpeed( newSpeed, m_speedImportance );
	}
	data[ i_previousVelocity ]					= comm.GetSpeed();
}

String CMoveSTMaintainTargetSpeed::GetTaskName() const
{
	return TXT("MaintainTargetSpeed at PredefinedPath");
}

void CMoveSTMaintainTargetSpeed::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );
	compiler << i_previousVelocity;	
}

void CMoveSTMaintainTargetSpeed::OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	TBaseClass::OnInitData( agent, data );
	data[ i_previousVelocity ] = 0.0f;
}
