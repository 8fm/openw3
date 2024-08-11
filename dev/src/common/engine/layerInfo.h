/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "layerStorage.h"
#include "../core/events.h"

// Forward class declarations
class CLayer;
class CLayerGroup;
class CFileLoadingStats;

///////////////////////////////////////////////////////////////////////////////////////////////

/// Layer loading context
class LayerLoadingContext
{
public:
	CFileLoadingStats*		m_stats;		//!< Stats to gather during loading
	Bool					m_queueEvent;	//!< Queue layer event after loading
	Bool					m_loadHidden;	//!< Load layer even if it is hidden

public:
	LayerLoadingContext();
};

///////////////////////////////////////////////////////////////////////////////////////////////

/// Layer type
enum ELayerType
{
	LT_AutoStatic,
	LT_NonStatic
};

BEGIN_ENUM_RTTI( ELayerType );
	ENUM_OPTION( LT_AutoStatic );
	ENUM_OPTION( LT_NonStatic );
END_ENUM_RTTI();

///////////////////////////////////////////////////////////////////////////////////////////////

/// Automatic processing for layer
enum ELayerMergedContent
{
	LMC_Auto,
	LMC_ForceAlways,
	LMC_ForceNever,
};

BEGIN_ENUM_RTTI( ELayerMergedContent );
	ENUM_OPTION( LMC_Auto );
	ENUM_OPTION( LMC_ForceAlways );
	ENUM_OPTION( LMC_ForceNever );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////
// Layer Build Tags - This allows us to tag the layerinfo object
// with some information which can be used to help aid building streaming
// data, and general filtering.
//
// The current enum is by no means complete - it should be added to, and
// expanded as required.
//////////////////////////////////////////////////////////////////////////
enum ELayerBuildTag
{
	LBT_None,				// None has been set - this means the layer will be ignored.
	LBT_Ignored,			// The layer should be ignored.
	LBT_EnvOutdoor,			// The layer is for an outdoor environments.
	LBT_EnvIndoor,			// The layer is for indoor environments.
	LBT_EnvUnderground,		// The layer is for under the terrain. (Dungeons, Sewers etc...)
	LBT_Quest,				// The layer is a quest layer.
	LBT_Communities,		// The layer is a communities layer.
	LBT_Audio,				// The is used for audio.
	LBT_Nav,				// This is used for navigation layers.
	LBT_Gameplay,			// The layer can store persistent entities
	LBT_DLC,				// DLC layer
	LBT_Max					// Counter for the size of the enum.
};

BEGIN_ENUM_RTTI( ELayerBuildTag );
	ENUM_OPTION( LBT_None );			    
	ENUM_OPTION( LBT_Ignored );
	ENUM_OPTION( LBT_EnvOutdoor );
	ENUM_OPTION( LBT_EnvIndoor );
	ENUM_OPTION( LBT_EnvUnderground );
	ENUM_OPTION( LBT_Quest );		
	ENUM_OPTION( LBT_Communities );
	ENUM_OPTION( LBT_Audio );
	ENUM_OPTION( LBT_Nav );
	ENUM_OPTION( LBT_Gameplay );
	ENUM_OPTION( LBT_DLC );
END_ENUM_RTTI();

#ifndef NO_EDITOR
namespace LayerBuildTagColors
{
	extern Color None;
	extern Color Ignored;
	extern Color EnvOutdoor;
	extern Color EnvIndoor;
	extern Color Quest;
	extern Color Communities;
	extern Color EnvUnderground;
	extern Color Audio;
	extern Color Nav;
	const Color& GetColorFor( ELayerBuildTag tag );
};
#endif
///////////////////////////////////////////////////////////////////////////////////////////////


