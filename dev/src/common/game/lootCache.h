/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "lootEntry.h"

class CInventoryComponent;
class ILootManager;

class CLootCache
{
public:

	CLootCache();
	void Enable( Bool enable );
	RED_FORCE_INLINE Bool IsEnabled() const { return m_enabled; }
	void Update( CInventoryComponent* inventory, Bool force = false );
	void AddItemsFromDefinition( CInventoryComponent *inventory, const CName& lootDefinitionName );
	Bool IsRenewable() const;
	Bool IsReadyToRenew() const;
	void Save( IGameSaver* saver );
	void Load( IGameLoader* loader );

	void StreamLoad( ISaveFile* loader, Uint32 version );
	void StreamSave( ISaveFile* saver );

private:

	CInventoryComponent*		m_inventory;
	Bool						m_enabled;
	TDynArray< SLootEntry >		m_entries;
	EngineTime					m_nextLootUpdate;
	ILootManager*				m_lootManager;

	void Init( CInventoryComponent* inventory );
	void AddItemToInventory( CInventoryComponent* inventory, const SLootEntry& entry );
	
	Bool ValidateMaxCount( SLootEntry& entry );
	void UpdateItemMaxCount( const CName& itemName, Uint32 generatedQuantity );
};