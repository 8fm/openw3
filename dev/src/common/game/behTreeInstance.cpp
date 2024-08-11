#include "build.h"

/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "behTreeInstance.h"

//#include "aiParameters.h"
#include "aiParameters.h"
#include "behTree.h"
#include "behTreeVars.h"
#include "behTreeMetanodeOnSpawn.h"
#include "behTreeNode.h"
#include "behTreeTask.h"
#include "behTreeScriptedNode.h"
#include "aiParameters.h"
#include "behTreeStateSerializer.h"


RED_DISABLE_WARNING_MSC( 4239) // nonstandard extension used : 'argument' : conversion from 'CBehTreeValBool' to 'CBehTreeValBool &'

////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CBehTreeInstance );

const Char* CBehTreeInstance::NULL_TEXT = TXT("NULL");

void CBehTreeInstance::SetOwner( CActor* actor )
{
	m_actor = actor;
	m_npc = Cast< CNewNPC >( actor );
}

const Char* CBehTreeInstance::GetActorDebugName() const
{
	return ( m_actor ? m_actor->GetName().AsChar() : NULL_TEXT );
}

void CBehTreeInstance::Bind( const IBehTreeNodeDefinition* treeDefinition, CBehTreeSpawnContext& context )
{
	// Only one initialization is allowed
	ASSERT( !m_treeDefinition );

	m_treeDefinition = treeDefinition;

	{
		PC_SCOPE_PIX(CBehTreeInstance_BindInstance);
		m_treeInstance = m_treeDefinition->SpawnInstance( this, context );
	}
	{
		PC_SCOPE_PIX(CBehTreeInstance_BindEventListeners);
		BindEventListeners( context );
	}
}

void CBehTreeInstance::OnSpawn( CBehTreeSpawnContext& context )
{
	if ( m_treeInstance && m_treeDefinition )
	{
		{
			PC_SCOPE_PIX(CBehTreeInstance_SpawnInstance);
			m_treeDefinition->OnSpawn( m_treeInstance, context );
		}
		{
			PC_SCOPE_PIX(CBehTreeInstance_BindEventListeners);
			BindEventListeners( context );
		}
	}
}

void CBehTreeInstance::Unbind()
{
	PrepareDestruction();
	DestroyRootAndListeners();
}

void CBehTreeInstance::PrepareDestruction()
{
	// clear all commands
	ProcessDelayedNodeDeletion();

	ClearCombatTarget();

	// Reset handle
	DiscardHandles();

	// Unbinding is valid only if the instance was properly bound before
	ASSERT( m_treeDefinition );

	// Reset machine
	m_machine = nullptr;

	// Unbind graph itself
	m_treeDefinition = nullptr;
}

void CBehTreeInstance::DestroyRootAndListeners()
{
	if ( m_treeInstance )
	{
		m_eventListeners.ClearFast();
		m_treeInstance->OnDestruction();
		delete m_treeInstance;
		m_treeInstance = nullptr;
	}
}

void CBehTreeInstance::RegisterNodeForDeletion( IBehTreeNodeInstance* node )
{
	m_delayedDestruction.PushBack( node );
}

void CBehTreeInstance::OnReattachAsync( CBehTreeSpawnContext& context )
{
	CBehTreeEvent e( CNAME( OnReattachFromPoolAsync ), &context, CBehTreeSpawnContext::GetStaticClass() );
	OnEvent( e, SBehTreeEvenListeningData::TYPE_GAMEPLAY );

	BindEventListeners( context );
}

void CBehTreeInstance::OnReattach( CBehTreeSpawnContext& context )
{
	for ( auto it = m_onReattachCallback.Begin(), end = m_onReattachCallback.End(); it != end; ++it )
	{
		(*it)->OnReattach( this, context );
	}

	CBehTreeEvent e( CNAME( OnReattachFromPool ), &context, CBehTreeSpawnContext::GetStaticClass() );
	OnEvent( e, SBehTreeEvenListeningData::TYPE_GAMEPLAY );

	BindEventListeners( context );
}

void CBehTreeInstance::OnDetached()
{
	CAIStorage::OnDetached();

	ProcessDelayedNodeDeletion();

	if ( m_treeInstance && m_treeInstance->IsActive() )
	{
		m_treeInstance->Deactivate();
	}
}

void CBehTreeInstance::ForcedUpdate()
{
	m_localDelta = 0.f;
	m_treeInstance->Update();
}

