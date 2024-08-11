/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "inventoryComponent.h"

class CItemEntityProxy;
class CItemEntityManager;
struct SItemSpawnInfo;

// Interface for claiming dropped item when it's timer finishes
class IDroppedItemClaimer
{
public:
	//! Destructor
	virtual ~IDroppedItemClaimer() { }

	//! Item entity dropped on the floor timer finished, return true if proxy ownership was taken and proxy shouldn't be destroyed
	virtual Bool OnDroppedItemTimeOut( CItemEntityProxy* proxy ) = 0;
};

class CItemEntityProxy
{
	friend class CItemEntityManager;

	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Gameplay );

public:
	CName					m_itemName;					//<! Name of the item in items definitions
	TDynArray< CName >		m_slotItems;				//<! List of enhancement items inserted	
	THandle< CEntity >		m_parentAttachmentEntity;	//<! Entity having the item attached
	CLayer*					m_subjectLayer;				//<! Layer for this proxy's entity
	CName					m_slotName;					//<! Name of the slot in parent entity i'm attached to
	String					m_template;
	Bool					m_collapse;
	Bool					m_invisible;
	Bool					m_dirty;					//<! New item is loading
	SItemUniqueId			m_item;

private:
	CItemEntityProxy( CName itemName, CLayer* layer, const String& templateName, Bool collapse, Bool invisible, SItemUniqueId item )
		: m_itemName( itemName )
		, m_subjectLayer( layer )
		, m_template( templateName )
		, m_collapse( collapse )  
		, m_invisible( invisible )
		, m_dirty( true )
		, m_item( item )
	{ 

	}
	~CItemEntityProxy() {}

public:
	//! Setup item entity attachment info
	void SetAttachment( CEntity* parentEntity, CName slotName = CName::NONE );

	//! Reattach once again to the same entity and slot
	void Reattach();

	//! Skin item on given entity
	void SkinOn( CEntity* parentEntity );
	
	//! Enable collision info reporting on entity
	void EnableCollisionInfoReporting();

	//! Reconfigure proxy for new item
	void ChangeItem( CName itemName, CName slotOverride, const TDynArray< CName >& slotItems, const String& templateName, Bool collapse, SItemUniqueId item );

	//! Get entity this item is attached to, returning NULL is treated as if the item was dropped
	CEntity* GetParentEntity() const { return m_parentAttachmentEntity.Get(); }

	//! Is item owned by the player
	Bool IsPlayerEntityItem() const { return Cast< CPlayer >( GetParentEntity() ) != nullptr; }

	//! Get spawn info for the proxy
	Bool GetItemSpawnInfo( SItemSpawnInfo& itemSpawnInfo ) const;
};

class CItemEntity : public CEntity
{
	DECLARE_ENGINE_CLASS( CItemEntity, CEntity, 0 );

	friend class CItemEntityManager;

protected:
	Float							m_timeToDespawn;	//<! After item is dropped, it will disappear after that time
	THandle<CEntity>				m_parentEntity;		//<! Entity this item is attached to
	THandle<CComponent>				m_rootMesh;			//<! Root mesh component of the item
	CItemEntityProxy*				m_proxy;			//<! Item entity proxy this item is bound to
	Bool							m_reportToScript;	//<! Report changes to script
	Bool							m_customAttacht;
	Bool							m_customDetacht;
public:
	CItemEntity();
	virtual ~CItemEntity() { }

	virtual Bool CustomAttach(){ return true; }
	virtual void CustomDetach(){}

	// Entity was attached to world
	virtual void OnAttached( CWorld* world ) override;

	// Entity was detached from world
	virtual void OnDetached( CWorld* world ) override;

	// Entity is being destroyed
	virtual void OnDestroyed( CLayer* layer ) override;

	virtual const IActorInterface* QueryActorInterface() const override;

#ifndef NO_DATA_VALIDATION
	// Check data, can be called either for node on the level or for node in the template
	virtual void OnCheckDataErrors( Bool isInTemplate ) const;
#endif // NO_DATA_VALIDATION

