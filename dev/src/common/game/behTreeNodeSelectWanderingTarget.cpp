#include "build.h"
#include "behTreeNodeSelectWanderingTarget.h"

#include "../engine/tagManager.h"
#include "../engine/pathlibWorld.h"
#include "../engine/areaComponent.h"


#include "aiSpawnTreeParameters.h"
#include "behTreeCustomMoveData.h"
#include "behTreeInstance.h"
#include "commonGame.h"
#include "encounter.h"
#include "gameWorld.h"
#include "movableRepresentationPathAgent.h"
#include "wanderPointComponent.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeSelectWanderingTargetDecoratorDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeWanderingTaggedTargetDecoratorDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeRandomWanderingTargetDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeHistoryWanderingTargetDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeDynamicWanderingTargetDefinition )

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeSelectWanderingTargetDecoratorInstance
///////////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeSelectWanderingTargetDecoratorInstance::SelectNextPoint()
{
	return false;
}
Bool CBehTreeNodeSelectWanderingTargetDecoratorInstance::OnEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == CNAME( AI_RequestNextWanderingPoint ) )
	{
		SGameplayEventParamInt* params = e.m_gameplayEventData.Get< SGameplayEventParamInt >();
		if ( params )
		{
			if ( SelectNextPoint() )
			{
				params->m_value = true;
			}
		}

		return false;
	}
	return Super::OnEvent( e );
}
CName CBehTreeNodeSelectWanderingTargetDecoratorInstance::SelectNextPointRequestName()
{
	return CNAME( AI_RequestNextWanderingPoint );
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeWanderingTaggedTargetDecoratorInstance
///////////////////////////////////////////////////////////////////////////////
CBehTreeNodeWanderingTaggedTargetDecoratorInstance::CBehTreeNodeWanderingTaggedTargetDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_currentTarget( NULL )
{
	if ( def.m_pointsGroupTag.HasVariableSet() )
	{
		m_pointsGroupTag = def.m_pointsGroupTag.GetVal( context );
	}
	else
	{
		m_pointsGroupTag = CIdleBehaviorsDefaultParameters::GetDefaultWanderPointsTag( context );
	}
}

Bool CBehTreeNodeWanderingTaggedTargetDecoratorInstance::Activate()
{
	// Closest
	CTagManager* tagMgr = GGame->GetActiveWorld()->GetTagManager();
	CNode* target = tagMgr->GetTaggedNodeClosestTo( m_pointsGroupTag, GetOwner()->GetActor()->GetWorldPosition() );
	if ( target )
	{
		m_owner->SetActionTarget( target );
		if ( !Super::Activate() )
		{			
			m_owner->SetActionTarget( NULL );
			DebugNotifyActivationFail();
			return false;
		}

		m_currentTarget = Cast< CEntity >( target )->FindComponent< CWanderPointComponent >( );
		if( m_currentTarget.Get() != NULL )
		{
			return true;
		}
	}

	DebugNotifyActivationFail();
	return false;
}

void CBehTreeNodeWanderingTaggedTargetDecoratorInstance::Deactivate()
{
	Super::Deactivate();	
	m_owner->SetActionTarget( NULL );
}

void CBehTreeNodeWanderingTaggedTargetDecoratorInstance::OnSubgoalCompleted( eTaskOutcome outcome )
{
	if( outcome == BTTO_FAILED )
	{
		Complete( BTTO_FAILED );
	}
	else
	{
		if( SelectNextPoint() )
		{
			m_child->Activate();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeRandomWanderingTargetInstance
///////////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeRandomWanderingTargetInstance::SelectNextPoint()
{
	if( m_currentTarget.Get() )
	{
		Uint32 randomIndex		= GEngine->GetRandomNumberGenerator().Get< Uint32 >( m_currentTarget.Get()->GetConnectionsSize() );
		CEntity *const entity	= m_currentTarget.Get()->GetConnection( randomIndex );
		if ( entity )
		{
			CWanderPointComponent* wanderpoint = entity->FindComponent< CWanderPointComponent >( );
			if ( wanderpoint )
			{
				m_currentTarget	= wanderpoint;
				m_owner->SetActionTarget( wanderpoint );
				return true;
			}
		}
		else
		{
			ERR_GAME(TXT("Wander point link is NULL, check %ls"), m_currentTarget.Get()->GetEntity()->GetName().AsChar() );
		}
	}
	
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeHistoryWanderingTargetInstance
///////////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeHistoryWanderingTargetInstance::SelectNextPoint()
{
	CWanderPointComponent* currentTarget = m_currentTarget.Get();
	if( currentTarget )
	{
		m_wanderData.AddHistoryEntry( currentTarget );

		// calculate desired heading
		CActor* actor = m_owner->GetActor();
		const Vector& currentPos = actor->GetWorldPositionRef();

		// void calculate history based average history position
		Float sumWeight = 0.f;
		Vector averagePos( Vector::ZEROS );
		for ( Uint32 i = 1, n = m_wanderData.GetCollectedHistorySize(); i < n; ++i )
		{
			CWanderPointComponent* wanderPoint = m_wanderData.GetHistory( i );
			if ( !wanderPoint )
			{
				continue;
			}

			const Vector& pointPos = wanderPoint->GetWorldPositionRef();
			Float baseWeight = Float( 1.f / Float( i ) );
			sumWeight += baseWeight;
			Float ratio = baseWeight / sumWeight;

			averagePos = pointPos * ratio + averagePos * (1.f - ratio);
		}

		Bool hasDesiredAngle = false;
		Vector desiredDir( Vector::ZEROS );
		if ( sumWeight > 0.f )
		{
			desiredDir = (currentPos - averagePos).Normalized3();
			hasDesiredAngle = true;
		}

		Int32 maxScore = -1024;
		CWanderPointComponent* selection = NULL;
		for( Uint32 i = 0, n = currentTarget->GetConnectionsSize(); i < n; ++i )
		{
			CEntity* pointEntity = currentTarget->GetConnection( i );
			if( pointEntity )
			{
				CWanderPointComponent* nextNode = pointEntity->FindComponent< CWanderPointComponent >();
				if( nextNode )
				{
					Int32 score = GetNodeScore( nextNode, currentPos, hasDesiredAngle, desiredDir );
					if( score > maxScore )
					{
						maxScore = score;
						selection = nextNode;
					}
				}
			}
		}

		if ( selection )
		{
			m_currentTarget = selection;
			m_owner->SetActionTarget( selection );
			return true;
		}
		m_currentTarget = NULL;
	}

	return false;
}

Int32 CBehTreeNodeHistoryWanderingTargetInstance::GetNodeScore( CWanderPointComponent* wanderComponent, const Vector& currentPos, Bool hasDesiredAngle, const Vector& desiredDir )
{
	// Current node
	if( m_currentTarget.Get() == wanderComponent )
	{
		return -1024;
	}

	Int32 score = 0;

	// History based
	Int32 historyPos = m_wanderData.GetHistoryPosition( wanderComponent );
	if( historyPos >= 0 )
	{
		// History penalty
		Float ratio = 1.f - ( Float( historyPos ) / Float( m_wanderData.GetAbsoluteHistorySize() ) );
		// NOTICE: Its dependent to constants below. As move dir + random can made 200 difference, so we need to make at least the same for full ratio (with historyPos == 0 )
		score -= Int32( ratio * 201.f );
	}

	// Move direction based
	if ( hasDesiredAngle )
	{
		const Vector& wanderPos = wanderComponent->GetWorldPositionRef();
		Vector wanderDir = (wanderPos - currentPos).Normalized3();

		Float cosAngle = wanderDir.Dot3( desiredDir );

		Float ratio = cosAngle;
		score += Int32( ratio * 100.f );
	}
	

	// Random factor
	score += GEngine->GetRandomNumberGenerator().Get< Int32 >( 100 );

	return score;
}

void CBehTreeNodeHistoryWanderingTargetInstance::OnSubgoalCompleted( eTaskOutcome outcome )
{
	if( outcome == BTTO_FAILED )
	{
		Complete( BTTO_FAILED );
	}
	else
	{
		if( SelectNextPoint() )
		{
			m_child->Activate();
		}
		else
		{
			// Potential dead end, clear history to have a good start next time
			//m_wanderData.ClearHistory();
			Complete( BTTO_FAILED );
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDynamicWanderingTargetInstance
///////////////////////////////////////////////////////////////////////////////
CBehTreeNodeDynamicWanderingTargetInstance::CBehTreeNodeDynamicWanderingTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_customMoveData( owner )
	, m_queryTimeout( 0.f )
{	
	m_minimalWanderDistanceSq = def.m_minimalWanderDistance.GetVal( context );
	m_minimalWanderDistanceSq *= m_minimalWanderDistanceSq;

	if ( def.m_useGuardArea.GetVal( context ) )
	{
		m_guardAreaDataPtr = CBehTreeGuardAreaDataPtr( m_owner );
	}
}

void CBehTreeNodeDynamicWanderingTargetInstance::OnSpawn( const IBehTreeNodeDefinition& defNode, CBehTreeSpawnContext& context )
{
	const Definition& def = static_cast< const Definition& >( defNode );

	EntityHandle entityHandle;
	CAreaComponent* area = NULL;
	context.GetValRef< EntityHandle >( def.m_dynamicWanderAreaName_var, entityHandle );
	if( entityHandle.Get() )
	{
		area = entityHandle.Get()->FindComponent< CAreaComponent >();
	}
	if ( !area )
	{
		area = CIdleBehaviorsDefaultParameters::GetDefaultWanderArea( context );
	}
	m_dynamicWanderArea = area;

	Super::OnSpawn( defNode, context );
}

void CBehTreeNodeDynamicWanderingTargetInstance::LazyCreateQuery()
{
	if ( !m_findSpotQuery )
	{
		m_findSpotQuery = new CQueryReacheableSpotInAreaRequest();
	}
}

CAreaComponent* CBehTreeNodeDynamicWanderingTargetInstance::GetWanderingArea() const
{
	CBehTreeGuardAreaData* guardAreaData = m_guardAreaDataPtr.Get();
	if ( guardAreaData )
	{
		CAreaComponent* guardArea = guardAreaData->GetGuardArea();
		if ( guardArea )
		{
			return guardArea;
		}
	}
	return m_dynamicWanderArea.Get();
}

Bool CBehTreeNodeDynamicWanderingTargetInstance::FindSpot()
{
	LazyCreateQuery();

	switch ( m_findSpotQuery->GetQueryState() )
	{
	case CQueryReacheableSpotInAreaRequest::STATE_DISPOSED:
	case CQueryReacheableSpotInAreaRequest::STATE_SETUP:
		{
			Float time = m_owner->GetLocalTime();
			if ( m_queryTimeout > time )
			{
				return false;
			}
			CAreaComponent* area = GetWanderingArea();
			if ( !area )
			{
				m_queryTimeout = time + 5.f;
				return false;
			}
			CActor* actor = m_owner->GetActor();
			CWorld* world = actor->GetLayer()->GetWorld();
			const Box& bbox = area->GetBoundingBox();
			Vector desiredPos(
				bbox.Min.X + (bbox.Max.X - bbox.Min.X) * GEngine->GetRandomNumberGenerator().Get< Float >(),
				bbox.Min.Y + (bbox.Max.Y - bbox.Min.Y) * GEngine->GetRandomNumberGenerator().Get< Float >(),
				(bbox.Min.Z + bbox.Max.Z) * 0.5f
				);
			m_findSpotQuery->RefreshSetup( world, actor, area, desiredPos );
			m_findSpotQuery->Submit( *world->GetPathLibWorld() );
		}
		
		break;
	case CQueryReacheableSpotInAreaRequest::STATE_ONGOING:
		break;
	case CQueryReacheableSpotInAreaRequest::STATE_COMPLETED_SUCCESS:
		return true;
	case CQueryReacheableSpotInAreaRequest::STATE_COMPLETED_FAILURE:
		m_queryTimeout = m_owner->GetLocalTime() + 5.f;
		m_findSpotQuery->Dispose();
		break;
	}
	return false;
}

Bool CBehTreeNodeDynamicWanderingTargetInstance::IsAvailable()
{
	if ( !FindSpot() )
	{
		return false;
	}

	return Super::IsAvailable();
}
Int32 CBehTreeNodeDynamicWanderingTargetInstance::Evaluate()
{
	if ( !FindSpot() )
	{
		return -1;
	}

	return Super::Evaluate();
}

Bool CBehTreeNodeDynamicWanderingTargetInstance::Activate()
{
	if ( SelectNextPoint() && Super::Activate() )
	{
		return true;		
	}

	DebugNotifyActivationFail();
	return false;
}




void CBehTreeNodeDynamicWanderingTargetInstance::OnSubgoalCompleted( eTaskOutcome outcome )
{
	if( outcome == BTTO_FAILED )
	{
		Complete( BTTO_FAILED );
	}
	else
	{
		if( SelectNextPoint() )
		{
			if ( !m_child->Activate() )
			{
				Complete( BTTO_FAILED );
			}
		}
		else
		{
			Complete( BTTO_FAILED );
		}
	}
}

Bool CBehTreeNodeDynamicWanderingTargetInstance::SelectNextPoint()
{
	if ( !FindSpot() )
	{
		return false;
	}

	const Vector3& pos = m_findSpotQuery->GetComputedPosition();

	m_customMoveData->SetTarget( pos.X, pos.Y, pos.Z );

	m_findSpotQuery->Dispose();

	return true;
}

void CBehTreeNodeDynamicWanderingTargetInstance::Update()
{
	// update asynchronous request processing
	FindSpot();

	Super::Update();
}

///////////////////////////////////////////////////////////////////////////////
// CWanderData
///////////////////////////////////////////////////////////////////////////////
CWanderData::CWanderData()
	: m_lastVisitIndex( 0 )
	, m_historySize( 0 )
{
	ClearHistory();
}


Bool CWanderData::AddHistoryEntry( CWanderPointComponent* wanderPoint )
{
	Uint32 historyMaxSize =  GetAbsoluteHistorySize();
	m_lastVisitIndex = (m_lastVisitIndex + 1) % historyMaxSize;

	if( m_historySize < historyMaxSize )
	{
		++m_historySize;
	}

	m_history[m_lastVisitIndex] = wanderPoint;
	return true;
}

Int32 CWanderData::GetHistoryPosition( CWanderPointComponent* wanderPoint ) const
{
	// Get most recent (since repeats are allowed)
	Uint32 generationsSince = 0;
	if( wanderPoint )
	{
		for( Int32 i = m_lastVisitIndex-1; i >= 0; --i )
		{
			if( wanderPoint == m_history[i].Get() )
			{
				return generationsSince;
			}
			++generationsSince;
		}

		for( Uint32 i = m_historySize-1; i > m_lastVisitIndex; --i )
		{
			if( wanderPoint == m_history[i].Get() )
			{
				return i;
			}
			++generationsSince;
		}
	}

	return -1;
}

void CWanderData::ClearHistory()
{
	for( Uint32 i = 0; i < GetAbsoluteHistorySize(); ++i )
	{
		m_history[i] = nullptr;
	}
	m_historySize = 0;
}

CWanderPointComponent* CWanderData::GetLastVisitedPoint() const
{
	return m_history[ m_lastVisitIndex ].Get();
}