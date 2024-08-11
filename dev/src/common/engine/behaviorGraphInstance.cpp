/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphAnimationManualSlot.h"
#include "behaviorGraphAnimationMixerSlot.h"
#include "behaviorGraphAnimationSlotNode.h"
#include "behaviorGraphPointCloudLookAtNode.h"
#include "behaviorGraphContainerNode.h"
#include "behaviorGraphEngineValueNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphPoseSlotNode.h"
#include "behaviorGraphStateMachine.h"
#include "behaviorGraphStateNode.h"
#include "behaviorGraphTopLevelNode.h"
#include "behaviorGraph.inl"
#include "behaviorGraphInstance.inl"
#include "../engine/animationManager.h"
#include "../engine/characterConstraint.h"
#include "../engine/animationController.h"
#include "../engine/animatedComponent.h"
#include "../engine/skeletalAnimationEntry.h"
#include "../engine/extAnimEvent.h"
#include "../core/queue.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphInstance );

//////////////////////////////////////////////////////////////////////////

SBehaviorGraphInstanceStoredVariables::SBehaviorGraphInstanceStoredVariables()
	:	m_floatVariablesNum( 0 )
	,	m_vectorVariablesNum( 0 )
	,	m_internalFloatVariablesNum( 0 )
	,	m_internalVectorVariablesNum( 0 )
{
}

void SBehaviorGraphInstanceStoredVariables::AddFloatVariable( const CBehaviorGraph* _graph, CName _name, Float _value )
{
	ASSERT( m_floatVariablesNum < NUM_VARIABLES, TXT("Make array to store float variables bigger") );
	m_floatVariables[ m_floatVariablesNum ] = SBehaviorGraphInstanceStoredVariable( _graph, _name, _value );
	m_floatVariablesNum = Min( NUM_VARIABLES, m_floatVariablesNum + 1 );
}

void SBehaviorGraphInstanceStoredVariables::AddVectorVariable( const CBehaviorGraph* _graph, CName _name, const Vector& _value )
{
	ASSERT( m_vectorVariablesNum < NUM_VARIABLES, TXT("Make array to store vector variables bigger") );
	m_vectorVariables[ m_vectorVariablesNum ] = SBehaviorGraphInstanceStoredVariable( _graph, _name, _value );
	m_vectorVariablesNum = Min( NUM_VARIABLES, m_vectorVariablesNum + 1 );
}

void SBehaviorGraphInstanceStoredVariables::AddInternalFloatVariable( const CBehaviorGraph* _graph, CName _name, Float _value )
{
	ASSERT( m_internalFloatVariablesNum < NUM_VARIABLES, TXT("Make array to store internal float variables bigger") );
	m_internalFloatVariables[ m_internalFloatVariablesNum ] = SBehaviorGraphInstanceStoredVariable( _graph, _name, _value );
	m_internalFloatVariablesNum = Min( NUM_VARIABLES, m_internalFloatVariablesNum + 1 );
}

void SBehaviorGraphInstanceStoredVariables::AddInternalVectorVariable( const CBehaviorGraph* _graph, CName _name, const Vector& _value )
{
	ASSERT( m_internalVectorVariablesNum < NUM_VARIABLES, TXT("Make array to store internal vector variables bigger") );
	m_internalVectorVariables[ m_internalVectorVariablesNum ] = SBehaviorGraphInstanceStoredVariable( _graph, _name, _value );
	m_internalVectorVariablesNum = Min( NUM_VARIABLES, m_internalVectorVariablesNum + 1 );
}

//////////////////////////////////////////////////////////////////////////

CBehaviorGraphInstance::CBehaviorGraphInstance()
	: m_data( NULL )
	, m_root( NULL )
	, m_internalState( IIS_Waiting )
	, m_eventHandler( NULL )
	, m_editorListener( nullptr )
#ifndef NO_EDITOR_GRAPH_SUPPORT
	, m_isOpenInEditor( false )
#endif
{

}

CBehaviorGraphInstance::~CBehaviorGraphInstance()
{
	ASSERT( !IsActive() );
	ASSERT( !IsBinded() );
	m_constraints.ClearPtr();
	m_listeners.Clear();
	m_editorListener = NULL;
	if ( m_eventHandler )
	{
		delete m_eventHandler;
	}
}

void CBehaviorGraphInstance::Bind( const CName& name, const THandle< CBehaviorGraph >& graph, const InstanceDataLayout& dataLayout, CAnimatedComponent* component )
{
	PC_SCOPE_PIX( CBehaviorGraphInstance_Bind );
	ASSERT( !IsBinded() );

	// Set name
	m_graph = graph;
	m_instanceName = name;

	// Instance data
	m_data = dataLayout.CreateBuffer( this, TXT( "CBehaviorGraphInstance" ) );
	ASSERT( m_data );

	// Attached to animated component
	m_animatedComponent = component;

	// Variables
	FillVariables( graph );

	// Root node
	m_root = SafeCast< CBehaviorGraphTopLevelNode >( graph->m_rootNode );
	m_root->OnInitInstance( *this );

	// Custom data
	m_defaultStateMachine = graph->m_defaultStateMachine;

	if ( GAnimationManager )
	{
		GAnimationManager->OnGraphInstanceBinded( this );
	}
}

void CBehaviorGraphInstance::Unbind()
{
	ASSERT( IsBinded() );

	if ( GAnimationManager )
	{
		GAnimationManager->OnGraphInstanceUnbinded( this );
	}

	if ( m_root && m_data )
	{
#ifndef NO_EDITOR
		m_root->OnReleaseInstance( *this );
#else
		const TDynArray< CBehaviorGraphNode* >& nodesToRelease = GetGraph()->GetNodesToRelease();
		if ( nodesToRelease.Size() > 0 )
		{
			for ( CBehaviorGraphNode* node : nodesToRelease )
			{
				node->OnReleaseInstance( *this );
			}
		}
		else
		{
			m_root->OnReleaseInstance( *this );
		}
#endif
	}

	if ( m_data )
	{
		m_data->Release();
		m_data = NULL;
	}

	m_processedEvents.Clear();
	m_activationNotifications.Clear();
	m_deactivationNotifications.Clear();
	m_scriptEventNotifications.Clear();
	
	m_listeners.Clear();
	
	m_root = NULL;
	m_graph = NULL;
	m_animatedComponent = NULL;

	if ( m_editorListener )
	{
		m_editorListener->OnUnbind();
	}
}

Bool CBehaviorGraphInstance::IsBinded() const
{
	return m_root || m_data ? true : false;
}

void CBehaviorGraphInstance::Activate()
{
	PC_SCOPE_PIX( CBehaviorGraphInstance_Activate );
	ASSERT( IsBinded() );
	ASSERT( m_internalState == IIS_Waiting );

	if ( m_editorListener )
	{
		m_editorListener->OnActivated();
	}

	m_active = true;
	m_timeActive = 0.0f;
	m_root->Activate( *this );
}

void CBehaviorGraphInstance::Deactivate()
{
	ASSERT( IsBinded() );
	ASSERT( m_internalState == IIS_Waiting );

	if ( m_editorListener )
	{
		m_editorListener->OnDeactivated();
	}

	while( m_root->IsActive( *this ) )
	{
		m_root->Deactivate( *this );
	}

	m_active = false;
	m_timeActive = 0.0f;
}

void CBehaviorGraphInstance::DeactivateAndReset()
{
	ASSERT( IsBinded() );
	ASSERT( m_internalState == IIS_Waiting );

	if ( m_editorListener )
	{
		m_editorListener->OnDeactivated();
	}

	while( m_root->IsActive( *this ) )
	{
		m_root->Deactivate( *this );
	}

	m_active = false;
	m_timeActive = 0.0f;

	ResetInternal();
}

Bool CBehaviorGraphInstance::IsActive() const
{
	return m_active;
}

CBehaviorGraph* CBehaviorGraphInstance::GetGraph()
{
	return m_root ? const_cast< CBehaviorGraphTopLevelNode* >( m_root )->GetGraph() : NULL;
}

const CBehaviorGraph* CBehaviorGraphInstance::GetGraph() const
{
	return m_root ? m_root->GetGraph() : NULL;
}

Uint32 CBehaviorGraphInstance::GetSize() const
{
	return sizeof( CBehaviorGraphInstance ) + m_data ? m_data->GetSize() : 0;
}

void CBehaviorGraphInstance::OnEventProcessed( const CName& name )
{
	m_processedEvents.PushBackUnique( name );

	if ( m_editorListener )
	{
		m_editorListener->OnEventProcessed( name );
	}

	for ( Uint32 i=0; i<m_listeners.Size(); ++i )
	{
		m_listeners[ i ]->OnBehaviorEventProcessed( name );
	}
}

void CBehaviorGraphInstance::NotifyOfNodesActivation( const CName& name )
{
	m_activationNotifications.PushBackUnique( name );

	if ( m_editorListener )
	{
		m_editorListener->OnNotifyOfNodesActivation( name );
	}

	for ( Uint32 i=0; i<m_listeners.Size(); ++i )
	{
		m_listeners[ i ]->OnBehaviorActivationNotify( name );
	}
}

void CBehaviorGraphInstance::NotifyOfNodesDeactivation( const CName& name )
{
	m_deactivationNotifications.PushBackUnique( name );

	if ( m_editorListener )
	{
		m_editorListener->OnNotifyOfNodesDeactivation( name );
	}

	for ( Uint32 i=0; i<m_listeners.Size(); ++i )
	{
		m_listeners[ i ]->OnBehaviorDeactivationNotify( name );
	}
}

Bool CBehaviorGraphInstance::ActivationNotificationReceived( const CName& name ) const
{
	ASSERT( m_internalState == IIS_Waiting );
	return m_activationNotifications.Exist( name );
}

