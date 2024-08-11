/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fxTrackItemPlayItemEffect.h"
#include "actor.h"

IMPLEMENT_ENGINE_CLASS( CFXTrackItemPlayItemEffect );

CFXTrackItemPlayItemEffect::CFXTrackItemPlayItemEffect()
{
}

/// Runtime player for item effect
class CFXTrackItemPlayItemEffectPlayData : public IFXTrackItemPlayData
{
public:
	const CFXTrackItemPlayItemEffect*				m_trackItem;		//!< Data
	THandle< CInventoryComponent >					m_inventory;		//!< Inventory component containing the subject item
	SItemUniqueId									m_itemId;			//!< Unique id of the item
	CName											m_effectName;		//!< Name of the effect

public:
	CFXTrackItemPlayItemEffectPlayData( const CFXTrackItemPlayItemEffect* trackItem, CInventoryComponent* inventoryComponent, SItemUniqueId itemId, CName effectName )
		: IFXTrackItemPlayData( inventoryComponent , trackItem )
		, m_trackItem( trackItem )
		, m_inventory( inventoryComponent )
		, m_itemId( itemId )
		, m_effectName( effectName )
	{
	};

	~CFXTrackItemPlayItemEffectPlayData()
	{
	}

	virtual void OnStop() 
	{
		m_inventory.Get()->StopItemEffect( m_itemId, m_effectName );
	}

	virtual void OnTick( const CFXState& fxState, Float timeDelta )
	{
	}
};

IFXTrackItemPlayData* CFXTrackItemPlayItemEffect::OnStart( CFXState& fxState ) const
{
	if ( !m_category || !m_effectName )
	{
		// Satan ignores you
		return NULL;
	}

	CEntity* entity = fxState.GetEntity();
	ASSERT( entity );
	CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( entity );
	if ( !gameplayEntity )
	{
		// Satan doesn't recognize you
		return NULL;
	}

	CInventoryComponent* inventory = gameplayEntity->GetInventoryComponent();
	if ( !inventory )
	{
		// Satan doesn't like you
		return NULL;
	}

	SItemUniqueId itemId;

	// We operate on item category
	itemId = inventory->GetItemByCategory( m_category, true );

	if ( itemId == SItemUniqueId::INVALID )
	{
		// Such item is not mount already, find any possessed item of this category
		itemId = inventory->GetItemByCategory( m_category, false );
	}

	if ( ( itemId == SItemUniqueId::INVALID ) && m_itemName_optional )
	{
		// Item is fully specified, used as backup
		itemId = inventory->GetItemId( m_itemName_optional );
	}

	// Finally check if we can play the effect
	if ( itemId == SItemUniqueId::INVALID )
	{
		return NULL;
	}

	// Play effect on the item
	inventory->PlayItemEffect( itemId, m_effectName );

	// Return play data
	return new CFXTrackItemPlayItemEffectPlayData( this, inventory, itemId, m_effectName );
}

void CFXTrackItemPlayItemEffect::SetName( const String& name )
{
}

String CFXTrackItemPlayItemEffect::GetName() const
{
	return TXT("Item effect");
}

void CFXTrackItemPlayItemEffect::GetNamesList( TDynArray< CName >& names ) const
{
	CDefinitionsManager* dm = GCommonGame->GetDefinitionsManager();

	if ( !dm )
	{
		return;
	}

	names = dm->GetItemsOfCategory( m_category );	
}
