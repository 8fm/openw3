/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "node.h"
#include "hitProxyId.h"
#include "editorNodeMovementHook.h"
#include "customIntrusiveList.h"

class IAttachment;
class ISkeletonDataProvider;
class ISlotProvider;
class IAnimatedObjectInterface;
class AttachmentSpawnInfo;
class ComponentSpawnInfo;
class CVertexComponent;
class CEffectParameters;
class IRenderProxyInterface;
class CJobImmediateCollector;
class CHitProxyMap;
class IGameSaver;
class IGameLoader;
class CFXParameters;
struct STickManagerContext;
#ifdef USE_UMBRA
class CUmbraScene;
#endif // USE_UMBRA


#ifndef NO_LOG

#define VALIDATION_FAIL( format, ... )	RED_LOG( Validation, format, ## __VA_ARGS__ )

#else

#define VALIDATION_FAIL( format, ... )

#endif

/// Component flags
enum EComponentFlags
{
	CF_UsedInAppearance		= FLAG( 0 ),		// The component was created from an appearance
	CF_StreamedComponent	= FLAG( 1 ),		// The component was created by the streaming system
};


/// Attachment group for components
enum EAttachmentGroup
{
	ATTACH_GROUP_P2,		// Navigation mesh, terrain
	ATTACH_GROUP_P1,		// Static mesh
	ATTACH_GROUP_WAYPOINT,	// Waypoints
	ATTACH_GROUP_0,			// Normal components
	ATTACH_GROUP_A1,		// Animated components
	ATTACH_GROUP_A2,		// Skinned meshes

	ATTACH_GROUP_MAX,
};

/// Effect parameter type
enum EEffectParameterType
{
	EPT_None,
	EPT_Float,
	EPT_Vector,
	EPT_Color,
};

namespace PathLib
{
	class IComponent;
};

class CNavigationCookingContext;

/// Efect parameter value
class EffectParameterValue
{
protected:
	EEffectParameterType		m_type;
	Float						m_floatValue;
	Vector						m_vectorValue;
	Color						m_colorValue;

public:
	RED_INLINE EffectParameterValue()
		: m_type( EPT_None )
		, m_floatValue( 0.0f )
	{};

	RED_INLINE bool IsFloat() const
	{
		return m_type == EPT_Float;
	}

	RED_INLINE void SetFloat( Float val )
	{
		m_type = EPT_Float;
		m_floatValue = val;
	}

	RED_INLINE Float GetFloat() const
	{
		return m_floatValue;
	}

	RED_INLINE bool IsVector() const
	{
		return m_type == EPT_Vector;
	}

	RED_INLINE void SetVector( const Vector& val )
	{
		m_type = EPT_Vector;
		m_vectorValue = val;
	}

	RED_INLINE const Vector& GetVector() const
	{
		return m_vectorValue;
	}

	RED_INLINE bool IsColor() const
	{
		return m_type == EPT_Color;
	}

	RED_INLINE void SetColor( const Color& val )
	{
		m_type = EPT_Color;
		m_colorValue = val;
	}

	RED_INLINE const Color& GetColor() const
	{
		return m_colorValue;
	}
};

struct SPositionInfo
{
	Vector	position;
	String	layerName;
	String	entityName;
	String	componentName;
};

typedef THashMap< Uint64, TDynArray< SPositionInfo > > TPositionMap;

/// Component spawn info
struct SComponentSpawnInfo
{
	String		m_name;
	Vector		m_spawnPosition;
	Vector		m_spawnScale;
	EulerAngles	m_spawnRotation;
	const void*		m_customData;

	SComponentSpawnInfo()
		: m_name( String::EMPTY )
		, m_spawnPosition( Vector::ZERO_3D_POINT )
		, m_spawnScale( Vector::ONES )
		, m_spawnRotation( EulerAngles::ZEROS )
		, m_customData( NULL )
	{
	}
};

//! Tick related component functionality
class CComponentTickProxy
{
	friend class CComponent;
	friend class CComponentTickManager;
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Engine );
	DEFINE_CUSTOM_INTRUSIVE_LIST_ELEMENT( CComponentTickProxy, tick );
