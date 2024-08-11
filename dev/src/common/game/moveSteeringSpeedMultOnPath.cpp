#include "build.h"
#include "moveSteeringSpeedMultOnPath.h"
#include "movementCommandBuffer.h"
#include "movingAgentComponent.h"


///////////////////////////////////////////////////////////////////////////////
// CMoveSTSpeedMultOnPath
///////////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CMoveSTSpeedMultOnPath )
void CMoveSTSpeedMultOnPath::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	Float speedRel;
	if ( comm.GetGoal().GetFlag( CNAME( SpeedMultOnPath ), speedRel )  )
	{
		speedRel							= Max( speedRel, 0.1f );
		const CMovingAgentComponent& agent	= comm.GetAgent();
		const Float steeringGraphSpeed		= speedRel / agent.GetSpeedRelSpan();
		
		const Float speed = comm.GetSpeed();
		comm.ModifySpeed( speed * steeringGraphSpeed );
	}
}

String CMoveSTSpeedMultOnPath::GetTaskName() const
{
	return TXT("Speed Mult On Path");
}