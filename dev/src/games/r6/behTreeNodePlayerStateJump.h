/*
 * Copyright � 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "behTreeNodePlayerState.h"

//////////////////////////////////////////////////////////////////////////
class CBehTreeNodePlayerStateJumpInstance;

class CBehTreeNodePlayerStateJumpDefinition : public IBehTreeNodePlayerStateDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodePlayerStateJumpDefinition, IBehTreeNodePlayerStateDefinition, CBehTreeNodePlayerStateJumpInstance, Jump )
	DECLARE_AS_R6_ONLY

protected:
	CBehTreeValFloat m_maxSpeed;
	CBehTreeValFloat m_accel;
	CBehTreeValFloat m_decel;
	CBehTreeValFloat m_brake;
	CBehTreeValFloat m_dotToBrake;

	CBehTreeValFloat m_gravityUp;
	CBehTreeValFloat m_gravityDown;
	CBehTreeValFloat m_jumpSpeedImpulse;
	CBehTreeValFloat m_maxVerticalSpeedAbs;


public:
	CBehTreeNodePlayerStateJumpDefinition();
	virtual IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = nullptr ) const;
};




BEGIN_CLASS_RTTI( CBehTreeNodePlayerStateJumpDefinition )

	PARENT_CLASS( IBehTreeNodePlayerStateDefinition )

	PROPERTY_INLINED( m_maxSpeed				, TXT( "Maximum speed in meters per second." ) )
	PROPERTY_INLINED( m_accel					, TXT( "Acceleration" ) )
	PROPERTY_INLINED( m_decel					, TXT( "Deceleration" ) )
	PROPERTY_INLINED( m_brake					, TXT( "" ) )
	PROPERTY_INLINED( m_dotToBrake				, TXT( "" ) )

	PROPERTY_INLINED( m_gravityUp				, TXT( "" ) )
	PROPERTY_INLINED( m_gravityDown				, TXT( "" ) )
	PROPERTY_INLINED( m_jumpSpeedImpulse		, TXT( "" ) )
	PROPERTY_INLINED( m_maxVerticalSpeedAbs		, TXT( "" ) )

END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CBehTreeNodePlayerStateJumpInstance : public IBehTreeNodePlayerStateInstance
{
	typedef IBehTreeNodePlayerStateInstance Super;

protected:
	Float	m_maxSpeed;
	Float	m_accel;
	Float	m_decel;
	Float	m_brake;
	Float	m_dotToBrake;


	Float	m_gravityUp;
	Float	m_gravityDown;
	Float	m_jumpSpeedImpulse;
	Float	m_maxVerticalSpeedAbs;
	Bool	m_stillJumping;

public:
	typedef CBehTreeNodePlayerStateJumpDefinition Definition;

	CBehTreeNodePlayerStateJumpInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
	virtual ~CBehTreeNodePlayerStateJumpInstance();

	////////////////////////////////////////////////////////////////////
	//! Behavior execution cycle
	void Update() override;
	Bool Activate() override;
	void Deactivate() override;

	////////////////////////////////////////////////////////////////////
	//! Custom interface
	Bool Interrupt() override;

	////////////////////////////////////////////////////////////////////
	//! Evaluation
	virtual Bool IsAvailable();
};