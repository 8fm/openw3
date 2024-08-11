#pragma once

#include "../../common/game/moveLocomotionSegment.h"


///////////////////////////////////////////////////////////////////////////////

class CMoveLSDirect : public IMoveLocomotionSegment
{

private:
	Vector			m_DeltaDisplacement;
	Vector			m_PlayerMovement;
	EulerAngles		m_DeltaRotation;

public:
	CMoveLSDirect();

	// -------------------------------------------------------------------------
	// IMoveLocomotionSegment implementation
	// -------------------------------------------------------------------------
	virtual Bool Activate( CMovingAgentComponent& agent );
	virtual ELSStatus Tick( Float timeDelta, CMovingAgentComponent& agent );
	virtual void Deactivate( CMovingAgentComponent& agent );
	virtual Bool CanBeCanceled() const { return false; }
	virtual void GenerateDebugFragments( CMovingAgentComponent& agent, CRenderFrame* frame );
	virtual void GenerateDebugPage( TDynArray< String >& debugLines ) const;

	// -------------------------------------------------------------------------
	// Direct movement specific
	// -------------------------------------------------------------------------
	void AddPlayerMovement( const Vector& translation );
	void AddTranslation( const Vector& translation );
	void AddRotation( const EulerAngles& rotation );
	void ResetTranslationAndRotation();

	// -------------------------------------------------------------------------
	// IMoveLocomotionSegment interface
	// -------------------------------------------------------------------------
	virtual CMoveLSDirect* AsCMoveLSDirect() override { return this; }
};
