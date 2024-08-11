#pragma once

struct SCraftingRecipe
{
	DECLARE_RTTI_STRUCT( SCraftingRecipe );

	SCraftingRecipe()
	{
	}

	THandle< CEntityTemplate >					m_craftedItem;
	TDynArray< THandle< CEntityTemplate > >		m_requiredItems;
};

BEGIN_CLASS_RTTI( SCraftingRecipe );
	PROPERTY_EDIT_NAME( m_craftedItem,		TXT("i_craftedItem"), TXT("") );
	PROPERTY_EDIT_NAME( m_requiredItems,	TXT("i_requiredItems"), TXT("") );
END_CLASS_RTTI();

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CCraftingBench : public CGameplayEntity
{
	DECLARE_ENGINE_CLASS( CCraftingBench, CGameplayEntity, 0 );

private:
	TDynArray< SCraftingRecipe >		m_recipies;

	CName GetItemName( CEntityTemplate* _entityTemplate );

public:
	CCraftingBench()
	{
	}

	TDynArray< SCraftingRecipe >& GetRecipies()	{ return m_recipies; }

	Bool CanCraftItem( SCraftingRecipe& _recipe, CR6InventoryComponent& _inventory );
	Bool CraftItem( SCraftingRecipe& _recipe, CR6InventoryComponent& _inventory );

	void funcCraftItem( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CCraftingBench );
	PARENT_CLASS( CGameplayEntity );
	PROPERTY_EDIT_NAME( m_recipies,	TXT("i_recipies"), TXT("") );	
	NATIVE_FUNCTION( "I_CraftItem"				, funcCraftItem );
END_CLASS_RTTI();
