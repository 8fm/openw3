/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "movingAgentComponent.h"


class CMovingAgentComponent;
class IMovementCommandBuffer;
struct SMoveLocomotionGoal;

///////////////////////////////////////////////////////////////////////////////

class IMoveSteeringCondition : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IMoveSteeringCondition, CObject );

public:
	virtual ~IMoveSteeringCondition() {}

	virtual Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const = 0;

	// Returns the name of the condition
	virtual String GetConditionName() const { return TXT("Cond[]"); }

	// Builds the runtime data layout for this condition
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) {}

	// Initializes the runtime data of this condition
	virtual void OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data ) {}

	// Deinitializes the runtime data of this condition
	virtual void OnDeinitData( CMovingAgentComponent& agent, InstanceBuffer& data ) {}

	virtual void GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame, InstanceBuffer& data ) const;
};
BEGIN_ABSTRACT_CLASS_RTTI( IMoveSteeringCondition )
	PARENT_CLASS( CObject )
END_CLASS_RTTI()

enum ELogicOperator
{
	ELO_And,
	ELO_Or
};

BEGIN_ENUM_RTTI( ELogicOperator );
	ENUM_OPTION( ELO_And );
	ENUM_OPTION( ELO_Or );
END_ENUM_RTTI();

class CMoveSteeringCompositeCondition : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSteeringCompositeCondition, IMoveSteeringCondition, 0 );

protected:
	IMoveSteeringCondition* m_firstCondition;
	IMoveSteeringCondition* m_secondCondition;
	Bool					m_notFirstCondition;
	Bool					m_notSecondCondition;
	ELogicOperator			m_operator;

public:
	CMoveSteeringCompositeCondition();

	virtual Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const override;
	virtual String GetConditionName() const override;
};

BEGIN_CLASS_RTTI( CMoveSteeringCompositeCondition )
	PARENT_CLASS( IMoveSteeringCondition )
	PROPERTY_INLINED( m_firstCondition, TXT( "First condition" ) )
	PROPERTY_EDIT	( m_notFirstCondition, TXT( "Invert first condition" ) )
	PROPERTY_EDIT	( m_operator, TXT( "Logic operator" ) )
	PROPERTY_INLINED( m_secondCondition, TXT( "Second condition" ) )
	PROPERTY_EDIT	( m_notSecondCondition, TXT( "Invert second condition" ) )
END_CLASS_RTTI()


///////////////////////////////////////////////////////////////////////////////

class CMoveSCAgentSpeed : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSCAgentSpeed, IMoveSteeringCondition, 0 );

private:
	Float m_rangeMin;
	Float m_rangeMax;
	
public:
	CMoveSCAgentSpeed();

	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const;

	virtual String GetConditionName() const { return TXT( "AgentSpeed" ); }
};
BEGIN_CLASS_RTTI( CMoveSCAgentSpeed )
	PARENT_CLASS( IMoveSteeringCondition )
	PROPERTY_EDIT( m_rangeMin, TXT( "Minimum speed value for the range" ) )
	PROPERTY_EDIT( m_rangeMax, TXT( "Maximum speed value for the range" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CMoveSCHeadingOutputLength : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSCHeadingOutputLength, IMoveSteeringCondition, 0 );

private:
	Float m_minOutputLength;
	Bool m_considerSpeed;

public:
	CMoveSCHeadingOutputLength()
		: m_minOutputLength( 0.3f )
		, m_considerSpeed( false )												{}

	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const;

	String GetConditionName() const override;
};
BEGIN_CLASS_RTTI( CMoveSCHeadingOutputLength )
	PARENT_CLASS( IMoveSteeringCondition )
	PROPERTY_EDIT( m_minOutputLength, TXT( "Minimum heading output" ) )
	PROPERTY_EDIT( m_considerSpeed, TXT( "Multiply algorithm input with current speed output" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CMoveSCGoalChannel : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSCGoalChannel, IMoveSteeringCondition, 0 );

private:
	Bool						m_orientationRequired;
	Bool						m_headingRequired;

public:
	CMoveSCGoalChannel();

	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const;

	virtual String GetConditionName() const { return TXT( "GoalChannel" ); }
};
BEGIN_CLASS_RTTI( CMoveSCGoalChannel )
	PARENT_CLASS( IMoveSteeringCondition )
	PROPERTY_EDIT( m_orientationRequired, TXT( "Does goal have an orientation specified" ) )
	PROPERTY_EDIT( m_headingRequired, TXT( "Does goal have a heading specified" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CMoveSCIsGoalSet : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSCIsGoalSet, IMoveSteeringCondition, 0 );

public:
	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const override;

	String GetConditionName() const override;
};
BEGIN_CLASS_RTTI( CMoveSCIsGoalSet )
	PARENT_CLASS( IMoveSteeringCondition )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CMoveSCIsGoalHeadingSet : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSCIsGoalHeadingSet, IMoveSteeringCondition, 0 );

