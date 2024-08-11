#include "build.h"
#include "behTreeSailorMoveTo.h"
#include "../../common/game/behTreeInstance.h"
#include "../../common/game/BoatComponent.h"
#include "../../common/engine/tagManager.h"


//////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicSailorMoveToDefinition
IBehTreeNodeInstance* CBehTreeNodeAtomicSailorMoveToDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
    return new Instance( *this, owner, context, parent );
}

//////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicSailorMoveToInstance
CBehTreeNodeAtomicSailorMoveToInstance::CBehTreeNodeAtomicSailorMoveToInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
        : CBehTreeNodeAtomicActionInstance( def, owner, context, parent ) 
		, m_entityTag( def.m_entityTag.GetVal( context ) )
		, m_boatTag( def.m_boatTag.GetVal( context ) )
		, m_boatComponent()
		, m_destinationReached( false )
{
	SBehTreeEvenListeningData e;
	e.m_eventName = CNAME( BoatReachedEndOfPath );
	e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( e, this );
}
void CBehTreeNodeAtomicSailorMoveToInstance::OnDestruction()
{
	SBehTreeEvenListeningData e;
	e.m_eventName = CNAME( BoatReachedEndOfPath );
	e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( e, this );

	Super::OnDestruction();
}

Bool CBehTreeNodeAtomicSailorMoveToInstance::OnListenedEvent( CBehTreeEvent& e )
{
	
	if ( e.m_eventName == CNAME( BoatReachedEndOfPath ) )
	{
		m_destinationReached = true;
	}
	return false;
}

Bool CBehTreeNodeAtomicSailorMoveToInstance::Activate()
{
	// [ Step ] Get the boat
	CEntity* boatEntity = (CEntity*)GGame->GetActiveWorld()->GetTagManager()->GetTaggedNode( m_boatTag );
    if( boatEntity == nullptr)
    {
		DebugNotifyActivationFail();
        return false;
    }

	CBoatComponent* boatComponent = boatEntity->FindComponent< CBoatComponent >( );
    if( boatComponent == nullptr )
    {
		DebugNotifyActivationFail();
        return false;
    }
	m_boatComponent = boatComponent;
	// [ Step ] Get the goal entity
	CEntity* entity = (CEntity*)GGame->GetActiveWorld()->GetTagManager()->GetTaggedNode( m_entityTag );
    if( entity == nullptr)
    {
		DebugNotifyActivationFail();
        return false;
    }
	
	// [ Step ] Compute starting point of the path
	Vector destinationPoint = entity->GetWorldPositionRef();
  
	// [ Step ] Order Boat to go to that position
	Matrix boatLocalToWorld;
    boatComponent->GetLocalToWorld( boatLocalToWorld );
    boatComponent->PathFindingMoveToLocation( destinationPoint, boatLocalToWorld.GetAxisY() );
	m_destinationReached = false;
	return CBehTreeNodeAtomicActionInstance::Activate();
}

void CBehTreeNodeAtomicSailorMoveToInstance::Update()
{
	CBoatComponent* boatComponent = m_boatComponent.Get();
	if ( boatComponent == nullptr )
	{
		Complete( BTTO_FAILED );
		return;
	}
	if ( m_destinationReached )
	{
		Complete( BTTO_SUCCESS );
		return;
	}
	CBehTreeNodeAtomicActionInstance::Update();
}

