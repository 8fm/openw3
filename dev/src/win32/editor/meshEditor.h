/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "dataError.h"
#include "meshPreviewPanel.h"
#include "editorPreviewCameraProvider.h"
#include "../../common/core/dataError.h"
#include "../../common/core/diskFile.h"

class CBoundControl;
class CEdBindingsBuilder;
class CEdUndoManager;

enum EEdMeshEditorPage {
	MEP_Properties,
	MEP_Materials,
	MEP_LODs,
	MEP_BakeAO,
	MEP_Report,
	MEP_VertexPaint,
	MEP_PhysicalRepresentation,
	MEP_SoundInfo,
	MEP_Cloth,
	MEP_Destructible,
	MEP_PhysXDestructible,

	// Do not move
	MEP_MeshEditorPagesNum,
};


struct SClothMaterialPreset
{
	String m_name;
	Float m_bendingStiffness;
	Float m_shearingStiffness;
	Float m_tetherStiffness;
	Float m_tetherLimit;
	Float m_damping;
	Float m_drag;
	Bool m_comDamping;
	Float m_friction;
	Float m_massScale;
	Float m_gravityScale;
	Float m_inertiaScale;
	Float m_hardStretchLimitation;
	Float m_maxDistanceBias;
	Float m_selfcollisionThickness;
	Float m_selfcollisionStiffness;
};

struct EEdMeshEditorPageHashFunc
{
	static RED_INLINE Uint32 GetHash( EEdMeshEditorPage page )
	{
		return ( Uint32 ) page;
	}
};

