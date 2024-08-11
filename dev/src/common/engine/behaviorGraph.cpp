/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphContainerNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphNodeIterator.h"
#include "behaviorGraphPoseSlotNode.h"
#include "behaviorGraphStateMachine.h"
#include "behaviorGraphTopLevelNode.h"
#include "behaviorGraphAnimationSlotNode.h"
#include "../engine/animationManager.h"
#include "animatedComponent.h"

#include "../core/instanceDataLayoutCompiler.h"
#include "../core/queue.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CBehaviorGraph );
IMPLEMENT_ENGINE_CLASS( SBehaviorGraphInstanceSlot );

RED_DEFINE_STATIC_NAME( MovementAdjustmentLocation );
RED_DEFINE_STATIC_NAME( FinalStepDistance );
RED_DEFINE_STATIC_NAME( FinalStep );
RED_DEFINE_STATIC_NAME( MovementAdjustmentActive );

CBehaviorGraph::CBehaviorGraph()
	: m_rootNode( NULL )
	, m_generateEditorFragments( false )
	, m_defaultStateMachine( NULL )
	, m_sourceDataRemoved( false )
{
	m_variables.SetBehaviorGraph( this );
	m_vectorVariables.SetBehaviorGraph( this );
	m_events.SetBehaviorGraph( this );

	m_internalVariables.SetBehaviorGraph( this );
	m_internalVectorVariables.SetBehaviorGraph( this );

	CreateTrackNames();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//-------------- TO REFACTOR ----------------
	// create top level node - one for whole graph
	m_rootNode = CreateObject< CBehaviorGraphTopLevelNode >( this );
	m_rootNode->OnSpawned( GraphBlockSpawnInfo( CBehaviorGraphTopLevelNode::GetStaticClass() ) );
	m_rootNode->OnRebuildSockets();
	//-------------------------------------------

#endif
}

CBehaviorGraph::~CBehaviorGraph()
{
}

void CBehaviorGraph::CompileDataLayout()
{
	InstanceDataLayoutCompiler compiler( m_dataLayout );

	// Float variables
	compiler << i_rtFloatVariables;

	// Vector variables
	compiler << i_rtVectorVariables;

	// Internal float variables
	compiler << i_rtInternalFloatVariables;

	// Internal vector variables
	compiler << i_rtInternalVectorVariables;

	// Nodes
	m_rootNode->OnBuildDataLayout( compiler );

	// Create layout
	m_dataLayout.ChangeLayout( compiler );
}

void CBehaviorGraph::Reload()
{
	CacheData();

	GenerateBlocksId();

	CompileDataLayout();
}

CBehaviorGraphInstance*	CBehaviorGraph::CreateInstance( CAnimatedComponent* component, const CName& name ) const
{
	PC_SCOPE_PIX( CBehaviorGraph_CreateInstance );
#ifdef BEH_DEBUG
	CTimeCounter behTimeCounter;
#endif

	if ( !component || !component->GetSkeleton() )
	{
		return NULL;
	}

	// Create the instance object
	CBehaviorGraphInstance* instance = CreateObject< CBehaviorGraphInstance >( component );
	if ( !instance )
	{
		BEH_ERROR( TXT("Unable to create instance of behavior '%ls': no object created"), GetFriendlyName().AsChar() );		
		return NULL;
	}

	instance->Bind( name, this, m_dataLayout, component );


#ifdef BEH_DEBUG
	{
		const Float timeUsed = behTimeCounter.GetTimePeriod();
		BEH_LOG( TXT("Instancing behavior graph '%ls' took %1.2fms"), GetDepotPath().AsChar(), timeUsed * 1000.0f );
		Uint32 dataSize = instance->GetInstanceBuffer().GetSize();
		BEH_LOG( TXT("Size of behavior graph instance '%ls' : %1.2fKB"), GetDepotPath().AsChar(), dataSize / 1024.0f );
	}
#endif

	// Return created instance
	return instance;
}

void CBehaviorGraph::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );
	
	file << m_rootNode;	
	file << m_variables;
	file << m_events;
	file << m_vectorVariables;
	// note: as CBehaviorVariableList should not change to derive from CObject, it detects with engine version if it should or nor load internal variables
	if (file.m_version >= VER_BEHAVIOR_GRAPH_INTERNAL_VARIABLES_ADDED ||
		file.IsWriter())
	{
		file << m_internalVariables;
		file << m_internalVectorVariables;
	}
}

#ifndef NO_RESOURCE_COOKING
void CBehaviorGraph::OnCook( class ICookerFramework& cooker )
{
	TBaseClass::OnCook( cooker );
	CacheConnections();
}
#endif

void CBehaviorGraph::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	m_variables.OnPostLoad();
	m_vectorVariables.OnPostLoad();
	m_internalVariables.OnPostLoad();
	m_internalVectorVariables.OnPostLoad();

	CheckDefaultStateMachine();

	CreateTrackNames();

	CacheData();

	GenerateBlocksId();

	CompileDataLayout();
}


void CBehaviorGraph::SetAsCloneOf( const CBehaviorGraph* otherGraph )
{
	// Root
	m_rootNode->Discard();
	m_rootNode = NULL;

	m_rootNode = SafeCast< CBehaviorGraphContainerNode >( otherGraph->m_rootNode->Clone( this ) );
	
	// Default state machine
	m_defaultStateMachine = otherGraph->m_defaultStateMachine ? 
		SafeCast< CBehaviorGraphStateMachineNode >( FindNodeById( otherGraph->m_defaultStateMachine->GetId() ) ) : NULL;

	// Custom track names
	m_customTrackNames = otherGraph->m_customTrackNames;

	// Generate editor fragments
	m_generateEditorFragments = otherGraph->m_generateEditorFragments;

	// Variables and events
	m_variables = otherGraph->m_variables;
	m_vectorVariables = otherGraph->m_vectorVariables;
	m_events = otherGraph->m_events;
	m_internalVariables = otherGraph->m_internalVariables;
	m_internalVectorVariables = otherGraph->m_internalVectorVariables;
}

