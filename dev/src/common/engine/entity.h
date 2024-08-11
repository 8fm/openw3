/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved. 
*/

#pragma once
#include "node.h"
#include "updateTransformManager.h"
#include "renderVisibilityQuery.h"
#include "worldLookup.h"
#include "../core/sharedDataBuffer.h"
#include "../core/loadingJob.h"

/// Forward declarations
class CTriggerAreaComponent;
class IRenderEntityGroup;
struct SBehaviorGraphOutput;
struct BehaviorEventInfo;
class ICollisionListener;
class IEntityActionsRouter;
class CFXState;
class CComponent;
struct CEntityAppearance;
class CEntityStreamingProxy;
class CEntityStreamingData;
struct SEntityTemplateColoringEntry;
class IMaterial;
class IRenderProxy;
class CLayer;
class EntitySpawnInfo;
class IActorInterface;
struct SComponentSpawnInfo;
struct CAnimationEventFired;
class CEntityExternalAppearance;

class CNavigationCookingContext;

enum EAnimationEventType : CEnum::TValueType;

//--------------------------------------------------------------------

// Static entity flags - SAVED WITH ENTITY
enum EEntityStaticFlags
{
	ESF_Streamed				= FLAG( 0 ),		// Entity can be streamed (old m_streamed flag)
	ESF_NoVisibilityQuery		= FLAG( 1 ),		// Do not bother with visibility query for this entity
	ESF_NoCompExtract			= FLAG( 2 ),		// Do not extract components form this ent when cooking
	ESF_NoCompMerge				= FLAG( 3 ),		// Do not merge components (into shadow meshes, LOD meshes, etc)
};

BEGIN_BITFIELD_RTTI( EEntityStaticFlags, 2 /*bytes*/ );
	BITFIELD_OPTION( ESF_Streamed );
	BITFIELD_OPTION( ESF_NoVisibilityQuery );
	BITFIELD_OPTION( ESF_NoCompExtract );	
	BITFIELD_OPTION( ESF_NoCompMerge );
END_BITFIELD_RTTI();

//--------------------------------------------------------------------

// Dynamic entity flags - NOT SAVED WITH ENTITY
enum EEntityDynamicFlags
{
	EDF_StreamedIn					= FLAG( 0 ),		// Entity contains streamed data
	EDF_StreamingLocked				= FLAG( 1 ),		// Streaming of this entity is locked (entity will not stream out)
	EDF_ForceNoLOD					= FLAG( 2 ),
	EDF_RestoredFromLayerStorage	= FLAG(	3 ),		// State have been appl	EDF_RestoredFromLayerStorage	= FLAG(	2 ),		// State have been applied using layer storage feature 
	EDF_DebugBreakOnStreamIn		= FLAG( 4 ),		// Break on streaming in this entity (debug only)
	EDF_DebugBreakOnStreamOut		= FLAG( 5 ),		// Break on streaming out this entity (debug only)
	EDF_AlwaysTick					= FLAG( 6 ),		// Tick won't be disabled for this entity (but they can still be budgeted)
	EDF_DisableAllDissolves			= FLAG( 7 ),		// If true, then all entity's drawable components will have dissolve disabled.
	EDF_DuringUpdateTransform		= FLAG( 8 ),		// It's true when update transform is going to process this entity, so during that Destroy can't be called
	EDF_DestroyAfterTransformUpdate	= FLAG( 9 ),		// We can't destroy entities while updating transforms, so the destruction must be deferred
};

BEGIN_BITFIELD_RTTI( EEntityDynamicFlags, 2 /*bytes*/ );
	BITFIELD_OPTION( EDF_StreamedIn );
	BITFIELD_OPTION( EDF_StreamingLocked );
	BITFIELD_OPTION( EDF_RestoredFromLayerStorage )
	BITFIELD_OPTION( EDF_DebugBreakOnStreamIn )
	BITFIELD_OPTION( EDF_DebugBreakOnStreamOut )
	BITFIELD_OPTION( EDF_AlwaysTick )
	BITFIELD_OPTION( EDF_DuringUpdateTransform )
END_BITFIELD_RTTI();

//--------------------------------------------------------------------

/// Type of movement
enum EMoveType
{
	MT_Walk,
	MT_Run,
	MT_FastRun,
	MT_Sprint,
	MT_AbsSpeed
};

BEGIN_ENUM_RTTI( EMoveType );
	ENUM_OPTION( MT_Walk );
	ENUM_OPTION( MT_Run );
	ENUM_OPTION( MT_FastRun );
	ENUM_OPTION( MT_Sprint );
	ENUM_OPTION( MT_AbsSpeed );
END_ENUM_RTTI();

//--------------------------------------------------------------------

enum EMoveFailureAction
{
	MFA_REPLAN,
	MFA_EXIT
};
BEGIN_ENUM_RTTI( EMoveFailureAction );
	ENUM_OPTION( MFA_REPLAN );
	ENUM_OPTION( MFA_EXIT );
END_ENUM_RTTI();

//--------------------------------------------------------------------

// Streaming component world creation notification type
enum EStreamingWorldNotification
{
	// Do not update the world state (use only from world itself or when you create the components temporarily)
	SWN_DoNotNotifyWorld = 0,

	// Update the world state (so that when the camera moves away from the entity, the streamed components will be destroyed)
	SWN_NotifyWorld
};

//--------------------------------------------------------------------

// Data passed to script in OnSpawned event
struct SEntitySpawnData
{
	DECLARE_RTTI_STRUCT( SEntitySpawnData );

	SEntitySpawnData()
		: m_restored( false )
	{
	}

	Bool m_restored;	// Entity restored
};

BEGIN_CLASS_RTTI( SEntitySpawnData );
	PROPERTY( m_restored );
END_CLASS_RTTI();

//--------------------------------------------------------------------

// Flags for entity (saved in a lame way - try not to use, use static entity flags and dynamic entity flags instead)
enum EEntityFlags
{
	//TODO: EF_CreatedByGardenerTool to be removed
	EF_CreatedByGardenerTool	= FLAG( 0 ),		// (DEPRECATED) Created by gardener

	EF_DestroyableFromScript	= FLAG( 1 ),		// Destroyable from script
	EF_ManagedEntity			= FLAG( 2 ),		// Entity is managed - will be preserved on dynamic layer
	EF_Poolable					= FLAG( 6 ),		// This entity is managed by the EntitiesPool system
	EF_HasAdditionalData		= FLAG( 7 ),		// The entity has additional data stored
};

//--------------------------------------------------------------------

struct SMaterialReplacementInfo
{
	DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_Engine );

	THandle< IMaterial > material;
	CName				tag;
	CName				exclusionTag;
	Bool				drawOriginal;
	TDynArray< CName >	includeList;
	TDynArray< CName >	excludeList;
	Bool				forceMeshAlternatives;

	SMaterialReplacementInfo()
	{
		material = NULL;
		tag = CName::NONE;
		exclusionTag = CName::NONE;
		drawOriginal = false;
		forceMeshAlternatives = false;
	}
};

//--------------------------------------------------------------------


class CFXTrackItemParameterFloatPlayData;

struct SAllComponentsEffectParamInfo
{
	TDynArray< CFXTrackItemParameterFloatPlayData* > m_floatParamPlayData;
};

class CAnimationSyncToken;

#ifndef NO_EDITOR
// Contains the streaming state of the entity - used only for tools
// You are not supposed to use this outside of PrepareStreamingComponentsEnumeration
// and FinishStreamingComponentsEnumeration
struct SEntityStreamingState
{
	friend class CEntity;

private:
	Bool	m_streamedIn;
	Bool	m_wasLocked;
};
#endif


