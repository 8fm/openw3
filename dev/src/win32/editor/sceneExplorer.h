/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../../common/core/resourcepaths.h"
class CEdLayerBrowser;
class CNode;
class CNavmeshComponent;

class CEdScenePresetManager
{
	struct SPresetEntry
	{
		String m_path;
		TDynArray< String > m_hidden;

		SPresetEntry(){}
	};

	struct SPreset
	{
		String m_name;
		TDynArray< SPresetEntry* > m_entries;
		Bool m_root;
		TDynArray< String > m_rootHidden;

		SPreset(){}
		SPreset( const String& name ) : m_name( name ) {}
		~SPreset();

		void Clear();
		void Assign( const SPreset& preset );
		
		String ToString() const;
		Bool FromString( const String& data );
	};

	TDynArray< SPreset* >			m_presets;
	SPreset*						m_active;

	void Clear();
	SPreset* Find( const String& name ) const;
	SPreset* FindOrCreate( const String& name );

public:
	CEdScenePresetManager();
	~CEdScenePresetManager();

	RED_INLINE Bool Has( const String& name) const { return Find( name ) != NULL; }

	void SetActive( const String& name, Bool create );
	const String& GetActive() const;

	void GetPresetNames( TDynArray< String >& names ) const;

	void Reset();
	void StoreGroupsTo( const String& name );
	void RestoreGroupsFrom( const String& name );
	void Remove( const String& name );
	Bool Rename( const String& oldName, const String& newName );

	Bool SetPresetFromString( const String& data, String& parsedPresetName );
	String GetPresetAsString( const String& name ) const;

	void Save();
	void Load();
};

enum ESceneSortingMode
{
	SSM_NoSorting,
	SSM_Alphabetically
};

enum ECopyMode
{
	CM_Entities,
	CM_Layers,
	CM_Default
};

/// Scene explorer
class CEdSceneExplorer: public wxPanel, public IEdEventListener, public ISavableToConfig, public CDropTarget
{
    DECLARE_EVENT_TABLE();

	friend class SceneView;

private:
	class SceneView*						m_sceneView;
    wxComboBox*								m_presetComboBox;
	CEdScenePresetManager					m_presetManager;
	class CEntityListManager*				m_entityListManager;
	THashSet< THandle< ISerializable > >	m_globalFilter;
	ESceneSortingMode						m_sortingMode;	
	class CEdErrorsListDlg*					m_duplicatesErrorsList;

protected:
    TDynArray< THandle< CLayerInfo > > m_loadedByHand;

    virtual void OnInternalIdle();

public:
    CEdSceneExplorer( wxWindow* parent );
	~CEdSceneExplorer();

    // Get active layer info
    CLayerInfo* GetActiveLayerInfo();

    // Get active layer
    CLayer* GetActiveLayer();

    // Set active layer
    void ChangeActiveLayer( CLayerInfo* layerInfo );

    // Update layer
    void UpdateLayer( CLayer* layer );

    // Select particular scene object
    void SelectSceneObject( ISerializable* object, Bool clearSelection = false );

	// Deselect particular scene object
	void DeselectSceneObject( ISerializable* object );

    // add object to scene explorer
    void AddSceneObject( ISerializable *object );

    // remove object from scene explorer
    void RemoveSceneObject( ISerializable *object );

    // save session stuff
    void SaveSession( CConfigurationManager &config );
    void RestoreSession( CConfigurationManager &config );

    // Clear presets
    void ClearPresets();

	// Update selected node
	void UpdateSelected( Bool expandToSelection = true );

	// Returns the selected items
	void GetSelection( TDynArray< ISerializable* >& selection );

	// Saves world with modified layers only
	void SaveWorldModifiedOnly();

	// Remove empty layers
	void OnRemoveEmptyLayers( wxCommandEvent& event );

    void MarkTreeItemsForUpdate();

	void LoadAllLayers( Bool quiet = false );

	RED_INLINE class CEntityListManager* GetEntityListManager() const { return m_entityListManager; }

	void OnEntityListMassActions( class CEntityList* entityList, const LayerEntitiesArray& entities );

	// Returns true if the given serializable is filtered out
	Bool IsFilteredOut( ISerializable* serializable ) const;

	// Returns true if there is are global filters
	Bool HasGlobalFilters() const;

	// Clear the global filter
	void ClearGlobalFilter();

	// Adds the given serializable to the global filter
	void AddToGlobalFilter( ISerializable* serializable );

	// Removes the given serializable from the global filter
	void RemoveFromGlobalFilter( ISerializable* serializable );

