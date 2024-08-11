/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "jobTreeLeaf.h"
#include "communitySystem.h"
#include "../../common/core/gatheredResource.h"
#include "../engine/behaviorGraphStack.h"


IMPLEMENT_RTTI_ENUM( EJobForceOutDropMode );
IMPLEMENT_ENGINE_CLASS( CJobActionBase );
IMPLEMENT_ENGINE_CLASS( CJobAction );
IMPLEMENT_ENGINE_CLASS( CJobForceOutAction );

RED_DEFINE_STATIC_NAME( categoryName );
RED_DEFINE_STATIC_NAME( animName );

CGatheredResource resAnimCategories( TXT("gameplay\\globals\\animations.csv"), RGF_NotCooked );

CAnimationsCategoriesResourcesManager::CAnimationsCategoriesResourcesManager() : C2dArraysResourcesManager( resAnimCategories )
{

}

CJobActionBase::CJobActionBase()
	: m_animBlendIn( 0.2f )
	, m_animBlendOut( 0.2f )
	, m_fireBlendedEvents( true )
	, m_isSkippable( true )
	, m_allowedLookAtLevel( LL_Body )
{
}

void CJobActionBase::Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties )
{
	if ( property->GetName() == CNAME( categoryName ) )
	{
		valueProperties.m_array = &SAnimationsCategoriesResourcesManager::GetInstance().Get2dArray();
		valueProperties.m_valueColumnName = TXT("friendlyName");
	}
	/*
	else if ( property->GetName() == CNAME( animName ) )
	{
	valueProperties.m_array = m_animCategoryArray;
	valueProperties.m_columnName = TXT("animation");
	if ( !m_animCategory.Empty() )
	{
	S2daValueFilter filter;
	filter.m_columnName = TXT("category");
	filter.m_keywords.PushBack( m_animCategory );
	valueProperties.m_filters.PushBack( filter );
	}
	}
	*/
}

void CJobActionBase::GetMotionExtraction( CAnimatedComponent* animatedComp, Vector& translation, Float& rotation, Float* animDuration ) const
{
	// This animation is an approach, since npc was spawned during work, don't play it
	CSkeletalAnimationSetEntry* animationEntry = animatedComp->GetAnimationContainer()->FindAnimation( m_animName );
	CSkeletalAnimation* animation = animationEntry ? animationEntry->GetAnimation() : nullptr;
	if ( animation && animation->HasExtractedMotion() )
	{
		Float duration = animation->GetDuration();
		Matrix movement;
		animation->GetMovementAtTime( duration, movement );
		translation = movement.GetTranslation();
		rotation = movement.GetYaw();
		if ( animDuration )
		{
			*animDuration = duration * GetDurationMultiplier();
		}
	}
	else
	{
		translation.SetZeros();
		rotation = 0.f;
		if ( animation && animDuration )
		{
			*animDuration = animation->GetDuration() * GetDurationMultiplier();
		}
	}
}

void CJobActionBase::GetMotionExtractionForTime( CAnimatedComponent* animatedComp, Vector& translation, Float& rotation, Float timeRatio ) const
{
	// This animation is an approach, since npc was spawned during work, don't play it
	CSkeletalAnimationSetEntry* animationEntry = animatedComp->GetAnimationContainer()->FindAnimation( m_animName );
	CSkeletalAnimation* animation = animationEntry ? animationEntry->GetAnimation() : nullptr;
	if ( animation && animation->HasExtractedMotion() )
	{
		Float duration = animation->GetDuration();
		Matrix movement;
		animation->GetMovementAtTime( duration*timeRatio, movement );
		translation = movement.GetTranslation();
		rotation = movement.GetYaw();
	}
	else
	{
		translation.SetZeros();
		rotation = 0.f;
	}
}

Float CJobActionBase::GetDurationMultiplier() const
{
	return 1.f;
}

void CJobActionBase::OnStarted( TActionPointID currentApId, SJobActionExecutionContext& actionContext ) const
{
}

void CJobActionBase::OnAnimEvent( TActionPointID currentApId, SJobActionExecutionContext& actionContext, const CAnimationEventFired &event ) const
{
}

void CJobActionBase::OnFinished( TActionPointID currentApId, SJobActionExecutionContext& actionContext ) const
{

}

Bool CJobActionBase::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	if ( propertyName == TXT("isLookAtEnabled") )
	{
		Bool lookAtEnabled;
		readValue.AsType< Bool >( lookAtEnabled );
		m_allowedLookAtLevel = lookAtEnabled ? LL_Body : LL_Null;

		return true;
	}	

	// Pass to base class
	return TBaseClass::OnPropertyMissing( propertyName, readValue );
}

//////////////////////////////////////////////////////////////////////////

CJobAction::CJobAction()
{
}