private:
	CComponent* m_component;				// Managed component
	Float		m_totalRecentTickTime;		// Total recent tick time (total across all tick groups this component is registered with)
	Float		m_accumulatedDeltaTime;		// Accumulated delta time
	Uint8		m_tickMask;					// In which tick groups are we registered in; see ETickGroup
	Uint8		m_tickSuppressionMask;		// value != 0 indicates tick is disabled; see CComponent::ETickBudgetingReason
	Uint8		m_tickBudgetingMask;		// value != 0 indicates tick is budgeted; see CComponent::ETickSuppressReason
	Bool		m_useImmediateJobs_MainPass;
	Bool		m_useImmediateJobs_ActorPass;

	CComponentTickProxy( CComponent* component );
	RED_FORCE_INLINE ~CComponentTickProxy()
	{
		ASSERT( !m_tickMask, TXT("Component hasn't been unregistered from all tick groups. Expect crashes in CComponentTickManager!") );
	}
	RED_FORCE_INLINE Bool IsTickSuppressed() const { return m_tickSuppressionMask != 0; }
	RED_FORCE_INLINE Bool IsTickSuppressed( Uint8 reason ) const { return ( m_tickSuppressionMask & reason ) != 0; }

	RED_FORCE_INLINE Bool IsTickBudgeted() const { return m_tickBudgetingMask != 0; }
	RED_FORCE_INLINE Bool IsTickBudgeted( Uint8 reason ) const { return ( m_tickBudgetingMask & reason ) != 0; }

	RED_FORCE_INLINE Bool UseImmediateJobs_MainPass() const { return m_useImmediateJobs_MainPass; }
	RED_FORCE_INLINE Bool UseImmediateJobs_ActorPass() const { return m_useImmediateJobs_ActorPass; }
};

/// Base entity component
class CComponent : public CNode
#ifndef NO_EDITOR
	, public IEditorNodeMovementHook
#endif
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CComponent, CNode );

	friend class CAreaTree;
	friend class CWorld;
	friend class CComponentTickManager;

protected:

#ifndef NO_COMPONENT_GRAPH
	Int16			m_graphPositionX;			//!< Position X in entity graph
	Int16			m_graphPositionY;			//!< Position Y in entity graph
	CHitProxyID		m_hitProxyId;				//!< ID of hit proxy object
#endif

	String						m_name;

private:
	CComponent*					m_nextAttachedComponent;	//!< Next attached component in internal attachment list
	CComponent**				m_prevAttachedComponent;	//!< Link to pointer in previous attached component in internal attachment list
	CComponentTickProxy*		m_tickProxy;				//!< Tick related functionality
	Bool						m_isStreamed;				//!< Is the component streamed?
	Uint8						m_componentFlags;			//!< Component flags