public:
	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const override;

	String GetConditionName() const override;
};
BEGIN_CLASS_RTTI( CMoveSCIsGoalHeadingSet )
	PARENT_CLASS( IMoveSteeringCondition )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CMoveSCOrientationMatchHeading : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSCOrientationMatchHeading, IMoveSteeringCondition, 0 );
public:
	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const override;

	String GetConditionName() const override;
};
BEGIN_CLASS_RTTI( CMoveSCOrientationMatchHeading )
	PARENT_CLASS( IMoveSteeringCondition )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CMoveSCFlags : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSCFlags, IMoveSteeringCondition, 0 );

private:
	Uint8		m_movementFlags;

public:
	CMoveSCFlags();

	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const;

	virtual String GetConditionName() const { return TXT( "Flags" ); }
};
BEGIN_CLASS_RTTI( CMoveSCFlags )
	PARENT_CLASS( IMoveSteeringCondition )
	PROPERTY_BITFIELD_EDIT( m_movementFlags, EMovementFlags, TXT("Movement flags that need to be set") );
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CMoveSCCompareBehaviorVariable : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSCCompareBehaviorVariable, IMoveSteeringCondition, 0 );

private:
	CName				m_variableName;
	Float				m_referenceVal;
	ECompareFunc		m_comparison;

public:
	CMoveSCCompareBehaviorVariable();

	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const;

	virtual String GetConditionName() const { return TXT( "CompareBehaviorVariable" ); }
};
BEGIN_CLASS_RTTI( CMoveSCCompareBehaviorVariable )
	PARENT_CLASS( IMoveSteeringCondition )
	PROPERTY_EDIT( m_variableName, TXT( "Variable name" ) );
	PROPERTY_EDIT( m_referenceVal, TXT( "Reference value" ) );
	PROPERTY_EDIT( m_comparison, TXT( "Comparison function" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CMoveSCWaitingForNotification : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSCWaitingForNotification, IMoveSteeringCondition, 0 );

private:
	CName				m_notificationName;

public:
	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const;

	virtual String GetConditionName() const { return TXT( "IsWaitingForNotification" ); }
};
BEGIN_CLASS_RTTI( CMoveSCWaitingForNotification )
	PARENT_CLASS( IMoveSteeringCondition )
	PROPERTY_EDIT( m_notificationName, TXT( "Notification we're waiting for" ) );
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CMoveSCWasNotificationReceived : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSCWasNotificationReceived, IMoveSteeringCondition, 0 );

private:
	CName				m_notificationName;

public:
	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const;

	virtual String GetConditionName() const { return TXT( "NotificationReceived" ); }
};
BEGIN_CLASS_RTTI( CMoveSCWasNotificationReceived )
	PARENT_CLASS( IMoveSteeringCondition )
	PROPERTY_EDIT( m_notificationName, TXT( "Notification we're waiting for" ) );
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CMoveSCWasEventTriggered : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSCWasEventTriggered, IMoveSteeringCondition, 0 );

public:
	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const;

	virtual String GetConditionName() const { return TXT( "WasEventTriggered" ); }
};
BEGIN_CLASS_RTTI( CMoveSCWasEventTriggered )
	PARENT_CLASS( IMoveSteeringCondition )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CMoveSCAnimEvent : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSCAnimEvent, IMoveSteeringCondition, 0 );

private:
	class Handler : public IEventHandler< CAnimationEventFired >, public Red::System::NonCopyable
	{
	private:
		const CMoveSCAnimEvent&			m_owner;
		Bool							m_triggered;

	public:
		Handler( const CMoveSCAnimEvent& owner );

		virtual	void HandleEvent( const CAnimationEventFired &event );

		RED_INLINE Bool WasTriggered() const { return m_triggered; }

		void Reset() { m_triggered = false; }
	};