void CBehTreeInstance::Activate()
{
}
void CBehTreeInstance::Deactivate()
{
	m_actor->SetIsAIControlled( false );
	if ( m_treeInstance && m_treeInstance->IsActive() )
	{
		m_treeInstance->Deactivate();
	}
}

void CBehTreeInstance::Update( Float delta )
{
	m_localDelta = delta;
	m_localTime += delta;

	ProcessDelayedNodeDeletion();

	ASSERT( m_treeInstance != NULL, TXT("Update calling with NULL tree instance") );
	if ( m_treeInstance != NULL )
	{
		if( m_treeInstance->IsActive() )
		{
			m_treeInstance->Update();
		}
		else
		{
			m_actor->SetIsAIControlled( true );
			m_treeInstance->Activate();
		}
	}
}

Bool CBehTreeInstance::IsInCombat() const
{
	RED_HALT( "AI IsInCombat functionality is implemented only for project R4!");
	return false;
}

void CBehTreeInstance::SetIsInCombat( Bool inCombat )
{
	m_actor->OnCombatModeSet( inCombat );
}

void CBehTreeInstance::OnEvent( CBehTreeEvent& e, SBehTreeEvenListeningData::EType type ) const
{
	ASSERT( m_treeInstance != NULL, TXT("OnEvent calling with NULL tree instance") );

	if ( m_treeInstance != NULL )
	{
		m_treeInstance->OnEvent( e );
	}

	SBehTreeEvenListeningData eventData;
	eventData.m_eventName = e.m_eventName;
	eventData.m_eventType = type;
	
	const auto itFind = m_eventListeners.Find( eventData );
	if ( itFind != m_eventListeners.End() )
	{
		const auto& list = itFind->m_second;
		for ( auto it = list.Begin(), end = list.End(); it != end; ++it )
		{
			IBehTreeNodeInstance* instance = *it;
			if ( instance->OnListenedEvent( e ) )
			{
				instance = instance->GetParent();
				while ( instance )
				{
					instance->MarkDirty();
					instance = instance->GetParent();
				} 
			}
		}
	}
}

void CBehTreeInstance::ProcessDelayedNodeDeletion()
{
	while( !m_delayedDestruction.Empty() )
	{
		IBehTreeNodeInstance* subTree = m_delayedDestruction.Back();
		m_delayedDestruction.PopBack();

		subTree->OnDestruction();
		delete subTree;
	}
}

//! Process animation event
void CBehTreeInstance::ProcessAnimationEvent( const CExtAnimEvent* event, EAnimationEventType eventType, const SAnimationEventAnimInfo& animInfo ) const
{
	CBehTreeEvent e( event, eventType, &animInfo );
	OnEvent( e, SBehTreeEvenListeningData::TYPE_ANIM );
}
void CBehTreeInstance::OnGameplayEvent( CName name, void* additionalData, IRTTIType* additionalDataType ) const
{
	CBehTreeEvent e( name, additionalData, additionalDataType );
	OnEvent( e, SBehTreeEvenListeningData::TYPE_GAMEPLAY );
}

THandle< CNode > CBehTreeInstance::GetNamedTarget( CName targetName )
{ 
	const auto itFind = m_namedTarget.Find( targetName );
	if ( itFind != m_namedTarget.End() )
	{
		return itFind->m_second;
	}
	return nullptr; 
}

void CBehTreeInstance::SetNamedTarget( const CName targetName, const THandle< CNode >& node ) 
{ 
	auto itFind = m_namedTarget.Find( targetName );
	if ( itFind == m_namedTarget.End() )
	{
		m_namedTarget.Insert( targetName, node );
	}
	else
	{
		itFind->m_second = node;
	}	
}

void CBehTreeInstance::SetCombatTarget( const THandle< CActor >& node, Bool registerAsAttacker )
{
	// TODO: Remove this targeting system (implemented on actor)
	CActor* currentTarget = m_combatTarget.Get();
	if ( currentTarget )
	{
		currentTarget->RegisterAttacker( m_actor, false );
	}
	m_combatTarget = node;
	if( registerAsAttacker )
	{
		CActor* newTarget = m_combatTarget.Get();
		if ( newTarget )
		{
			newTarget->RegisterAttacker( m_actor, true );
		}
	}
}

void CBehTreeInstance::ClearCombatTarget()
{
	if ( m_actor )
	{
		CActor* combatTarget = m_combatTarget.Get();
		if ( combatTarget )
		{
			combatTarget->RegisterAttacker( m_actor, false );
		}
	}
	m_combatTarget = nullptr;
	
}