//////////////////////////////////////////////////////////////////////////
// NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE
//
// IEntityListener should not be used in general at this point. It exists
// to support one particular case, but is not intended for the general
// public.
// DO NOT USE IT!
//    -- TG
//////////////////////////////////////////////////////////////////////////
/// Allows another class to receive notifications when an entity is changed in certain ways.
class IEntityListener
{
	friend class CEntity;

#ifndef RED_FINAL_BUILD
private:
	// In a non-final build, the listener can track what entities it has registered with, so that we can later check that
	// everything is unregistered correctly. Failure to unregister before being deleted could result in crashes later on
	// when a entity makes some change. Would likely be difficult to track down, so we have this.
	TDynArray< THandle< CEntity > > m_listenedEntities;
#endif

public:
	virtual ~IEntityListener();

	virtual void OnNotifyEntityComponentAdded( CEntity* entity, CComponent* component ) = 0;
	virtual void OnNotifyEntityComponentRemoved( CEntity* entity, CComponent* component ) = 0;
	virtual void OnNotifyEntityRenderProxyAdded( CEntity* entity, CComponent* component, IRenderProxy* proxy ) = 0;
	virtual void OnNotifyEntityRenderProxyRemoved( CEntity* entity, CComponent* component ) = 0;
};

//////////////////////////////////////////////////////////////////////////

/// The entity class
class CEntity : public CNode
{
	friend class CEntityTemplate;
	friend class CStreamingSectorData;
	friend class CAppearanceComponent;
	friend class CEdEntityGraphEditor;
	friend class CEdEntityEditor;
	friend class CLayerCooker;
	friend class CLayer;
	friend class CLayerStorage;

	// TEMP HACK FIX : Cooker dependency analyzer needs to get dependencies from entity's streamed buffers.
	friend class CCookerDependencyLoaderEntityTemplate;

	DECLARE_ENGINE_CLASS( CEntity, CNode, 0 )

	// No particular entity heading
	static const Float HEADING_ANY;

protected:
	TDynArray< CComponent* >							m_components;					//!< Container for components
	TDynArray< CFXState*, MC_EntityFxState >			m_activeEffects;				//!< Active effects playing on this entity
	THandle< CEntityTemplate >							m_template;						//!< Entity template this entity is using
	SMaterialReplacementInfo*							m_materialReplacementInfo;		//!< Material replacement info
	SAllComponentsEffectParamInfo*						m_allComponentsEffectParamInfo;	//!< Mesh effect param applied to all components

#ifndef RED_FINAL_BUILD
	// Temporary until resave
	TDynArray< Uint8 >									m_oldBuffer[3];
#endif

#ifndef NO_EDITOR
	CEntityGroup*										m_containingGroup;
	String												m_name;
#endif

private:
	//////////////////////////////////////////////////////////////////////////
	// NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE
	//
	// As noted above for IEntityListener, this is not intended for the
	// general public.
	// DO NOT USE IT!
	//    -- TG
	TDynArray< IEntityListener* > m_entityListeners;
	//////////////////////////////////////////////////////////////////////////

protected:
	CName									m_autoPlayEffectName;			//!< Auto play effect given by name
	TRenderVisibilityQueryID				m_visibilityQuery;				//!< Per entity visiblity query, can be used to access visibility information (usually from last frame) - note that it's not that heavy as it used to be
	Uint16									m_entityStaticFlags;			//!< Entity static (saved) flags
	Uint16									m_entityDynamicFlags;			//!< Entity dynamic (not saved) flags
	Uint8									m_entityFlags;					//!< Entity flags (TODO: should not be saved, runtime only)
	Uint8									m_streamingDistance;			//!< Streaming distance divided by 8
	CEntityStreamingProxy*					m_streamingProxy;				//!< Entity streaming proxy
	SharedDataBuffer						m_streamingDataBuffer;			//!< Data buffer containing the streamed components in serialized form
	TDynArray< THandle< CComponent > >		m_streamingComponents;			//!< Streamed components (note: same objects exist in m_components too)
	TDynArray< THandle< IAttachment > >		m_streamingAttachments;			//!< Streamed attachments
#ifndef NO_EDITOR
	Bool									m_partOfAGroup : 1;				//!< Set by CEntityGroup if this entity is part of a group
	Uint16									m_forceAutoHideDistance;		//!< If specified overrides autohide distance of all mesh components
#endif

public:
	enum EHideReason
	{
		HR_Default						= FLAG( 0 ),
		HR_Lod							= FLAG( 1 ),
		HR_Scene						= FLAG( 2 ),
		HR_SceneArea					= FLAG( 3 ),		// Scene area; hiding non-scene actors
		HR_Scripts						= FLAG( 4 ),

		HR_All							= HR_Default | HR_Lod | HR_Scene | HR_SceneArea | HR_Scripts
	};

	// Marks entity as being updated by Update Transform Manager
	RED_INLINE void MarkAsDuringUpdateTransform() { SetDynamicFlag( EDF_DuringUpdateTransform, true ); }

	// Invalidates flag indicating that this entity is being updated by Update Transform Manager
	RED_INLINE void ResetDuringUpdateTransformFlag() { SetDynamicFlag( EDF_DuringUpdateTransform, false ); }

	// Returns the current streaming distance
	RED_INLINE Uint32 GetStreamingDistance() const { return static_cast< Uint32 >( m_streamingDistance ) << 3; }

	// Sets the streaming distance
	RED_INLINE void SetStreamingDistance( Uint32 distance );

	// Get the entity template
	RED_INLINE RED_MOCKABLE CEntityTemplate* GetEntityTemplate() const { return m_template.Get(); }

	// Is the entity spawned ?
	RED_FORCE_INLINE Bool IsSpawned() const { return HasFlag( NF_PostAttachSpawnCalled ); }

	// Is this entity managed ?
	RED_FORCE_INLINE Bool IsManaged() const { return CheckEntityFlag( EF_ManagedEntity ); }

	// Get entity flags
	RED_FORCE_INLINE Uint8 GetEntityFlags() const { return m_entityFlags; }

	// Set runtime entity flag
	RED_FORCE_INLINE void SetEntityFlag( Uint32 flag ) { m_entityFlags |= flag; }

	// Clear runtime entity flag
	RED_FORCE_INLINE void ClearEntityFlag( Uint32 flag ) { m_entityFlags &= ~flag; }

	// Check if runtime entity flag is set
	RED_FORCE_INLINE const Bool CheckEntityFlag( const EEntityFlags flag ) const { return 0 != (m_entityFlags & flag); }

	// Check static entity flag, static flags are usually set in editor
	RED_FORCE_INLINE const Bool CheckStaticFlag( const EEntityStaticFlags flag ) const { return 0 != (m_entityStaticFlags & flag); }

	// Check static entity flag, static flags are usually set in editor
	RED_FORCE_INLINE const Bool CheckDynamicFlag( const EEntityDynamicFlags flag ) const { return 0 != (m_entityDynamicFlags & flag); }

	// Get the static flags
	RED_FORCE_INLINE const Uint16 GetStaticFlags( ) const { return m_entityStaticFlags; }

	// Get auto play effect name
	RED_FORCE_INLINE CName GetAutoPlayEffectName() const { return m_autoPlayEffectName; }

	// Get list of components
	RED_MOCKABLE RED_FORCE_INLINE const TDynArray< CComponent* >& GetComponents() const { return m_components; }

	// Get raw placement
	RED_FORCE_INLINE const EngineTransform& GetRawPlacement() const { return m_transform; }

	// Should this entity be streamed ?
	RED_FORCE_INLINE const Bool ShouldBeStreamed() const { return CheckStaticFlag( ESF_Streamed ); }
	
	// Is this entity streamed in
	RED_FORCE_INLINE const Bool IsStreamedIn() const { return CheckDynamicFlag( EDF_StreamedIn ); }

	// Is this streaming locked for this entity
	RED_FORCE_INLINE const Bool IsStreamingLocked() const { return CheckDynamicFlag( EDF_StreamingLocked ); }

#ifndef NO_EDITOR
	// Get overridden autohide distance value for this entity
	RED_FORCE_INLINE const Uint16 GetForceAutoHideDistance () const { return m_forceAutoHideDistance; }
#endif