	// Set sorting mode
	void SetSortingMode( ESceneSortingMode mode );

	// Get sorting mode
	RED_INLINE ESceneSortingMode GetSortingMode() const { return m_sortingMode; }

	void CheckLayersEntitiesCompatibility( const TDynArray< CLayerInfo* >& layers, Bool modifiedOnly ) const;

	void OnCopy( ECopyMode copyMode = CM_Default, Bool cut = false );
	void OnPaste( ECopyMode copyMode = CM_Default, Bool pasteToParentGroup = false );

protected:
    // Refresh layer presets list
    void RefreshPresetComboBox();

	CNavmeshComponent* GetSelectedNavmeshComponent();

protected:

    // Events
    void DispatchEditorEvent( const CName& name, IEdEventData* data );
    //void OnTreeItemMenu( wxTreeEvent& event );	
    //void OnTreeItemActivated( wxTreeEvent& event );
    //void OnTreeItemSelChanged( wxTreeEvent& event );
    //void OnTreeItemExpanding( wxTreeEvent& event );
    //void OnTreeItemCollapsing( wxTreeEvent& event );
    void OnLookAtNode( wxCommandEvent& event );
	void OnMovePlayerThere( wxCommandEvent& event );
    void OnCopyNode( wxCommandEvent& event );
    void OnCutNode( wxCommandEvent& event );
    void OnRemoveNode( wxCommandEvent& event );
    void OnPresetChanged( wxCommandEvent& event );
    void OnSavePresets( wxCommandEvent& event );
	void OnRenamePreset( wxCommandEvent& event );
	void OnDeletePreset( wxCommandEvent& event );
	void OnCopyPreset( wxCommandEvent& event );
	void OnPastePreset( wxCommandEvent& event );
	void OnDisplayLBTColors( wxCommandEvent& event );
	void OnNoSorting( wxCommandEvent& event );
	void OnAlphabeticSorting( wxCommandEvent& event );
	void OnRemoveObjectFilters( wxCommandEvent& event );
    void OnLoadPresets( wxCommandEvent& event );
    void OnPresetActions( wxCommandEvent& event );
    void OnDisplayOptions( wxCommandEvent& event );
	void OnFilterBoxText( wxCommandEvent& event );
    void OnLoadLayerGroup( wxCommandEvent& event );
    void OnUnloadLayerGroup( wxCommandEvent& event );
    void OnRemoveLayerGroup( wxCommandEvent& event );
	void OnGenerateResourceGraph( wxCommandEvent& event );
	void OnCollectEmptyEntities( wxCommandEvent& event );
	void OnCollectLightEntities( wxCommandEvent& event );
    void OnAddLayerGroup( wxCommandEvent& event );
    void OnAddLayer( wxCommandEvent& event );
    void OnSaveLayerGroupHierarchy( wxCommandEvent& event );
	void OnSaveLayerGroupThis( wxCommandEvent& event );
    void OnSaveLayer( wxCommandEvent& event );
    void OnLoadLayer( wxCommandEvent& event );
    void OnSetActiveLayer( wxCommandEvent& event );
    void OnSelectAllEntities( wxCommandEvent& event );
    void OnSelectFromSpecifiedLayers( wxCommandEvent& event );
    void OnSelectByResource( wxCommandEvent& event );
	void OnEditEncounter( wxCommandEvent& event );
    void OnEditEnvironment( wxCommandEvent& event );
	void OnGenerateNavmesh( wxCommandEvent& event );
	void OnComputeNavmeshBasedBounds( wxCommandEvent& event );
	void OnStopNavmeshGeneration( wxCommandEvent& event );
	void OnGenerateNavgraph( wxCommandEvent& event );
	void OnResetNavmeshParams( wxCommandEvent& event );
	void OnChangeSwarmPOIType( wxCommandEvent& event );
	void OnFixSwarm( wxCommandEvent& event );
	void OnFixAllSwarmLairs( wxCommandEvent& event );
	void OnGenerateCompatibilityErrorsReport( wxCommandEvent& event );
	void OnGenerateSwarmCollisions( wxCommandEvent& event );
	void OnShowStreamingInfo( wxCommandEvent& event );
	void OnForceStreamIn( wxCommandEvent& event );
	void OnForceStreamOut( wxCommandEvent& event );
	void OnIgnoreStream( wxCommandEvent& event );
	void OnUnignoreStream( wxCommandEvent& event );
	void OnClearIgnoreList( wxCommandEvent& event );
	void OnShowIgnoredEntities( wxCommandEvent& event );
	void OnResetInstanceProperties( wxCommandEvent& event );
	void OnMassActions( wxCommandEvent& event );
	void OnFindDuplicates( wxCommandEvent& event );
	void OnFindDuplicateIdTags( wxCommandEvent& event );
	void OnAddToEntityList( wxCommandEvent& event );
	void OnLoadEntityList( wxCommandEvent& event );

