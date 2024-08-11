#include "build.h"
#include "moveSteeringNode.h"

#include "../core/instanceDataLayoutCompiler.h"

#include "movementCommandBuffer.h"
#include "moveSteeringCondition.h"
#include "moveSteeringTask.h"
#include "moveSteeringBehavior.h"


IMPLEMENT_ENGINE_CLASS( IMoveSteeringNode )
IMPLEMENT_ENGINE_CLASS( IMoveSNComposite )
IMPLEMENT_ENGINE_CLASS( CMoveSNComposite )
IMPLEMENT_ENGINE_CLASS( CMoveSNCondition )
IMPLEMENT_ENGINE_CLASS( CMoveSNTask )

///////////////////////////////////////////////////////////////////////////////

IMoveSteeringNode::IMoveSteeringNode()
	: m_enabled( true )
#ifndef NO_EDITOR_STEERING_SUPPORT
	, m_graphPosX( 0 )
	, m_graphPosY( 0 )
#endif

{
}

#ifndef NO_EDITOR_STEERING_SUPPORT
void IMoveSteeringNode::OffsetNodesPosition( Int32 offsetX, Int32 offsetY )
{
	m_graphPosX += offsetX;
	m_graphPosY += offsetY;
}
#endif

void IMoveSteeringNode::CollectNodes( TDynArray< IMoveSteeringNode* >& nodes ) const
{
	nodes.PushBack( const_cast< IMoveSteeringNode* >( this ) );
}

void IMoveSteeringNode::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta, ISteeringBehaviorListener* listener ) const
{
	if ( listener )
	{
		listener->OnNodeActivation( *this );
	}

	if ( !IsEnabled() )
	{
		return;
	}

	OnCalculateSteering( comm, data, timeDelta, listener );
}

void	IMoveSteeringNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )						{}
void	IMoveSteeringNode::OnGraphActivation( CMovingAgentComponent& owner, InstanceBuffer& data )			{}
void	IMoveSteeringNode::OnGraphDeactivation( CMovingAgentComponent& owner, InstanceBuffer& data )		{}
void	IMoveSteeringNode::OnBranchDeactivation( CMovingAgentComponent& owner, InstanceBuffer& data ) const	{}
void	IMoveSteeringNode::OnInitData( CMovingAgentComponent& owner, InstanceBuffer& data )					{}
void	IMoveSteeringNode::OnDeinitData( CMovingAgentComponent& owner, InstanceBuffer& data )				{}
String	IMoveSteeringNode::GetNodeCaption() const															{ return TXT( "SteeringNode" ); }
String	IMoveSteeringNode::GetNodeName() const																{ return TXT( "BaseSteeringNode" ); }
Bool	IMoveSteeringNode::CorrectChildrenOrder()															{ return false; }
void	IMoveSteeringNode::GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) const {}
void	IMoveSteeringNode::OnCalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta, ISteeringBehaviorListener* listener ) const {}

///////////////////////////////////////////////////////////////////////////////

void IMoveSNComposite::OnGraphActivation( CMovingAgentComponent& owner, InstanceBuffer& data )
{
	for ( auto it = m_children.Begin(), end = m_children.End(); it != end; ++it )
	{
		(*it)->OnGraphActivation( owner, data );
	}

	TBaseClass::OnGraphActivation( owner, data );
}

void IMoveSNComposite::OnGraphDeactivation( CMovingAgentComponent& owner, InstanceBuffer& data )
{
	for ( auto it = m_children.Begin(), end = m_children.End(); it != end; ++it )
	{
		(*it)->OnGraphDeactivation( owner, data );
	}

	TBaseClass::OnGraphDeactivation( owner, data );
}

void IMoveSNComposite::OnBranchDeactivation( CMovingAgentComponent& owner, InstanceBuffer& data ) const
{
	for ( auto it = m_children.Begin(), end = m_children.End(); it != end; ++it )
	{
		(*it)->OnBranchDeactivation( owner, data );
	}

	TBaseClass::OnBranchDeactivation( owner, data );
}

void IMoveSNComposite::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	for ( auto it = m_children.Begin(), end = m_children.End(); it != end; ++it )
	{
		(*it)->OnBuildDataLayout( compiler );
	}
}

void IMoveSNComposite::OnInitData( CMovingAgentComponent& owner, InstanceBuffer& data )
{
	for ( auto it = m_children.Begin(), end = m_children.End(); it != end; ++it )
	{
		(*it)->OnInitData( owner, data );
	}
}

void IMoveSNComposite::OnDeinitData( CMovingAgentComponent& owner, InstanceBuffer& data )
{
	for ( auto it = m_children.Begin(), end = m_children.End(); it != end; ++it )
	{
		(*it)->OnDeinitData( owner, data );
	}
}

