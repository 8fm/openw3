/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "metalinkComponent.h"

#include "../engine/pathlibObstacleSpawnContext.h"
#include "../engine/pathlibWorld.h"

#include "aiExplorationParameters.h"
#include "movableRepresentationPathAgent.h"
#include "wayPointCookingContext.h"

IMPLEMENT_ENGINE_CLASS( CMetalinkComponent )
IMPLEMENT_ENGINE_CLASS( CMetalinkWithAIQueueComponent )
IMPLEMENT_ENGINE_CLASS( CMetalinDestinationComponent )

////////////////////////////////////////////////////////////////////////////
// CMetalinkComponent
////////////////////////////////////////////////////////////////////////////
CMetalinkComponent::CMetalinkComponent()
	: m_aiAction( NULL )
	, m_pathfindingCostMultiplier( 1.f )
	, m_useInternalObstacle( false )
	, m_enabledByDefault( true )
	, m_enabled( false )
	, m_isGhostLink( false )
	, m_questTrackingPortal( false )
{}

Bool CMetalinkComponent::FindDestinationPosition( CPathLibWorld& pathlib, Vector3& outPosition )
{
	if ( m_destinationEntityTag )
	{
		CNavigationCookingContext* context = pathlib.GetCookingContext();
		if ( !context )
		{
			return false;
		}
		CWayPointCookingContext* waypointsContext = context->Get< CWayPointCookingContext >();
		const CWayPointCookingContext::WaypointsMappingList* waypointsList = waypointsContext->GetWaypointsByTag( m_destinationEntityTag );
		if ( !waypointsList )
		{
			PATHLIB_ERROR( TXT("PathLib error! Metalink %s couldn't find its destination."), GetFriendlyName().AsChar() );
			return false;
		}
		if ( waypointsList->Size() > 1 )
		{
			PATHLIB_ERROR( TXT("PathLib error! Metalink %s had too many possible destinations."), GetFriendlyName().AsChar() );
			return false;
		}
		const SWayPointCookingData* wp = waypointsContext->GetWaypoint( (*waypointsList)[ 0 ] );
		ASSERT( wp );
		outPosition = wp->m_position;
		return true;
	}

	CEntity* destinationEntity = GetEntity();

	for ( ComponentIterator< CWayPointComponent > it( destinationEntity ); it; ++it )
	{
		CWayPointComponent* currComponent = *it;
		if ( m_destinationWaypointComponent.Empty() || currComponent->GetName() == m_destinationWaypointComponent )
		{
			outPosition = currComponent->GetWorldPositionRef().AsVector3();
			return true;
		}
	}
	return false;
}
Bool CMetalinkComponent::ConfigureGraph( GraphConfiguration& configuration, CPathLibWorld& pathlib )
{
	Vector3 posDestination;

	if( !FindDestinationPosition( pathlib, posDestination ) )
	{
		return false;
	}

	// find obstacle component
	CComponent* obstacleComponent = NULL;

	if ( m_useInternalObstacle )
	{
		CEntity* internalObstacleEntity = m_internalObstacleEntity.Get();
		if ( !internalObstacleEntity )
		{
			internalObstacleEntity = GetEntity();
		}
		const auto& components = internalObstacleEntity->GetComponents();
		for ( auto it = components.Begin(), end = components.End(); it != end; ++it )
		{
			CComponent* c = *it;
			IComponent* pathlibComponent = c->AsPathLibComponent();
			if ( pathlibComponent && pathlibComponent->AsObstacleComponent() )
			{
				if( m_internalObstacleComponent.Empty() || c->GetName() == m_internalObstacleComponent )
				{
					obstacleComponent = c;
				}
			}
		}
	}


	const Vector3& pos0 = GetWorldPositionRef().AsVector3();
	const Vector3& pos1 = posDestination;

	Bool injectGraph = true;

	configuration.m_nodes.Resize( 2 );
	configuration.m_nodes[ 0 ].m_pos = pos0;
	configuration.m_nodes[ 0 ].m_nodeFlags = PathLib::NF_DEFAULT;
	configuration.m_nodes[ 1 ].m_pos = pos1;
	configuration.m_nodes[ 1 ].m_nodeFlags = PathLib::NF_DEFAULT;
	configuration.m_connections.Resize( 1 );
	GraphConfiguration::Connection& con = configuration.m_connections[ 0 ];
	con.m_ind[ 0 ] = 0;
	con.m_ind[ 1 ] = 1;
	con.m_linkCost = PathLib::CalculateLinkCost( (pos0 - pos1).Mag() * m_pathfindingCostMultiplier );
	con.m_linkFlags = PathLib::NF_DEFAULT;
	if ( m_questTrackingPortal )
	{
		con.m_linkFlags |= PathLib::NF_IS_GHOST_LINK | PathLib::NF_PLAYER_ONLY_PORTAL;
		configuration.m_nodes[ 0 ].m_nodeFlags |= PathLib::NF_PLAYER_ONLY_PORTAL;
		configuration.m_nodes[ 1 ].m_nodeFlags |= PathLib::NF_PLAYER_ONLY_PORTAL;
	}
	else
	{
		injectGraph = false;
		if ( m_aiAction.IsValid() )
		{
			con.m_linkFlags |= PathLib::NF_IS_CUSTOM_LINK;
		}
	}

	if( m_isGhostLink )
	{	
		con.m_linkFlags |= PathLib::NF_IS_GHOST_LINK;
	}
	if ( obstacleComponent )
	{
		configuration.m_internalObstacle = new PathLib::CObstacleSpawnData();
		if ( !configuration.m_internalObstacle->Initialize( obstacleComponent, pathlib, nullptr ) )
		{
			delete configuration.m_internalObstacle;
		}
	}
	configuration.ComputeBBox();
	configuration.m_injectGraph = injectGraph;

	return true;
}

