/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "entityColorVariant.h"
#include "entityAppearance.h"
#include "../core/engineTransform.h"
#include "../core/resource.h"
#include "../core/latentDataBuffer.h"
#include "../core/list.h"
#include "../core/hashset.h"
#include "../core/dependencyLoader.h"
#include "entityTemplatePreloadedEffects.h"
#include "jobEffectPreloading.h"
#include "entityTemplateCookedEffect.h"

class CEntity;
class CFXDefinition;
class CEntityTemplateParam;
class HardAttachmentSpawnInfo;

class DependencySavingContext;

enum ECookingPlatform : Int32;

//////////////////////////////////////////////////////////////////////////
//! Loading context used by CDependencyLoaderCreateEntity
struct SDependencyLoaderCreateEntityContext : DependencyLoadingContext
{
	CClass* m_srcEntityClass; //!< Source class to convert from
	CClass* m_dstEntityClass; //!< Destination class to convert to

	SDependencyLoaderCreateEntityContext()
		: m_srcEntityClass( NULL )
		, m_dstEntityClass( NULL )
	{}
};

//! Entity template specific dependency loader capable of instancing entity of a subclass of its template's entity class
class CDependencyLoaderCreateEntity : public CDependencyLoader
{
public:
	CDependencyLoaderCreateEntity( IFile& file, CDiskFile* loadedFile )
		: CDependencyLoader( file, loadedFile )
	{}

	virtual CClass* ChangeExportClass( CClass* exportClass, const DependencyLoadingContext* context ) override
	{
		const SDependencyLoaderCreateEntityContext* subContext = (const SDependencyLoaderCreateEntityContext*) context;
		return ( subContext->m_srcEntityClass == exportClass ) ? subContext->m_dstEntityClass : exportClass;
	}
};

//////////////////////////////////////////////////////////////////////////

struct SAttachmentReplacement
{
	CName m_oldName;
	CName m_oldClass;
	CName m_newName;
	CName m_newClass;

	SAttachmentReplacement();
	SAttachmentReplacement( const SAttachmentReplacement& src )
		: m_oldName( src.m_oldName )
		, m_oldClass( src.m_oldClass )
		, m_newName( src.m_newName )
		, m_newClass( src.m_newClass )
	{

	}

	DECLARE_RTTI_STRUCT( SAttachmentReplacement )
};

BEGIN_CLASS_RTTI( SAttachmentReplacement );
	PROPERTY( m_oldName );
	PROPERTY( m_oldClass );
	PROPERTY( m_newName );
	PROPERTY( m_newClass );
END_CLASS_RTTI();

struct SAttachmentReplacements
{
	TDynArray< SAttachmentReplacement > m_replacements;

	RED_INLINE SAttachmentReplacements() {}

	RED_INLINE SAttachmentReplacements( const SAttachmentReplacements& src )
		: m_replacements( src.m_replacements )
	{
	}

	void Add( CComponent* oldComponent, CComponent* newComponent );
	void Apply( struct SSavedAttachments& savedAttachments );

	DECLARE_RTTI_STRUCT( SAttachmentReplacements )
};

BEGIN_CLASS_RTTI( SAttachmentReplacements );
	PROPERTY( m_replacements );
END_CLASS_RTTI();

struct SSavedAttachments
{
	struct SSavedAttachment
	{
		IAttachment* m_attachmentCopy;
		CName m_parentName;
		CName m_parentClass;
		CName m_childName;
		CName m_childClass;
	};

	TDynArray< SSavedAttachment > m_attachments;
	SAttachmentReplacements m_replacements;

	~SSavedAttachments();

	void Add( IAttachment* attachment );
	void Add( const TList< IAttachment* >& attachments );
	void Add( CComponent* component );
	void Add( CEntity* entity );
	void AddReplacement( CComponent* oldComponent, CComponent* newComponent );
	void Apply( CEntity* entity );
};

//////////////////////////////////////////////////////////////////////////

class IEntityTemplatePropertyOwner
{
public:
	virtual CEntityTemplate* Editor_GetEntityTemplate() = 0;
};

//////////////////////////////////////////////////////////////////////////

/// Instancing info for entity template
class EntityTemplateInstancingInfo
{
public:
	Bool		m_detachFromTemplate;			//!< Detach created entity from the template
	Bool		m_previewOnly;					//!< Entity is spawned for a preview in editor
	Bool		m_async;						//!< We are doing the instancing on the thread
	CClass*		m_entityClass;					//!< Optional parameter indicating what class to instanciate (expects subclass of the entity template's entity)

public:
	EntityTemplateInstancingInfo()
		: m_detachFromTemplate( false )
		, m_previewOnly( false )
		, m_async( false )
		, m_entityClass( NULL )
	{};
};


