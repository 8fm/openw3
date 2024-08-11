/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "idTag.h"
#include "../core/latentDataBuffer.h"
#include "../core/resource.h"
#include "../core/events.h"

class CWorld;
class CLayerInfo;
class CEntityTemplate;
class CBrushCompiledData;
class CLayerCooker;
class CLayerStorage;
class IJobEntitySpawn;
class CJobSpawnEntity;
class CJobSpawnEntityList;
class CSectorData;

#define DECLARE_NAMED_EVENT_HANDLER( name )																								\
	public:																																\
	static CName EventHandlerName() { return CNAME( name ); }																			\
	Bool IsA( CName eventClass ) override { if ( eventClass == CNAME( name ) ) return true; return Super::IsA( eventClass ); }

class ISpawnEventHandler
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Engine );

	friend class EntitySpawnInfo;
public:
	ISpawnEventHandler()
		: m_next( NULL )
	{}

	virtual ~ISpawnEventHandler();

	virtual void OnPostAttach( CEntity* entity );

	virtual Bool IsA( CName eventClass );

	static CName EventHandlerName()								{ return CName::NONE; }	

private:
	ISpawnEventHandler*			m_next;
};

/// Information about how to spawn a new entity
class EntitySpawnInfo
{
public:
	String							m_name;						//!< Entity name
	TDynArray< CName >				m_appearances;				//!< Appearances to choose from, or choose from all if empty
	TagList							m_tags;						//!< Tags to use
	THandle< CEntityTemplate >		m_template;					//!< Entity template to spawn
	Red::TUniquePtr< ISpawnEventHandler > m_handler;			//!< Handler to be used for processing
	CResource*						m_resource;					//!< Spawn resource ( e.g. mesh for StaticMesh template )
	CClass*							m_entityClass;				//!< Class of entity to spawn
	Vector							m_spawnPosition;			//!< Initial entity position
	EulerAngles						m_spawnRotation;			//!< Initial entity rotation
	Vector							m_spawnScale;				//!< Initial entity scale
	IdTag							m_idTag;					//!< Allocated IdTag for entity, only dynamic IdTags are supported
	CGUID							m_guid;						//!< GUID to apply to entity
	Bool							m_detachTemplate;			//!< Detach from entity template
	Bool							m_previewOnly;				//!< Entity is spawned for preview, do not remove inactive component	
	Bool							m_canThrowOutUnusedComponents;//!< Can throw out components not used in appearances?
	Bool							m_entityNotSavable;			//!< (Gameplay) entity doesnt have to be saved when unloading layer
	Bool							m_importantEntity;			//!< Is this entity important or can we skip spawning due to performance issues
	Bool							m_forceNonStreamed;			//!< Should this entity be set to non streamable even though in data it is set so
	Uint8							m_entityFlags;				//!< Entity flags to set
	Int32							m_encounterEntryGroup;		//!< CBaseCreatureEntry::m_group

public:
	EntitySpawnInfo();
	EntitySpawnInfo( EntitySpawnInfo&& c );
	EntitySpawnInfo& operator=( EntitySpawnInfo&& c );
	~EntitySpawnInfo();

	void AddHandler( ISpawnEventHandler* handler );
	void OnPostAttach( CEntity* entity ) const;
	void Swap( EntitySpawnInfo& c );

	template < class TSpawnHandler >
	class HandlerIterator
	{
	protected:
		ISpawnEventHandler*				m_handler;
		CName							m_handlerTypeName;

		void							SkipFiltered()			{ while( m_handler ) { if ( m_handler->IsA( TSpawnHandler::EventHandlerName() ) ) { return; } m_handler = m_handler->m_next; } }
	public:
		HandlerIterator( const EntitySpawnInfo& spawnInfo )		{ m_handler = spawnInfo.m_handler.Get(); SkipFiltered(); }

		operator Bool() const									{ return m_handler != nullptr; }
		void Next()												{ m_handler = m_handler->m_next; SkipFiltered(); }
		TSpawnHandler* operator*() const						{ return static_cast< TSpawnHandler* >( m_handler ); }
	};

private:

	EntitySpawnInfo( const EntitySpawnInfo & );
	EntitySpawnInfo & operator=( const EntitySpawnInfo & );
};



struct SLayerMemorySnapshot
{
	String	m_name;

	Uint32	m_memStatic;
	Uint32	m_memSerialize;
	Uint32	m_memDynamic;

