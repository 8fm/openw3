/**
* Copyright ©2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../engine/persistentEntity.h"
#include "../engine/localizableObject.h"
#include "characterStats.h"
#include "../engine/propertyAnimationSet.h"
#include "gameplayInfoCache.h"

struct SItemUniqueId;

class CInventoryComponent;
class CCharacterStats;
class CPropertyAnimationSet;	// animated properties

class CInteraction;

enum EGameplayEntityFlags
{
	FLAG_HasWind	= FLAG( 0 ),
	FLAG_HasVehicle	= FLAG( 1 ),
};

BEGIN_ENUM_RTTI( EGameplayEntityFlags )
	ENUM_OPTION( FLAG_HasWind )
	ENUM_OPTION( FLAG_HasVehicle )
END_ENUM_RTTI()


struct SEntityMaterialEffectInfo
{
	DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_Gameplay );

	TDynArray< CName >			m_activeEffects;				//!< List of active material effects.
	SMaterialReplacementInfo*	m_savedMaterialReplacementInfo;	//!< When the last material effect is stopped, this will be used to restore any material replacements.

	SEntityMaterialEffectInfo() : m_savedMaterialReplacementInfo( NULL ) {}
	~SEntityMaterialEffectInfo() { ClearMaterialReplacement(); }

	void ClearMaterialReplacement()
	{
		if ( m_savedMaterialReplacementInfo )
		{
			delete m_savedMaterialReplacementInfo;
			m_savedMaterialReplacementInfo = NULL;
		}
	}

	void SetMaterialReplacement( SMaterialReplacementInfo* mtlReplacement )
	{
		ClearMaterialReplacement();

		if ( mtlReplacement )
		{
			m_savedMaterialReplacementInfo = new SMaterialReplacementInfo( *mtlReplacement );
		}
	}

	void SetMaterialReplacement( IMaterial* material, Bool drawOriginal, const CName& tag, const CName& exclusionTag, const TDynArray<CName>* includeList, const TDynArray<CName>* excludeList, Bool forceMeshAlternatives )
	{
		ClearMaterialReplacement();

		m_savedMaterialReplacementInfo = new SMaterialReplacementInfo();
		m_savedMaterialReplacementInfo->material = material;
		m_savedMaterialReplacementInfo->drawOriginal = drawOriginal;
		m_savedMaterialReplacementInfo->tag = tag;
		m_savedMaterialReplacementInfo->exclusionTag = exclusionTag;
		m_savedMaterialReplacementInfo->forceMeshAlternatives = forceMeshAlternatives;
		if ( includeList != nullptr )
		{
			m_savedMaterialReplacementInfo->includeList.PushBack( *includeList );
		}
		if ( excludeList != nullptr )
		{
			m_savedMaterialReplacementInfo->excludeList.PushBack( *excludeList );
		}
	}
};

//////////////////////////////////////////////////////////////////////////
// W3 related stuff - have to placed here, since there is need for CGameplayEntity to use it

enum EFocusModeVisibility
{
	FMV_None,
	FMV_Interactive,
	FMV_Clue,
};

BEGIN_ENUM_RTTI( EFocusModeVisibility )
	ENUM_OPTION( FMV_None );
	ENUM_OPTION( FMV_Interactive );
	ENUM_OPTION( FMV_Clue );
END_ENUM_RTTI()

//////////////////////////////////////////////////////////////////////////

/// An entity meaningful in terms of the gameplay
class CGameplayEntity : public CPeristentEntity, public ILocalizableObject
{
	DECLARE_ENGINE_CLASS( CGameplayEntity, CPeristentEntity, 0 )	

public:
	// Manages animation events sent to scripts
	struct ScriptAnimEventManager
	{
	private:
		struct ChildListener
		{
			THandle< CNode > m_object;
			CName m_functionName;

			ChildListener( CNode* object, CName functionName );
		};

		struct Entry
		{
			CName m_eventName;
			CName m_functionName;
			TDynArray< ChildListener > m_children;
		};

		struct SEventData
		{
			THandle< CGameplayEntity >	m_entity;
			CName						m_eventName;
			EAnimationEventType			m_eventType;
			SAnimationEventAnimInfo		m_animInfo;
		};

		TDynArray< Entry > m_entries;
		
		static TDynArray< SEventData >	s_queuedEvents;

		Entry* FindEntry( CName eventName );

	public:
		void AddCallback( CName eventName, CName functionName );
		void RemoveCallback( CName eventName );

		void AddChildCallback( CNode* child, CName eventName, CName functionName );
		void RemoveChildCallback( CNode* child, CName eventName );

		void OnEvent( CGameplayEntity* entity, CName eventName, EAnimationEventType eventType, const SAnimationEventAnimInfo& animInfo );

		static void FireQueuedEvents();
		static void ClearQueuedEvents();

	private:

		void OnEvent_Internal( CGameplayEntity* entity, CName eventName, EAnimationEventType eventType, const SAnimationEventAnimInfo& animInfo );
	};


protected:

	Vector										m_aimVector;					//!< The relative position for showing GUI aim (e.g. for highlighting containers)
	mutable THandle< CInventoryComponent >		m_inventoryComponent;			//!< Inventory 
	CPropertyAnimationSet*						m_propertyAnimationSet;			//!< animated properties storage
	CCharacterStats*							m_stats;						//!< Stats
	LocalizedString								m_displayName;					//!< Localized display name																			// its actually an Uint32
	Uint32										m_gameplayFlags;				//!< Gameplay flags
	SGameplayInfoCache							m_infoCache;					//!< Cached information about entity gameplay functionality (IsInteractive, HasSomething, etc...)	// its actually an Uint32
	Bool										m_isInteractionActivator;		//!< This entity will activate interactions
	EFocusModeVisibility						m_focusModeVisibility;			//!< Type of highlight in focus mode

	SEntityMaterialEffectInfo*					m_materialEffects;				//!< If a material effect is active, this will hold the relevent information.
	ScriptAnimEventManager						m_scriptAnimEventManager;
	
	CName										m_sfxTagCached;
#ifdef RED_ASSERTS_ENABLED
	Bool										m_sfxTagCachedFlag;
#endif

#ifndef RED_FINAL_BUILD
	TSet< EShowFlags >							m_scriptedVisualDebugFlags;		//!< List of registered visual debug filters
	class CScriptedRenderFrame*					m_scriptedRenderFrame;			//!< Scripted interface for CRenderFrame
#endif

public:
	CGameplayEntity();

	virtual void OnInitialized() override;

	virtual void OnPostComponentsInitializedAsync() override;

	//! Is this entity interaction activator
	RED_INLINE Bool IsInteractionActivator() const { return m_isInteractionActivator; }

	//! Get (and create if needed) character statistics
	RED_INLINE CCharacterStats* GetCharacterStats() { return m_stats ? m_stats : ( m_stats = ::CreateObject< CCharacterStats >( this ) ); }

	//! Get the localized display name of entity
	RED_INLINE const LocalizedString& GetLocalizedDisplayName() const { return m_displayName; }

	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

	void ProcessAnimationEvent( const CAnimationEventFired* event ) override;

public:
	//! Get the inventory component
	RED_INLINE CInventoryComponent* GetInventoryComponent() const { return m_inventoryComponent.Get(); }

	//! Instantiate equipment state
	void InitInventory( Bool initFromTemplate = true );

	// return animated properties storage
	CPropertyAnimationSet* GetPropertyAnimationSet() { return m_propertyAnimationSet; }

	// CObjects - called when property was changed externally
	virtual void OnPropertyExternalChanged( const CName& propertyName );

	//! Item was added to inventory
	virtual void OnGrabItem( SItemUniqueId itemId, CName slot );

	//! Item was removed from inventory
	virtual void OnPutItem( SItemUniqueId itemId, Bool emptyHand );

	//! Item was mounted
	virtual void OnMountItem( SItemUniqueId itemId, Bool wasHeld );

	//! Item was unmounted
	virtual void OnUnmountItem( SItemUniqueId itemId );

	//! Item was enhanced
	virtual void OnEnhanceItem( SItemUniqueId enhancedItemId, Int32 slotIndex );

	//! Item enhancement was removed
	virtual void OnRemoveEnhancementItem( SItemUniqueId enhancedItemId, Int32 slotIndex );

	//! Item was added to inventory
	virtual void OnAddedItem( SItemUniqueId itemId );
	
	//! Item was removed from inventory
	virtual void OnRemovedItem( SItemUniqueId itemId );

	//! Item ability was added
	virtual void OnItemAbilityAdded( SItemUniqueId itemId, CName ability );

	//! Item ability was removed
	virtual void OnItemAbilityRemoved( SItemUniqueId itemId, CName ability );

	//! Try and equip item
	virtual void EquipItem( SItemUniqueId itemId, Bool ignoreMount );

	//! Try and unequip item
	virtual void UnequipItem( SItemUniqueId itemId );

	// Entity was loaded from existing layer
	virtual void OnLoaded();

	// Layer this entity is on is unloading. Called after all detaches.
	virtual void OnLayerUnloading();

	// Update transform data (called from multiple threads!!!)
	virtual void OnUpdateTransformEntity();
	
	//! Serialize entity
	virtual void OnSerialize( IFile& file ) override;

	//! Notify renderer proxy attached to entity
	virtual void OnProxyAttached( CComponent* component, IRenderProxy* proxy ) override;

	//! Update entity state after stream in
	virtual void OnStreamIn() override;

public:
	//! Start a "material effect", which overrides any current material replacements. Multiple material effects may
	//! be active, material replacement will be restored after all have been stopped.
	virtual void StartMaterialEffect( const CName& effectName );
	virtual void StopMaterialEffect( const CName& effectName );
	Bool HasMaterialEffect() const { return m_materialEffects != NULL; }

	//! Set all components with a given tag as either visible or invisible.
	virtual void SetComponentsVisible( const CName& componentTag, Bool visible );

	//! Set material replacement
	virtual Bool SetMaterialReplacement( IMaterial* material, Bool drawOriginal = false, const CName& tag = CName::NONE, const CName& exclusionTag = CName::NONE, const TDynArray< CName >* includeList = nullptr, const TDynArray< CName >* excludeList = nullptr, Bool forceMeshAlternatives = false ) override;

	//! Disable material replacement
	virtual void DisableMaterialReplacement();

public:
	// Apply equipment from given appearance
	void ApplyAppearanceEquipment( const CEntityAppearance* appearance );

public:
	//! Get display name of entity ( can be localized )
	virtual String GetDisplayName() const;

	//! Get all localized strings inside this object
	virtual void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) /*const*/;