struct VoicetagAppearancePair
{
	CName	m_voicetag;
	CName	m_appearance;

	VoicetagAppearancePair() : m_voicetag( CName::NONE ), m_appearance( CName::NONE ) {};
	VoicetagAppearancePair( CName voicetag, CName appearance ) : m_voicetag( voicetag ), m_appearance( appearance ) {};

	bool operator == ( const VoicetagAppearancePair& rhs ) const
	{
		return m_voicetag == rhs.m_voicetag && m_appearance == rhs.m_appearance;
	}

	DECLARE_RTTI_STRUCT( VoicetagAppearancePair )
};

BEGIN_CLASS_RTTI( VoicetagAppearancePair );
	PROPERTY_EDIT( m_voicetag, TXT( "Voicetag" ) );
	PROPERTY_EDIT( m_appearance, TXT( "Appearance" ) );
END_CLASS_RTTI();

RED_DECLARE_NAME( voicetagAppearances )

///////////////////////////////////////////////////////////

/// Stores coloring information for a single mesh component referenced
/// directly or indirectly by an entity template
struct SEntityTemplateColoringEntry
{
	CName				m_appearance;			//!< Appearance for the setting (can be NONE)
	CName				m_componentName;		//!< Name of the mesh component
	CColorShift			m_colorShift1;			//!< Color shift 1 to use
	CColorShift			m_colorShift2;			//!< Color shift 2 to use

	SEntityTemplateColoringEntry() : m_appearance( CName::NONE ), m_componentName( CName::NONE ) {}
	SEntityTemplateColoringEntry( const SEntityTemplateColoringEntry& src )
	{
		m_appearance = src.m_appearance;
		m_componentName = src.m_componentName;
		m_colorShift1 = src.m_colorShift1;
		m_colorShift2 = src.m_colorShift2;
	}
	SEntityTemplateColoringEntry( const CName& appearance, const CName& componentName, const CColorShift& colorShift1, const CColorShift colorShift2 )
	{
		m_appearance = appearance;
		m_componentName = componentName;
		m_colorShift1 = colorShift1;
		m_colorShift2 = colorShift2;
	}

	DECLARE_RTTI_STRUCT( SEntityTemplateColoringEntry )
};

BEGIN_CLASS_RTTI( SEntityTemplateColoringEntry );
	PROPERTY_EDIT( m_appearance, TXT("Appearance") );
	PROPERTY_EDIT( m_componentName, TXT("Component Name") );
	PROPERTY_EDIT( m_colorShift1, TXT("Color Shift 1") );
	PROPERTY_EDIT( m_colorShift2, TXT("Color Shift 2") );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////

struct SEntityTemplateOverride
{
	CName					m_componentName;				//!< Name of the component to override
	CName					m_className;					//!< Class name of the component to override
	TDynArray< CName >		m_overriddenProperties;			//!< Names of the properties to override

	SEntityTemplateOverride() : m_componentName( CName::NONE ), m_className( CName::NONE ) {}
	SEntityTemplateOverride( const SEntityTemplateOverride& src )
		: m_componentName( src.m_componentName )
		, m_className( src.m_className )
		, m_overriddenProperties( src.m_overriddenProperties )
	{}

	DECLARE_RTTI_STRUCT( SEntityTemplateOverride )
};

BEGIN_CLASS_RTTI( SEntityTemplateOverride );
	PROPERTY( m_componentName );
	PROPERTY( m_className );
	PROPERTY( m_overriddenProperties );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

/// Represents a single streamed attachment (attachment between two components where
/// at least one is a streamed component, in serialized form)
struct SStreamedAttachment
{
	CName				m_parentName;		//! parent (source) component info
	CName				m_parentClass;		//! ...
	CName				m_childName;		//! child (target) component info
	CName				m_childClass;		//! ...
	TDynArray< Uint8 >	m_data;				//! attachment in serialized form

	//! Initialize this structure from the given attachment
	Bool InitFromAttachment( IAttachment* attachment );

	//! Create the attachment described in this structure in the given entity
	//! Optionally fail the attachment creation if it is not for one of the components in the array
	Bool CreateInEntity( CEntity* entity, IAttachment*& attachment, const TDynArray< CComponent* >* limitToComponents ) const;

	RED_FORCE_INLINE Uint32 CalcHash() const { return m_parentName.CalcHash() ^ m_parentClass.CalcHash() ^ m_childName.CalcHash() ^ m_childClass.CalcHash(); }

	DECLARE_RTTI_STRUCT( SStreamedAttachment );
};