	//! Attach item to parent entity
	Bool ApplyAttachmentTo( CEntity* parentEntity, CName slot = CName::NONE );
	
	//! Detach item from current parent
	void Detach( Bool keepMaterialReplacement = false );

	//! Start timer and put root rigid mesh component in physics state
	void Drop();

	//! Disable physics and stop timer
	void Undrop();

	void HackCopyRootMeshComponentLocalToWorld();

	//! Is foreground entity - eg. cat effect
	virtual Bool IsForegroundEntity() const;

	// Attachment changed
	virtual void OnAttachmentUpdate();

	// Called when the appearance changes
	virtual void OnAppearanceChanged( const CEntityAppearance& appearance ) override;

	const CItemEntityProxy * GetItemProxy() const { return m_proxy; }
	const THandle< CEntity > GetParentEntity() const { return m_parentEntity; }

	//! Enable collisions on item
	void EnableCollisions( Bool enable );

	//! Set item physics simulation type to kinematic
	void SwitchToKinematic( Bool enable );

	void Collapse( Bool collapse );

	Bool IsDependentComponentGameplayLODable( CComponent* component ) override { return !m_parentEntity || m_parentEntity->IsGameplayLODable(); }
	Bool IsGameplayLODable() override { return !m_parentEntity || m_parentEntity->IsGameplayLODable(); }

	//! Fast-heuristic "is visible"
	Bool IsVisible() const;

	// patch 1.1 hack fix. needs to be removed once asset will be fixed.
	virtual Bool HACK_ForceInitialKinematicMotion() const override { return true; }

public:
	Float	GetTimeToDespawn() const { return m_timeToDespawn; }

	void SetItemProxy( CItemEntityProxy* proxy );

private:
	Bool ApplyHardAttachment( CComponent* component, const HardAttachmentSpawnInfo& info );
	CComponent* FindRootComponentWithAnimatedInterface() const;
private:
	void funcGetMeshComponent( CScriptStackFrame& stack, void* result );
	void funcGetParentEntity( CScriptStackFrame& stack, void* result );
	void funcGetItemCategory( CScriptStackFrame& stack, void* result );
	void funcGetItemTags( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CItemEntity );
	PARENT_CLASS( CEntity );
	PROPERTY_EDIT( m_timeToDespawn, TXT("Time that have to pass before dropped item will despawn") );
	PROPERTY_EDIT( m_reportToScript, TXT("Report changes to script") );
	NATIVE_FUNCTION( "GetMeshComponent", funcGetMeshComponent );
	NATIVE_FUNCTION( "GetParentEntity", funcGetParentEntity );
	NATIVE_FUNCTION( "GetItemCategory", funcGetItemCategory );
	NATIVE_FUNCTION( "GetItemTags", funcGetItemTags );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CItemEntityManager;

enum EItemSpawnSource
{
	ISS_NotSpawned,
	ISS_SpawnRequest,
	ISS_Depot,
	ISS_Job,
};

struct SItemSpawnInfo
{
	String	m_templatePath;
	CName	m_appearance;
};

struct SItemSpawnRequestInfo
{
	enum EItemSpawnRequestResult
	{
		ISRR_Succeed,
		ISRR_Waiting,
		ISRR_FailedToLoadResource,
		ISRR_FailedToSpawnEntity,
		ISRR_FailedUnknown,
	};

	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_Gameplay );

	CItemEntityProxy*				m_proxy;		//<! Proxy this request relates to
	CJobLoadResource*				m_resourceJob;	//<! Load resource (template) job
	THandle< CEntityTemplate >		m_resource;		//<! Pointer to depot resource (in order not to create job)
	CJobSpawnEntity*				m_spawnItemJob;	//<! Spawn entity job
	CItemEntity*					m_itemEntity;
	CName							m_appearance;	//<! optional: appearance to choose for item
	CName							m_colorVariant;	//<! optional: color variant to choose for item