Bool CBehaviorGraphInstance::DeactivationNotificationReceived( const CName& name ) const
{
	ASSERT( m_internalState == IIS_Waiting );
	return m_deactivationNotifications.Exist( name );
}

void CBehaviorGraphInstance::NotifyOfScriptedNodesNotification( const CName& name, const CName & sourceState )
{
	m_scriptEventNotifications.PushBackUnique( SBehaviorGraphScriptNotification( name, sourceState ) );
}

Uint32 CBehaviorGraphInstance::GetEventProcessedNum() const
{
	return m_processedEvents.Size();	
}

const TDynArray< SBehaviorGraphScriptNotification >& CBehaviorGraphInstance::GetScriptedNodesNotifications() const
{
	return m_scriptEventNotifications;
}

Uint32 CBehaviorGraphInstance::GetActivationNotificationNum() const
{
	return m_activationNotifications.Size();
}

Uint32 CBehaviorGraphInstance::GetDeactivationNotificationNum() const
{
	return m_deactivationNotifications.Size();
}

void CBehaviorGraphInstance::GenerateEditorFragments( CRenderFrame* frame )
{
	ASSERT( m_root );

	if ( !m_root || !m_active )
	{
		return;
	}

	const CBehaviorGraph* graph = m_root->GetGraph();
	if ( graph->GenerateEditorFragments() == false )
	{
		return;
	}
	
	m_root->OnGenerateFragments( *this, frame );
}

void CBehaviorGraphInstance::Update( SBehaviorUpdateContext& context, Float timeDelta )
{
	m_internalState = IIS_Updating;
	
	ASSERT( IsActive() );
	ASSERT( IsBinded() );

	PC_SCOPE( BehaviorUpdate );

	if ( m_editorListener )
	{
		m_editorListener->OnPreUpdateInstance( timeDelta );
	}

	// update time - this also means that we will never have time active 0
	m_timeActive += timeDelta;

	// Update constraints
	for ( Int32 i=(Int32)m_constraints.Size()-1; i>=0; --i )
	{
		IAnimationConstraint* constraint = m_constraints[i];

		constraint->Update( *this, timeDelta );

		if ( constraint->IsFinished() )
		{
			m_constraints.Erase( m_constraints.Begin() + i );
			constraint->Deactivate( *this );
			delete constraint;
		}
	}

	// Clear processed event list
	m_processedEvents.ClearFast();

	// Clear notification lists
	m_activationNotifications.ClearFast();
	m_deactivationNotifications.ClearFast();
	m_scriptEventNotifications.ClearFast();

	// Update graph
	m_root->Update( context, *this, timeDelta );

	if ( m_editorListener )
	{
		m_editorListener->OnPostUpdateInstance( timeDelta );
	}

	m_internalState = IIS_Waiting;
}

void CBehaviorGraphInstance::Sample( SBehaviorSampleContext& context, SBehaviorGraphOutput& pose )
{
	m_internalState = IIS_Sampling;

	ASSERT( IsActive() );
	ASSERT( IsBinded() );

	PC_SCOPE( BehaviorSample );

	if ( m_editorListener )
	{
		m_editorListener->OnPreSampleInstance();
	}

	DEBUG_ANIM_POSES( pose )

	m_root->Sample( context, *this, pose );
	
	DEBUG_ANIM_POSES( pose )

	if ( m_editorListener )
	{
		m_editorListener->OnPostSampleInstance( pose );
	}

	m_internalState = IIS_Waiting;
}

void CBehaviorGraphInstance::Reset()
{
	ASSERT( m_root );
	ASSERT( IsBinded() );
	ASSERT( m_internalState == IIS_Waiting );

	m_root->Deactivate( *this );

	ResetInternal();
	
	m_root->Activate( *this );
}

void CBehaviorGraphInstance::ResetInternal()
{
	ASSERT( m_root );
	ASSERT( IsBinded() );
	ASSERT( m_internalState == IIS_Waiting );

	if ( m_editorListener )
	{
		m_editorListener->OnReset();
	}

	ResetVariables();

	m_root->Reset( *this );

	m_processedEvents.Clear();
	m_activationNotifications.Clear();
	m_deactivationNotifications.Clear();
	m_scriptEventNotifications.Clear();
}

void CBehaviorGraphInstance::ResetVariables()
{
	ASSERT( m_internalState == IIS_Waiting );
	ASSERT( m_root );
	const CBehaviorGraph* graph = m_root->GetGraph();
	graph->ResetRuntimeVariables( *this );
}

void CBehaviorGraphInstance::FillVariables( const CBehaviorGraph* graph )
{
	ASSERT( m_internalState == IIS_Waiting );
	graph->FillRuntimeVariables( *this );
}

EVectorVariableType CBehaviorGraphInstance::GetVectorVariableType( CName name ) const
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return VVT_Position;
	}

	const CBehaviorGraph* graph = m_root->GetGraph();

	CBehaviorVectorVariable* var = graph->GetVectorVariables().GetVariable( name );
	ASSERT( var );

	return var ? var->GetType() : VVT_Position;
}

Uint32 CBehaviorGraphInstance::GetEventId( const CName& name ) const
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return 0;
	}

	const CBehaviorGraph* graph = m_root->GetGraph();

	const CBehaviorEventsList& events = graph->GetEvents();

	return events.GetEventId( name );
}

const CName& CBehaviorGraphInstance::GetEventName( Uint32 id ) const
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return CName::NONE;
	}

	const CBehaviorGraph* graph = m_root->GetGraph();

	const CBehaviorEventsList& events = graph->GetEvents();

	return events.FindEventNameById( id );
}

Bool CBehaviorGraphInstance::GenerateEvent( const CName& name )
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return false;
	}

	if ( m_editorListener )
	{
		m_editorListener->OnGenerateEvent( name );
	}

	Uint32 eventID = GetEventId( name );

	if ( eventID != CBehaviorEventsList::NO_EVENT )
	{
		CBehaviorEvent event( eventID );
		return m_root->ProcessEvent( *this, event );
	}

	return false;
}

Bool CBehaviorGraphInstance::GenerateForceEvent( const CName& name )
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return false;
	}

	if ( m_editorListener )
	{
		m_editorListener->OnGenerateForceEvent( name );
	}

	Uint32 eventID = GetEventId( name );

	if ( eventID != CBehaviorEventsList::NO_EVENT )
	{
		CBehaviorEvent event( eventID );
		return m_root->ProcessForceEvent( *this, event );
	}

	return false;
}

void CBehaviorGraphInstance::AddListener( IBehaviorGraphInstanceListener* listener )
{
	m_listeners.PushBackUnique( listener );
}

Bool CBehaviorGraphInstance::RemoveListener( IBehaviorGraphInstanceListener* listener )
{
	return m_listeners.Remove( listener );
}

#ifndef NO_EDITOR

void CBehaviorGraphInstance::SetEditorListener( IBehaviorGraphInstanceEditorListener* listener )
{
	ASSERT( !m_editorListener );
	m_editorListener = listener;
}

void CBehaviorGraphInstance::RemoveEditorListener()
{
	m_editorListener = NULL;
}

Bool CBehaviorGraphInstance::HasEditorListener() const 
{ 
	return m_editorListener != NULL; 
}

IBehaviorGraphInstanceEditorListener* CBehaviorGraphInstance::GetEditorListener() const 
{ 
	return m_editorListener; 
}

#endif

Bool CBehaviorGraphInstance::IsEventProcessed( const CName& name ) const
{
	ASSERT( m_internalState == IIS_Waiting );
	return m_processedEvents.Exist( name );
}

Int32 CBehaviorGraphInstance::FindBoneByName( const CName& boneName ) const
{
	if ( !boneName )
	{
		return -1;
	}

	return FindBoneByName( boneName.AsString().AsChar() );
}

Int32 CBehaviorGraphInstance::FindBoneByName( const Char* boneName ) const
{
	if ( !boneName )
	{
		return -1;
	}

	if ( !m_animatedComponent )
	{
		return -1;
	}

	return m_animatedComponent->FindBoneByName( boneName );
}

Bool CBehaviorGraphInstance::HasFloatValue( const CName name ) const
{
	if ( !m_root )
	{
		return false;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->HasRuntimeFloatVariable( *this, name );
}

Bool CBehaviorGraphInstance::HasInternalFloatValue( const CName name ) const
{
	if ( !m_root )
	{
		return false;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->HasRuntimeInternalFloatVariable( *this, name );
}

Bool CBehaviorGraphInstance::HasVectorValue( const CName name ) const
{
	if ( !m_root )
	{
		return false;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->HasRuntimeVectorVariable( *this, name );
}

const Float* CBehaviorGraphInstance::GetFloatValuePtr( const CName name ) const
{
	if ( !m_root )
	{
		return nullptr;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->GetRuntimeFloatVariablePtr( *this, name );
}

const Float* CBehaviorGraphInstance::GetInternalFloatValuePtr( const CName name ) const
{
	if ( !m_root )
	{
		return nullptr;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->GetRuntimeInternalFloatVariablePtr( *this, name );
}

const Vector* CBehaviorGraphInstance::GetVectorValuePtr( const CName name ) const
{
	if ( !m_root )
	{
		return nullptr;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->GetRuntimeVectorVariablePtr( *this, name );
}

Vector CBehaviorGraphInstance::GetVectorValue( const CName name ) const
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return Vector::ZERO_3D_POINT;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->GetRuntimeVectorVariable( *this, name );
}

Float CBehaviorGraphInstance::GetFloatValue( const CName name, Float defValue ) const
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return defValue;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->GetRuntimeFloatVariable( *this, name, defValue );
}

Bool CBehaviorGraphInstance::SetVectorValue( const CName name, const Vector& value )
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return false;
	}

	const CBehaviorGraph* graph = m_root->GetGraph();
	if ( m_editorListener )
	{
		m_editorListener->OnSetVectorValue( name, value );
	}

	return graph->SetRuntimeVectorVariable( *this, name, value );
}

Bool CBehaviorGraphInstance::SetFloatValue( const CName name, const Float value )
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return false;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();

	if ( m_editorListener )
	{
		m_editorListener->OnSetFloatValue( name, value );
	}

	return graph->SetRuntimeFloatVariable( *this, name, value );
}

Vector CBehaviorGraphInstance::ResetVectorValue( CName name )
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return Vector::ZERO_3D_POINT;
	}

	if ( m_editorListener )
	{
		m_editorListener->OnResetVectorValue( name );
	}

	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->ResetRuntimeVectorVariable( *this, name );
}