PathLib::IComponent* CMetalinkComponent::AsPathLibComponent()
{
	return this;
}
CComponent* CMetalinkComponent::AsEngineComponent()
{
	return this;
}

void CMetalinkComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CMetalinkComponent_OnAttached );

	m_enabled = m_enabledByDefault;
	if ( m_enabled )
	{
		IMetalinkComponent::Attach( world );
	}
}
void CMetalinkComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	if ( m_enabled )
	{
		IMetalinkComponent::Detach( world );
	}
}
void CMetalinkComponent::Enable( Bool b, Bool persistant )
{
	if ( b != m_enabled )
	{
		m_enabled = b;
		CWorld* world = GetWorld();
		if ( world )
		{
			if ( m_enabled )
			{
				IMetalinkComponent::Enable( world );
			}
			else
			{
				IMetalinkComponent::Disable( world );
			}
		}
	}

	if ( persistant )
	{
		m_enabledByDefault = b;
	}
}

PathLib::IMetalinkSetup::Ptr CMetalinkComponent::CreateMetalinkSetup() const
{
	return PathLib::CMetalinkSetupFactory::GetInstance().GetClassFactory( PathLib::MetalinkClassId( EGameMetalinkType::T_METALINK ) )->Request();
}

#ifndef NO_EDITOR
Bool CMetalinkComponent::RemoveOnCookedBuild()
{
	if ( m_questTrackingPortal || !m_aiAction.IsValid() )
	{
		return true;
	}
	return false;
}
#endif


////////////////////////////////////////////////////////////////////////////
// CMetalinkComponentNavigationSetup
////////////////////////////////////////////////////////////////////////////
PathLib::MetalinkClassId CMetalinkComponentNavigationSetup::GetClassId() const
{
	return PathLib::MetalinkClassId( EGameMetalinkType::T_METALINK );
}

Bool CMetalinkComponentNavigationSetup::AgentPathfollowUpdate( RuntimeData& r, PathLib::CAgent* agent, const Vector3& interactionPoint, const Vector3& destinationPoint )
{
	CPathAgent* pathAgent = agent->As< CPathAgent >();
	if ( pathAgent && pathAgent->TestLine( interactionPoint ) )
	{
		CComponent* component = r.GetEngineComponent( pathAgent );
		if ( component )
		{
			CMetalinkComponent* metalink = static_cast< CMetalinkComponent* >( component );
			IAIExplorationTree* aiAction = metalink->GetAIAction();
			if ( aiAction )
			{
				aiAction->Setup( interactionPoint, destinationPoint, metalink );
				pathAgent->ForceMovementAIAction( aiAction );
				return false;
			}
		}
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////
// CMetalinkWithAIQueueComponent
////////////////////////////////////////////////////////////////////////////
CMetalinkWithAIQueueComponent::CMetalinkWithAIQueueComponent()
{

}
CMetalinkWithAIQueueComponent::~CMetalinkWithAIQueueComponent()
{

}

IAIQueueMetalinkInterface* CMetalinkWithAIQueueComponent::GetAIQueueInterface()
{
	return this;
}


////////////////////////////////////////////////////////////////////////////
// CMetalinDestinationComponent
////////////////////////////////////////////////////////////////////////////
#ifndef NO_EDITOR
Bool CMetalinDestinationComponent::RemoveOnCookedBuild()
{
	return true;
}
#endif