public:

	// Tick related
	enum ETickSuppressReason
	{
		SR_Default						= FLAG( 0 ),
		SR_Distance						= FLAG( 1 ),
		SR_Visibility					= FLAG( 2 ),
		SR_Freeze						= FLAG( 3 ),
		SR_Lod							= FLAG( 4 ),
		// NOTICE: With current CComponentTickProxy setup we have only 8 flags here!
	};

	enum ETickBudgetingReason
	{
		BR_Distance						= FLAG( 0 ),
		BR_Visibility					= FLAG( 1 ),
		BR_Lod							= FLAG( 2 )
		// NOTICE: With current CComponentTickProxy setup we have only 8 flags here!
	};

	// Get component tick mask
	RED_FORCE_INLINE Uint8 GetTickMask() const { return m_tickProxy ? m_tickProxy->m_tickMask : 0; }

	//! Suppress component from ticking
	void SuppressTick( Bool suppress, ETickSuppressReason reason );
	//! Is component suppressed, returns count
	RED_FORCE_INLINE Bool IsTickSuppressed() const { return m_tickProxy ? m_tickProxy->IsTickSuppressed() : false; }
	RED_FORCE_INLINE Bool IsTickSuppressed( ETickSuppressReason reason ) const { return m_tickProxy ? m_tickProxy->IsTickSuppressed( reason ) : false; }
	//! Toggles tick budgeting for this component
					 void SetTickBudgeted( Bool enable, ETickBudgetingReason reason );
	//! Gets whether this component's tick is budgeted
	RED_FORCE_INLINE Bool IsTickBudgeted() const { return m_tickProxy ? m_tickProxy->IsTickBudgeted() : false; }
	//! Gets whether this component's tick is budgeted
	RED_FORCE_INLINE Bool IsTickBudgeted( ETickBudgetingReason reason ) const { return m_tickProxy ? m_tickProxy->IsTickBudgeted( reason ) : false; }
	//! Gets or creates if not done before tick proxy
	RED_FORCE_INLINE CComponentTickProxy* GetOrCreateTickProxy() { return m_tickProxy ? m_tickProxy : ( m_tickProxy = new CComponentTickProxy( this ) ); }
	//! Gets tick proxy; returns nullptr if not created
	RED_FORCE_INLINE CComponentTickProxy* GetTickProxy() const { return m_tickProxy; }

	//! Is this component a root component
	RED_FORCE_INLINE Bool IsRootComponent() const { return m_transformParent == nullptr && GetParentAttachments().Empty(); }

public:
    // Get next attached component
	RED_INLINE CComponent* GetNextAttachedComponent() const { return m_nextAttachedComponent; }

	// Get entity that owns this component (fast)
	CEntity* GetEntity() const;

	// Get root entity. This entity can be attached to parent entity etc. (slow)
	CEntity* GetRootEntity() const;

	RED_INLINE Bool IsStreamed() const { return m_isStreamed; }
	void SetStreamed( Bool enable );

	CWorld* GetWorld() const;

		// Component flags because we ran out of node flags
	RED_INLINE void SetComponentFlag( Uint8 flag ) { m_componentFlags |= flag; };
	RED_INLINE Bool HasComponentFlag( Uint8 flag ) const { return ( m_componentFlags & flag ) != 0; }
	RED_INLINE void ClearComponentFlag( Uint8 flag ) { m_componentFlags &= ~flag; };

	// Returns true if the component has been used (created by) an appearance
	RED_INLINE Bool IsUsedInAppearance() const { return HasComponentFlag( CF_UsedInAppearance ); }

#ifndef NO_COMPONENT_GRAPH

public:
	//! Get component hit proxy ID
	RED_INLINE const CHitProxyID& GetHitProxyID() const { return m_hitProxyId; }

#endif

public:
	CComponent();
	~CComponent();

	// Destroy component, calls GetEntity()->DestroyComponent()
	// DO NOT MAKE IT VIRTUAL, use OnDestroyed()
	void Destroy();

	// Get attachment group for this component - it determines the order
	virtual EAttachmentGroup GetAttachGroup() const;

	// Get layer this node was created in
	virtual CLayer* GetLayer() const;

	// Initialize before attaching
	void Initialize();

	// Uninitialize after detaching
	void Uninitialize();

	// Attach component to world, not delayed, not safe to call anywhere
	void AttachToWorld( CWorld* world );

	// Detach component from world, not delayed, not safe to call anywhere
	void DetachFromWorld( CWorld* world );

	// Is component internal logic enabled ?
	virtual Bool IsEnabled() const;

	// Toggle component internal state, implemented in child classes
	virtual void SetEnabled( Bool enabled );

	// Change parent, use with care
	virtual void ChangeParent( CEntity* newParent );

	//! Try to use given resource passed from editor during spawning (ex. mesh for StaticMesh template)
	virtual Bool TryUsingResource( CResource * /*resource*/ ) { return false; }

	//! Flag that tells that this components needs to be initialized before everything else
	virtual Bool ShouldInitializeBeforeOtherComponents() const { return false; }

	// Refresh render proxies
	virtual void RefreshRenderProxies(){ }

	// Toggle auto fade on this component
	void SetAutoFade( Bool fadeIn );

	// Cast to component ( faster than Cast<> )
	virtual CComponent* AsComponent() { return this; }

	// Cast to component - const version ( faster than Cast<> )
	virtual const CComponent* AsComponent() const { return this; }
	
	virtual Bool IsIndividual() const { return false; }

	// Collect immediate jobs
	virtual void CollectImmediateJobs( STickManagerContext& context, CTaskBatch& taskBatch ) {}

	// Set resource 
	virtual void SetResource( CResource* resource );
	virtual void GetResource( TDynArray< const CResource* >& resources ) const;