BEGIN_CLASS_RTTI( SStreamedAttachment );
	PROPERTY( m_parentName );
	PROPERTY( m_parentClass );
	PROPERTY( m_childName );
	PROPERTY( m_childClass );
	PROPERTY( m_data );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////

/// Slot init info
struct EntitySlotInitInfo
{
	CName				m_componentName;		//!< Animated component
	CName				m_boneName;				//!< Parent bone name
	EngineTransform		m_transform;			//!< Relative transform
	Bool				m_freePositionAxisX;	//!< Free position X
	Bool				m_freePositionAxisY;	//!< Free position Y
	Bool				m_freePositionAxisZ;	//!< Free position Z
	Bool				m_freeRotation;			//!< Do not lock rotation to the parent slot

	RED_INLINE EntitySlotInitInfo()
		: m_freePositionAxisX( false )
		, m_freePositionAxisY( false )
		, m_freePositionAxisZ( false )
		, m_freeRotation( false )
	{};
};

///////////////////////////////////////////////////////////

/// Entity slot declaration
class EntitySlot
{
	DECLARE_RTTI_STRUCT( EntitySlot );

protected:
	CName				m_name;					//!< Slot name
	CName				m_componentName;		//!< Animated component
	CName				m_boneName;				//!< Parent bone name
	EngineTransform		m_transform;			//!< Relative transform
	Bool				m_freePositionAxisX;	//!< Free position X
	Bool				m_freePositionAxisY;	//!< Free position Y
	Bool				m_freePositionAxisZ;	//!< Free position Z
	Bool				m_freeRotation;			//!< Do not lock rotation to the parent slot
	Bool				m_wasIncluded;			//!< Is this slot included from base templates ?

public:
	//! Get slot name
	RED_INLINE const CName& GetName() const { return m_name; }

	//! Get component name
	RED_INLINE const CName& GetComponentName() const { return m_componentName; }

	//! Get bone name
	RED_INLINE const CName& GetBoneName() const { return m_boneName; }

	//! Get transform
	RED_INLINE const EngineTransform& GetTransform() const { return m_transform; }

	//! Was the slot included from base templates ?
	RED_INLINE Bool WasIncluded() const { return m_wasIncluded; }

public:
	EntitySlot();
	EntitySlot( const CName& slotName, const EntitySlotInitInfo* initInfo );

	//! Set as included
	void SetIncluded();

	//! Set slot bone name
	void SetBoneName( const CName& boneName );

	//! Initialize from component
	bool InitializeFromComponent( CEntity* entity, CComponent* component );

	//! Resolve slot matrix
	bool CalcMatrix( CEntity const * entity, Matrix& outLocalToWorld, String* outError ) const;

	//! Attach specified component using this slot
	IAttachment* CreateAttachment( const HardAttachmentSpawnInfo& spawnInfo, CEntity* entity, CNode* nodeToAttach, String* outError ) const;

#ifndef NO_EDITOR
	//! Set slot transform in world space ( editor only )
	Bool SetTransformWorldSpace( CEntity* entity, const Vector* posWS, const EulerAngles* rotWS );
#endif
};

BEGIN_CLASS_RTTI( EntitySlot );
	PROPERTY( m_wasIncluded );
	PROPERTY_EDIT( m_name, TXT("Slot name") );
	PROPERTY_CUSTOM_EDIT( m_componentName, TXT("Animated component"), TXT("EntityComponentList") );
	PROPERTY_CUSTOM_EDIT( m_boneName, TXT("Parent bone name"), TXT("SlotBoneList") );
	PROPERTY_EDIT( m_transform, TXT("Relative slot transform") );
	PROPERTY_EDIT( m_freePositionAxisX, TXT("Free position axis X") );
	PROPERTY_EDIT( m_freePositionAxisY, TXT("Free position axis Y") );
	PROPERTY_EDIT( m_freePositionAxisZ, TXT("Free position axis Z") );
	PROPERTY_EDIT( m_freeRotation, TXT("Do not lock rotation to the parent slot") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

//! Entry in the list of instance properties for components
struct SComponentInstancePropertyEntry
{
	CName	m_component;			//!< The component's name
	CName	m_property;				//!< The property's name

	SComponentInstancePropertyEntry(){}
	SComponentInstancePropertyEntry( const SComponentInstancePropertyEntry& rhs )
		: m_component( rhs.m_component )
		, m_property( rhs.m_property )
	{}
	SComponentInstancePropertyEntry( const CName& component, const CName& prop )
		: m_component( component )
		, m_property( prop )
	{}

	bool operator == ( const SComponentInstancePropertyEntry& rhs ) const
	{
		return m_component == rhs.m_component && m_property == rhs.m_property;
	}

	bool operator != ( const SComponentInstancePropertyEntry& rhs ) const
	{
		return m_component != rhs.m_component || m_property != rhs.m_property;
	}

	DECLARE_RTTI_STRUCT( SComponentInstancePropertyEntry );
};

BEGIN_CLASS_RTTI( SComponentInstancePropertyEntry );
	PROPERTY( m_component );
	PROPERTY( m_property );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////

/// Entity template
class CEntityTemplate : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CEntityTemplate, CResource, "w2ent", "Entity template" );

	friend class CEntityTemplateCooker;
