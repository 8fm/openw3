/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behTreeMachine.h"

#include "aiParametersSpawnList.h"
#include "behTree.h"
#include "behTreeLog.h"
#include "behTreeNode.h"
#include "behTreeInstance.h"
#include "behTreeMachineListener.h"
#include "aiLog.h"
#include "aiParameters.h"
#include "../core/feedback.h"


IMPLEMENT_ENGINE_CLASS( CBehTreeMachine );

void CBehTreeMachine::OnFinalize()
{
	Uninitialize();

	// Pass to base class
	TBaseClass::OnFinalize();
}

void CBehTreeMachine::FreeInstance()
{
	// ctremblay ++ BEGIN HACK. moving destruction to a task. It's too slow for Main thread
	class CBehTreeInstanceDestructionTask : public CTask
	{
	public:
		CBehTreeInstanceDestructionTask( CBehTreeInstance* instance )
			: m_instance( instance )
		{}

		void Run()
		{
			m_instance->DestroyRootAndListeners();
			m_instance->Discard();
		}

#ifndef NO_DEBUG_PAGES
		virtual const Char* GetDebugName() const override final { return TXT("CBehTreeInstanceDestructionTask"); }
		virtual Uint32 GetDebugColor() const override final { return COLOR_UINT32( 100, 0, 0 ); }
#endif

	private:

		CBehTreeInstance * m_instance;
	};

	if( m_instance )
	{
		IBehTreeNodeInstance* node = m_instance->GetInstanceRootNode();
		if ( node && node->IsActive() )
		{
			node->Deactivate();
		}
		
		// notify a tree about incoming asynchronous destruction
		m_instance->OnGameplayEvent( CNAME( AI_PreAsynchronousDestruction ), nullptr, nullptr );

		// synchronous destruction part
		m_instance->PrepareDestruction();
		m_instance->SetParent( nullptr );
		
		// asynchronous destruction call
		CBehTreeInstanceDestructionTask* task = new ( CTask::Root ) CBehTreeInstanceDestructionTask( m_instance );
		GTaskManager->Issue( *task, TSP_Low );
		task->Release();

		m_instance = NULL;
	}
	// ctremblay -- END HACK
}

void CBehTreeMachine::SetupContext( CBehTreeSpawnContext& context, const SAIParametersSpawnList& spawnParameters )
{
	IAIParameters* spawnSystemParameters = spawnParameters.m_spawnSystemParameters;
	if ( spawnSystemParameters )
	{
		context.Push( spawnSystemParameters );
	}

	for ( Int32 i = 0, n = m_aiParameters.Size(); i != n; ++i )
	{
		IAIParameters* parameter = m_aiParameters[ i ].Get();
		if ( parameter )
		{
			context.Push( parameter );
		}
	}

	const auto& list = spawnParameters.m_list;
	for ( Uint32 i = 0, n = list.Size(); i != n; ++i )
	{
		IAIParameters* parameter = list[ i ].Get();
		if ( parameter )
		{
			context.Push( parameter );
		}
	}
}

Bool CBehTreeMachine::Initialize( const SAIParametersSpawnList& list )
{
	CActor* actor = FindParent< CActor >();
	ASSERT( actor );

	if( !m_aiRes )
	{
		BT_ERROR_RAW( TXT("CBehTreeMachine:: Null tree resource: %s!!!"), actor ? actor->GetName().AsChar() : TXT("NULL") );
		return false;
	}

	const IBehTreeNodeDefinition* tree = m_aiRes->GetRootNode();

	if( !tree || !actor )
	{
		BT_ERROR_RAW( TXT("CBehTreeMachine::Initialize null tree, actor: %s!!!"), actor ? actor->GetName().AsChar() : TXT("NULL") );
		if( actor )
		{
			SET_ERROR_STATE( actor,  TXT("CBehTreeMachine::Initialize null tree") );
		}
		return false;
	}

	FreeInstance();

	CClass* behTreeInstanceClass = GCommonGame->CBehTreeInstanceClass();
	// Create the instance object
	{
		PC_SCOPE_PIX( CBehTreeInstance_CreateObject );
		m_instance = CreateObject< CBehTreeInstance >( behTreeInstanceClass, this );
	}
	m_stopped = false;
	m_initializedOnSpawn = false;

	{
		PC_SCOPE_PIX( CBehTreeInstance_Bind );
		// Bind instance to current graph and data buffer
		m_instance->SetOwner( actor );
		m_instance->SetMachine( this );

		CBehTreeSpawnContext context( false );
		m_spawnParameters = list;

		SetupContext( context, list );

		m_instance->Bind( tree, context );

		
	}
	
#ifdef EDITOR_AI_DEBUG
	ClearAllBreakpoints( );
#endif

	return true;
}

