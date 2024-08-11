#pragma once

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CR6InventoryItemComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CR6InventoryItemComponent, CComponent, 0 );

	friend class CR6InventoryComponent;

private:
	CName					m_name;
	CName					m_category;
	Bool					m_stackable;
	Uint32					m_quantity;	
	CName					m_itemUniqueID;

	CR6InventoryComponent*	m_inventory;

public:
	CR6InventoryItemComponent();

	CName GetItemName() const					{ return m_name; }
	CName GetItemCategory()	const				{ return m_category; }
	Bool IsStackable() const					{ return m_stackable; }
	Uint32 GetQuantity() const					{ return m_quantity; }
	CR6InventoryComponent* GetInventory() const	{ return m_inventory; }

	virtual void OnAttachFinished( CWorld* world );

private:
	void funcGetInventory			( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CR6InventoryItemComponent );
PARENT_CLASS( CComponent );
	PROPERTY_EDIT_NAME( m_name,			TXT("i_name"),			TXT( "Item Name" ) );
	PROPERTY_EDIT_NAME( m_category,		TXT("i_category"),		TXT( "Item Category" ) );
	PROPERTY_EDIT_NAME( m_stackable,	TXT("i_stackable"),		TXT( "Is Item Stackable (e.g Ammo etc)." ) );
	PROPERTY_EDIT_NAME( m_quantity,		TXT("i_quantity"),		TXT( "Item Quantity (only used if item is stackable)" ) );
	PROPERTY_EDIT_NAME( m_itemUniqueID,	TXT("i_itemUniqueID"),	TXT( "Item Unique (ID)" ) );

	NATIVE_FUNCTION( "I_GetInventory",	funcGetInventory				);
END_CLASS_RTTI();