protected:
	typedef THashMap< CGUID, CObject* > TObjectsHashMap;

protected:
	CEntity*									m_entityObject;			// Entity object to instance
	CName										m_entityClass;			// Entity class
	TDynArray< CEntityBodyPart >				m_bodyParts;			// List of entity body parts
	TDynArray< CEntityAppearance >				m_appearances;			// List of entity appearance settings
	TDynArray< CName >							m_usedAppearances;		// List of appearances that may be used by instances of this template
	TDynArray< VoicetagAppearancePair >			m_voicetagAppearances;	// List of mapped voicetags for appearances
	TDynArray< CFXDefinition* >					m_effects;				// Effects defined at this entity template
	TDynArray< CEntityTemplateParam* >			m_templateParams;		// Additional parameters of the template that don't necessarily originate in the engine
	TDynArray< EntitySlot >						m_slots;				// Entity placement slots
	TDynArray< SEntityTemplateColoringEntry >	m_coloringEntries;		// Entity template coloring entries for mesh components
	TDynArray< SComponentInstancePropertyEntry>	m_instancePropEntries;	// Instance property entries for components

	// Cooking and preloading
	TDynArray< CEntityTemplateCookedEffect >	m_cookedEffects;		// Structures describing cooked effects from buffer
	CEntityTemplatePreloadedEffects*			m_effectsPreloaded;		// Preloaded effects created by loading jobs
	Uint32										m_cookedEffectsVersion;	// For backward compatibility

	static Uint32								s_cookedEffectsCurrentVersion;	// For backward compatibility

protected:

	TDynArray< THandle< CEntityTemplate > >		m_includes;				// List of included templates
	TDynArray< Uint8, MC_EntityTemplate >		m_data;					// Template data, this entity template only - specifically at this level.
	Vector										m_backgroundOffset;		// Background offset
	Bool										m_properOverrides;		// TEMPORARY UNTIL MASSIVE ENTITY RESAVE: m_overrides contains proper data
																		// If this is false, it will recalculate overrides on OnPostLoad
	CDateTime									m_dataCompilationTime;	// Time when the flat compiled data was made

	TDynArray< SEntityTemplateOverride >		m_overrides;			// Contains overridden properties from inherited components
	TDynArray< Uint8 >							m_flatCompiledData;		// Compiled data of this template with all the includes

protected:
	TDynArray< SStreamedAttachment >			m_streamedAttachments;	// Attachments with streamed components

public:

	// Get graph background offset
	RED_INLINE const Vector& GetBackgroundOffset() const { return m_backgroundOffset; }

	// Set graph background offset
	RED_INLINE void SetBackgroundOffset( const Vector& offset ) { m_backgroundOffset = offset; }

	//! Get included entity templates for modification
    RED_INLINE TDynArray< THandle< CEntityTemplate > >& GetIncludes() { return m_includes; }

	//! Get included entity templates
	RED_INLINE const TDynArray< THandle< CEntityTemplate > > & GetIncludes() const { return m_includes; }

	//! Get effect defined at this entity template
	RED_INLINE const TDynArray< CFXDefinition* >& GetEffects() const { return m_effects; }

	//! Get additional parameters of the entity template
	RED_INLINE const TDynArray< CEntityTemplateParam* >& GetTemplateParams() const { return m_templateParams; }

	//! Gets the name of entity class
	RED_INLINE const CName& GetEntityClassName() const { return m_entityClass; }

	//! Gets the slots defined in this entity template
	RED_INLINE const TDynArray< EntitySlot >& GetSlots() const { return m_slots; }

public:
	Bool FindComponentOverrideIndex( CComponent* component, Uint32& index ) const;

public:
	Bool HasOverridesForComponent( const CComponent* component ) const;
	void ResetPropertiesForComponent( CComponent* component, const TDynArray< CName >& propertiesToReset ) const;
	void RemoveOverrideForComponentProperty( CComponent* component, const CName& propertyName, Bool resetPropertyValue = true );
	void RemoveOverrideForComponentProperties( CComponent* component, const TDynArray< CName >& propertiesToRemove, Bool resetPropertyValues = true );
	void RemoveAllOverridesForComponent( CComponent* component, Bool resetPropertyValues = true  );
	void CreatePropertyOverride( CComponent* component, const CName& propertyName );
	void CreatePropertyOverridesForComponent( CComponent* includedComponent, CComponent* localComponent );
	void GetOverridenPropertiesForComponent( CComponent* component, TDynArray< CName >& propertyNames );
	Int32 GetIncludeHierarchyLevelForOverridenComponent( CComponent* component ) const; // returns -1 if no overrides

