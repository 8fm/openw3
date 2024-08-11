#include "build.h"
#include "r4PlayerLocomotionSegment.h"

#include "r4PlayerController.h"


CR4DirectControlLocomotionSegment::CR4DirectControlLocomotionSegment( const THandle< CR4LocomotionDirectController >& controller )
	: m_controller( controller )
{

}
CR4DirectControlLocomotionSegment::~CR4DirectControlLocomotionSegment()
{
	// 
}

Bool CR4DirectControlLocomotionSegment::Activate( CMovingAgentComponent& agent )
{
	if ( !m_controller->Activate() )
	{
		return false;
	}

	return true;
}

// updates the segment
ELSStatus CR4DirectControlLocomotionSegment::Tick( Float timeDelta, CMovingAgentComponent& agent )
{
	m_controller->Update();
	return LS_InProgress;
}

// Deactivates the segment
void CR4DirectControlLocomotionSegment::Deactivate( CMovingAgentComponent& agent )
{
	m_controller->Deactivate();
}

// Checks if the segment can be interrupted
Bool CR4DirectControlLocomotionSegment::CanBeCanceled() const
{
	return true;
}

// Serialization support
void CR4DirectControlLocomotionSegment::OnSerialize( IFile& file )
{
	Super::OnSerialize( file );
}

// Debug draw
void CR4DirectControlLocomotionSegment::GenerateDebugFragments( CMovingAgentComponent& agent, CRenderFrame* frame )
{}
void CR4DirectControlLocomotionSegment::GenerateDebugPage( TDynArray< String >& debugLines ) const
{}