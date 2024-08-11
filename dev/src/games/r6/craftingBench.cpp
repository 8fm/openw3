#include "build.h"

#include "craftingBench.h"
#include "r6InventoryComponent.h"
#include "r6InventoryItemComponent.h"
#include "../../common/engine/dynamicLayer.h"

IMPLEMENT_ENGINE_CLASS( SCraftingRecipe );
IMPLEMENT_ENGINE_CLASS( CCraftingBench );

CName CCraftingBench::GetItemName( CEntityTemplate* _entityTemplate )
{
	// get item entity
	CGameplayEntity* entity = (_entityTemplate ? Cast<CGameplayEntity>( _entityTemplate->GetEntityObject() ) : 0);
	if( !entity )
	{
		// TODO: report error
		return CName::NONE;
	}

	// get inventory item component
	ComponentIterator<CR6InventoryItemComponent> it( entity );
	CR6InventoryItemComponent* item = ( it ? *it : 0 );
	if( !item )
	{
		// TODO: report error
		return CName::NONE;
	}

	return item->GetItemName();
}

Bool CCraftingBench::CanCraftItem( SCraftingRecipe& _recipe, CR6InventoryComponent& _inventory )
{
	// bad recipe?
	if( _recipe.m_requiredItems.Size() == 0 )
	{
		return false;
	}

	// check all required items
	for( Uint32 i=0; i<_recipe.m_requiredItems.Size(); ++i )
	{
		// get required item
		CEntityTemplate* requiredItemTemplate = _recipe.m_requiredItems[ i ].Get();
		CName itemName = GetItemName( requiredItemTemplate );

		// check if we got item
		if( !_inventory.HasItem( itemName, true ) )
		{
			return false;
		}
	}

	return true;
}

Bool CCraftingBench::CraftItem( SCraftingRecipe& _recipe, CR6InventoryComponent& _inventory )
{
	if( !CanCraftItem( _recipe, _inventory ) )
		return false;

	// check all required items
	for( Uint32 i=0; i<_recipe.m_requiredItems.Size(); ++i )
	{
		// get required item
		CEntityTemplate* requiredItemTemplate = _recipe.m_requiredItems[ i ].Get();
		CName itemName = GetItemName( requiredItemTemplate );

		// get inventory item
		CR6InventoryItemComponent* item = _inventory.GetItemByName( itemName );
		if( !item )
			return false;

		_inventory.DestroyItem( item );
	}

	// create the new item
	EntitySpawnInfo einfo;
	einfo.m_template = _recipe.m_craftedItem.Get();
	CEntity* createdItem = GGame->GetActiveWorld()->GetDynamicLayer()->CreateEntitySync( einfo );

	if( createdItem )
	{
		ComponentIterator<CR6InventoryItemComponent> it( createdItem );
		CR6InventoryItemComponent* createdInventoryItem = (it ? *it : 0);
		if( createdInventoryItem )
		{
			_inventory.AddItem( createdInventoryItem );
			return true;
		}
	}

	return false;
}

void CCraftingBench::funcCraftItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SCraftingRecipe, _recipe, SCraftingRecipe() );
	GET_PARAMETER( THandle<CR6InventoryComponent>, _inventory, NULL );
	FINISH_PARAMETERS;

	if( !_inventory.Get() )
		return;

	RETURN_BOOL( CraftItem( _recipe, *_inventory.Get() ) );
}