private:
	void RebuildPropertyOverridesFromIncludes( CEntity* sourceEntity, Bool addExternalFlags );

public:
	CEntityTemplate();
	~CEntityTemplate();

	// Serialization
	virtual void OnSerialize( IFile& file );

	// OnFinalize
	virtual void OnFinalize();

	// Object was loaded
	virtual void OnPostLoad();

	// Is this template detachable ? (does not contain any data except components)
	Bool IsDetachable() const;

#ifndef NO_EDITOR
	// Removes all components, streaming data, etc from the template effectively "resetting" it
	void ResetEntityTemplateData();

	// Update the streamed attachments using the streamed components in the given entity
	void UpdateStreamedAttachmentsFromEntity( CEntity* entity, const TDynArray< CComponent* >& streamedComponents );

	// Set entity class
	void SetEntityClass( CClass* entityClass );

	// Property changed
	virtual void OnPropertyPostChange( IProperty* property );

#endif

	// Create the streamed attachments inside the given entity for the given new components
	void CreateStreamedAttachmentsInEntity( CEntity* entity, const TDynArray< CComponent* >& newComponents, TDynArray< IAttachment* >& createdAttachments ) const;

#ifndef NO_RESOURCE_COOKING
	// Resource cooking
	virtual void OnCook( class ICookerFramework& cooker ) override;

	// Capture data
	void CaptureData( CEntity* source, Bool keepStreamedComponents = false, Bool emitModifiedEvent = true );
#endif

	// Scripts were reloaded
	virtual void OnScriptReloaded();

#ifndef NO_EDITOR
	// Resource was pasted from clipboard
	virtual void OnPaste( Bool wasCopied );
#endif
	// (Re-)Create full entity data buffer - a serialized entity with all included processed
	// The owner is needed for components and attachments that depend on being initialized on some
	// layer owner... however if this is set to null, the entity template itself will be used as the owner
	// (but this might crash some components/attachments)
	Bool CreateFullDataBuffer( CObject* owner, const EntityTemplateInstancingInfo& instanceInfo, SSavedAttachments* extraAttachments );

	// (Re-)Create full entity data buffer from specified entity
	// Overrides existing data
	Bool CreateFullDataBufferFromEntity( CEntity* entity );

#ifndef NO_DATA_VALIDATION
	virtual void OnCheckDataErrors() const override;
	virtual void OnFullValidation( const String& additionalContext ) const override;
#endif

protected:

#ifndef NO_RESOURCE_COOKING
	// Create compiled data buffer for object instancing
	void CompileDataBuffer();

	// Cook entity data for given platform
	void CookDataBuffer( class ICookerFramework& cookerFramework, ECookingPlatform platform );
#endif	

	// Create raw entity not using the cache
	CEntity* CreateEntityUncached( CObject* owner, const EntityTemplateInstancingInfo& instanceInfo, SSavedAttachments* savedAttachments, SSavedAttachments* extraAttachments, Bool destroyBrokenAttachments ) const;

private:
	// Saves object(s) via the supplied saving context, and store the object back in the destination data buffer.
	Bool SaveToMemoryDataArray( DependencySavingContext& savingContext, TDynArray< Uint8, MC_EntityTemplate >* dstBuffer );

	// Load an entity from a data src array.
	Bool LoadEntityFromMemoryDataArray( const TDynArray< Uint8 >& srcDataBuffer, SDependencyLoaderCreateEntityContext& loadingContext, CEntity** loadedEntity ) const;

public:

	// Get priority
	virtual ResourceReloadPriority GetReloadPriority() { return 30; }

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	// Reload resource
	virtual Bool Reload(Bool confirm);
#endif

/************************************************************************/
/* Entity Creation                                                      */
/************************************************************************/
public:
	// Create instance of this template
	virtual CEntity* CreateInstance( CLayer* parentLayer, const EntityTemplateInstancingInfo& instanceInfo ) const;

	// Create instance of this template
	virtual CObject* CreateTemplateInstance( CObject* parent ) const;

	// Get template default instance
	virtual const CObject* GetTemplateInstance() const;

	// Returns true if we should embed this resource while cooking
	virtual Bool ShouldEmbedResource() const;

	// HACK: Required by for CreateInstance() optimizations to work.
	CEntity* GetEntityObject() const	{ return m_entityObject; }

	//! Check if this template is based on given entity template
	Bool IsBasedOnTemplate( const CEntityTemplate* entityTemplate ) const;

