#include "build.h"

#include "r6InventoryItemComponent.h"
#include "r6InventoryComponent.h"

IMPLEMENT_ENGINE_CLASS( CR6InventoryItemComponent );

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
CR6InventoryItemComponent::CR6InventoryItemComponent() 
	: m_stackable( false ) ,
	  m_quantity( 1 ) ,
	  m_inventory( NULL )
{
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6InventoryItemComponent::OnAttachFinished( CWorld* world )
{
	TBaseClass::OnAttachFinished( world );

	// store GUID as name
	CGUID guid = GetEntity()->GetGUID();
	Char entityGuidStr[ RED_GUID_STRING_BUFFER_SIZE ];
	guid.ToString( entityGuidStr, RED_GUID_STRING_BUFFER_SIZE );
	m_itemUniqueID = CName( String::Printf( TXT("%s"), entityGuidStr ) );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6InventoryItemComponent::funcGetInventory( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_HANDLE( CR6InventoryComponent, m_inventory );
}