	// Has entity and its component been loaded?
	Bool IsFullyLoaded() const;

	// Is entity rendering-ready?
	Bool IsRenderingReady() const;

	//! Get proxy visibility query
	RED_FORCE_INLINE TRenderVisibilityQueryID GetVisibilityQuery() const { return m_visibilityQuery; }

#ifndef NO_EDITOR
	RED_INLINE void SetPartOfAGroup( Bool partOfAGroup ) { m_partOfAGroup = partOfAGroup; }
	RED_INLINE Bool GetPartOfAGroup() const { return m_partOfAGroup; }
	RED_INLINE CEntityGroup* GetContainingGroup() const { return m_containingGroup; }
	RED_INLINE void SetContainingGroup( CEntityGroup* entityGroup ) { m_containingGroup = entityGroup; }
#endif

public:
	CEntity();
	virtual ~CEntity();

	virtual void OnFinalize();

	// Destroy this entity if entity is not updated by update transform manager (calls ExecuteDestroy())
	// Otherwise defer destroy execution after update (sets EDF_DestroyAfterTransformUpdate)
	// DO NOT MAKE IT VIRTUAL - use OnDestroyed() instead
	void Destroy();

	// Called after update transform manager finish it's job in current world tick
	void ProcessPendingDestroyRequest();

private:
	// Destroy this entity - calls GetLayer()->DestroyEntity()
	// DO NOT MAKE IT VIRTUAL - use OnDestroyed() instead
	void ExecuteDestroy();

public:
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags ) override;
	virtual Int32 GetSubObjectCount() const;
	virtual Int32 GetSoundSubObjectCount()const;
	RED_INLINE virtual const CName& GetSfxTag() const { return CName::NONE; }

	virtual Bool GetSubObjectWorldMatrix( Uint32 index, Matrix& matrix ) const;
	Matrix GetBoneReferenceMatrixMS( Uint32 index ) const;

public:
	// Calculate entity bounding box by adding up boxes of drawable components
	RED_MOCKABLE Box CalcBoundingBox() const;

	// Detach this entity from template it was created from
	void DetachTemplate( Bool markComponentsAsNonIncluded = false );

#ifndef NO_ERROR_STATE
	//! Set entity error state (use SET_ERROR_STATE macro)
	virtual void SetErrorState( const String & description ) const;
#endif

#ifndef NO_DEBUG_PAGES
	// Provide additional extended debug information about the object
	virtual void OnDebugPageInfo( class CDebugPageHTMLDocument& doc ) override;
#endif

	// Get template that is used to instance this object
	virtual CObject* GetTemplate() const;

	//! Get entity last reported error state
	virtual const String& GetErrorState() const;

	// Get object name for script debugger
	virtual Bool GetScriptDebuggerName( String& debugName ) const;

	// Get entity localized display name
	virtual String GetDisplayName() const;

	// Is this a player
	virtual Bool IsPlayer() const { return false; }

	// Get rendering entity group assigned to this entity ( used for a per-character effects )
	virtual IRenderEntityGroup* GetRenderEntityGroup() const { return NULL; }

	// Cast to entity ( faster than Cast<> )
	virtual CEntity* AsEntity() { return this; }

	// Cast to entity - const version ( faster than Cast<> )
	virtual const CEntity* AsEntity() const { return this; }

	virtual IEntityActionsRouter*  AsEntityActionsRouter() { return nullptr; }

	// Get layer this node was created in
	virtual CLayer* GetLayer() const;

	//! Get object friendly name
	virtual String GetFriendlyName() const;

	//! Is this ok to extract components from this entity during sector data building ?
	virtual Bool CanExtractComponents( const Bool isOnStaticLayer ) const;

	//! Returns the local streamed component data buffer
	RED_INLINE const SharedDataBuffer& GetLocalStreamedComponentDataBuffer() const { return m_streamingDataBuffer; }

	//! Get old buffers
#ifndef RED_FINAL_BUILD
	RED_INLINE const TDynArray< Uint8 >& GetOldStreamingBuffer( const Uint32 index ) const { return m_oldBuffer[index]; }
#endif

	//! Create the streamed components for this entity - DO NOT ATTACH THEM
	void PrecacheStreamedComponents( CEntityStreamingData& outData, Bool createComponentsFromIncludes=true ) const;

	//! Create the streamed components
	void CreateStreamedComponents( EStreamingWorldNotification notification, Bool createComponentsFromIncludes=true, const CEntityStreamingData* precachedData = nullptr, Bool notifyComponents=true );

	//! Destroy the streamed components
	void DestroyStreamedComponents( EStreamingWorldNotification notification );

	//! Creates the streamed attachments stored in the given template for the given new components and the already streamed-in components
	void CreateStreamedAttachmentsForNewComponentsUsingTemplate( CEntityTemplate* entityTemplate, const TDynArray< CComponent* >& newComponents, TDynArray< IAttachment* >& createdAttachments );

	//! Creates the streamed attachments stored in this entity's template for the given new components and the already streamed-in components
	void CreateStreamedAttachmentsForNewComponents( const TDynArray< CComponent* >& newComponents, TDynArray< IAttachment* >& createdAttachments );

	//! Collect all resources referenced in the streaming data
	void CollectResourcesInStreaming( TDynArray< String >& outResourcePaths ) const;

	//! Enable/Disable streaming on this entity
	void SetStreamed( const Bool streamed );

	//! Get entity's streaming priority (it is JP_StreamingObject by default for all entities)
	virtual EJobPriority GetStreamingPriority() const { return JP_StreamingObject; }

#ifndef NO_EDITOR
	//! Returns true if the given component should be streamed
	static Bool AllowStreamingForComponent( CComponent* component );

	//! Removes all streaming data from this entity
	void RemoveAllStreamingData();

	//! Update the streaming distance. This must be called when all components
	//! (included, appearance, streamed, etc) are already in memory
	void UpdateStreamingDistance();

	//! Update the streamed component data buffers from the components
	//! If includeExistingComponents is true, the existing components serialized in the streaming
	//! buffers are created (streamed in) before the new buffers are generated
	Bool UpdateStreamedComponentDataBuffers( Bool includeExistingComponents = true );

	//! Stream components in and optionally lock the streaming so that nothing is (un)streamed
	//! The state structure will be filled with the current streaming state and must be
	//! restored using FinishStreamingComponentsEnumeration
	void PrepareStreamingComponentsEnumeration( SEntityStreamingState& state, Bool lockStreaming, EStreamingWorldNotification notification = SWN_NotifyWorld );

	//! Restores the streaming state before PrepareStreamingComponentsEnumeration was called
	void FinishStreamingComponentsEnumeration( const SEntityStreamingState& state, EStreamingWorldNotification notification = SWN_NotifyWorld );

	//! Sets the streaming lock and returns the previous state
	Bool SetStreamingLock( Bool lock );

	//! Removes any streaming information that refers to the given component
	//! (used in the editor when changing a component's streaming flag)
	void RemoveComponentFromStreaming( CComponent* component );

	//! Returns true if the component can be safely unstreamed
	Bool CanUnstream() const;
#endif

	//! Forces components with async resources to load immediately
	void ForceFinishAsyncResourceLoads();

	// Called on entity creation thread, after initialization and after! apply appearance 
	void PostComponentsInitializedAsync(); // not virtual

