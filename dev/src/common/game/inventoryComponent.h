/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "itemUniqueId.h"
#include "inventoryItem.h"
#include "inventoryEditor.h"
#include "lootCache.h"
#include "../engine/behaviorGraphAnimationNode.h"
#include "../engine/component.h"
#include "../engine/scaleable.h"

class CItemEntity;

#ifndef NO_LOG

#define ITEM_LOG( format, ... )		RED_LOG( Items, format, ## __VA_ARGS__ )
#define ITEM_WARN( format, ... )	RED_LOG( Items, format, ## __VA_ARGS__ )
#define ITEM_ERR( format, ... )		RED_LOG( Items, format, ## __VA_ARGS__ )

#else

#define ITEM_LOG( format, ... )	
#define ITEM_WARN( format, ... )
#define ITEM_ERR( format, ... )	

#endif

//////////////////////////////////////////////////////////////////////////
// Only dynamic use, spawned when part list have to be emitted to script

struct SItemParts
{
	DECLARE_RTTI_STRUCT( SItemParts );

	CName	m_itemName;
	Int32	m_quantity;
};

BEGIN_CLASS_RTTI( SItemParts );
PROPERTY( m_itemName );
PROPERTY( m_quantity );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CItemAnimationSyncToken : public CAnimationSyncToken
{
	DECLARE_RTTI_SIMPLE_CLASS( CItemAnimationSyncToken );

	SItemUniqueId					m_itemUniqueId;			//!< Identifier of animated item
	THandle< CInventoryComponent >	m_inventoryComponent;	//!< Inventory component which has animated item
	THandle< CAnimatedComponent >	m_syncedAnimated;		//!< Animated component to be synced through token

private:
	Bool							m_tokenFailedToLoad;		//!< True when token failed to load, false when before loading or after successful one
	CName							m_lastUsedAnimationName;	//!< Cached name of last animation used in Sync method

public:
	CItemAnimationSyncToken();
	virtual void Sync( CName animationName, const CSyncInfo& syncInfo, Float weight ) override;
	virtual void Reset() override;
	virtual Bool IsValid() const override;
};

BEGIN_CLASS_RTTI( CItemAnimationSyncToken );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SItemChangedData
{
	DECLARE_RTTI_STRUCT( SItemChangedData )
	SItemChangedData() : m_quantity( 0 ), m_informGui( false )
	{}
	SItemChangedData( CName itemName, Int32 quantity, TDynArray< SItemUniqueId >& arr, Bool informGui ) 
		: m_itemName( itemName ), m_quantity( quantity ), m_informGui( informGui ), m_ids( arr ) 
	{}
	CName						m_itemName;
	Int32						m_quantity;	
	Bool						m_informGui;
	TDynArray< SItemUniqueId > m_ids;
};

BEGIN_CLASS_RTTI( SItemChangedData );
	PROPERTY( m_itemName );
	PROPERTY( m_quantity );
	PROPERTY( m_ids );
	PROPERTY( m_informGui );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EInventoryEventType
{
	IET_Empty,
	IET_ItemAdded,
	IET_ItemRemoved,
	IET_ItemQuantityChanged,
	IET_ItemTagChanged,
	IET_InventoryRebalanced,
};

BEGIN_ENUM_RTTI( EInventoryEventType )
	ENUM_OPTION( IET_Empty );
	ENUM_OPTION( IET_ItemAdded );
	ENUM_OPTION( IET_ItemRemoved );
	ENUM_OPTION( IET_ItemQuantityChanged );
	ENUM_OPTION( IET_ItemTagChanged );
	ENUM_OPTION( IET_InventoryRebalanced );
END_ENUM_RTTI()

//////////////////////////////////////////////////////////////////////////

class IInventoryListener
{
public:

	virtual void OnInventoryEvent( CInventoryComponent* inventory,	// inventory object
								   EInventoryEventType eventType,	// event type
								   SItemUniqueId itemId,			// item id, or INVALID for 'remove' or events from associated inventory
								   Int32 quantity,					// quantity of item ( positive for Add, negative for Remove, pos. or neg. for QuantityChange
								   Bool fromAssociatedInventory		// event was called from associated inventory
							      ) = 0;
};

//////////////////////////////////////////////////////////////////////////