void CBehTreeInstance::OnCombatTargetDestroyed()
{
	m_combatTarget = nullptr;
}

void CBehTreeInstance::BindEventListeners( CBehTreeSpawnContext& context )
{
	auto& listenersList = context.GetEventListeners();

	if ( !listenersList.Empty() )
	{
		struct SPred
		{
			typedef CBehTreeSpawnContext::SEventHandlerInfo t; 
			RED_INLINE Bool operator()( const t& l1, const t& l2 ) const
			{
				return l1.m_event < l2.m_event;
			}
		};
		::Sort( listenersList.Begin(), listenersList.End(), SPred() );
	
		// count unique events on sorted input
		Uint32 uniqueEventTypes = 1;
		SBehTreeEvenListeningData* lastEvent = &listenersList[ 0 ].m_event;
		for( auto it = listenersList.Begin()+1, end = listenersList.End(); it != end; ++it )
		{
			if ( it->m_event != *lastEvent )
			{
				++uniqueEventTypes;
				lastEvent = &it->m_event;
			}
		}
		m_eventListeners.Reserve( uniqueEventTypes );
		
		// create listeners array using dynarray interface with sorted input
		for( auto it = listenersList.Begin(), end = listenersList.End(); it != end; )
		{
			CBehTreeSpawnContext::SEventHandlerInfo& elem = *it;

			Uint32 eventCount = 1;
			auto itNextEvent = it + 1;
			for ( ; itNextEvent != end; ++itNextEvent )
			{
				if ( (*itNextEvent).m_event != elem.m_event )
					break;
				++eventCount;
			}

			auto itCurrentList = m_eventListeners.Find( elem.m_event );
			if ( itCurrentList == m_eventListeners.End() )
			{
				itCurrentList = m_eventListeners.Insert( elem.m_event, TDynArray< IBehTreeNodeInstance* >() );
			}
			auto& listeners = itCurrentList->m_second;
			// produce listeners list
			Uint32 previousSize = listeners.Size();
			listeners.Grow( eventCount );
			for ( Uint32 i = 0; i < eventCount; ++i )
			{
				listeners[ previousSize + i ] = (*(it + i)).m_instance;
			}
			it = itNextEvent;
		}
		listenersList.ClearFast();
	}
	ASSERT( m_eventListeners.IsSorted() );
}

void CBehTreeInstance::RemoveEventListener( const SBehTreeEvenListeningData& data, IBehTreeNodeInstance* node )
{
	auto itFind = m_eventListeners.Find( data );
	if ( itFind != m_eventListeners.End() )
	{
		auto& nodeList = itFind->m_second;
		for ( auto it = nodeList.Begin(), end = nodeList.End(); it != end; ++it )
		{
			if ( (*it) == node )
			{
				nodeList.EraseFast( it );
				break;
			}
		}
	}
	
}

void CBehTreeInstance::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( file.IsGarbageCollector() )
	{
		CAIStorage::OnGarbageCollector( file );
		CAIScriptableStorage::OnGarbageCollector( file );
	}
}

void CBehTreeInstance::OnFinalize()
{
	TBaseClass::OnFinalize();
	DestroyRootAndListeners();
	CAIStorage::Clear();
}

Bool CBehTreeInstance::OnPoolRequest()
{
	if ( m_treeInstance )
	{
		SGameplayEventParamInt eventParamInt( true );
		void* pEventParamInt = static_cast< void* >( &eventParamInt );
		OnGameplayEvent( CNAME( PrePoolRequest ), &eventParamInt, SGameplayEventParamInt::GetStaticClass() );

		if ( eventParamInt.m_value == false )
		{
			return false;
		}

		OnGameplayEvent( CNAME( OnPoolRequest ) );

		if ( m_treeInstance->IsActive() )
		{
			m_treeInstance->Deactivate();
		}
	}
	
	return true;
}

void CBehTreeInstance::DescribeTicketsInfo( TDynArray< String >& info )
{
	
}

void CBehTreeInstance::AddMetanodeToReattachCallback( const IBehTreeMetanodeOnSpawnDefinition* node )
{
	m_onReattachCallback.PushBack( const_cast< IBehTreeMetanodeOnSpawnDefinition* >( node ) );
}

void CBehTreeInstance::SaveState( IGameSaver* saver )
{
	CBehTreeStateSerializer serializer( m_treeInstance );
	serializer.Save( saver );
}
Bool CBehTreeInstance::LoadState( IGameLoader* loader )
{
	CBehTreeStateSerializer serializer( m_treeInstance );
	return serializer.Load( loader );
}