Uint32 CBehaviorGraph::GenerateBlocksId()
{
	TQueue< CBehaviorGraphNode* > containerNodes;

	containerNodes.Push( m_rootNode );

	Uint32 id = 0;

	while( !containerNodes.Empty() )
	{
		CBehaviorGraphNode* currNode = containerNodes.Front();

		currNode->SetId( ++id );

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

	return id;
}

Bool CBehaviorGraph::IsSourceDataRemoved() const
{
	return m_sourceDataRemoved;
}

Uint32 CBehaviorGraph::GetSize() const
{
	return static_cast< Uint32 >( sizeof( CBehaviorGraph ) + m_dataLayout.GetSize() + 
		m_poseSlots.DataSize() + m_animSlots.DataSize() + 
		m_variables.GetSize() + m_vectorVariables.GetSize() + m_events.GetSize() +
		m_internalVariables.GetSize() + m_internalVectorVariables.GetSize() +
		m_stateMachines.DataSize() );
}

Uint32 CBehaviorGraph::GetAllNodes( TDynArray< CBehaviorGraphNode* >& nodes ) const
{
	class CFinder
	{
	public:
		CFinder() {}

		void Find( CBehaviorGraphNode* node, TDynArray< CBehaviorGraphNode* >& nodes ) 
		{
			ASSERT( node );

			if ( node->IsA< CBehaviorGraphContainerNode >() )
			{
				CBehaviorGraphContainerNode *containerNode = SafeCast< CBehaviorGraphContainerNode >( node );

				TDynArray< CGraphBlock* >& children = containerNode->GetConnectedChildren();

				for( Uint32 i=0; i<children.Size(); ++i )
				{
					if ( CBehaviorGraphNode* bgChild = Cast< CBehaviorGraphNode >( children[i] ) )
					{
						Find( bgChild, nodes );	
					}
				}
			}

			nodes.PushBack( node );
		}
	};

	CFinder finder;
	finder.Find( m_rootNode, nodes );

	return nodes.Size();
}

void CBehaviorGraph::CreateTrackNames()
{
	// Fill custom track names
	if (m_customTrackNames.Size() < SBehaviorGraphOutput::NUM_CUSTOM_FLOAT_TRACKS)
	{
		for (Uint32 i=m_customTrackNames.Size(); i<SBehaviorGraphOutput::NUM_CUSTOM_FLOAT_TRACKS; i++)
		{
			CName name = CName( String::Printf( TXT("CustomTrack%d"), i ) );
			m_customTrackNames.PushBack(name);
		}
	}

	ASSERT(m_customTrackNames.Size() >= SBehaviorGraphOutput::NUM_CUSTOM_FLOAT_TRACKS);
}

String CBehaviorGraph::GetCustomTrackNameStr( Int32 index ) const
{
	ASSERT((Int32)m_customTrackNames.Size() > index);

	return m_customTrackNames[index].AsString();
}

String CBehaviorGraph::GetFloatTrackNameStr( Int32 index ) const
{
	return GetFloatTrackName(index).AsString();
}

const CName& CBehaviorGraph::GetCustomTrackName( Int32 index ) const
{
	ASSERT((Int32)m_customTrackNames.Size() > index);

	return m_customTrackNames[index];
}

const CName& CBehaviorGraph::GetFloatTrackName( Int32 index ) const
{
	if (index == SBehaviorGraphOutput::FTT_FOV)
	{
		return CNAME( FOV );
	}
	else if (index == SBehaviorGraphOutput::FTT_DOF_Override )
	{
		return CNAME( DOF_Override );
	}
	else if (index == SBehaviorGraphOutput::FTT_DOF_FocusDistFar )
	{
		return CNAME( DOF_FocusDistFar );
	}
	else if (index == SBehaviorGraphOutput::FTT_DOF_BlurDistFar )
	{
		return CNAME( DOF_BlurDistFar );
	}
	else if (index == SBehaviorGraphOutput::FTT_DOF_Intensity )
	{
		return CNAME( DOF_Intensity );
	}
	else if (index == SBehaviorGraphOutput::FTT_DOF_FocusDistNear )
	{
		return CNAME( DOF_FocusDistNear );
	}
	else if (index == SBehaviorGraphOutput::FTT_DOF_BlurDistNear )
	{
		return CNAME( DOF_BlurDistNear );
	}
	else
	{
		return CName::NONE;
	}
}

void CBehaviorGraph::SetCustomFloatTrackName(String name, Uint32 index)
{
	if (index < m_customTrackNames.Size())
	{
		CName newName = CName( name );
		m_customTrackNames[index] = newName;
	}
}

void CBehaviorGraph::CacheConnections()
{
	PC_SCOPE( CacheConnections );

	// Cannot cache connections if source graph data was removed
	if ( m_sourceDataRemoved )
	{
		return;
	}

	// Cache connections on top level block
	if ( m_rootNode )
	{
		m_rootNode->CacheConnections();
	}
}

void CBehaviorGraph::CacheSlots()
{
	m_poseSlots.Clear();
	m_animSlots.Clear();

	for ( BehaviorGraphNodeIterator it( this ); it; ++it )
	{
		CBehaviorGraphNode* node = (*it);
		if ( node )
		{
			CBehaviorGraphAnimationBaseSlotNode* animSlot = Cast< CBehaviorGraphAnimationBaseSlotNode >( node );
			if ( animSlot )
			{
				m_animSlots.PushBack( animSlot );
			}
			else
			{
				CBehaviorGraphPoseSlotNode* poseSlot = Cast< CBehaviorGraphPoseSlotNode >( node );
				if ( poseSlot )
				{
					m_poseSlots.PushBack( poseSlot );
				}
			}
		}
	}
}

void CBehaviorGraph::CacheStateMachines()
{
	Int32 increaseBy = 10;
	m_stateMachines.Clear();
	m_stateMachines.Reserve( increaseBy );

	for ( BehaviorGraphNodeIterator it( this ); it; ++it )
	{
		CBehaviorGraphNode* node = (*it);
		if ( node )
		{
			CBehaviorGraphStateMachineNode* stateMachine = Cast< CBehaviorGraphStateMachineNode >( node );
			if ( stateMachine )
			{
				if ( m_stateMachines.Size() == m_stateMachines.Capacity() )
				{
					m_stateMachines.Reserve( m_stateMachines.Capacity() + increaseBy );
				}
				m_stateMachines.PushBack( stateMachine );
			}
		}
	}

	// now reverse it as it comes in order inside container first, then container and we want exactly reverse situation
	Int32 halfSize = m_stateMachines.SizeInt() >> 1; // / 2
	TDynArray< CBehaviorGraphStateMachineNode* >::iterator smFwd = m_stateMachines.Begin();
	TDynArray< CBehaviorGraphStateMachineNode* >::iterator smBwd = m_stateMachines.End();
	-- smBwd;
	for ( Int32 idx = 0; idx < halfSize; ++ idx, ++ smFwd, -- smBwd )
	{
		CBehaviorGraphStateMachineNode* temp = *smFwd;
		*smFwd = *smBwd;
		*smBwd = temp;
	}
}

void CBehaviorGraph::CacheNodesToRelease()
{
	m_nodesToReleaseList.Clear();
	m_rootNode->CollectNodesToRelease( m_nodesToReleaseList );
}

void CBehaviorGraph::CacheData()
{
	// Cache graph connections inside blocks
	CacheConnections();

	// Cache slots
	CacheSlots();

	// Cache state machines
	CacheStateMachines();

	// Cache nodes to release
	CacheNodesToRelease();  // do not use property for this before 1.1 patch (witcher3)
}

void CBehaviorGraph::CleanupSourceData()
{
	TBaseClass::CleanupSourceData();

	// Do not remove data twice
	if ( !m_sourceDataRemoved )
	{
		// Cache data
		CacheData();

		// Generate block ids
		GenerateBlocksId();

		// Remove all graph sockets and graph connections ( they are cached now )
		if ( m_rootNode )
		{
			m_rootNode->RemoveConnections();
		}

		// Mark behavior graph as cached
		m_sourceDataRemoved = true;
	}
}

void CBehaviorGraph::CheckDefaultStateMachine()
{
	if ( m_defaultStateMachine )
	{
		class CNodeFinder
		{
		public:
			CNodeFinder() {}

			Bool Find( CBehaviorGraphNode* node, CBehaviorGraphNode* targetNode ) 
			{
				if ( node && node == targetNode )
				{
					return true;
				}

				if ( node && node->IsA< CBehaviorGraphContainerNode >() )
				{
					CBehaviorGraphContainerNode *containerNode = SafeCast< CBehaviorGraphContainerNode >( node );

					TDynArray< CGraphBlock* >& children = containerNode->GetConnectedChildren();

					for( Uint32 i=0; i<children.Size(); ++i )
					{
						if ( CBehaviorGraphNode* bgChild = Cast< CBehaviorGraphNode >( children[i] ) )
						{
							if ( Find( bgChild, targetNode ) )
							{
								return true;
							}
						}
					}
				}

				return false;
			}
		};

		CNodeFinder finder;
		if ( !finder.Find( m_rootNode, m_defaultStateMachine ) )
		{
			BEH_WARN( TXT("Graph '%ls' hasn't got default state machine."), GetFriendlyName().AsChar() );

			if ( !GIsEditor )
			{
				BEH_WARN( TXT("See log. Graph hasn't got default state machine.") );
			}

			m_defaultStateMachine = NULL;
		}
	}
}

CBehaviorGraphNode* CBehaviorGraph::FindNodeById( Uint32 id ) const
{
	TQueue< CBehaviorGraphNode* > containerNodes;

	containerNodes.Push( m_rootNode );

	while( !containerNodes.Empty() )
	{
		CBehaviorGraphNode* currNode = containerNodes.Front();

		if ( currNode->GetId() == id )
		{
			return currNode;
		}

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

	return NULL;
}

void CBehaviorGraph::SetDefaultStateMachine( CBehaviorGraphStateMachineNode* stateMachine )
{
	m_defaultStateMachine = stateMachine;
}

template< class UsedVarArray, class VarList >
Int32 RemoveUnusedVariablesFrom( UsedVarArray const & used, VarList& list )
{
	UsedVarArray toRemove;
	for ( auto it = list.GetVariables().Begin(), end = list.GetVariables().End(); it != end; ++it )
	{
		if ( !used.Exist( it->m_first ) )
		{
			toRemove.PushBack( it->m_first );
		}
	}

	for ( auto it = toRemove.Begin(), end = toRemove.End(); it != end; ++it )
	{
		list.RemoveVariable( *it );
	}

	return toRemove.Size();
}

template< class UsedEvtArray, class EvtList >
Int32 RemoveUnusedEventsFrom( UsedEvtArray const & used, EvtList& list )
{
	Int32 result = 0;
	Uint32 idx = 0;
	while ( idx < list.GetNumEvents() )
	{
		auto evt = list.GetEventName( idx );
		if ( ! used.Exist( evt ) )
		{
			list.RemoveEvent( evt );
			++ result;
		}
		else
		{
			++ idx;
		}
	}
	return result;
}

Int32 CBehaviorGraph::RemoveUnusedVariablesAndEvents()
{
	Int32 result = 0;
#ifndef NO_EDITOR_GRAPH_SUPPORT
	TDynArray<CName> usedVariables;
	TDynArray<CName> usedVectorVariables;
	TDynArray<CName> usedEvents;
	TDynArray<CName> usedInternalVariables;
	TDynArray<CName> usedInternalVectorVariables;
	// variables that might be used by events, not directly by behavior graph - don't remove them!
	// yes, this is hack, but there's no trivial way to check animations and events inside animations and make sure that they are not using variables from behavior graph
	usedVariables.PushBack( CNAME(FinalStepDistance) );
	usedVariables.PushBack( CNAME(MovementAdjustmentActive) );
	usedVectorVariables.PushBack( CNAME(MovementAdjustmentLocation) );
	usedEvents.PushBack( CNAME(FinalStep) );
	CollectUsedVariablesAndEvents( usedVariables, usedVectorVariables, usedEvents, usedInternalVariables, usedInternalVectorVariables );
	result += RemoveUnusedVariablesFrom( usedVariables, GetVariables() );
	result += RemoveUnusedVariablesFrom( usedVectorVariables, GetVectorVariables() );
	result += RemoveUnusedEventsFrom( usedEvents, GetEvents() );
	result += RemoveUnusedVariablesFrom( usedInternalVariables, GetInternalVariables() );
	result += RemoveUnusedVariablesFrom( usedInternalVectorVariables, GetInternalVectorVariables() );
#endif
	return result;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraph::OnVariableNameChanged()
{
	m_variables.OnVariableNameChanged();
	m_vectorVariables.OnVariableNameChanged();
	m_internalVariables.OnVariableNameChanged();
	m_internalVectorVariables.OnVariableNameChanged();
}

void CBehaviorGraph::CollectUsedVariablesAndEvents( TDynArray<CName>& variables, TDynArray<CName>& vectorVariables, TDynArray<CName>& events, TDynArray<CName>& internalVariables, TDynArray<CName>& internalVectorVariables ) const
{
	if ( m_rootNode )
	{
		m_rootNode->GetUsedVariablesAndEvents( variables, vectorVariables, events, internalVariables, internalVectorVariables );
	}
}

void CBehaviorGraph::EnumVectorVariableNames( TDynArray< CName >& names ) const
{
	for ( auto it = m_vectorVariables.GetVariables().Begin(), end = m_vectorVariables.GetVariables().End(); it != end; ++it )
	{
		const CBehaviorVectorVariable *behaviorVar = it->m_second;
		if ( behaviorVar )
		{
			names.PushBack( behaviorVar->m_name );
		}
	}
}

void CBehaviorGraph::EnumVariableNames( TDynArray< CName >& names, Bool onlyModifiableByEffect ) const
{
	for ( auto it = m_variables.GetVariables().Begin(), end = m_variables.GetVariables().End(); it != end; ++it )
	{
		const CBehaviorVariable *behaviorVar = it->m_second;
		if ( behaviorVar )
		{
			names.PushBack( behaviorVar->m_name );
		}
	}
}

void CBehaviorGraph::EnumEventNames( TDynArray< CName> & names, Bool onlyModifiableByEffect ) const
{
	for ( Uint32 i = 0; i < m_events.GetNumEvents(); ++i )
	{
		const CBehaviorEventDescription *behaviorEvent = m_events.GetEvent( i );
		if ( behaviorEvent && onlyModifiableByEffect && behaviorEvent->m_isModifiableByEffect )
		{
			names.PushBack( behaviorEvent->GetEventName() );
		}
		else if ( behaviorEvent && !onlyModifiableByEffect )
		{
			names.PushBack( behaviorEvent->GetEventName() );
		}
	}
}

void CBehaviorGraph::EnumInternalVectorVariableNames( TDynArray< CName >& names ) const
{
	for ( auto it = m_internalVectorVariables.GetVariables().Begin(), end = m_internalVectorVariables.GetVariables().End(); it != end; ++it )
	{
		const CBehaviorVectorVariable *behaviorVar = it->m_second;
		if ( behaviorVar )
		{
			names.PushBack( behaviorVar->m_name );
		}
	}
}

void CBehaviorGraph::EnumInternalVariableNames( TDynArray< CName >& names, Bool onlyModifiableByEffect ) const
{
	for ( auto it = m_internalVariables.GetVariables().Begin(), end = m_internalVariables.GetVariables().End(); it != end; ++it )
	{
		const CBehaviorVariable *behaviorVar = it->m_second;
		if ( behaviorVar )
		{
			names.PushBack( behaviorVar->m_name );
		}
	}
}

Uint32 CBehaviorGraph::GetNumberOfNodes() const
{
	// OMG, Yes i know - Iterators for behavior node have to be written!
	TDynArray< CBehaviorGraphNode* > nodes;
	GetAllNodes( nodes );
	return nodes.Size();
}

void CBehaviorGraph::EnumUsedAnimations( TDynArray< CName >& animations ) const
{
	m_rootNode->CollectUsedAnimations( animations );
}

#endif

CBehaviorGraphNode*	CBehaviorGraph::FindNodeByName( const String &name ) const
{
	return FindNodeByName( name, m_rootNode, true );
}

CBehaviorGraphNode* CBehaviorGraph::FindNodeByName( const String &name, CBehaviorGraphNode* startNode, Bool recursive ) const
{
	TDynArray< CBehaviorGraphNode* > nodes;
	FindNodesByName( name, startNode, recursive, nodes );

	if ( nodes.Size() > 0 )
	{
		if ( nodes.Size() > 1 )
		{
			BEH_WARN( TXT("CBehaviorGraph::FindNodeByName - Found more then one node with name '%ls'. First node will be returned."), name.AsChar() );
		}

		return nodes[0];
	}
	else
	{
		return NULL;
	}
}

void CBehaviorGraph::FindNodesByName( const String &name, TDynArray< CBehaviorGraphNode* >& nodesOut ) const
{
	FindNodesByName( name, m_rootNode, true, nodesOut );
}

void CBehaviorGraph::FindNodesByName( const String &name, CBehaviorGraphNode* startNode, Bool recursive, TDynArray< CBehaviorGraphNode* >& nodesOut ) const
{
	class CFinder
	{
		String	m_name;
		Bool	m_recursive;
	public:
		CFinder( const String &name, Bool recursive )
			: m_name( name ), m_recursive(recursive)
		{
		}

		void Find( CBehaviorGraphNode *node, TDynArray< CBehaviorGraphNode* >& nodesOut ) 
		{
			if ( !node )
			{
				return;
			}

			if ( node->GetName() == m_name )
			{
				nodesOut.PushBack( node );
			}

			if ( node->IsA< CBehaviorGraphContainerNode >() )
			{
				CBehaviorGraphContainerNode *compositeNode = SafeCast< CBehaviorGraphContainerNode >( node );

				const TDynArray< CGraphBlock* >& children = compositeNode->GetConnectedChildren();

				for( Uint32 i=0; i<children.Size(); ++i )
				{
					if ( CBehaviorGraphNode* bgChild = Cast< CBehaviorGraphNode >( children[i] ) )
					{
						if ( m_recursive )
						{
							Find( bgChild, nodesOut );
						}
						else if ( bgChild->GetName() == m_name )
						{
							nodesOut.PushBack( bgChild );
						}
					}
				}
			}
		}
	};

	CFinder finder( name, recursive );

	return finder.Find( startNode, nodesOut );
}

const Float* CBehaviorGraph::GetFloatValuePtr( CName name ) const
{
	CBehaviorVariable* var = m_variables.GetVariable( name );
	return var ? &var->m_value : nullptr;
}

const Float* CBehaviorGraph::GetInternalFloatValuePtr( CName name ) const
{
	CBehaviorVariable* var = m_internalVariables.GetVariable( name );
	return var ? &var->m_value : nullptr;
}

const Vector* CBehaviorGraph::GetVectorValuePtr( CName name ) const
{
	CBehaviorVectorVariable* var = m_vectorVariables.GetVariable( name );
	return var ? &var->m_value : nullptr;
}

const Vector* CBehaviorGraph::GetInternalVectorValuePtr( CName name ) const
{
	CBehaviorVectorVariable* var = m_internalVectorVariables.GetVariable( name );
	return var ? &var->m_value : nullptr;
}

Float CBehaviorGraph::GetFloatVariableMin( CName name ) const
{
	CBehaviorVariable* var = m_variables.GetVariable( name );
	return var ? var->m_minValue : 0.f;
}

Float CBehaviorGraph::GetFloatVariableMax( CName name ) const
{
	CBehaviorVariable* var = m_variables.GetVariable( name );
	return var ? var->m_maxValue : 0.f;
}

Float CBehaviorGraph::GetFloatVariableDefault( CName name ) const
{
	CBehaviorVariable* var = m_variables.GetVariable( name );
	return var ? var->m_defaultValue : 0.f;
}

Vector CBehaviorGraph::GetVectorVariableMin( CName name ) const
{
	CBehaviorVectorVariable* var = m_vectorVariables.GetVariable( name );
	return var ? var->m_minValue : Vector::ZERO_3D_POINT;
}

Vector CBehaviorGraph::GetVectorVariableMax( CName name ) const
{
	CBehaviorVectorVariable* var = m_vectorVariables.GetVariable( name );
	return var ? var->m_maxValue : Vector::ZERO_3D_POINT;
}

Vector CBehaviorGraph::GetVectorVariableDefault( CName name ) const
{
	CBehaviorVectorVariable* var = m_vectorVariables.GetVariable( name );
	return var ? var->m_defaultValue : Vector::ZERO_3D_POINT;
}

Float CBehaviorGraph::GetRuntimeFloatVariable( const CBehaviorGraphInstance& instance, CName name, Float defVal ) const
{
	if ( CBehaviorVariable* var = m_variables.GetVariable( name ) )
	{
		return instance[ i_rtFloatVariables ][ var->m_varIndex ];
	}

	//BEH_WARN( TXT("Get float variable - couldn't find float variable") );

	return defVal;
}

Bool CBehaviorGraph::HasRuntimeFloatVariable( const CBehaviorGraphInstance& instance, CName name ) const
{
	return m_variables.HasVariable( name );
}

Bool CBehaviorGraph::HasRuntimeInternalFloatVariable( const CBehaviorGraphInstance& instance, CName name ) const
{
	return m_internalVariables.HasVariable( name );
}

Bool CBehaviorGraph::HasRuntimeVectorVariable( const CBehaviorGraphInstance& instance, CName name ) const
{
	return m_vectorVariables.HasVariable( name );
}

const Float* CBehaviorGraph::GetRuntimeFloatVariablePtr( const CBehaviorGraphInstance& instance, CName name ) const
{
	if ( CBehaviorVariable* var = m_variables.GetVariable( name ) )
	{
		return &instance[ i_rtFloatVariables ][ var->m_varIndex ];
	}
	return nullptr;
}

const Float* CBehaviorGraph::GetRuntimeInternalFloatVariablePtr( const CBehaviorGraphInstance& instance, CName name ) const
{
	if ( CBehaviorVariable* var = m_internalVariables.GetVariable( name ) )
	{
		return &instance[ i_rtInternalFloatVariables ][ var->m_varIndex ];
	}
	return nullptr;
}

const Vector* CBehaviorGraph::GetRuntimeVectorVariablePtr( const CBehaviorGraphInstance& instance, CName name ) const
{
	if ( CBehaviorVectorVariable* var = m_vectorVariables.GetVariable( name ) )
	{
		return &instance[ i_rtVectorVariables ][ var->m_varIndex ];
	}
	return nullptr;
}

Vector CBehaviorGraph::GetRuntimeVectorVariable( const CBehaviorGraphInstance& instance, CName name ) const
{
	if ( CBehaviorVectorVariable* var = m_vectorVariables.GetVariable( name ) )
	{
		return instance[ i_rtVectorVariables ][ var->m_varIndex ];
	}

	//BEH_WARN( TXT("Get vector variable - couldn't find vector variable") );

	return Vector::ZERO_3D_POINT;
}

Bool CBehaviorGraph::SetRuntimeFloatVariable( CBehaviorGraphInstance& instance, CName name, Float value ) const
{
	if ( const CBehaviorVariable* var = m_variables.GetVariable( name ) )
	{
		instance[ i_rtFloatVariables ][ var->m_varIndex ] = var->SetValue( value );
		return true;
	}

	//BEH_WARN( TXT("Set float variable - couldn't find float variable") );
	return false;
}

Bool CBehaviorGraph::SetRuntimeVectorVariable( CBehaviorGraphInstance& instance, CName name, const Vector& value ) const
{
	if ( const CBehaviorVectorVariable* var = m_vectorVariables.GetVariable( name ) )
	{
		instance[ i_rtVectorVariables ][ var->m_varIndex ] = var->SetValue( value );
		return true;
	}

	//BEH_WARN( TXT("Set vector variable - couldn't find vector variable") );
	return false;
}

Float CBehaviorGraph::ResetRuntimeFloatVariable( CBehaviorGraphInstance& instance, CName name ) const
{
	if ( const CBehaviorVariable* var = m_variables.GetVariable( name ) )
	{
		return instance[ i_rtFloatVariables ][ var->m_varIndex ] = var->Reset();
	}

	//BEH_WARN( TXT("Reset float variable - couldn't find float variable") );
	return 0.0f;
}

Vector CBehaviorGraph::ResetRuntimeVectorVariable( CBehaviorGraphInstance& instance, CName name ) const
{
	if ( const CBehaviorVectorVariable* var = m_vectorVariables.GetVariable( name ) )
	{
		return instance[ i_rtVectorVariables ][ var->m_varIndex ] = var->Reset();
	}

	//BEH_WARN( TXT("Reset vactor variable - couldn't find float variable") );
	return Vector::ZERO_3D_POINT;
}

Uint32 CBehaviorGraph::GetRuntimeFloatVariablesNum( CBehaviorGraphInstance& instance ) const
{
	return Min( m_variables.GetNumVariables(), instance[ i_rtFloatVariables ].Size() );
}

Uint32 CBehaviorGraph::GetRuntimeVectorVariablesNum( CBehaviorGraphInstance& instance ) const
{
	return Min( m_vectorVariables.GetNumVariables(), instance[ i_rtVectorVariables ].Size() );
}

void CBehaviorGraph::StoreRuntimeFloatVariables( CBehaviorGraphInstance& instance, TDynArray< Float >& variables ) const
{
	variables = instance[ i_rtFloatVariables ];
}

void CBehaviorGraph::StoreRuntimeVectorVariables( CBehaviorGraphInstance& instance, TDynArray< Vector >& variables ) const
{
	variables = instance[ i_rtVectorVariables ];
}

void CBehaviorGraph::RestoreRuntimeFloatVariables( CBehaviorGraphInstance& instance, const TDynArray< Float >& variables ) const
{
	if( instance[ i_rtFloatVariables ].Size() == variables.Size() )
	{
		instance[ i_rtFloatVariables ] = variables;
	}
	else
	{
		BEH_ERROR( TXT("RestoreRuntimeFloatVariables array size not matching, variables not restored") );
	}
}

void CBehaviorGraph::RestoreRuntimeVectorVariables( CBehaviorGraphInstance& instance, const TDynArray< Vector >& variables ) const
{
	if( instance[ i_rtVectorVariables ].Size() == variables.Size() )
	{
		instance[ i_rtVectorVariables ] = variables;
	}
	else
	{
		BEH_ERROR( TXT("RestoreRuntimeVectorVariables array size not matching, variables not restored") );
	}
}

void CBehaviorGraph::FillRuntimeVariables( CBehaviorGraphInstance& instance ) const
{
	PC_SCOPE_PIX( CBehaviorGraph_FillRuntimeVariables );
	// Float variables
	auto& floatVariables = instance[ i_rtFloatVariables ];
	floatVariables.Resize( m_variables.GetIndexBasedSize() );
	
	for ( auto it = m_variables.GetVariables().Begin(), end = m_variables.GetVariables().End(); it != end; ++it )
	{
		const CBehaviorVariable* var = it->m_second;
		ASSERT( var );
		floatVariables[ var->m_varIndex ] = var->GetDefaultValue();
	}

	// Vector variables
	auto& vectorVariables = instance[ i_rtVectorVariables ];
	vectorVariables.Resize( m_vectorVariables.GetIndexBasedSize() );

	for ( auto it = m_vectorVariables.GetVariables().Begin(), end = m_vectorVariables.GetVariables().End(); it != end; ++it )
	{
		const CBehaviorVectorVariable* var = it->m_second;
		ASSERT( var );

		vectorVariables[ var->m_varIndex ] = var->GetDefaultValue();
	}

	// Internal float variables
	auto& internalFloatVariables = instance[ i_rtInternalFloatVariables ];
	internalFloatVariables.Resize( m_internalVariables.GetIndexBasedSize() );

	for ( auto it = m_internalVariables.GetVariables().Begin(), end = m_internalVariables.GetVariables().End(); it != end; ++it )
	{
		const CBehaviorVariable* var = it->m_second;
		ASSERT( var );
		internalFloatVariables[ var->m_varIndex ] = var->GetDefaultValue();
	}

	// Vector variables
	auto& internalVectorVariables = instance[ i_rtInternalVectorVariables ];
	internalVectorVariables.Resize( m_internalVectorVariables.GetIndexBasedSize() );

	for ( auto it = m_internalVectorVariables.GetVariables().Begin(), end = m_internalVectorVariables.GetVariables().End(); it != end; ++it )
	{
		const CBehaviorVectorVariable* var = it->m_second;
		ASSERT( var );
		internalVectorVariables[ var->m_varIndex ] = var->GetDefaultValue();
	}
}

void CBehaviorGraph::ResetRuntimeVariables( CBehaviorGraphInstance& instance ) const
{
	auto& floats = instance[ i_rtFloatVariables ];
	for ( auto it = m_variables.GetVariables().Begin(), end = m_variables.GetVariables().End(); it != end; ++it )
	{
		const CBehaviorVariable* var = it->m_second;
		floats[ var->m_varIndex ] = var->Reset();
	}

	auto& vectors = instance[ i_rtVectorVariables ];
	for ( auto it = m_vectorVariables.GetVariables().Begin(), end = m_vectorVariables.GetVariables().End(); it != end; ++it )
	{
		const CBehaviorVectorVariable* var = it->m_second;
		vectors[ var->m_varIndex ] = var->Reset();
	}

	auto& internalFloats = instance[ i_rtInternalFloatVariables ];
	for ( auto it = m_internalVariables.GetVariables().Begin(), end = m_internalVariables.GetVariables().End(); it != end; ++it )
	{
		const CBehaviorVariable* var = it->m_second;
		internalFloats[ var->m_varIndex ] = var->Reset();
	}

	auto& internalVectors = instance[ i_rtInternalVectorVariables ];
	for ( auto it = m_internalVectorVariables.GetVariables().Begin(), end = m_internalVectorVariables.GetVariables().End(); it != end; ++it )
	{
		const CBehaviorVectorVariable* var = it->m_second;
		internalVectors[ var->m_varIndex ] = var->Reset();
	}
}

Float CBehaviorGraph::GetInternalFloatVariableMin( CName name ) const
{
	CBehaviorVariable* var = m_internalVariables.GetVariable( name );
	return var ? var->m_minValue : 0.f;
}

Float CBehaviorGraph::GetInternalFloatVariableMax( CName name ) const
{
	CBehaviorVariable* var = m_internalVariables.GetVariable( name );
	return var ? var->m_maxValue : 0.f;
}

Float CBehaviorGraph::GetInternalFloatVariableDefault( CName name ) const
{
	CBehaviorVariable* var = m_internalVariables.GetVariable( name );
	return var ? var->m_defaultValue : 0.f;
}

Vector CBehaviorGraph::GetInternalVectorVariableMin( CName name ) const
{
	CBehaviorVectorVariable* var = m_internalVectorVariables.GetVariable( name );
	return var ? var->m_minValue : Vector::ZERO_3D_POINT;
}

Vector CBehaviorGraph::GetInternalVectorVariableMax( CName name ) const
{
	CBehaviorVectorVariable* var = m_internalVectorVariables.GetVariable( name );
	return var ? var->m_maxValue : Vector::ZERO_3D_POINT;
}

Vector CBehaviorGraph::GetInternalVectorVariableDefault( CName name ) const
{
	CBehaviorVectorVariable* var = m_internalVectorVariables.GetVariable( name );
	return var ? var->m_defaultValue : Vector::ZERO_3D_POINT;
}

Float CBehaviorGraph::GetRuntimeInternalFloatVariable( const CBehaviorGraphInstance& instance, CName name ) const
{
	if ( CBehaviorVariable* var = m_internalVariables.GetVariable( name ) )
	{
		return instance[ i_rtInternalFloatVariables ][ var->m_varIndex ];
	}

	//BEH_WARN( TXT("Get float variable - couldn't find float variable") );

	return 0.f;
}

Vector CBehaviorGraph::GetRuntimeInternalVectorVariable( const CBehaviorGraphInstance& instance, CName name ) const
{
	if ( const CBehaviorVectorVariable* var = m_internalVectorVariables.GetVariable( name ) )
	{
		return instance[ i_rtInternalVectorVariables ][ var->m_varIndex ];
	}

	//BEH_WARN( TXT("Get vector variable - couldn't find vector variable") );

	return Vector::ZERO_3D_POINT;
}

Bool CBehaviorGraph::SetRuntimeInternalFloatVariable( CBehaviorGraphInstance& instance, CName name, Float value ) const
{
	if ( const CBehaviorVariable* var = m_internalVariables.GetVariable( name ) )
	{
		instance[ i_rtInternalFloatVariables ][ var->m_varIndex ] = var->SetValue( value );
		return true;
	}

	//BEH_WARN( TXT("Set float variable - couldn't find float variable") );
	return false;
}

Bool CBehaviorGraph::SetRuntimeInternalVectorVariable( CBehaviorGraphInstance& instance, CName name, const Vector& value ) const
{
	if ( const CBehaviorVectorVariable* var = m_internalVectorVariables.GetVariable( name ) )
	{
		instance[ i_rtInternalVectorVariables ][ var->m_varIndex ] = var->SetValue( value );
		return true;
	}

	//BEH_WARN( TXT("Set vector variable - couldn't find vector variable") );
	return false;
}

Float CBehaviorGraph::ResetRuntimeInternalFloatVariable( CBehaviorGraphInstance& instance, CName name ) const
{
	if ( const CBehaviorVariable* var = m_internalVariables.GetVariable( name ) )
	{
		return instance[ i_rtInternalFloatVariables ][ var->m_varIndex ] = var->Reset();
	}

	//BEH_WARN( TXT("Reset float variable - couldn't find float variable") );
	return false;
}

Vector CBehaviorGraph::ResetRuntimeInternalVectorVariable( CBehaviorGraphInstance& instance, CName name ) const
{
	if ( const CBehaviorVectorVariable* var = m_internalVectorVariables.GetVariable( name ) )
	{
		return instance[ i_rtInternalVectorVariables ][ var->m_varIndex ] = var->Reset();
	}

	//BEH_WARN( TXT("Reset vactor variable - couldn't find float variable") );
	return Vector::ZERO_3D_POINT;
}

Uint32 CBehaviorGraph::GetRuntimeInternalFloatVariablesNum( CBehaviorGraphInstance& instance ) const
{
	return Min( m_internalVariables.GetNumVariables(), instance[ i_rtInternalFloatVariables ].Size() );
}

Uint32 CBehaviorGraph::GetRuntimeInternalVectorVariablesNum( CBehaviorGraphInstance& instance ) const
{
	return Min( m_internalVectorVariables.GetNumVariables(), instance[ i_rtInternalVectorVariables ].Size() );
}

void CBehaviorGraph::StoreRuntimeInternalFloatVariables( CBehaviorGraphInstance& instance, TDynArray< Float >& variables ) const
{
	variables = instance[ i_rtInternalFloatVariables ];
}

void CBehaviorGraph::StoreRuntimeInternalVectorVariables( CBehaviorGraphInstance& instance, TDynArray< Vector >& variables ) const
{
	variables = instance[ i_rtInternalVectorVariables ];
}

void CBehaviorGraph::RestoreRuntimeInternalFloatVariables( CBehaviorGraphInstance& instance, const TDynArray< Float >& variables ) const
{
	if( instance[ i_rtInternalFloatVariables ].Size() == variables.Size() )
	{
		instance[ i_rtInternalFloatVariables ] = variables;
	}
	else
	{
		BEH_ERROR( TXT("RestoreRuntimeFloatVariables array size not matching, variables not restored") );
	}
}

void CBehaviorGraph::RestoreRuntimeInternalVectorVariables( CBehaviorGraphInstance& instance, const TDynArray< Vector >& variables ) const
{
	if( instance[ i_rtInternalVectorVariables ].Size() == variables.Size() )
	{
		instance[ i_rtInternalVectorVariables ] = variables;
	}
	else
	{
		BEH_ERROR( TXT("RestoreRuntimeVectorVariables array size not matching, variables not restored") );
	}
}

void CBehaviorGraph::FillRuntimeInternalVariables( CBehaviorGraphInstance& instance ) const
{
	// Float variables
	instance[ i_rtInternalFloatVariables ].Clear();

	for ( auto it = m_internalVariables.GetVariables().Begin(), end = m_internalVariables.GetVariables().End(); it != end; ++it )
	{
		instance[ i_rtInternalFloatVariables ].Insert( it->m_first, it->m_second->GetDefaultValue() );
	}

	// Vector variables
	instance[ i_rtInternalVectorVariables ].Clear();

	for ( auto it = m_internalVectorVariables.GetVariables().Begin(), end = m_internalVectorVariables.GetVariables().End(); it != end; ++it )
	{
		instance[ i_rtInternalVectorVariables ].Insert( it->m_first, it->m_second->GetDefaultValue() );
	}
}

void CBehaviorGraph::ResetRuntimeInternalVariables( CBehaviorGraphInstance& instance ) const
{
	for ( auto it = m_internalVariables.GetVariables().Begin(), end = m_internalVariables.GetVariables().End(); it != end; ++it )
	{
		const CBehaviorVariable* var = it->m_second;
		instance[ i_rtInternalFloatVariables ][ var->m_varIndex ] = var->Reset();
	}

	for ( auto it = m_internalVectorVariables.GetVariables().Begin(), end = m_internalVectorVariables.GetVariables().End(); it != end; ++it )
	{
		const CBehaviorVectorVariable* var = it->m_second;
		instance[ i_rtInternalVectorVariables ][ var->m_varIndex ] = var->Reset();
	}
}

///////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
