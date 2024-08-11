/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "layer.h"
#include "../core/serializable.h"

// Forward class declarations
class WorldLoadingContext;
class LayerGroupLoadingContext;
class IGameSaver;
class IGameLoader;
class CWorldLayersVisibilityManagementTask;
class LayerLoadingContext;

struct SLayerGroupMemorySnapshot
{
	String	m_name;
	Bool	m_loaded;

	Uint32	m_memStatic;
	Uint32	m_memSerialize;
	Uint32	m_memDynamic;

	Uint32	m_entitiesNum;
	Uint32	m_attachedCompNum;

	TDynArray< SLayerGroupMemorySnapshot, MC_Debug >	m_subGroupsSnapshot;
	TDynArray< SLayerMemorySnapshot, MC_Debug >			m_layersSnapshot;

	SLayerGroupMemorySnapshot() : m_loaded( false ), m_memStatic( 0 ), m_memSerialize( 0 ), m_memDynamic( 0 ), m_entitiesNum( 0 ), m_attachedCompNum( 0 ) {}
};

/// Group of layers
class CLayerGroup : public ISerializable
{	
	DECLARE_RTTI_SIMPLE_CLASS_WITH_ALLOCATOR( CLayerGroup, MC_LayerSystem );

	friend class CWorld;

public:
	typedef TDynArray< CLayerInfo*, MC_LayerSystem >	TLayerList;
	typedef TDynArray< CLayerGroup*, MC_LayerSystem >	TGroupList;

protected:
	TLayerList						m_layers;					//!< Layers in this group
	TGroupList						m_subGroups;				//!< Internal groups

	CWorld*							m_world;					//!< Owner
	CLayerGroup*					m_parentGroup;				//!< Parent group
	String							m_name;						//!< Name of the layer group
	String							m_depotPath;				//!< Depot path to layer group directory
	String							m_absolutePath;				//!< Absolute path to layer group directory

	Bool							m_isVisible;				//!< Is this layer group visible
	Bool							m_isVisibleOnStart;			//!< Is this layer group visible on start
	Bool							m_oldSystemGroupFlag;		//!< Old system group flag, to be removed
	Bool							m_hasEmbeddedLayerInfos;	//!< The group has the inner groups and the layer infos pulled (cooked)
	Bool							m_shouldSave;
	Bool							m_isDLC;					//!< DLC group

	Uint64							m_idHash;

	void ComputeIDHash();

public:

#ifndef NO_DEBUG_PAGES
	//! Checks if the debug error flag is set on any of the children
	Bool GetErrorFlag() const;
#endif

	//! Should this layer be saved?
	RED_INLINE Bool ShouldSave() const { return m_shouldSave; }

	//! Performs recursive check as to whether this savegame shall be in savegame;
	Bool CheckShouldSave();

	//! Get owner
	RED_INLINE CWorld* GetWorld() const { return m_world; }

	//! Get layer group name
	RED_INLINE const String& GetName() const { return m_name; }

	//! Get layer group owner ( NULL for world group )
	RED_INLINE CLayerGroup* GetParentGroup() const { return m_parentGroup; }

	//! Get sub groups
	RED_INLINE const TGroupList& GetSubGroups() const { return m_subGroups; }

	//! Get layers
	RED_INLINE const TLayerList& GetLayers() const { return m_layers; }

	// Get absolute path
	RED_INLINE const String& GetAbsolutePath() const { return m_absolutePath; }

	//! Get depot path
	RED_INLINE const String& GetDepotPath() const { return m_depotPath; }

	// Is the group visible
	RED_INLINE Bool IsVisible() const { return m_isVisible; }

	// Is the group visible on start
	RED_INLINE Bool IsVisibleOnStart() const { return m_isVisibleOnStart; }

	// State of old "system group" flag (to be removed)
	RED_INLINE Bool IsOldSystemGroupFlagSet() const { return m_oldSystemGroupFlag; }

	// Is this a root group (world group)
	RED_INLINE Bool IsRootGroup() const { return (nullptr == m_parentGroup); }