	SItemSpawnRequestInfo();
	~SItemSpawnRequestInfo();

	EItemSpawnRequestResult Process( CItemEntityManager* manager );
	EItemSpawnRequestResult ProcessCancel( CItemEntityManager* manager );
	
	Bool LoadResource( CItemEntityManager* manager );
	Bool SpawnEntity( CItemEntityManager* manager );
	Bool SetSpawnedEntity( CEntity* spawnedEntity );
	Bool InitItemEntity( CItemEntityManager* manager );
	Bool ShouldRestart( const SItemSpawnInfo& itemSpawnInfo ) const;

	Bool operator==( const SItemSpawnRequestInfo& rhs )
	{
		return m_proxy == rhs.m_proxy;
	}

	friend Bool operator==( const SItemSpawnRequestInfo& lhs , const SItemSpawnRequestInfo& rhs )
	{
		return lhs.m_proxy == rhs.m_proxy;
	}
};

struct SItemEffectStartRequestInfo
{
	CItemEntityProxy*		m_proxy;
	CName					m_effectName;
};

struct SDroppedItemInfo
{
	CItemEntityProxy*		m_proxy;		//<! Dropped item proxy
	Float					m_timeLeft;		//<! Time left to call claimer, or despawn item if no claimer
	IDroppedItemClaimer*	m_claimer;		//<! Custom handler for dropped item timeout ( action point for instance, willing to reset it's state )

	SDroppedItemInfo() : m_proxy( NULL ), m_timeLeft( 999.0f ), m_claimer( NULL ) {} 

	Bool operator==( const SDroppedItemInfo& rhs )
	{
		return m_proxy == rhs.m_proxy;
	}

	friend Bool operator==( const SDroppedItemInfo& lhs, const SDroppedItemInfo& rhs )
	{
		return lhs.m_proxy == rhs.m_proxy;
	}
};

enum EItemHand
{
	IH_Right,
	IH_Left,
	IH_Both,
	IH_None,
};

BEGIN_ENUM_RTTI( EItemHand )
	ENUM_OPTION( IH_Right )
	ENUM_OPTION( IH_Left )
	ENUM_OPTION( IH_Both )
	ENUM_OPTION( IH_None )
	END_ENUM_RTTI()

enum EItemLatentAction
{
	ILA_Draw,
	ILA_Holster,
	ILA_Switch,
};

BEGIN_ENUM_RTTI( EItemLatentAction )
	ENUM_OPTION( ILA_Draw )
	ENUM_OPTION( ILA_Holster )
	ENUM_OPTION( ILA_Switch )
END_ENUM_RTTI()

struct SAnimItemSyncEvent
{
	CActor*				m_itemOwner;
	CName				m_equipSlot;
	CName				m_holdSlot;
	EItemLatentAction	m_actionType;
	EAnimationEventType	m_type;
};

class CLatentItemAction;
class CLatentItemQueuedAction;

class CItemEntityManager
{
	typedef THashMap< CItemEntityProxy*, THandle< CItemEntity > >	TItemEntitiesMap;
	typedef TDynArray< SItemSpawnRequestInfo* >::iterator			TSpawnRequestIterator;
	typedef TDynArray< CLatentItemAction* >							TLatentItemActionsList;
	typedef TDynArray< CLatentItemQueuedAction* >					TLatentItemQueuedActionsList;
	typedef THashMap< CEntityTemplate*, Uint32 >					TTemplatesRefCountMap;

	friend struct SItemSpawnRequestInfo;

	struct STemplateTimeoutCounter
	{
		THandle< CEntityTemplate > entityTemplate;
		Float			 timeout;

		friend Bool operator==( const STemplateTimeoutCounter& lhs, const STemplateTimeoutCounter& rhs )
		{
			return lhs.entityTemplate == rhs.entityTemplate;
		}
	};

