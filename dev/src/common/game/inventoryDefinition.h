#pragma once
#include "itemUniqueId.h"
#include "../../common/engine/entityTemplateParams.h"
#include "inventoryEditor.h"


//////////////////////////////////////////////////////////////////////////

class IInventoryInitializer : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IInventoryInitializer, CObject );

public:
	//! Spawn item for given category
	virtual CName EvaluateItemName( CName category ) const = 0;

	//! Get item name if possible, or at least some "random" word
	virtual String GetDescription() const = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( IInventoryInitializer );
PARENT_CLASS( CObject );
END_CLASS_RTTI();

class CInventoryInitializerRandom : public IInventoryInitializer
{
	DECLARE_ENGINE_CLASS( CInventoryInitializerRandom, IInventoryInitializer, 0 );

public:
	//! Get item
	virtual CName EvaluateItemName( CName category ) const;

	//! Get item name if possible, or at least some "random" word
	virtual String GetDescription() const;
};

BEGIN_CLASS_RTTI( CInventoryInitializerRandom );
PARENT_CLASS( IInventoryInitializer );
END_CLASS_RTTI();

class CInventoryInitializerUniform : public IInventoryInitializer, public INamesListOwner
{
	DECLARE_ENGINE_CLASS( CInventoryInitializerUniform, IInventoryInitializer, 0 );

protected:
	CName	m_itemName;		//<! Precisely defined item

public:
	virtual void GetNamesList( TDynArray< CName >& names ) const;
	//! Get item
	virtual CName EvaluateItemName( CName category ) const;

	//! Get item name if possible, or at least some "random" word
	virtual String GetDescription() const;
};

BEGIN_CLASS_RTTI( CInventoryInitializerUniform );
PARENT_CLASS( IInventoryInitializer );
PROPERTY_CUSTOM_EDIT( m_itemName, TXT("Item to be precisely set"), TXT("SuggestedListSelection") );
END_CLASS_RTTI();


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CInventoryDefinitionEntry : public CObject
{
	DECLARE_ENGINE_CLASS( CInventoryDefinitionEntry, CObject, 0 );
	CName					m_category;
	Uint32					m_quantityMin;
	Uint32					m_quantityMax;
	Float					m_probability;
	Bool					m_isMount;
	Bool					m_isLootable;
	IInventoryInitializer*	m_initializer;

public:
	CInventoryDefinitionEntry() 
		: m_initializer( NULL )
		, m_quantityMin( 1 )
		, m_quantityMax( 1 )
		, m_probability( 100.0f )
		, m_isMount( false )
		, m_isLootable( true )
	{}

	//! Get category of this entry
	CName	GetCategory() const { return m_category; }

	//! Get quantity, performing probability computation etc.
	Uint32	EvaluateQuantity() const;

	//! Get item name if possible, or at least some "random" word
	String	GetEntryDescription() const;

	IInventoryInitializer* GetInitializer() const { return m_initializer; }
};

BEGIN_CLASS_RTTI( CInventoryDefinitionEntry );
PARENT_CLASS( CObject );
PROPERTY_CUSTOM_EDIT( m_category, TXT("Category this entry defines"), TXT("ItemCategorySelection") );
PROPERTY_EDIT( m_quantityMin, TXT("Minimum quantity of the item") );
PROPERTY_EDIT( m_quantityMax, TXT("Maximum quantity of the item") );
PROPERTY_EDIT( m_probability, TXT("Probability of spawning an item") );
PROPERTY_EDIT( m_isMount, TXT("Is item mount to slot?") );
PROPERTY_EDIT( m_isLootable, TXT("Is item dropped after death") );
PROPERTY_INLINED( m_initializer, TXT("Choose initializer for this category") );
END_CLASS_RTTI();


class CInventoryDefinition : public CEntityTemplateParam
{
	DECLARE_ENGINE_CLASS( CInventoryDefinition, CEntityTemplateParam, 0 );
private:
	TDynArray< CInventoryDefinitionEntry* >	m_entries;

public:
	//! Get all entries
	const TDynArray< CInventoryDefinitionEntry* >& GetEntries() const { return m_entries; }

	//! Add new entry, if entry with given category already exists - return false
	Bool AddEntry( CInventoryDefinitionEntry* entry );

	//! Remove entry, return false if had no such entry
	Bool RemoveEntry( CInventoryDefinitionEntry* entry );
private:
};

BEGIN_CLASS_RTTI( CInventoryDefinition );
	PARENT_CLASS( CEntityTemplateParam );
	PROPERTY( m_entries );
END_CLASS_RTTI();