const CName& CJobAction::GetPlace() const { return m_place; }
const CName& CJobAction::GetItemName() const { return m_itemName; }

void CJobAction::OnStarted( TActionPointID currentApId, SJobActionExecutionContext& actionContext ) const
{
	if ( !actionContext.IsValid() )
	{
		return;
	}

	TBaseClass::OnStarted( currentApId, actionContext );
}

void CJobAction::OnAnimEvent( TActionPointID currentApId, SJobActionExecutionContext& actionContext, const CAnimationEventFired &event ) const
{
	if ( !actionContext.IsValid() )
	{
		return;
	}

	TBaseClass::OnAnimEvent( currentApId, actionContext, event );

	if ( event.GetEventName() == CNAME( take_item ) )
	{
		PerformPickUp( currentApId, actionContext );
		return;
	}
	else if ( event.GetEventName() == CNAME( leave_item ) )
	{
		PerformPut( currentApId, actionContext );	
		return;
	}
}

void CJobAction::OnFinished( TActionPointID currentApId, SJobActionExecutionContext& actionContext ) const
{
	if ( !actionContext.IsValid() )
	{
		return;
	}

	TBaseClass::OnFinished( currentApId, actionContext );
}

void CJobAction::PerformPickUp( TActionPointID currentApId, SJobActionExecutionContext& actionContext ) const
{
	if ( !actionContext.m_item1.IsUsed() )
	{
		PerformPickUp( currentApId, actionContext, actionContext.m_item1 );
	}
	else if ( !actionContext.m_item2.IsUsed() )
	{
		PerformPickUp( currentApId, actionContext, actionContext.m_item2 );
	}
	else
	{
		RED_ASSERT( false, TXT( "Cannot pick up more than 2 action point items. Fix job tree." ) );
	}
}

void CJobAction::PerformPickUp( TActionPointID currentApId, SJobActionExecutionContext& actionContext, SJobActionExecutionContext::SItemInfo& itemInfo ) const
{
	// Get name of the item to pick up
	const CName itemName = GetItemName();
	if ( !itemName ) 
	{
		// No item specified, nothing to do
		return;
	}

	// Is such item defined?
	const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( itemName );
	if ( !itemDef )
	{
		ITEM_ERR( TXT("PerformPickUp - item %s definition not found!"), itemName.AsString().AsChar() );
		return;
	}

	// Get npc's inventory
	CNewNPC* npc = actionContext.m_NPCActor;
	CInventoryComponent* npcInventory = npc->GetInventoryComponent();
	if ( !npcInventory ) 
	{
		ITEM_WARN( TXT("PerformPickUp - npc %s is missing inventory component !!!"), npc->GetFriendlyName().AsChar() );
		return;
	}	

	// Get the action point inventory component
	CActionPointManager* apMan = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
	CInventoryComponent* apInventory = ( currentApId != ActionPointBadID ) ? apMan->GetInventory( currentApId ) : NULL;
	if ( !apInventory )
	{
		ITEM_ERR( TXT("PerformPickUp - action point %s is missing inventory component. Trying to pick up item %s from it"), apMan->GetFriendlyAPName( currentApId ).AsChar(), itemName.AsString().AsChar() );
		return;
	}
	
	// Does action point contain the item we want to pick up?
	if ( !apInventory->HasItem( itemName ) )
	{
		ITEM_WARN( TXT("PerformPickUp - item %s not found in ap %s inventory!"), itemName.AsString().AsChar(), apMan->GetFriendlyAPName( currentApId ).AsChar() );
		return;
	}

	//// Everything seems fine, start processing.
	SItemUniqueId itemId = apInventory->GetItemId( itemName );
	SInventoryItem* invItem = apInventory->GetItem( itemId );
	ASSERT( invItem && "Huge fuckup! HasItem returns true while GetItem fails" );

	// Store item id from ap's inventory
	itemInfo.m_apItem = itemId;

	// Give npc a logic copy of ap's item, along with it's entity proxy; store added item id
	itemInfo.m_npcItem = npcInventory->AddFakeItem( invItem->GetName(), invItem->GetItemEntityProxy() );
	// If item was dropped, let's remove it from dropped queue
	SItemEntityManager::GetInstance().UnregisterItemDropped( invItem->GetItemEntityProxy(), false );
	invItem->ReleaseProxy();
	invItem->SetIsProxyTaken( true ); // mark ap's inventory item proxy as taken by other inventory

	// Mount item to npc hand
	CInventoryComponent::SMountItemInfo mountInfo;
	mountInfo.m_toHand = true;
	npcInventory->MountItem( itemInfo.m_npcItem, mountInfo );

	// Force collecting sync tokens again
	if ( const CAnimatedComponent* animatedComponent = npc->GetRootAnimatedComponent() )
	{
		if ( CBehaviorGraphStack* behaviorStack = animatedComponent->GetBehaviorStack() )
		{
			behaviorStack->SetNeedRefreshSyncTokensOnSlot( CNAME( NPC_ANIM_SLOT ), true );
		}
	}
}

