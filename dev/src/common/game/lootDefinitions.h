/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class ILootManager
{
public:

	virtual Int32 GetItemMaxCount( const CName& itemName )  = 0;
	virtual Bool UpdateItemMaxCount( const CName& itemName, Uint32 generatedQuantity ) = 0;
};

class CLootDefinitionBase
{
public:

	RED_INLINE CLootDefinitionBase() : m_name( CName::NONE ) { }
	RED_INLINE CLootDefinitionBase( const CName& name ) : m_name( name ) { }
	RED_INLINE const CName& GetName() const { return m_name; }
	RED_INLINE void SetName( const CName& name ) { m_name = name; }

protected:

	CName	m_name;

private:

	DECLARE_RTTI_SIMPLE_CLASS( CLootDefinitionBase )
};

BEGIN_CLASS_RTTI( CLootDefinitionBase );
END_CLASS_RTTI();

class CLootDefinitions
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Gameplay );
public:

	static const String NODE_ROOT;
	static const String NODE_DEFINITIONS;
	static const String NODE_LOOT_DEFINITIONS;

	virtual ~CLootDefinitions() { }

	virtual CLootDefinitionBase* GetDefinition( const CName& name ) { return NULL; }
	virtual const CLootDefinitionBase* GetDefinition( const CName& name ) const { return NULL; }
	virtual Bool GetDefinitions( TDynArray< CLootDefinitionBase* > & definitions ) { return false; }
	virtual Bool GetDefinitions( TDynArray< const CLootDefinitionBase* > & definitions ) const { return false; }
	virtual Bool GetDefinitionsNames( TDynArray< CName > & names ) const { return false; }	
	virtual Bool ChooseLootEntries( const CEntity* entity, TDynArray< SLootEntry > & entries, const CInventoryComponent* inventory ) const { return false; }
	virtual Bool ChooseLootEntries( const CLootDefinitionBase* definition, TDynArray< SLootEntry > & entries, const CInventoryComponent* inventory  ) const { return false; }

	virtual Bool AddNewDefinition( const CName& name, CLootDefinitionBase** newDefinition ) { return false; }
	virtual Bool RemoveDefinition( const CName& name ) { return false; }
	virtual Bool IsDefinitionNameUnique( const CName& name ) const;

	virtual Bool Clear() { return true; }
	virtual Bool Load( CXMLReader* reader, const Red::System::GUID& creatorTag ) { return false; }
	virtual Bool ReadNode( CXMLReader* reader, const Red::System::GUID& creatorTag ) { return false; }
	virtual Bool Save( CXMLWriter* writer ) { return false; }

	virtual Bool RemoveDefinitions( const Red::System::GUID& creatorTag ) { return false; }

	virtual Bool UpdateDefinitionName( const CName& oldName, const CName& newName ) { return false; }
	virtual Bool GetLootDefinitionsFilenames( TDynArray< String > & filenames ) const;

	virtual void ValidateLootDefinitions( bool listAllItemDefs ) { };

protected:

	Bool FileHasOnlyOneLootDefinitionNode( const String& filename ) const;
};