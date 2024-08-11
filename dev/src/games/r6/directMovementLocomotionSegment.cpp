#include "build.h"

#include "directMovementLocomotionSegment.h"



CMoveLSDirect::CMoveLSDirect( )
	: m_DeltaDisplacement( Vector::ZEROS )
	, m_DeltaRotation(EulerAngles::ZEROS)
{
}

Bool CMoveLSDirect::Activate( CMovingAgentComponent& agent )
{

	return true;
}

void CMoveLSDirect::Deactivate( CMovingAgentComponent& agent )
{
}

ELSStatus CMoveLSDirect::Tick( Float timeDelta, CMovingAgentComponent& agent )
{
	agent.AddDeltaMovement( m_PlayerMovement * timeDelta, EulerAngles::ZEROS );
	agent.AddDeltaMovement( m_DeltaDisplacement, m_DeltaRotation );

	m_PlayerMovement	= Vector::ZEROS;
	m_DeltaDisplacement	= Vector::ZEROS;
	m_DeltaRotation		= EulerAngles::ZEROS;

	return LS_InProgress;
}

void CMoveLSDirect::GenerateDebugFragments( CMovingAgentComponent& agent, CRenderFrame* frame )
{
}

void CMoveLSDirect::GenerateDebugPage( TDynArray< String >& debugLines ) const
{
	debugLines.PushBack( TXT( "CMoveLSDirect" ) );
}

void CMoveLSDirect::AddPlayerMovement( const Vector& translation )
{
	m_PlayerMovement = translation;
}

void CMoveLSDirect::AddTranslation( const Vector& translation )
{
	m_DeltaDisplacement	+= translation;
}

void CMoveLSDirect::AddRotation( const EulerAngles& rotation )
{
	m_DeltaRotation += rotation;
}

void CMoveLSDirect::ResetTranslationAndRotation()
{
	m_DeltaDisplacement	= Vector::ZEROS;
	m_DeltaRotation		= EulerAngles::ZEROS;
}