void CJobAction::PerformPut( TActionPointID currentApId, SJobActionExecutionContext& actionContext ) const
{
	if ( actionContext.m_item1.IsUsed() )
	{
		PerformPut( currentApId, actionContext, actionContext.m_item1 );
	}
	if ( actionContext.m_item2.IsUsed() )
	{
		PerformPut( currentApId, actionContext, actionContext.m_item2 );
	}
}

void CJobAction::PerformPut( TActionPointID currentApId, SJobActionExecutionContext& actionContext, SJobActionExecutionContext::SItemInfo& itemInfo ) const
{
	// Get name of the item to put
	const CName itemName = GetItemName();
	if ( !itemName || !itemInfo.m_apItem ) 
	{
		// No item specified or no item picked up -> nothing to do
		return;
	}

	// Is such item defined?
	const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( itemName );
	if ( !itemDef )
	{
		ITEM_ERR( TXT("PerformPut - item %s definition not found!"), itemName.AsString().AsChar() );
		return;
	}

	// Get npc's inventory
	CNewNPC* npc = actionContext.m_NPCActor;
	CInventoryComponent* npcInventory = npc->GetInventoryComponent();
	if ( !npcInventory ) 
	{
		ITEM_WARN( TXT("PerformPut - npc %s is missing inventory component !!!"), npc->GetFriendlyName().AsChar() );
		return;
	}

	// get the action point inventory component
	CActionPointManager* apMan = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
	CInventoryComponent* apInventory = ( currentApId != ActionPointBadID ) ? apMan->GetInventory( currentApId ) : NULL;
	if ( !apInventory )
	{
		ITEM_ERR( TXT("PerformPut - action point %s is missing inventory component. Trying to pick up item %s from it"), apMan->GetFriendlyAPName( currentApId ).AsChar(), itemName.AsString().AsChar() );
		return;
	}

	// Does action point contain the item we want to pick up?
	if ( !apInventory->HasItem( itemName ) )
	{
		ITEM_WARN( TXT("PerformPut - item %s not found in ap %s inventory!"), itemName.AsString().AsChar(), apMan->GetFriendlyAPName( currentApId ).AsChar() );
		return;
	}

	SItemUniqueId itemIdAP = apInventory->GetItemId( itemName );
	ASSERT( itemIdAP );
	ASSERT( itemIdAP == itemInfo.m_apItem && "looks like action point item id changed since picking up, report this!" );
	if ( itemIdAP != itemInfo.m_apItem )
	{
		return;
	}

	SInventoryItem* apItem = apInventory->GetItem( itemIdAP );
	ASSERT( apItem );

	// Revalidate item entity proxy for further use
	SInventoryItem* npcItem = npcInventory->GetItem( itemInfo.m_npcItem );
	if ( npcItem != nullptr )
	{
		apItem->MoveProxy( npcItem );
	}
	apItem->SetIsProxyTaken( false );
	apItem->RevalidateProxy();

	// Unmount item entity from npc hand
	npcInventory->UnMountItem( itemInfo.m_npcItem );

	// Remove logic item from npc inventory
	npcInventory->RemoveFakeItem( itemInfo.m_npcItem );

	if ( apItem->IsMounted() )
	{
		// Mount item entity back to action point slot
		CInventoryComponent::SMountItemInfo mountInfo;
		mountInfo.m_force = true;
		apInventory->MountItem( itemIdAP, mountInfo );
	}
	else
	{
		// Despawn item entity, it will be spawned again when used by npc.
		apInventory->DespawnItem( itemIdAP );
	}

	// Clear item info in action context
	itemInfo.Reset();
}

//////////////////////////////////////////////////////////////////////////

CJobForceOutAction::CJobForceOutAction()
	: m_itemDropMode( JFODM_DropAll )
	, m_speedMul( 1.f )
{
}
Float CJobForceOutAction::GetDurationMultiplier() const
{
	return 1.f / m_speedMul;
}

void CJobForceOutAction::OnAnimEvent( TActionPointID currentApId, SJobActionExecutionContext& actionContext, const CAnimationEventFired &event ) const
{
	if ( !actionContext.IsValid() )
	{
		return;
	}

	TBaseClass::OnAnimEvent( currentApId, actionContext, event );

	if ( event.GetEventName() == CNAME( drop_item ) )
	{
		DropActionPointItems( currentApId, actionContext );
	}
}