	Uint32	m_entitiesNum;
	Uint32	m_attachedCompNum;

	SLayerMemorySnapshot() : m_memStatic( 0 ), m_memSerialize( 0 ), m_memDynamic( 0 ), m_entitiesNum( 0 ), m_attachedCompNum( 0 ) {}
};

// TODO: REMOVE
struct SStreamedDetachedEntityData
{
	CGUID				m_entityGuid;
	LatentDataBuffer	m_data;

	Bool operator==( const SStreamedDetachedEntityData& rhs ) const
	{
		return m_entityGuid == rhs.m_entityGuid;
	}
};

typedef TDynArray< CEntity*, MC_LayerSystem > LayerEntitiesArray;

/// Base layer - a high level building block for world
class CLayer
:	public CResource
#ifndef NO_EDITOR_EVENT_SYSTEM
,	public IEdEventListener
#endif
{
	DECLARE_ENGINE_RESOURCE_CLASS( CLayer, CResource, "w2l", "World layer" );

	friend class CWorld;
	friend class CLayerCooker;

protected:
	CWorld*					m_world;				//!< World this layer is attached to
	LayerEntitiesArray		m_entities;				//!< List of entities in this layer
	Uint32					m_nameCount;			//!< Name counter
	Bool					m_cached;				//!< Informs, whether this layer should be cached
	CLayerInfo*				m_layerInfo;			//!< Layer info pointing to this layer
	Int32					m_numAttached;			//!< Number of components attached to this layer
	THandle< CSectorData >	m_sectorData;			//!< Cooked sector data (note: it's temporary placed in the layer, should be at the larger scope)
	Uint32					m_sectorDataId;			//!< ID of the sector data registered in world

public:
	//! Should this layer be cached ?
	RED_INLINE Bool ShouldBeCached() const { return m_cached; }

	//! Disable caching for this layer
	RED_INLINE void DisableCaching() { m_cached = false; }

	//! Does this layer have any entities ?
	RED_INLINE Bool HasEntities() const { return m_entities.Size() > 0; }

	//! Does this layer have given entity
	RED_INLINE Bool HasEntity( CEntity* entity ) const { return m_entities.Exist( entity ); }

	//! (DEPRECATED) Get number of components attached on this layer
	RED_INLINE Int32 GetNumAttachedComponents() const { return m_numAttached; }

	//! (DEPRECATED) Count up/down the number of attached components
	RED_INLINE void UpdateAttachedComponentsCount( const Int32 delta ) { m_numAttached += delta; }

	//! Get compiled brush data for this layer
	RED_INLINE CBrushCompiledData* GetBrushCompiledData() const { return /*m_brushCompiledData*/ NULL ; }

	//! Get entity list
	RED_MOCKABLE RED_INLINE const LayerEntitiesArray& GetEntities() const { return m_entities; }

	//! Get sector data
	RED_INLINE THandle< CSectorData > GetSectorData() const { return m_sectorData; }

public:
	CLayer();

#ifndef NO_EDITOR_RESOURCE_SAVE
	//! Called just before this resource is saved in editor
	virtual void OnResourceSavedInEditor();
#endif

	//! Called after file save
	virtual void OnSave() override;

	//! Raw serialization
	virtual void OnSerialize( IFile& file ) override;

	//! Cooking
#ifndef NO_RESOURCE_COOKING
	virtual void OnCook( class ICookerFramework& cooker ) override;
#endif

	//! Debug info
#ifndef NO_DEBUG_PAGES
	virtual void OnDebugPageInfo( class CDebugPageHTMLDocument& doc );
#endif

	//! Finalize object before destruction
	virtual void OnFinalize() override;

	//! Loaded
	virtual void OnPostLoad() override;
	
#ifndef NO_DATA_VALIDATION
	//! Check data
	virtual void OnCheckDataErrors() const;
#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	//! Reload
	virtual Bool Reload( Bool confirm );
#endif
	
public:
	// Set this layer's layer info
	void SetLayerInfo( CLayerInfo* layerInfo );

	// Get object GUID
	const CGUID& GetGUID() const;

	// Get this layer's layer info
	CLayerInfo* GetLayerInfo() const;

	// Get world we are attached to
	CWorld* GetWorld() const{ return m_world;}

	// Is this layer attached to world ?
	Bool IsAttached() const;

	// Attach layer to given world
	void AttachToWorld( CWorld* world );

	// Detach layer from world
	void DetachFromWorld();

	// Layer was loaded and will be attached
	void LayerLoaded();

public:
	// Get list of entities
	void GetEntities( TDynArray< CEntity* >& entities ) const;

	// Validate entity spawn info
	RED_MOCKABLE Bool ValidateSpawnInfo( const EntitySpawnInfo& info ) const;

	// Create new entity in this layer, done in the background
	CJobSpawnEntity* CreateEntityAsync( EntitySpawnInfo&& info );

	// Create new entity in this layer, done in the background
	CJobSpawnEntityList* CreateEntityListAsync( TDynArray< EntitySpawnInfo >&& infos );

	// Create new entity in this layer, synchronous, slow
	CEntity* CreateEntitySync( const EntitySpawnInfo& info );

	// Add cloned entity to the layer
	void AddClonedEntity( CEntity *entity );

	// Destroy all entities in this layer + empty the m_entities array
	void DestroyAllEntities();

	// Attach entity that has been removed from some other layer ( UNDO )
	void AddEntity( CEntity* entity );

#ifndef NO_EDITOR
	void ForceAddEntity( CEntity* entity );
#endif
	// Detach entity that belongs to this layer, but do not destroy it (it may be added to other layer) ( UNDO )
	void RemoveEntity( CEntity* entity );

	// Link spawned entity with this layer ( low level )
	void LinkEntityWithLayer( CEntity* entity, const EntitySpawnInfo& spawnInfo );

protected:
	void RestoreEntityState( CEntity* entity );

	void StoreEntityState( CEntity* entity );

public:

	// Find entity with given name
	CEntity* FindEntity( const String& name ) const;

	// Find entity with given guid
	CEntity* FindEntity( const CGUID& guid ) const;

	// Move entity from this layer to another
	Bool MoveEntity( CEntity* entity, CLayer *layer );

	// Notify layer entities that we are about to get unloaded
	void NotifyUnloading();

	// Generate unique entity name
	String GenerateUniqueEntityName( const String& base );

	// Generate unique entity name (cheap version)
	static void GenerateUniqueEntityNameCheap( CEntity* entity, const String& base, String& outStr );

	// Paste entities serialized in given memory pool, if no spawn translation is given then entities are spawned in original position
	Bool PasteSerializedEntities( const TDynArray< Uint8 >& data, LayerEntitiesArray& pastedEntities, Bool relativeSpawn, const Vector& spawnPosition, const EulerAngles& spawnRotation );

	// Remove shadows from this layer
	void RemoveShadowsFromLayer();

	// Add shadows to this layer
	void AddShadowsToLayer();

	// Add shadows from local lights to this layer
	void AddShadowsFromLocalLightsToLayer();

	// Create brush compiled data object for this layer
	CBrushCompiledData* CreateBrushCompiledData();

#ifndef NO_RESOURCE_USAGE_INFO
	//! Resource usage reporting
	void CollectResourceUsage( class IResourceUsageCollector& collector ) const;
#endif

#ifndef NO_EDITOR
	void ConvertToStreamed(bool templatesOnly = false);

	Bool IsCommunityLayer();
#endif
	
public:
	// Should we prevent GC from collecting this resource
	virtual Bool PreventCollectingResource() const;

	// Should we prevent from losing changes to this resource when unloading
	virtual Bool PreventFullUnloading() const;

protected:
#ifndef NO_EDITOR_EVENT_SYSTEM
	// Process event
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );
#endif	
	// Recreate entities created from given template
	void RecreateEntieties( CEntityTemplate* entTemplate );

protected:
	virtual void OnEntityAdded( const CEntity* entity );
	virtual void OnEntityRemoved( const CEntity* entity );

public:
	// Get layer storage for entities
	virtual CLayerStorage* GetLayerStorage();

	Uint32 CalcDataSize() const;
	void CalcMemSnapshot( SLayerMemorySnapshot& snapshot ) const;

	// Destroy entity that belongs to this layer
	void DestroyEntity( CEntity* entity );

private:

	void RemoveEmptyEntityPointers();
};

BEGIN_CLASS_RTTI( CLayer );
	PARENT_CLASS( CResource );
	PROPERTY( m_entities );
	PROPERTY( m_sectorData );
	PROPERTY( m_nameCount );
END_CLASS_RTTI();
