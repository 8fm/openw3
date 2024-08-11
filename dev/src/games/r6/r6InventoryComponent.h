#pragma once

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CR6InventoryComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CR6InventoryComponent, CComponent, 0 );

private:
	TDynArray< THandle<CR6InventoryItemComponent> >		m_inventoryItems;
	TDynArray< THandle<CEntityTemplate> >				m_startingItems;

public:
	CR6InventoryComponent();

	void AddItem				( CR6InventoryItemComponent* _component );
	Bool RemoveItem				( CR6InventoryItemComponent* _component );
	Bool DropItem				( CR6InventoryItemComponent* _component );
	Bool DropItem				( CR6InventoryItemComponent* _component, Vector3 _position, EulerAngles _rotation, Vector3 _velocity );
	Bool DestroyItem			( CR6InventoryItemComponent* _component );
	void RemoveAllItems			();

	Bool HasItem( CName _itemName, Bool _recursive = false );
	CR6InventoryItemComponent* GetItemByName( CName _itemName, Bool _recursive = false );
	void GetItemsByCategory( CName _categoryName, TDynArray< CR6InventoryItemComponent* >& _result, Bool _recursive = false );
	Uint32 GetItemQuantity( CName _itemName, Bool _recursive = false );

	const TDynArray< THandle<CR6InventoryItemComponent> >& GetItems() const { return m_inventoryItems; }

protected:
	Bool RemoveItemFromWorld( CR6InventoryItemComponent* _component );
	Bool AddItemToWorld( CR6InventoryItemComponent* _component, Bool _disableRigidbody = false );

	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );
	virtual void OnTick( Float timeDelta );

private:
	void funcAddItem			( CScriptStackFrame& stack, void* result );
	void funcRemoveItem			( CScriptStackFrame& stack, void* result );
	void funcDropItem			( CScriptStackFrame& stack, void* result );
	void funcDropItemToLocation	( CScriptStackFrame& stack, void* result );
	void funcDestroyItem		( CScriptStackFrame& stack, void* result );
	void funcRemoveAllItems		( CScriptStackFrame& stack, void* result );
	void funcHasItem			( CScriptStackFrame& stack, void* result );
	void funcGetItemByName		( CScriptStackFrame& stack, void* result );
	void funcGetItemsByCategory	( CScriptStackFrame& stack, void* result );
	void funcGetItemQuantity	( CScriptStackFrame& stack, void* result );
	void funcRemoveItemFromWorld( CScriptStackFrame& stack, void* result );
	void funcAddItemToWorld		( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CR6InventoryComponent );
	PARENT_CLASS( CComponent );
	
	PROPERTY_NAME( m_inventoryItems, TXT("i_inventoryItems") );
	PROPERTY_EDIT_NAME( m_startingItems, TXT("i_startingItems"), TXT( "Starting Items" ) );

	NATIVE_FUNCTION( "I_AddItem"				, funcAddItem				);
	NATIVE_FUNCTION( "I_RemoveItem"				, funcRemoveItem			);
	NATIVE_FUNCTION( "I_DropItem"				, funcDropItem				);
	NATIVE_FUNCTION( "I_DropItemToLocation"		, funcDropItemToLocation	);
	NATIVE_FUNCTION( "I_DestroyItem"			, funcDestroyItem			);
	NATIVE_FUNCTION( "I_RemoveAllItems"			, funcRemoveAllItems		);
	NATIVE_FUNCTION( "I_HasItem"				, funcHasItem				);
	NATIVE_FUNCTION( "I_GetItemByName"			, funcGetItemByName			);
	NATIVE_FUNCTION( "I_GetItemsByCategory"		, funcGetItemsByCategory	);
	NATIVE_FUNCTION( "I_GetItemQuantity"		, funcGetItemQuantity		);
	NATIVE_FUNCTION( "I_RemoveItemFromWorld"	, funcRemoveItemFromWorld	);
	NATIVE_FUNCTION( "I_AddItemToWorld"			, funcAddItemToWorld		);

END_CLASS_RTTI();
