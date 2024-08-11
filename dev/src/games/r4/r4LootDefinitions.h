/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/game/lootDefinitions.h"

class CR4LootDefinitionBase : public CLootDefinitionBase
{
public:

	CR4LootDefinitionBase();
	CR4LootDefinitionBase( const CName& name );

	RED_INLINE Uint32 GetQuantityMin() const { return m_quantityMin; }
	RED_INLINE Uint32 GetQuantityMax() const { return m_quantityMax; }
	Uint32 GetRandomQuantity( Uint32 seed = 0 ) const;
	Bool CheckPlayerLevel( Uint32 playerLevel ) const;
	Bool CheckCrafterLevel( Uint32 crafterLevel ) const;

	RED_INLINE const Red::System::GUID& GetCreatorTag() const { return m_creatorTag; }

protected:

	Bool	ReadBaseAttributes( CXMLReader* reader, const Red::System::GUID& creatorTag );
	Bool	WriteBaseAttributes( CXMLWriter* writer );
	void	Validate();

	Uint32	m_quantityMin;
	Uint32	m_quantityMax;
	Uint32	m_playerLevelMin;
	Uint32	m_playerLevelMax;
	Uint32	m_crafterLevelMin;
	Uint32	m_crafterLevelMax;

	Red::System::GUID					m_creatorTag; //! tag is used for determination of definition creator (e.g. DLC mounter)
		
	DECLARE_RTTI_SIMPLE_CLASS_WITH_ALLOCATOR( CR4LootDefinitionBase, MC_Gameplay );
};

