/*
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "extAnimItemEvents.h"
#include "definitionsManager.h"
#include "bgNpc.h"

IMPLEMENT_RTTI_ENUM( EItemAction );
IMPLEMENT_RTTI_ENUM( EGettingItem );
IMPLEMENT_ENGINE_CLASS( CExtAnimItemEvent );

CExtAnimItemEvent::CExtAnimItemEvent()
	: CExtAnimEvent()
	, m_action( IA_Mount )
	, m_category( CName::NONE )
	, m_itemName_optional( CName::NONE )
	, m_itemGetting( GI_ByName )
{
	m_reportToScript = false;
}


CExtAnimItemEvent::CExtAnimItemEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName )
	: CExtAnimEvent( eventName, animationName, startTime, trackName )
	, m_action( IA_Mount )
	, m_category( CName::NONE )
	, m_itemName_optional( CName::NONE )
{
	m_reportToScript = false;
}

CExtAnimItemEvent::~CExtAnimItemEvent()
{
}

namespace
{
	SItemUniqueId AddItemByName( CName itemName, CInventoryComponent* inventory )
	{
		inventory->AddItem( itemName );
		return inventory->GetItemId( itemName );		
	}
}

RED_DEFINE_STATIC_NAME( customCsSlot1 );
RED_DEFINE_STATIC_NAME( customCsSlot2 );

void CExtAnimItemEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	ASSERT( component );
	if ( component )
	{
		CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( component->GetEntity() );
		if ( gameplayEntity )
		{
			CInventoryComponent* inventory = gameplayEntity->GetInventoryComponent();
			if ( inventory )
			{
				SItemUniqueId itemId;
				
				if ( m_itemGetting == GI_ByCategory )
				{
					itemId = GetItemByCategory( inventory, m_category );
				}

				if ( !itemId && m_itemName_optional )
				{
					itemId = inventory->GetItemId( m_itemName_optional );
					if ( !itemId )
					{
						itemId = AddItemByName( m_itemName_optional, inventory );
						SInventoryItem* item = inventory->GetItem( itemId );
						if ( item != nullptr )
						{
							item->SetIsAddedByAnimation( true );
						}
					}
				}

				if ( !itemId && m_itemGetting == GI_ByName )
				{
					itemId = GetItemByCategory( inventory, m_category );
				}

				if ( !itemId )
				{
					if ( m_category == CNAME( silversword ) )
					{
						itemId = GetItemByCategory( inventory,  CNAME( steelsword ) );
					}
					else if	( m_category == CNAME( steelsword ) )
					{
						itemId = GetItemByCategory( inventory,  CNAME( silversword ));
					}			
				}

				const SInventoryItem* item = inventory->GetItem( itemId );
				if ( !item )
				{
					return;
				}
				
				CInventoryComponent::SMountItemInfo mountInfo;

				switch( m_action )
				{
				case IA_Mount:
					ITEM_LOG( TXT("Anim item event: Mounting item %s to slot"), item->GetName().AsString().AsChar() );
					inventory->MountItem( itemId, mountInfo );
					gameplayEntity->EquipItem( itemId, true );
					break;
				case IA_MountToHand:
					ITEM_LOG( TXT("Anim item event: Mounting item %s to hand"), item->GetName().AsString().AsChar() );
					mountInfo.m_toHand = true;
					inventory->MountItem( itemId, mountInfo );
					gameplayEntity->EquipItem( itemId, true );
					break;
				case IA_MountToLeftHand:
					ITEM_LOG( TXT("Anim item event: Mounting item %s to left hand"), item->GetName().AsString().AsChar() );
					mountInfo.m_toHand = true;
					mountInfo.m_slotOverride = CNAME( l_weapon );
					inventory->MountItem( itemId, mountInfo );
					gameplayEntity->EquipItem( itemId, true );
					break;
				case IA_MountToRightHand:
					ITEM_LOG( TXT("Anim item event: Mounting item %s to right hand"), item->GetName().AsString().AsChar() );
					mountInfo.m_toHand = true;
					mountInfo.m_slotOverride = CNAME( r_weapon );
					inventory->MountItem( itemId, mountInfo );
					gameplayEntity->EquipItem( itemId, true );
					break;
				case IA_MountToCustomSlot1:
					mountInfo.m_slotOverride = CNAME( customCsSlot1 );
					inventory->MountItem( itemId, mountInfo );
					gameplayEntity->EquipItem( itemId, true );
					break;
				case IA_MountToCustomSlot2:
					mountInfo.m_slotOverride = CNAME( customCsSlot2 );
					inventory->MountItem( itemId, mountInfo );
					gameplayEntity->EquipItem( itemId, true );
					break;
				case IA_Unmount:
					ITEM_LOG( TXT("Anim item event: Unmounting item %s"), item->GetName().AsString().AsChar() );
					inventory->UnMountItem( itemId, true );
					gameplayEntity->UnequipItem( itemId );
					if ( item->IsAddedByAnimation() )
					{
						inventory->RemoveItem( itemId );
					}
					break;
				default: ASSERT( 0 );
				}
			}
		}
		else
		{
			CBgNpc* npc = Cast< CBgNpc >( component->GetEntity() );
			if ( npc )
			{
				switch( m_action )
				{
				case IA_Mount:
					{
						npc->HoldItem( m_category );
						break;
					}
				case IA_MountToHand:
				case IA_MountToLeftHand:
				case IA_MountToRightHand:
					{
						npc->MountItem( m_category );
						break;
					}
				case IA_Unmount:
					break;
// 					{
// 						npc->UnmountItem( m_category );
// 						break;
// 					}
				default: ASSERT( 0 );
				}
			}
		}
	}
}

SItemUniqueId CExtAnimItemEvent::GetItemByCategory( CInventoryComponent* inventory, CName category ) const
{
	return  inventory->GetItemByCategoryForScene( category );
}

void CExtAnimItemEvent::GetNamesList( TDynArray< CName >& names ) const
{
	CDefinitionsManager* dm = GCommonGame->GetDefinitionsManager();

	if ( !dm )
	{
		return;
	}

	names = dm->GetItemsOfCategory( m_category );	
}

void CExtAnimItemEvent::PreprocessItem( CGameplayEntity* gpEnt, SItemUniqueId& spawnedItem, CName item, CName ignoreItemsWithTag )
{
	CInventoryComponent* inventory = gpEnt->GetInventoryComponent();
	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	CInventoryComponent::SMountItemInfo info;

	if ( !inventory )
	{
		return;
	}

	if ( item == CNAME( silversword ) || item == CNAME( steelsword ) )
	{
		const Bool haveAnySword = inventory->GetItemByCategory( CNAME( steelsword ), false ) || inventory->GetItemByCategory( CNAME( silversword ), false );
		auto func = [ item, haveAnySword, inventory, gpEnt, info, ignoreItemsWithTag ]( CName cat, CName mirrorCat, CName defItem, SItemUniqueId& spawnedItem )
		{
			if ( item == cat )
			{
				SItemUniqueId id;
				if( haveAnySword )
				{
					id = inventory->GetItemByCategoryForScene( cat, ignoreItemsWithTag );
					if ( !id )
					{
						id = inventory->GetItemByCategoryForScene( mirrorCat, ignoreItemsWithTag );
					}			
				}
				if ( !id )
				{
					id = spawnedItem = AddItemByName( defItem, inventory );
				}	
				if( id && !inventory->IsItemMounted( id ) )
				{
					inventory->MountItem( id, info );
					gpEnt->EquipItem( id, true );
				}	
			}
		};

		func( CNAME( silversword ), CNAME( steelsword ), CNAME( SilverSword1 ), spawnedItem );
		func( CNAME( steelsword ), CNAME( silversword ), CNAME( SteelSword1 ), spawnedItem );
	}
	else if( !inventory->HasItem( item ) )
	{
		spawnedItem = AddItemByName( item, inventory );
		if ( spawnedItem && item == CNAME( crossbow ) )
		{
			inventory->MountItem( spawnedItem, info );
			gpEnt->EquipItem( spawnedItem, true );
		}
	}
	else 
	{
		SItemUniqueId id = inventory->GetItemId( item );
		if( id && !inventory->IsItemMounted( id ) && item == CNAME( crossbow ) )
		{
			inventory->MountItem( id, info );
			gpEnt->EquipItem( id, true );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_RTTI_ENUM( EItemEffectAction ); 
IMPLEMENT_ENGINE_CLASS( CExtAnimItemEffectEvent );

CExtAnimItemEffectEvent::CExtAnimItemEffectEvent()
	 : CExtAnimEvent()
	 , m_itemSlot( CNAME( r_weapon ) )
	 , m_effectName( CNAME( item_effect ) )
	 , m_action( IEA_Start )
{
	m_reportToScript = false;
}

CExtAnimItemEffectEvent::CExtAnimItemEffectEvent( const CName& eventName,
									 const CName& animationName, Float startTime, const String& trackName )
									 : CExtAnimEvent( eventName, animationName, startTime, trackName )						
									 , m_itemSlot( CNAME( r_weapon ) )
									 , m_effectName( CNAME( item_effect ) )
									 , m_action( IEA_Start )
{
	m_reportToScript = false;
}

void CExtAnimItemEffectEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	CExtAnimEvent::Process( info, component );
	ASSERT( component );

	if ( component )
	{
		CActor* actor = Cast< CActor >( component->GetEntity() );
		if ( actor )
		{
			CInventoryComponent* inventory = actor->GetInventoryComponent();
			if ( !inventory )
			{
				return;
			}

			SItemUniqueId itemId = inventory->GetItemIdHeldInSlot( m_itemSlot );

			if ( itemId == SItemUniqueId::INVALID )
			{
				// No item in hand
				return;
			}

			SInventoryItem* item = inventory->GetItem( itemId );
			if ( !item )
			{
				ITEM_ERR( TXT("CExtAnimItemEffectEvent::Process - item id in actor's hand is wrong, debug!") );
				return;
			}

			if ( item->GetItemEntityProxy() )
			{
				if ( m_action == IEA_Start )
				{
					SItemEntityManager::GetInstance().PlayEffectOnEntity( item->GetItemEntityProxy(), m_effectName );
				}
				else
				{
					SItemEntityManager::GetInstance().StopEffectOnEntity( item->GetItemEntityProxy(), m_effectName );
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CExtAnimItemEffectDurationEvent );

CExtAnimItemEffectDurationEvent::CExtAnimItemEffectDurationEvent()
: CExtAnimDurationEvent()
, m_itemSlot( CNAME( r_weapon ) )
, m_effectName( CNAME( item_effect ) )
{
	m_reportToScript = false;
	m_alwaysFiresEnd = true;
}

CExtAnimItemEffectDurationEvent::CExtAnimItemEffectDurationEvent( const CName& eventName, const CName& animationName, Float startTime, Float duration, const String& trackName )
: CExtAnimDurationEvent( eventName, animationName, startTime, duration, trackName )						
, m_itemSlot( CNAME( r_weapon ) )
, m_effectName( CNAME( item_effect ) )
{
	m_reportToScript = false;
	m_alwaysFiresEnd = true;
}

void CExtAnimItemEffectDurationEvent::Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	CExtAnimDurationEvent::Start( info, component );
	ProcessEffect( info, component, IEA_Start );
}

void CExtAnimItemEffectDurationEvent::Stop( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	CExtAnimDurationEvent::Stop( info, component );
	ProcessEffect( info, component, IEA_Stop );
}

void CExtAnimItemEffectDurationEvent::ProcessEffect( const CAnimationEventFired& info, CAnimatedComponent* component, EItemEffectAction action ) const
{
	ASSERT( component );

	if ( component )
	{
		CActor* actor = Cast< CActor >( component->GetEntity() );
		if ( actor )
		{
			CInventoryComponent* inventory = actor->GetInventoryComponent();
			if ( !inventory )
			{
				return;
			}

			SItemUniqueId itemId = inventory->GetItemIdHeldInSlot( m_itemSlot );

			if ( itemId == SItemUniqueId::INVALID )
			{
				// No item in hand
				return;
			}

			SInventoryItem* item = inventory->GetItem( itemId );
			if ( !item )
			{
				ITEM_ERR( TXT("CExtAnimItemEffectEvent::Process - item id in actor's hand is wrong, debug!") );
				return;
			}

			if ( item->GetItemEntityProxy() )
			{
				if ( action == IEA_Start )
				{
					SItemEntityManager::GetInstance().PlayEffectOnEntity( item->GetItemEntityProxy(), m_effectName );
				}
				else
				{
					SItemEntityManager::GetInstance().StopEffectOnEntity( item->GetItemEntityProxy(), m_effectName );
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CExtAnimItemAnimationEvent );

CExtAnimItemAnimationEvent::CExtAnimItemAnimationEvent()
	: CExtAnimEvent()
	, m_itemCategory( CName::NONE )
	, m_itemAnimationName( CName::NONE )
{
	m_reportToScript = false;
}

CExtAnimItemAnimationEvent::CExtAnimItemAnimationEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName )
	 : CExtAnimEvent( eventName, animationName, startTime, trackName )
	 , m_itemCategory( CName::NONE )
	 , m_itemAnimationName( CName::NONE )
{
	m_reportToScript = false;
}

void CExtAnimItemAnimationEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	CExtAnimEvent::Process( info, component );
	ASSERT( component );

	if ( component )
	{
		CActor* actor = Cast< CActor >( component->GetEntity() );
		if ( actor )
		{
			CInventoryComponent* inventory = actor->GetInventoryComponent();
			if ( !inventory )
			{
				return;
			}

			SItemUniqueId itemId = inventory->GetItemByCategory( m_itemCategory, true );
			if ( itemId != SItemUniqueId::INVALID )
			{
				inventory->PlayItemAnimation( itemId, m_itemAnimationName );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CExtAnimItemBehaviorEvent );

CExtAnimItemBehaviorEvent::CExtAnimItemBehaviorEvent()
	: CExtAnimEvent()
	, m_itemCategory()
	, m_event()
{
	m_reportToScript = false;
}

CExtAnimItemBehaviorEvent::CExtAnimItemBehaviorEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName )
	: CExtAnimEvent( eventName, animationName, startTime, trackName )
	, m_itemCategory()
	, m_event()
{
	m_reportToScript = false;
}

void CExtAnimItemBehaviorEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	CExtAnimEvent::Process( info, component );
	ASSERT( component );

	if ( component )
	{
		CActor* actor = Cast< CActor >( component->GetEntity() );
		if ( actor )
		{
			CInventoryComponent* inventory = actor->GetInventoryComponent();
			if ( !inventory )
			{
				return;
			}

			SItemUniqueId itemId = inventory->GetItemByCategory( m_itemCategory, true );
			if ( itemId != SItemUniqueId::INVALID )
			{
				inventory->RaiseItemBehaviorEvent( itemId, m_event );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_RTTI_ENUM( EDropAction );
IMPLEMENT_ENGINE_CLASS( CExtAnimDropItemEvent );

CExtAnimDropItemEvent::CExtAnimDropItemEvent()
	: CExtAnimEvent()
	, m_action( DA_DropAny )
{
	m_reportToScript = false;
}

CExtAnimDropItemEvent::CExtAnimDropItemEvent( const CName& eventName,
					  const CName& animationName, Float startTime, const String& trackName )
					  : CExtAnimEvent( eventName, animationName, startTime, trackName )
					  , m_action( DA_DropAny )
{
	m_reportToScript = false;
}

void CExtAnimDropItemEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	CExtAnimEvent::Process( info, component );
	ASSERT( component );
	
	if ( component )
	{
		CActor* actor = Cast< CActor >( component->GetEntity() );
		if ( actor )
		{
			if( m_action == DA_DropLeftHand || m_action == DA_DropAny )
			{
				actor->EmptyHand( CNAME( l_weapon ), true );
			}
			if( m_action == DA_DropRightHand || m_action == DA_DropAny )
			{
				actor->EmptyHand( CNAME( r_weapon ), true );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CExtAnimLookAtEvent );

CExtAnimLookAtEvent::CExtAnimLookAtEvent()
	: CExtAnimDurationEvent()
	, m_level( LL_Head )
{
	m_reportToScript = false;
}

CExtAnimLookAtEvent::CExtAnimLookAtEvent( const CName& eventName,
		 const CName& animationName, 
		 Float startTime, 
		 Float duration, 
		 const String& trackName )
	 : CExtAnimDurationEvent( eventName, animationName, startTime, duration, trackName )
	 , m_level( LL_Head )
{
	m_reportToScript = false;
}

void CExtAnimLookAtEvent::SetActorLookAtLevel( CAnimatedComponent* component, ELookAtLevel level ) const
{
	if ( component )
	{
		CActor* actor = Cast< CActor >( component->GetEntity() );
		if ( actor )
		{
			actor->SetLookAtLevel( level );
		}
		else
		{
			CBgNpc* npc = Cast< CBgNpc >( component->GetEntity() );
			if ( npc )
			{
				npc->SetLookAtLevel( level );
			}
		}
	}
}

void CExtAnimLookAtEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	ASSERT( component );

	if ( info.m_alpha > 0.9f )
	{
		SetActorLookAtLevel( component, m_level );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CExtAnimItemSyncEvent );

CExtAnimItemSyncEvent::CExtAnimItemSyncEvent()
	: CExtAnimEvent()
	, m_equipSlot( CName::NONE )
	, m_action( ILA_Draw )
	, m_holdSlot( CName::NONE )
{
	m_reportToScript = false;
}


CExtAnimItemSyncEvent::CExtAnimItemSyncEvent( const CName& eventName,
									 const CName& animationName, Float startTime, const String& trackName )
									 : CExtAnimEvent( eventName, animationName, startTime, trackName )
									 , m_equipSlot( CName::NONE )
									 , m_action( ILA_Draw )
									 , m_holdSlot( CName::NONE )
{
	m_reportToScript = false;
}

void CExtAnimItemSyncEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	ASSERT( component );

	CActor* actor = Cast< CActor >( component->GetEntity() );
	if ( actor )
	{
		SAnimItemSyncEvent syncEvent;
		syncEvent.m_itemOwner = actor;
		syncEvent.m_equipSlot = m_equipSlot;
		syncEvent.m_holdSlot = m_holdSlot;
		syncEvent.m_actionType = m_action;
		syncEvent.m_type = AET_Tick;

		SItemEntityManager::GetInstance().OnItemAnimSyncEvent( syncEvent );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CExtAnimItemSyncDurationEvent );

CExtAnimItemSyncDurationEvent::CExtAnimItemSyncDurationEvent()
	: CExtAnimDurationEvent()
	, m_equipSlot( CName::NONE )
	, m_action( ILA_Draw )
	, m_holdSlot( CName::NONE )
{
	m_reportToScript = false;
}


CExtAnimItemSyncDurationEvent::CExtAnimItemSyncDurationEvent(	const CName& eventName, 
																const CName& animationName, 
																Float startTime, 
																Float duration, 
																const String& trackName )
	 : CExtAnimDurationEvent( eventName, animationName, startTime, duration, trackName )
	 , m_equipSlot( CName::NONE )
	 , m_action( ILA_Draw )
	 , m_holdSlot( CName::NONE )
{
	m_reportToScript = false;
}

void CExtAnimItemSyncDurationEvent::Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	ASSERT( component );

	CActor* actor = Cast< CActor >( component->GetEntity() );
	if ( actor )
	{
		SAnimItemSyncEvent syncEvent;
		syncEvent.m_itemOwner = actor;
		syncEvent.m_equipSlot = m_equipSlot;
		syncEvent.m_holdSlot = m_holdSlot;
		syncEvent.m_actionType = m_action;
		syncEvent.m_type = AET_DurationStart;

		SItemEntityManager::GetInstance().OnItemAnimSyncEvent( syncEvent );
	}
}

void CExtAnimItemSyncDurationEvent::Stop( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	ASSERT( component );

	CActor* actor = Cast< CActor >( component->GetEntity() );
	if ( actor )
	{
		SAnimItemSyncEvent syncEvent;
		syncEvent.m_itemOwner = actor;
		syncEvent.m_equipSlot = m_equipSlot;
		syncEvent.m_holdSlot = m_holdSlot;
		syncEvent.m_actionType = m_action;
		syncEvent.m_type = AET_DurationEnd;

		SItemEntityManager::GetInstance().OnItemAnimSyncEvent( syncEvent );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CExtAnimItemSyncWithCorrectionEvent );

CExtAnimItemSyncWithCorrectionEvent::CExtAnimItemSyncWithCorrectionEvent()
	: CExtAnimDurationEvent()
	, m_equipSlot( CName::NONE )
	, m_action( ILA_Draw )
	, m_holdSlot( CName::NONE )
	, m_correctionBone( TXT("IK_Weapon") )
{
	m_reportToScript = false;
}


CExtAnimItemSyncWithCorrectionEvent::CExtAnimItemSyncWithCorrectionEvent( const CName& eventName, 
																		 const CName& animationName, 
																		 Float startTime, 
																		 Float duration, 
																		 const String& trackName )
	: CExtAnimDurationEvent( eventName, animationName, startTime, duration, trackName )
	, m_equipSlot( CName::NONE )
	, m_action( ILA_Draw )
	, m_holdSlot( CName::NONE )
	, m_correctionBone( TXT("IK_Weapon") )
{
	m_reportToScript = false;
}

void CExtAnimItemSyncWithCorrectionEvent::Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	ASSERT( component );

	/*CActor* actor = Cast< CActor >( info.m_animatedComponent->GetEntity() );
	if ( actor )
	{
		SAnimItemSyncEvent syncEvent;
		syncEvent.m_itemOwner = actor;
		syncEvent.m_slotName = m_equipSlot;
		syncEvent.m_hand = m_hand;
		syncEvent.m_type = AIST_Action;
		syncEvent.m_actionType = m_action;

		SItemEntityManager::GetInstance().OnItemAnimSyncEventOccurred( syncEvent );
	}*/
}