	struct STemplatePreloadInfo
	{
		CName				m_itemName;
		CItemEntityProxy*   m_proxy;
		CJobLoadResource*	m_job;
		Bool				m_queueAttachment;
		Float				m_timeTaken;

		STemplatePreloadInfo() : m_job( NULL ), m_timeTaken( 0.0f ), m_queueAttachment( false ), m_proxy( NULL ) {}
	};

	struct SDeploymentItemEntityTemplate
	{
		THandle< CEntityTemplate >	m_template;
		Double						m_lastUseTime;

		void RefreshLifetime();

		Bool IsExpired();

		SDeploymentItemEntityTemplate() : m_template( NULL ) 
		{
			RefreshLifetime();
		}

		SDeploymentItemEntityTemplate( CEntityTemplate* entityTemplate ) 
			: m_template( entityTemplate )
		{
			RefreshLifetime();
		}
	};

protected:
	TItemEntitiesMap								m_itemEntitiesRegistry;			//<! Registry of the item entities given by
	TDynArray< CItemEntityProxy* >					m_failedToLoadProxies;	
	TDynArray< CItemEntityProxy* >					m_attachProxiesQueue;			//<! Proxies to be attached to something
	TDynArray< SItemEffectStartRequestInfo >		m_effectQueue;					//<! Queue for starting effects
	TDynArray< SItemSpawnRequestInfo* >				m_unfinishedRequestsTemp;		//! Temporary buffer to minimize dynamic memory allocations at run-time
	TDynArray< SItemSpawnRequestInfo* >				m_spawnEntitiesQueue;			//<! Entity proxies queued to have entity spawned
	TDynArray< SItemSpawnRequestInfo* >				m_canceledSpawnRequests;		//<! Requests that needs to be properly canceled (and cleaned)
	TDynArray< SDroppedItemInfo >					m_droppedItems;					//<! Registry of items dropped, ticking their despawn timers etc.
	TLatentItemActionsList							m_latentItemActionsList;		//<! Latent item actions in progress
	TLatentItemQueuedActionsList					m_latentQueuedItemActionsList;	//<! Queued latent item actions

	TList< STemplatePreloadInfo >					m_templatesPreloading;			//<! Templates being preloaded for future use
	TDynArray< TPair< CName, SDeploymentItemEntityTemplate > >	m_preloadedTemplates;			//<! Templates already preloaded

	Red::Threads::CMutex							m_spawnEntitiesQueueLock;		//<! Protects access to interactions tree
	Float											m_refreshTemplatesTimer;

	static const Uint32 MAX_DROPPED_ITEMS = 3;										//<! Seems reasonable (?)

public:
	//! Constructor
	CItemEntityManager();

	//! Register new item entity proxy
	CItemEntityProxy* CreateNewProxy( CName itemName, const SItemDefinition& itemDef, const TDynArray< CName >& enhancements, CLayer* subjectLayer, const String& templateName, Bool collapse, Bool invisible, SItemUniqueId item );

	//! Destroy item proxy
	void DestroyProxy( CItemEntityProxy* proxy );

	//! Check if proxy has been registered
	Bool IsProxyRegistred( CItemEntityProxy* proxy ) const;

	//! Destroy entity and remove proxy from registry
	void DestroyItem( CItemEntityProxy* proxy );

	//! Destroy entity and handle ref count
	void DestroyEntityInternal( CItemEntity* itemEntity );

	//! Safely set attachment for item entity proxy
	void SetProxyAttachment( CItemEntityProxy* proxy, CEntity* parentEntity, CName slotName = CName::NONE, Bool force = false );
	void SkinProxyItem( CItemEntityProxy* proxy, CEntity* parentEntity );

	void ChangeProxyItem( CItemEntityProxy* proxy, CName itemName, CName slotOverride, const TDynArray< CName >& slotItems, const String& templateName, Bool collapse, SItemUniqueId item );

	//! Safely set item dropped. if duration left as default, the value from entity CItemEntity will be taken
	void DropItemByProxy( CItemEntityProxy* proxy, Float duration = -1.0f, IDroppedItemClaimer* claimer = nullptr );