void CJobForceOutAction::OnFinished( TActionPointID currentApId, SJobActionExecutionContext& actionContext ) const
{
	if ( !actionContext.IsValid() )
	{
		return;
	}

	TBaseClass::OnFinished( currentApId, actionContext );

	CNewNPC* npc = actionContext.m_NPCActor;

	switch ( m_itemDropMode )
	{
	case JFODM_DropAll:
		DropActionPointItems( currentApId, actionContext );
		npc->EmptyHands( true );
		break;
	case JFODM_DropNonWeapon:
		DropActionPointItems( currentApId, actionContext );
		CInventoryComponent * comp = npc->GetInventoryComponent();
		if( comp )
		{
			const SItemUniqueId leftItemId = comp->GetItemIdHeldInSlot( CNAME( l_weapon ) );
			const SItemUniqueId rightItemId = comp->GetItemIdHeldInSlot( CNAME( r_weapon ) );
			if ( leftItemId != SItemUniqueId::INVALID && !npc->GetInventoryComponent()->IsWeapon( leftItemId ) )
			{
				npc->GetInventoryComponent()->DropItem( leftItemId );
			}
			if ( rightItemId != SItemUniqueId::INVALID && !npc->GetInventoryComponent()->IsWeapon( rightItemId ) )
			{
				npc->GetInventoryComponent()->DropItem( rightItemId );
			}
		}

		break;
	}
}

void CJobForceOutAction::DropActionPointItems( TActionPointID workplaceApId, SJobActionExecutionContext& actionContext ) const
{
	if ( actionContext.m_item1.IsUsed() )
	{
		DropActionPointItems( workplaceApId, actionContext, actionContext.m_item1 );
	}
	if ( actionContext.m_item2.IsUsed() )
	{
		DropActionPointItems( workplaceApId, actionContext, actionContext.m_item2 );
	}
}

void CJobForceOutAction::DropActionPointItems( TActionPointID workplaceApId, SJobActionExecutionContext& actionContext, SJobActionExecutionContext::SItemInfo& itemInfo ) const
{
	// get the action point inventory component
	CActionPointManager* apMgr = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
	CInventoryComponent* apInventory = ( workplaceApId != ActionPointBadID ) ? apMgr->GetInventory( workplaceApId ) : NULL;
	if ( !itemInfo.m_apItem || !apInventory || !apMgr )
	{
		// Clear item info in action context
		itemInfo.Reset();
		return;
	}

	CNewNPC* npc = actionContext.m_NPCActor;
	CInventoryComponent* npcInventory = npc->GetInventoryComponent();
	ASSERT( npcInventory );

	if( !npcInventory )
	{
		return;
	}

	SInventoryItem* item = apInventory->GetItem( itemInfo.m_apItem );
	if ( item )
	{
		// Revalidate item entity proxy for further use
		SInventoryItem* npcItem = npcInventory->GetItem( itemInfo.m_npcItem );
		if ( npcItem != nullptr )
		{
			item->MoveProxy( npcItem );
		}
		item->SetIsProxyTaken( false );
		item->RevalidateProxy();

		// Unmount item from npc hand
		npcInventory->UnMountItem( itemInfo.m_npcItem, false );

		// Remove logic item from npc inventory
		npcInventory->RemoveFakeItem( itemInfo.m_npcItem );

		// Revalidate item entity proxy for further use
		item->RevalidateProxy();

		if ( item->IsMounted() )
		{
			CItemEntityProxy* droppedProxy = npcInventory->GetDroppedProxy( itemInfo.m_npcItem );
			// if item was dropped, let's try to claim ownership and wait for drop timeout to mount it again
			if ( droppedProxy != nullptr && SItemEntityManager::GetInstance().ClaimDroppedItem( droppedProxy, apMgr ) )
			{
				item->SetProxy( droppedProxy, false );
				item->RevalidateProxy();
				apMgr->RegisterDroppedItem( workplaceApId, droppedProxy );
			}
			else if ( !item->GetItemEntityProxy() )
			{
				// Item is expected to be mount in ap when not used by npc, but proxy is not valid anymore ( got despawned somehow ), mount it to ap then
				CInventoryComponent::SMountItemInfo mountInfo;
				mountInfo.m_force = true;
				apInventory->MountItem( itemInfo.m_apItem, mountInfo );
			}
			else
			{
				// Item is expected to be mount in ap when not used by npc. Drop it and register for recollection.
				SItemEntityManager::GetInstance().DropItemByProxy( item->GetItemEntityProxy(), -1.0f, apMgr );
				apMgr->RegisterDroppedItem( workplaceApId, item->GetItemEntityProxy() );
			}
		}
		else
		{
			// Item is not expected to lay in AP. Despawn item entity, it will be spawned again when used by npc.
			apInventory->DespawnItem( itemInfo.m_apItem );
		}
	}

	// Clear item info in action context
	itemInfo.Reset();
}