Float CBehaviorGraphInstance::ResetFloatValue( CName name )
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return 0;
	}

	if ( m_editorListener )
	{
		m_editorListener->OnResetVectorValue( name );
	}

	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->ResetRuntimeFloatVariable( *this, name );
}

Uint32 CBehaviorGraphInstance::GetFloatValuesNum()
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return 0;
	}

	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->GetRuntimeFloatVariablesNum( *this );
}

Uint32 CBehaviorGraphInstance::GetVectorValuesNum()
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return 0;
	}

	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->GetRuntimeVectorVariablesNum( *this );
}

Float CBehaviorGraphInstance::GetFloatValueMin( CName name ) const
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return 0.f;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->GetFloatVariableMin( name );
}

Float CBehaviorGraphInstance::GetFloatValueMax( CName name ) const
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return 0.f;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->GetFloatVariableMax( name );
}

Float CBehaviorGraphInstance::GetFloatValueDefault( CName name ) const
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return 0.f;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->GetFloatVariableDefault( name );
}

Vector CBehaviorGraphInstance::GetVectorValueMin( CName name ) const
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return Vector::ZERO_3D_POINT;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->GetVectorVariableMin( name );
}

Vector CBehaviorGraphInstance::GetVectorValueMax( CName name ) const
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return Vector::ZERO_3D_POINT;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->GetVectorVariableMax( name );
}

Vector CBehaviorGraphInstance::GetVectorValueDefault( CName name ) const
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return Vector::ZERO_3D_POINT;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->GetVectorVariableDefault( name );
}

Vector CBehaviorGraphInstance::GetInternalVectorValue( const CName name ) const
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return Vector::ZERO_3D_POINT;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->GetRuntimeInternalVectorVariable( *this, name );
}

Float CBehaviorGraphInstance::GetInternalFloatValue( const CName name ) const
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return 0.f;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->GetRuntimeInternalFloatVariable( *this, name );
}

Bool CBehaviorGraphInstance::SetInternalVectorValue( const CName name, const Vector& value )
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return false;
	}

	const CBehaviorGraph* graph = m_root->GetGraph();

	if ( m_editorListener )
	{
		m_editorListener->OnSetInternalVectorValue( name, value );
	}

	return graph->SetRuntimeInternalVectorVariable( *this, name, value );
}

Bool CBehaviorGraphInstance::SetInternalFloatValue( const CName name, const Float value )
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return false;
	}

	const CBehaviorGraph* graph = m_root->GetGraph();

	if ( m_editorListener )
	{
		m_editorListener->OnSetInternalFloatValue( name, value );
	}

	return graph->SetRuntimeInternalFloatVariable( *this, name, value );
}

Vector CBehaviorGraphInstance::ResetInternalVectorValue( CName name )
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return Vector::ZERO_3D_POINT;
	}

	if ( m_editorListener )
	{
		m_editorListener->OnResetInternalVectorValue( name );
	}

	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->ResetRuntimeInternalVectorVariable( *this, name );
}

Float CBehaviorGraphInstance::ResetInternalFloatValue( CName name )
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return 0;
	}

	if ( m_editorListener )
	{
		m_editorListener->OnResetInternalVectorValue( name );
	}

	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->ResetRuntimeInternalFloatVariable( *this, name );
}

Uint32 CBehaviorGraphInstance::GetInternalFloatValuesNum()
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return 0;
	}

	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->GetRuntimeInternalFloatVariablesNum( *this );
}

Uint32 CBehaviorGraphInstance::GetInternalVectorValuesNum()
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return 0;
	}

	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->GetRuntimeInternalVectorVariablesNum( *this );
}

Float CBehaviorGraphInstance::GetInternalFloatValueMin( CName varName ) const
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return 0.f;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->GetInternalFloatVariableMin( varName );
}

Float CBehaviorGraphInstance::GetInternalFloatValueMax( CName varName ) const
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return 0.f;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->GetInternalFloatVariableMax( varName );
}

Float CBehaviorGraphInstance::GetInternalFloatValueDefault( CName varName ) const
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return 0.f;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->GetInternalFloatVariableDefault( varName );
}

Vector CBehaviorGraphInstance::GetInternalVectorValueMin( CName varName ) const
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return Vector::ZERO_3D_POINT;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->GetInternalVectorVariableMin( varName );
}

Vector CBehaviorGraphInstance::GetInternalVectorValueMax( CName varName ) const
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return Vector::ZERO_3D_POINT;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->GetInternalVectorVariableMax( varName );
}

Vector CBehaviorGraphInstance::GetInternalVectorValueDefault( CName varName ) const
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return Vector::ZERO_3D_POINT;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	return graph->GetInternalVectorVariableDefault( varName );
}

CBehaviorGraphPoseSlotNode*	CBehaviorGraphInstance::FindPoseSlot( const CName &slotName ) const
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return NULL;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	
	if ( graph->IsSourceDataRemoved() )
	{
		const TDynArray< CBehaviorGraphPoseSlotNode* >& slots = graph->GetPoseSlots();
		for ( Uint32 i=0; i<slots.Size(); ++i )
		{
			CBehaviorGraphPoseSlotNode* slot = slots[ i ];

			if ( slot->GetSlotName() == slotName && slot->IsActive( *this ) )
			{
				return slot;
			}
		}
		return NULL;
	}
	else
	{
		return Cast< CBehaviorGraphPoseSlotNode >( FindNodeByName( slotName.AsString(), true ) );
	}
}

CBehaviorGraphAnimationBaseSlotNode* CBehaviorGraphInstance::FindAnimSlot( const CName &slotName, Bool onlyActive ) const
{
	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return NULL;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();

	if ( graph->IsSourceDataRemoved() )
	{
		const TDynArray< CBehaviorGraphAnimationBaseSlotNode* >& slots = graph->GetAnimSlots();
		for ( Uint32 i=0; i<slots.Size(); ++i )
		{
			CBehaviorGraphAnimationBaseSlotNode* slot = slots[ i ];

			if ( slot->GetSlotName() == slotName )
			{
				if ( onlyActive && !slot->IsActive( *this ) )
				{
					continue;
				}

				return slot;
			}
		}
		return NULL;
	}
	else
	{
		return Cast< CBehaviorGraphAnimationBaseSlotNode >( FindNodeByName( slotName.AsString(), onlyActive ) );
	}
}