	//! Destroy all entities
	void DestroyAll();

	//! Queue entity proxy with attachment change
	void QueueItemEntityAttachmentUpdate( CItemEntityProxy* itemProxy );

	//! Queue entity proxy for spawning it's entity
	void QueueItemEntitySpawnRequest( CItemEntityProxy* itemProxy );

	//! Queue entity proxy for effect action
	void QueueItemEffectStart( CItemEntityProxy* itemProxy, CName effectName );

	//! Play effect on proxy's entity, if this entity is spawned
	void PlayEffectOnEntity( CItemEntityProxy* itemProxy, CName effectName );
	void StopEffectOnEntity( CItemEntityProxy* itemProxy, CName effectName );

	//! Play animation on proxy's entity
	void PlayAnimationOnEntity( CItemEntityProxy* itemProxy, CName animationName );
	void StopAnimationOnEntity( CItemEntityProxy* itemProxy );

	//! Raise behaviour event on proxy's entity
	void RaiseBehaviorEventOnEntity( CItemEntityProxy* itemProxy, CName eventName );

	//! Enable collision info reporting on proxy's entity
	void EnableCollisionInfoReporting( CItemEntityProxy* itemProxy );

	//! Get item entity, DON'T use it
	CItemEntity* GetItemEntity( CItemEntityProxy* proxy );

	//! Checks if template is already loaded by the depot
	// if so, it addrefs the loaded template and adds it to rootset (it will be unloaded, when resource is not needed anymore)
	THandle< CEntityTemplate > CheckForLoadedEntityTemplate( SItemSpawnRequestInfo* spawnInfo ) const;

	///! Is manager doing something (loading, attaching ect.)
	Bool IsDoingSomething() const;

	//! Perform queued attachments etc
	void OnTick( Float timeDelta );

	//! Needed for garbage collector
	void OnSerialize( IFile &file );

	//! Add item to the registry of dropped items
	void RegisterItemDropped( CItemEntityProxy* proxy, Float duration = -1.0f, IDroppedItemClaimer* claimer = nullptr );

	//! Remove proxy from registry of dropped items, if "finalize" set to true, claimer will be informed and/or proxy destroyed
	void UnregisterItemDropped( CItemEntityProxy* proxy, Bool finalize );

	//! Claim dropped item, returns true on success
	Bool ClaimDroppedItem( CItemEntityProxy* proxy, IDroppedItemClaimer* claimer, Bool force = false );

	//! Register item latent action
	Bool RegisterItemLatentAction( CLatentItemAction* action );

	//! Queue item latent action
	void QueueItemLatentAction( CLatentItemQueuedAction* action );

	//! Has actor any latent item actions
	Bool HasActorAnyLatentAction( const CActor* actor ) const;

	//! Has actor latent action for item
	Bool HasActorLatentActionForItem( const CActor* actor, SItemUniqueId itemId ) const;

	//! Has actor queued latent action for item
	Bool HasActorQueuedLatentActionForItem( const CActor* actor, SItemUniqueId itemId ) const;

	//! Get latent actions for actor
	Bool GetActorLatentDrawActionsItems( const CActor* actor, TDynArray< SItemUniqueId >& items ) const;	

	//! Get item entity 
	CItemEntity* GetItemEntityIfSpawned( CItemEntityProxy* proxy );
	const CItemEntity* GetItemEntityIfSpawned( const CItemEntityProxy* proxy ) const;

	Bool EntityItemFaliedToLoad( const CItemEntityProxy* proxy ) const;

	void CancelLatentActionsForActor( CActor* actor, CName holdSlot );

	Bool HasActorCollidableLatentActionsInProgress( const CActor* actor, const SItemDefinition* itemDef ) const;