#ifndef NO_EDITOR
	//! Returns information about the entity (note: it can be slow)
	virtual void GetAdditionalInfo( TDynArray< String >& info ) const;
#endif

private:
	CEntity* CreatePreviewWindowEntityInstance( CLayer* parentLayer, const EntityTemplateInstancingInfo& instanceInfo ) const;
	CEntity* CreateEntityInstanceFromCompiledData( CLayer* parentLayer, const EntityTemplateInstancingInfo& instanceInfo ) const;

/************************************************************************/
/* Appearances                                                          */
/************************************************************************/
public: // Local appearances access (used mainly by editor and internally by this class)

	//! Add appearance to this entity template
	Bool AddAppearance( const CEntityAppearance& appearance );

	//! Remove appearance from this entity template
	void RemoveAppearance( const CEntityAppearance& appearance );

	//! Get voicetag for given appearance
	CName GetApperanceVoicetag( const CName& appearanceName ) const;

	//! Refreshes voicetag-appearances table, resets appearances that are no longer used
	void RefreshVoicetagAppearances();

	//! Get the list of voicetags used in appearances
	RED_INLINE const TDynArray< VoicetagAppearancePair >& GetVoicetagAppearances() const { return m_voicetagAppearances; }

	//! Add voicetags used in appearances
	RED_INLINE void AddVoicetagsAppearances( TDynArray< VoicetagAppearancePair >& voicetagsAppearances ) { m_voicetagAppearances.PushBack( voicetagsAppearances ); }

	//! Remove voicetags used in appearances
	RED_INLINE void RemoveVoicetagsAppearances( TDynArray< VoicetagAppearancePair >& voicetagsAppearances ) { for ( auto& voicetag : voicetagsAppearances ) { m_voicetagAppearances.Remove( voicetag );	} }

	//! Get entity appearance defined in this template (not in includes) by index
	RED_INLINE CEntityAppearance& GetAppearance( Uint32 index ) { return m_appearances[index]; }

	//! Get list of appearances
	RED_INLINE const TDynArray< CEntityAppearance > &GetAppearances() const { return m_appearances; }

public: // Enabled appearances access (used by external systems)

	// Check if given appearance is enabled
	RED_INLINE Bool IsAppearanceEnabled( const CName &name ) const { return m_usedAppearances.Exist( name ); }

	//! Get used appearances names (filter for enabled appearances)
	const TDynArray<CName> & GetEnabledAppearancesNames() const { return m_usedAppearances; }

	//! Get used appearances names (filter for enabled appearances)
	TDynArray<CName> & GetEnabledAppearancesNames() { return m_usedAppearances; }

	//! Get all appearances in the entity template
	RED_INLINE void GetAllEnabledAppearances( TDynArray< const CEntityAppearance* > & appearances ) const { GetAllAppearances( appearances, &m_usedAppearances ); }

public: // Local and included appearances access (used by external systems)

	//! Get entity appearance by name
	const CEntityAppearance	* GetAppearance( const CName &name, Bool recursive = true ) const;

	//! Get all appearances in the entity template
	void GetAllAppearances( TDynArray< const CEntityAppearance* > & appearances, const TDynArray< CName > * filter = NULL ) const;

	//! Get all appearances in the entity template
	void GetAllAppearances( TDynArray< CEntityAppearance* > & appearances, const TDynArray< CName > * filter = NULL );

	Bool ExportAppearance( const CName &name, CEntityAppearance& appearance ) const;

	Bool ImportAppearance( const CName &name, const CEntityAppearance& appearance );

/************************************************************************/
/* Slots																*/
/************************************************************************/

public:
	//! Find template slot by name
	const EntitySlot* FindSlotByName( const CName& slotName, Bool recursive ) const;	

	//! Add slot to entity
	bool AddSlot( const CName& name, const EntitySlotInitInfo* initInfo = NULL, Bool markAsModifie = true );

	//! Add slot to entity
	void AddSlot( const CName& name, const EntitySlot& entitySlot, Bool markAsModifie = true );

	//! Remove slot from entity
	bool RemoveSlot( const CName& name, Bool markAsModifie = true );

	//! Collect all slots
	void CollectSlots( TDynArray< const EntitySlot* >& slots, Bool recursive ) const;

#ifndef NO_EDITOR
	//! Set slot transform in world space, only for slots in this template ( editor only )
	Bool SetSlotTransform( CEntity* entity, const CName& name, const Vector* posWS, const EulerAngles* rotWS );
#endif