void IMoveSNComposite::AddChild( IMoveSteeringNode* child )
{
	if ( child )
	{
		m_children.PushBack( child );
		OnChildrenChanged();
	}
}

void IMoveSNComposite::RemoveChild( IMoveSteeringNode* child )
{
	if ( child )
	{
		m_children.Remove( child );
		child->Discard();
		OnChildrenChanged();
	}
}

#ifndef NO_EDITOR_STEERING_SUPPORT
void IMoveSNComposite::OffsetNodesPosition( Int32 offsetX, Int32 offsetY )
{
	for ( auto it = m_children.Begin(), end = m_children.End(); it != end; ++it )
	{
		(*it)->OffsetNodesPosition( offsetX, offsetY );
	}
}
#endif // NO_EDITOR_STEERING_SUPPORT

void IMoveSNComposite::CollectNodes( TDynArray< IMoveSteeringNode* >& nodes ) const
{
	nodes.PushBack( const_cast< IMoveSNComposite* >( this ) );
	for ( auto it = m_children.Begin(), end = m_children.End(); it != end; ++it )
	{
		(*it)->CollectNodes( nodes );
	}
}

Int32 IMoveSNComposite::GetChildIdx( IMoveSteeringNode* node ) const
{
	Int32 count = (Int32)m_children.Size();
	for ( Int32 i = 0; i < count; ++i )
	{
		if ( m_children[i] == node )
		{
			return i;
		}
	}

	return -1;
}


#ifndef NO_EDITOR_STEERING_SUPPORT
Bool IMoveSNComposite::CorrectChildrenOrder()
{
	Bool modified = false;

	// Correct children order
	for( Uint32 i=0; i<m_children.Size(); i++ )
	{		
		Int32 x = m_children[i]->GetGraphPosX();
		for( Uint32 j = i+1; j<m_children.Size(); j++ )
		{
			Int32 x2 = m_children[j]->GetGraphPosX();	

			// If wrong order swap order
			if( x2 < x )
			{
				m_children.Swap( i, j );
				x = x2;
				modified = true;
			}
		}
	}

	// Correct recursively
	for( Uint32 i=0; i<m_children.Size(); i++ )
	{
		modified = m_children[i]->CorrectChildrenOrder() || modified;
	}

	return modified;
}

#endif // NO_EDITOR_STEERING_SUPPORT

void IMoveSNComposite::GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) const 
{
	Uint32 count = m_children.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		m_children[i]->GenerateDebugFragments( agent, frame );
	}
}

///////////////////////////////////////////////////////////////////////////////

CMoveSNComposite::CMoveSNComposite()
{
	m_groupName = TXT( "Group" );
}

String CMoveSNComposite::GetNodeCaption() const
{
	return m_groupName;
}

String CMoveSNComposite::GetNodeName() const
{
	return TXT( "Group" );
}

void CMoveSNComposite::OnCalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta, ISteeringBehaviorListener* listener ) const
{
	for ( TDynArray< IMoveSteeringNode* >::const_iterator it = m_children.Begin(); it != m_children.End(); ++it )
	{
		(*it)->CalculateSteering( comm, data, timeDelta, listener );
	}
}

///////////////////////////////////////////////////////////////////////////////

void CMoveSNCondition::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_childActive;

	if ( m_condition )
	{
		m_condition->OnBuildDataLayout( compiler );
	}

	TBaseClass::OnBuildDataLayout( compiler );
}

void CMoveSNCondition::OnGraphActivation( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	data[ i_childActive ] = CHILD_UNACTIVE;

	TBaseClass::OnGraphActivation( agent, data );
}

void CMoveSNCondition::OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	data[ i_childActive ] = CHILD_UNACTIVE;

	if ( m_condition )
	{
		m_condition->OnInitData( agent, data );
	}

	TBaseClass::OnInitData( agent, data );
}

void CMoveSNCondition::OnDeinitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	if ( m_condition )
	{
		m_condition->OnDeinitData( agent, data );
	}

	TBaseClass::OnDeinitData( agent, data );
}

void CMoveSNCondition::GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) const
{
#ifndef NO_EDITOR_FRAGMENTS
	InstanceBuffer* dataPtr = agent.GetCurrentSteeringRuntimeData();
	if ( !dataPtr )
	{
		return;
	}

	if ( m_condition )
	{
		m_condition->GenerateDebugFragments( agent, frame, *dataPtr );
	}

	InstanceBuffer& data = *dataPtr;

	Uint8 activeChild = data[ i_childActive ];
	if ( activeChild != CHILD_UNACTIVE )
	{
		m_children[ activeChild ]->GenerateDebugFragments( agent, frame );
	}
#endif
}

