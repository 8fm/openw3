#include "build.h"
#include "r4PlayerController.h"

IMPLEMENT_ENGINE_CLASS( CR4LocomotionDirectController );
IMPLEMENT_ENGINE_CLASS( CR4LocomotionDirectControllerScript );

////////////////////////////////////////////////////////////////////////
// CR4LocomotionDirectController
////////////////////////////////////////////////////////////////////////
CR4LocomotionDirectController::CR4LocomotionDirectController()
	: m_moveSpeed( 0.f )
	, m_moveRotation( 0.f )
{}
CR4LocomotionDirectController::~CR4LocomotionDirectController()
{} 

void CR4LocomotionDirectController::Update()
{
	CMovingAgentComponent* agent = m_agent.Get();
	UpdateLocomotion();
	//agent->RequestMoveSpeed( m_moveSpeed );
	//agent->SetMoveSpeedRel( m_moveSpeed );
	agent->SetMoveRotation( m_moveRotation );
}
Bool CR4LocomotionDirectController::Activate()
{
	return true;
}
void CR4LocomotionDirectController::Deactivate()
{}
////////////////////////////////////////////////////////////////////////
// CR4LocomotionDirectControllerScript
////////////////////////////////////////////////////////////////////////
CR4LocomotionDirectControllerScript::CR4LocomotionDirectControllerScript()
{}
CR4LocomotionDirectControllerScript::~CR4LocomotionDirectControllerScript()
{}

void CR4LocomotionDirectControllerScript::UpdateLocomotion()
{
	CallFunction( this, CNAME( UpdateLocomotion ) );
}
Bool CR4LocomotionDirectControllerScript::Activate()
{
	Bool result = false;
	CallFunctionRet< Bool >( this, CNAME( Activate ), result );
	return result;
}
void CR4LocomotionDirectControllerScript::Deactivate()
{
	CallFunction( this, CNAME( Deactivate ) );
}