	// Is this a DLC group?
	RED_INLINE Bool IsDLC() const { return m_isDLC; }

public:
	CLayerGroup(); // do not use directly
	CLayerGroup( CWorld* world, CLayerGroup* parentGroup, const String& name, const String& depotFilePath, const String& absoluteFilePath );
	virtual ~CLayerGroup();

	// Object serialization
	virtual void OnSerialize( IFile& file );

#ifndef NO_DATA_VALIDATION
	virtual void OnCheckDataErrors() const;
#endif

	virtual Uint32 CalcObjectDynamicDataSize() const;

public:
	//! Is this group loaded
	Bool IsLoaded( Bool recursive = false ) const;

    //! Is this group fully unloaded
    Bool IsFullyUnloaded() const;

	//! Is this group being loaded 
	Bool IsLoading() const;

	//! Is this group or any group below is being loaded ?
	Bool IsLoadingAnyGroup() const;

	//! Is this group being unloaded ?
	Bool IsUnloading() const;

	//! Is this a system group (recursive check)
	virtual Bool IsSystemGroup() const;

	//! Hashed id of given LayerGroup
	Uint64 GetLayerGroupId() const { return m_idHash; }
	
public:
	//! Load layer groups ( sync, slow, editor only )
	Bool SyncLoad( LayerLoadingContext& context );

	//! Unload layer group ( sync, slow, editor only )
	Bool SyncUnload();

#ifndef NO_EDITOR_WORLD_SUPPORT

	// Refreshes layers structure and reloads layers
	void Refresh();

	// Reload
	void Reload();

	// Save group, recursive
	Bool Save( Bool recursive, Bool onlyModified );

	// Sync
	void Sync();

#endif

	// Get group depot directory
	CDirectory* GetDepotDirectory() const;

	// Get layers from this group and all subgroups
	void GetLayers( TDynArray< CLayerInfo * > &layers, Bool loadedOnly, Bool recursive = true, Bool nonSystem = false /*HACK STREAMING*/ );

	// Get all layer that are persistent (will not be hidden)
	void GetPersistentLayers( TDynArray< CLayerInfo * > &layers, Bool loadedOnly, Bool recursive = true );

	// Get layers from this group and all subgroups
	void GetLayerGroups( TDynArray< CLayerGroup* > &layerGroups );

	// Get layers from this group and all subgroups
	Uint32 GetLayerGroups( CLayerGroup** layerGroups, Uint32 maxCount );

	// Get global layers from this group and all subgroups 
	Uint32 GetLayerGroupsForSave( CLayerGroup** globalLayerGroups, Uint32 maxCount );

	// Update the attachment/detachment of layers
	void ConditionalAttachToWorld();

	// Remove shadows from this layer group
	void RemoveShadowsFromGroup();
		
	// Add shadows to this layer group
	void AddShadowsToGroup();

	// Add shadows from local lights to this layer group
	void AddShadowsFromLocalLightsToGroup();

#ifndef NO_EDITOR
	void ConvertToStreamed(bool templatesOnly = false);
#endif

	// Update internal loading state
	void UpdateLoadingState();

	// Reset layer group visibility flag
	void ResetVisiblityFlag( const CGameInfo& info );

	// Change the initial visibility flag for a layer group (property setter)
	void SetInitialVisibility( Bool value );

	// Get layer group world path
	void GetLayerGroupPath(  String& outPath ) const;

	// Pull layer groups and layer infos into the CWorld resource (makes the loading faster and saves the layer enumeration step)
	void PullLayerGroupsAndLayerInfos();

	// Layer can be hidden in final game
	Bool CanBeHidden() const { return true; }

#ifndef NO_EDITOR
	Bool Rename( const String& newName );

	void MoveContents( CDirectory* destDir );

	void UpdatePath();
#endif

protected:
	friend class CWorldLayersVisibilityManagementTask;

	// Toggle layer group visibility ( show/hide layer group and layer within ) - this function can only be called
	// by a layer visibility changing task
	void SetVisiblityFlag( Bool isVisible, Bool isInPartition, TDynArray< CLayerInfo* >& changedLayers );

public:
#ifndef NO_EDITOR_WORLD_SUPPORT
	// Create new layer
	CLayerInfo* CreateLayer( const String& name, bool createAlways = false );

