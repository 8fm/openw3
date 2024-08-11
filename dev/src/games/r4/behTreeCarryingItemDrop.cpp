#include "build.h"

#include "behTreeCarryingItemDrop.h"

IBehTreeNodeInstance* CBehTreeNodeDropItemDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

Bool CBehTreeDropCarryableItemInstance::IsAvailable()
{
	CBehTreeCarryingItemData* carryData = m_carryingItemsData.Get();

	Bool toRet = ( carryData->GetCurrentStorePoint() && carryData->GetCurrentStorePoint()->IfHasFreeSpace() && carryData->GetCarriedItem() );
	if( !toRet )
	{
		DebugNotifyAvailableFail();
	}

	return toRet && Super::IsAvailable();
}

Bool CBehTreeDropCarryableItemInstance::Activate()
{
	m_slideDone = false;
	CBehTreeCarryingItemData* carryData = m_carryingItemsData.Get();

	CEntity* carriedItem = carryData->GetCarriedItem();
	CCarryableItemStorePointComponent* itemStore = carryData->GetCurrentStorePoint();	
	Bool canActivate = itemStore != nullptr && carriedItem != nullptr;

	if( canActivate )
	{
		m_canBeCompleted = false;		
		return Super::Activate();
	}

	return false;
}

void CBehTreeDropCarryableItemInstance::Update()
{	
	CActor* actor = m_owner->GetActor();
	CBehTreeCarryingItemData* carryingData = m_carryingItemsData.Get();	
	CCarryableItemStorePointComponent* storePoint = carryingData->GetCurrentStorePoint();

	if( !storePoint )
	{
		Complete( IBehTreeNodeInstance::BTTO_FAILED );
		return;
	}

	if( !m_slideDone && ( storePoint->GetWorldPosition().DistanceSquaredTo2D( actor->GetWorldPosition() ) > 0.05f 
		|| Abs( storePoint->GetWorldRotation().Yaw - actor->GetWorldRotation().Yaw ) > 0.05f ) )
	{
		CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
		float yawDistance = storePoint->GetWorldRotation().Yaw - actor->GetWorldRotation().Yaw;
		mac->Slide( storePoint->GetWorldPosition() - actor->GetWorldPosition(), EulerAngles( 0, 0, yawDistance ));
	}
	else
	{
		if( !m_slideDone )
		{
			m_owner->GetNPC()->RaiseBehaviorForceEvent( CNAME( DropItem ) );
			m_slideDone = true;
		}		
		if( m_canBeCompleted )
		{	
			Complete( IBehTreeNodeInstance::BTTO_SUCCESS );			
		}
	}
}

Bool CBehTreeDropCarryableItemInstance::OnEvent( CBehTreeEvent& e )
{
	if( e.m_eventName == CNAME( DetachItem ) )
	{		
		CBehTreeCarryingItemData* carryData = m_carryingItemsData.Get();		
		CEntity* carriedItem = carryData->GetCarriedItem();

		if( carriedItem )
		{
			carriedItem->BreakAttachment();
			carryData->SetCarriedItem( nullptr );
		}

		CCarryableItemStorePointComponent* storePoint = carryData->GetCurrentStorePoint();
		if( storePoint )
		{					
			storePoint->PutItem( carriedItem);	;			
			storePoint->FreeReservation( );
		}
		m_canBeCompleted = true;

		CActor* ownerActor = m_owner->GetActor();
		ownerActor->SetBehaviorVariable( CNAME( heldItemType ), ( float ) EEHI_None );
	}
	return false;
}