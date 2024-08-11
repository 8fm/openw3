#include "build.h"
#include "r4PlayerControlAction.h"

#include "r4PlayerController.h"
#include "r4PlayerLocomotionSegment.h"
#include "r4Player.h"

CR4PlayerControlAction::CR4PlayerControlAction( CR4Player* player )
	: ActorAction( player, ActorAction_R4Reserved_PC )
{

}
CR4PlayerControlAction::~CR4PlayerControlAction()
{

}
Bool CR4PlayerControlAction::Start( const THandle< CR4LocomotionDirectController >& playerController )
{
	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();

	if ( !mac || mac->IsEnabled() == false )
	{
		// the component is turned off - therefore we can't use any move-related functionality
		return false;
	}

	m_controller = playerController;
	m_controller->SetMovingAgent( m_actor->GetMovingAgentComponent() );

	mac->AttachLocomotionController( *this );

	return true;
}
Bool CR4PlayerControlAction::Update( Float timeDelta )
{
	Super::Update( timeDelta );
	return true;
}
void CR4PlayerControlAction::Stop()
{
	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( mac )
	{
		mac->DetachLocomotionController( *this );
	}
	m_controller->SetMovingAgent( NULL );
	m_controller = NULL;
}

String CR4PlayerControlAction::GetDescription() const
{
	static const String STR( TXT("Player action") );
	return STR;
}
void CR4PlayerControlAction::OnGCSerialize( IFile& file )
{
	Super::OnGCSerialize( file );

	file << m_controller;
}
void CR4PlayerControlAction::OnSegmentFinished( EMoveStatus status )
{
	if ( CanUseLocomotion() )
	{
		CR4DirectControlLocomotionSegment* segment = new CR4DirectControlLocomotionSegment( m_controller );
		locomotion().PushSegment( segment );
	}
}
void CR4PlayerControlAction::OnControllerAttached()
{
	CR4DirectControlLocomotionSegment* segment = new CR4DirectControlLocomotionSegment( m_controller );
	locomotion().PushSegment( segment );
}
void CR4PlayerControlAction::OnControllerDetached()
{
	
}
