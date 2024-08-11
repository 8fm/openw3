#include "build.h"

#include "playerInventoryPanel.h"
#include "r6Player.h"
#include "r6InteractionComponent.h"
#include "r6InventoryComponent.h"
#include "r6InventoryItemComponent.h"
#include "craftingBench.h"
#include "../../common/engine/renderFrame.h"

IMPLEMENT_ENGINE_CLASS( CPlayerInventoryPanel );
IMPLEMENT_ENGINE_CLASS( CPlayerCraftingPanel );

void CPlayerInventoryPanel::DrawInventory( CRenderFrame* frame )
{
	if( !m_isVisible )
		return;

	Int32 currentX = (Int32)m_panelPosition.X;
	Int32 currentY = (Int32)m_panelPosition.Y;	
	frame->AddDebugScreenText( currentX, currentY, TXT( "PLAYER INVENTORY " ), Color::GREEN, NULL, true, Color::BLACK );	
	currentY += m_lineHeight * 2;

	// get inventory component
	ComponentIterator<CR6InventoryComponent> it( m_player );
	CR6InventoryComponent* inv = ( it ? *it : NULL );
	if( !inv )
		return;

	// get items
	const TDynArray< THandle<CR6InventoryItemComponent> >& items = inv->GetItems();
	m_selectedItemIndex = Clamp( m_selectedItemIndex, 0, (Int32)items.Size() - 1 );	
	m_selectedItem = THandle< CR6InventoryItemComponent >();
	m_selectedInteraction = THandle< CR6InteractionComponent >();

	// header
	frame->AddDebugScreenText( currentX, currentY, String::Printf( TXT( "%-4s . %-45s %s" ), TXT("#"),TXT("Item"),TXT("Quantity") ), Color::GREEN,NULL, true, Color::BLACK );
	currentY+=m_lineHeight;	

	// draw items
	for( Uint32 i=0; i<items.Size(); ++i )
	{		
		const CR6InventoryItemComponent* item = items[ i ].Get();
		if( item == NULL )
			continue;

		// is the item selected
		Bool bSelected = ( m_selectedItemIndex == (Int32)i );
		if( bSelected )
			m_selectedItem = THandle< CR6InventoryItemComponent >( const_cast<CR6InventoryItemComponent*>( item ) );

		frame->AddDebugScreenText( currentX, 
								   currentY, 
								   String::Printf( TXT( "%-4d .     %-45s %d" ), i + 1, item->GetEntity()->GetDisplayName().AsChar(), item->GetQuantity() ), 
								   Color::GREEN, 
								   NULL, 
								   true, 
								   bSelected ? Color::YELLOW : Color::BLACK );

		// draw available actions
		ComponentIterator< CR6InteractionComponent > it( Cast< CEntity >( item->GetParent() ) );
		Int32 action = 0;
		for( ; it; ++it )
		{
			CR6InteractionComponent* interaction = *it;
			if( !interaction )
				continue;

			// can interact
			Bool result = false;
			CallFunctionRet< Bool >( interaction, CNAME( CanInteract ), result );
			if( !result )
				continue;

			// get interaction name
			CName interactionName;
			CallFunctionRet< CName >( interaction, CNAME( GetInteractionName ), interactionName );

			// is interaction selected?
			bSelected = ( action == m_selectedInteractionIndex ) && ( m_selectedItemIndex == (Int32)i );

			// print interaction name
			frame->AddDebugScreenText( currentX + 200 + action * 80, 
				currentY, 
				String::Printf( TXT( "%-20s" ), interactionName.AsString().AsChar() ), 
				Color::GREEN, 
				NULL, 
				true, 
				bSelected ? Color::YELLOW : Color::BLACK );

			if( bSelected )
				m_selectedInteraction = THandle< CR6InteractionComponent >( interaction );

			action++;
		}

		// clamp selected interaction index
		if( ( m_selectedItemIndex == (Int32)i ) )
		{
			m_selectedInteractionIndex = Clamp( m_selectedInteractionIndex, 0, action - 1 );
		}

		currentY += m_lineHeight;
	}	
}

void CPlayerCraftingPanel::DrawCrafting( CRenderFrame* frame )
{
	if( !m_isVisible || !m_player )
		return;

	Int32 currentX = (Int32)m_panelPosition.X;
	Int32 currentY = (Int32)m_panelPosition.Y;	
	frame->AddDebugScreenText( currentX, currentY, TXT( "CRAFTING" ), Color::GREEN, NULL, true, Color::BLACK );	
	currentY += m_lineHeight * 2;

	// close panel?
	if( GCommonGame->GetInputManager()->GetActionValue( CNAME( GI_Crouch ) )  > 0.9f )
		m_isVisible = false;	

	if( !m_bench )
		return;

	// get player inventory
	ComponentIterator<CR6InventoryComponent> it( m_player );
	CR6InventoryComponent* inventory = ( it ? *it : 0 );
	if( !inventory )
		return;

	// get items
	TDynArray< SCraftingRecipe >& recipies = m_bench->GetRecipies();
	m_selectedItemIndex = Clamp( m_selectedItemIndex, 0, (Int32)recipies.Size() - 1 );
	m_selectedItem = recipies[ m_selectedItemIndex ];

	// draw items
	for( Uint32 i=0; i<recipies.Size(); ++i )
	{		
		SCraftingRecipe& recipe = recipies[ i ];

		// is the item selected
		Bool bSelected = ( m_selectedItemIndex == (Int32)i );
		Bool bCanCraft = m_bench->CanCraftItem( recipe, *inventory );
		Color textColor = bCanCraft ? Color::GREEN : Color::RED;

		// get what item will be created
		CEntityTemplate* createdItem = recipe.m_craftedItem.Get();
		CGameplayEntity* entity = (createdItem ? Cast<CGameplayEntity>( createdItem->GetEntityObject() ) : 0);
		if( !entity )
			continue;

		frame->AddDebugScreenText( currentX, currentY, String::Printf( TXT( "%-4d .     %s  REQUIRES " ), i + 1, entity->GetDisplayName().AsChar() ), textColor, NULL, true, bSelected ? Color::YELLOW : Color::BLACK );

		// list required items
		for( Uint32 j=0; j<recipe.m_requiredItems.Size(); ++j )
		{
			// get required item
			CEntityTemplate* requiredItemTemplate = recipe.m_requiredItems[ j ].Get();
			CGameplayEntity* requiredItem = (requiredItemTemplate ? Cast<CGameplayEntity>( requiredItemTemplate->GetEntityObject() ) : 0);
			if( !requiredItem )
				continue;

			frame->AddDebugScreenText( currentX + 200 + j * 80, currentY, requiredItem->GetDisplayName().AsChar(), textColor, NULL, true, bSelected ? Color::YELLOW : Color::BLACK );
		}

		currentY += m_lineHeight;
	}
}

void CPlayerCraftingPanel::funcGetBench( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_HANDLE( CCraftingBench, m_bench );
}

void CPlayerCraftingPanel::funcSetBench( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CCraftingBench >, _bench, NULL );
	FINISH_PARAMETERS;
	m_bench = _bench.Get();
}