public:
	// Called when entity is being destroyed, only called for entities on valid layers
	virtual void OnDestroyed( CLayer* layer );

	// Called just before layer is saved in editor
	virtual void OnLayerSavedInEditor();

	// Object was loaded
	virtual void OnPostLoad() override;
	
	// Object was instanced
	virtual void OnPostInstanced() override;

	// All inherited components are included
	virtual void OnIncludesFinished();

	// Called on entity creation thread, after instantiation
	virtual void OnCreatedAsync( const EntitySpawnInfo& info );

	// Called on entity creation thread, after initialization and after! apply appearance 
	virtual void OnPostComponentsInitializedAsync();

	// Entity was created, not yet attached or initialized
	virtual void OnCreated( CLayer* layer, const EntitySpawnInfo& info );

	// Callen on entity creation thread when pooling out
	virtual void OnRestoredFromPoolAsync( const EntitySpawnInfo& info );

	// Object was reattached from pool
	virtual void OnRestoredFromPool( CLayer* layer, const EntitySpawnInfo& info );

	// Entity was loaded from existing layer
	virtual void OnLoaded();

	// Entity was initialized
	virtual void OnInitialized();

	// Entity was uninitialized
	virtual void OnUninitialized();

	// Entity was attached to world
	virtual void OnAttached( CWorld* world );

	// Entity was detached from world
	virtual void OnDetached( CWorld* world );

	// All components of entity has been attached
	virtual void OnAttachFinished( CWorld* world );

	// Version of OnAttachFinished called only in editor (when editing entity template)
	virtual void OnAttachFinishedEditor( CWorld* world );

	// Entity was pasted in ( in editor )
	virtual void OnPasted( CLayer* layer );

	// Real-time timer elapsed; id is the id as returned from CTickManager::AddTimer()
	virtual void OnTimer( const CName name, Uint32 id, Float timeDelta );

	// Game-time timer elapsed; id is the id as returned from CTickManager::AddTimer()
	virtual void OnTimer( const CName name, Uint32 id, GameTime timeDelta );

	// Tick occurred
	virtual void OnTick( Float timeDelta );

#ifdef USE_ANSEL
	// LOD Tick occurred
	virtual void HACK_ANSEL_OnLODTick() { }
#endif // USE_ANSEL

	// Something has entered trigger area owned by this entity
	virtual void OnAreaEnter( CTriggerAreaComponent* area, CComponent* activator );

	// Something has exited trigger area owned by this entity
	virtual void OnAreaExit( CTriggerAreaComponent* area, CComponent* activator );

	// Trigger area owned by this entity has been (de)activated
	virtual void OnAreaActivated( CTriggerAreaComponent* area, Bool activated );

	// Proxy of some component was de-attached
	virtual void OnProxyDetached( CComponent* );

	// Proxy of some component was attached
	virtual void OnProxyAttached( CComponent*, IRenderProxy* proxy );

	// Called after streamable components have been streamed in and their own OnStreamIn calls have been made
	virtual void OnStreamIn(){}
	
	// Called before streamable components are to be streamed out, before the components' own OnStreamOut is called
	virtual void OnStreamOut(){}

	// Collect animation synchronization tokens
	virtual void OnCollectAnimationSyncTokens( CName animationName, TDynArray< CAnimationSyncToken* >& tokens ) const;

	// Prepare entity for validation
	virtual void OnPrepare() const {};

#ifndef NO_EDITOR_ENTITY_VALIDATION
	// Called before saving
	virtual Bool OnValidate( TDynArray< String >& log ) const;
#endif

	// Serialization
	virtual void OnSerialize( IFile& file );

#ifndef NO_RESOURCE_COOKING
	// Resource cooking
	virtual void OnCook( class ICookerFramework& cooker ) override;
#endif

#ifndef NO_EDITOR
	// Navigation cooker specyfic code. Cooker is doing double pass on entities, so they can mark themselves before we will start to do obstacle computation and stuff
	void PostNavigationCook( CWorld* world );
	virtual void OnNavigationCook( CWorld* world, CNavigationCookingContext* context );
	void OnNavigationCookerInitialization( CWorld* world, CNavigationCookingContext* context );
#endif

	// Property is going to be changed
	virtual void OnPropertyPreChange( IProperty* property );
	
	// Property has changed
	virtual void OnPropertyPostChange( IProperty* property );

	// Property was read from file that is no longer in the object, returns true if data was handled
	virtual Bool OnPropertyMissing( CName propertyName, const CVariant& readValue ) override;

	// Process behavior output
	virtual void OnProcessBehaviorPose( const CAnimatedComponent* /*poseOwner*/, const SBehaviorGraphOutput& /*pose*/ ) {};
	
	// Process interaction ( TODO: move to game code! )
	virtual void OnProcessInteractionExecute( class CInteractionComponent* /*interaction*/ ) {};

	// Called when scripts are being reloaded
	virtual void OnScriptPreCaptureSnapshot();

	// Called when scripts were successfully reloaded
	virtual void OnScriptReloaded();

	// Called when move speed was changed
	virtual void OnMoveSpeedChanged( CAnimatedComponent* component, Float newSpeed );

	// Played effect was removed from the active effects
	void OnActiveEffectRemoved( CFXState* effectInstance );

#ifndef NO_DATA_VALIDATION
	// Check data, can be called either for node on the level or for node in the template
	virtual void OnCheckDataErrors( Bool isInTemplate ) const;

	// Full (and slow) validation
	void FullValidation( const String& additionalContext ) const;
#endif

	// Layer this entity is on is unloading. Called after all detaches.
	virtual void OnLayerUnloading();

	// Called when the appearance changes
	virtual void OnAppearanceChanged( const CEntityAppearance& appearance );

	// Check if we can safely detach the entity
	virtual Bool OnPoolRequest();

public:
	// Add real time based entity timer
	Uint32 AddTimer( const CName& name, Float period, Bool repeats = false, Bool scatter = false, ETickGroup group = TICK_Main, Bool savable = false, Bool overrideExisting = true );

	// Add gameplay time based entity timer
	Uint32 AddTimer( const CName& name, GameTime period, Bool repeats = false, Bool scatter = false, ETickGroup group = TICK_Main, Bool savable = false, Bool overrideExisting = true );

	// Remove all entity timers matching the name and group
	void RemoveTimer( const CName& name, ETickGroup group = TICK_Main );

	// Remove entity timer by id
	void RemoveTimer( Uint32 id, ETickGroup group = TICK_Main );

	// Remove all entity timers
	void RemoveTimers();

public:
	void UpdateTransformEntity( SUpdateTransformContext& context, Bool parentScheduledUpdateTransform = false );

protected:
	virtual void OnUpdateTransformEntity();

	//! Invoked after setting value for m_autoPlayEffectName
	virtual void OnSetAutoPlayEffectName() {}

public:
	//! Get the actor interface
	virtual IActorInterface* QueryActorInterface() { return NULL; }

	//! Get the actor interface
	virtual const IActorInterface* QueryActorInterface() const { return NULL; }

	CSoundEmitterComponent* GetSoundEmitterComponent( Bool createIfDoesntExist = true );

public:
	//! Get first root animated component
	virtual CAnimatedComponent* GetRootAnimatedComponent() const;