protected:
	// Entity is being destroyed
	virtual void OnDestroyed( CLayer* layer ) override;

	// Entity was created, not yet attached or initialized
	virtual void OnCreated( CLayer* layer, const EntitySpawnInfo& info ) override;

	//! Entity was attached to world
	virtual void OnAttached( CWorld* world ) override;

	//! Entity was detached from world
	virtual void OnDetached( CWorld* world ) override;

	//! Entity was finally attached to world
	virtual void OnAttachFinished( CWorld* world ) override;

	//! Entity was finally attached to world in editor
	virtual void OnAttachFinishedEditor( CWorld* world ) override;

	//! Called when we need to store gameplay state of this entity
	virtual void OnSaveGameplayState( IGameSaver* saver );

	//! Called when we need to restore gameplay state of this entity
	virtual void OnLoadGameplayState( IGameLoader* loader );

	//! Should save?
	virtual Bool CheckShouldSave() const;

	//! Is this ok to extract components from this entity during sector data building ?
	virtual Bool CanExtractComponents( const Bool isOnStaticLayer ) const;

#ifndef NO_EDITOR
		// Editor only stuff
	virtual void EditorOnTransformChangeStart() override;
	// Editor only stuff
	virtual void EditorOnTransformChanged() override;
	// Editor only stuff
	virtual void EditorOnTransformChangeStop() override;

	virtual void EditorPostCreation() override;
	virtual void OnComponentTransformChanged( CComponent* component ) override;
