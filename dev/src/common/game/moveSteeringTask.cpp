#include "build.h"
#include "moveSteeringTask.h"
#include "movementCommandBuffer.h"
#include "movementGoal.h"
#include "movableRepresentationPathAgent.h"
#include "gameplayStorage.h"
#include "actorsManager.h"
#include "..\engine\behaviorGraphUtils.inl"
#include "movementAdjustor.h"

IMPLEMENT_ENGINE_CLASS( IMoveSteeringTask );

///////////////////////////////////////////////////////////////////////////////
// IMoveSteeringTask
///////////////////////////////////////////////////////////////////////////////

void		IMoveSteeringTask::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const		{}
String		IMoveSteeringTask::GetTaskName() const																					{ return TXT("Base Task"); }
void		IMoveSteeringTask::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )											{}
void		IMoveSteeringTask::OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data )										{}
void		IMoveSteeringTask::OnDeinitData( CMovingAgentComponent& agent, InstanceBuffer& data )									{}
void		IMoveSteeringTask::OnGraphActivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const						{}
void		IMoveSteeringTask::OnGraphDeactivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const						{}
void		IMoveSteeringTask::OnBranchDeactivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const						{}
void		IMoveSteeringTask::GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) const				{}
