#pragma once

#include "itemUniqueId.h"
#include "../engine/entityTemplateParams.h"
#include "inventoryEditor.h"


class IEquipmentInitializer : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IEquipmentInitializer, CObject );

public:
	//! Spawn item for given category
	virtual CName EvaluateItemName( CName category ) const = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( IEquipmentInitializer );
PARENT_CLASS( CObject );
END_CLASS_RTTI();

class CEquipmentInitializerRandom : public IEquipmentInitializer
{
	DECLARE_ENGINE_CLASS( CEquipmentInitializerRandom, IEquipmentInitializer, 0 );

public:
	//! Get item
	virtual CName EvaluateItemName( CName category ) const;
};

BEGIN_CLASS_RTTI( CEquipmentInitializerRandom );
PARENT_CLASS( IEquipmentInitializer );
END_CLASS_RTTI();

class CEquipmentInitializerUniform : public IEquipmentInitializer//, public INamesListOwner
{
	DECLARE_ENGINE_CLASS( CEquipmentInitializerUniform, IEquipmentInitializer, 0 );

protected:
	CName	m_itemName;		//<! Precisely defined item

public:
	CEquipmentInitializerUniform() {}

	//void GetNamesList( TDynArray< CName >& names ) const;
	//! Get item
	virtual CName EvaluateItemName( CName category ) const;
};

BEGIN_CLASS_RTTI( CEquipmentInitializerUniform );
PARENT_CLASS( IEquipmentInitializer );
PROPERTY_CUSTOM_EDIT( m_itemName, TXT("Item to be precisely set"), TXT("SuggestedListSelection") );
END_CLASS_RTTI();


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CEquipmentDefinitionEntry : public CObject, public INamesListOwner
{
	DECLARE_ENGINE_CLASS( CEquipmentDefinitionEntry, CObject, 0 );
	CName					m_category;
	CName					m_defaultItemName;
	IEquipmentInitializer*	m_initializer;

public:
	//const TDynArray< CName >* m_availableItems;

public:
	CEquipmentDefinitionEntry() : /*m_availableItems( NULL ),*/ m_initializer( NULL ) {}

	void GetNamesList( TDynArray< CName >& names ) const;

	//! Get category of this entry
	CName	GetCategory() const { return m_category; }

	//! Get item this entry specifies
	CName	GetItem() const;
};

BEGIN_CLASS_RTTI( CEquipmentDefinitionEntry );
PARENT_CLASS( CObject );
PROPERTY_CUSTOM_EDIT( m_category, TXT("Category this entry defines"), TXT("ItemCategorySelection") );
PROPERTY_CUSTOM_EDIT( m_defaultItemName, TXT("Default item, that will be mount when none is mount"), TXT("SuggestedListSelection") );
PROPERTY_INLINED( m_initializer, TXT("Choose initializer for this category") );
END_CLASS_RTTI();


class CEquipmentDefinition : public CEntityTemplateParam
{
	DECLARE_ENGINE_CLASS( CEquipmentDefinition, CEntityTemplateParam, 0 );
private:
	TDynArray< CEquipmentDefinitionEntry* >	m_entries;

public:
	//const TDynArray< CName >* m_availableCategories;

public:
	//CEquipmentDefinition() : m_availableCategories( NULL ) {}

	//! Get all entries
	const TDynArray< CEquipmentDefinitionEntry* >& GetEntries() const { return m_entries; }

	//! Get default item for given category
	CName GetDefaultItemForCategory( CName category ) const;

	//! Add new entry, if entry with given category already exists - return false
	Bool AddEntry( CEquipmentDefinitionEntry* entry );

	//! Remove entry, return false if had no such entry
	Bool RemoveEntry( CEquipmentDefinitionEntry* entry );
private:
};

BEGIN_CLASS_RTTI( CEquipmentDefinition );
PARENT_CLASS( CEntityTemplateParam );
PROPERTY( m_entries );
END_CLASS_RTTI();