CBehaviorGraphNode*	CBehaviorGraphInstance::FindNodeByName( const String &name, Bool onlyActive ) const
{
	if ( !IsBinded() )
	{
		ASSERT( IsBinded() && TXT("Behavior instance is unbined") );
		return NULL;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();

	if ( onlyActive )
	{
		TDynArray< CBehaviorGraphNode* > nodes;
		graph->FindNodesByName( name, nodes );

		for ( Uint32 i=0; i<nodes.Size(); ++i )
		{
			CBehaviorGraphNode* node = nodes[i];
			if ( node->IsActive( *this ) )
			{
				return node;
			}
		}

		return NULL;
	}
	else
	{
		return graph->FindNodeByName( name );
	}
}

CBehaviorGraphNode*	CBehaviorGraphInstance::FindNodeByName( const String &name, CBehaviorGraphNode* startNode, Bool recursive, Bool onlyActive ) const
{
	if ( !IsBinded() )
	{
		ASSERT( IsBinded() && TXT("Behavior instance is unbined") );
		return NULL;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();

	if ( onlyActive )
	{
		TDynArray< CBehaviorGraphNode* > nodes;
		graph->FindNodesByName( name, nodes );

		for ( Uint32 i=0; i<nodes.Size(); ++i )
		{
			CBehaviorGraphNode* node = nodes[i];
			if ( node->IsActive( *this ) )
			{
				return node;
			}
		}

		return NULL;
	}
	else
	{
		return graph->FindNodeByName( name, startNode, recursive );
	}
}

void CBehaviorGraphInstance::OnOpenInEditor()
{
#ifndef NO_EDITOR_GRAPH_SUPPORT
	m_isOpenInEditor = true;
#endif
	if ( m_root )
	{
		m_root->OnOpenInEditor( *this );
	}
}

void CBehaviorGraphInstance::ClearActivationAlphas()
{
	if ( !IsBinded() )
	{
		ASSERT( IsBinded() && TXT("Behavior instance is unbined") );
		return;
	}

	TQueue< CBehaviorGraphNode* > containerNodes;

	CBehaviorGraphTopLevelNode* root = const_cast< CBehaviorGraphTopLevelNode* >( m_root );

	containerNodes.Push( root );

	while( !containerNodes.Empty() )
	{
		CBehaviorGraphNode* currNode = containerNodes.Front();

		currNode->SetActivationAlpha( *this, 0.0f );

		CBehaviorGraphContainerNode* containerNode = Cast< CBehaviorGraphContainerNode >( currNode );

		if ( containerNode )
		{
			for( Uint32 i=0; i<containerNode->GetConnectedChildren().Size(); ++i )
			{
				if ( CBehaviorGraphNode* bgNode = Cast< CBehaviorGraphNode >( containerNode->GetConnectedChildren()[i] ) )
				{
					containerNodes.Push( bgNode );
				}
			}
		}

		containerNodes.Pop();
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphInstance::EnumVectorVariableNames( TDynArray< CName >& names ) const
{
	if ( !IsBinded() )
	{
		ASSERT( IsBinded() && TXT("Behavior instance is unbined") );
		return;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	graph->EnumVectorVariableNames( names );
}

void CBehaviorGraphInstance::EnumVariableNames( TDynArray< CName >& names, Bool onlyModifiableByEffect ) const
{
	if ( !IsBinded() )
	{
		ASSERT( IsBinded() && TXT("Behavior instance is unbined") );
		return;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	graph->EnumVariableNames( names, onlyModifiableByEffect );
}

void CBehaviorGraphInstance::EnumEventNames( TDynArray< CName> & names, Bool onlyModifiableByEffect ) const
{
	if ( !IsBinded() )
	{
		ASSERT( IsBinded() && TXT("Behavior instance is unbined") );
		return;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	graph->EnumEventNames( names, onlyModifiableByEffect );
}

void CBehaviorGraphInstance::EnumInternalVectorVariableNames( TDynArray< CName >& names ) const
{
	if ( !IsBinded() )
	{
		ASSERT( IsBinded() && TXT("Behavior instance is unbined") );
		return;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	graph->EnumInternalVectorVariableNames( names );
}

void CBehaviorGraphInstance::EnumInternalVariableNames( TDynArray< CName >& names, Bool onlyModifiableByEffect ) const
{
	if ( !IsBinded() )
	{
		ASSERT( IsBinded() && TXT("Behavior instance is unbined") );
		return;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	graph->EnumInternalVariableNames( names, onlyModifiableByEffect );
}

#endif

void CBehaviorGraphInstance::ProcessActivationAlphas()
{
	if ( !IsBinded() )
	{
		ASSERT( IsBinded() && TXT("Behavior instance is unbined") );
		return;
	}

	// Clear all alphas at the beginning
	ClearActivationAlphas();

	// Now, do the processing, starting with rootNode
	m_root->ProcessActivationAlpha( *this, 1.0f );
}

void CBehaviorGraphInstance::UpdateCachedAnimationPointers()
{
	if ( m_root )
	{
		m_root->OnUpdateAnimationCache( *this );
	}
}

Bool CBehaviorGraphInstance::ActivateConstraint( const Vector &target, 
												 const CName activationVariableName, 
												 const CName variableToControlName, 
												 Float timeout )
{
	ASSERT( m_internalState == IIS_Waiting );

	const Bool hasControlVar = HasFloatValue( activationVariableName );
	const Bool hasVarToControl = HasFloatValue( variableToControlName );

	if ( !hasControlVar || !hasVarToControl )
	{
		return false;
	}

	IAnimationConstraint* constraint = new CAnimationConstraintStatic( target, activationVariableName, variableToControlName, timeout );
	m_constraints.PushBack(constraint);

	if ( m_editorListener )
	{
		m_editorListener->OnActivateConstraint( constraint );
	}

	return true;
}

Bool CBehaviorGraphInstance::ActivateConstraint( const CNode* target, 
												 const CName activationVariableName, 
												 const CName variableToControlName, 
												 Float timeout )
{
	ASSERT( m_internalState == IIS_Waiting );

	if ( !HasFloatValue( activationVariableName ) || !HasVectorValue( variableToControlName ) )
	{
		return false;
	}

	IAnimationConstraint* constraint = new CAnimationConstraint( target, activationVariableName, variableToControlName, timeout );

	m_constraints.PushBack(constraint);

	if ( m_editorListener )
	{
		m_editorListener->OnActivateConstraint( constraint );
	}

	return true;
}

Bool CBehaviorGraphInstance::ActivateConstraint( const CAnimatedComponent* target, 
												 const Int32 boneIndex,
												 const CName activationVariableName, 
												 const CName variableToControlName,
												 Bool useOffset,
												 const Matrix& offsetMatrix,
												 Float timeout
												 )
{
	ASSERT( m_internalState == IIS_Waiting );
	
	if ( !HasFloatValue( activationVariableName ) || !HasVectorValue( variableToControlName ) )
	{
		return false;
	}

	CAnimationBoneConstraint* constraint = new CAnimationBoneConstraint( target, boneIndex, activationVariableName, variableToControlName, useOffset, offsetMatrix, timeout );
	
	m_constraints.PushBack(constraint);

	if ( m_editorListener )
	{
		m_editorListener->OnActivateConstraint( constraint );
	}
	
	return true;
}

Bool CBehaviorGraphInstance::ChangeConstraintTarget( const CNode* target, const CName activationVariableName, const CName variableToControlName, Float timeout )
{
	ASSERT( m_internalState == IIS_Waiting );

	if ( HasConstraint( activationVariableName ) )
	{
		Bool ret = DeactivateConstraint( activationVariableName );
		ASSERT( ret );
		return ActivateConstraint( target, activationVariableName, variableToControlName, timeout );
	}

	return false;
}

Bool CBehaviorGraphInstance::ChangeConstraintTarget( const Vector &target, const CName activationVariableName, const CName variableToControlName, Float timeout )
{
	ASSERT( m_internalState == IIS_Waiting );

	if ( HasConstraint( activationVariableName ) )
	{
		Bool ret = DeactivateConstraint( activationVariableName );
		ASSERT( ret );
		return ActivateConstraint( target, activationVariableName, variableToControlName, timeout );
	}

	return false;
}

Bool CBehaviorGraphInstance::ChangeConstraintTarget( const CAnimatedComponent* target, const Int32 boneIndex, const CName activationVariableName, const CName variableToControlName, Bool useOffset, const Matrix& offsetMatrix, Float timeout )
{
	ASSERT( m_internalState == IIS_Waiting );

	if ( boneIndex < 0 )
	{
		return false;
	}

	if ( HasConstraint( activationVariableName ) )
	{
		Bool ret = DeactivateConstraint( activationVariableName );
		ASSERT( ret );
		return ActivateConstraint( target, boneIndex, activationVariableName, variableToControlName, useOffset, offsetMatrix, timeout );
	}

	return false;
}

Bool CBehaviorGraphInstance::GetConstraintTarget( const CName activationVariableName, Vector& value )
{
	ASSERT( m_internalState == IIS_Waiting );

	if ( HasFloatValue( activationVariableName ) )
	{
		for ( Uint32 i=0; i<m_constraints.Size(); ++i )
		{
			if ( m_constraints[i]->IsUnderControlBy( activationVariableName ) )
			{
				value = m_constraints[i]->GetControledValue( *this );
				return true;
			}
		}
	}
	
	return false;
}

Uint32 CBehaviorGraphInstance::GetConstraintsNum() const
{
	return m_constraints.Size();
}

Bool CBehaviorGraphInstance::DeactivateConstraint( const CName activationVariableName )
{
	ASSERT( m_internalState == IIS_Waiting );

	Bool result = false;

	for ( Int32 i=(Int32)m_constraints.Size()-1; i>=0; i-- )
	{
		if ( m_constraints[i]->IsUnderControlBy( activationVariableName ) )
		{
			IAnimationConstraint* constraint = m_constraints[i];
			m_constraints.Erase( m_constraints.Begin() + i );

			if ( m_editorListener )
			{
				m_editorListener->OnDeactivateConstraint( constraint );
			}

			constraint->Deactivate( *this );
			delete constraint;
			result = true;
		}
	}

	return result;
}

Bool CBehaviorGraphInstance::HasConstraint( const CName activationVariableName ) const
{
	for ( Uint32 i=0; i<m_constraints.Size(); ++i )
	{
		if ( m_constraints[i]->IsUnderControlBy( activationVariableName ) )
		{
			return true;
		}
	}

	return false;
}
Bool CBehaviorGraphInstance::PauseSlotAnimation( const CName& slot, Bool pause )
{
	CBehaviorGraphAnimationBaseSlotNode* node = FindAnimSlot( slot );
	if ( node )
	{		
		node->PauseAnimation( *this, pause );
		return true;
	}

	return false;
}

Bool CBehaviorGraphInstance::PlaySlotAnimation( const CName& slot, const CName& animation, const SBehaviorSlotSetup* slotSetup )
{
	ASSERT( m_internalState == IIS_Waiting );

	CBehaviorGraphAnimationBaseSlotNode* node = FindAnimSlot( slot );
	if ( node )
	{
		if ( m_editorListener )
		{
			m_editorListener->OnPlaySlotAnimation( node, animation, slotSetup );
		}

		return node->PlayAnimation( *this, animation, slotSetup);
	}

	return false;
}

Bool CBehaviorGraphInstance::PlaySlotAnimation( const CName& slot, CSkeletalAnimationSetEntry* skeletalAnimation, const SBehaviorSlotSetup* slotSetup, Bool onlyActive )
{
	ASSERT( m_internalState == IIS_Waiting );

	CBehaviorGraphAnimationBaseSlotNode* node = FindAnimSlot( slot, onlyActive );
	if ( node )
	{
		if ( m_editorListener )
		{
			m_editorListener->OnPlaySlotAnimation( node, skeletalAnimation, slotSetup );
		}

		return node->PlayAnimation( *this, skeletalAnimation, slotSetup );
	}

	return false;
}

Bool CBehaviorGraphInstance::StopSlotAnimation( const CName& slot, Float blendOutTime, Bool onlyActive )
{
	ASSERT( m_internalState == IIS_Waiting );

	CBehaviorGraphAnimationBaseSlotNode* node = FindAnimSlot( slot, onlyActive );
	if ( node )
	{
		if ( m_editorListener )
		{
			m_editorListener->OnStopSlotAnimation( node );
		}

		return node->StopAnimation( *this, blendOutTime );
	}

	return false;
}

Bool CBehaviorGraphInstance::HasSlotAnimation( const CName& slot, Bool onlyActive ) const
{
	ASSERT( m_internalState == IIS_Waiting );

	CBehaviorGraphAnimationBaseSlotNode* node = FindAnimSlot( slot, onlyActive );
	if ( node )
	{
		return node->IsPlayingSlotAnimation( *this );
	}

	return false;
}

Bool CBehaviorGraphInstance::GetLookAt( const CName& nodeId, CBehaviorPointCloudLookAtInterface& nodeInterface )
{
	ASSERT( m_internalState == IIS_Waiting );

	TDynArray< CBehaviorGraphPointCloudLookAtNode* > nodes;
	GetNodesOfClass( nodes );

	const Uint32 size = nodes.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CBehaviorGraphPointCloudLookAtNode* node = nodes[ i ];
		if ( node->GetLookAtName() == nodeId )
		{
			nodeInterface.Init( node, this );
			return true;
		}
	}

	return false;
}

Bool CBehaviorGraphInstance::GetSlot( const CName& slot, IBehaviorGraphSlotInterface& slotInterface )
{
	ASSERT( m_internalState == IIS_Waiting );

	CBehaviorGraphAnimationBaseSlotNode* node = FindAnimSlot( slot );
	if ( node )
	{
		slotInterface.Init( node, this );

		return true;
	}

	return false;
}

Bool CBehaviorGraphInstance::GetSlot( const CName& slot, CBehaviorManualSlotInterface& slotInterface, Bool onlyActive )
{
	ASSERT( m_internalState == IIS_Waiting );

	CBehaviorGraphAnimationManualSlotNode* node = Cast< CBehaviorGraphAnimationManualSlotNode >( FindNodeByName( slot.AsString(), onlyActive ) );
	if ( node )
	{
		slotInterface.Init( node, this );

		return true;
	}

	return false;
}

Bool CBehaviorGraphInstance::GetSlot( const CName& slot, CBehaviorMixerSlotInterface& slotInterface, Bool onlyActive )
{
	ASSERT( m_internalState == IIS_Waiting );

	CBehaviorGraphAnimationMixerSlotNode* node = Cast< CBehaviorGraphAnimationMixerSlotNode >( FindNodeByName( slot.AsString(), onlyActive ) );
	if ( node )
	{
		slotInterface.Init( node, this );

		return true;
	}

	return false;
}

Bool CBehaviorGraphInstance::SetNeedRefreshSyncTokensOnSlot( const CName& slot, Bool value )
{
	ASSERT( m_internalState == IIS_Waiting );

	CBehaviorGraphAnimationBaseSlotNode* node = FindAnimSlot( slot );
	if ( node )
	{
		node->SetNeedRefreshSyncTokens( *this, value );

		return true;
	}

	return false;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
Bool CBehaviorGraphInstance::AppendSyncTokenForEntityOnSlot( const CName& slot, const CEntity* entity )
{
	ASSERT( m_internalState == IIS_Waiting );

	CBehaviorGraphAnimationBaseSlotNode* node = FindAnimSlot( slot );
	if ( node )
	{
		node->AppendSyncTokenForEntity( *this, entity );

		return true;
	}

	return false;
}
#endif

Bool CBehaviorGraphInstance::HasSlotListener( ISlotAnimationListener* listener ) const
{
	ASSERT( m_internalState == IIS_Waiting );

	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return NULL;
	}

	const CBehaviorGraph* graph = m_root->GetGraph();

	if ( graph->IsSourceDataRemoved() )
	{
		const TDynArray< CBehaviorGraphAnimationBaseSlotNode* >& slots = graph->GetAnimSlots();
		for ( Uint32 i=0; i<slots.Size(); ++i )
		{
			CBehaviorGraphAnimationBaseSlotNode* slot = slots[ i ];

			if ( slot->HasListener( *this, listener ) )
			{
				return true;
			}
		}
	}
	else
	{
		TDynArray< CBehaviorGraphNode* > nodes;
		for ( Uint32 i=0; i<nodes.Size(); ++i )
		{
			CBehaviorGraphAnimationBaseSlotNode* slot = Cast< CBehaviorGraphAnimationBaseSlotNode >( nodes[ i ] );

			if ( slot && slot->HasListener( *this, listener ) )
			{
				return true;
			}
		}
	}
	return false;
}

const CName& CBehaviorGraphInstance::GetSlotAnimation( const CName& slot ) const
{
	CBehaviorGraphAnimationBaseSlotNode* node = FindAnimSlot( slot );
	if ( node )
	{
		return node->GetAnimationName();
	}

	return CName::NONE;
}

Bool CBehaviorGraphInstance::DetachSlotListener( const CName& slotName, ISlotAnimationListener* listener )
{
	ASSERT( m_internalState == IIS_Waiting );

	if ( !m_root )
	{
		ASSERT( m_root && TXT("Behavior instance is unbined") );
		return false;
	}

	const CBehaviorGraph* graph = m_root->GetGraph();

	Bool ret = false;

	if ( graph->IsSourceDataRemoved() )
	{
		const TDynArray< CBehaviorGraphAnimationBaseSlotNode* >& slots = graph->GetAnimSlots();
		for ( Uint32 i=0; i<slots.Size(); ++i )
		{
			CBehaviorGraphAnimationBaseSlotNode* slot = slots[ i ];

			if ( slot->GetSlotName() == slotName )
			{
				ret |= slot->DetachListener( *this, listener );
			}
		}
	}
	else
	{
		TDynArray< CBehaviorGraphAnimationBaseSlotNode* > slots;
		GetNodesOfClass( slots );

		for ( Uint32 i=0; i<slots.Size(); ++i )
		{
			CBehaviorGraphAnimationBaseSlotNode* slot = slots[ i ];

			if ( slot->GetSlotName() == slotName )
			{
				ret |= slot->DetachListener( *this, listener );
			}
		}
	}

	return ret;
}

void CBehaviorGraphInstance::EnumAnimationSlots( TDynArray< String >& slots ) const
{
	TDynArray<CBehaviorGraphAnimationBaseSlotNode*> animSlots;
	GetNodesOfClass<CBehaviorGraphAnimationBaseSlotNode>( animSlots );

	for ( Uint32 i=0; i<animSlots.Size(); i++ )
	{
		if ( !animSlots[i]->GetName().Empty() )
		{
			slots.PushBack( animSlots[i]->GetName() );
		}
	}
}

Bool CBehaviorGraphInstance::SetSlotPose( const CName& slot, const CAnimatedComponent* componentWithPoseLS, Float blendTime, EBlendType type, IPoseSlotListener* l )
{
	ASSERT( m_internalState == IIS_Waiting );

	CBehaviorGraphPoseSlotNode* node = FindPoseSlot( slot );
	if ( node )
	{
		if ( m_editorListener )
		{
			m_editorListener->OnSetSlotPose( node, blendTime, type );
		}

		node->SetPose( *this, componentWithPoseLS, blendTime, type, l );

		return true;
	}

	return false;
}

Bool CBehaviorGraphInstance::SetSlotPose( const CName& slot, const TDynArray< AnimQsTransform >&poseLS, const TDynArray< Float >& floatTracks, Float blendTime, EBlendType type, IPoseSlotListener* l, const Matrix& localToWorld )
{
	ASSERT( m_internalState == IIS_Waiting );

	CBehaviorGraphPoseSlotNode* node = FindPoseSlot( slot );
	if ( node )
	{
		if ( m_editorListener )
		{
			m_editorListener->OnSetSlotPose( node, blendTime, type );
		}

		node->SetPose( *this, poseLS, floatTracks, localToWorld, blendTime, type, l );

		return true;
	}

	return false;
}

Bool CBehaviorGraphInstance::ResetSlotPose( const CName& slot )
{
	ASSERT( m_internalState == IIS_Waiting );

	CBehaviorGraphPoseSlotNode* node = FindPoseSlot( slot );
	if ( node )
	{
		if ( m_editorListener )
		{
			m_editorListener->OnResetSlotPose( node );
		}

		node->ResetPose( *this );

		return true;
	}

	return false;
}

Bool CBehaviorGraphInstance::IsPoseSlotActive( const CName& slot ) const
{
	CBehaviorGraphPoseSlotNode* node = FindPoseSlot( slot );
	if ( node )
	{
		return node->IsSlotActive( *this );
	}

	return false;
}

void CBehaviorGraphInstance::EnumPoseSlots( TDynArray< String >& slots ) const
{
	TDynArray<CBehaviorGraphPoseSlotNode*> poseSlots;
	GetNodesOfClass<CBehaviorGraphPoseSlotNode>( poseSlots );

	for ( Uint32 i=0; i<poseSlots.Size(); i++ )
	{
		if ( !poseSlots[i]->GetName().Empty() ) 
		{
			slots.PushBack( poseSlots[i]->GetName() );
		}
	}
}

Bool CBehaviorGraphInstance::GetSyncInfo( CBehaviorSyncInfo& info )
{
	ASSERT( m_internalState == IIS_Waiting );

	ASSERT( m_root && TXT("Behavior instance is unbined") );
	if ( !m_root )
	{
		return false;
	}

	info.m_instanceName = m_instanceName;

	const CBehaviorGraph* graph = m_root->GetGraph();
	const CBehaviorGraphStateMachineNode* machine = graph->GetDefaultStateMachine();

	return machine ? machine->GetSyncInfoForInstance( info, *this ) : false;
}

Bool CBehaviorGraphInstance::GetOutboundSyncTags( SBehaviorSyncTags& tags, const CBehaviorGraphStateMachineNode* startingWithStateMachine )
{
	ASSERT( m_internalState == IIS_Waiting || m_internalState == IIS_Updating );

	ASSERT( m_root && TXT("Behavior instance is unbined") );
	if ( !m_root )
	{
		return false;
	}

	Bool result = false;
	Bool gather = startingWithStateMachine == nullptr;
	const CBehaviorGraph* graph = m_root->GetGraph();
	const TDynArray< CBehaviorGraphStateMachineNode* >& stateMachines = graph->GetStateMachines();
	for ( TDynArray< CBehaviorGraphStateMachineNode* >::const_iterator iSM = stateMachines.Begin(); iSM != stateMachines.End(); ++ iSM )
	{
		CBehaviorGraphStateMachineNode* sm = *iSM;
		ASSERT( sm != NULL, TXT("Cached state machine is null") );
		gather = gather || *iSM == startingWithStateMachine;
		if ( gather && sm->IsActive( *this ) )
		{
			result = sm->GetOutboundSyncTags( tags, *this ) || result;
		}
	}

	// return true if anything was gathered
	return result;
}

Bool CBehaviorGraphInstance::ApplyInboundSyncTags( SBehaviorSyncTags& tags, const CBehaviorGraphStateMachineNode* afterStateMachine )
{
	ASSERT( m_internalState == IIS_Waiting || m_internalState == IIS_Updating );

	ASSERT( m_root && TXT("Behavior instance is unbined") );
	if ( !m_root )
	{
		return false;
	}

	Bool result = false;
	Bool apply = afterStateMachine == nullptr;
	const CBehaviorGraph* graph = m_root->GetGraph();
	const TDynArray< CBehaviorGraphStateMachineNode* >& stateMachines = graph->GetStateMachines();
	for ( TDynArray< CBehaviorGraphStateMachineNode* >::const_iterator iSM = stateMachines.Begin(); iSM != stateMachines.End(); ++ iSM )
	{
		CBehaviorGraphStateMachineNode* sm = *iSM;
		ASSERT( sm != NULL, TXT("Cached state machine is null") );
		if ( apply && sm->IsActive( *this ) )
		{
			result = sm->ApplyInboundSyncTags( tags, *this ) || result;
		}
		// after (!)
		apply = apply || *iSM == afterStateMachine;
	}

	// return true if anything was gathered
	return result;
}

void CBehaviorGraphInstance::StoreInstanceVariables( SBehaviorGraphInstanceStoredVariables& syncVars )
{
	const CBehaviorGraph* graph = m_root->GetGraph();
	// store values of all variables
	{
		auto vars = graph->GetVariables().GetVariables();
		for ( auto it = vars.Begin(), end = vars.End(); it != end; ++it )
		{
			if ( CBehaviorVariable* var = it->m_second )
			{
				if ( var->ShouldBeSyncedBetweenGraphs() )
				{
					syncVars.AddFloatVariable( graph, it->m_first, GetFloatValue( it->m_first, var->GetDefaultValue() ) );
				}
			}
		}
	}
	{
		auto vars = graph->GetVectorVariables().GetVariables();
		for ( auto it = vars.Begin(), end = vars.End(); it != end; ++it )
		{
			if ( CBehaviorVectorVariable* var = it->m_second )
			{
				if ( var->ShouldBeSyncedBetweenGraphs() )
				{
					syncVars.AddVectorVariable( graph, it->m_first, GetVectorValue( it->m_first ) );
				}
			}
		}
	}
	{
		auto vars = graph->GetInternalVariables().GetVariables();
		for ( auto it = vars.Begin(), end = vars.End(); it != end; ++it )
		{
			if ( CBehaviorVariable* var = it->m_second )
			{
				if ( var->ShouldBeSyncedBetweenGraphs() )
				{
					syncVars.AddInternalFloatVariable( graph, it->m_first, GetFloatValue( it->m_first, var->GetDefaultValue() ) );
				}
			}
		}
	}
	{
		auto vars = graph->GetInternalVectorVariables().GetVariables();
		for ( auto it = vars.Begin(), end = vars.End(); it != end; ++it )
		{
			if ( CBehaviorVectorVariable* var = it->m_second )
			{
				if ( var->ShouldBeSyncedBetweenGraphs() )
				{
					syncVars.AddInternalVectorVariable( graph, it->m_first, GetVectorValue( it->m_first ) );
				}
			}
		}
	}
}

void CBehaviorGraphInstance::RestoreInstanceVariables( const SBehaviorGraphInstanceStoredVariables& syncVars )
{
	const CBehaviorGraph* graph = m_root->GetGraph();
	// set variables in graph basing on sync variables
	{
		Uint32 num = syncVars.m_floatVariablesNum;
		const SBehaviorGraphInstanceStoredVariable* var = syncVars.m_floatVariables;
		for ( Uint32 idx = 0; idx < num; ++ idx, ++ var )
		{
			if ( graph->HasFloatValue( var->m_name ) )
			{
				SetFloatValue( var->m_name, var->m_floatValue );
			}
		}
	}
	{
		Uint32 num = syncVars.m_vectorVariablesNum;
		const SBehaviorGraphInstanceStoredVariable* var = syncVars.m_vectorVariables;
		for ( Uint32 idx = 0; idx < num; ++ idx, ++ var )
		{
			if ( graph->HasVectorValue( var->m_name ) )
			{
				SetVectorValue( var->m_name, var->m_vectorValue );
			}
		}
	}
	{
		Uint32 num = syncVars.m_internalFloatVariablesNum;
		const SBehaviorGraphInstanceStoredVariable* var = syncVars.m_internalFloatVariables;
		for ( Uint32 idx = 0; idx < num; ++ idx, ++ var )
		{
			if ( graph->HasInternalFloatValue( var->m_name ) )
			{
				SetInternalFloatValue( var->m_name, var->m_floatValue );
			}
		}
	}
	{
		Uint32 num = syncVars.m_internalVectorVariablesNum;
		const SBehaviorGraphInstanceStoredVariable* var = syncVars.m_internalVectorVariables;
		for ( Uint32 idx = 0; idx < num; ++ idx, ++ var )
		{
			if ( graph->HasInternalVectorValue( var->m_name ) )
			{
				SetInternalVectorValue( var->m_name, var->m_vectorValue );
			}
		}
	}
}

Bool CBehaviorGraphInstance::SynchronizeTo( const CBehaviorSyncInfo& info )
{
	ASSERT( m_internalState == IIS_Waiting );

	ASSERT( m_root && TXT("Behavior instance is unbined") );
	if ( !m_root || m_instanceName == info.m_instanceName )
	{
		return false;
	}

	const CBehaviorGraph* graph = m_root->GetGraph();
	const CBehaviorGraphStateMachineNode* machine = graph->GetDefaultStateMachine();

	return machine ? machine->SynchronizeInstanceTo( info, *this ) : false;
}

Bool CBehaviorGraphInstance::IsSynchronizing()
{
	ASSERT( m_internalState == IIS_Waiting );

	ASSERT( m_root && TXT("Behavior instance is unbined") );
	if ( !m_root )
	{
		return false;
	}

	const CBehaviorGraph* graph = m_root->GetGraph();
	const CBehaviorGraphStateMachineNode* machine = graph->GetDefaultStateMachine();

	return machine ? machine->IsSynchronizing( *this ) : false;
}

Bool CBehaviorGraphInstance::GetSyncInfo( CSyncInfo& info )
{
	ASSERT( m_internalState == IIS_Waiting );

	ASSERT( m_root && TXT("Behavior instance is unbined") );
	if ( m_root )
	{
		m_root->GetSyncInfo( *this, info );
		return true;
	}
	return false;
}

Bool CBehaviorGraphInstance::SynchronizeTo( const CSyncInfo& info )
{
	ASSERT( m_internalState == IIS_Waiting );

	ASSERT( m_root && TXT("Behavior instance is unbined") );
	if ( m_root )
	{
		m_root->SynchronizeTo( *this, info );
		return true;
	}
	return false;
}

Bool CBehaviorGraphInstance::HasDefaultStateMachine() const
{
	ASSERT( m_root && TXT("Behavior instance is unbined") );
	if ( !m_root )
	{
		return false;
	}

	const CBehaviorGraph* graph = m_root->GetGraph();

	return graph->GetDefaultStateMachine() != NULL;
}

Bool CBehaviorGraphInstance::SetStateInDefaultStateMachine( const String& state )
{
	ASSERT( m_internalState == IIS_Waiting );

	ASSERT( m_root && TXT("Behavior instance is unbined") );
	if ( !m_root )
	{
		return false;
	}

	const CBehaviorGraph* graph = m_root->GetGraph();

	const CBehaviorGraphStateMachineNode* machine = graph->GetDefaultStateMachine();

	if ( machine )
	{
		CBehaviorGraphNode* node = FindNodeByName( state, const_cast< CBehaviorGraphStateMachineNode* >( machine ), false );
		if ( node && node->IsA< CBehaviorGraphStateNode >() )
		{
			CBehaviorGraphStateNode* newState = SafeCast< CBehaviorGraphStateNode >( node );

			CBehaviorGraphNode* currNode = machine->GetCurrentState( *this );
			CBehaviorGraphStateNode* currState = Cast< CBehaviorGraphStateNode >( currNode );
			if ( currState )
			{
				ASSERT( currState->IsActive( *this ) );
				currState->Deactivate( *this );
			}

			machine->SwitchToState( newState, *this );

			newState->Activate( *this );

			return true;
		}
	}

	return false;
}

String CBehaviorGraphInstance::GetCurrentStateInDefaultStateMachine() const
{
	ASSERT( m_internalState == IIS_Waiting );

	ASSERT( m_root && TXT("Behavior instance is unbined") );
	if ( !m_root )
	{
		return String::EMPTY;
	}
	const CBehaviorGraph* graph = m_root->GetGraph();
	const CBehaviorGraphStateMachineNode* machine = graph->GetDefaultStateMachine();
	return machine ? machine->GetCurrentStateName( *this ): String::EMPTY;
}

String CBehaviorGraphInstance::GetCurrentStateInStateMachine( const String& stateMachine ) const
{
	ASSERT( m_internalState == IIS_Waiting );

	CBehaviorGraphStateMachineNode* machine = Cast< CBehaviorGraphStateMachineNode >( FindNodeByName( stateMachine ) );
	if ( machine )
	{
		CBehaviorGraphNode* node = machine->GetCurrentState( *this );
		if ( node )
		{
			return node->GetName();
		}
	}

	return String::EMPTY;
}

//////////////////////////////////////////////////////////////////////////

CBehaviorGraphSimpleLogger::CBehaviorGraphSimpleLogger( const CBehaviorGraphInstance* instance )
	: m_instance ( instance )
{
	m_messageTimeDuration = 5.f;
	m_messages = new tMessageArray[ MC_Last ];
}

CBehaviorGraphSimpleLogger::~CBehaviorGraphSimpleLogger()
{
	Reset();

	delete [] m_messages;
	m_messages = NULL;
}

void CBehaviorGraphSimpleLogger::Reset()
{
	Uint32 size = (Uint32)MC_Last;

	for (Uint32 i=0; i<size; i++)
	{
		m_messages[i].Clear();
	}
}

Float CBehaviorGraphSimpleLogger::GetMsgDuration() const
{
	return m_messageTimeDuration;
}

void CBehaviorGraphSimpleLogger::SetMsgDuration( Float duration )
{
	m_messageTimeDuration = duration;
}

const CBehaviorGraphSimpleLogger::tMessageArray& CBehaviorGraphSimpleLogger::GetMessages( EMessageCategory category ) const
{
	ASSERT( category < MC_Last );
	return m_messages[ category ];
}

void CBehaviorGraphSimpleLogger::Msg( const String& text, Uint32 category, Uint32 color )
{
	ASSERT( category < MC_Last );

	for ( Uint32 i=0; i<m_messages[category].Size(); i++ )
	{
		if ( m_messages[category][i].m_text == text )
		{
			m_messages[category][i].m_time = m_messageTimeDuration;

			m_messages[category].PushBack( m_messages[category][i] );
			m_messages[category].Erase( m_messages[category].Begin() + i );

			return;
		}
	}

	SMessage info;
	info.m_text = text;
	info.m_color = color;
	info.m_time = m_messageTimeDuration;

	m_messages[category].PushBack( info );
}

void CBehaviorGraphSimpleLogger::MsgWithPrefix( const String& prefix, const String& text, Uint32 category, Uint32 color )
{
	ASSERT( category < MC_Last );

	for ( Uint32 i=0; i<m_messages[category].Size(); i++ )
	{
		const String& str = m_messages[category][i].m_text;

		if ( 0 == Red::System::StringCompare( str.AsChar(), prefix.AsChar(), prefix.GetLength() ) )
		{
			m_messages[category][i].m_time = m_messageTimeDuration;
			m_messages[category][i].m_text = prefix + TXT(" ") + text;

			m_messages[category].PushBack( m_messages[category][i] );
			m_messages[category].Erase( m_messages[category].Begin() + i );

			return;
		}
	}

	SMessage info;
	info.m_text = prefix + TXT(" ") + text;
	info.m_color = color;
	info.m_time = m_messageTimeDuration;

	m_messages[category].PushBack( info );
}

void CBehaviorGraphSimpleLogger::OnPreUpdateInstance( Float& dt )
{
	Uint32 size = (Uint32)MC_Last;
	for ( Uint32 i=0; i<size; i++ )
	{
		for ( Int32 j=(Int32)m_messages[i].Size()-1; j>=0; j-- )
		{
			m_messages[i][j].m_time -= dt;

			if ( m_messages[i][j].m_time < 0 )
			{
				m_messages[i].Erase( m_messages[i].Begin() + j );
			}
		}
	}
}

void CBehaviorGraphSimpleLogger::OnPostSampleInstance( const SBehaviorGraphOutput& pose )
{
	for ( Uint32 i=0; i<pose.m_numEventsFired; i++ )
	{
		ASSERT( pose.m_eventsFired[i].m_extEvent != NULL );
		String text = pose.m_eventsFired[i].m_extEvent->GetEventName().AsString();
		Msg( text, MC_Events, COLOR_EVENT_ANIM );
	}
}

void CBehaviorGraphSimpleLogger::OnGenerateEvent( const CName& event )
{
	Msg( event.AsString(), MC_Events, COLOR_EVENT );
}

void CBehaviorGraphSimpleLogger::OnGenerateForceEvent( const CName& event )
{
	Msg( event.AsString(), MC_Events, COLOR_EVENT_FORCE );
}

void CBehaviorGraphSimpleLogger::OnSetVectorValue( CName name, const Vector& value )
{
	String valueStr = name.AsString() + TXT(" ") + ToString( value );
	Msg( valueStr, MC_Variables, COLOR_VAR_VECTOR );
}

void CBehaviorGraphSimpleLogger::OnSetFloatValue( CName name, const Float value )
{
	String valueStr = name.AsString() + TXT(" -> ") + ToString( value );
	Msg( valueStr, MC_Variables, COLOR_VAR_FLOAT );
}

void CBehaviorGraphSimpleLogger::OnResetFloatValue( CName name )
{
	String valueStr = name.AsString() + TXT(" - reset");
	Msg( valueStr, MC_Variables, COLOR_VAR_FLOAT );
}

void CBehaviorGraphSimpleLogger::OnResetVectorValue( CName name )
{
	String valueStr = name.AsString() + TXT(" - reset");
	Msg( valueStr, MC_Variables, COLOR_VAR_VECTOR );
}

void CBehaviorGraphSimpleLogger::OnActivateConstraint( const IAnimationConstraint* constraint )
{
	String str = TXT("Activate constraint: ") + constraint->ToString();
	Msg( str, MC_Instance, COLOR_ON );
}

void CBehaviorGraphSimpleLogger::OnDeactivateConstraint( const IAnimationConstraint* constraint )
{
	String str = TXT("Deactivate constraint: ") + constraint->ToString();
	Msg( str, MC_Instance, COLOR_OFF );
}

void CBehaviorGraphSimpleLogger::OnPlaySlotAnimation( const CBehaviorGraphNode* slot, const CName& animation, const SBehaviorSlotSetup* slotSetup )
{
	String str = String::Printf( TXT("Play slot animation: slot %s, animation %s, blend in %f, blend out %f"),
		slot->GetName().AsChar(), animation.AsString().AsChar(), slotSetup->m_blendIn, slotSetup->m_blendOut );
	Msg( str, MC_Instance, COLOR_ON );
}

void CBehaviorGraphSimpleLogger::OnPlaySlotAnimation( const CBehaviorGraphNode* slot, const CSkeletalAnimationSetEntry* animation, const SBehaviorSlotSetup* slotSetup )
{
	String str = String::Printf( TXT("Play slot animation: slot %s, animation ptr %s, blend in %f, blend out %f"),
		slot->GetName().AsChar(), animation->GetName().AsString().AsChar(), slotSetup->m_blendIn, slotSetup->m_blendOut );
	Msg( str, MC_Instance, COLOR_ON );
}

void CBehaviorGraphSimpleLogger::OnStopSlotAnimation( const CBehaviorGraphNode* slot )
{
	String str = String::Printf( TXT("Stop slot animation: slot %s, animation %s, blend in %f, blend out %f"), slot->GetName().AsChar() );
	Msg( str, MC_Instance, COLOR_OFF );
}

void CBehaviorGraphSimpleLogger::OnSetSlotPose( const CBehaviorGraphNode* slot, Float blend, EBlendType )
{
	String str = String::Printf( TXT("Set slot pose: slot %s, blend in %f "), slot->GetName().AsChar(), blend );
	Msg( str, MC_Instance, COLOR_ON );
}

void CBehaviorGraphSimpleLogger::OnResetSlotPose( const CBehaviorGraphNode* slot )
{
	String str = String::Printf( TXT("Reset slot pose: slot %s"), slot->GetName().AsChar() );
	Msg( str, MC_Instance, COLOR_OFF );
}

void CBehaviorGraphSimpleLogger::OnEventProcessed( const CName& event )
{
	Msg( event.AsString(), MC_Notifications, COLOR_DEFAULT );
}

void CBehaviorGraphSimpleLogger::OnNotifyOfNodesActivation( const CName& name )
{
	Msg( name.AsString(), MC_Notifications, COLOR_ON );
}

void CBehaviorGraphSimpleLogger::OnNotifyOfNodesDeactivation( const CName& name )
{
	Msg( name.AsString(), MC_Notifications, COLOR_OFF );
}

void CBehaviorGraphSimpleLogger::OnNodeUpdate( const CBehaviorGraphNode* node )
{
	if ( node->IsA< CBehaviorGraphStateNode >() )
	{
		String text = node->GetParentNode()->GetName() + TXT(":") + node->GetName();
		Msg( text, MC_States, COLOR_DEFAULT );
	}
 	else if ( node->IsA< CBehaviorGraphEngineValueNode >() )
 	{
 		const CBehaviorGraphEngineValueNode* valueNode = static_cast< const CBehaviorGraphEngineValueNode* >( node );
		CBehaviorGraphInstance* inst = const_cast< CBehaviorGraphInstance* >( m_instance );
 		String text = String::Printf( TXT("%f"), valueNode->GetValue( *inst ) );
 		MsgWithPrefix( valueNode->GetType().AsChar(), text, MC_Variables, COLOR_DEFAULT );
 	}
}

/*
void CBehaviorGraphHistoryLogger::OnPaused( Bool paused )
{
	SSnapshot& s = GetSnapshot();
	if ( paused )
	{
		s.m_messages.PushBack( TXT("Paused") );
	}
	else
	{
		s.m_messages.PushBack( TXT("Unpaused") );
	}
}

void CBehaviorGraphHistoryLogger::OnReset()
{
	SSnapshot& s = GetSnapshot();
	s.m_messages.PushBack( TXT("Reset") );
}

void CBehaviorGraphHistoryLogger::OnUnbind()
{
	SSnapshot& s = GetSnapshot();
	s.m_messages.PushBack( TXT("Unbind") );
}

void CBehaviorGraphHistoryLogger::OnActivated()
{
	SSnapshot& s = GetSnapshot();
	s.m_messages.PushBack( TXT("Activated") );
}

void CBehaviorGraphHistoryLogger::OnDeactivated()
{
	SSnapshot& s = GetSnapshot();
	s.m_messages.PushBack( TXT("Deactivated") );
}

void CBehaviorGraphHistoryLogger::OnPreUpdateInstance()
{
	SSnapshot& s = GetSnapshot();
	s.m_messages.PushBack( TXT("PreUpdate") );
}

void CBehaviorGraphHistoryLogger::OnPostUpdateInstance()
{
	SSnapshot& s = GetSnapshot();
	s.m_messages.PushBack( TXT("PostUpdate") );
}

void CBehaviorGraphHistoryLogger::OnPreSampleInstance()
{
	SSnapshot& s = GetSnapshot();
	s.m_messages.PushBack( TXT("PreSample") );
}

void CBehaviorGraphHistoryLogger::OnPostSampleInstance()
{
	SSnapshot& s = GetSnapshot();
	s.m_messages.PushBack( TXT("PostSample") );
}

void CBehaviorGraphHistoryLogger::OnGenerateEvent( const CName& event )
{
	if ( m_eventListEnabled )
	{
		SSnapshot& s = GetSnapshot();
		s.m_messages.PushBack( String::Printf( TXT("Genarate event: %s"), event.AsChar() ) );
		s.m_eventsList.PushBack( s.m_messages.Size() - 1 );
	}
}

void CBehaviorGraphHistoryLogger::OnGenerateForceEvent( const CName& event )
{
	if ( m_eventListEnabled )
	{
		SSnapshot& s = GetSnapshot();
		s.m_messages.PushBack( String::Printf( TXT("Genarate force event: %s"), event.AsChar() ) );
		s.m_eventsList.PushBack( s.m_messages.Size() - 1 );
	}
}

void CBehaviorGraphHistoryLogger::OnSetVectorValue( const String& name, Int32 id, const Vector& value )
{
	if ( m_variablesListEnabled )
	{
		SSnapshot& s = GetSnapshot();
		String var = ToString( value );
		s.m_messages.PushBack( String::Printf( TXT("Set vector value, name %s, id %d -> %s"), name.AsChar(), id, var.AsChar() ) );
	}
}

void CBehaviorGraphHistoryLogger::OnSetVectorValue( Int32 id, const Vector& value )
{
	SSnapshot& s = GetSnapshot();
	String var = ToString( value );
	s.m_messages.PushBack( String::Printf( TXT("Set vector value, id %d -> %s"), id, var.AsChar() ) );
}

void CBehaviorGraphHistoryLogger::OnSetFloatValue( const String& name, Int32 id, const Float value )
{
	SSnapshot& s = GetSnapshot();
	String var = ToString( value );
	s.m_messages.PushBack( String::Printf( TXT("Set float value, name %s, id %d -> %s"), name.AsChar(), id, var.AsChar() ) );
}

void CBehaviorGraphHistoryLogger::OnSetFloatValue( Int32 id, const Float value )
{
	SSnapshot& s = GetSnapshot();
	String var = ToString( value );
	s.m_messages.PushBack( String::Printf( TXT("Set float value, id %d -> %s"), id, var.AsChar() ) );
}

void CBehaviorGraphHistoryLogger::OnResetFloatValue( Int32 id )
{
	SSnapshot& s = GetSnapshot();
	s.m_messages.PushBack( String::Printf( TXT("Rerset float value, id %d -> %s"), id ) );
}

void CBehaviorGraphHistoryLogger::OnResetVectorValue( Int32 id )
{
	SSnapshot& s = GetSnapshot();
	s.m_messages.PushBack( String::Printf( TXT("Rerset vector value, id %d -> %s"), id ) );
}

void CBehaviorGraphHistoryLogger::OnActivateConstraint( const IAnimationConstraint* constraint )
{
	SSnapshot& s = GetSnapshot();
	String str = TXT("Activate constraint: ") + constraint->ToString();
	s.m_messages.PushBack( str );
}

void CBehaviorGraphHistoryLogger::OnDeactivateConstraint( const IAnimationConstraint* constraint )
{
	SSnapshot& s = GetSnapshot();
	String str = TXT("Deactivate constraint: ") + constraint->ToString();
	s.m_messages.PushBack( str );
}

void CBehaviorGraphHistoryLogger::OnPlaySlotAnimation( const CBehaviorGraphNode* slot, const String& animation, Float blendIn, Float blendOut )
{
	SSnapshot& s = GetSnapshot();
	s.m_messages.PushBack( String::Printf( TXT("Play slot animation: slot %s, animation %s, blend in %f, blend out %f"),
		slot->GetName().AsChar(), animation.AsChar(), blendIn, blendOut ) );
}

void CBehaviorGraphHistoryLogger::OnPlaySlotAnimation( const CBehaviorGraphNode* slot, const CSkeletalAnimationSetEntry* animation, Float blendIn, Float blendOut )
{
	SSnapshot& s = GetSnapshot();
	s.m_messages.PushBack( String::Printf( TXT("Play slot animation: slot %s, animation ptr %s, blend in %f, blend out %f"),
		slot->GetName().AsChar(), animation->GetName().AsChar(), blendIn, blendOut ) );
}

void CBehaviorGraphHistoryLogger::OnStopSlotAnimation( const CBehaviorGraphNode* slot )
{
	SSnapshot& s = GetSnapshot();
	s.m_messages.PushBack( String::Printf( TXT("Stop slot animation: slot %s, animation %s, blend in %f, blend out %f"), slot->GetName().AsChar() ) );
}

void CBehaviorGraphHistoryLogger::OnSetSlotPose( const CBehaviorGraphNode* slot, Float blend )
{
	SSnapshot& s = GetSnapshot();
	s.m_messages.PushBack( String::Printf( TXT("Set slot pose: slot %s, blend in %f "), slot->GetName().AsChar(), blend ) );
}

void CBehaviorGraphHistoryLogger::OnResetSlotPose( const CBehaviorGraphNode* slot )
{
	SSnapshot& s = GetSnapshot();
	s.m_messages.PushBack( String::Printf( TXT("Reset slot pose: slot %s"), slot->GetName().AsChar() ) );
}

void CBehaviorGraphHistoryLogger::OnEventProcessed( const CName& event )
{
	SSnapshot& s = GetSnapshot();
	s.m_messages.PushBack( String::Printf( TXT("Event processed: %s"), event.AsChar() ) );
}

void CBehaviorGraphHistoryLogger::OnNotifyOfNodesActivation( const CName& name )
{
	SSnapshot& s = GetSnapshot();
	s.m_messages.PushBack( String::Printf( TXT("Node activation notification: %s"), name.AsChar() ) );
}

void CBehaviorGraphHistoryLogger::OnNotifyOfNodesDeactivation( const CName& name )
{
	SSnapshot& s = GetSnapshot();
	s.m_messages.PushBack( String::Printf( TXT("Node deactivation notification: %s"), name.AsChar() ) );
}

void CBehaviorGraphHistoryLogger::OnNodeUpdate( const CBehaviorGraphNode* node )
{
	//SSnapshot& s = GetSnapshot();
	//s.m_messages.PushBack( String::Printf( TXT("Node update: '%ls', %s"), node->GetName().AsChar(), node->GetClass()->GetName().AsChar() ) );
}

void CBehaviorGraphHistoryLogger::OnNodeSample( const CBehaviorGraphNode* node )
{
	//SSnapshot& s = GetSnapshot();
	//s.m_messages.PushBack( String::Printf( TXT("Node sample: '%ls', %s"), node->GetName().AsChar(), node->GetClass()->GetName().AsChar() ) );
}

void CBehaviorGraphHistoryLogger::OnNodeActivated( const CBehaviorGraphNode* node )
{
	//SSnapshot& s = GetSnapshot();
	//s.m_messages.PushBack( String::Printf( TXT("Node activation: '%ls', %s"), node->GetName().AsChar(), node->GetClass()->GetName().AsChar() ) );
}

void CBehaviorGraphHistoryLogger::OnNodeDeactivated( const CBehaviorGraphNode* node )
{
	//SSnapshot& s = GetSnapshot();
	//s.m_messages.PushBack( String::Printf( TXT("Node deactivation: '%ls', %s"), node->GetName().AsChar(), node->GetClass()->GetName().AsChar() ) );
}
*/

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