/// Info about layer in world
class CLayerInfo
:	public ISerializable
#ifndef NO_EDITOR_EVENT_SYSTEM
,	public IEdEventListener
#endif
{
	DECLARE_RTTI_SIMPLE_CLASS_WITH_ALLOCATOR( CLayerInfo, MC_LayerSystem );

public:
	//! Should this layer be saved?
	RED_INLINE Bool ShouldSave() const { return m_shouldSave; }

	//! Performs recursive check as to whether this savegame shall be in savegame;
	Bool CheckShouldSave();

	// Called before object's property is changed in the editor
	virtual void OnPropertyPreChange( IProperty* property ) override;

	// Called after object's property is changed in the editor
	virtual void OnPropertyPostChange( IProperty* property ) override;

#ifndef NO_DEBUG_PAGES
	//! Sets an error flag
	RED_INLINE void SetErrorFlag( Bool val ) { m_debugErrorFlag = val; }

	//! Checks the error flag
	RED_INLINE Bool GetErrorFlag() const { return m_debugErrorFlag; }
#endif 

	//! Get layer short name
	RED_INLINE const String& GetShortName() const { return m_shortName; }

	//! Get tags
	RED_INLINE const TagList& GetTags() const { return m_tags; }
	RED_INLINE void SetTags( const TagList& tagsToSet ) { m_tags = tagsToSet; }

	//! Get the layer file path
	RED_INLINE const String& GetDepotPath() const { return m_depotFilePath; }

	//! Get world this layer is in
	RED_INLINE CWorld* GetWorld() const { return m_world.Get(); }

	//! Get the layer group this layer is in
	RED_INLINE CLayerGroup* GetLayerGroup() const { ASSERT( m_layerGroup ); return m_layerGroup; }

	//! Get the layer ( if loaded )
	RED_INLINE CLayer* GetLayer() const { return m_layer.Get(); }

	//! Is the layer loaded
	RED_INLINE Bool IsLoaded() const { return m_layer.IsValid(); }

	//! Is the layer being loaded
	RED_INLINE Bool IsLoading() const { return m_loadingToken != NULL; }

	//! Is the layer pending destroy ?
	RED_INLINE Bool IsPendingDestroy() const { return m_layerToDestroy != NULL; }

	//! Is the layer begin unloaded ?
	RED_INLINE Bool IsUnloading() const { return m_requestUnload; }

	//! Is this an environement layer
	RED_INLINE Bool IsEnvironment() const { return (m_layerBuildTag == LBT_EnvIndoor) || (m_layerBuildTag == LBT_EnvOutdoor) || (m_layerBuildTag == LBT_EnvUnderground); }

	//! Returns the type of the layer
	RED_INLINE ELayerType GetLayerType() const { return m_layerType; }

	//! Is the layer visible ( attached )
	RED_INLINE Bool IsVisible() const { return m_isVisible; }

	//! Get layer internal GUID
	RED_INLINE const CGUID& GetGUID() const { return m_guid; }

	//! Get gameplay data storage
	RED_INLINE CLayerStorage* GetLayerStorage() { return &m_storage; }

	//! Get gameplay data storage
	RED_INLINE const CLayerStorage* GetLayerStorage() const { return &m_storage; }

	//! Set layer to destroy (temporary hack)
	RED_INLINE void SetLayerToDestroy( CLayer* layer ) { m_layerToDestroy = layer; }

	//! Get current layer build tag
	RED_INLINE ELayerBuildTag GetLayerBuildTag() const { return m_layerBuildTag; }

	//! Get layer content processing mode
	RED_INLINE ELayerMergedContent GetMergedContentMode() const { return m_layerMergeContentMode; }
	RED_INLINE void SetMergedContentMode( ELayerMergedContent val ) { m_layerMergeContentMode = val; }

	RED_INLINE Bool IsVisibilityForced() const { return m_forceVisible; }

	//! Returns true if this is a DLC layer
	RED_INLINE Bool IsDLC() const { return m_layerBuildTag == LBT_DLC; }

public:
	CLayerInfo();
	CLayerInfo( CWorld* world, CLayerGroup* layerGroup, const String& depotFilePath );
	virtual ~CLayerInfo();

	//! Object serialization
	virtual void OnPreSave();
	virtual void OnSerialize( IFile& file );

public:
	// Called to initialize loaded layer info
	void Init( CWorld* world, CLayerGroup* layerGroup, const String &depotFilePath );

	// Called to initialize loaded layer info when no world present (hack)
	void InitNoWorld( const String &depotFilePath );

	// Load layer synchronously, editor only
	Bool SyncLoad( LayerLoadingContext& loadingContext );

	// Unload layer synchronously
	Bool SyncUnload();

	// Request that this layer group should be loaded
	Bool RequestLoad( class CGenericCountedFence* loadingFence = nullptr );

	// Request that this layer group should be unloaded
	Bool RequestUnload( Bool purgeLayerStorage = false );

	// Finish async loading if it's happending, does nothing if layer is not loading
	void FinishAsyncLoading();

#ifndef NO_EDITOR
	// Load layer synchronously, unattached to the world, cook only
	Bool SyncLoadUnattached();

	// Unload layer synchronously, unattached to the world, cook only
	Bool SyncUnloadUnattached();
#endif

	// Allows for the visibility flag to be ignored, and for the layer to be treated as visible.
	void ForceVisible( Bool visible );

	// Toggle layer visibility
	void Show( Bool visible );

	//! Get the layer hierarchy path (if 'omitRoot' is true, then the preceding word "World" is cut out)
	void GetHierarchyPath( String &path, Bool omitRoot = false ) const;

	// Determine merged visibility - layer flag & layer group flags
	Bool GetMergedVisiblityFlag() const;

	// Mark if layer is used for preview
	void MarkAsUsedForPreview( Bool flag );

	// Mark as modified
	Bool MarkModified();

	// Pull layer groups and layer infos into the CWorld resource (makes the loading faster and saves the layer enumeration step)
	void PullLayerGroupsAndLayerInfo();

	// Change the layer build tag
	void SetLayerBuildTag( const ELayerBuildTag& val );

	// Change the layer type
	void SetLayerType( const ELayerType& newType );

	// Set streaming layer
	void SetStreamingLayer( Bool val );

	// Is streaming layer?
	Bool IsStreamingLayer() const;

	void UpdatePath();

public:
	//! Save layer state to save game
	void SaveState( IGameSaver* saver );

	//! Load layer state from save game
	void LoadState( IGameLoader* loader );

#ifndef NO_EDITOR_WORLD_SUPPORT

public:
	// Called to initialize empty layer
	Bool Create();

	// Called to initialize layer info with a given layer
	Bool Create( CLayer *layer );

	// Called to create and initialize a basic empty layer with layer info in memory.
	void CreateBasic();

	// Get latest version from repository
	void GetLatest();

	// Reload if necessary
	void Refresh( Bool confirm = true );

	// Reload
	void Reload();

	// Save layer to disk file
	Bool Save();

	// Remove this layer from world
	Bool Remove( Bool confirm=true );

	// Rename this layer
	Bool Rename( const String& newName );

	// Prepare for play in editor
	void CreateLayerCopyForPIE();

	// Restore layer after PIE - unload part
	void RestoreAfterPIEUnload();

	// Restore layer after PIE - load part
	void RestoreAfterPIELoad();

	// Append layer info object to layer file that is bound with this layer info
	Bool AppendLayerInfoObject();

	// Append layer info to the specified file
	Bool AppendLayerInfoObject( IFile& file );

	// Append layer info to the specified path
	Bool AppendLayerInfoObject( const String& fileAbsolutePath );

	// Calculate layer bounding box as a bounding box of it's entities
	void CalculateBoundingBox( Box& box );

#endif

public:
	Uint32 CalcObjectDynamicDataSize() const;


protected:
	// Update attachment state
	void ConditionalAttachToWorld();

	// Upload internal state loading
	Bool UpdateLoadingState( Bool sync = false );

	// Link this layer group and below to the world, needed for cooked builds because m_world is not serialized 
	void LinkToWorld( CWorld* world );

	// Finish the loading job
	void FinishLoadingJob();

protected:
#ifndef NO_EDITOR_EVENT_SYSTEM
	// Editor global event
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );
#endif

	THandle< CWorld >			m_world;					//!< World we are owned by
	THandle< CLayer >			m_layer;					//!< Loaded layer, valid only if layer is truly loaded

	CLayer*						m_layerToDestroy;			//!< Destroy when all components are detached
	CLayerGroup*				m_layerGroup;				//!< Parent layer group
	CJobLoadResource*			m_loadingToken;				//!< Async loading token

	ELayerBuildTag				m_layerBuildTag;			//!< Tells us what the layer is being used for. Quest/Environment etc...
	ELayerType					m_layerType;				//!< Type of the layer (can control, among others, isStatic's state)
	ELayerMergedContent			m_layerMergeContentMode;	//!< How to process merged content for this layer
	Bool						m_streamingLayer;			//!< Used to identify if the layer was created during the build step to be a specific streaming layer.

	THandle< CLayer >			m_editorCopy;				//!< Local layer copy in editor, cached during PIE
	TagList						m_tags;						//!< Tags of this layer
	CGUID						m_guid;						//!< GUID
	String						m_depotFilePath;			//!< Path to layer file
	String						m_shortName;				//!< Layer short name, derived from file name
	CLayerStorage				m_storage;					//!< Gameplay data storage
	Bool						m_hasEmbeddedLayerInfo;		//!< The group has the inner groups and the layer infos pulled (cooked)

	// runtime flags
	Bool						m_shouldSave:1;				//!< Layer should be saved in the SAVE GAME
	Bool						m_requestUnload:1;			//!< Unload layer
	Bool						m_isVisible:1;				//!< True if layer is visible
	Bool						m_forceVisible:1;			//!< Ignore the m_isVisible flag - this is used when we do drop in drop out play in the editor.
#ifndef NO_DEBUG_PAGES
	Bool						m_debugErrorFlag:1;
#endif

	friend class CWorld;
	friend class CLayerGroup;
};

BEGIN_CLASS_RTTI( CLayerInfo );
	PARENT_CLASS( ISerializable );
	PROPERTY_EDIT( m_tags, TXT("Tags") ); 
	PROPERTY_EDIT( m_layerType, TXT("Type of this layer (use LT_NonStatic if the layer is to be hidden/modified at runtime)") );
	PROPERTY_EDIT( m_layerBuildTag, TXT("Tag for layer usage, used specifically for cooking optimizations. (Environments, Quest etc...)"));
	PROPERTY_EDIT( m_layerMergeContentMode, TXT("How to process merged content for this layer" ) );
	PROPERTY( m_streamingLayer );
	PROPERTY( m_depotFilePath );
	PROPERTY( m_shortName );
	PROPERTY( m_guid );
	PROPERTY( m_hasEmbeddedLayerInfo );
END_CLASS_RTTI();