void CMoveSNCondition::OnBranchDeactivation( CMovingAgentComponent& owner, InstanceBuffer& data ) const 
{
	Uint8 activeChild = data[ i_childActive ];
	if ( activeChild != CHILD_UNACTIVE )
	{
		m_children[ activeChild ]->OnBranchDeactivation( owner, data );
	}
}

String CMoveSNCondition::GetNodeCaption() const
{
	if ( m_condition )
	{
		return String::Printf( TXT( "Cond [%s%s]"), m_invertCondition ? TXT("! ") : TXT(""), m_condition->GetConditionName().AsChar() );
	}
	else
	{
		return TXT( "Cond EMPTY" );
	}
}
String CMoveSNCondition::GetNodeName() const
{
	return TXT( "Condition" );
}
Bool CMoveSNCondition::CanAddChildren() const
{
	return m_children.Size() < 2;
}
void CMoveSNCondition::SetCondition( IMoveSteeringCondition* condition )
{
	if ( m_condition )
	{
		m_condition->Discard();
	}
	m_condition = condition;
}

void CMoveSNCondition::OnCalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta, ISteeringBehaviorListener* listener ) const
{
	CMovingAgentComponent& agent = comm.GetAgent();
	const SMoveLocomotionGoal& goal = comm.GetGoal();

	// if one exists, the first child is always the 'true' branch, and the second one is the 'false branch'
	Uint8 newActiveChild = ( m_condition && !(m_condition->Evaluate( comm, data ) ^ m_invertCondition) );
	// we need to keep i_childActive flag updated
	Uint8& currActiveChild = data[ i_childActive ];

	if ( newActiveChild >= m_children.Size() )
	{
		if ( currActiveChild != CHILD_UNACTIVE )
		{
			m_children[ currActiveChild ]->OnBranchDeactivation( comm.GetAgent(), data );
			currActiveChild = CHILD_UNACTIVE;
		}
		return;
	}
	if ( currActiveChild != newActiveChild )
	{
		if ( currActiveChild != CHILD_UNACTIVE )
		{
			m_children[ currActiveChild ]->OnBranchDeactivation( comm.GetAgent(), data );
		}
		currActiveChild = newActiveChild;
	}

	m_children[ newActiveChild ]->CalculateSteering( comm, data, timeDelta, listener );
}

///////////////////////////////////////////////////////////////////////////////

CMoveSNTask::CMoveSNTask()
: m_task( NULL )
{
}

void CMoveSNTask::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	if ( m_task )
	{
		m_task->OnBuildDataLayout( compiler );
	}

	TBaseClass::OnBuildDataLayout( compiler );
}

void CMoveSNTask::OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	if ( m_task )
	{
		m_task->OnInitData( agent, data );
	}

	TBaseClass::OnInitData( agent, data );
}

void CMoveSNTask::OnDeinitData( CMovingAgentComponent& agent, InstanceBuffer& data )
{
	if ( m_task )
	{
		m_task->OnDeinitData( agent, data );
	}

	TBaseClass::OnDeinitData( agent, data );
}

void CMoveSNTask::OnGraphActivation( CMovingAgentComponent& owner, InstanceBuffer& data )
{
	if ( m_task )
	{
		m_task->OnGraphActivation( owner, data );
	}

	TBaseClass::OnGraphActivation( owner, data );
}

void CMoveSNTask::OnGraphDeactivation( CMovingAgentComponent& owner, InstanceBuffer& data )
{
	if ( m_task )
	{
		m_task->OnGraphDeactivation( owner, data );
	}

	TBaseClass::OnGraphDeactivation( owner, data );
}

void CMoveSNTask::OnBranchDeactivation( CMovingAgentComponent& owner, InstanceBuffer& data ) const
{
	if ( m_task )
	{
		m_task->OnBranchDeactivation( owner, data );
	}

	TBaseClass::OnBranchDeactivation( owner, data );
}

String CMoveSNTask::GetNodeCaption() const
{
	if ( m_task )
	{
		return String::Printf( TXT("Task [%s]"), m_task->GetTaskName().AsChar() );
	}
	else
	{
		return TXT( "Task EMPTY" );
	}
}

String CMoveSNTask::GetNodeName() const
{
	return TXT( "Task" );
}

void CMoveSNTask::SetTask( IMoveSteeringTask* task )
{
	if ( m_task )
	{
		m_task->Discard();
	}
	m_task = task;
}

void CMoveSNTask::OnCalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta, ISteeringBehaviorListener* listener ) const
{
	if ( !m_task )
	{
		return;
	}
	m_task->CalculateSteering( comm, data, timeDelta );
}

void CMoveSNTask::GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) const 
{
	if ( m_task )
	{
		m_task->GenerateDebugFragments( agent, frame );
	}
}

///////////////////////////////////////////////////////////////////////////////
