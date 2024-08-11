/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CInventoryReport
{
	struct ItemOccurenceInfo
	{
		Uint32		pathIndex;
		Uint32		min,max;
		Float		probability;
	};

	struct ItemData
	{
		CName							itemName;
		TDynArray< ItemOccurenceInfo >	occurences;

		friend Bool operator==( const ItemData& lhs, const ItemData& rhs )
		{
			return lhs.itemName == rhs.itemName;
		}
	};

	TDynArray< String >							m_paths;
	THashMap< CName, TDynArray< ItemData > >	m_items;

public:
	Bool CreateReport( CInventoryDefinition* inventoryDef, const String& path );

	ItemData* FindItemDataOrCreate( CName itemName, CName itemCategory );

	Bool WriteXML() const;
};