public:
	virtual void OnPropertyPostChange( IProperty* property );
	virtual void OnAttachFinished( CWorld* /*world*/ ){};
	virtual void OnAttachFinishedEditor( CWorld* /*world*/ ){};
	virtual void OnItemEntityAttached( const CEntity* parentEntity ){};
	virtual void OnCutsceneStarted();
	virtual void OnCutsceneEnded();
	virtual void OnCinematicStorySceneStarted() {}
	virtual void OnCinematicStorySceneEnded() {}
	virtual void OnDetachFromEntityTemplate() {}

	virtual Float GetAutoHideDistance() const { return GetMaxAutohideDistance(); }
	virtual Float GetDefaultAutohideDistance() const { return 50.0f; }
	virtual Float GetMaxAutohideDistance() const { return 2000.0f; }

	virtual Uint32 GetOcclusionId() const { return 0; }

	// recalculate autohide distance if the default one is -1.0f
	static Float CalculateAutoHideDistance( const Float autoHideDistance, const Box& boundingBox, const Float defaultDistance, const Float maxDistance );
protected:
	Float CalculateAutoHideDistance( const Float autoHideDistance, const Box& boundingBox ) const
	{
		return CalculateAutoHideDistance( autoHideDistance, boundingBox, GetDefaultAutohideDistance(), GetMaxAutohideDistance() );
	}

public:
#ifndef NO_RESOURCE_USAGE_INFO
	//! Resource usage reporting
	virtual void CollectResourceUsage( class IResourceUsageCollector& collector, const Bool isStremable ) const;
#endif

#ifdef USE_UMBRA
	virtual Bool ShouldBeCookedAsOcclusionData() const { return false; }
#endif

public:

#ifndef NO_COMPONENT_GRAPH

	// Get graph editor position
	void GetGraphPosition( Int32& x, Int32& y ) const;

	// Set graph editor position
	void SetGraphPosition( Int32 x, Int32 y );

#endif

#ifndef NO_EDITOR
	virtual void OnNavigationCook( CWorld* world, CNavigationCookingContext* context );
	virtual void OnNavigationCookerInitialization( CWorld* world, CNavigationCookingContext* context );
	virtual void PostNavigationCook( CWorld* world );

	virtual Bool RemoveOnCookedBuild();
#endif

protected:
	virtual void OnUpdateTransformNode( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;
	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld );

public:
	// Query render proxy interface
	virtual IRenderProxyInterface* QueryRenderProxyInterface();

	// Query render proxy interface ( const )
	virtual const IRenderProxyInterface* QueryRenderProxyInterface() const;

	virtual class CPhysicsWrapperInterface* GetPhysicsRigidBodyWrapper() const { return 0; }

	// Refresh visibility - do it locally and for all attached components & entities
	void RefreshVisibilityFlag( Bool force = false );

	// Internal implementation
	void RefreshVisibilityFlag( NodeProcessingContext& context, Bool force );

	// Refresh visibility - virtual CNode interface
	virtual void RefreshNodeVisibilityFlag( NodeProcessingContext& context, Bool force ) override;

	virtual void ToggleVisibility( Bool visible );