/************************************************************************/
/* Coloring Entries                                                     */
/************************************************************************/
public:
	//! Add or update coloring entry
	void AddColoringEntry( const SEntityTemplateColoringEntry& entry );

	//! Add or update coloring entry without an appearance
	RED_INLINE void AddColoringEntry( const CName& componentName, const CColorShift& colorShift1, const CColorShift& colorShift2 )
	{
		AddColoringEntry( SEntityTemplateColoringEntry( CName::NONE, componentName, colorShift1, colorShift2 ) );
	}

	//! Add or update coloring entry with an appearance (or no appearance if CName::NONE is used)
	RED_INLINE void AddColoringEntry( const CName& appearance, const CName& componentName, const CColorShift& colorShift1, const CColorShift& colorShift2 )
	{
		AddColoringEntry( SEntityTemplateColoringEntry( appearance, componentName, colorShift1, colorShift2 ) );
	}

	//! Remove the coloring entry with the given appearance and component name
	void RemoveColoringEntry( const CName& appearance, const CName& componentName );
	
	//! Remove the coloring entry with the given component and no appearance
	RED_INLINE void RemoveColoringEntry( const CName& componentName )
	{
		RemoveColoringEntry( CName::NONE, componentName );
	}

	//! Remove all coloring entries
	void RemoveAllColoringEntries();

	//! Update the color shift of the coloring entry with the given component name and appearance, if it exists
	void UpdateColoringEntry( const CName& appearance, const CName& componentName, const CColorShift& colorShift1, const CColorShift& colorShift2 );

	//! Update the color shift of the coloring entry with the given component name and no appearance, if it exists
	RED_INLINE void UpdateColoringEntry( const CName& componentName, const CColorShift& colorShift1, const CColorShift& colorShift2 )
	{
		UpdateColoringEntry( CName::NONE, componentName, colorShift1, colorShift2 );
	}

	//! Find the coloring entry for the given appearance and component name, returns false if no such entry is found
	Bool FindColoringEntry( const CName& appearance, const CName& componentName, CColorShift& colorShift, CColorShift& colorShift2 ) const;

	//! Find the coloring entry for the component name and no appearance, returns false if no such entry is found
	RED_INLINE Bool FindColoringEntry( const CName& componentName, CColorShift& colorShift1, CColorShift& colorShift2 ) const
	{
		return FindColoringEntry( CName::NONE, componentName, colorShift1, colorShift2 );
	}

	//! Return all coloring entries in this entity template
	RED_INLINE const TDynArray<SEntityTemplateColoringEntry>& GetAllColoringEntries() const
	{
		return m_coloringEntries;
	}

	//! Returns an array with all coloring entries for the given appearance
	void GetColoringEntriesForAppearance( const CName& appearance, TDynArray<SEntityTemplateColoringEntry>& entries ) const;

	//! Returns an array with all coloring entries without an appearance
	RED_INLINE void GetColoringEntriesWithoutAppearance( TDynArray<SEntityTemplateColoringEntry>& entries ) const
	{
		GetColoringEntriesForAppearance( CName::NONE, entries );
	}

/************************************************************************/
/* Instance Property Entries                                            */
/************************************************************************/
public:
	//! Clears all local instance property entries
	void ClearInstancePropertyEntries();

	//! Adds an instance property entry for the given component and property (if one doesn't already exists)
	void AddInstancePropertyEntry( const CName& componentName, const CName& propertyName );

	//! Removes the instance property entry for the given component and property
	void RemoveInstancePropertyEntry( const CName& componentName, const CName& propertyName );

	//! Returns the instance property entries for the given component
	void GetInstancePropertiesForComponent( const CName& componentName, TDynArray< CName >& propertyNames ) const;

	//! Returns all instance property entries stored in this entity template and its includes
	void GetAllInstanceProperties( TDynArray<SComponentInstancePropertyEntry>& entries ) const;

	//! Returns the instance property entries stored locally in this entity template
	RED_INLINE const TDynArray<SComponentInstancePropertyEntry>& GetLocalInstanceProperties() const { return m_instancePropEntries; }

/************************************************************************/
/* Effects                                                              */
/************************************************************************/
public:
	//! Add effect to entity template
	CFXDefinition* AddEffect( const String& effectName );

	//! Add effect to entity template
	Bool AddEffect( CFXDefinition* effect );

	//! Remove effect from entity template
	Bool RemoveEffect( CFXDefinition* effect );

	//! Find effect by name
	CFXDefinition* FindEffect( const CName& effectName, Bool recursive = true );

	//! Find effect for animation
	CFXDefinition* FindEffectForAnimation( const CName& animationName, Bool recursive = true );

	//! Find effect by name
	Bool HasEffect( const CName& effectName, Bool recursive = true ) const;

	//! Find effect for animation
	Bool HasEffectForAnimation( const CName& animationName, Bool recursive = true ) const;

	//! Get all effects from this template and includes
	void GetAllEffects( TDynArray< CFXDefinition* >& allEffects );

	//! Get all effects from this template and includes
	void GetAllEffectsNames( TDynArray< CName >& allEffects ) const;

	//! Preload all effects
	void PreloadAllEffects();

	//! Preload effect
	Bool PreloadEffect( const CName& effect );

	//! Preload effect for animation
	Bool PreloadEffectForAnimation( const CName& animName );