public:
	// Create component by class and name
	CComponent* CreateComponent( CClass* componentClass, const SComponentSpawnInfo& spawnInfo );

	// Add component
	void AddComponent( CComponent* component );

	// Detach component that belongs to this entity, but do not destroy it (it may be added to other entity)
	void RemoveComponent( CComponent* component, bool leaveParentAttachments = false );

	// Move component from another entity
	void MoveComponent( CComponent* component );

	// Generate unique name, if name is changed return true
	Bool GenerateUniqueComponentName( CComponent* component ) const;

	// Find component by name
	CComponent* FindComponent( const String& name, Bool caseSensitive = true ) const;

	// Find component by name
	CComponent* FindComponent( CName name ) const;

	// Find component by name and class
	CComponent* FindComponent( CName componentName, const CName& componentClass ) const;

	// Find component by GUID
	CComponent* FindComponent( const CGUID& guid ) const;

	// Find component by class
	template< class T > RED_INLINE T* FindComponent() const;
	
	template< class T > RED_INLINE T* FindComponent( const String& name ) const;

	// Find component by class and name
	template< class T > RED_INLINE T* FindComponent( const Char* name ) const;

	// Find component by class and name
	template< class T > RED_INLINE T* FindComponent( CName name ) const;

	// Destroy component
	void DestroyComponent( CComponent* component );

	// Destroy all entity components
	void DestroyAllComponents();

	// Destroy all CStaticMeshComponents and replace them by CMeshComponents
	void ConvertAllStaticMeshesToMeshes();

	// Destroy all CMeshComponents and replace them by CStaticMeshComponents 
	void ConvertAllMeshesToStatic();

	// Destroy all CMeshComponents and replace them by CRigidMeshComponents
	void ConvertAllMeshesToRigidMeshes();

	// Initialize before attaching
	void Initialize();

	// Uninitialize after detaching
	void Uninitialize();

	// Attach to world
	void AttachToWorld( CWorld* world );

	// Detach from world
	void DetachFromWorld( CWorld* world );

	// Destroy entity internals
	void DestroyEntityInternals();
	
	// Entity is poolable and the pool request destruction. 
	RED_MOCKABLE void DestroyFromPool(); 

	// Is this entity in active game world?
	Bool IsInGame() const;

	// Set raw placement
	void SetRawPlacement( const Vector* position, const EulerAngles* rotation, const Vector* scale );
    virtual void SetRawPlacementNoScale( const Matrix& newPlacement );

	// Set the hide in game flag - this will disable visiblity fo entity and all attached entities ( aka. weapons )
	virtual void SetHideInGame( Bool hideInGame, Bool immediate = false, EHideReason hr = HR_Default );
	// Gets reason this entity was hidden for; returns combination of EHideReason flags
	virtual Uint32 GetHideReason() const;

		// Refresh visiblity flag of all components
	void RefreshChildrenVisibility( Bool force = false );

	// Refresh visibility - virtual CNode interface
	virtual void RefreshNodeVisibilityFlag( NodeProcessingContext& context, Bool force ) override;

	// Refresh visiblity flag of all components - internal implementation
	void RefreshChildrenVisibility( NodeProcessingContext& context, Bool force );

	// Mark entity as suspended from rendering
	void SuspendRendering( Bool flag );

	// Is entity foreground object? eg. player - no cat effect on it
	virtual Bool IsForegroundEntity() const { return false; }

	// Duplciate this entity and keep it in same layer
	CEntity* Duplicate( CLayer* placeOnLayer ) const;

#ifndef NO_RESOURCE_USAGE_INFO
	//! Resource usage reporting
	virtual void CollectResourceUsage( class IResourceUsageCollector& collector ) const;
#endif

public:
	// Teleport entity to new location
	virtual Bool Teleport( const Vector& position, const EulerAngles& rotation );

	// Teleport to node
	virtual Bool Teleport( CNode* node, Bool applyRotation = true );

	//! Is Master entity
	virtual Bool IsMasterEntity() const { return false; }

public:
	//! Is there any effect alive (including stopping effects)?
	Bool IsAnyEffectAlive() const;

	//! Are we playing any effect ?
	Bool IsPlayingAnyEffect() const;

	//! Are we playing particular effect
	//@param treatStoppingAsPlaying - if set to true, stopping effect is treated as active / playing effect
	Bool IsPlayingEffect( const CName& effectTag, Bool treatStoppingAsPlaying = true ) const;

	//! Are we playing particular effect
	//@param treatStoppingAsPlaying - if set to true, stopping effect is treated as active / playing effect
	Bool IsPlayingEffect( const CFXDefinition* def, Bool treatStoppingAsPlaying = true ) const;

	//! Is the effect paused
	Bool IsPausedEffect( const CName& effectTag ) const;

	//! Is the effect paused
	Bool IsPausedEffect( const CFXDefinition* def ) const;

	//! Play effect on this entity
	virtual Bool PlayEffect( const CName& effectName, const CName& boneName = CName::NONE, const CNode* targetNode = NULL, const CName& targetBone = CName::NONE );

	//! Play effect on this entity
	Bool PlayEffect( const CFXDefinition* effect, const CName& effectTag = CName::NONE, Float startTime=0.0f, const CName& boneName = CName::NONE, const CNode* targetNode = NULL, const CName& targetBone = CName::NONE, Bool forAnim = false );

	//! Check if entity has this effect defined
	Bool HasEffect( const CName& effectName );

	void GetActiveEffects( TDynArray< CName >& effects );

	//! Play effect preview, also plays animation if effect is bound to one
	Bool PlayEffectPreview( const CFXDefinition* effect );

	//! Play animation effect on this entity
	virtual Bool PlayEffectForAnimation( const CName& animationName, Float startTime=0.0f );

	//! Can start action from script
	virtual Bool CanPerformActionFromScript( CScriptStackFrame& /*stack*/ ) const { return true; }

	//! Stop effect playing on this entity
	Bool StopEffect( const CName& effectTag );

	//! Is the effect paused
	Bool StopEffect( const CFXDefinition* def );

	//! Pause/Unpause effect playing on this entity
	Bool PauseEffect( const CName& effectTag, Bool isPaused );

	//! Pause/Unpause effect playing on this entity
	Bool PauseEffect( const CFXDefinition* def, Bool isPaused );

	//! Play all effects playing on this entity
	void PlayAllEffects();

	//! Pause all effects playing on this entity
	void PauseAllEffects( Bool isPaused );

	//! Stop all effects playing on this entity
	void StopAllEffects();

	//! Destroy effect playing on this entity
	Bool DestroyEffect( const CName& effectTag );

	//! Destroy all effects playing on this entity
	void DestroyAllEffects();

	//! Sets effect intensity for specified component (or all components if componentName not specified or set to NONE) and parameter name
	//! Due to backward-compatibility issues, if effectParameterName was not specified (or set to CName::NONE), it's assumed to be 'MeshEffectScalar3'
	void SetEffectIntensity( const CName& effectName, Float intensity, const CName& specificComponentName = CName::NONE, const CName& effectParameterName = CName::NONE );

	//! Set effect parameter intensity for specified component (or all components if set to NONE) and parameter name
	void SetEffectsParameterValue( Float intensity, const CName &specificComponentName, CName effectIntensityParameterName );

	//! Returns active effect state for a given effect name or NULL if not found any.
	CFXState* GetActiveEffect( const CName& effectName );

	//! Propagates the given call to the item entities attached to this entity's root animated component
	template<typename F> RED_INLINE void PropagateCallToItemEntities( F func ) const;

	//! Gets whether given dependent component is subject to "component LODding"
	virtual Bool IsDependentComponentGameplayLODable( CComponent* component ) { return true; }
	// Gets whether this entity is subject to LODding
	virtual Bool IsGameplayLODable();

	void SetForceNoLOD( Bool enable );

	RED_INLINE void SetDisableAllDissolves( Bool disable );

public:
	// Raises a behavior event to graphs of all animated components in the actor entity. Returns true if event was processed by any graph.
	Bool RaiseBehaviorEventForAll( const CName& eventName );

	// Raises a behavior event to root animated component
	Bool RaiseBehaviorEvent( const CName& eventName );

	// Raises a behavior force event to graphs of all animated components in the actor entity. Returns true if event was processed by any graph.
	Bool RaiseBehaviorForceEventForAll( const CName& eventName );

	// Raises a behavior force event to root animated component
	Bool RaiseBehaviorForceEvent( const CName& eventName );

	// Sets behavior variable for root animated component
	Bool SetBehaviorVariable( const CName varName, Float value, Bool inAllInstances = false );

	// Sets behavior variable for root animated component
	Float GetBehaviorFloatVariable( const CName varName, Float defValue = 0.0f );

	// Has entity animation ( slow )
	Bool HasAnimation( const CName& animationName, Bool rootOnly ) const;

	// Freeze all animated components
	void FreezeAllAnimatedComponents();

	// Unfreeze all animated components
	void UnfreezeAllAnimatedComponents();