	void ProcessSpawnRequests();
	void ProcessAttachmentRequests();
	void ProcessEffectRequests();
	void ProcessDroppedItems( Float timeDelta );
	void ProcessLatentActions( Float timeDelta );
	void ProcessQueuedLatentActions();

	void ProcessPreloadingTemplates( Float timeDelta );

	//! Start preloading entity template for later use or addref it
	void AquireDeploymentEntityTemplate( CName itemName, CItemEntityProxy* proxy, bool queueAttachment );

	//! Get preloaded template (template may not be preloaded)
	CEntityTemplate* GetPreloadedEntityTemplate( CName itemName, const String& templatePath );

	Bool IsProxyInSpawnQue( const CItemEntityProxy* proxy ) const;
	Bool IsProxyInAttachQue( const CItemEntityProxy* proxy ) const;

#if 0
	//! Preload all deployment templates
	void PreloadTemplates();
#endif

public:
	void OnItemAnimSyncEvent( const SAnimItemSyncEvent& syncEvent );

};

typedef TSingleton< CItemEntityManager > SItemEntityManager;

//////////////////////////////////////////////////////////////////////////

// Drey TODO: more precise types
enum ELatentItemActionType
{
	LIAT_Draw,
	LIAT_Holster,
	LIAT_Switch
};

class CLatentItemAction
{
protected:
	static const Float		MAX_TIME_FOR_LATENT_ACTION;

	THandle< CActor >		m_actor;
	SItemUniqueId			m_itemId;
	CName					m_activationNotification;
	CName					m_deactivationNotification;
	CName					m_equipSlot;
	CName					m_holdSlot;

	Float					m_timer;
	Bool					m_started;
	Bool					m_ended;
	Bool					m_finished;
	ELatentItemActionType	m_type;

public:
	CLatentItemAction(	CActor* actor, 
						SItemUniqueId itemId, 
						const CName& act, 
						const CName& deact, 
						const CName& equipSlot, 
						const CName& holdSlot );

	virtual ~CLatentItemAction() {}

	virtual Bool IsCollidableWith( const CActor* actionOwner, const SItemDefinition* itemDef ) const;

	virtual Bool ShouldProcessSyncEvent( const SAnimItemSyncEvent& syncEvent ) const = 0;
	virtual void ProcessSyncEvent( const SAnimItemSyncEvent& syncEvent ) = 0;

protected:
	virtual Bool IsProcessed() const = 0;
	virtual Bool OnProcessed() = 0;

public:
	virtual Bool IsValid() const;

	void Cancel();
	void Finish();
	Bool Update( Float timeDelta );

	const CActor* GetActor() const;
	SItemUniqueId GetItem() const;
	ELatentItemActionType GetType() const { return m_type; }
	
	RED_INLINE const CName& GetHoldSlot() const { return m_holdSlot; }

	void LogWarn() const;

protected:
	virtual void OnFinished() {}

	Bool Process();

	Bool DoesActionStart( const CActor* actor ) const;
	Bool DoesActionEnd( const CActor* actor ) const;
	
	
};

class CLatentItemActionSingleSync : public CLatentItemAction
{
protected:
	Bool				m_sync;
	Bool				m_processed;

public:
	CLatentItemActionSingleSync(	CActor* actor, 
									SItemUniqueId itemId, 
									const CName& act, 
									const CName& deact, 
									const CName& equipSlot, 
									const CName& holdSlot );

	virtual void ProcessSyncEvent( const SAnimItemSyncEvent& syncEvent );

public:
	virtual Bool ShouldProcessSyncEvent( const SAnimItemSyncEvent& syncEvent ) const = 0;

protected:
	virtual Bool IsProcessed() const;
	virtual Bool OnProcessed();
};

class CLatentItemActionDraw : public CLatentItemActionSingleSync
{
public:
	CLatentItemActionDraw(	CActor* actor, 
							SItemUniqueId itemId, 
							const CName& act, 
							const CName& deact, 
							const CName& equipSlot, 
							const CName& holdSlot );

