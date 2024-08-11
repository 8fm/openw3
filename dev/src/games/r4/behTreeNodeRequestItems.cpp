/*
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeRequestItems.h"

#include "../../common/engine/behaviorGraphStack.h"

#include "../../common/game/inventoryComponent.h"

#include "abilityManager.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeRequestItemsDefinition )

RED_DEFINE_STATIC_NAME( shield )
RED_DEFINE_STATIC_NAME( SelectedWeapon )
RED_DEFINE_STATIC_NAME( ItemProcessing )
RED_DEFINE_STATIC_NAME( ItemProcessingRequiresIdle )

CBehTreeNodeRequestItemsInstance::CBehTreeNodeRequestItemsInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_LeftItemType( def.m_LeftItemType.GetVal( context ) )
	, m_RightItemType( def.m_RightItemType.GetVal( context ) )
	, m_chooseSilverIfPossible( def.m_chooseSilverIfPossible.GetVal( context ) )
	, m_witcherSwordButTakenOutFromHip( false )
	, m_processLeftItem( false )
	, m_processRightItem( false )
	, m_itemsAreAvailable( false )
	, m_takeBowArrow( false )
	, m_takeBolt( false )
	, m_flaggedItemsProcessing( false )
	, m_executionState( STATE_INITIAL )
	, m_inventoryStateHash( CInventoryComponent::GetInvalidStateHash() )
{
	////////////////////////////////////////
	// SUPER HACK
	if ( def.m_behaviorGraphVarName )
	{
		Int32 behaviorGraph = context.GetVal( def.m_behaviorGraphVarName, -1 );
		if ( behaviorGraph == 8 ) // EBG_Combat_2Handed_Sword
		{
			m_witcherSwordButTakenOutFromHip = true;
			m_chooseSilverIfPossible = false;
		}
	}
	////////////////////////////////////////
	

	if ( m_chooseSilverIfPossible && m_RightItemType != CNAME( steelsword ) && m_RightItemType == CNAME( silversword ) )
	{
		m_chooseSilverIfPossible = false;
	}
}


Bool CBehTreeNodeRequestItemsInstance::DetermineRequiredItemsInternal()
{
	CActor* actor = m_owner->GetActor();
	CInventoryComponent* inventory = actor->GetInventoryComponent();
	if ( !inventory )
	{
		return false;
	}

	auto funProcessHand =
		[ inventory ] ( CName itemCategory, Bool& processingFlag ) -> Bool
	{
		processingFlag = false;
		if ( itemCategory == CNAME( None ) )
		{
			SInventoryItem* item = inventory->GetItemHeldInSlot( CNAME( l_weapon ) );
			if ( item && item->GetCategory() != itemCategory )
			{
				processingFlag = true;
			}
		}
		else if ( itemCategory != CNAME( Any ) )
		{
			processingFlag = true;

			Bool foundItem = false;

			// find item by category
			const auto& itemList = inventory->GetItems();
			for ( const SInventoryItem& item : itemList )
			{
				if ( item.GetCategory() == itemCategory )
				{
					// item is found
					foundItem = true;
					if ( item.IsHeld() )
					{
						// item is held, so we don't have to process it on activation
						processingFlag = false;
						break;
					}
				}
			}
			if ( !foundItem )
			{
				// search by tag fallback
				for ( const SInventoryItem& item : itemList )
				{
					if ( item.GetTags().Exist( itemCategory ) )
					{
						// item is found
						foundItem = true;
						if ( item.IsHeld() )
						{
							// item is held, so we don't have to process it on activation
							processingFlag = false;
							break;
						}
					}
				}
			}
			return foundItem;
		}
		return true;
	};

	Bool itemsAreAvailable = 
		funProcessHand( m_LeftItemType, m_processLeftItem ) &&
		funProcessHand( m_RightItemType, m_processRightItem );

	return itemsAreAvailable;
}

void CBehTreeNodeRequestItemsInstance::SelectWitcherWeapon( Float weaponType )
{
	if ( CAnimatedComponent* ac = m_owner->GetActor()->GetRootAnimatedComponent() )
	{
		if ( CBehaviorGraphStack* stack = ac->GetBehaviorStack() )
		{
			ac->GetBehaviorStack()->SetBehaviorVariable( CNAME( SelectedWeapon ), weaponType, false );
		}
	}
}

void CBehTreeNodeRequestItemsInstance::BeginItemProcessingAction( CName leftItem, CName rightItem )
{
	SActorRequiredItems info( leftItem, rightItem );
	CActor* actor = m_owner->GetActor();
	actor->SetRequiredItems( info );
	actor->ProcessRequiredItemsState();
}


Bool CBehTreeNodeRequestItemsInstance::DetermineRequiredItems()
{
	// Witcher sword silver processing hack
	if ( m_chooseSilverIfPossible )
	{
		if ( CActor* combatTarget = m_owner->GetCombatTarget().Get() )
		{
			if ( W3AbilityManager* abilityManager = W3AbilityManager::Get( combatTarget ) )
			{
				if ( abilityManager->UsesEssence() )
				{
					m_RightItemType = CNAME( silversword );
					if ( DetermineRequiredItemsInternal() )	
					{
						SelectWitcherWeapon( 1 );
						return true;
					}
				}
			}
		}

		m_RightItemType = CNAME( steelsword );
		SelectWitcherWeapon( 0 );
	}
	return DetermineRequiredItemsInternal();
}

Bool CBehTreeNodeRequestItemsInstance::CheckForRequiredItems()
{
	CActor* actor = m_owner->GetActor();
	CInventoryComponent* inventory = actor->GetInventoryComponent();
	if ( !inventory )
	{
		return false;
	}

	Uint32 currentInventoryStateHash = inventory->GetItemsStateHash();
	if ( m_inventoryStateHash != currentInventoryStateHash )
	{
		m_inventoryStateHash = currentInventoryStateHash;
		m_itemsAreAvailable = DetermineRequiredItems();
	}

	return m_itemsAreAvailable;
}

void CBehTreeNodeRequestItemsInstance::StartItemsProcessing()
{
	if ( !m_flaggedItemsProcessing )
	{
		m_owner->GetActor()->SignalGameplayEvent( CNAME( ItemProcessing ), 1 );

		m_flaggedItemsProcessing = true;
	}
}

void CBehTreeNodeRequestItemsInstance::StopItemsProcessing()
{
	if ( m_flaggedItemsProcessing )
	{
		m_owner->GetActor()->SignalGameplayEvent( CNAME( ItemProcessing ), 0 );

		m_flaggedItemsProcessing = false;
	}
}

void CBehTreeNodeRequestItemsInstance::RequestIdleProcessing()
{
	if ( !m_flaggedItemsIdleProcessing )
	{
		m_owner->GetActor()->SignalGameplayEvent( CNAME( ItemProcessingRequiresIdle ), 1 );

		m_flaggedItemsIdleProcessing = true;
	}
}

void CBehTreeNodeRequestItemsInstance::CancelRequestIdleProcessing()
{
	if ( m_flaggedItemsIdleProcessing )
	{
		m_owner->GetActor()->SignalGameplayEvent( CNAME( ItemProcessingRequiresIdle ), 0 );

		m_flaggedItemsIdleProcessing = false;
	}
}

Bool CBehTreeNodeRequestItemsInstance::IsAvailable()
{
	if ( !m_isActive && !CheckForRequiredItems() )
	{
		return false;
	}

	return Super::IsAvailable();
}

Int32 CBehTreeNodeRequestItemsInstance::Evaluate()
{
	if ( !m_isActive && !CheckForRequiredItems() )
	{
		return -1;
	}

	return Super::Evaluate();
}

////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeRequestItemsInstance::Activate()
{
	m_executionState = STATE_INITIAL;

	StartItemsProcessing();

	return Super::Activate();
}

void CBehTreeNodeRequestItemsInstance::Deactivate()
{
	StopItemsProcessing();
	CancelRequestIdleProcessing();

	Super::Deactivate();
}

void CBehTreeNodeRequestItemsInstance::Update()
{
	Bool keepProcessing;
	do 
	{
		keepProcessing = false;

		switch ( m_executionState )
		{
		case STATE_INITIAL:
			{
				// wait for possible draw/sheath animations that are already processed to finish
				CActor* actor = m_owner->GetActor();
				if ( !actor->HasLatentItemAction() )
				{
					m_executionState = STATE_PROCESS_REQUESTS;
					keepProcessing = true;
				}
			}
			break;

		case STATE_PROCESS_REQUESTS:
			// determine required processing
			if ( !DetermineRequiredItems() )
			{
				Complete( BTTO_FAILED );
				return;
			}
			// start processing
			if ( m_processLeftItem || m_processRightItem )
			{
				if ( m_chooseSilverIfPossible )
				{
					// draw weapon
					m_executionState = STATE_DRAW_START;
					keepProcessing = true;
				}
				else
				{
					// sheathe weapon
					CName leftItemRequest = m_processLeftItem ? CNAME( None ) : CNAME( Any );
					CName rightItemRequest = m_processRightItem ? CNAME( None ) : CNAME( Any );
					BeginItemProcessingAction( leftItemRequest, rightItemRequest );

					m_executionState = STATE_SHEATHE;
					keepProcessing = true;
				}
			}
			else
			{
				// no action required
				m_executionState = STATE_OK;
				StopItemsProcessing();
			}
			break;

		case STATE_SHEATHE:
			{
				// wait for sheath animation to end
				CActor* actor = m_owner->GetActor();
				if ( !actor->HasLatentItemAction() )
				{
					// determine next step
					if ( m_LeftItemType == CNAME( shield ) )
					{
						// draw shield
						m_executionState = STATE_SHIELD_START;
					}
					else
					{
						// draw all required items
						m_executionState = STATE_DRAW_START;
					}

					keepProcessing = true;
				}
			}
			break;
		case STATE_SHIELD_START:
			{
				// start draw shield action
				RequestIdleProcessing();
				BeginItemProcessingAction( m_LeftItemType, CNAME( Any ) );
				m_executionState = STATE_SHIELD;
			}
			break;

		case STATE_SHIELD:
			{
				// wait for draw shield action to end
				CActor* actor = m_owner->GetActor();
				if ( !actor->HasLatentItemAction() )
				{
					CancelRequestIdleProcessing();
					actor->OnProcessRequiredItemsFinish();

					m_executionState = STATE_DRAW_START;
					keepProcessing = true;
				}
			}
			

			break;

		case STATE_DRAW_START:
			{
				if ( m_witcherSwordButTakenOutFromHip )
				{
					SelectWitcherWeapon( 2 );
				}
				// start final items request
				BeginItemProcessingAction( m_LeftItemType, m_RightItemType );
				m_executionState = STATE_DRAW;
				keepProcessing = true;
			}
			break;

		case STATE_DRAW:
			{
				// wait for final item request to complete
				CActor* actor = m_owner->GetActor();
				if ( !actor->HasLatentItemAction() )
				{
					// everything is completed
					StopItemsProcessing();
					actor->OnProcessRequiredItemsFinish();
					
					m_executionState = STATE_OK;
					keepProcessing = false;
				}
			}
			break;

		case STATE_OK:
			break;

		default:
			ASSUME( false );
		}

	} while ( keepProcessing );

	Super::Update();
}

Bool CBehTreeNodeRequestItemsInstance::OnEvent( CBehTreeEvent& e ) 
{
	return Super::OnEvent( e );
}