#ifndef NO_EDITOR
	// Find entity animation ( slow )
	CSkeletalAnimationSetEntry* FindAnimation( const CName& animationName ) const;

	// Editor only stuff
	virtual void EditorOnTransformChangeStart() override;

	// Editor only stuff
	virtual void EditorOnTransformChanged() override;

	// Editor only stuff
	virtual void EditorOnTransformChangeStop() override;

	// Called pre deletion from editor
	virtual void EditorPreDeletion() override;

	// Called after entity was created in editor
	virtual void EditorPostCreation();

	// Called after an object is duplicated (SHIFT + DRAG)
	virtual void EditorPostDuplication( CNode* originalNode ) override;

	virtual const String & GetName() const { return m_name; }
#endif

	void SetName( const String & name );

public:
	// Called on cutscene started
	virtual void OnCutsceneStarted();

	// Called on cutscene ended
	virtual void OnCutsceneEnded();

public:
	//! Generate debug fragments ( called by preview panel and witcher game )
	virtual void GenerateDebugFragments( CRenderFrame* /*frame*/ ) {}

	// For new event system
	virtual void ProcessAnimationEvent( const CAnimationEventFired* event ) {}

	//! Prepare entity for template saving
	void PrepareEntityForTemplateSaving();

public:
	//! Set material replacement
	virtual Bool SetMaterialReplacement( IMaterial* material, Bool drawOriginal = false, const CName& tag = CName::NONE, const CName& exclusionTag = CName::NONE, const TDynArray< CName >* includeList = nullptr, const TDynArray< CName >* excludeList = nullptr, Bool forceMeshAlternatives = false );

	//! Disable material replacement
	virtual void DisableMaterialReplacement();
	
	//! Has material replacement?
	virtual Bool HasMaterialReplacement();

	//! Has material replacement?
	virtual void SendParametersMaterialReplacement( const Vector& params );

	//! Returns information about the current material replacement
	RED_INLINE const SMaterialReplacementInfo* GetMaterialReplacementInfo() const { return m_materialReplacementInfo; }

public:
	// Applies mesh component coloring using the given entry
	void ApplyMeshComponentColoringUsing( const SEntityTemplateColoringEntry& entry ); 

	// Applies mesh component coloring using the current appearance if any, the default (NONE)
	// entries if no appearances are set or returns false if no coloring information set
	Bool ApplyMeshComponentColoring();

public:
	// Get the detailed visibility result for entity visibility query
	// This will return value for last frame not the current one
	ERenderVisibilityResult GetLastFrameVisibility() const;

	//! Was this entity visible last frame ?
	//! NOTE: this is not checking the shadows, only the main viewport
	Bool WasVisibleLastFrame() const;

	virtual Bool WasInventoryVisibleLastFrame() const { return false; }

public:
	//! Register float parameter played on all components, this will allow newly attached components to match the previous ones
	virtual void SetAllComponentsFloatParameter( CFXTrackItemParameterFloatPlayData* playData );
	virtual void DisableAllComponentsFloatParameter( CFXTrackItemParameterFloatPlayData* playData );
	virtual Bool HasAllComponentsFloatParameter();

	Bool CreateAttachmentImpl( THandle< CEntity > parentHandle, CName slot, const Vector& relativePosition = Vector::ZERO_3D_POINT, const EulerAngles& relativeRotation = EulerAngles::ZEROS );
	Bool CreateAttachmentAtBoneWSImpl( THandle< CEntity > parentHandle, CName bone, Vector const & worldLocation, EulerAngles const & worldRotation );
	Bool BreakAttachment();

protected:
	virtual void OnAttachmentCreated();
	virtual void OnAttachmentBroken();

protected: 
	// Called when SetGUID is hit
	virtual void OnGUIDChanged( const CGUID& oldGuid ) override;

	// Change entity dynamic flag - please do not expose to general public
	void SetDynamicFlag( const EEntityDynamicFlags flag, const Bool val = true );

	// Clear entity dynamic flag - please do not expose to general public
	void ClearDynamicFlag( const EEntityDynamicFlags flag );

	// Change entity static flag - please do not expose to general public
	void SetStaticFlag( const EEntityStaticFlags flag, const Bool val = true );

	// Clear entity static flag - please do not expose to general public
	void ClearStaticFlag( const EEntityStaticFlags flag );

	//! Restore GUID
	void RestoreGUIDIfNeeded();

	//! Load specialized per-component data for templated entities (area shape, etc)
	void LoadPerComponentData( IFile& file );

	//! Save specialized per-component data for templated entities (area shape, etc)
	void SavePerComponentData( IFile& file );

	//! Save local-to-instance properties from components in templated entities
	void LoadPerComponentInstanceProperties( IFile& file );

	//! Save local-to-instance properties from components in templated entities
	void SavePerComponentInstanceProperties( IFile& file );

	// Notify that child component has been attached
	void NotifyComponentsAttached();

	// Notify that entity was streamed in
	void NotifyComponentsStreamedIn();

	// Load components state
	virtual void LoadStreamedComponentsState() {}

	// Save components state
	virtual void SaveStreamedComponentsState() {}

	// Register entity in streaming grid
	void RegisterInStreamingGrid( CWorld* world );

	// Unregister entity form streaming grid
	void UnregisterFromStreamingGrid( CWorld* world );

	// Update entity in the streaming grid
	void UpdateInStreamingGrid( CWorld* world );

	// Resave entity streaming data - NOTE: does not regenerate the data, just resaves existing one
	void ResaveStreamingBuffer( const class DependencySavingContext& savingContext );

	//////////////////////////////////////////////////////////////////////////
	// NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE
	//
	// As noted above for IEntityListener, this is not intended for the
	// general public.
	// DO NOT USE IT!
	//    -- TG
	//////////////////////////////////////////////////////////////////////////
public:
	Bool RegisterEntityListener( IEntityListener* listener );
	Bool UnregisterEntityListener( IEntityListener* listener );
private:
    void FixRootAnimatedComponentPosition( void ); // Root animated component with 0 parent attachments should be 1st on m_components list, fix it here
	void SendNotifyComponentAdded( CComponent* component );
	void SendNotifyComponentRemoved( CComponent* component );
	//////////////////////////////////////////////////////////////////////////

	void StreamedFlagChanged();
	void ForceAutoHideDistanceChanged();

	//! Strip the streaming data buffer
	void PurgeStreamingBuffer();

public:
	Bool ExportExternalAppearance( const CName &name, CEntityExternalAppearance* entityExternalAppearance ) const;
	Bool ImportExternalAppearance( const CEntityExternalAppearance* entityExternalAppearance );

#ifndef NO_EDITOR
public:
	virtual void OnComponentTransformChanged( CComponent* /*component*/ ) {}
#endif

	// patch 1.1 hack fix. needs to be removed once asset will be fixed.
	virtual Bool HACK_ForceInitialKinematicMotion() const { return false; }

protected:
	//! AutoBinding support
	virtual Bool IsAutoBindingPropertyTypeSupported( const IRTTIType* type ) const override;
	virtual Bool ValidateAutoBindProperty( const CProperty* autoBindProperty, CName bindingName ) const override;
	virtual Bool ResolveAutoBindProperty( const CProperty* autoBindProperty, CName bindingName, void* resultData ) const override;