	// Create custom layer file
	CLayerInfo* CreateCustomLayer( const String& name, const String& ext, bool createAlways = false );

	// Add existing layer to group
	void AddLayer( CLayerInfo *info );

	// Remove layer
	void RemoveLayer( CLayerInfo* layer );

#endif

	// Find layer by name
	CLayerInfo* FindLayer( const String& name );

	// Find layer by GUID ( recursive )
	CLayerInfo* FindLayer( const CGUID& guid );

	// Find layer by name ignoring the letters casing
	CLayerInfo* FindLayerCaseless( const String& name );

#ifndef NO_EDITOR_WORLD_SUPPORT

	// Create new layer group
	CLayerGroup* CreateGroup( const String& name, Bool createAlways = false, Bool addToPerforceAlways = false );

	// Remove layer group
	void RemoveGroup( CLayerGroup* layerGroup );

	// Remove this group
	Bool Remove();

#endif

	// Find layer group by name
	CLayerGroup* FindGroup( const String& name );

	// Find layer group by name ignoring the letters casing
	CLayerGroup* FindGroupCaseless( const String& name );

	// Find layer group by relative path
	CLayerGroup* FindGroupByPath( const String& path );

	// Find layer by relative string path
	CLayerInfo* FindLayerByPath( const String& path );

	// Create group path name
	String GetGroupPathName( const CLayerGroup* base ) const;

	// Create group path name
	Uint32 GetGroupPathName( const CLayerGroup* base, AnsiChar* buffer, Uint32 bufferSize ) const;

	// List all layers
	void ListAllLayers( TList< CLayerInfo* > &layers, Bool recursive );

	// List all layers
	void ListAllLayers( TDynArray< CLayerInfo* > &layers, Bool recursive );

	// List all visible layers
	void ListAllVisibleLayers( TDynArray< CLayerInfo* > &layers, Bool recursive );

	// Grab layer info from file on disk
	static CLayerInfo* GrabRawDynamicLayerInfo( const String& depotPath );

	// Grab layer info from file on disk and initialize it
	CLayerInfo* GrabDynamicLayerInfo( const String& depotPath );

	// Link this layer group and below to the world, needed for cooked builds because m_world is not serialized 
	void LinkToWorld( CWorld* world );

public:
	//! Save layer group state to save game
	void SaveState( IGameSaver* saver );

	//! Load layer group state from save game
	void LoadState( IGameLoader* loader );

protected:
	// Populate layer group (scan for layers and sub groups)
	void Populate();

	// Populate layer groups from DLCs (this is called on on the top-level layergroup)
	void PopulateFromDLC();

	// Can we save the config file for this group ?
	Bool CanSave() const;

public:
	Uint32 CalcDataSize() const;

	void CalcMemSnapshot( SLayerGroupMemorySnapshot& snapshots ) const;

private:
	//! Performs non-recursive check as to whether this savegame shall be in savegame;
	RED_INLINE Bool CheckShouldSave_NoRecurse();
};

BEGIN_CLASS_RTTI( CLayerGroup )
	PARENT_CLASS( ISerializable )
	PROPERTY( m_name )
	PROPERTY( m_depotPath )
	PROPERTY( m_absolutePath )
	PROPERTY_EDIT( m_isVisibleOnStart, TXT("Is this layer group visible on start") )
	PROPERTY_SETTER( m_isVisibleOnStart, SetInitialVisibility )
	PROPERTY_NAME( m_oldSystemGroupFlag, TXT("systemGroup") )
	PROPERTY( m_hasEmbeddedLayerInfos )
	PROPERTY( m_idHash )
END_CLASS_RTTI()

/// System layer group
class CSystemLayerGroup : public CLayerGroup
{
	DECLARE_RTTI_SIMPLE_CLASS( CSystemLayerGroup);

public:
	CSystemLayerGroup();
	CSystemLayerGroup( CWorld* world, CLayerGroup* parentGroup, const String& name, const String& depotFilePath, const String& absoluteFilePath );

	virtual Bool IsSystemGroup() const { return true; }
};

BEGIN_CLASS_RTTI( CSystemLayerGroup );
	PARENT_CLASS( CLayerGroup );	
END_CLASS_RTTI();