#include "build.h"

#include "behTreeCarryingItemPick.h"
#include "carryableItemsStorePoint.h"
#include "carryableItemsRegistry.h"

IBehTreeNodeInstance* CBehTreeNodePickItemDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

Bool CBehTreePickCarryableItemInstance::IsAvailable()
{
	CBehTreeCarryingItemData* carryingData = m_carryingItemsData.Get();
	
	CCarryableItemStorePointComponent* itemStore = carryingData->GetCurrentStorePoint();
	Bool toRet = ( itemStore && !itemStore->IsEmpty() && !carryingData->GetCarriedItem() );
	if( !toRet )
	{
		DebugNotifyAvailableFail();
		return false;
	}
	CActor* actor = m_owner->GetActor();
	
	const Float MAX_DISTANCE = 1.f;
	if ( itemStore->GetWorldPosition().DistanceSquaredTo( actor->GetWorldPosition() ) > MAX_DISTANCE*MAX_DISTANCE )
	{
		DebugNotifyAvailableFail();
		return false;
	}

	return Super::IsAvailable();
}

Bool CBehTreePickCarryableItemInstance::Activate()
{
	m_slideDone		= false;
	m_itemAttached	= false;
	return Super::Activate();
}

void CBehTreePickCarryableItemInstance::Deactivate()
{
	Super::Deactivate();
	CBehTreeCarryingItemData* carryingData = m_carryingItemsData.Get();
	CCarryableItemStorePointComponent* itemStore = carryingData->GetCurrentStorePoint();
	if( itemStore )
	{
		itemStore->FreeReservation( );
	}	
}

void CBehTreePickCarryableItemInstance::Update()
{

	CActor* actor = m_owner->GetActor();
	CBehTreeCarryingItemData* carryingData = m_carryingItemsData.Get();
	
	CCarryableItemStorePointComponent* storePoint = carryingData->GetCurrentStorePoint();
	if( !storePoint )
	{
		Complete( IBehTreeNodeInstance::BTTO_FAILED );
		return;
	}
	storePoint->FreeReservation( );	
	
	if( !m_slideDone )
	{
		Vector diff = storePoint->GetWorldPosition() - actor->GetWorldPosition();
		Float yawDistance = EulerAngles::AngleDistance( storePoint->GetWorldRotation().Yaw, actor->GetWorldRotation().Yaw );
		CMovingAgentComponent* mac = actor->GetMovingAgentComponent();

		if ( diff.SquareMag2() > 0.05f 
			|| Abs( yawDistance ) > 0.05f )
		{
			mac->Slide( diff, EulerAngles( 0, 0, -yawDistance ) );
		}
		else
		{
			m_slideDone = true;
			mac->CancelMove();
		}
	}

	if( m_slideDone )
	{
		if( m_overrideTurnOnTime < 0 )
		{
			CCarryableItemStorePointComponent* itemStore = carryingData->GetCurrentStorePoint();
			if( !itemStore || itemStore->IsEmpty() )
			{
				Complete( IBehTreeNodeInstance::BTTO_FAILED );
				return;
			}

			TDynArray< THandle< CEntity > >& items = itemStore->GetStoreItems();

			CEntity* itemToPick = nullptr;
			Int32 itemIdx=0;
			for( ; itemIdx<items.SizeInt() && !itemToPick; ++itemIdx )
			{
				itemToPick = items[ itemIdx ].Get();
			}
			--itemIdx;
			if( !itemToPick )
			{
				Complete( IBehTreeNodeInstance::BTTO_FAILED );
				return;
			}
			CEntity* ownerEnt = m_owner->GetEntity();

			ownerEnt->SetBehaviorVariable( CNAME( heldItemType ), ( float )GR4Game->GetCarryableItemsRegistry()->GetItemType( itemStore->GetStoredItemType() ) );
			ownerEnt->RaiseBehaviorForceEvent( CNAME( PickUpItem ) );
			
			//itemToPick->CreateAttachmentAtBoneWSImpl( m_owner->GetEntity(), m_boneToAttachItem, GetAttachmentPosWS( itemStore->GetStoredItemType() ), ownerEnt->GetWorldRotation() );
			//if( success )
			//{
				CBehTreeCarryingItemData* carryData = m_carryingItemsData.Get();
				carryData->SetCarriedItem( itemToPick, itemStore->GetStoredItemType() );
				items.RemoveAt( itemIdx );			
				m_overrideTurnOnTime = ( Float ) GGame->GetEngineTime() + 6.f;
			//}
			//else
			//{
			//	Complete( IBehTreeNodeInstance::BTTO_FAILED );			
			//}		
		}
		else if ( m_itemAttached || m_overrideTurnOnTime < ( Float ) GGame->GetEngineTime() )
		{					
			m_overrideTurnOnTime = -1;
			Complete( IBehTreeNodeInstance::BTTO_SUCCESS );	
		}

	}
}

Vector3 CBehTreePickCarryableItemInstance::GetAttachmentPosWS( String& itemType )
{
	CActor* ownerActor = m_owner->GetActor();
	const CAnimatedComponent* actorAnimatedComponent = ownerActor->GetRootAnimatedComponent();	

	Int32 rBoneIndex = actorAnimatedComponent->FindBoneByName( m_rBoneToAttachItem );
	Int32 lBoneIndex = actorAnimatedComponent->FindBoneByName( m_lBoneToAttachItem );
	if ( rBoneIndex == -1 || lBoneIndex == -1 )
	{
		return ownerActor->GetWorldPosition();
	}

	Vector3 pos = actorAnimatedComponent->GetBoneMatrixWorldSpace( rBoneIndex ).GetTranslation();// + actorAnimatedComponent->GetBoneMatrixWorldSpace( lBoneIndex ).GetTranslation();
	//pos /= 2;
	//pos.Z += GR4Game->GetCarryableItemsRegistry()->GetPickOffset( itemType );

	return pos;
}

Bool CBehTreePickCarryableItemInstance::OnEvent( CBehTreeEvent& e )
{
	if( e.m_eventName == CNAME( AttachItem ) )
	{
		m_itemAttached = true;
		CBehTreeCarryingItemData* carryData = m_carryingItemsData.Get();
		CCarryableItemStorePointComponent* storePoint = carryData->GetCurrentStorePoint();

		if( storePoint && carryData->GetCarriedItem() )
		{		
			carryData->GetCarriedItem()->CreateAttachmentAtBoneWSImpl( 
				m_owner->GetEntity(), 
				m_rBoneToAttachItem, 
				GetAttachmentPosWS( storePoint->GetStoredItemType() ), m_owner->GetActor()->GetWorldRotation() 
			);
		}
	}
	return false;
}