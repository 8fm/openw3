#include "build.h"

#include "../../common/game/aiSpawnTreeParameters.h"
#include "../../common/game/reactionSceneActor.h"

#include "behTreeCarryingItemBaseDecorator.h"
#include "../../common/engine/tagManager.h"
#include "../../common/engine/areaComponent.h"



BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeDecoratorCarryingItemManagerDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeDecoratorCarryingItemsBaseDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionIsCarryingItemDefinition )


///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorCarryingItemManagerInstance
///////////////////////////////////////////////////////////////////////////////

CBehTreeNodeDecoratorCarryingItemManagerInstance::CBehTreeNodeDecoratorCarryingItemManagerInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_carryingItemsData( owner )		
	, m_dropOnDeactivation( def.m_dropOnDeactivation.GetVal( context ) )
{
	{
		SBehTreeEvenListeningData e;
		e.m_eventName = CNAME( OnPoolRequest );
		e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		context.AddEventListener( e, this );
	}
}

void CBehTreeNodeDecoratorCarryingItemManagerInstance::OnDestruction()
{
	{
		SBehTreeEvenListeningData e;
		e.m_eventName = CNAME( OnPoolRequest );
		e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		m_owner->RemoveEventListener( e, this );
	}
	Super::OnDestruction();
}

Bool CBehTreeNodeDecoratorCarryingItemManagerInstance::Activate()
{
	Bool ret = Super::Activate();
	if ( ret )
	{
		CReactionSceneActorComponent* reactionSceneActor = m_owner->GetActor()->FindComponent< CReactionSceneActorComponent >();
		if( reactionSceneActor )
		{
			reactionSceneActor->LockChats();
		}
	}

	return ret;
}

Bool CBehTreeNodeDecoratorCarryingItemManagerInstance::Interrupt()
{
	return Super::Interrupt();
}

void CBehTreeNodeDecoratorCarryingItemManagerInstance::Deactivate()
{						
	CReactionSceneActorComponent* reactionSceneActor = m_owner->GetActor()->FindComponent< CReactionSceneActorComponent >();
	if( reactionSceneActor )
	{
		reactionSceneActor->UnlockChats();
	}
	
	if ( m_dropOnDeactivation )
	{
		DropImmediate();
	}	

	Super::Deactivate();
}

void CBehTreeNodeDecoratorCarryingItemManagerInstance::DropImmediate()
{
	CBehTreeCarryingItemData* carryingData = m_carryingItemsData.Get();
	CCarryableItemStorePointComponent* storePoint = carryingData->GetCurrentStorePoint();
	CEntity* carriedItem = carryingData->GetCarriedItem();

	if( carriedItem )
	{
		carriedItem->BreakAttachment();
		carryingData->SetCarriedItem( nullptr );		
	}
	if( storePoint )
	{
		storePoint->FreeReservation( );			
		storePoint->PutItem( carriedItem );		
		carryingData->SetCurrentStorePoint( nullptr );
	}			

	m_owner->GetActor()->SetBehaviorVariable( CNAME( heldItemType ), ( float ) EEHI_None );
}

Bool CBehTreeNodeDecoratorCarryingItemManagerInstance::OnEvent( CBehTreeEvent& e )
{
	if( e.m_eventName == CNAME( AI_Load_IdleRoot ) )
	{
		DropImmediate();
	}
	return Super::OnEvent( e );
}

Bool CBehTreeNodeDecoratorCarryingItemManagerInstance::OnListenedEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == CNAME( OnPoolRequest ) )
	{	
		DropImmediate();
	}
	return Super::OnListenedEvent( e );
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorCarryingItemsBaseInstance
///////////////////////////////////////////////////////////////////////////////

CBehTreeDecoratorCarryingItemsBaseInstance::CBehTreeDecoratorCarryingItemsBaseInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )	
	, m_storeTag( def.m_storeTag.GetVal( context ) )		
	, m_carryingItemsData( owner )
	, m_customMoveData( owner )

{	
	EntityHandle entityHandle;
	CAreaComponent* area = NULL;
	context.GetValRef< EntityHandle >( def.m_carryingAreaName_var, entityHandle );
	if( entityHandle.Get() )
	{
		area = entityHandle.Get()->FindComponent< CAreaComponent >();
	}
	if ( !area )
	{
		area = CIdleBehaviorsDefaultParameters::GetDefaultWanderArea( context );
	}
	m_carryingArea = area;
}

Bool CBehTreeDecoratorCarryingItemsBaseInstance::IsAvailable()
{
	CAreaComponent* carryingArea = m_carryingArea.Get();	

	return carryingArea && Super::IsAvailable();
}


