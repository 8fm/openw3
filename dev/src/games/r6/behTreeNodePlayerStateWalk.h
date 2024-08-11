/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "behTreeNodePlayerState.h"

//////////////////////////////////////////////////////////////////////////
class CBehTreeNodePlayerStateWalkInstance;

class CBehTreeNodePlayerStateWalkDefinition : public IBehTreeNodePlayerStateDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodePlayerStateWalkDefinition, IBehTreeNodePlayerStateDefinition, CBehTreeNodePlayerStateWalkInstance, Walk )
	DECLARE_AS_R6_ONLY

protected:
	CBehTreeValFloat m_maxSpeed;
	CBehTreeValFloat m_accel;
	CBehTreeValFloat m_decel;
	CBehTreeValFloat m_brake;
	CBehTreeValFloat m_dotToBrake;

	CBehTreeValCName m_uniqueAnimation;
public:
	CBehTreeNodePlayerStateWalkDefinition();
	virtual IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = nullptr ) const;
};

BEGIN_CLASS_RTTI( CBehTreeNodePlayerStateWalkDefinition )

	PARENT_CLASS( IBehTreeNodePlayerStateDefinition )

	PROPERTY_INLINED( m_maxSpeed			, TXT("Maximum speed in meters per second.") )
	PROPERTY_INLINED( m_accel				, TXT("Acceleration") )
	PROPERTY_INLINED( m_decel				, TXT("Deceleration") )
	PROPERTY_INLINED( m_brake				, TXT("") )
	PROPERTY_INLINED( m_dotToBrake			, TXT("") )

	PROPERTY_INLINED( m_uniqueAnimation		, TXT( "If this name is set, it will be sent to the animation as a trigger instead of default 'Walk'." ) )

END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CBehTreeNodePlayerStateWalkInstance : public IBehTreeNodePlayerStateInstance
{
	typedef IBehTreeNodePlayerStateInstance Super;

protected:
	Float m_maxSpeed;
	Float m_accel;
	Float m_decel;
	Float m_brake;
	Float m_dotToBrake;
public:
	typedef CBehTreeNodePlayerStateWalkDefinition Definition;

	CBehTreeNodePlayerStateWalkInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
	virtual ~CBehTreeNodePlayerStateWalkInstance();

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