/************************************************************************/
/* Parameters                                                           */
/************************************************************************/
public:
	//! Adds a new template parameter
	Bool AddParameter( CEntityTemplateParam* param );

	//! Adds a new template parameter with check is unique type
	Bool AddParameterUnique( CEntityTemplateParam* param );

	//! Removes new template parameter
	Bool RemoveParameter( CEntityTemplateParam* param );

	//! Get all appearances in the entity template
	template< class T >
	void GetAllParameters( TDynArray< T* >& params, Bool exploreTemplateList = true ) const;

	//! Find parameter by type
	template< class T >
	T* FindParameter( Bool recursive = true, Bool exploreTemplateList = true ) const;

	//! Functions for collecting params derived from CGameplayEntityParam with custom overriding 
	void CollectGameplayParams( TDynArray< CGameplayEntityParam* >& params, CClass* ofClass, Bool exploreTemplateList = true, Bool recursive = true ) const;
	CGameplayEntityParam * FindGameplayParam( CClass* ofClass, Bool checkIncludes = false, Bool exploreTemplateList = true ) const;
	template< class T > RED_INLINE T* FindGameplayParamT( Bool checkIncludes = false ) const { CGameplayEntityParam* param = FindGameplayParam( T::GetStaticClass(), checkIncludes ); return param ? static_cast< T* > ( param ) : NULL; } 

	//! Find parameter by type and predicate
	template< class T, class Pred >
	T* FindParameter( Bool recursive, const Pred &pred, Bool exploreTemplateList = true ) const;

public:
	static void IncludeEntityTemplate( CObject* layer, CEntity* entity, CEntityTemplate* includedEntityTemplate, const CEntityTemplate* localEntityTemplate, TDynArray< THandle< CResource > > & collected, TDynArray< CComponent* >& addedComponents, struct SSavedAttachments* savedAttachments, THashSet< CName >* foundOverrides, bool fullInclude = true, bool fullAttach = true );
	static void IncludeComponents( CObject* layer, CEntity* entity, const Uint8* componentsData, Uint32 dataSize, TDynArray< CComponent* >& addedComponents, struct SSavedAttachments* savedAttachments, Bool includedFromTemplate );

private:
	static void ProcessIncludedComponent( CComponent* component, const SEntityTemplateOverride* componentOverride, CEntity* entity, TDynArray< CComponent* > &addedComponents, struct SSavedAttachments* savedAttachments, bool properOverrides, bool fullAttach = true, bool includedFromTemplate = true );
	
	//! During IncludeEntityTemplate(), attachments between the included components can be broken. These functions are used to help restore them after inclusion is done.
	static void IncludeTemplateSaveAttachments( const TDynArray< CComponent* >& components, SSavedAttachments& savedAttachments );
	static void IncludeTemplateRestoreBrokenAttachments( const TDynArray< CComponent* >& addedComponents, CEntity* entity, TDynArray< struct SSavedAttachment >& savedAttachments );
};

BEGIN_CLASS_RTTI( CEntityTemplate );
	PARENT_CLASS( CResource );
	PROPERTY( m_includes );
	PROPERTY( m_overrides );
	PROPERTY( m_properOverrides );
	PROPERTY_NOT_COOKED( m_backgroundOffset );
	PROPERTY_NOT_COOKED( m_dataCompilationTime );
	PROPERTY( m_entityClass );
	PROPERTY( m_entityObject );
	PROPERTY( m_bodyParts );
	PROPERTY( m_appearances );
	PROPERTY( m_usedAppearances );
	PROPERTY( m_voicetagAppearances );
	PROPERTY( m_effects );
	PROPERTY( m_slots );
	PROPERTY( m_templateParams );
	PROPERTY( m_coloringEntries );
	PROPERTY_NOT_COOKED( m_instancePropEntries );
	PROPERTY( m_flatCompiledData );
	PROPERTY( m_streamedAttachments );
	PROPERTY( m_cookedEffects );
	PROPERTY( m_cookedEffectsVersion );
END_CLASS_RTTI();

#include "entityTemplate.inl"
