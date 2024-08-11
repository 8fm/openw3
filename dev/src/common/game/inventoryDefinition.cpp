/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "inventoryDefinition.h"
#include "../../common/game/definitionsManager.h"
#include "../core/configVar.h"
#include "../core/configVarLegacyWrapper.h"

IMPLEMENT_ENGINE_CLASS( CInventoryDefinitionEntry );
IMPLEMENT_ENGINE_CLASS( CInventoryDefinition );

IMPLEMENT_ENGINE_CLASS( IInventoryInitializer );
IMPLEMENT_ENGINE_CLASS( CInventoryInitializerRandom );
IMPLEMENT_ENGINE_CLASS( CInventoryInitializerUniform );


String CInventoryDefinitionEntry::GetEntryDescription() const
{
	String description = m_category.AsString();
	description += TXT(":");
	if ( !m_initializer )
	{
		return description;
	}
	description += m_initializer->GetDescription();

	return description;
}

Uint32 CInventoryDefinitionEntry::EvaluateQuantity() const
{
	// TEMPSHIT, to be removed after balancing
	Float chanceMultiplier = 1.0f;
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("user"), TXT("Gameplay"), TXT("TempChancesMultiplier"), chanceMultiplier );

	// Scale probability by global scaling factor	
	Float scaledProbability = m_probability;
	if ( m_probability < 99.9f )
	{
		scaledProbability *= chanceMultiplier;
	}

	// Test probability of adding this entry's item
	if ( GEngine->GetRandomNumberGenerator().Get< Float >() <= ( scaledProbability / 99.5f ) )
	{
		// test passed, evaluate quantity
		Uint32 quantity = m_quantityMin;
		if ( m_quantityMax != m_quantityMin )
		{
			quantity = GEngine->GetRandomNumberGenerator().Get< Uint32 >( m_quantityMin , m_quantityMax + 1 );
		}

		// Done
		return quantity;
	}

	// Sorry, not this time
	return 0;
}

Bool CInventoryDefinition::AddEntry( CInventoryDefinitionEntry* entry )
{
	for ( Uint32 i=0; i<m_entries.Size(); ++i )
	{
		if ( m_entries[i]->m_category == entry->m_category )
		{
			// Already have entry for this category
			return false;
		}
	}

	// done
	m_entries.PushBack( entry );
	return true;
}

Bool CInventoryDefinition::RemoveEntry( CInventoryDefinitionEntry* entry )
{
	return m_entries.Remove( entry );
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CName CInventoryInitializerRandom::EvaluateItemName( CName category ) const
{
	const TDynArray< CName >& itemPool = GCommonGame->GetDefinitionsManager()->GetItemsOfCategory( category );
	Uint32 randomIndex = GEngine->GetRandomNumberGenerator().Get< Uint32 >( itemPool.Size() );
	return itemPool[ randomIndex ];
}

String CInventoryInitializerRandom::GetDescription() const
{
	return TXT( "random" );
}

//////////////////////////////////////////////////////////////////////////

CName CInventoryInitializerUniform::EvaluateItemName( CName category ) const
{
	return m_itemName;
}

String CInventoryInitializerUniform::GetDescription() const
{
	return m_itemName.AsString();
}

void CInventoryInitializerUniform::GetNamesList( TDynArray< CName >& names ) const
{
	CDefinitionsManager* dm = GCommonGame->GetDefinitionsManager();
	CInventoryDefinitionEntry* parentEntry = Cast< CInventoryDefinitionEntry >( GetParent() );
	
	if ( !parentEntry || !dm )
	{
		return;
	}

	names = dm->GetItemsOfCategory( parentEntry->GetCategory() );	
}