Bool CBehTreeDecoratorCarryingItemsBaseInstance::Activate()
{		
	CBehTreeCarryingItemData* carryingData = m_carryingItemsData.Get();
	CCarryableItemStorePointComponent* currentStorePoint = carryingData->GetCurrentStorePoint();

	bool success = false;

	if( currentStorePoint && currentStorePoint->IsReservedFor( m_owner->GetNPC() ) )
	{
		success = true;
		m_owner->SetActionTarget( currentStorePoint );
	}
	else 
	{
		success = FindNewPointToStoreItem();
	}
	if( success )
	{
		return Super::Activate();
	}

	return false;
}

bool CBehTreeDecoratorCarryingItemsBaseInstance::FindNewPointToStoreItem()
{
	CBehTreeCarryingItemData* carryingData = m_carryingItemsData.Get();
	CAreaComponent* carryingArea = m_carryingArea.Get();
	if( !carryingArea )
		return false;

	struct Functor : public Red::System::NonCopyable
	{
		CAreaComponent*								m_carryingArea;
		CBehTreeCarryingItemData*					m_carryingData;
		CCarryableItemStorePointComponent*			m_selectedStorePoint;
		CBehTreeDecoratorCarryingItemsBaseInstance* m_owner;
		float										m_probability;		

		Functor( CAreaComponent* carryingArea, CBehTreeCarryingItemData* carryingData, CBehTreeDecoratorCarryingItemsBaseInstance* owner )
			: m_carryingArea( carryingArea )		
			, m_carryingData( carryingData )
			, m_selectedStorePoint( NULL )
			, m_owner( owner )
			, m_probability( 0.1f )
		{}

		RED_INLINE Bool EarlyTest( CNode* node )
		{			
			if( m_selectedStorePoint )
				return false;

			CEntity* entity = Cast< CEntity >( node );

			if( !entity )
				return false;

			CCarryableItemStorePointComponent* storePoint = entity->FindComponent< CCarryableItemStorePointComponent >();

			if( !storePoint )
				return false;

			if( storePoint->IsReserved() )
				return false;

			Vector storePointPosition = storePoint->GetEntity()->GetPosition();

			if( !m_carryingArea->GetBoundingBox().Contains( storePointPosition ) )
				return false;			

			if( !m_owner->IfStorePointValid( storePoint, m_carryingData) )
				return false;

			if( GEngine->GetRandomNumberGenerator().Get< Float >( 1.0f ) > m_probability )
			{
				m_probability *= 1.1f;
				return false;
			}

			return true;			
		}
		RED_INLINE void Process( CNode* node, Bool isGuaranteedUnique )
		{
			CEntity* entity = Cast< CEntity >( node );
			m_selectedStorePoint = entity->FindComponent< CCarryableItemStorePointComponent >();			
		}		
	} functor( carryingArea, carryingData, this );

	int tries = 0;
	while( tries++ < 3 && !functor.m_selectedStorePoint )
	{
		GGame->GetActiveWorld()->GetTagManager()->IterateTaggedNodes( m_storeTag, functor );
	}	

	if( !functor.m_selectedStorePoint )
	{//disable probability and try again
		functor.m_probability = 2;
		GGame->GetActiveWorld()->GetTagManager()->IterateTaggedNodes( m_storeTag, functor );
	}

	carryingData->SetCurrentStorePoint( nullptr );

	if( functor.m_selectedStorePoint )
	{	
		if( !functor.m_selectedStorePoint->IsReserved() )
		{
			carryingData->SetCurrentStorePoint( functor.m_selectedStorePoint );
			functor.m_selectedStorePoint->ReserveStorePoint( m_owner->GetNPC() );
		}	
	}

	m_owner->SetActionTarget( carryingData->GetCurrentStorePoint() );	
	return true;
}

CBehTreeNodeConditionIsCarryingItemInstance::CBehTreeNodeConditionIsCarryingItemInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeConditionInstance( def, owner, context, parent )
	, m_carryingItemsData( owner )
{}

Bool CBehTreeNodeConditionIsCarryingItemInstance::ConditionCheck()
{
	CBehTreeCarryingItemData* carryingData = m_carryingItemsData.Get();		
	CEntity* carriedItem = carryingData->GetCarriedItem();
	return carriedItem != nullptr;	
}

void CBehTreeDecoratorCarryingItemsBaseInstance::OnSubgoalCompleted( eTaskOutcome outcome )
{
	if ( outcome == BTTO_FAILED )
	{
		CBehTreeCarryingItemData* carryingData = m_carryingItemsData.Get();
		carryingData->SetCurrentStorePoint( nullptr );
	}
	IBehTreeNodeDecoratorInstance::OnSubgoalCompleted( outcome );
}