BEGIN_CLASS_RTTI( CR4LootDefinitionBase );
	PARENT_CLASS( CLootDefinitionBase );
	PROPERTY_EDIT( m_quantityMin, TXT( "Min. quantity" ) );
	PROPERTY_EDIT( m_quantityMax, TXT( "Max. quantity" ) );
	PROPERTY_EDIT( m_playerLevelMin, TXT( "Min. player level" ) );
	PROPERTY_EDIT( m_playerLevelMax, TXT( "Max. player level" ) );
	PROPERTY_EDIT( m_crafterLevelMin, TXT( "Min. crafter level" ) );
	PROPERTY_EDIT( m_crafterLevelMax, TXT( "Max. crafter level" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CR4LootItemDefinition : public CR4LootDefinitionBase
{
public:

	CR4LootItemDefinition()
		: m_chance( 100.0f )
	{}

	Bool ReadAttributes( CXMLReader* reader, const Red::System::GUID& creatorTag );
	Bool WriteAttributes( CXMLWriter* writer );

	Float GetChance() const { return m_chance; }

private:

	Float	m_chance;

	DECLARE_RTTI_SIMPLE_CLASS( CR4LootItemDefinition )
};

BEGIN_CLASS_RTTI( CR4LootItemDefinition );
	PARENT_CLASS( CR4LootDefinitionBase );
	PROPERTY_CUSTOM_EDIT( m_name, TXT( "Item name" ), TXT( "ItemSelection" ) );
	PROPERTY_EDIT( m_chance, TXT( "Chance" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CR4LootContainerDefinition : public CR4LootDefinitionBase
{
public:

	CR4LootContainerDefinition()
		: m_respawnTime( 0 )
	{}
	CR4LootContainerDefinition( const CName& name )
		: CR4LootDefinitionBase( name )
		, m_respawnTime( 0 )
	{}

	Uint32 GetRespawnTime() const { return m_respawnTime; }
	Bool GetItems( TDynArray< const CR4LootItemDefinition* > & items ) const;
	const TDynArray< CR4LootItemDefinition > & GetItems() const;

	void CopyItems( const CR4LootContainerDefinition& lootContainerDefinition );

	Bool ReadAttributes( CXMLReader* reader, const Red::System::GUID& creatorTag );
	Bool WriteAttributes( CXMLWriter* writer );

	Bool RemoveDefinitions( const Red::System::GUID& creatorTag );

private:

	Uint32								m_respawnTime;
	TDynArray< CR4LootItemDefinition >	m_items;

	DECLARE_RTTI_SIMPLE_CLASS( CR4LootContainerDefinition )
};

BEGIN_CLASS_RTTI( CR4LootContainerDefinition );
	PARENT_CLASS( CR4LootDefinitionBase );
	PROPERTY_EDIT( m_name, TXT( "Container (unique) name" ) );
	PROPERTY_EDIT( m_respawnTime, TXT( "Respawn time. Can be overriden by entity param value." ) );
	PROPERTY_EDIT( m_items, TXT( "Loot items" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CR4LootParam;
class CR4LootContainerParam;

class CR4LootDefinitions : public CLootDefinitions
{

public:
	CLootDefinitionBase* GetDefinition( const CName& name ) override;
	const CLootDefinitionBase* GetDefinition( const CName& name ) const override;
	Bool GetDefinitions( TDynArray< CLootDefinitionBase* > & definitions ) override;
	Bool GetDefinitions( TDynArray< const CLootDefinitionBase* > & definitions ) const override;
	Bool GetDefinitionsNames( TDynArray< CName > & names ) const override;
	Bool ChooseLootEntries( const CEntity* entity, TDynArray< SLootEntry > & entries, const CInventoryComponent* inventory  ) const override;
	Bool ChooseLootEntries( const CLootDefinitionBase* definition, TDynArray< SLootEntry > & entries, const CInventoryComponent* inventory  ) const override;

	Bool AddNewDefinition( const CName& name, CLootDefinitionBase** newDefinition ) override;
	Bool RemoveDefinition( const CName& name ) override;

	Bool RemoveDefinitions( const Red::System::GUID& creatorTag ) override;

	Bool Clear() override;
	Bool Load( CXMLReader* reader, const Red::System::GUID& creatorTag ) override;
	Bool ReadNode( CXMLReader* reader, const Red::System::GUID& creatorTag ) override;
	Bool Save( CXMLWriter* writer ) override;

	Bool UpdateDefinitionName( const CName& oldName, const CName& newName ) override;

	void ValidateLootDefinitions( bool listAllItemDefs ) override;

private:

	// a decorator for CR4LootItemDefinition with additional "area max count" functionality
	struct SR4LootItemDescriptor
	{
		DECLARE_STRUCT_MEMORY_POOL_ALIGNED( MemoryPool_SmallObjects, MC_Gameplay, __alignof(SR4LootItemDescriptor) );

		const CR4LootItemDefinition*	m_definition;
		Int32							m_areaMaxCount;

		SR4LootItemDescriptor();
		SR4LootItemDescriptor( const CR4LootItemDefinition* definition, Int32 areaMaxCount );

		CName GetName() const;
		Float GetChance() const;
		Bool CheckPlayerLevel( Uint32 playerLevel ) const;	
		Bool CheckCrafterLevel( Uint32 crafterLevel ) const;
		Uint32 GetQuantityMin() const;
		Uint32 GetQuantityMax() const;
		Bool HasAreaLimit() const;
	};

	// a facade unifying CR4LootContainerParam and CR4LootContainerDefinition interfaces
	struct SR4LootContainerDescriptor
	{
		DECLARE_STRUCT_MEMORY_POOL_ALIGNED( MemoryPool_SmallObjects, MC_Gameplay, __alignof(SR4LootContainerDescriptor) );

		const CR4LootContainerParam*		m_param;
		const CR4LootContainerDefinition*	m_definition;

		SR4LootContainerDescriptor();
		SR4LootContainerDescriptor( const CR4LootContainerDefinition* definition );
		SR4LootContainerDescriptor( const CR4LootContainerParam* param, const CR4LootContainerDefinition* definition );

		CName GetName() const;
		Float GetChance() const;
		Bool CheckPlayerLevel( Uint32 playerLevel ) const;
		Bool CheckCrafterLevel( Uint32 crafterLevel ) const;
		Uint32 GetRespawnTime();
	};

	Bool ChooseLootEntries( TDynArray< SR4LootContainerDescriptor* > & containers, Uint32 maxContainersCount, TDynArray< SLootEntry > & entries, const CInventoryComponent* inventory ) const;
	Bool ChooseLootEntries( const CR4LootParam* lootParam, TDynArray< SLootEntry > & entries, const CInventoryComponent* inventory  ) const;
	Bool CreateItemsDescriptors( const TDynArray< CR4LootItemDefinition > & itemsDefs, TDynArray< SR4LootItemDescriptor > & items ) const;
	Bool CreateContainersDescriptors( const TDynArray< const CR4LootContainerParam* > & params, TDynArray< SR4LootContainerDescriptor* > & containers ) const;

	typedef THashMap< CName, CR4LootContainerDefinition > TContainersMap;
	TContainersMap m_containers;
};

//////////////////////////////////////////////////////////////////////////

class CR4LootContainerParam
{
public:
	CR4LootContainerParam()
		: m_name( CName::NONE )
		, m_chance( 100.0f )
		, m_respawnTime( 0 )
	{}

	CName GetName() const { return m_name; }
	Float GetChance() const { return m_chance; }
	Uint32 GetRespawnTime() const { return m_respawnTime; }

private:

	CName		m_name;
	Float		m_chance;
	Uint32		m_respawnTime;

	DECLARE_RTTI_SIMPLE_CLASS_WITH_ALLOCATOR( CR4LootContainerParam, MC_Gameplay );
};

BEGIN_CLASS_RTTI( CR4LootContainerParam );
	PROPERTY_CUSTOM_EDIT( m_name, TXT( "Container name" ), TXT( "LootNameSelectionEditor" ) );
	PROPERTY_EDIT( m_chance, TXT( "Chance" ) );
	PROPERTY_EDIT( m_respawnTime, TXT( "Respawn time. If equal to 0 value from container definition will be used" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CR4LootParam : public CGameplayEntityParam
{
public:
	CR4LootParam();
	Uint32 GetUsedContainersCount( Uint32 seed ) const;
	Bool GetContainersParams( TDynArray< const CR4LootContainerParam* > & containers ) const;

	RED_INLINE Bool IsAlwaysPresent() { return m_alwaysPresent; }

protected:

	TDynArray< CR4LootContainerParam >	m_containers;
	Uint32								m_usedContainersMin;
	Uint32								m_usedContainersMax;
	Bool								m_alwaysPresent;

	DECLARE_ENGINE_CLASS_WITH_ALLOCATOR( CR4LootParam, CGameplayEntityParam, MC_Gameplay );
};

BEGIN_CLASS_RTTI( CR4LootParam );
	PARENT_CLASS( CGameplayEntityParam );
	PROPERTY_EDIT( m_containers, TXT( "Loot containers params list" ) );
	PROPERTY_EDIT( m_usedContainersMin, TXT( "Minimum number of loot containers to use" ) );
	PROPERTY_EDIT( m_usedContainersMax, TXT( "Maximum number of loot containers to use" ) );
	PROPERTY_EDIT( m_alwaysPresent, TXT( "Specifies if it's always present (not randomized)" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SR4LootNameProperty
{
	DECLARE_RTTI_STRUCT( SR4LootNameProperty )

	SR4LootNameProperty()
		: m_lootName( CName::NONE )
	{}

	CName m_lootName;
};

BEGIN_CLASS_RTTI( SR4LootNameProperty )
	PROPERTY_CUSTOM_EDIT( m_lootName, TXT( "Loot definition name" ), TXT( "LootNameSelectionEditor" ) )
END_CLASS_RTTI()