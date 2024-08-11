/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "inventoryReport.h"
#include "../../common/game/inventoryDefinition.h"
#include "../../common/core/xmlFileWriter.h"


Bool CInventoryReport::CreateReport( CInventoryDefinition* inventoryDef, const String& path )
{
	if ( !inventoryDef )
	{
		return false;
	}

	m_paths.PushBackUnique( path );
	Int32 pathIndex = m_paths.GetIndex( path );

	const TDynArray< CInventoryDefinitionEntry* >& entries = inventoryDef->GetEntries();

	for ( Uint32 i=0; i<entries.Size(); ++i )
	{
		CInventoryDefinitionEntry* entry = entries[i];
		ASSERT( entry );
		if ( entry )
		{
			IInventoryInitializer* initializer = entry->GetInitializer();
			ASSERT( initializer );
			if ( initializer && initializer->IsA< CInventoryInitializerUniform >() )
			{
				const CName itemCategory = entry->GetCategory();
				const CName itemName = initializer->EvaluateItemName( itemCategory );

				ItemData* itemData = FindItemDataOrCreate( itemName, itemCategory );
				ASSERT( itemData );

				ItemOccurenceInfo newInfo;
				newInfo.min = entry->m_quantityMin;
				newInfo.max = entry->m_quantityMax;
				newInfo.probability = entry->m_probability;
				newInfo.pathIndex = pathIndex;

				itemData->occurences.PushBack( newInfo );
			}
		}
	}

	return true;
}

Bool CInventoryReport::WriteXML() const
{
	THashMap< CName, TDynArray< ItemData > >::const_iterator it = m_items.Begin();
	for ( ; it != m_items.End(); ++it )
	{
		const TDynArray< ItemData >& itemDataArray = it->m_second;
		const CName category = it->m_first;

		IFile* file = GFileManager->CreateFileWriter( GFileManager->GetDataDirectory() + TXT("inv_report_") + category.AsString() + TXT(".xml"), FOF_AbsolutePath );
		ASSERT( file );

		CXMLFileWriter xmlWriter( *file );

		for ( Uint32 i=0; i<itemDataArray.Size(); ++i )
		{
			const ItemData& itemData = itemDataArray[i];
			xmlWriter.BeginNode( TXT("item") );
			xmlWriter.SetAttribute( TXT("name"), itemData.itemName.AsString() );

			const TDynArray< ItemOccurenceInfo >& occurences = itemData.occurences;
			for ( Uint32 j=0; j<occurences.Size(); ++j )
			{
				const ItemOccurenceInfo& occurenceInfo = occurences[j];
				xmlWriter.BeginNode( TXT("a") );
				xmlWriter.SetAttribute( TXT("path"), m_paths[ occurenceInfo.pathIndex ] );
				xmlWriter.AttributeT( TXT("min"), occurenceInfo.min );
				xmlWriter.AttributeT( TXT("max"), occurenceInfo.max );
				xmlWriter.AttributeT( TXT("probability"), occurenceInfo.probability );
				xmlWriter.EndNode();
			}

			xmlWriter.EndNode();
		}

		xmlWriter.Flush();
		delete file;
		file = NULL;
	}

	return  true;
}

CInventoryReport::ItemData* CInventoryReport::FindItemDataOrCreate( CName itemName, CName itemCategory )
{
	TDynArray< ItemData >& items = m_items[ itemCategory ];
	for ( Uint32 i=0; i<items.Size(); ++i )
	{
		if ( items[i].itemName == itemName )
		{
			return &(items[i]);
		}
	}

	ItemData* itemData = new (items) ItemData;
	itemData->itemName = itemName;

	return itemData;
}
