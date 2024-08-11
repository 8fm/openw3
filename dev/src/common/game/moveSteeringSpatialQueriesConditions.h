/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "moveSteeringCondition.h"


///////////////////////////////////////////////////////////////////////////////
// CMoveSCNavigationClearLine
///////////////////////////////////////////////////////////////////////////////
class CMoveSCNavigationClearLine : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSCNavigationClearLine, IMoveSteeringCondition, 0 );
protected:
	Float						m_destinationForward;
	Float						m_destinationLeft;
	Float						m_testRadius;
	Bool						m_useCharacterOrientation;
	Bool						m_useSteeringOutput;
	Bool						m_useGoalDirection;

public:
	CMoveSCNavigationClearLine()
		: m_destinationForward( 1.f )
		, m_destinationLeft( 0.f )
		, m_testRadius( -1.f )
		, m_useCharacterOrientation( true )
		, m_useSteeringOutput( false )
		, m_useGoalDirection( false )														{}


	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const override;

	virtual String GetConditionName() const override { return TXT( "NavigationClearLine" ); }

	virtual void GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame, InstanceBuffer& data ) const override;
};

BEGIN_CLASS_RTTI( CMoveSCNavigationClearLine )
	PARENT_CLASS( IMoveSteeringCondition )
	PROPERTY_EDIT( m_destinationForward, TXT( "Line destination 'forward' component" ) )
	PROPERTY_EDIT( m_destinationLeft, TXT("Line destination 'left' component" ) )
	PROPERTY_EDIT( m_testRadius, TXT("Width of the test. If testWidth < 0 than test uses agent personal space.") )
	PROPERTY_EDIT( m_useCharacterOrientation, TXT( "If set to true, test will be oriented relatively to character orientation" ) )
	PROPERTY_EDIT( m_useSteeringOutput, TXT( "If set to true, test will be oriented relatively to current steering heading output" ) )
	PROPERTY_EDIT( m_useGoalDirection, TXT( "If set to true, test will be oriented relatively to goal heading input" ) )
END_CLASS_RTTI()