void CExtAnimItemSyncWithCorrectionEvent::Stop( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	ASSERT( component );

	CActor* actor = Cast< CActor >( component->GetEntity() );
	if ( actor )
	{
		SAnimItemSyncEvent syncEvent;
		syncEvent.m_itemOwner = actor;
		syncEvent.m_equipSlot = m_equipSlot;
		syncEvent.m_holdSlot = m_holdSlot;
		syncEvent.m_actionType = m_action;

		SItemEntityManager::GetInstance().OnItemAnimSyncEvent( syncEvent );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CExtAnimReattachItemEvent );

CExtAnimReattachItemEvent::CExtAnimReattachItemEvent()
	: CExtAnimDurationEvent()
{
	m_reportToScript = false;
}


CExtAnimReattachItemEvent::CExtAnimReattachItemEvent( const CName& eventName, 
	const CName& animationName, 
	Float startTime, 
	Float duration, 
	const String& trackName )
	: CExtAnimDurationEvent( eventName, animationName, startTime, duration, trackName )
{
	m_reportToScript = false;
}

void CExtAnimReattachItemEvent::Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	ASSERT( component );
	if ( component )
	{
		CActor* actor = Cast< CActor >( component->GetEntity() );
		if ( actor )
		{
			CInventoryComponent* inventory = actor->GetInventoryComponent();
			if ( inventory )
			{
				CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
				if ( defMgr->CategoryExists( m_item ) )
				{
					// Item specified by category
					SItemUniqueId itemId = inventory->GetItemByCategory( m_item );
					if ( itemId )
					{
						CItemEntity* itemEntity = inventory->GetItemEntityUnsafe( itemId );
						if ( itemEntity )
						{
							// item entity dig out, reattach it
							if ( !itemEntity->ApplyAttachmentTo( actor, m_targetSlot ) )
							{
								ITEM_WARN( TXT( "CExtAnimReattachItemEvent::Process - failed to reattach item %s to target slot %s" ), m_item.AsString().AsChar(), m_targetSlot.AsString().AsChar() );
								return;
							}
							ITEM_LOG( TXT( "Reattaching item %s to %s, it will be reverted after %f" ), m_item.AsString().AsChar(), m_targetSlot.AsString().AsChar(), m_duration );
						}
					}
				}
			}
		}
	}
}

void CExtAnimReattachItemEvent::Stop( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	ASSERT( component );
	if ( component )
	{
		CActor* actor = Cast< CActor >( component->GetEntity() );
		if ( actor )
		{
			CInventoryComponent* inventory = actor->GetInventoryComponent();
			if ( inventory )
			{
				CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
				if ( defMgr->CategoryExists( m_item ) )
				{
					// Item specified by category
					SItemUniqueId itemId = inventory->GetItemByCategory( m_item );
					if ( itemId )
					{
						CInventoryComponent::SMountItemInfo mountInfo;
						mountInfo.m_toHand = true;
						inventory->MountItem( itemId, mountInfo );
						ITEM_LOG( TXT( "Item %s is being restored to it's usual hold slot" ), m_item.AsString().AsChar(), m_targetSlot.AsString().AsChar(), m_duration );
					}
				}
			}
		}
	}
}