private:
	CName						m_eventName;
	EAnimationEventType			m_eventType;

	// runtime data
	TInstanceVar< TGenericPtr >		i_eventHandler;

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	virtual void OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data );

	virtual void OnDeinitData( CMovingAgentComponent& agent, InstanceBuffer& data );

	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const;

	virtual String GetConditionName() const { return TXT( "AnimEvent" ); }

	RED_INLINE EAnimationEventType GetEventType() const { return m_eventType; }
};
BEGIN_CLASS_RTTI( CMoveSCAnimEvent )
	PARENT_CLASS( IMoveSteeringCondition )
	PROPERTY_EDIT( m_eventName, TXT( "Animation event name" ) )
	PROPERTY_EDIT( m_eventType, TXT( "Animation event type" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CMoveSCHeadingToGoalAngle : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSCHeadingToGoalAngle, IMoveSteeringCondition, 0 );

private:
	Float		m_acceptableDiff;

public:
	CMoveSCHeadingToGoalAngle();

	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const;

	virtual String GetConditionName() const { return TXT( "HeadingToGoalAngle" ); }
};
BEGIN_CLASS_RTTI( CMoveSCHeadingToGoalAngle )
	PARENT_CLASS( IMoveSteeringCondition )
	PROPERTY_EDIT( m_acceptableDiff, TXT( "Acceptable angular difference between heading and direction to goal" ) )
END_CLASS_RTTI()


///////////////////////////////////////////////////////////////////////////////

class CMoveSCArrival : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSCArrival, IMoveSteeringCondition, 0 );

private:
	Float		m_arrivalDistance;
	Float		m_acceptableAngleToGoal;

public:
	CMoveSCArrival()
		: m_arrivalDistance( 1.5f )
		, m_acceptableAngleToGoal( 70.0f )	{}

	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const;

	virtual String GetConditionName() const { return TXT( "Arrival" ); }
};
BEGIN_CLASS_RTTI( CMoveSCArrival )
	PARENT_CLASS( IMoveSteeringCondition )
	PROPERTY_EDIT( m_arrivalDistance, TXT( "Below that distance the condition will consider being at arrival distance" ) )
	PROPERTY_EDIT( m_acceptableAngleToGoal, TXT( "Acceptable angular difference between actor rotation and direction to goal, over that angle the condition will will true" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CMoveSCDistanceToGoal : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSCDistanceToGoal, IMoveSteeringCondition, 0 );

private:
	Float		m_maxDistance;

public:
	CMoveSCDistanceToGoal();

	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const;

	virtual String GetConditionName() const { return TXT( "DistanceToGoal" ); }
};
BEGIN_CLASS_RTTI( CMoveSCDistanceToGoal )
	PARENT_CLASS( IMoveSteeringCondition )
	PROPERTY_EDIT( m_maxDistance, TXT( "Max distance to goal" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CMoveSCDistanceToDestination : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSCDistanceToDestination, IMoveSteeringCondition, 0 );

private:
	Float		m_maxDistance;
	Bool		m_considerGoalTolerance;

public:
	CMoveSCDistanceToDestination();

	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const;

	virtual String GetConditionName() const { return TXT( "DistanceToDestination" ); }
};
BEGIN_CLASS_RTTI( CMoveSCDistanceToDestination )
	PARENT_CLASS( IMoveSteeringCondition )
	PROPERTY_EDIT( m_maxDistance, TXT( "Max distance to destination" ) )
	PROPERTY_EDIT( m_considerGoalTolerance, TXT( "Consider goal tolerance" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CMoveSCDistanceToTarget : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSCDistanceToTarget, IMoveSteeringCondition, 0 );

private:
	Float		m_maxDistance;
	CName		m_namedTarget;

public:
	CMoveSCDistanceToTarget()
		: m_maxDistance( 5.f )													{}

	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const;

	virtual String GetConditionName() const { return TXT( "DistanceToTarget" ); }
};
BEGIN_CLASS_RTTI( CMoveSCDistanceToTarget )
	PARENT_CLASS( IMoveSteeringCondition )
	PROPERTY_EDIT( m_maxDistance, TXT( "Max distance to destination" ) )
	PROPERTY_EDIT( m_namedTarget, TXT( "Fill up to use named target" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CMoveSCScriptedCondition : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSCScriptedCondition, IMoveSteeringCondition, 0 );

public:
	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const;

	String GetConditionName() const;
};
BEGIN_CLASS_RTTI( CMoveSCScriptedCondition )
	PARENT_CLASS( IMoveSteeringCondition )
END_CLASS_RTTI()


///////////////////////////////////////////////////////////////////////////////

class CMoveSCHasTargetNodeSetCondition : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSCHasTargetNodeSetCondition, IMoveSteeringCondition, 0 );

public:
	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const;

	String GetConditionName() const;
};
BEGIN_CLASS_RTTI( CMoveSCHasTargetNodeSetCondition )
	PARENT_CLASS( IMoveSteeringCondition )
END_CLASS_RTTI()

