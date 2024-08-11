/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

// Forward declarations
class CEntityList;
class CEntityListManager;

// List actions handler
typedef void (*FEntityListActionsHandler)( CEntityList* entityList, const LayerEntitiesArray& entities, void* userData );

// Single entry in an entity list
struct SEntityListEntry
{
	CGUID				m_guid;		//!< Entity GUID
	String				m_name;		//!< Entity name
	String				m_path;		//!< Entity path
	THandle< CEntity >	m_entity;	//!< Handle to an entity

	void SetEntity( CEntity* entity );
};

// An entity list
class CEntityList
{
	friend class CEntityListUI;
	friend class CEntityListManager;

	String							m_name;			//!< Entity list name
	TDynArray< SEntityListEntry >	m_entries;		//!< Entity list entries
	THashSet< THandle< CEntity > >	m_entities;		//!< Entities set for quick duplicate checks
	CEntityListManager*				m_manager;		//!< Entity list managet this list belongs to
	class CEntityListUI*			m_ui;			//!< Entity list UI
	FEntityListActionsHandler		m_actions;		//!< List-specific Actions handler
	void*							m_actionsData;	//!< Handler user data

	//! Refresh the entity list
	void Refresh();

	//! The list has been modified
	void ListModified();

	//! Construct an entity list in the given name and manager
	CEntityList( const String& name, CEntityListManager* manager );

	// Destructor
	~CEntityList();

public:
	//! Returns the name of this entity list
	RED_INLINE const String& GetName() const { return m_name; }

	//! Returns the entries of this entity list
	RED_INLINE const TDynArray< SEntityListEntry >& GetEntries() const { return m_entries; };

	//! Returns the manager of this entity list
	RED_INLINE CEntityListManager* GetManager() const { return m_manager; }

	//! Clears this entity list
	void Clear();

	//! Adds a new entity to this list
	void Add( CEntity* entity );

	//! Adds the entity in the array to this list
	void Add( const TDynArray< CEntity* >& entities );

	//! Removes the given entity from this list
	void Remove( CEntity* entity );

	//! Scans the list for the given entity, returns true if found
	Bool GetIndex( CEntity* entity, Uint32& index ) const;

	//! Returns true if the given entity exists in the list
	Bool Exist( CEntity* entity ) const;

	//! Returns the actions handler for this list
	FEntityListActionsHandler GetActionsHandler() const;

	//! Sets the actions handler for this list. If null, this will use the manager's actions handler
	void SetActionsHandler( FEntityListActionsHandler handler, void* userData );

	//! Saves the list to the given file
	void SaveToFile( const String& absolutePath ) const;

	//! Loads the list from the given file
	Bool LoadFromFile( const String& absolutePath );

	//! Shows the entity list in the given entity list manager
	void Show();

	//! Hides the entity list in the given entity list manager
	void Hide();

	//! Returns true if the list is visible
	Bool IsVisible() const;

	//! Returns the world markers for this entity list
	const TDynArray<Vector>& GetWorldMarkers() const;
};

// Entity list manager
class CEntityListManager
{
	friend class CEntityList;
	friend class CEntityListUI;

	CWorld*								m_world;
	TDynArray< CEntityList* >			m_lists;		//!< All managed entity lists
	THashMap< String, CEntityList* >	m_listByName;	//!< Name-to-list map
	FEntityListActionsHandler			m_actions;		//!< Manager-wide sctions handler
	void*								m_actionsData;	//!< Handler user data

	// Called by CEntityList to notify about a modification
	void ListModified( CEntityList* list );

public:
	// Create a new entity list manager for the given world
	CEntityListManager( CWorld* world );

	// Returns the world of this manager
	RED_INLINE CWorld* GetWorld() const { return m_world; }

	// Create or (if createNew is true) create a new list using the given name
	CEntityList* FindByName( const String& listName, Bool createNew = true );

	// Returns all managed lists
	RED_INLINE const TDynArray< CEntityList* >& GetAllManagedLists() const { return m_lists; }

	//! Sets the actions handler for this manager
	void SetActionsHandler( FEntityListActionsHandler handler, void* userData );
};
