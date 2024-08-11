/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "equipmentState.h"
#include "inventoryDefinition.h"

#include "definitionsManager.h"

IMPLEMENT_ENGINE_CLASS( CEquipmentDefinitionEntry );
IMPLEMENT_ENGINE_CLASS( CEquipmentDefinition );

IMPLEMENT_ENGINE_CLASS( IEquipmentInitializer );
IMPLEMENT_ENGINE_CLASS( CEquipmentInitializerRandom );
IMPLEMENT_ENGINE_CLASS( CEquipmentInitializerUniform );


void CEquipmentDefinitionEntry::GetNamesList( TDynArray< CName >& names ) const
{
	/*CEquipmentDefinition* parentDef = Cast< CEquipmentDefinition >( GetParent() );
	if ( parentDef && parentDef->m_availableCategories )
	{
		names = *parentDef->m_availableCategories;	
	}*/

	names = GCommonGame->GetDefinitionsManager()->GetItemsOfCategory( m_category );
}

CName CEquipmentDefinitionEntry::GetItem() const
{
	if ( m_initializer )
	{
		return m_initializer->EvaluateItemName( m_category ); 
	}	
	return CName::NONE;
}

Bool CEquipmentDefinition::AddEntry( CEquipmentDefinitionEntry* entry )
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

Bool CEquipmentDefinition::RemoveEntry( CEquipmentDefinitionEntry* entry )
{
	return m_entries.Remove( entry );
}

CName CEquipmentDefinition::GetDefaultItemForCategory( CName category ) const
{
	for ( Uint32 i=0; i<m_entries.Size(); ++i )
	{
		if ( m_entries[i]->m_category == category )
		{
			return m_entries[i]->m_defaultItemName;
		}
	}

	// not found
	return CName::NONE;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CName CEquipmentInitializerRandom::EvaluateItemName( CName category ) const
{
	const TDynArray< CName >& itemPool = GCommonGame->GetDefinitionsManager()->GetItemsOfCategory( category );
	Uint32 randomIndex = GEngine->GetRandomNumberGenerator().Get< Uint32 >( itemPool.Size() );
	return itemPool[ randomIndex ];
}

//////////////////////////////////////////////////////////////////////////

CName CEquipmentInitializerUniform::EvaluateItemName( CName category ) const
{
	return m_itemName;
}

//void CEquipmentInitializerUniform::GetNamesList( TDynArray< CName >& names ) const
//{
//	CEquipmentDefinitionEntry* parentEntry = Cast< CEquipmentDefinitionEntry >( GetParent() );
//	if ( parentEntry /*&& parentEntry->m_availableItems*/ )
//	{
//		//names = *parentEntry->m_availableItems;
//		names = GCommonGame->GetDefinitionsManager()->GetItemsOfCategory( parentEntry->m_category );
//	}
//}