public:
	// Serialization
	virtual void OnSerialize( IFile& file );

	// Finalized
	virtual void OnFinalize();

	// Called when component is spawned ( usually called in entity template editor )
	virtual void OnSpawned( const SComponentSpawnInfo& spawnInfo );

	// Called when component is about to be destroyed ( usually called in entity template editor )
	virtual void OnDestroyed();

	// Called when component is initialized before being attached
	virtual void OnInitialized();

	// Called after component is initialized (and after entity apply appearance) but before being attached, called from spawn job thread
	virtual void OnPostComponentInitializedAsync();

	// Called when component is uninitialized after being detached
	virtual void OnUninitialized();

	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );

	// Called when component is detached from world ( layer gets hidden, etc )
	virtual void OnDetached( CWorld* world );

	// Perform component attachment to world, calls OnAttached for components that were not attached before
	void PerformAttachment( CWorld* world );

	// Perform component detached from world, calls OnDetached for components that were attached
	void PerformDetachment( CWorld* world );

	// Perform component uninitialization, calls Uninitialize if was not called before
	void PerformUninitialization();

	// Perform component initialization, calls Initialize if was not called before
	void PerformInitialization();

	// !SLOW! Perform full component recreation (detach -> deinitialize -> initialize -> attach)
	void PerformFullRecreation();

#ifdef USE_UMBRA
	virtual Bool OnAttachToUmbraScene( CUmbraScene* umbraScene, const struct VectorI& bounds ) { return false; }
#endif

#ifndef NO_EDITOR_FRAGMENTS
	// Generate editor rendering fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );
#endif

	// Generate hit proxy fragments for editor
	virtual void OnGenerateEditorHitProxies( CHitProxyMap& map );

	// Handle missing property
	virtual Bool OnPropertyMissing( CName propertyName, const CVariant& readValue );

	// Called when the component has been added or removed because of an appearance change in the entity
	virtual void OnAppearanceChanged( Bool added );

	// Called after the component has been created from the streaming system
	virtual void OnStreamIn();

	// Called before the component is destroyed by the streaming system
	virtual void OnStreamOut();

	// Visibility flag somewhere below was changed
	virtual void OnRefreshVisibilityFlag();

	// Visibility flag was forced to change immediately
	virtual void OnVisibilityForced();

	// New parent attachment was added
	virtual void OnParentAttachmentAdded( IAttachment* attachment );

	// Parent attachment was broken
	virtual void OnParentAttachmentBroken( IAttachment* attachment );

#ifndef NO_EDITOR_ENTITY_VALIDATION
	// Called when entity is being validated
	virtual Bool OnValidate( TDynArray< String >& /*log*/ ) const { return true; }
#endif

public:
	// Component update, called before physics simulation
	virtual void OnTickPrePhysics( Float timeDelta );

	// Component update, called before physics simulation
	virtual void OnTickPrePhysicsPost( Float timeDelta );

	// Main component update
	virtual void OnTick( Float timeDelta );

	// Component update, called after physics simulation
	virtual void OnTickPostPhysics( Float timeDelta );

	// Component update, called after physics simulation, second
	virtual void OnTickPostPhysicsPost( Float timeDelta );

	// Component update, called after update transform
	virtual void OnTickPostUpdateTransform( Float timeDelta );

	//! Called when we need to store gameplay state of this entity
	virtual void OnSaveGameplayState( IGameSaver* saver );
	virtual Bool CheckShouldSave() const { return ShouldSave(); /* if somebody just set the falg manually, then it should remain set */ }
	void SetShouldSave( Bool should );

	//! Called when we need to restore gameplay state of this entity
	virtual void OnLoadGameplayState( IGameLoader* loader ); 

	virtual Bool CanBeSavedDirectlyToStream() const { return false; }
	virtual void StreamSave( ISaveFile* stream ) { RED_UNUSED( stream ); }
	virtual void StreamLoad( ISaveFile* stream, Uint32 version ) { RED_UNUSED( stream ); RED_UNUSED( version ); }

	//! Called after entity was instanced
	virtual void OnPostInstanced() override;

	//! Called after entity this component is in was loaded from a layer file, before OnAttach is called and before entity state is restored from save game
	virtual void OnEntityLoaded() {}

	virtual void OnPropertyExternalChanged( const CName& propertyName ) override;

	// reset physical simulation on component
	virtual void OnResetClothAndDangleSimulation() {}

