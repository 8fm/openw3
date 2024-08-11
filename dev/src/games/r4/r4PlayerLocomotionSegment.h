#pragma once

#include "../../common/game/moveLocomotionSegment.h"


class CR4LocomotionDirectController;

class CR4DirectControlLocomotionSegment : public IMoveLocomotionSegment
{
	typedef IMoveLocomotionSegment Super;
protected:
	THandle< CR4LocomotionDirectController >		m_controller;
public:
	CR4DirectControlLocomotionSegment( const THandle< CR4LocomotionDirectController >& controller );
	~CR4DirectControlLocomotionSegment();


	Bool Activate( CMovingAgentComponent& agent ) override;

	// updates the segment
	ELSStatus Tick( Float timeDelta, CMovingAgentComponent& agent ) override;

	// Deactivates the segment
	void Deactivate( CMovingAgentComponent& agent ) override;

	// Checks if the segment can be interrupted
	Bool CanBeCanceled() const override;

	// Serialization support
	void OnSerialize( IFile& file ) override;

	// Debug draw
	void GenerateDebugFragments( CMovingAgentComponent& agent, CRenderFrame* frame ) override;
	void GenerateDebugPage( TDynArray< String >& debugLines ) const override;
};

