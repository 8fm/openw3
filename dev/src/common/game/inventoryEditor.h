/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once


class CEntityTemplate;
class CItemEntity;
class CActor;
#include "../../common/game/itemUniqueId.h"

// Used by editor
namespace InventoryEditor
{
	void GetItemList( TDynArray< CName >& items );
	void GetAbilityList( TDynArray< CName >& abilities );
	void GetItemCategoriesList( TDynArray< CName >& categories );
	void GetItemsOfCategory( TDynArray< CName >& items, CName category );

	void RelodDefinitions();
};

//////////////////////////////////////////////////////////////////////////

class INamesListOwner
{
public:
	virtual void GetNamesList( TDynArray< CName >& names ) const = 0;
};

//////////////////////////////////////////////////////////////////////////

class IEquipmentPreview
{
private:
	TDynArray< CItemEntity* >				m_itemsInPreview;
	TDynArray< CEntityTemplate* >			m_templatesInPreview;

public:
	void ClearEquipmentPreviewItems( CEntity* entity );

	void ApplyEquipmentPreview( CEntity* entity );
};

//////////////////////////////////////////////////////////////////////////

enum EItemDisplaySlot
{
	IDS_RightHand,
	IDS_LeftHand,
	IDS_Mount
};

class CItemDisplayer
{
public:
	SItemUniqueId	m_lastItemDisplayed;

public:
	//! Get an actor this displayer attaches items to
	virtual CActor*	GetActorEntity() { return NULL; }

	//! Apply given item
	virtual void DisplayItem( CName itemName, EItemDisplaySlot slot );

	//! Unapply last item
	virtual void UndoItemDisplay();
};

