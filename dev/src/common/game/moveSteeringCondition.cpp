#include "build.h"

#include "../../common/engine/behaviorGraphStack.h"

#include "moveSteeringCondition.h"
#include "movementCommandBuffer.h"
#include "movementGoal.h"
#include "../core/instanceDataLayoutCompiler.h"

IMPLEMENT_ENGINE_CLASS( IMoveSteeringCondition )
IMPLEMENT_ENGINE_CLASS( CMoveSCAgentSpeed )
IMPLEMENT_ENGINE_CLASS( CMoveSteeringCompositeCondition )
IMPLEMENT_ENGINE_CLASS( CMoveSCHeadingOutputLength )
IMPLEMENT_ENGINE_CLASS( CMoveSCGoalChannel )
IMPLEMENT_ENGINE_CLASS( CMoveSCIsGoalSet )
IMPLEMENT_ENGINE_CLASS( CMoveSCIsGoalHeadingSet )
IMPLEMENT_ENGINE_CLASS( CMoveSCOrientationMatchHeading )
IMPLEMENT_ENGINE_CLASS( CMoveSCFlags )
IMPLEMENT_ENGINE_CLASS( CMoveSCCompareBehaviorVariable )
IMPLEMENT_ENGINE_CLASS( CMoveSCWaitingForNotification )
IMPLEMENT_ENGINE_CLASS( CMoveSCWasNotificationReceived )
IMPLEMENT_ENGINE_CLASS( CMoveSCWasEventTriggered )
IMPLEMENT_ENGINE_CLASS( CMoveSCAnimEvent )
IMPLEMENT_ENGINE_CLASS( CMoveSCHeadingToGoalAngle )
IMPLEMENT_ENGINE_CLASS( CMoveSCArrival )
IMPLEMENT_ENGINE_CLASS( CMoveSCDistanceToGoal )
IMPLEMENT_ENGINE_CLASS( CMoveSCDistanceToDestination )
IMPLEMENT_ENGINE_CLASS( CMoveSCDistanceToTarget )
IMPLEMENT_ENGINE_CLASS( CMoveSCScriptedCondition )
IMPLEMENT_ENGINE_CLASS( CMoveSCHasTargetNodeSetCondition )
IMPLEMENT_RTTI_ENUM( ELogicOperator )

RED_DEFINE_STATIC_NAME( GetConditionName )

 void IMoveSteeringCondition::GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame, InstanceBuffer& data ) const
{

}


CMoveSteeringCompositeCondition::CMoveSteeringCompositeCondition()
	: m_firstCondition( nullptr )
	, m_secondCondition( nullptr )
	, m_notFirstCondition( false )
	, m_notSecondCondition( false )
	, m_operator( ELO_And )
{
}

Bool CMoveSteeringCompositeCondition::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	if ( !m_firstCondition )
	{
		return false;
	}

	Bool result = m_firstCondition->Evaluate( comm, data ) ^ m_notFirstCondition;

	if ( !m_secondCondition )
	{
		return result;
	}

	if ( m_operator == ELO_And )
	{
		return result && ( m_secondCondition->Evaluate( comm, data ) ^ m_notSecondCondition );

	}
	else if ( m_operator == ELO_Or ) 
	{
		return result || ( m_secondCondition->Evaluate( comm, data ) ^ m_notSecondCondition );
	}

	return false;

}

String CMoveSteeringCompositeCondition::GetConditionName() const 
{
	String conditionName;
	conditionName.Reserve( 64 );

	auto funAppendConditionName = [ &conditionName ] ( Bool invert, const IMoveSteeringCondition* condition )
	{
		if ( condition )
		{
			if ( invert )
			{
				conditionName += TXT("!");
			}

			conditionName += condition->GetConditionName();
		}
	};

	funAppendConditionName( m_notFirstCondition, m_firstCondition );
	conditionName += m_operator == ELO_And ? TXT(" && ") : TXT( " || ");
	funAppendConditionName( m_notSecondCondition, m_secondCondition);

	return conditionName; 
}

