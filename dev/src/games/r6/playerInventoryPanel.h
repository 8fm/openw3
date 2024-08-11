#pragma  once

class CR6Player;
class CR6InventoryItemComponent;
class CR6InteractionComponent;
class CCraftingBench;

#include "craftingBench.h"

class CPlayerInventoryPanel : public CObject
{
	DECLARE_ENGINE_CLASS( CPlayerInventoryPanel, CObject, 0 );
private:
	CR6Player*							m_player;
	Vector								m_panelPosition;
	Int32									m_lineHeight;
	
	Bool								m_isVisible;
	Int32									m_selectedItemIndex;
	Int32									m_selectedInteractionIndex;
	THandle<CR6InventoryItemComponent>	m_selectedItem;
	THandle<CR6InteractionComponent>	m_selectedInteraction;

public:
	CPlayerInventoryPanel() : m_panelPosition( 500, 300, 0 ), m_lineHeight( 20 ), m_selectedItemIndex( 0 ), m_selectedInteractionIndex( 0 )
	{
	}

	RED_INLINE void SetPlayer( CR6Player* player ) { m_player = player; }

	void DrawInventory( CRenderFrame* frame );
};

BEGIN_CLASS_RTTI( CPlayerInventoryPanel );
	PARENT_CLASS( CObject );
	PROPERTY_NAME( m_isVisible					, TXT( "i_isVisible" )		);
	PROPERTY_NAME( m_selectedItemIndex			, TXT( "i_selectedItemIndex" )	);
	PROPERTY_NAME( m_selectedItem				, TXT( "i_selectedItem" )	);
	PROPERTY_NAME( m_selectedInteractionIndex	, TXT( "i_selectedInteractionIndex" )	);
	PROPERTY_NAME( m_selectedInteraction		, TXT( "i_selectedInteraction" )	);
END_CLASS_RTTI();


// TEMP CRAFTING PANEL: - TODO Should be replaced with proper GUI at some point
class CPlayerCraftingPanel : public CObject
{
	DECLARE_ENGINE_CLASS( CPlayerCraftingPanel, CObject, 0 );
private:
	CR6Player*							m_player;
	Vector								m_panelPosition;
	Int32									m_lineHeight;
	CCraftingBench*						m_bench;

	Bool								m_isVisible;
	Int32									m_selectedItemIndex;
	SCraftingRecipe						m_selectedItem;

public:
	CPlayerCraftingPanel() : m_panelPosition( 500, 300, 0 ), m_lineHeight( 20 ), m_selectedItemIndex( 0 ), m_bench( NULL )
	{
	}

	RED_INLINE void SetPlayer( CR6Player* player ) { m_player = player; }

	void DrawCrafting( CRenderFrame* frame );

public:
	void funcGetBench			( CScriptStackFrame& stack, void* result );
	void funcSetBench			( CScriptStackFrame& stack, void* result );

};

BEGIN_CLASS_RTTI( CPlayerCraftingPanel );
PARENT_CLASS( CObject );
	PROPERTY_NAME( m_isVisible					, TXT( "i_isVisible" )			);
	PROPERTY_NAME( m_selectedItemIndex			, TXT( "i_selectedItemIndex" )	);
	PROPERTY_NAME( m_selectedItem				, TXT( "i_selectedItem" )		);

	NATIVE_FUNCTION( "I_GetBench"				, funcGetBench );
	NATIVE_FUNCTION( "I_SetBench"				, funcSetBench );
END_CLASS_RTTI();
// END TEMP CRAFTING PANEL