protected:
	void funcAddTimer( CScriptStackFrame& stack, void* result );
	void funcAddGameTimeTimer( CScriptStackFrame& stack, void* result );
	void funcRemoveTimer( CScriptStackFrame& stack, void* result );
	void funcRemoveTimerById( CScriptStackFrame& stack, void* result );
	void funcRemoveTimers( CScriptStackFrame& stack, void* result );
	void funcFindWaypoint( CScriptStackFrame& stack, void* result );
	void funcDestroy( CScriptStackFrame& stack, void* result );
	void funcTeleport( CScriptStackFrame& stack, void* result );
	void funcTeleportWithRotation( CScriptStackFrame& stack, void* result );
	void funcTeleportToNode( CScriptStackFrame& stack, void* result );
	void funcGetPathComponent( CScriptStackFrame& stack, void* result );
	void funcGetComponent( CScriptStackFrame& stack, void* result );
	void funcGetComponentByClassName( CScriptStackFrame& stack, void* result );
	void funcGetComponentsByClassName( CScriptStackFrame& stack, void* result );
	void funcGetComponentByUsedBoneName( CScriptStackFrame& stack, void* result );
	void funcGetComponentsCountByClassName( CScriptStackFrame& stack, void* result );
	void funcGetBoneIndex( CScriptStackFrame& stack, void* result );
	void funcGetBoneWorldMatrixByIndex( CScriptStackFrame& stack, void* result );
	void funcGetBoneReferenceMatrixMS( CScriptStackFrame& stack, void* result );
	void funcGetMoveTarget( CScriptStackFrame& stack, void* result );
	void funcGetMoveHeading( CScriptStackFrame& stack, void* result );
	void funcGetRootAnimatedComponent( CScriptStackFrame& stack, void* result );
	void funcPreloadBehaviorsToActivate( CScriptStackFrame& stack, void* result );
	void funcActivateBehaviors( CScriptStackFrame& stack, void* result );
    void funcActivateBehaviorsSync( CScriptStackFrame& stack, void* result );
    void funcSetCollisionType( CScriptStackFrame& stack, void* result );
	void funcActivateAndSyncBehavior( CScriptStackFrame& stack, void* result );
	void funcActivateAndSyncBehaviors( CScriptStackFrame& stack, void* result );
	void funcAttachBehavior( CScriptStackFrame& stack, void* result );
    void funcAttachBehaviorSync( CScriptStackFrame& stack, void* result );
	void funcDetachBehavior( CScriptStackFrame& stack, void* result );
	void funcRaiseEvent( CScriptStackFrame& stack, void* result );
	void funcRaiseForceEvent( CScriptStackFrame& stack, void* result );
	void funcIsEffectActive( CScriptStackFrame& stack, void* result );
	void funcDuplicate( CScriptStackFrame& stack, void* result );
	void funcSetHideInGame( CScriptStackFrame& stack, void* result );

	// E3 Hack for horse
	void funcRaiseForceEventWithoutTestCheck( CScriptStackFrame& stack, void* result );
	void funcRaiseEventWithoutTestCheck( CScriptStackFrame& stack, void* result ); // this one was here before but should be removed
	// End

	void funcSetBehaviorVariable( CScriptStackFrame& stack, void* result );
	void funcGetBehaviorVariable( CScriptStackFrame& stack, void* result );
	void funcSetBehaviorVectorVariable( CScriptStackFrame& stack, void* result );
	void funcGetBehaviorVectorVariable( CScriptStackFrame& stack, void* result );
	void funcGetBehaviorGraphInstanceName( CScriptStackFrame& stack, void* result );
	void funcPlayEffect( CScriptStackFrame& stack, void* result );
	void funcPreloadEffect( CScriptStackFrame& stack, void* result );
	void funcPreloadEffectForAnimation( CScriptStackFrame& stack, void* result );
	void funcGetAutoEffect( CScriptStackFrame& stack, void* result );
	void funcSetAutoEffect( CScriptStackFrame& stack, void* result );
	void funcPlayEffectOnBone( CScriptStackFrame& stack, void* result );
	void funcStopEffect( CScriptStackFrame& stack, void* result );
	void funcDestroyEffect( CScriptStackFrame& stack, void* result );
	void funcStopAllEffects( CScriptStackFrame& stack, void* result );
	void funcDestroyAllEffects( CScriptStackFrame& stack, void* result );
	void funcHasEffect( CScriptStackFrame& stack, void* result );
	void funcSoundEvent( CScriptStackFrame& stack, void* result );
	void funcTimedSoundEvent( CScriptStackFrame& stack, void* result );
	void funcSoundSwitch( CScriptStackFrame& stack, void* result );
	void funcSoundParameter( CScriptStackFrame& stack, void* result );
	void funcSoundIsActiveAny( CScriptStackFrame& stack, void* result );
	void funcSoundIsActiveName( CScriptStackFrame& stack, void* result );
	void funcSoundIsActive( CScriptStackFrame& stack, void* result );
	void funcGetDisplayName( CScriptStackFrame& stack, void* result );
	void funcFade( CScriptStackFrame& stack, void* result );
	void funcWaitForEventProcessing( CScriptStackFrame& stack, void* result );
	void funcWaitForBehaviorNodeActivation( CScriptStackFrame& stack, void* result );
	void funcWaitForBehaviorNodeDeactivation( CScriptStackFrame& stack, void* result );
	void funcBehaviorNodeDeactivationNotificationReceived( CScriptStackFrame& stack, void* result );
	void funcWaitForAnimationEvent( CScriptStackFrame& stack, void* result );
	void funcCreateAttachment( CScriptStackFrame& stack, void* result );
	void funcCreateAttachmentAtBoneWS( CScriptStackFrame& stack, void* result );
	void funcBreakAttachment( CScriptStackFrame& stack, void* result );
	void funcHasAttachment( CScriptStackFrame& stack, void* result );
	void funcHasSlot( CScriptStackFrame& stack, void* result );
	void funcCreateChildAttachment( CScriptStackFrame& stack, void* result );
	void funcBreakChildAttachment( CScriptStackFrame& stack, void* result );
	void funcHasChildAttachment( CScriptStackFrame& stack, void* result );
	void funcCalcEntitySlotMatrix( CScriptStackFrame& stack, void* result );
	void funcSetEffectIntensity( CScriptStackFrame& stack, void* result );
	void funcSetKinematic(  CScriptStackFrame& stack, void* result );
	void funcSetStatic( CScriptStackFrame& stack, void* result );
	void funcIsRagdolled(  CScriptStackFrame& stack, void* result );
	void funcIsStatic( CScriptStackFrame& stack, void* result );
	void funcGetGuidHash( CScriptStackFrame& stack, void* result );
	void funcCalcBoundingBox( CScriptStackFrame& stack, void* result );
	void funcHasTagInLayer( CScriptStackFrame& stack, void* result );
	
};

BEGIN_CLASS_RTTI( CEntity );
	PARENT_CLASS( CNode );
//	PROPERTY( m_components ); - DO NOT, DO NOT, consult Dex
	PROPERTY_NOSERIALIZE( m_components );
	PROPERTY( m_template );
	PROPERTY( m_streamingDataBuffer );
	PROPERTY( m_streamingDistance );
	PROPERTY_BITFIELD_EDIT( m_entityStaticFlags, EEntityStaticFlags, TXT("Extra entity flags") );
	PROPERTY_EDIT( m_autoPlayEffectName, TXT("Auto play effect name") );
	PROPERTY( m_entityFlags );
#ifndef NO_EDITOR
	PROPERTY_EDIT_NOT_COOKED( m_name, TXT("Node name") );
	PROPERTY_EDIT_NOT_COOKED( m_forceAutoHideDistance, TXT("Override autohide distance of mesh components") );