	void OnGroupItems( wxCommandEvent& event );
	void OnUngroupItems( wxCommandEvent& event );
	void OnLockGroup( wxCommandEvent& event );
	void OnUnlockGroup( wxCommandEvent& event );
	void OnRemoveFromGroup( wxCommandEvent& event );
    
    void OnLayerCheckOut( wxCommandEvent &event );
    void OnLayerSubmit( wxCommandEvent &event );
    void OnLayerRevert( wxCommandEvent &event );
    void OnLayerAdd( wxCommandEvent &event );
    void OnLayerSync( wxCommandEvent &event );
	void OnCopyLayerPath( wxCommandEvent &event );
    void OnRemoveLayer( wxCommandEvent &event );
	void OnRenameLayer( wxCommandEvent &event );
	void OnCopyLayer( wxCommandEvent& event );
	void OnCutLayer( wxCommandEvent& event );
	void OnPasteLayer( wxCommandEvent& event );

    void OnLayerShow( wxCommandEvent &event );
    void OnLayerHide( wxCommandEvent &event );

    void OnGroupCheckOut( wxCommandEvent &event );
    void OnGroupSubmit( wxCommandEvent &event );
    void OnGroupRevert( wxCommandEvent &event );
    void OnGroupAdd( wxCommandEvent &event );
    void OnGroupSync( wxCommandEvent &event );
	void OnRenameGroup( wxCommandEvent &event );

    void OnWorldGetLatest( wxCommandEvent &event );
    void OnWorldCheckOut( wxCommandEvent &event );
    void OnWorldSubmit( wxCommandEvent &event );
    void OnWorldRevert( wxCommandEvent &event );
	void OnWorldDeleteLayerGroupFiles( wxCommandEvent &event );
	void OnWorldSaveDependencyFile( wxCommandEvent &event );

    void OnImportResource( wxCommandEvent& event );
    void OnExportResource( wxCommandEvent& event );

    void OnMoveContentToLayer( wxCommandEvent& event );
    void OnMergeLayers( wxCommandEvent& event );

    void OnAddShadowsToGroup( wxCommandEvent& event );
    void OnRemoveShadowsToGroup( wxCommandEvent& event );	
    void OnAddShadowsToLayer( wxCommandEvent& event );
    void OnRemoveShadowsToLayer( wxCommandEvent& event );	

	void OnAddShadowsFromLocalLightsToLayer( wxCommandEvent& event );
	void OnAddShadowsFromLocalLightsToGroup( wxCommandEvent& event );

	void OnConvertLayerToStreamed( wxCommandEvent& event );	
	void OnConvertGroupToStreamed( wxCommandEvent& event );	
	void OnConvertGroupToStreamedTemplatesOnly( wxCommandEvent& event );

	void OnConvertGroupToNonStatic( wxCommandEvent& event );
	void OnConvertGroupToStatic( wxCommandEvent& event );
	void OnConvertAllLayersTo( wxCommandEvent& event );

	void OnResetLightChannels( wxCommandEvent& event );
	void OnRemoveForceNoAutohide( wxCommandEvent& event );
	void RemoveForceNoAutohide( CLayerGroup* group, TDynArray<String> &unableToCheckoutLayers, 
		Uint32 &processedItemsAutohide, Uint32 &currentGroupNum, Uint32 &currentLayerNum );

	void OnInspectObject( wxCommandEvent& event );

    Bool OnDropText( wxCoord x, wxCoord y, String &text );

    void OnMouseCaptureLost( wxMouseCaptureLostEvent& WXUNUSED(event) );

private:

    void UpdatePresetsUI();

	void ShowSceneMassActionsDialog( class IEdMassActionIterator* iterator );

	void SelectSpecifiedEntities( SceneView* sceneView, const CLayerInfo *layerInfo, const CClass* givenClass = nullptr, 
		const TDynArray< CClass* >& componentsClasses = TDynArray< CClass* >(), const TDynArray< CClass* >& excludedComponents = TDynArray< CClass* >() );

	void SaveLayerGroups( const TDynArray< CLayerGroup* >& layerGroups, Bool modifiedOnly );
};

Bool LoadLayers( const TDynArray< CLayerInfo* >& layersToLoad, LayerGroupLoadingContext& loadingContext, Bool quiet = false );
