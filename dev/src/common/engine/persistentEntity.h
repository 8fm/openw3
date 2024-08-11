/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "entity.h"
#include "idTag.h"
#include "layerStorage.h"

class IGameSaver;
class IGameLoader;


#ifndef NO_SAVE_VERBOSITY
	class CSaveClassStatsCollector
	{
		// debug only
		// use only with vs! (clering memory on alloc) watch out!

		THashMap< CName, Uint32 >	m_entityClasses;
		THashMap< CName, Uint32 >	m_componentClasses;

	public:
		void Reset() { m_entityClasses.Clear(); m_componentClasses.Clear(); }
		void OnComponentClass( CName comp ) { ++m_componentClasses[ comp ];	}
		void OnEntityClass( CName ent ) { ++m_entityClasses[ ent ];	}

		void Dump()
		{
			RED_LOG( Save, TXT("************ Entity classes: ************") );
			for ( auto pair : m_entityClasses )
			{
				RED_LOG( Save, TXT("%ls: %ld"), pair.m_first.AsChar(), pair.m_second );
			}
			RED_LOG( Save, TXT("************ Component classes: ************") );
			for ( auto pair : m_componentClasses )
			{
				RED_LOG( Save, TXT("%ls: %ld"), pair.m_first.AsChar(), pair.m_second );
			}
				
		}
	};
#endif

/// Base class of all saveable entities
class CPeristentEntity : public CEntity
{
	DECLARE_ENGINE_CLASS( CPeristentEntity, CEntity, 0 );

private:
	IdTag				m_idTag;
	Bool				m_isSaveable;

public:
	//! Get entity IdTag
	RED_INLINE const IdTag& GetIdTag() const { return m_idTag; }

	//! HACK: Check if the entity is marked as savable 
	// (created for lights as they have to be gameplayEntities but their state shouldn't be saved)
	RED_INLINE Bool IsSavable() const { return m_isSaveable; }

public:
	CPeristentEntity();
	virtual ~CPeristentEntity();

	// Entity was attached to world
	virtual void OnAttached( CWorld* world );

	// Entity was detached from world
	virtual void OnDetached( CWorld* world );

	// Entity was created, not yet attached or initialized
	virtual void OnCreated( CLayer* layer, const EntitySpawnInfo& info );

	//! Called when we need to store gameplay state of this entity
	virtual void OnSaveGameplayState( IGameSaver* saver );

	void SaveState( IGameSaver* saver, ISaveFile* directStream ); 
	void RestoreState( IGameLoader* loader, ISaveFile* directStream, Uint32 version ); 

	virtual void SaveStreamData( ISaveFile* directStream ); 
	virtual void LoadStreamData( ISaveFile* directStream, Uint32 version ); 

	void SaveComponents( IGameSaver* saver );

	void SaveProperties( IGameSaver* saver );

	void SaveAutoEffects( IGameSaver* saver );

	//! Called when we need to restore gameplay state of this entity
	virtual void OnLoadGameplayState( IGameLoader* loader );

	void LoadComponents( IGameLoader* loader );

	void LoadProperties( IGameLoader* loader );

	void LoadAutoEffects( IGameLoader* loader );

	//! Entity was pasted in editor
	virtual void OnPasted( CLayer* layer );

	//! Called just before layer is saved in editor
	virtual void OnLayerSavedInEditor();

	// Object was loaded
	virtual void OnPostLoad();

	//! Should save?
	virtual Bool CheckShouldSave() const;

	//! Find attached persistent entity by IdTag
	static CPeristentEntity* FindByIdTag( const IdTag& tag );
	void ConvertToManagedEntity( const IdTag& dynamicTag );
	void ForgetTheState();

	// use for cactor/player which is in game project so I have to put it here to call it from performprefetch (engine project)
	virtual void ResetClothAndDangleSimulation() {};

	void SetIdTag( const IdTag& idTag );

protected:
	Bool ShouldLoadAndSaveComponentsState();
	virtual void LoadStreamedComponentsState() override;
	virtual void SaveStreamedComponentsState() override;
	void SaveStreamedComponent( CComponent* component, CLayerStorage::EntityData* entityData );

	CLayerStorage::EntityData* GetEntityData( Bool createIfNull );
	
	void SetShouldSave( Bool should );

	//! Invoked after setting value for m_autoPlayEffectName
	virtual void OnSetAutoPlayEffectName() override;

private:
	void RegisterToSearchMapIfNeeded();
	void UnregisterFromSearchMapIfNeeded();
};

BEGIN_CLASS_RTTI( CPeristentEntity );
	PARENT_CLASS( CEntity );
	PROPERTY_RO( m_idTag, TXT( "A PESEL number for the gamplay-relevant entity." ) );
	PROPERTY_EDIT( m_isSaveable, TXT("Save entity state on game save") );
END_CLASS_RTTI();