class CInventoryComponent	: public CComponent
							, public ILODable
{
	DECLARE_ENGINE_CLASS( CInventoryComponent, CComponent, 0 );
	
	friend class ActorActionWork;

public:
	struct SAddItemInfo
	{
		Uint32	m_quantity;

		Bool	m_informGui				: 1;
		Bool	m_markAsNew				: 1;
		Bool	m_isLootable			: 1;
		Bool	m_shouldBeRebalanced	: 1;

		SAddItemInfo()
			: m_quantity( 1 )
			, m_informGui( true )
			, m_markAsNew( false )
			, m_isLootable( false )
			, m_shouldBeRebalanced( false )
		{}
	};

	struct SMountItemInfo
	{
		Bool				m_toHand;			//!< Is it a hand mount
		Bool				m_force;			//!< Should it be a forced mount ( ignore other items of this category already mount )
		Bool				m_forceOverwritingCurrentItem;	//!< Mount this item even when default or the same as current (but using different template)
		CName				m_slotOverride;		//!< Specific slot, overrides slots specified in item definition
		CItemEntityProxy*	m_proxy;

		SMountItemInfo()
			: m_toHand( false )
			, m_force( false )
			, m_forceOverwritingCurrentItem( false )
			, m_proxy( nullptr )
		{}
	};

	struct SFindItemInfo
	{
		CName	m_category;
		Bool	m_mountOnly;
		Bool	m_ignoreDefaultItem;
		Bool	m_eligibleToMount;
		CName	m_holdSlot;
		CName	m_itemTag;
		CName	m_withoutTag;

		SFindItemInfo()
			: m_mountOnly( false )
			, m_ignoreDefaultItem( false )
			, m_eligibleToMount( false )
		{}
	};

public:
	//! Get items
	RED_INLINE const TDynArray< SInventoryItem, MC_Inventory >& GetItems() const { return m_items; }

protected:
	TDynArray< SInventoryItem, MC_Inventory >		m_items;			//!< Inventory items
	TDynArray< CName, MC_Inventory > m_failedLoadItemsToMount; //!< Category of items with mount flag and unsuccessful load form save (used for DLC failsafe support)
	THandle< CEntityTemplate >		m_containerTemplate;//!< Template for creating entity with thrown away items
	SItemUniqueIdGenerator			m_uniqueIdGenerator;//!< Generator of unique ids (unique across inventory component)

	CLootCache						m_loot;
	
	static const Uint32				MAX_ITEM_EXTENSION_SLOTS;	//!< Maximum number of slot items an item can wear

	TDynArray< IInventoryListener* >	m_listeners;
	Bool							m_suppressEvents;
	Bool							m_notifyScriptedListeners;
	Bool							m_turnOffSpawnItemsBudgeting;
	Uint32							m_itemsStateHash;

	Uint32							m_nextRebalance;
	Uint32							m_rebalanceEveryNSeconds;

	THashMap< CName, TDynArray< SItemUniqueId > >	m_categoryDependentItems;
	THashMap< CName, TDynArray< SItemUniqueId > >	m_itemDependentItems;

	THandle< CInventoryComponent >	m_associatedInventory;

	typedef THashMap< SItemUniqueId, CItemEntityProxy* >	TDroppedProxies;
	TDroppedProxies					m_droppedProxies;	//! Short memory of dropped proxies
	static const Uint32 MAX_DROPPED_PROXIES = 2;
public:
	static const Int32				INVALID_INDEX = -1;	//!< Invalid item index

	CInventoryComponent();
	~CInventoryComponent();
	
	virtual Bool CheckShouldSave() const;

	//! Property is going to be changed 
	virtual void OnPropertyPreChange( IProperty* property );
	//! Property was changed
	virtual void OnPropertyPostChange( IProperty* property );

	virtual Bool CanBeSavedDirectlyToStream() const override { return true; }
	virtual void StreamLoad( ISaveFile* loader, Uint32 version ) override;
	virtual void StreamSave( ISaveFile* saver ) override;
	// Called when component is attached to world
	virtual void OnAttached( CWorld* world ) override;
	// Called when component is detached from world ( layer gets hidden, etc )
	virtual void OnDetached( CWorld* world ) override;
	//! Called after entity this component is in was loaded from a layer file, before OnAttach is called and before entity state is restored from save game
	virtual void OnEntityLoaded();
	// Called when component is about to be destroyed ( usually called in entity template editor )
	virtual void OnDestroyed();

	virtual bool UsesAutoUpdateTransform() override { return false; }

	virtual void OnAppearanceChanged( Bool added );

	//! Check given unique id validity for this inventory
	Bool IsIdValid( SItemUniqueId itemId ) const;
	//! If there is specified item in inventory
	Bool HasItem( CName itemName ) const;
	//! Find item based on FindItem info
	SItemUniqueId FindItem( const SFindItemInfo& info ) const;
	//! Sorts the items
	void SortItems();
	//! Clear inventory ( holsters items if needed )
	void ClearInventory();
	//! Print contents of inventory
	void PrintInfo();
	//! Export to xml
	void Export( CXMLWriter& writer ) const;

	//! Drop item
	void DropItem( SItemUniqueId itemId, Bool removeFromInv = false, Float duration = -1.0f );

	//! Get proxy for item that was dropped
	CItemEntityProxy* GetDroppedProxy( SItemUniqueId itemId ) const;

	//! Remove item with specified index from inventory
	Bool RemoveItem( SItemUniqueId itemId, Int32 quantity = 1 );
	//! Remove all items from inventory
	void RemoveAllItems();

	//! Destroy item's entity
	void DespawnItem( SItemUniqueId itemId );
	
	//! Despawn all items
	void DespawnAllItems();

	//! Despawn items with owned proxy
	void DespawnOwnedItems();

	//! Holster and unmount all items if any
	void HideAllItems();
	void HideAllItemsRaw();
	void SetItemVisible( SItemUniqueId item, Bool flag );

	//! Throw away item with specified id
	CEntity* ThrowAwayItem( SItemUniqueId itemId, Uint32 quantity = 1 );
	//! Throw away items with given ids
	CEntity* ThrowAwayItems( const TDynArray< Int32 >& indices );
	//! Throw away all items, returns entity created
	CEntity* ThrowAwayAllItems();
	//! Throw away items, excluding those with any of given tags, returns entity created
	CEntity* ThrowAwayItemsFiltered( const TDynArray< CName >& excludedTags );
	//! Throw away items for loot
	CEntity* ThrowAwayLootableItems( Bool skipNoDropNoShop = false );

	//! Add specified item to inventory
	TDynArray< SItemUniqueId > AddItem( CName itemName, const SAddItemInfo& addItemInfo = SAddItemInfo() );
	//! Add fully defined item, adds item to stack if there is a match
	TDynArray< SItemUniqueId > AddItem( const SInventoryItem& item, const SAddItemInfo& addItemInfo = SAddItemInfo() );

	//! Transfer one item to other inventory ( holsters items if needed )
	TDynArray< SItemUniqueId > GiveItem( CInventoryComponent* otherInventory, SItemUniqueId id, Int32 quantity = 1 );
	//! Transfer many items to other inventory ( holsters items if needed )
	Bool GiveItems( CInventoryComponent* otherInventory, const TDynArray< Int32 >& indices );


	SItemUniqueId SplitItem( SItemUniqueId itemId, Int32 qty );

	//! Get item with given entity
	SInventoryItem* GetItem( CItemEntity * itemProxy );
	//! Get item with unique ItemId
	SInventoryItem* GetItem( SItemUniqueId itemId );
	//! Get item with unique itemId
	const SInventoryItem* GetItem( SItemUniqueId itemId ) const;

	//! Get id of first item with given name, returns SItemUniqueId::INVALID if not found
	SItemUniqueId GetItemId( CName itemName ) const;
	//! Get id of the equal item, returns SItemUniqueId::INVALID if not found
	SItemUniqueId GetItemId( const SInventoryItem& item ) const;

	//! Get number of items
	Uint32 GetItemCount( Bool useAssociatedInventory = false );
	//! Get item quantity by name
	Uint32 GetItemQuantityByName( CName itemName, Bool useAssociatedInventory = false, TDynArray< CName > * ignoreTags = nullptr );
	//! Get item quantity by category
	Uint32 GetItemQuantityByCategory( CName itemCategory, Bool useAssociatedInventory = false, TDynArray< CName > * ignoreTags = nullptr  );
	//! Get item quantity by tag
	Uint32 GetItemQuantityByTag( CName itemTag, Bool useAssociatedInventory = false, TDynArray< CName > * ignoreTags = nullptr  );
	//! Get item quantity by index
	Uint32 GetItemQuantity( SItemUniqueId itemIndex ) const;
	//! Get sum of all items quantity
	Uint32 GetAllItemsQuantity( Bool useAssociatedInventory = false, TDynArray< CName > * ignoreTags = nullptr );
	//! Get mount or held items that suits given category
	void GetItemsByCategory( CName category, TDynArray< SItemUniqueId > & items );
	//! Get mount or held item that suits given category
	SItemUniqueId GetItemByCategory( CName category, Bool mountOnly = true, Bool ignoreDefaultItem = false ) const;

	//Necesary to unify how scenes and cutscenes find weapon 
	SItemUniqueId GetItemByCategoryForScene( CName category, CName ignoreTag = CName::NONE ) const;

	//! Get all item abilities
	void GetItemAbilities( SItemUniqueId itemId, TDynArray< CName >& abilities ) const;

	//! Add tag to item
	Bool AddItemTag( SItemUniqueId itemId, CName tag );
	//! Remove tag from item
	Bool RemoveItemTag( SItemUniqueId itemId, CName tag );
	//! Get tags of given item, returns false if id is not valid
	Bool GetItemTags( SItemUniqueId itemId, TDynArray< CName >& tags ) const;
	//! Returns modifier associated with specified item tag.
	const SItemTagModifier* GetItemTagModifier( CName itemTag ) const;
	//! Check if the item has given tag
	Bool ItemHasTag( SItemUniqueId itemId, CName tag ) const;
	//! Checks if enhancement item has a given tag
	Bool HasEnhancementItemTag( SItemUniqueId enhancedItemId, Uint32 itemSlotIndex, CName tag ) const;
	
	//! Activates Quest Bonus associated with this inventory component.
	void ActivateQuestBonus( void );

	//! Returns item weight based on item definition, if none provided it calculates based on abilities.
	Float GetItemWeight( const SInventoryItem& invItem ) const;
	//! Returns item weight based on item definition, if none provided it calculates based on abilities.
	Float GetItemWeight( SItemUniqueId itemId ) const;

	//! Returns dynamic price based on item abilities.
	Int32 GetItemPrice( const SInventoryItem& invItem ) const;
	//! Returns dynamic price based on item abilities.
	Int32 GetItemPrice( SItemUniqueId itemId ) const;

	//! Returns dynamic price based on player location, merchant type, item category.
	Int32 GetItemPriceModified( const SInventoryItem& invItem, Bool playerSellingItem ) const;
	//! Returns dynamic price based on player location, merchant type, item category.
	Int32 GetItemPriceModified( SItemUniqueId itemId, Bool playerSellingItem ) const;

	//! Accumulates statValues based on name.
	void AccumulateItemStat( TDynArray< SItemStat >& itemStats, CName statType, Float statWeight ) const;
	//! Calculates list of itemStats based on given inventory item.
	void AccumulateItemStats( const SInventoryItem* invItem, TDynArray< SItemStat >& itemStats ) const;
	//! Returns the combined value of all items stats.
	Float TotalItemStats( const SInventoryItem& invItem ) const;

	//! Returns modifier used to differentiate amount of inventory funds.
	Float GetFundsModifier( void ) const;

	Bool AddSlot( SInventoryItem& invItem ) const;
	Uint8 GetSlotItemsLimit( SInventoryItem& invItem ) const;

	//! Returns the weight of the given modifier based on component tags.
	Float CalcItemTagModifierWeight( CName tagModifier ) const;

	//! Generates price per point of repair and total cost of repair for given item.
	void GetItemPriceRepair( const SInventoryItem& invItem, Int32& costRepairPoint, Int32& costRepairTotal ) const;
	//! Returns cost of removing an upgrade from given item.
	Int32 GetItemPriceRemoveUpgrade( const SInventoryItem& invItem ) const;
	//! Returns cost of disassembling a given item.
	Int32 GetItemPriceDisassemble( const SInventoryItem& invItem ) const;

	//! Returns cost of adding a slot to a given item.
	Int32 GetItemPriceAddSlot( const SInventoryItem& invItem ) const;

	//! Returns cost of adding a slot to a given item.
	Int32 GetItemPriceCrafting( const SInventoryItem& invItem ) const;

	//! Returns cost of removing an enchantment on a given item.
	Int32 GetItemPriceRemoveEnchantment( const SInventoryItem& invItem ) const;
	
	//! Returns cost of adding an enchantment to a given item.
	Int32 GetItemPriceEnchantItem( const SInventoryItem& invItem ) const;

	//! Get item durability
	Float GetItemDurability( SItemUniqueId itemId ) const;
	//! Get item initial durability
	Float GetItemInitialDurability( SItemUniqueId itemId ) const;
	//! Get item max durability
	Float GetItemMaxDurability( SItemUniqueId itemId ) const;
	//! Returns item's value for durability at 100%.
	Float GetItemMaxDurability( const SInventoryItem& invItem ) const;
	//! Returns ratio of durability/max durability
	Float GetItemDurabilityPercentage( const SInventoryItem& invItem ) const;
	//! Set item durability
	void SetItemDurability( SItemUniqueId itemId, Float durability );
	//! Check if item has durability
	Bool HasItemDurability( SItemUniqueId itemId ) const;
	//! Reduces the durability of a lootable item using pre-defined percentage range.
	void ReduceLootableItemDurability( SInventoryItem& item );

	//! Fill array with indices of items
	Bool FillIndices( TDynArray< Int32 >& outIndices ) const;
	//! Perform comparison considering randomized attribute modifiers
	Bool CompareItems( const SInventoryItem* itemA, const SInventoryItem* itemB ) const;
	//! Get item grid size
	Uint32 GetItemGridSize( SItemUniqueId itemId ) const;
	//! Is item a weapon?
	Bool IsWeapon( SItemUniqueId itemId ) const;
	//is any weapon hold in any slot
	Bool IsWeaponHeld() const;
	//! Get ingredients of the item
	CName GetCraftedItemName( SItemUniqueId itemId ) const;

	//! Add fake cloned item from ap
	SItemUniqueId AddFakeItem( CName itemName, CItemEntityProxy* proxy );
	//! Remove fake item
	void RemoveFakeItem( SItemUniqueId itemId );

	Bool SetPreviewColor( SItemUniqueId itemId, Uint32 colorId );
	Bool ClearPreviewColor( SItemUniqueId itemId );

	Bool ColorItem( SItemUniqueId itemId, SItemUniqueId dyeId );
	Bool ClearItemColor( SItemUniqueId itemId );

	//! Clears slot items and adds enchantment to item
	Bool EnchantItem( SItemUniqueId itemId, CName enchantmentName, CName enchantmentStat );
	//! Removes enchantment from item
	Bool UnenchantItem( SItemUniqueId itemId );

	//! Attaches extension item to enhanced item's slot items list, and removes extension item from inventory (returns false if no empty slots)
	Bool EnhanceItem( SItemUniqueId enhancedItemId, SItemUniqueId extensionItemId );
	//! Remove extension item from item, and add it to inventory, returns false if no such item in slot
	Bool RemoveEnhancementItem( SItemUniqueId enhancedItemId, CName extensionItemName );
	//! Remove extension item from item, and add it to inventory, returns false if no such item in slot
	Bool RemoveEnhancementItem( SItemUniqueId enhancedItemId, Uint32 itemSlotIndex );

	//! Get equip slot name
	CName GetItemEquipSlotName( SItemUniqueId itemId );
	//! Spawns all entities marked as 'mounted' or 'held' (makes them visible)
	void SpawnMountedItems();
	//! Spawns item if it has a 'mounted' or 'held' flag
	void SpawnItemIfMounted( SItemUniqueId itemId );

	//! Attach item in a way specified by the item itself
	Bool MountItem( SItemUniqueId itemId, const SMountItemInfo& info );
	//! Detach item from slot bone
	Bool UnMountItem( SItemUniqueId itemId, Bool destroyEntity = true, Bool emptyHand = true );
	//! Returns true if all mounted items are spawned
	Bool AreAllMountedItemsSpawned();
	//! Returns true if specified item has been mounted
	Bool IsItemMounted( CName itemName ) const;
	Bool IsAnyItemMounted( CName itemName ) const;
	Bool IsItemMounted( SItemUniqueId id ) const;
	Bool IsItemHeld( CName itemName ) const;
	Bool IsItemHeld( SItemUniqueId id ) const;

	//! Draw item
	Bool DrawItem( SItemUniqueId itemId, Bool instant = false, Bool sendEvents = true );
	//! Holster item
	Bool HolsterItem( SItemUniqueId itemId, Bool instant = false, Bool sendEvents = true );
	//! Get item in hold slot
	SItemUniqueId GetItemIdHeldInSlot( CName holdSlot );

	//! Draw weapon and attack
	Bool DrawWeaponAndAttack( SItemUniqueId itemId );
	//! Animate item
	void PlayItemAnimation( SItemUniqueId itemId, CName animationName );
	//! Stop animating item
	void StopItemAnimation( SItemUniqueId itemId );

	//! Raise behavior event on item
	void RaiseItemBehaviorEvent( SItemUniqueId itemId, CName eventName );

	//! Play effect on item
	void PlayItemEffect( SItemUniqueId itemId, CName effectName );
	//! Play effect on item
	void StopItemEffect( SItemUniqueId itemId, CName effectName );

	//! Try to get the item entity, if it is not yet spawned, you get NULL. Just don't use it if it is not absolutely necessary :)
	CItemEntity* GetItemEntityUnsafe( SItemUniqueId itemId );
	//! Spawn item's deployment entity instantly
	CEntity* GetDeploymentItemEntity( SItemUniqueId itemId, const Vector& position, const EulerAngles& rotation, Bool allocateIdTag = false );

	//! Get the UI-specific data for the item
	const SInventoryItemUIData &GetInventoryItemUIData( const SItemUniqueId& id ) const;
	//! Set the UI-specific data for the item
	void SetInventoryItemUIData( const SItemUniqueId& id, const SInventoryItemUIData &data );
	//! Get default item for category
	CName GetCategoryDefaultItem( CName category ) const;

	// Collect animation synchronization tokens
	void OnCollectAnimationSyncTokens( CName animationName, TDynArray< CAnimationSyncToken* >& tokens ) const;
	Bool TryToLoadSyncToken( const SInventoryItem* item, CName animationName, CAnimatedComponent*& outItemAnimatedComponent ) const;

	SInventoryItem* GetItemHeldInSlot( CName holdSlot );

	Uint32 GetItemsStateHash() const { return m_itemsStateHash; }
	static Uint32 GetInvalidStateHash() { return 0xffffffff; }
private:
	void MarkItemsListChanged() { ++m_itemsStateHash; }

	Bool IsPlayerOwner() const;

	//! Holster or unmount item
	void HideItem( Int32 itemIndex );

	//! Spawns item if it has a 'mounted' or 'held' flag
	void SpawnItemIfMounted( Int32 itemIndex );

	//! Spawn items bound to specified item
	void HelperMountBoundItems( const SItemDefinition* itemDef );
	void HelperUnmountBoundItems( const SItemDefinition* itemDef );

	// register given item to be updated (its template possibly changed) when item with given name/category will be (un)mounted
	void RegisterDependency( SItemUniqueId itemId, const SItemDefinition& itemDef );
	// unregister given item 
	void UnregisterDependency( SItemUniqueId itemId, const SItemDefinition& itemDef );
	void UnregisterDependency( SItemUniqueId itemId, const TDynArray< CName >& itemList, THashMap< CName, TDynArray< SItemUniqueId > >& dependencyList );

	// update depending items
	virtual void OnAffectingItemChanged( CName itemName, CName category );
	// update depending items (check they use right template)
	void UpdateDependingItems( const TDynArray< SItemUniqueId >& list );

	//! Check if item index is valid
	RED_INLINE Bool IsIndexValid( Int32 idx ) const
	{
		return idx >= 0 && idx < (Int32)m_items.Size();
	}

	//! Get array index for item with specified uniqueID
	Int32 UniqueIdToIndex( SItemUniqueId itemId ) const;

	//! Get array index for the first item with specified name
	Int32 NameToIndex( CName itemName ) const;

	Bool DoesBehaviorResponse( const CName& eventName ) const;

	void AddItemsFromTemplate( const CEntityTemplate* temp );

	//! Check all weapons marked as mounted or held, and if they're not equipped, "unmark" them.
	//! This will work only when called right after gameplay state load and before actual entities spawn.
	void FixMountedUnequippedWeapons();

public:
	void BalanceItemsWithPlayerLevel( Uint32 playerLevel );

protected:
	// ILODable
	void UpdateLOD( ILODable::LOD newLOD, CLODableManager* manager ) override;
	ILODable::LOD ComputeLOD( CLODableManager* manager ) const override;

public:
	Bool OnGrabItem( SItemUniqueId itemId );
	Bool OnPutItem( SItemUniqueId itemId );

	Bool AddListener( IInventoryListener* listener );
	Bool RemoveListener( IInventoryListener* listener );
	void NotifyListeners( EInventoryEventType eventType, SItemUniqueId id, Int32 quantity, Bool fromAssociatedInventory );
	
	void OnItemAdded( SItemChangedData& data );
	void OnItemRemoved( SItemUniqueId itemId, Uint32 quantity );

	void InitFromTemplate( const CEntityTemplate* temp );

	// Gets equip template - checks if item variant template should be used
	const String& GetTemplate( CName itemName ) const;

	Bool ShouldCollapse( CName itemName ) const;

	//! Get associated inventory
	CInventoryComponent* GetAssociatedInventory();

	Bool WasItemLooted( Uint32 itemID ) const;
	void NotifyItemLooted( Uint32 itemID, Bool looted = true );
	void NotifyQuestItemLooted( Uint32 itemID );
	void ResetContainerData();
	Uint32 GenerateSeed() const;

	Bool IsAContainer() const;

protected:
	// ------------------------------------------------------------------------
	// Scripting support
	// ------------------------------------------------------------------------
	void funcGetItemsNames( CScriptStackFrame& stack, void* result );
	void funcGetItemCount( CScriptStackFrame& stack, void* result );
	void funcGetItemName( CScriptStackFrame& stack, void* result );
	void funcGetItemCategory( CScriptStackFrame& stack, void* result );
	void funcGetItemClass( CScriptStackFrame& stack, void* result );
	void funcGetItemTags( CScriptStackFrame& stack, void* result );
	void funcItemHasTag( CScriptStackFrame& stack, void* result );
	void funcAddItemTag( CScriptStackFrame& stack, void* result );
	void funcRemoveItemTag( CScriptStackFrame& stack, void* result );
	void funcGetItemByItemEntity( CScriptStackFrame& stack, void* result ); 
	void funcGetItemQuantity( CScriptStackFrame& stack, void* result );
	void funcGetItemQuantityByName( CScriptStackFrame& stack, void* result );
	void funcGetItemQuantityByCategory( CScriptStackFrame& stack, void* result );
	void funcGetItemQuantityByTag( CScriptStackFrame& stack, void* result );
	void funcGetAllItemsQuantity( CScriptStackFrame& stack, void* result );
	void funcGetItemBaseAttributes( CScriptStackFrame& stack, void* result );
	void funcGetItemAttributes( CScriptStackFrame& stack, void* result );
	void funcGetItemAbilities( CScriptStackFrame& stack, void* result );
	void funcGetItemContainedAbilities( CScriptStackFrame& stack, void* result );
	void funcGiveItem( CScriptStackFrame& stack, void* result );
	void funcHasItem( CScriptStackFrame& stack, void* result );
    void funcAddMultiItem( CScriptStackFrame& stack, void* result );
    void funcAddSingleItem( CScriptStackFrame& stack, void* result );
	void funcRemoveItem( CScriptStackFrame& stack, void* result );
	void funcGetItemEntityUnsafe( CScriptStackFrame& stack, void* result );
	void funcMountItem( CScriptStackFrame& stack, void* result );
	void funcUnmountItem( CScriptStackFrame& stack, void* result );
	void funcThrowAwayItem( CScriptStackFrame& stack, void* result );
	void funcThrowAwayAllItems( CScriptStackFrame& stack, void* result );
	void funcThrowAwayItemsFiltered( CScriptStackFrame& stack, void* result );
	void funcThrowAwayLootableItems( CScriptStackFrame& stack, void* result );
	void funcPrintInfo( CScriptStackFrame& stack, void* result );
	void funcDespawnItem( CScriptStackFrame& stack, void* result );
	void funcGetAllItems( CScriptStackFrame& stack, void* result );
	void funcGetItemId( CScriptStackFrame& stack, void* result );
	void funcGetItemsIds( CScriptStackFrame& stack, void* result );
	void funcGetItemsByTag( CScriptStackFrame& stack, void* result );
	void funcGetItemsByCategory( CScriptStackFrame& stack, void* result );
	void funcIsIdValid( CScriptStackFrame& stack, void* result );
	void funcGetItemEnhancementSlotsCount( CScriptStackFrame& stack, void* result );
	void funcGetItemEnhancementItems( CScriptStackFrame& stack, void* result );
	void funcGetItemEnhancementCount( CScriptStackFrame& stack, void* result );
	void funcHasEnhancementItemTag( CScriptStackFrame& stack, void* result );
	void funcGetItemColor( CScriptStackFrame& stack, void* result );
	void funcIsItemColored( CScriptStackFrame& stack, void* result );
	void funcSetPreviewColor( CScriptStackFrame& stack, void* result );
	void funcClearPreviewColor( CScriptStackFrame& stack, void* result );
	void funcColorItem( CScriptStackFrame& stack, void* result );
	void funcClearItemColor( CScriptStackFrame& stack, void* result );
	void funcGetEnchantment( CScriptStackFrame& stack, void* result );
	void funcIsItemEnchanted( CScriptStackFrame& stack, void* result );
	void funcEnchantItem( CScriptStackFrame& stack, void* result );
	void funcUnenchantItem( CScriptStackFrame& stack, void* result );
	void funcEnhanceItem( CScriptStackFrame& stack, void* result );
	void funcRemoveItemEnhancementByIndex( CScriptStackFrame& stack, void* result );
	void funcRemoveItemEnhancementByName( CScriptStackFrame& stack, void* result );
	void funcGetCraftedItemName( CScriptStackFrame& stack, void* result );
	void funcIsItemMounted( CScriptStackFrame& stack, void* result );
	void funcIsItemHeld( CScriptStackFrame& stack, void* result );
	void funcGetDeploymentItemEntity( CScriptStackFrame& stack, void* result );
	void funcPlayItemEffect( CScriptStackFrame& stack, void* result );
	void funcStopItemEffect( CScriptStackFrame& stack, void* result );
	void funcDropItem( CScriptStackFrame& stack, void* result ); 
	void funcGetItemHoldSlot( CScriptStackFrame& stack, void* result ); 
	void funcEnableLoot( CScriptStackFrame& stack, void* result );
	void funcUpdateLoot( CScriptStackFrame& stack, void* result );
	void funcAddItemsFromLootDefinition( CScriptStackFrame& stack, void* result );
	void funcIsLootRenewable( CScriptStackFrame& stack, void* result );
	void funcIsReadyToRenew( CScriptStackFrame& stack, void* result );
	void funcRemoveAllItems( CScriptStackFrame& stack, void* result );
    void funcGetSchematicIngredients( CScriptStackFrame& stack, void* result );
    void funcGetSchematicRequiredCraftsmanType( CScriptStackFrame& stack, void* result );
	void funcGetSchematicRequiredCraftsmanLevel( CScriptStackFrame& stack, void* result );
	void funcGetInventoryItemUIData( CScriptStackFrame& stack, void* result );
	void funcSetInventoryItemUIData( CScriptStackFrame& stack, void* result );
	void funcSortInventoryUIData( CScriptStackFrame& stack, void* result );
	void funcGetItemRecyclingParts( CScriptStackFrame& stack, void* result );
	void funcGetItemSetName( CScriptStackFrame& stack, void* result );
	void funcGetItemGridSize( CScriptStackFrame& stack, void* result );
	void funcSplitItem( CScriptStackFrame& stack, void* result );
	void funcSetItemStackable( CScriptStackFrame& stack, void* result );
	void funcGetCategoryDefaultItem( CScriptStackFrame& stack, void* result );

	void funcHasItemDurability( CScriptStackFrame& stack, void* result );
	void funcGetItemDurability( CScriptStackFrame& stack, void* result );
	void funcSetItemDurability( CScriptStackFrame& stack, void* result );
	void funcGetItemInitialDurability( CScriptStackFrame& stack, void* result );
	void funcGetItemMaxDurability( CScriptStackFrame& stack, void* result );

	void funcActivateQuestBonus( CScriptStackFrame& stack, void* result );

	void funcGetItem( CScriptStackFrame& stack, void* result );
	void funcGetFundsModifier( CScriptStackFrame& stack, void* result );
	void funcAddSlot( CScriptStackFrame& stack, void* result );
	void funcGetSlotItemsLimit( CScriptStackFrame& stack, void* result );
	void funcGetItemPriceAddSlot( CScriptStackFrame& stack, void* result );
	void funcGetItemPriceCrafting( CScriptStackFrame& stack, void* result );
	void funcGetItemPriceRemoveEnchantment( CScriptStackFrame& stack, void* result );
	void funcGetItemPriceEnchantItem( CScriptStackFrame& stack, void* result );
	void funcGetItemPriceDisassemble( CScriptStackFrame& stack, void* result );
	void funcGetItemPriceRemoveUpgrade( CScriptStackFrame& stack, void* result );
	void funcGetItemPriceRepair( CScriptStackFrame& stack, void* result );
	void funcGetItemPriceModified( CScriptStackFrame& stack, void* result );

	void funcGetInventoryItemPriceModified( CScriptStackFrame& stack, void* result );
	void funcGetItemPrice( CScriptStackFrame& stack, void* result );
	void funcGetItemWeight( CScriptStackFrame& stack, void* result );
	void funcTotalItemStats( CScriptStackFrame& stack, void* result );

    void funcGetItemLocalizedNameByUniqueID( CScriptStackFrame& stack, void* result );
    void funcGetItemLocalizedDescriptionByUniqueID( CScriptStackFrame& stack, void* result );

    void funcGetItemLocalizedNameByName( CScriptStackFrame& stack, void* result );
    void funcGetItemLocalizedDescriptionByName( CScriptStackFrame& stack, void* result );

    void funcGetItemIconPathByUniqueID( CScriptStackFrame& stack, void* result );
    void funcGetItemIconPathByName( CScriptStackFrame& stack, void* result );

    void funcGetNumOfStackedItems( CScriptStackFrame& stack, void* result );

	void funcGetItemAttributeValue( CScriptStackFrame& stack, void* result );
	void funcGetItemAbilityAttributeValue( CScriptStackFrame& stack, void* result );	
	void funcAddItemCraftedAbility( CScriptStackFrame& stack, void* result );
	void funcRemoveItemCraftedAbility( CScriptStackFrame& stack, void* result );

	void funcAddItemBaseAbility( CScriptStackFrame& stack, void* result );
	void funcRemoveItemBaseAbility( CScriptStackFrame& stack, void* result );
	
	void funcPreviewItemAttributeAfterUpgrade( CScriptStackFrame& stack, void* result );
	void funcGetItemFromSlot( CScriptStackFrame& stack, void* result );
	void funcGetItemModifierFloat( CScriptStackFrame& stack, void* result );
	void funcSetItemModifierFloat( CScriptStackFrame& stack, void* result );
	void funcGetItemModifierInt( CScriptStackFrame& stack, void* result );
	void funcSetItemModifierInt( CScriptStackFrame& stack, void* result );

	void funcNotifyScriptedListeners( CScriptStackFrame& stack, void* result );
	void funcInitInvFromTemplate( CScriptStackFrame& stack, void* result );

	void funcBalanceItemsWithPlayerLevel( CScriptStackFrame& stack, void* result );

	void funcNotifyItemLooted( CScriptStackFrame& stack, void* result );
	void funcResetContainerData( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CInventoryComponent )
	PARENT_CLASS( CComponent )
	//PROPERTY_EDIT( m_items, TXT( "Inventory items" ) );
	PROPERTY_EDIT( m_containerTemplate, TXT( "Entity created when throwing away items" ) );
	PROPERTY_EDIT( m_rebalanceEveryNSeconds, TXT( "Min time to next rebalance to player level, if 0 - then this is never rebalanced." ) );
	PROPERTY_EDIT( m_turnOffSpawnItemsBudgeting, TXT( "Turn off spawn items budgeting system." ) );
	NATIVE_FUNCTION( "GetItemModifierFloat", funcGetItemModifierFloat );
	NATIVE_FUNCTION( "SetItemModifierFloat", funcSetItemModifierFloat );
	NATIVE_FUNCTION( "GetItemModifierInt", funcGetItemModifierInt );
	NATIVE_FUNCTION( "SetItemModifierInt", funcSetItemModifierInt );
	NATIVE_FUNCTION( "GetItemFromSlot", funcGetItemFromSlot);
	NATIVE_FUNCTION( "GetItemsNames", funcGetItemsNames );
	NATIVE_FUNCTION( "GetItemCount", funcGetItemCount );
	NATIVE_FUNCTION( "GetItemName", funcGetItemName );
	NATIVE_FUNCTION( "GetItemCategory", funcGetItemCategory );
	NATIVE_FUNCTION( "GetItemClass", funcGetItemClass );
	NATIVE_FUNCTION( "ItemHasTag", funcItemHasTag );
	NATIVE_FUNCTION( "GetItemTags", funcGetItemTags );
	NATIVE_FUNCTION( "AddItemTag", funcAddItemTag );
	NATIVE_FUNCTION( "RemoveItemTag", funcRemoveItemTag );
	NATIVE_FUNCTION( "GetItemByItemEntity", funcGetItemByItemEntity );
	
	NATIVE_FUNCTION( "GetItemQuantity", funcGetItemQuantity );
	NATIVE_FUNCTION( "GetItemQuantityByName", funcGetItemQuantityByName );
	NATIVE_FUNCTION( "GetItemQuantityByCategory", funcGetItemQuantityByCategory );
	NATIVE_FUNCTION( "GetItemQuantityByTag", funcGetItemQuantityByTag );
	NATIVE_FUNCTION( "GetAllItemsQuantity", funcGetAllItemsQuantity );
	NATIVE_FUNCTION( "GetItemEntityUnsafe", funcGetItemEntityUnsafe );
	NATIVE_FUNCTION( "GetItemBaseAttributes", funcGetItemBaseAttributes );
	NATIVE_FUNCTION( "GetItemAttributes", funcGetItemAttributes );
	NATIVE_FUNCTION( "GetItemAttributeValue", funcGetItemAttributeValue );
	NATIVE_FUNCTION( "GetItemAbilityAttributeValue", funcGetItemAbilityAttributeValue );
	NATIVE_FUNCTION( "GetItemAbilities", funcGetItemAbilities );
	NATIVE_FUNCTION( "GetItemContainedAbilities", funcGetItemContainedAbilities );
	NATIVE_FUNCTION( "GiveItem", funcGiveItem );
	NATIVE_FUNCTION( "HasItem", funcHasItem );
    NATIVE_FUNCTION( "AddMultiItem", funcAddMultiItem );
    NATIVE_FUNCTION( "AddSingleItem", funcAddSingleItem );
	NATIVE_FUNCTION( "RemoveItem", funcRemoveItem );
	NATIVE_FUNCTION( "MountItem", funcMountItem );
	NATIVE_FUNCTION( "UnmountItem", funcUnmountItem );
	NATIVE_FUNCTION( "ThrowAwayItem", funcThrowAwayItem );
	NATIVE_FUNCTION( "ThrowAwayAllItems", funcThrowAwayAllItems );
	NATIVE_FUNCTION( "ThrowAwayItemsFiltered", funcThrowAwayItemsFiltered );
	NATIVE_FUNCTION( "ThrowAwayLootableItems", funcThrowAwayLootableItems );
	NATIVE_FUNCTION( "PrintInfo", funcPrintInfo );
	NATIVE_FUNCTION( "DespawnItem", funcDespawnItem );
	NATIVE_FUNCTION( "GetAllItems", funcGetAllItems );
	NATIVE_FUNCTION( "GetItemId", funcGetItemId );
	NATIVE_FUNCTION( "GetItemsIds", funcGetItemsIds );
	NATIVE_FUNCTION( "GetItemsByTag", funcGetItemsByTag );
	NATIVE_FUNCTION( "GetItemsByCategory", funcGetItemsByCategory );
	NATIVE_FUNCTION( "IsIdValid", funcIsIdValid );
	NATIVE_FUNCTION( "GetItemEnhancementSlotsCount", funcGetItemEnhancementSlotsCount );
	NATIVE_FUNCTION( "GetItemEnhancementItems", funcGetItemEnhancementItems );
	NATIVE_FUNCTION( "GetItemEnhancementCount", funcGetItemEnhancementCount );
	NATIVE_FUNCTION( "HasEnhancementItemTag", funcHasEnhancementItemTag );
	NATIVE_FUNCTION( "GetItemColor", funcGetItemColor );
	NATIVE_FUNCTION( "IsItemColored", funcIsItemColored );
	NATIVE_FUNCTION( "SetPreviewColor", funcSetPreviewColor );
	NATIVE_FUNCTION( "ClearPreviewColor", funcClearPreviewColor );
	NATIVE_FUNCTION( "ColorItem", funcColorItem );
	NATIVE_FUNCTION( "ClearItemColor", funcClearItemColor );
	NATIVE_FUNCTION( "GetEnchantment", funcGetEnchantment );
	NATIVE_FUNCTION( "IsItemEnchanted", funcIsItemEnchanted );
	NATIVE_FUNCTION( "EnchantItem", funcEnchantItem );
	NATIVE_FUNCTION( "UnenchantItem", funcUnenchantItem );
	NATIVE_FUNCTION( "EnhanceItem", funcEnhanceItem );
	NATIVE_FUNCTION( "RemoveItemEnhancementByIndex", funcRemoveItemEnhancementByIndex );
	NATIVE_FUNCTION( "RemoveItemEnhancementByName", funcRemoveItemEnhancementByName );
	NATIVE_FUNCTION( "GetCraftedItemName", funcGetCraftedItemName );
	NATIVE_FUNCTION( "IsItemMounted", funcIsItemMounted ); 
	NATIVE_FUNCTION( "IsItemHeld", funcIsItemHeld ); 
	NATIVE_FUNCTION( "GetDeploymentItemEntity", funcGetDeploymentItemEntity ); 
	NATIVE_FUNCTION( "PlayItemEffect", funcPlayItemEffect );
	NATIVE_FUNCTION( "StopItemEffect", funcStopItemEffect );
	NATIVE_FUNCTION( "DropItem", funcDropItem );
	NATIVE_FUNCTION( "GetItemHoldSlot", funcGetItemHoldSlot );
	NATIVE_FUNCTION( "EnableLoot", funcEnableLoot );
	NATIVE_FUNCTION( "UpdateLoot", funcUpdateLoot );
	NATIVE_FUNCTION( "AddItemsFromLootDefinition", funcAddItemsFromLootDefinition );
	NATIVE_FUNCTION( "IsLootRenewable", funcIsLootRenewable );
	NATIVE_FUNCTION( "IsReadyToRenew", funcIsReadyToRenew );
	NATIVE_FUNCTION( "RemoveAllItems", funcRemoveAllItems );
	NATIVE_FUNCTION( "GetInventoryItemUIData", funcGetInventoryItemUIData );
    NATIVE_FUNCTION( "SetInventoryItemUIData", funcSetInventoryItemUIData );
    NATIVE_FUNCTION( "GetSchematicIngredients", funcGetSchematicIngredients );
    NATIVE_FUNCTION( "GetSchematicRequiredCraftsmanType", funcGetSchematicRequiredCraftsmanType);
	NATIVE_FUNCTION( "GetSchematicRequiredCraftsmanLevel", funcGetSchematicRequiredCraftsmanLevel);
	NATIVE_FUNCTION( "GetItemRecyclingParts", funcGetItemRecyclingParts );
	NATIVE_FUNCTION( "GetItemGridSize", funcGetItemGridSize );
	NATIVE_FUNCTION( "SplitItem", funcSplitItem );
	NATIVE_FUNCTION( "SetItemStackable", funcSetItemStackable );
	NATIVE_FUNCTION( "GetCategoryDefaultItem", funcGetCategoryDefaultItem );

	NATIVE_FUNCTION( "HasItemDurability", funcHasItemDurability );
	NATIVE_FUNCTION( "GetItemDurability", funcGetItemDurability );
	NATIVE_FUNCTION( "SetItemDurability", funcSetItemDurability );
	NATIVE_FUNCTION( "GetItemInitialDurability", funcGetItemInitialDurability );
	NATIVE_FUNCTION( "GetItemMaxDurability", funcGetItemMaxDurability );
	
	NATIVE_FUNCTION( "ActivateQuestBonus", funcActivateQuestBonus );

	NATIVE_FUNCTION( "GetItem", funcGetItem );
	NATIVE_FUNCTION( "GetFundsModifier", funcGetFundsModifier );
	NATIVE_FUNCTION( "AddSlot", funcAddSlot );
	NATIVE_FUNCTION( "GetSlotItemsLimit", funcGetSlotItemsLimit );

	NATIVE_FUNCTION( "GetItemPriceAddSlot", funcGetItemPriceAddSlot );
	NATIVE_FUNCTION( "GetItemPriceCrafting", funcGetItemPriceCrafting );
	NATIVE_FUNCTION( "GetItemPriceRemoveEnchantment", funcGetItemPriceRemoveEnchantment );
	NATIVE_FUNCTION( "GetItemPriceEnchantItem", funcGetItemPriceEnchantItem );
	NATIVE_FUNCTION( "GetItemPriceDisassemble", funcGetItemPriceDisassemble );
	NATIVE_FUNCTION( "GetItemPriceRemoveUpgrade", funcGetItemPriceRemoveUpgrade );
	NATIVE_FUNCTION( "GetItemPriceRepair", funcGetItemPriceRepair );
	NATIVE_FUNCTION( "GetItemPriceModified", funcGetItemPriceModified );
	NATIVE_FUNCTION( "GetInventoryItemPriceModified", funcGetInventoryItemPriceModified );
	NATIVE_FUNCTION( "GetItemPrice", funcGetItemPrice );
	NATIVE_FUNCTION( "GetItemWeight", funcGetItemWeight );
	NATIVE_FUNCTION( "TotalItemStats", funcTotalItemStats );

	NATIVE_FUNCTION( "GetItemSetName", funcGetItemSetName );
	NATIVE_FUNCTION( "SortInventoryUIData", funcSortInventoryUIData );
	NATIVE_FUNCTION( "AddItemCraftedAbility", funcAddItemCraftedAbility );
	NATIVE_FUNCTION( "RemoveItemCraftedAbility", funcRemoveItemCraftedAbility );
	NATIVE_FUNCTION( "AddItemBaseAbility", funcAddItemBaseAbility );
	NATIVE_FUNCTION( "RemoveItemBaseAbility", funcRemoveItemBaseAbility );
	NATIVE_FUNCTION( "PreviewItemAttributeAfterUpgrade", funcPreviewItemAttributeAfterUpgrade );

    NATIVE_FUNCTION( "GetItemLocalizedNameByUniqueID", funcGetItemLocalizedNameByUniqueID );
    NATIVE_FUNCTION( "GetItemLocalizedDescriptionByUniqueID", funcGetItemLocalizedDescriptionByUniqueID );

    NATIVE_FUNCTION( "GetItemLocalizedNameByName", funcGetItemLocalizedNameByName );
    NATIVE_FUNCTION( "GetItemLocalizedDescriptionByName", funcGetItemLocalizedDescriptionByName );

    NATIVE_FUNCTION( "GetItemIconPathByUniqueID", funcGetItemIconPathByUniqueID );
    NATIVE_FUNCTION( "GetItemIconPathByName", funcGetItemIconPathByName );

    NATIVE_FUNCTION( "GetNumOfStackedItems" , funcGetNumOfStackedItems );

	NATIVE_FUNCTION( "NotifyScriptedListeners" , funcNotifyScriptedListeners );
	NATIVE_FUNCTION( "InitInvFromTemplate", funcInitInvFromTemplate );

	NATIVE_FUNCTION( "BalanceItemsWithPlayerLevel", funcBalanceItemsWithPlayerLevel );

	NATIVE_FUNCTION( "NotifyItemLooted", funcNotifyItemLooted );
	NATIVE_FUNCTION( "ResetContainerData", funcResetContainerData );
END_CLASS_RTTI()

RED_DECLARE_NAME( ItemContainer );

//////////////////////////////////////////////////////////////////////////

class CItemIterator
{
private:
	CInventoryComponent*	m_inventory;
	Int32					m_index;
	Uint16					m_flagFilter;

public:
	CItemIterator(CInventoryComponent* inventory,Uint16 flagFilter=0) : m_index( -1 ), m_inventory( inventory ), m_flagFilter( flagFilter )
	{
		ASSERT( m_inventory );
		Next();
	}

	//! Is current item valid
	RED_INLINE operator Bool () const
	{
		return IsValid();
	}

	//! Advance to next
	RED_INLINE void operator++ ()
	{
		Next();
	}

	//! Get current
	RED_INLINE const SInventoryItem* operator*()
	{
		ASSERT( m_index < (Int32)m_inventory->GetItemCount() );
		return m_inventory->GetItem( (SItemUniqueId)m_index );
	}

protected:
	//! Is the iterator valid ?
	Bool IsValid() const;

	//! Advance to next component
	void Next();

private:
	//! Assignment is illegal
	RED_INLINE CItemIterator& operator=( const CInventoryComponent& other )
	{
		return *this;
	}
};

//////////////////////////////////////////////////////////////////////////

struct SItemNameProperty
{
	DECLARE_RTTI_STRUCT( SItemNameProperty )

	SItemNameProperty()
		: m_itemName( CName::NONE )
	{}

	CName m_itemName;
};

BEGIN_CLASS_RTTI( SItemNameProperty )
	PROPERTY_CUSTOM_EDIT( m_itemName, TXT( "Ite, name" ), TXT( "ItemSelection" ) )
END_CLASS_RTTI()