///////////////////////////////////////////////////////////////////////////////

CMoveSCAgentSpeed::CMoveSCAgentSpeed()
: m_rangeMin( 0.0f )
, m_rangeMax( 0.0f )
{
}

Bool CMoveSCAgentSpeed::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	const CMovingAgentComponent& agent = comm.GetAgent();
	Float agentSpeed = agent.GetAbsoluteMoveSpeed();
	return agentSpeed >= m_rangeMin && agentSpeed <= m_rangeMax;
}

///////////////////////////////////////////////////////////////////////////////

Bool CMoveSCHeadingOutputLength::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	Float inputSq = comm.GetHeading().SquareMag();
	if ( m_considerSpeed )
	{
		Float speed = comm.GetSpeed();
		inputSq *= speed*speed;
	}

	return inputSq >= m_minOutputLength*m_minOutputLength;
}

String CMoveSCHeadingOutputLength::GetConditionName() const
{
	return TXT("OutputIsGreaterThen");
}

///////////////////////////////////////////////////////////////////////////////

CMoveSCGoalChannel::CMoveSCGoalChannel()
	: m_orientationRequired( true )
	, m_headingRequired( true )
{
}

Bool CMoveSCGoalChannel::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	const SMoveLocomotionGoal& goal = comm.GetGoal();
	if ( m_headingRequired && goal.IsHeadingGoalSet() == false )
	{
		return false;
	}

	if ( m_orientationRequired && goal.IsOrientationGoalSet() == false )
	{
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// CMoveSCIsGoalSet
///////////////////////////////////////////////////////////////////////////////
Bool CMoveSCIsGoalSet::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	const SMoveLocomotionGoal& goal = comm.GetGoal();
	return goal.CanBeProcessed();
}

String CMoveSCIsGoalSet::GetConditionName() const
{
	static const String NAME( TXT( "IsGoalSet" ) );
	return NAME;
}

///////////////////////////////////////////////////////////////////////////////
// CMoveSCIsGoalHeadingSet
///////////////////////////////////////////////////////////////////////////////
Bool CMoveSCIsGoalHeadingSet::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	const SMoveLocomotionGoal& goal = comm.GetGoal();
	return goal.IsHeadingGoalSet();
}

String CMoveSCIsGoalHeadingSet::GetConditionName() const
{
	static const String NAME( TXT( "IsGoalHeadingSet" ) );
	return NAME;
}


///////////////////////////////////////////////////////////////////////////////
// CMoveSCOrientationMatchHeading
///////////////////////////////////////////////////////////////////////////////
Bool CMoveSCOrientationMatchHeading::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	const SMoveLocomotionGoal& goal = comm.GetGoal();
	return goal.ShouldMatchOrientationWithHeading();
}
String CMoveSCOrientationMatchHeading::GetConditionName() const
{
	static const String NAME( TXT( "OrientationMatchHeading" ) );
	return NAME;
}

///////////////////////////////////////////////////////////////////////////////

CMoveSCFlags::CMoveSCFlags()
: m_movementFlags( 0 )
{
}

///////////////////////////////////////////////////////////////////////////////

Bool CMoveSCFlags::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	const CMovingAgentComponent& agent = comm.GetAgent();
	return ( agent.GetMovementFlags() & m_movementFlags ) == m_movementFlags;
}

///////////////////////////////////////////////////////////////////////////////

CMoveSCCompareBehaviorVariable::CMoveSCCompareBehaviorVariable()
: m_referenceVal( 0.0f )
, m_comparison( CF_Equal )
{

}