/// Mesh editor
class CEdMeshEditor : public wxSmartLayoutPanel
					, public IEdEventListener
					, public IEditorPreviewCameraProvider
					, public IDataErrorListener
{
	DECLARE_EVENT_TABLE()

protected:
	CMeshTypeResource*							m_mesh;				//!< Mesh to edit
	CEdMeshPreviewPanel*						m_preview;			//!< Preview panel
	CMeshComponent*								m_component;		//!< Preview entity
	class CEdMeshMaterialList*					m_materialList;		//!< Materials
	class CEdMeshLODList*						m_lodList;				//!< LODs
	class CEdPropertiesBrowserWithStatusbar*	m_properties;		//!< Mesh properties
	class CEdMeshPhysicalRepresentation*		m_physicalRepresentation;
	class CEdDestructionPhysicalRepresentation*	m_destrPhysRepresentation;
	class CEdMeshSoundInfo*						m_soundInfo;

	wxNotebook*									m_pages;			//!< Editor pages
	Bool										m_statsGenerated;	//!< Mesh stats were generated

	// ao options
	wxCheckBox*									m_secondaryBounce;
	wxCheckBox*									m_coloredAO;
	wxCheckBox*									m_diffuseTextures;
	wxTextCtrl*									m_energyMultiplier;

	wxRadioBox*									m_lightType;
	wxColourPickerCtrl*							m_lightColor;

	wxTextCtrl*									m_lightDirectionX;
	wxTextCtrl*									m_lightDirectionY;
	wxTextCtrl*									m_lightDirectionZ;

	wxTextCtrl*									m_lightStrength;

	wxTextCtrl*									m_samplesFirst;
	wxTextCtrl*									m_samplesSecond;

	wxCheckBox*									m_useGround;
	wxColourPickerCtrl*							m_groundColor;
	wxTextCtrl*									m_groundZ;

	wxTextCtrl*									m_octreeDensity;
	
	wxButton*									m_buildOctree;
	wxButton*									m_bakeAO;

	wxChoice*									m_displayedLodChoice;

	wxButton*									m_mergeChunks;
	wxButton*									m_autoMergeChunks;
	wxButton*									m_removeChunk;
	wxButton*									m_removeSkinning;
	wxChoice*									m_chunkMaterials;

	wxGauge*									m_progressAO;

	wxSpinCtrl*									m_vertexPaintLod;

	class CVoxelOctree*							m_octree;	

	typedef THashMap< EEdMeshEditorPage, Int32, EEdMeshEditorPageHashFunc >	TPageIndexMap;
	TPageIndexMap								m_pageIndices;		//!< Current indices of all pages. If a page has been removed, its index is -1.

	CEdBindingsBuilder*							m_boundControls;	//!< Live-update bindings for current mesh. Created based on mesh type.
	
	CEdUndoManager*								m_undoManager;

	TSortedArray< SDataError, SDataError::BySeverity > m_dataErrors;		//!< Errors collected by data error reporter for active mesh
	TDynArray< String >							m_dependentResourcesPaths;

	/// Bit of a HACK, basically when setting all live-update controls to new values (when first loading an apex resource, or when restoring defaults),
	/// Each time a control's value is changed it triggers the resource to be rebuilt. We use this flag to temporarily disable the rebuild, so it only
	/// needs to be done a single time.
	Bool m_freezeApexUpdates;

	EulerAngles m_windDirection;
	Float m_windStrength;
	Float m_windStrengthScale;

	THashMap<String, SClothMaterialPreset> m_clothPresets;

public:
	CEdMeshEditor( wxWindow* parent, CMeshTypeResource* mesh );
	~CEdMeshEditor();

	virtual wxString GetShortTitle() { return m_mesh->GetFile()->GetFileName().AsChar() + wxString(TXT(" - Mesh Editor")); }
	CMeshTypeResource* GetMesh() const { return m_mesh; }

	// implement IDataErrorListener interface
	virtual void OnDataErrorReported( const SDataError& error );
	virtual void StartProcessing()													{	}
	virtual void ProcessDataErrors( const TDynArray< SDataError >& errors )			{	}
	virtual void StoreNonProcessedErrors( const TDynArray< SDataError >& errors )	{	}
	virtual void StopProcessing()													{	}

	// get data errors
	void GetDataErrors( TDynArray< String >& arrayForErrors );
	void AddDataErrors( const TDynArray< SDataError >& dataErrors );

	// Save / Load options from config file
	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();

	void UpdateDisplayedLODChoice();
	void UpdateMeshStats();
	void UpdateMaterialList();
	void UpdateChunkMaterialsChoice();
	void UpdateLODList( Bool preserveExpansionState );
	void RefreshPreviewRenderingProxy();
	void RefreshPreviewBBoxInfo();

	virtual IEditorPreviewCameraProvider::Info GetPreviewCameraInfo() const override;

protected:
	void OnApexPropertyChanged( CBoundControl* boundControl );
	void OnApexDestMaxDepthChanged( CBoundControl* boundControl );
	void OnPreviewWindChanged( CBoundControl* boundControl );

	// Callbacks passed to CEdBindingsBuilder to handle events from bound controls.
	static void OnApexPropertyChangedCallback( void* obj, CBoundControl* boundControl ) { ((CEdMeshEditor*)obj)->OnApexPropertyChanged( boundControl ); }
	static void OnApexDestMaxDepthChangedCallback( void* obj, CBoundControl* boundControl ) { ((CEdMeshEditor*)obj)->OnApexDestMaxDepthChanged( boundControl ); }
	static void OnPreviewWindChangedCallback( void* obj, CBoundControl* boundControl ) { ((CEdMeshEditor*)obj)->OnPreviewWindChanged( boundControl ); }

	void SetupPages();

	/// Refresh the bound controls, so that they reflect the values they are bound to.
	void RefreshBoundControls();

	void RefreshDestructMaterials();

	void OnApexRestoreDefaults( wxCommandEvent& event );
	void OnApexDestructibleReset( wxCommandEvent& event );
	void OnApexDestructibleMtlSelected( wxCommandEvent& event );
	void OnApexDestructibleMtlFill( wxCommandEvent& event );
	void OnApexDestructibleDensity( wxCommandEvent& event );
	
	void OnSave( wxCommandEvent& event );
	void OnUndo( wxCommandEvent& event );
	void OnRedo( wxCommandEvent& event );
	void OnLODSelected( wxTreeEvent& event );
	void OnPageChanged( wxNotebookEvent& event );
	void OnToolRecalculateBoundingBox( wxCommandEvent& event );
	void OnShowWireframe( wxCommandEvent& event );
	void OnShowBoundingBox( wxCommandEvent& event );
	void OnShowCollision( wxCommandEvent& event );
	void OnShowNavObstacles( wxCommandEvent& event );
	void OnShowTBN( wxCommandEvent& event );
	void OnRemoveUnusedMaterials( wxCommandEvent& event );
	void OnRemoveUnusedBones( wxCommandEvent& event );
	void OnRemoveSkinningData( wxCommandEvent& event );
	void OnMergeDuplicatedChunks( wxCommandEvent& event );
	void OnReimportMesh( wxCommandEvent& event );
	void OnReimportMeshMenu( wxCommandEvent& event );
	void OnUpdateStats( wxCommandEvent& event );
	void OnLinkClicked( wxHtmlLinkEvent& event );
	void OnZoomExtents( wxCommandEvent& event );
	void OnDisplayedLODChanged( wxCommandEvent& event );

	void OnSwapCollisionTriangles( wxCommandEvent& event );
	void OnGenerateBillboards( wxCommandEvent& event );
	void OnExportRE( wxCommandEvent& event );
	void OnLODExportRE( wxCommandEvent& event );
	void OnAddMesh( wxCommandEvent& event );
	void OnExportFbx( wxCommandEvent& event );

	void OnBuildOctree( wxCommandEvent& event );
	void OnBakeAO( wxCommandEvent& event );
	void OnUseGroundAO( wxCommandEvent& event );
	
#ifdef USE_SIMPLYGON
	void OnAddLOD( wxCommandEvent& event );
	void OnGenerateLODs( wxCommandEvent& event );
	void OnRemoveLOD( wxCommandEvent& event );
#endif
	void OnMergeChunks( wxCommandEvent& event );
	void OnMergeChunksWith( wxCommandEvent& event );
	void OnAutoMergeChunks( wxCommandEvent& event );
	void OnRemoveChunk( wxCommandEvent& event );
	void OnRemoveSkinning( wxCommandEvent& event );
	void OnChunkMaterialChanged( wxCommandEvent& event );

	void OnUpdateUI( wxUpdateUIEvent& event );

	void OnVertexPaintEditToggle( wxCommandEvent& event );	

	void OnClothPresetChoice( wxCommandEvent& event );
	void OnClothPresetSelectFile( wxCommandEvent& event );

	Bool LoadClothPresets( const String& filename );

	void DispatchEditorEvent( const CName& name, IEdEventData* data );
	void OnPropertyModified( wxCommandEvent& event );

	void UpdateMeshStatsNow();
	void ShowMaterialsTabAndHighlightMaterial( const String& materialName );

	Bool SetSelectedPage( EEdMeshEditorPage page );
	Bool RemovePage( EEdMeshEditorPage page );
	Bool IsPageSelected( EEdMeshEditorPage page );	

	Bool DoMergeChunks( const TDynArray< Uint32 >& chunkIndices, Uint32 materialIndex );
	void DoAutoMergeChunks();
	Bool CheckIfChunksShouldBeMerged( Bool showWarning );

//	void OnVertexPaintLodSelected( wxCommandEvent& event );

	void OnActivate( wxActivateEvent& event );

	String CreateChunkMaterialName( Uint32 index ) const;
	void ReimportMesh( Bool reuseMesh, Bool reuseVolumes, Bool regenerateVolumes );
	void ValidateResources();

	// Export to RE part
	void ExportToREFile( const String& path, Int32 lodLevelIdx = -1 ) const;
};