#endif

public:
	// Entity aiming
	RED_INLINE Vector GetAimVector() const { return m_aimVector; }
	RED_INLINE void SetAimVector( const Vector &value ) { m_aimVector = value; }
	virtual Vector GetAimPosition() const;
	virtual Vector GetBarPosition() const;

	virtual void GetStorageBounds( Box& box ) const override;

	// Gameplay flags
	RED_INLINE Bool HasGameplayFlags( EGameplayEntityFlags flags ) const { return ( m_gameplayFlags & flags ) != 0; }
	RED_INLINE void SetGameplayFlags( EGameplayEntityFlags flags ) { m_gameplayFlags |= flags; }
	RED_INLINE void ClearGameplayFlags( EGameplayEntityFlags flags ) { m_gameplayFlags &= ~flags; }

	RED_INLINE const SGameplayInfoCache& GetInfoCache() { return m_infoCache; }

	RED_INLINE EFocusModeVisibility GetFocusModeVisibility() const { return m_focusModeVisibility; }
	void SetFocusModeVisibility( EFocusModeVisibility focusModeVisibility, Bool persistent = false, Bool force = false );

	//! Start animation on all properties with matching animation name; count of 0 indicates infinite looping; lengthScale indicates how much to scale the total animation time by
	void PlayPropertyAnimation( CName animationName, Uint32 count = 0, Float lengthScale = 1.0f, EPropertyCurveMode mode = PCM_Forward );
	//! Stops animation on all properties with matching animation name and restores initial property values
	void StopPropertyAnimation( CName animationName, Bool restoreInitialValues = true );
	//! Stops all animations and restores initial property values
	void StopAllPropertyAnimations( Bool restoreInitialValues = true );
	//! Rewinds animation on all properties with matching animation name to given time; Note: changes property values immediately
	void RewindPropertyAnimation( CName animationName, Float time );
	//! Pause animation on all properties with matching animation name
	void PausePropertyAnimation( CName animationName );
	//! Unpause animation on all properties with matching animation name
	void UnpausePropertyAnimation( CName animationName );
	//! Gets currently running animation's time; returns true on success
	Bool GetPropertyAnimationInstanceTime( CName propertyName, CName animationName, Float& outTime );
	//! Gets animation's length
	Bool GetPropertyAnimationLength( CName propertyName, CName animationName, Float& length );
	//! Gets animation's transform at given time; returns true on success
	Bool GetPropertyAnimationTransformAt( CName propertyName, CName animationName, Float time, EngineTransform& outTransform );

	RED_INLINE virtual const CName& GetSfxTag() const { RED_ASSERT( m_sfxTagCachedFlag ); return m_sfxTagCached; }
	
