/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "idTag.h"
#include "entityTemplate.h"
#include "../core/softHandle.h"

// Forward declaration
class CPeristentEntity;
class IGameDataStorage;
class IGameSaver;
class IGameLoader;
class EntitySpawnInfo;

struct SLayerStorageDumpInfo
{
	CClass* m_class;
	Uint32 m_size;
};

extern THashMap<CGUID,SLayerStorageDumpInfo> GLayerStorageDumpInfoMap;



/// Gameplay data storage for layer objects
class CLayerStorage
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_GameSave, MC_Gameplay );

protected:
	struct SpawnData
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_GameSave, MC_Gameplay );

	public:
		Vector								m_position;		//!< Spawn position
		Vector								m_scale;		//!< Spawn scale
		EulerAngles							m_rotation;		//!< Spawn rotation
		TagList								m_tags;			//!< Tag list
		String								m_name;			//!< Entity name
		TSoftHandle< CEntityTemplate >		m_template;		//!< Source template for dynamic entities
		CName								m_appearance;	//!< Entity appearance
		Uint8								m_entityFlags;	//!< Entity flags

		SpawnData();

		//! Store data
		void StoreData( IGameSaver* saver ) const;

		//! Restore data
		void RestoreData( IGameLoader* loader );

		//! Pull the state from the entity
		void PullData( CEntity* entity );

		//! Push the state to the entity spawn info
		void PushData( EntitySpawnInfo& spawnInfo );

		//! Get data size
		Uint32 GetDataSize() const;
	};

public:
	struct ComponentData
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_GameSave, MC_Gameplay );

	public:
		CGUID m_guid;
		IGameDataStorage* m_storedData;
		ComponentData* m_next;

		ComponentData();
		~ComponentData();

		Uint32 GetDataSize() const;
	};

	struct EntityData
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_GameSave, MC_Gameplay );

	public:
		IdTag							m_idTag;		//!< Entity IdTag
		IGameDataStorage*				m_data;			//!< Entity gameplay data
		SpawnData*						m_spawnData;	//!< Spawn data for dynamic entities
		ComponentData* 					m_componentsData;//!< List of streamed components' data

		EntityData();
		~EntityData();

		//! Store data
		void StoreData( IGameSaver* saver ) const;

		//! Restore data
		void RestoreData( IGameLoader* loader );

		//! Update stored data from entity
		void CaptureStateFrom( CPeristentEntity* entity );

		//! Apply stored state to entity
		void ApplyStateTo( CPeristentEntity* entity ) const;

		ComponentData* GetComponentData( const CGUID& guid );

		IGameSaver* GetComponentSaver( const CGUID& guid );
		IGameLoader* GetComponentLoader( const CGUID& guid );

		void ClearComponentData();
		Int32 GetNumComponents() const;

		//! Get data size
		Uint32 GetDataSize() const;
	};

	struct EntityDataByIdTagHashFunc
	{
		static RED_FORCE_INLINE Uint32 GetHash( const EntityData* owner ) { return owner->m_idTag.CalcHash(); }
		static RED_FORCE_INLINE Uint32 GetHash( const IdTag& tag ) { return tag.CalcHash(); }
	};
	struct EntityDataByIdTagEqualFunc
	{
		static RED_FORCE_INLINE Bool Equal( const EntityData* a, const EntityData* b ) { return a == b; }
		static RED_FORCE_INLINE Bool Equal( const EntityData* a, const IdTag& tag ) { return a->m_idTag == tag; }
	};
	typedef THashSet< EntityData*, EntityDataByIdTagHashFunc, EntityDataByIdTagEqualFunc > TEntityByGUIDSet;

	TEntityByGUIDSet	m_entities;			//!< Gameplay data for entities
	Bool				m_closed;			//!< Colsed storage don't accept data anymore

public:
	CLayerStorage();
	~CLayerStorage();

	//! Get number of entities in the storage
	RED_INLINE Uint32 GetNumEntities() const { return m_entities.Size(); }

	//! Restore managed entities
	void RestoreManagedEntities( CLayer* layer );

	//! Register managed entity from layer storage
	void RegisterManagedEntity( CPeristentEntity* entity );

	//! Unregister managed entity from layer storage
	void UnregisterManagedEntity( CPeristentEntity* entity );

	//! Restore entity state from layer storage
	void RestoreEntityState( CPeristentEntity* entity, CLayer* layer );

	//! Save entity state to layer storage
	void SaveEntityState( CPeristentEntity* entity );

	//! Remove entity state from storage
	Bool RemoveEntityState( const IdTag& idTag );
	Bool RemoveEntityState( CPeristentEntity* entity );

	//! Reset storage
	void ResetStorage();

	//! Close storage for good
	void PurgeAndCloseStorage();

	//! Updates storage from entities
	void UpdateStorage( CLayer* parentLayer );

	//! Store layer data
	void StoreData( IGameSaver* saver );

	//! Updates and stores layer data
	void UpdateAndStoreData( IGameSaver* saver, CLayer* parentLayer );

	//! Restore layer data
	void RestoreData( IGameLoader* loader );

	//! Restore player data
	Bool RestorePlayerEntityDataOnly( IGameLoader* loader, const IdTag& playerIdTag, const TDynArray< IdTag >& attachments );

	//! Gets whether layer storage has anything to save
	Bool ShouldSave( CLayer* parentLayer ) const;

	//! Get data size
	Uint32 GetDataSize() const;

	//! Get number of components in data storage
	Uint32 GetNumComponents() const;

	//! Transfers entity data from this storage to target storage; returns entity data if found, nullptr otherwise
	EntityData* TransferEntityData( CLayerStorage* targetStorage, const IdTag& idTag );

	//! Finds entity storage
	EntityData* FindEntityData( const IdTag& idTag );

	//! Returns entity storage
	CLayerStorage::EntityData* GetEntityData( const IdTag& idTag, Bool createIfNull );
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