#endif
	NATIVE_FUNCTION( "AddTimer", funcAddTimer );
	NATIVE_FUNCTION( "AddGameTimeTimer", funcAddGameTimeTimer );
	NATIVE_FUNCTION( "RemoveTimer", funcRemoveTimer );
	NATIVE_FUNCTION( "RemoveTimerById", funcRemoveTimerById );
	NATIVE_FUNCTION( "RemoveTimers", funcRemoveTimers );
	//NATIVE_FUNCTION( "FindWaypoint", funcFindWaypoint );
	NATIVE_FUNCTION( "Destroy", funcDestroy );
	NATIVE_FUNCTION( "Teleport", funcTeleport );
	NATIVE_FUNCTION( "TeleportWithRotation", funcTeleportWithRotation );
	NATIVE_FUNCTION( "TeleportToNode", funcTeleportToNode );	
	//NATIVE_FUNCTION( "GetPathComponent", funcGetPathComponent );
	NATIVE_FUNCTION( "GetComponent", funcGetComponent );
	NATIVE_FUNCTION( "GetComponentByClassName", funcGetComponentByClassName );
	NATIVE_FUNCTION( "GetComponentsByClassName", funcGetComponentsByClassName );
	NATIVE_FUNCTION( "GetComponentByUsedBoneName", funcGetComponentByUsedBoneName );
	NATIVE_FUNCTION( "GetComponentsCountByClassName", funcGetComponentsCountByClassName );
	NATIVE_FUNCTION( "RaiseEvent", funcRaiseEvent );
    NATIVE_FUNCTION( "RaiseForceEvent", funcRaiseForceEvent );
    NATIVE_FUNCTION( "RaiseEventWithoutTestCheck", funcRaiseEventWithoutTestCheck );
    NATIVE_FUNCTION( "RaiseForceEventWithoutTestCheck", funcRaiseForceEventWithoutTestCheck );
	NATIVE_FUNCTION( "GetBoneIndex", funcGetBoneIndex );
	NATIVE_FUNCTION( "GetBoneWorldMatrixByIndex", funcGetBoneWorldMatrixByIndex );
	NATIVE_FUNCTION( "GetBoneReferenceMatrixMS", funcGetBoneReferenceMatrixMS );
	NATIVE_FUNCTION( "GetMoveTarget", funcGetMoveTarget );
	NATIVE_FUNCTION( "GetMoveHeading", funcGetMoveHeading );
	NATIVE_FUNCTION( "GetRootAnimatedComponent", funcGetRootAnimatedComponent );
	NATIVE_FUNCTION( "PreloadBehaviorsToActivate", funcPreloadBehaviorsToActivate );
	NATIVE_FUNCTION( "ActivateBehaviors", funcActivateBehaviors );
    NATIVE_FUNCTION( "ActivateBehaviorsSync", funcActivateBehaviorsSync );
	NATIVE_FUNCTION( "ActivateAndSyncBehavior", funcActivateAndSyncBehavior );
	NATIVE_FUNCTION( "ActivateAndSyncBehaviors", funcActivateAndSyncBehaviors );
	NATIVE_FUNCTION( "SetCollisionType", funcSetCollisionType );
	NATIVE_FUNCTION( "AttachBehavior", funcAttachBehavior );
    NATIVE_FUNCTION( "AttachBehaviorSync", funcAttachBehaviorSync );
	NATIVE_FUNCTION( "DetachBehavior", funcDetachBehavior );
	NATIVE_FUNCTION( "SetBehaviorVariable", funcSetBehaviorVariable );
	NATIVE_FUNCTION( "GetBehaviorVariable", funcGetBehaviorVariable );
	NATIVE_FUNCTION( "SetBehaviorVectorVariable", funcSetBehaviorVectorVariable );
	NATIVE_FUNCTION( "GetBehaviorVectorVariable", funcGetBehaviorVectorVariable );
	NATIVE_FUNCTION( "GetBehaviorGraphInstanceName", funcGetBehaviorGraphInstanceName );
	NATIVE_FUNCTION( "PlayEffect", funcPlayEffect );
	NATIVE_FUNCTION( "PlayEffectOnBone", funcPlayEffectOnBone );
	NATIVE_FUNCTION( "GetAutoEffect", funcGetAutoEffect );
	NATIVE_FUNCTION( "SetAutoEffect", funcSetAutoEffect );
	NATIVE_FUNCTION( "StopEffect", funcStopEffect );
	NATIVE_FUNCTION( "DestroyEffect", funcDestroyEffect );
	NATIVE_FUNCTION( "StopAllEffects", funcStopAllEffects );
	NATIVE_FUNCTION( "DestroyAllEffects", funcDestroyAllEffects );
	NATIVE_FUNCTION( "HasEffect", funcHasEffect );
	NATIVE_FUNCTION( "SoundEvent", funcSoundEvent )
	NATIVE_FUNCTION( "TimedSoundEvent", funcTimedSoundEvent )
	NATIVE_FUNCTION( "SoundSwitch", funcSoundSwitch )
	NATIVE_FUNCTION( "SoundParameter", funcSoundParameter )
	NATIVE_FUNCTION( "SoundIsActiveAny", funcSoundIsActiveAny )
	NATIVE_FUNCTION( "SoundIsActiveName", funcSoundIsActiveName )
	NATIVE_FUNCTION( "SoundIsActive", funcSoundIsActive )
	NATIVE_FUNCTION( "PreloadEffect", funcPreloadEffect );
	NATIVE_FUNCTION( "PreloadEffectForAnimation", funcPreloadEffectForAnimation );
	NATIVE_FUNCTION( "Fade", funcFade );
	NATIVE_FUNCTION( "CreateAttachment", funcCreateAttachment );
	NATIVE_FUNCTION( "CreateAttachmentAtBoneWS", funcCreateAttachmentAtBoneWS );
	NATIVE_FUNCTION( "BreakAttachment", funcBreakAttachment );
	NATIVE_FUNCTION( "HasAttachment", funcHasAttachment );
	NATIVE_FUNCTION( "HasSlot", funcHasSlot );
	NATIVE_FUNCTION( "CreateChildAttachment", funcCreateChildAttachment );
	NATIVE_FUNCTION( "BreakChildAttachment", funcBreakChildAttachment );
	NATIVE_FUNCTION( "HasChildAttachment", funcHasChildAttachment );
	NATIVE_FUNCTION( "CalcEntitySlotMatrix", funcCalcEntitySlotMatrix );
	NATIVE_FUNCTION( "SetEffectIntensity", funcSetEffectIntensity );
	NATIVE_FUNCTION( "SetKinematic", funcSetKinematic );
	NATIVE_FUNCTION( "SetStatic", funcSetStatic );
	NATIVE_FUNCTION( "IsRagdolled", funcIsRagdolled );
	NATIVE_FUNCTION( "IsStatic", funcIsStatic );
	NATIVE_FUNCTION( "I_GetDisplayName", funcGetDisplayName );
	NATIVE_FUNCTION( "IsEffectActive", funcIsEffectActive );
	NATIVE_FUNCTION( "Duplicate", funcDuplicate );
	NATIVE_FUNCTION( "SetHideInGame", funcSetHideInGame );
	NATIVE_FUNCTION( "GetGuidHash", funcGetGuidHash );
	NATIVE_FUNCTION( "CalcBoundingBox", funcCalcBoundingBox );
	NATIVE_FUNCTION( "HasTagInLayer", funcHasTagInLayer );


	// Latent functions
	NATIVE_FUNCTION( "WaitForEventProcessing", funcWaitForEventProcessing );
	NATIVE_FUNCTION( "WaitForBehaviorNodeActivation", funcWaitForBehaviorNodeActivation );
	NATIVE_FUNCTION( "WaitForBehaviorNodeDeactivation", funcWaitForBehaviorNodeDeactivation );
	NATIVE_FUNCTION( "BehaviorNodeDeactivationNotificationReceived", funcBehaviorNodeDeactivationNotificationReceived );
	NATIVE_FUNCTION( "WaitForAnimationEvent", funcWaitForAnimationEvent );
END_CLASS_RTTI();

#define ACTION_START_TEST if( !CanPerformActionFromScript( stack ) ) { RETURN_BOOL( false ); return; }

#ifdef NO_ERROR_STATE	
	#define SET_ERROR_STATE( _entity, _text )
#else
	#define SET_ERROR_STATE( _entity, _text ) _entity->SetErrorState( _text );
#endif

/////////////////////////////////////////////////////////

#include "entity.inl"