Bool CMoveSCCompareBehaviorVariable::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	const CMovingAgentComponent& agent = comm.GetAgent();
	CBehaviorGraphStack* stack = agent.GetBehaviorStack();
	ASSERT( stack );
	if ( !stack )
	{
		return false;
	}

	Float val = stack->GetBehaviorFloatVariable( m_variableName );

	switch( m_comparison )
	{
	case CF_Equal:					return MAbs( val - m_referenceVal ) < 1e-2;
	case CF_NotEqual:				return MAbs( val - m_referenceVal ) >= 1e-2;
	case CF_Less:					return val < m_referenceVal;
	case CF_LessEqual:				return val <= m_referenceVal;
	case CF_Greater:				return val > m_referenceVal;
	case CF_GreaterEqual:			return val >= m_referenceVal;
	default:						return true;
	}
}

///////////////////////////////////////////////////////////////////////////////

Bool CMoveSCWaitingForNotification::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	const SMoveLocomotionGoal& goal = comm.GetGoal();

	if ( m_notificationName.Empty() )
	{
		return true;
	}

	return goal.m_expectedBehNotification.Exist( m_notificationName );
}

///////////////////////////////////////////////////////////////////////////////

Bool CMoveSCWasNotificationReceived::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	const CMovingAgentComponent& agent = comm.GetAgent();
	if ( m_notificationName.Empty() )
	{
		return true;
	}

	CBehaviorGraphStack* stack = agent.GetBehaviorStack();
	ASSERT( stack );
	if ( stack )
	{
		return stack->DeactivationNotificationReceived( m_notificationName );
	}
	else
	{
		return true;
	}
}

///////////////////////////////////////////////////////////////////////////////

Bool CMoveSCWasEventTriggered::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	const SMoveLocomotionGoal& goal = comm.GetGoal();
	return !goal.m_expectedBehNotification.Empty();
}

///////////////////////////////////////////////////////////////////////////////

void CMoveSCAnimEvent::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_eventHandler;
}

void CMoveSCAnimEvent::OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	Handler* handler = new Handler( *this );
	agent.GetAnimationEventNotifier( m_eventName )->RegisterHandler( handler );
	data[ i_eventHandler ] = reinterpret_cast< TGenericPtr >( handler );
}

void CMoveSCAnimEvent::OnDeinitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	Handler* handler = reinterpret_cast< Handler* >( data[ i_eventHandler ] );
	agent.GetAnimationEventNotifier( m_eventName )->UnregisterHandler( handler );
	delete handler;
	data[ i_eventHandler ] = 0;
}

Bool CMoveSCAnimEvent::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	Handler* handler = reinterpret_cast< Handler* >( data[ i_eventHandler ] );

	Bool wasTriggered = handler->WasTriggered();
	handler->Reset();
	return wasTriggered;
}

CMoveSCAnimEvent::Handler::Handler( const CMoveSCAnimEvent& owner )
	: m_owner( owner )
	, m_triggered( false )
{
}

void CMoveSCAnimEvent::Handler::HandleEvent( const CAnimationEventFired &event )
{
	if ( event.m_type == m_owner.GetEventType() )
	{
		m_triggered = true;
	}
	else
	{
		m_triggered = false;
	}
}

///////////////////////////////////////////////////////////////////////////////

CMoveSCHeadingToGoalAngle::CMoveSCHeadingToGoalAngle()
	: m_acceptableDiff( 1.0f )
{
}

Bool CMoveSCHeadingToGoalAngle::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	const SMoveLocomotionGoal& goal = comm.GetGoal();
	const CMovingAgentComponent& agent = comm.GetAgent();
	if ( !goal.IsHeadingGoalSet() )
	{
		return true;
	}

	Float currHeading = agent.GetMoveDirectionWorldSpace();
	const Vector2& headingToGoal = goal.GetHeadingToGoal();
	Float headingAngleToGoal = ::EulerAngles::YawFromXY( headingToGoal.X, headingToGoal.Y );
	Float angleDiff = ::EulerAngles::AngleDistance( currHeading, headingAngleToGoal );
	return MAbs( angleDiff ) <= m_acceptableDiff;
}