void CBehTreeMachine::OnSpawn()
{
	if ( !m_initializedOnSpawn && m_instance )
	{
		PC_SCOPE_PIX( BehaviorTree_OnSpawn );
		m_initializedOnSpawn = true;

		CBehTreeSpawnContext context( false );

		SetupContext( context, m_spawnParameters );

		m_instance->OnSpawn( context );

		m_spawnParameters.Clear();
	}
}

void CBehTreeMachine::Uninitialize()
{
	FreeInstance();
}

void CBehTreeMachine::OnAttached()
{
	GCommonGame->RegisterBehTreeMachine( this );
}

void CBehTreeMachine::OnReattachedAsync( const SAIParametersSpawnList& spawnParameters )
{
	if ( m_instance )
	{
		CBehTreeSpawnContext context( false );

		SetupContext( context, spawnParameters );

		m_instance->OnReattachAsync( context );
	}
}

void CBehTreeMachine::OnReattached( const SAIParametersSpawnList& spawnParameters )
{
	if ( m_instance )
	{
		CBehTreeSpawnContext context( false );

		SetupContext( context, spawnParameters );

		m_instance->OnReattach( context );
	}
}

void CBehTreeMachine::OnDetached()
{
	if ( m_instance )
	{
		m_instance->OnDetached();
	}
	Stop( true );

#ifdef EDITOR_AI_DEBUG
	DebugStopListening();
#endif

	GCommonGame->UnregisterBehTreeMachine( this );
}

void CBehTreeMachine::Restart()
{
	if ( m_instance )
	{
		m_stopped = false;
		m_instance->Activate();
	}
}

void CBehTreeMachine::Stop( Bool silent )
{
	if( m_stopped )
		return;

	if( m_instance )
	{
		m_instance->Deactivate();
	}

	m_stopped = true;

#ifdef EDITOR_AI_DEBUG
	DebugTreeStateChanged();
#endif
}

void CBehTreeMachine::Tick( Float timeDelta )
{
	PC_SCOPE( BehTreeTick );

	if ( !m_stopped )
	{
		RED_ASSERT( m_instance != NULL, TXT("Behavior graph instance deleted!" ) );

		if ( m_instance != NULL )
		{
			m_instance->Update( timeDelta );
		}
	}
}

void CBehTreeMachine::ForceUpdate()
{
	Tick( 0.f );
}

#ifdef EDITOR_AI_DEBUG
	#define BT_DEBUG_UPDATE_NODE( instance, node ) DebugUpdateNode( instance, node );
#else
	#define BT_DEBUG_UPDATE_NODE( instance, node )
#endif

#ifdef EDITOR_AI_DEBUG
Bool CBehTreeMachine::ProcessBreakpoint( const IBehTreeNodeDefinition* node )
{
	if( m_instance )
	{
		TBreakpoints::iterator iter = Find( m_breakpoints.Begin(), m_breakpoints.End(), BreakpointInfo( node ) );
		if( iter != m_breakpoints.End() && iter->m_status != BS_Breaked )
		{
			if( iter->m_status != BS_Skip )
			{
				iter->m_status = BS_Breaked;
				BT_LOG( *m_instance, node, TXT("BREAKPOINT") );

				EFeedbackYesNoCancelResult result = GFeedback->AskYesNoCancel( TXT( "Breakpoint in a behaviour tree was hit and the game was paused.\nYes - start debugging (code)\nNo - continue execution\nCancel - continue execution and skip future hits of this breakpoint" ) );
				switch( result )
				{
				case FeedbackYes:
					RED_BREAKPOINT();
					break;
				case FeedbackNo:
					iter->m_status = BS_Normal;
					break;
				case FeedbackCancel:
					iter->m_status = BS_Skip;
					break;
				default:
					break;
				}
			}

			return true;
		}
	}

	return false;
}

void CBehTreeMachine::DebugUpdateNode( CBehTreeInstance& instance, const IBehTreeNodeInstance* node )
{
	if( node )
	{
		for( Uint32 i = 0; i < m_listeners.Size(); ++i )
		{
			m_listeners[i]->OnNodeResultChanged( node, node->IsActive() );
		}
	}
}

void CBehTreeMachine::DebugTreeStateChanged()
{
	for( Uint32 i = 0; i < m_listeners.Size(); ++i )
	{
		m_listeners[i]->OnTreeStateChanged();
	}
}

void CBehTreeMachine::DebugStopListening()
{
	for( Uint32 i = 0; i < m_listeners.Size(); ++i )
	{
		m_listeners[i]->OnStopListeningRequest();
		ClearAllBreakpoints( );
	}
}

#endif	//EDITOR_AI_DEBUG

//! Set debug listener
void CBehTreeMachine::AddListener( IBehTreeMachineListener* listener )
{
	RED_UNUSED( listener );
#ifdef EDITOR_AI_DEBUG
	if( listener )
	{
		Bool valid = true;
		for( Uint32 i = 0; i < m_listeners.Size(); ++i )
		{
			if( m_listeners[i] == listener )
			{
				valid = false;
				break;
			}
		}

		if( valid )
		{
			m_listeners.PushBack( listener );
			if( m_instance )
			{
				m_instance->SetIsDebugged( true );
			}
		}
	}
#endif
}