public:
	// Start sprite editing on this component
	// by Dex: this could be merged with OnEditorBeginVertexEdit
	virtual Bool OnEditorBeginSpriteEdit( TDynArray<CEntity*> &/*spritesToEdit*/ ) { return false; }

	// End sprite editing on this component
	// by Dex: this could be merged with OnEditorEndVertexEdit
	virtual void OnEditorEndSpriteEdit() {}

	// Begin vertex edit mode
	virtual Bool OnEditorBeginVertexEdit( TDynArray< Vector >& /*vertices*/, Bool& /*isClosed*/, Float& /*height*/ ) { return false; }

	// End vertex edit mode
	virtual void OnEditorEndVertexEdit() {}

	// Insert editor vertex
	virtual Bool OnEditorVertexInsert( Int32 /*edge*/, const Vector& /*wishedPosition*/, Vector& /*allowedPosition*/, Int32& /*outInsertPos*/ ) { return false; }

	// Editor vertex deleted
	virtual Bool OnEditorVertexDestroy( Int32 /*vertexIndex*/ ) { return false; }


	virtual PathLib::IComponent* AsPathLibComponent();

public:
	//! Get value of effect parameter
	virtual Bool GetEffectParameterValue( CName /*paramName*/, EffectParameterValue &/*value*/ /* out */ ) const { return false; }

	//! Set value of effect parameter
	virtual Bool SetEffectParameterValue( CName /*paramName*/, const EffectParameterValue &/*value*/ ) { return false; }

	//! Enumerate effect parameters
	virtual void EnumEffectParameters( CFXParameters &/*effectParams*/ /* out */ ) {}

public:
	//! Get all components attached to this one, recursive
	void GetAllChildAttachedComponents( TDynArray< CComponent* > &components ) const;

	//! Break all attachments of this component
	void BreakAllAttachments();

private:
	//! Link this component into internal list of attached components
	void LinkToAttachedComponentsList( CComponent*& list );

	//! Unlink this component from the internal list of attached components
	void UnlinkFromAttachedComponentsList();

public:
	//! Returns true if we should write this entity to disk when saving the entity
	virtual Bool ShouldWriteToDisk() const;

	//! Does this component support additional data saving in layer (even for template entity) ?
	virtual Bool SupportsAdditionalInstanceData() const { return false; }

	//! Serialize additional component data
	virtual void OnSerializeAdditionalData( IFile& /*file*/ ) {};

	//! Does this component have templated entity instance properties?
	Bool HasInstanceProperties() const;

	//! Save the instance properties for this component
	void SaveInstanceProperties( IFile& file ) const;

	//! Load the instance properties for this component
	void LoadInstanceProperties( IFile& file );

	//! Collects modified instance properties
	virtual void CollectModifiedInstanceProperties( TDynArray< CProperty* >& modifiedProperties ) const;

	//! Returns the names of the instance properties
	virtual void GetInstancePropertyNames( TDynArray< CName >& instancePropertyNames ) const;

	//! Returns true if the given property can be overriden by a template include
	virtual Bool CanOverridePropertyViaInclusion( const CName& propertyName ) const;

	//! Returns the minimum streaming distance for this component
	virtual Uint32 GetMinimumStreamingDistance() const { return 0; }

	//! Returns true if this component is mergable into world merged geometry
	virtual Bool IsMergableIntoWorldGeometry() const { return false; }

	virtual const String& GetName() const override { return m_name; }
	void SetName( const String& name ) { m_name = name; }

#ifndef NO_EDITOR
	virtual void EditorOnTransformChanged() override;
#endif