private:
	void CacheSfxTag();

	void funcGetInventory( CScriptStackFrame& stack, void* result );
	void funcGetDisplayName( CScriptStackFrame& stack, void* result );
	void funcGetCharacterStats( CScriptStackFrame& stack, void* result );
	void funcPlayPropertyAnimation( CScriptStackFrame& stack, void* result );
	void funcStopPropertyAnimation( CScriptStackFrame& stack, void* result );
	void funcRewindPropertyAnimation( CScriptStackFrame& stack, void* result );
	void funcGetPropertyAnimationInstanceTime( CScriptStackFrame& stack, void* result );
	void funcGetPropertyAnimationLength( CScriptStackFrame& stack, void* result );
	void funcGetPropertyAnimationTransformAt( CScriptStackFrame& stack, void* result );
	void funcGetGameplayEntityParam( CScriptStackFrame& stack, void* result );
	void funcEnableVisualDebug( CScriptStackFrame& stack, void* result );
	void funcGetStorageBounds( CScriptStackFrame& stack, void* result );
	void funcGetGameplayInfoCache( CScriptStackFrame& stack, void* result );
	void funcGetFocusModeVisibility( CScriptStackFrame& stack, void* result );
	void funcSetFocusModeVisibility( CScriptStackFrame& stack, void* result );
	void funcAddAnimEventCallback( CScriptStackFrame& stack, void* result );
	void funcRemoveAnimEventCallback( CScriptStackFrame& stack, void* result );
	void funcAddAnimEventChildCallback( CScriptStackFrame& stack, void* result );
	void funcRemoveAnimEventChildCallback( CScriptStackFrame& stack, void* result );
	void funcGetSfxTag( CScriptStackFrame& stack, void* result );

};

