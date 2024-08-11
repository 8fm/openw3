#include "build.h"
#include "applySteeringToPlayer.h"

#include "../../common/game/movementCommandBuffer.h"
#include "../../common/game/movementGoal.h"

IMPLEMENT_ENGINE_CLASS( CMoveSTApplySteeringToPlayerVariables );

///////////////////////////////////////////////////////////////////////////////
// CMoveSTApplySteeringToPlayerVariables
///////////////////////////////////////////////////////////////////////////////

//RED_DEFINE_STATIC_NAME( playerSpeed );
RED_DEFINE_STATIC_NAME( isSprinting );
RED_DEFINE_STATIC_NAME( playerDir );
RED_DEFINE_STATIC_NAME( playerInputAngSpeed );
RED_DEFINE_STATIC_NAME( rawPlayerHeading );
RED_DEFINE_STATIC_NAME( requestedMovementDirection );
RED_DEFINE_STATIC_NAME( requestedFacingDirection );

String CMoveSTApplySteeringToPlayerVariables::GetTaskName() const
{
	static const String TASKNAME( TXT( "ApplySteering to Player (variables)" ) );
	return TASKNAME;
}

void CMoveSTApplySteeringToPlayerVariables::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	CMovingAgentComponent& agent = comm.GetAgent();
	Float agentYaw = agent.GetWorldYaw();
	if ( comm.IsSpeedSet() )
	{
		agent.ForceSetRelativeMoveSpeed( comm.GetSpeed() );
		//comm.SetVar( BVC_Extra, CNAME( playerSpeed ), comm.GetSpeed() );
		comm.SetVar( BVC_Extra, CNAME( isSprinting ), 0.0f );
	}
	if ( comm.IsHeadingSet() )
	{
		Vector2 heading = comm.GetHeading();
		Float yaw = EulerAngles::YawFromXY( heading.X, heading.Y );
		Float playerDir = EulerAngles::NormalizeAngle180( yaw - agentYaw ); // go to -180 to 180 (right to left)
		playerDir = playerDir / 180.0f; // normalize
		playerDir = -playerDir; // -1 to 1 left to right
		comm.SetVar( BVC_Extra, CNAME( playerDir ), playerDir ); // -1 to 1 left to right
		comm.SetVar( BVC_Extra, CNAME( playerInputAngSpeed ), 0.0f ); // not used?
		comm.SetVar( BVC_Extra, CNAME( rawPlayerHeading ), 0.0f ); // not used?
		comm.SetVar( BVC_Extra, CNAME( requestedMovementDirection ), EulerAngles::NormalizeAngle( yaw ) ); // WS 0-360
	}
	if ( comm.IsRotationSet() )
	{
		Float rotation = comm.GetRotation();
		comm.SetVar( BVC_Extra, CNAME( requestedFacingDirection ), EulerAngles::NormalizeAngle( agentYaw + rotation ) ); // WS 0-360
	}
}