private:
	void funcGetEntity( CScriptStackFrame& stack, void* result );
	void funcIsEnabled( CScriptStackFrame& stack, void* result );
	void funcSetEnabled( CScriptStackFrame& stack, void* result );
	void funcSetPosition( CScriptStackFrame& stack, void* result );
	void funcSetRotation( CScriptStackFrame& stack, void* result );
	void funcSetScale( CScriptStackFrame& stack, void* result );
	void funcHasDynamicPhysic( CScriptStackFrame& stack, void* result );
	void funcHasCollisionType( CScriptStackFrame& stack, void* result );
	void funcGetPhysicalObjectLinearVelocity( CScriptStackFrame& stack, void* result );
	void funcGetPhysicalObjectAngularVelocity( CScriptStackFrame& stack, void* result );
	void funcSetPhysicalObjectLinearVelocity( CScriptStackFrame& stack, void* result );
	void funcSetPhysicalObjectAngularVelocity( CScriptStackFrame& stack, void* result );
	void funcGetPhysicalObjectMass( CScriptStackFrame& stack, void* result );
	void funcApplyTorqueToPhysicalObject( CScriptStackFrame& stack, void* result );
	void funcApplyLocalImpulseToPhysicalObject( CScriptStackFrame& stack, void* result );
	void funcApplyForceAtPointToPhysicalObject( CScriptStackFrame& stack, void* result );
	void funcApplyTorqueImpulseToPhysicalObject( CScriptStackFrame& stack, void* result );
	void funcApplyForceToPhysicalObject( CScriptStackFrame& stack, void* result );
	virtual void funcGetPhysicalObjectBoundingVolume( CScriptStackFrame& stack, void* result );
	void funcSetShouldSave( CScriptStackFrame& stack, void* result );
};

BEGIN_ABSTRACT_CLASS_RTTI( CComponent );
	PARENT_CLASS( CNode );
	PROPERTY_EDIT( m_name, TXT("Node name") );
	PROPERTY_EDIT( m_isStreamed, TXT("Is the component streamed") );

#ifndef NO_COMPONENT_GRAPH
	PROPERTY_NOT_COOKED( m_graphPositionX );
	PROPERTY_NOT_COOKED( m_graphPositionY );
#endif

	NATIVE_FUNCTION( "GetEntity", funcGetEntity );
	NATIVE_FUNCTION( "IsEnabled", funcIsEnabled );
	NATIVE_FUNCTION( "SetEnabled", funcSetEnabled );
	NATIVE_FUNCTION( "SetPosition", funcSetPosition );
	NATIVE_FUNCTION( "SetRotation", funcSetRotation );
	NATIVE_FUNCTION( "SetScale", funcSetScale );
	NATIVE_FUNCTION( "HasDynamicPhysic", funcHasDynamicPhysic );
	NATIVE_FUNCTION( "HasCollisionType", funcHasCollisionType );
	NATIVE_FUNCTION( "GetPhysicalObjectLinearVelocity", funcGetPhysicalObjectLinearVelocity );
	NATIVE_FUNCTION( "GetPhysicalObjectAngularVelocity", funcGetPhysicalObjectAngularVelocity );
	NATIVE_FUNCTION( "SetPhysicalObjectLinearVelocity", funcSetPhysicalObjectLinearVelocity );
	NATIVE_FUNCTION( "SetPhysicalObjectAngularVelocity", funcSetPhysicalObjectAngularVelocity );
	NATIVE_FUNCTION( "GetPhysicalObjectMass", funcGetPhysicalObjectMass );
	NATIVE_FUNCTION( "ApplyTorqueToPhysicalObject", funcApplyTorqueToPhysicalObject );
	NATIVE_FUNCTION( "ApplyForceAtPointToPhysicalObject", funcApplyForceAtPointToPhysicalObject );
	NATIVE_FUNCTION( "ApplyForceToPhysicalObject", funcApplyForceToPhysicalObject );
	NATIVE_FUNCTION( "ApplyLocalImpulseToPhysicalObject", funcApplyLocalImpulseToPhysicalObject );
	NATIVE_FUNCTION( "ApplyTorqueImpulseToPhysicalObject", funcApplyTorqueImpulseToPhysicalObject );
	NATIVE_FUNCTION( "GetPhysicalObjectBoundingVolume", funcGetPhysicalObjectBoundingVolume );
	NATIVE_FUNCTION( "SetShouldSave", funcSetShouldSave );
END_CLASS_RTTI();