BEGIN_CLASS_RTTI( CGameplayEntity );
	PARENT_CLASS( CPeristentEntity );
	PROPERTY_INLINED( m_propertyAnimationSet, TXT("Animated Properties Storage") );
	PROPERTY_CUSTOM_EDIT( m_displayName, TXT( "A localized name of this entity" ), TXT( "EntityDisplayNameSelector" ) );
	PROPERTY_NOSERIALIZE( m_stats ); // CODE SPLIT
	PROPERTY_EDIT( m_isInteractionActivator, TXT("This entity is interaction activator") );
	PROPERTY_EDIT( m_aimVector, TXT("Aim vector (shift)") );
	PROPERTY( m_gameplayFlags );
	PROPERTY_EDIT( m_focusModeVisibility, TXT("Type of highlight in focus mode") );
	NATIVE_FUNCTION( "GetInventory", funcGetInventory );
	NATIVE_FUNCTION( "GetDisplayName", funcGetDisplayName );
	NATIVE_FUNCTION( "GetCharacterStats", funcGetCharacterStats );
	NATIVE_FUNCTION( "PlayPropertyAnimation", funcPlayPropertyAnimation );
	NATIVE_FUNCTION( "StopPropertyAnimation", funcStopPropertyAnimation );
	NATIVE_FUNCTION( "RewindPropertyAnimation", funcRewindPropertyAnimation );
	NATIVE_FUNCTION( "GetGameplayEntityParam", funcGetGameplayEntityParam );
	NATIVE_FUNCTION( "GetPropertyAnimationInstanceTime", funcGetPropertyAnimationInstanceTime );
	NATIVE_FUNCTION( "GetPropertyAnimationLength", funcGetPropertyAnimationLength );
	NATIVE_FUNCTION( "GetPropertyAnimationTransformAt", funcGetPropertyAnimationTransformAt );
	NATIVE_FUNCTION( "EnableVisualDebug", funcEnableVisualDebug );
	NATIVE_FUNCTION( "GetStorageBounds", funcGetStorageBounds );
	NATIVE_FUNCTION( "GetGameplayInfoCache", funcGetGameplayInfoCache );
	NATIVE_FUNCTION( "GetFocusModeVisibility", funcGetFocusModeVisibility );
	NATIVE_FUNCTION( "SetFocusModeVisibility", funcSetFocusModeVisibility );

	NATIVE_FUNCTION( "AddAnimEventCallback", funcAddAnimEventCallback );
	NATIVE_FUNCTION( "RemoveAnimEventCallback", funcRemoveAnimEventCallback );
	NATIVE_FUNCTION( "AddAnimEventChildCallback", funcAddAnimEventChildCallback );
	NATIVE_FUNCTION( "RemoveAnimEventChildCallback", funcRemoveAnimEventChildCallback );
	NATIVE_FUNCTION( "GetSfxTag", funcGetSfxTag	);
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////////////////

class IEntityStateChangeRequest : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IEntityStateChangeRequest, CObject );

public:
	virtual ~IEntityStateChangeRequest() {}

	virtual void Execute( CGameplayEntity* entity ) {}

	// Generate a debug description of the request
	virtual String OnStateChangeRequestsDebugPage() const;

	// ------------------------------------------------------------------------
	// Save game
	// ------------------------------------------------------------------------
	// Saves state of the request
	void SaveState( IGameSaver* saver );

	// Restores state of the request
	static IEntityStateChangeRequest* RestoreState( IGameLoader* loader, CObject* parent );
};
BEGIN_ABSTRACT_CLASS_RTTI( IEntityStateChangeRequest );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CScriptedEntityStateChangeRequest : public IEntityStateChangeRequest
{
	DECLARE_ENGINE_CLASS( CScriptedEntityStateChangeRequest, IEntityStateChangeRequest, 0 );

public:
	virtual void Execute( CGameplayEntity* entity );
};
BEGIN_CLASS_RTTI( CScriptedEntityStateChangeRequest );
	PARENT_CLASS( IEntityStateChangeRequest );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