//! Remove debug listener
void CBehTreeMachine::RemoveListener( IBehTreeMachineListener* listener )
{
	RED_UNUSED( listener );
#ifdef EDITOR_AI_DEBUG
	if( listener )
	{
		for( Uint32 i = 0; i < m_listeners.Size(); ++i )
		{
			if( m_listeners[i] == listener )
			{
				if( m_listeners.Size() == 1 )
				{
					m_instance->SetIsDebugged( false );
				}

				m_listeners.RemoveAt( i );
				break;
			}
		}
	}
#endif
}

#ifdef EDITOR_AI_DEBUG

Bool CBehTreeMachine::IsBreakpointSet( const IBehTreeNodeDefinition* node, EBreakpointStatus& status )
{
	TBreakpoints::iterator iter = Find( m_breakpoints.Begin(), m_breakpoints.End(), BreakpointInfo( node ) );
	if( iter != m_breakpoints.End() )
	{
		status = iter->m_status;
		return true;
	}
	return false;
}

void CBehTreeMachine::SetBreakpoint( const IBehTreeNodeDefinition* node )
{
	m_breakpoints.PushBackUnique( BreakpointInfo( node ) );
}

void CBehTreeMachine::ClearBreakpoint( const IBehTreeNodeDefinition* node )
{
	m_breakpoints.Remove( BreakpointInfo( node ) );
}

void CBehTreeMachine::SkipBreakpoint( const IBehTreeNodeDefinition* node )
{
	TBreakpoints::iterator iter = Find( m_breakpoints.Begin(), m_breakpoints.End(), BreakpointInfo( node ) );
	if( iter != m_breakpoints.End() )
	{
		iter->m_status = BS_Skip;
	}
}

void CBehTreeMachine::ClearAllBreakpoints()
{
	m_breakpoints.Clear();
}

void CBehTreeMachine::DescribeAIState( String& ai )
{
	static const String SEPARATOR( TXT("->") );
	static const String LEFT_PAR( TXT("(") );
	static const String RIGHT_PAR( TXT(")") );
	static const String PARALLEL( TXT(" | ") );

	struct Local
	{
		static void DescribeNode( IBehTreeNodeInstance* node, String& ai )
		{
			ai += node->GetDebugName().AsString();

			Uint32 childCount = node->GetActiveChildCount();
			if ( childCount == 1 )
			{
				ai += SEPARATOR;
				DescribeNode( node->GetActiveChild( 0 ), ai );
			}
			else if ( childCount != 0 )
			{
				Bool b = false;
				for ( Uint32 i = 0; i < childCount; ++i )
				{
					if ( b )
					{
						ai += PARALLEL;
					}
					b = true;
					ai += LEFT_PAR;
					IBehTreeNodeInstance* childNode = node->GetActiveChild( i );
					DescribeNode( childNode, ai );
					ai += RIGHT_PAR;
				}
			}
		}
	};

	IBehTreeNodeInstance* node = m_instance->GetInstanceRootNode();
	if ( !node )
	{
		ai += TXT("UNITIALIZED");
	}
	else if ( !node->IsActive() )
	{
		ai += TXT("Unactive");
	}
	else
	{
		Local::DescribeNode( node, ai );
	}
}

#endif	//EDITOR_AI_DEBUG

String CBehTreeMachine::GetInfo() const
{
	String treeName = (m_instance && m_instance->GetDefinitionRootNode())? m_instance->GetDefinitionRootNode()->GetFriendlyName() : TXT("NULL");
	return String::Printf( TXT("BehTree: %s, stopped %s"), treeName.AsChar(), ToString( IsStopped() ).AsChar() );
}


void CBehTreeMachine::ProcessAnimationEvent( const CExtAnimEvent* event, EAnimationEventType eventType, const SAnimationEventAnimInfo& animInfo )
{
	if( !m_stopped )
	{
		m_instance->ProcessAnimationEvent( event, eventType, animInfo );
	}
}

void CBehTreeMachine::OnGameplayEvent( CName name, void* additionalData, IRTTIType* additionalDataType )
{
	ASSERT( !name.Empty() );
	if( !m_stopped )
	{
		m_instance->OnGameplayEvent( name, additionalData, additionalDataType );
	}
}

void CBehTreeMachine::SetAIDefinition( CAIBaseTree* baseTree )
{
	m_aiRes = baseTree->GetTree();
	m_aiParameters.PushBack( baseTree );
}


//! Interface to 'feed ai' with external parameters
void CBehTreeMachine::InjectAIParameters( IAIParameters* parameters )
{
	m_aiParameters.PushBack( parameters );
}
