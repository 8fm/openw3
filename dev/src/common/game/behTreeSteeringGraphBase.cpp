#include "build.h"
#include "behTreeSteeringGraphBase.h"

#include "behTreeInstance.h"
#include "moveSteeringBehavior.h"
#include "moveSteeringNode.h"

IMPLEMENT_ENGINE_CLASS( CAISteeringGraphData );
////////////////////////////////////////////////////////////////////////////
// CAISteeringGraphData
////////////////////////////////////////////////////////////////////////////
CAISteeringGraphData::CAISteeringGraphData()
	: m_mac( NULL )
	, m_nextUniqueOwnersId( 0 )
{
	m_activationsStack.Reserve( 2 );
}
CAISteeringGraphData::~CAISteeringGraphData()
{
	for ( auto it = m_graphs.Begin(), end = m_graphs.End(); it != end; ++it )
	{
		it->m_steeringGraph->ReleaseRealtimeDataInstance( m_mac, it->m_instanceBuffer );
	}
}

CAISteeringGraphData::GraphIndex CAISteeringGraphData::Add( CMoveSteeringBehavior* steeringGraph, CBehTreeInstance* owner )
{
	for ( GraphIndex i = 0, n = m_graphs.Size(); i != n; ++i )
	{
		if ( m_graphs[ i ].m_steeringGraph == steeringGraph )
		{
			return i;
		}
	}
	if ( !m_mac )
	{
		CActor* actor = owner->GetActor();
		m_mac = actor ? actor->GetMovingAgentComponent() : NULL;
		if ( !m_mac )
		{
			return INVALID_INDEX;
		}
	}

	// create new graph
	GraphData d;
	d.m_steeringGraph = steeringGraph;
	InstanceBuffer* instanceBuffer = steeringGraph->CreateRealtimeDataInstance( m_mac );
	d.m_instanceBuffer = instanceBuffer;
	
	// push new graph
	m_graphs.PushBack( d );
	return m_graphs.Size()-1;
}

void CAISteeringGraphData::ActivateGraph( CMovingAgentComponent* mac, GraphIndex idx, OwnerId ownerId )
{
	ActivationStackEntry e;
	e.m_graphIndex = idx;
	e.m_holder = ownerId;
	m_activationsStack.PushBack( e );
	CAISteeringGraphData::GraphData& data = m_graphs[ idx ];
	mac->SetCustomSteeringBehavior( data.m_steeringGraph, data.m_instanceBuffer );
}
void CAISteeringGraphData::DeactivateGraph( CMovingAgentComponent* mac, OwnerId ownerId )
{
	Bool isTopGraph = true;
	for ( Int32 i = m_activationsStack.Size()-1; i >= 0; --i )
	{
		if ( m_activationsStack[ i ].m_holder == ownerId )
		{
			m_activationsStack.RemoveAt( i );
			if ( isTopGraph )
			{
				if ( m_activationsStack.Empty() )
				{
					mac->ClearCustomSteeringBehavior();
				}
				else
				{
					CAISteeringGraphData::GraphData& data = m_graphs[ m_activationsStack.Back().m_graphIndex ];
					mac->SetCustomSteeringBehavior( data.m_steeringGraph, data.m_instanceBuffer );
				}
			}
			return;
		}
		isTopGraph = false;
	}
}

void CAISteeringGraphData::CustomSerialize( IFile& file ) 
{
	for ( auto it = m_graphs.Begin(), end = m_graphs.End(); it != end; ++it )
	{
		file << it->m_steeringGraph;
	}
	
}

////////////////////////////////////////////////////////////////////////////
// CAISteeringGraphData::CInitializer
////////////////////////////////////////////////////////////////////////////

CName CAISteeringGraphData::CInitializer::GetItemName() const
{
	return CNAME( SteeringGraphsList );
}
void CAISteeringGraphData::CInitializer::InitializeItem( CAIStorageItem& item ) const
{
	Super::InitializeItem( item );

	AddFlags( item, AISTORAGE_GARBAGECOLLECTED );
}
IRTTIType* CAISteeringGraphData::CInitializer::GetItemType() const
{
	return CAISteeringGraphData::GetStaticClass();
}


////////////////////////////////////////////////////////////////////////////
// CAISteeringGraphDataPtr
////////////////////////////////////////////////////////////////////////////
CAISteeringGraphDataPtr::CAISteeringGraphDataPtr( CMoveSteeringBehavior* steeringGraph, CBehTreeInstance* owner )
	: Super( CAISteeringGraphData::CInitializer(), owner )
{
	CAISteeringGraphData* data = Item();
	if ( data )
	{
		m_index = data->Add( steeringGraph, owner );
		if ( m_index == CAISteeringGraphData::INVALID_INDEX )
		{
			Clear();
		}
		else
		{
			m_uniqueId = data->GetUniqueOwnerId();
		}
		
	}
	
}

////////////////////////////////////////////////////////////////////////////
// CBehTreeSteeringGraphCommonInstance
////////////////////////////////////////////////////////////////////////////
CBehTreeSteeringGraphCommonInstance::CBehTreeSteeringGraphCommonInstance( const CBehTreeSteeringGraphCommonDef& def, CBehTreeInstance* owner, const CBehTreeSpawnContext& context )
{
	CMoveSteeringBehavior* steeringGraph = def.m_steeringGraph.GetVal( context );
	if ( steeringGraph )
	{
		m_data = Move( CAISteeringGraphDataPtr( steeringGraph, owner ) );
	}
}
CBehTreeSteeringGraphCommonInstance::CBehTreeSteeringGraphCommonInstance( CMoveSteeringBehavior* steeringGraph, CBehTreeInstance* owner )
{
	if ( steeringGraph )
	{
		m_data = Move( CAISteeringGraphDataPtr( steeringGraph, owner ) );
	}
}
void CBehTreeSteeringGraphCommonInstance::InitializeSteeringGraph( CMoveSteeringBehavior* steeringGraph, CBehTreeInstance* owner )
{
	m_data = Move( CAISteeringGraphDataPtr( steeringGraph, owner ) );
}


void CBehTreeSteeringGraphCommonInstance::ActivateSteering( CBehTreeInstance* owner )
{
	if ( m_data )
	{
		CActor* actor = owner->GetActor();
		CMovingAgentComponent* mac = actor ? actor->GetMovingAgentComponent() : NULL;
		if ( mac )
		{
			m_data->ActivateGraph( mac, m_data.GetIndex(), m_data.GetUid() );
		}
	}
}
void CBehTreeSteeringGraphCommonInstance::DeactivateSteering( CBehTreeInstance* owner )
{
	if ( m_data )
	{
		CActor* actor = owner->GetActor();
		CMovingAgentComponent* mac = actor ? actor->GetMovingAgentComponent() : NULL;
		if ( mac )
		{
			m_data->DeactivateGraph( mac, m_data.GetUid() );
		}
	}
}