	virtual Bool ShouldProcessSyncEvent( const SAnimItemSyncEvent& syncEvent ) const;

protected:
	virtual Bool OnProcessed();
};

class CLatentItemActionHolster : public CLatentItemActionSingleSync
{
public:
	CLatentItemActionHolster(	CActor* actor, 
								SItemUniqueId itemId, 
								const CName& act, 
								const CName& deact, 
								const CName& equipSlot, 
								const CName& holdSlot );

	virtual Bool ShouldProcessSyncEvent( const SAnimItemSyncEvent& syncEvent ) const;

protected:
	virtual Bool OnProcessed();
};

class CLatentItemActionDoubleSync : public CLatentItemAction
{
protected:
	Bool		m_syncStart;
	Bool		m_syncEnd;

	Bool		m_processedFirst;
	Bool		m_processedSecond;

	SItemUniqueId m_itemIdToChange;

public:
	CLatentItemActionDoubleSync(	CActor* actor, 
									SItemUniqueId itemIdToChange,
									SItemUniqueId itemIdNew, 
									const CName& act, 
									const CName& deact, 
									const CName& equipSlot, 
									const CName& holdSlot );

	virtual void ProcessSyncEvent( const SAnimItemSyncEvent& syncEvent );

public:
	virtual Bool ShouldProcessSyncEvent( const SAnimItemSyncEvent& syncEvent ) const = 0;

protected:
	virtual Bool IsProcessed() const;
	virtual Bool OnProcessed();

	virtual Bool ProcessActionForFirstItem();
	virtual Bool ProcessActionForSecItem();

	virtual Bool OnProcessActionForFirstItem() = 0;
	virtual Bool OnProcessActionForSecItem() = 0;
};

class CLatentItemActionSmoothSwitch : public CLatentItemActionDoubleSync
{
public:
	CLatentItemActionSmoothSwitch(	CActor* actor, 
								SItemUniqueId itemIdToChange,
								SItemUniqueId itemIdNew, 
								const CName& act, 
								const CName& deact, 
								const CName& equipSlot, 
								const CName& holdSlot );

	virtual Bool ShouldProcessSyncEvent( const SAnimItemSyncEvent& syncEvent ) const;

protected:
	virtual Bool OnProcessActionForFirstItem();
	virtual Bool OnProcessActionForSecItem();
};

class CLatentItemActionSequentialSwitch : public CLatentItemActionHolster
{
public:
	CLatentItemActionSequentialSwitch(	CActor* actor, 
										SItemUniqueId itemIdToHolster, 
										SItemUniqueId itemIdToDraw, 
										const CName& holsterAct, 
										const CName& holsterDeact, 
										const CName& holsterEquipSlot, 
										const CName& holsterHoldSlot );

protected:
	SItemUniqueId	m_itemIdToDraw;

protected:
	virtual void OnFinished();
};

class CLatentItemQueuedAction
{
protected:
	THandle< CActor >		m_actor;
	SItemUniqueId			m_itemId;
	const SItemDefinition*	m_itemDef;

public:
	CLatentItemQueuedAction( CActor* actor, SItemUniqueId itemId, const SItemDefinition* itemDef );
	virtual ~CLatentItemQueuedAction() {}

	const CActor* GetActor() const;
	SItemUniqueId GetItem() const { return m_itemId; }
	Bool CanBeProcessed() const;

	virtual Bool Process() = 0;
};

class CLatentItemQueuedActionDraw : public CLatentItemQueuedAction
{
public:
	CLatentItemQueuedActionDraw( CActor* actor, SItemUniqueId itemId, const SItemDefinition* itemDef );

	virtual Bool Process();
};

class CLatentItemQueuedActionHolster : public CLatentItemQueuedAction
{
public:
	CLatentItemQueuedActionHolster( CActor* actor, SItemUniqueId itemId, const SItemDefinition* itemDef );

	virtual Bool Process();
};

// TODO
// Akcje: Draw z korekcja
// Draw two items
// Holster two items