///////////////////////////////////////////////////////////////////////////////
Bool CMoveSCArrival::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	const SMoveLocomotionGoal&		goal	= comm.GetGoal();
	const CMovingAgentComponent&	agent	= comm.GetAgent();
	if ( goal.IsHeadingGoalSet() == false )
	{
		return true;
	}

	if ( goal.IsDistanceToGoalSet() == false )
	{
		return true;
	}

	// return false if we are not withing arrival distance
	if ( goal.GetDistanceToGoal() > m_arrivalDistance )
	{
		return false;
	}
	
	const Vector macHeading			= agent.GetHeading();
	const Vector2& goalHeading		= goal.GetHeadingToGoal();
	const Float goalHeadingAngle	= ::EulerAngles::YawFromXY( goalHeading.X, goalHeading.Y );
	const Float macHeadingAngle		= ::EulerAngles::YawFromXY( macHeading.X, macHeading.Y );
	const Float angleDiff			= ::EulerAngles::AngleDistance( macHeadingAngle, goalHeadingAngle );
	return MAbs( angleDiff ) > m_acceptableAngleToGoal;
}

///////////////////////////////////////////////////////////////////////////////

CMoveSCDistanceToGoal::CMoveSCDistanceToGoal()
	: m_maxDistance( 1.0f )
{

}

Bool CMoveSCDistanceToGoal::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	const SMoveLocomotionGoal& goal = comm.GetGoal();
	if ( goal.IsDistanceToGoalSet() == false )
	{
		return true;
	}

	return goal.GetDistanceToGoal() <= m_maxDistance;
}

///////////////////////////////////////////////////////////////////////////////

CMoveSCDistanceToDestination::CMoveSCDistanceToDestination()
	: m_maxDistance( 1.0f )
	, m_considerGoalTolerance( true )
{

}

Bool CMoveSCDistanceToDestination::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	const SMoveLocomotionGoal& goal = comm.GetGoal();
	if ( goal.IsDistanceToDestinationSet() == false )
	{
		return true;
	}

	Float distance = m_maxDistance;
	if ( m_considerGoalTolerance && goal.IsGoalToleranceSet() )
	{
		distance += goal.GetGoalTolerance();
	}
	return goal.GetDistanceToDestination() <= distance;
}
///////////////////////////////////////////////////////////////////////////////
Bool CMoveSCDistanceToTarget::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	const SMoveLocomotionGoal& goal = comm.GetGoal();
	CNode* targetNode;
	if ( m_namedTarget.Empty() )
	{
		targetNode = goal.GetGoalTargetNode();
	}
	else
	{
		targetNode = nullptr;
		goal.TGetFlag( m_namedTarget, targetNode );
	}
	
	if ( !targetNode )
	{
		return true;
	}

	return ( targetNode->GetWorldPositionRef() - comm.GetAgent().GetWorldPositionRef() ).SquareMag3() <= m_maxDistance;
}

///////////////////////////////////////////////////////////////////////////////

Bool CMoveSCScriptedCondition::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	const CMovingAgentComponent& agent = comm.GetAgent();
	const SMoveLocomotionGoal& goal = comm.GetGoal();
	THandle< CMovingAgentComponent > hAgent( const_cast< CMovingAgentComponent* >( &agent ) );

	Bool result = false;
	CallFunctionRet( const_cast< CMoveSCScriptedCondition* >( this ), CNAME( Evaluate ), hAgent, goal, result );
	return result;
}

String CMoveSCScriptedCondition::GetConditionName() const 
{ 
	String caption;
	CallFunctionRef( const_cast< CMoveSCScriptedCondition* >( this ), CNAME( GetConditionName ), caption );
	return String::Printf( TXT( "Script %s" ), caption.AsChar() ); 
}
///////////////////////////////////////////////////////////////////////////////
// CMoveSCHasTargetNodeSetCondition
///////////////////////////////////////////////////////////////////////////////
Bool CMoveSCHasTargetNodeSetCondition::Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const
{
	const SMoveLocomotionGoal& goal = comm.GetGoal();
	return goal.GetGoalTargetNode() != nullptr;
}

String CMoveSCHasTargetNodeSetCondition::GetConditionName() const 
{ 
	return TXT("HasTargetNodeSet");
}