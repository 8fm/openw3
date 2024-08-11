/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/engine/foliageRenderSettings.h"
#include "../../common/engine/meshRenderSettings.h"

class CEdWorldSceneDebugger : public wxFrame, public IEdEventListener
{
	DECLARE_EVENT_TABLE()

	struct SInternalEntityTemplateInfo
	{
		THandle< CEntityTemplate >	m_entityTemplate;
		Uint32						m_refCount;
		Uint32						m_dataSize;
		Bool						m_supposedDuplicated;
		Bool						m_meshComponentIsEmpty;
		TDynArray< String >			m_usedMeshes;
		TDynArray< const CEntity* >	m_entities;
		Int32						m_avgStreamingDist;
		Int32						m_maxStrDist;

		static Uint32		m_supposedDuplicateEntityCount;

		SInternalEntityTemplateInfo( const CEntityTemplate* entityTemplate, const CEntity* entity, Uint32 dataSize, const String& meshPath, Bool supposedDuplicated, 
			Bool meshComponentIsEmpty, Int32 avgComponentsStreamingDist, Int32 componentsMaxStrDist ) 
			: m_entityTemplate( entityTemplate )
			, m_refCount( 1 )
			, m_dataSize( dataSize )
			, m_supposedDuplicated( supposedDuplicated )
			, m_meshComponentIsEmpty( meshComponentIsEmpty )
			, m_avgStreamingDist( avgComponentsStreamingDist )
			, m_maxStrDist( componentsMaxStrDist )
		{
			m_usedMeshes.PushBack( meshPath );
			m_entities.PushBack( entity );
		}
	};

	struct SInternalMeshInfo
	{
		THandle< CMesh >	m_mesh;
		Uint32				m_refCount;
		Uint32				m_dataSize;
		Uint32				m_chunks;
		Uint32				m_triCount;
		Uint32				m_vertCount;
		Uint32				m_lodsCount;
		Uint32				m_texturesDataSize;
		TDynArray< String>	m_usedTextures;
		Float				m_autoHideDistance;

		SInternalMeshInfo( const CMesh* mesh, Uint32 dataSize, Uint32 chunks, Uint32 triCount, Uint32 vertCount, Uint32 lodsCount, Float autoHideDistance )
			: m_mesh( mesh )
			, m_refCount( 1 )
			, m_dataSize( dataSize )
			, m_chunks( chunks )
			, m_texturesDataSize( 0 )
			, m_lodsCount( lodsCount )
			, m_triCount( triCount )
			, m_vertCount( vertCount )
			, m_autoHideDistance( autoHideDistance )
		{
			/* intentionally empty */
		}
	};

	struct SInternalTextureInfo
	{
		THandle< CBitmapTexture >	m_texture;
		Uint32						m_dataSize;
		Uint32						m_refCount;
		TDynArray< String >			m_usedByTextureArray;
		TDynArray< String >			m_usedByMeshes;

		SInternalTextureInfo( const CBitmapTexture* texture, Uint32 dataSize )
			: m_texture( texture )
			, m_dataSize( dataSize )
			, m_refCount( 1 )
		{
		}

		SInternalTextureInfo( const CBitmapTexture* texture, Uint32 dataSize, const String& usedByMesh )
			: m_texture( texture )
			, m_dataSize( dataSize )
			, m_refCount( 1 )
		{
			m_usedByMeshes.PushBack( usedByMesh );
		}
	};

	struct SInternalTextureArrayInfo
	{
		THandle< CTextureArray >	m_textureArray;
		Uint32						m_dataSize;
		Uint32						m_refCount;
		TDynArray< String >			m_containedTextures;
		TDynArray< String >			m_usedByMeshes;

		SInternalTextureArrayInfo( const CTextureArray* textureArray )
			: m_textureArray( textureArray )
			, m_dataSize( 0 )
			, m_refCount( 1 )
		{
			/* intentionally empty */
		}
	};

	enum EApexType
	{
		AT_Cloth,
		AT_Destruction,

		At_Count
	};

	String ConvertEnumToString( enum EApexType apexType )
	{
		switch( apexType )
		{
		case AT_Cloth:
			return TXT("Cloth");
		case AT_Destruction:
			return TXT("Destruction");
		}
		return String::EMPTY;
	};

	struct SInternalApexInfo
	{
		THandle< CApexResource >				m_apexResource;
		Uint32									m_refCount;
		TDynArray< String >						m_usedByEntities;
		EApexType								m_apexType;
		Uint32									m_lods;
		Uint32									m_triangles;
		Uint32									m_vertices;

		SInternalApexInfo( const CApexResource* apexResource, EApexType type, Uint32 lods = 0, Uint32 triangles = 0, Uint32 vertices = 0 )
			: m_apexResource( apexResource )
			, m_refCount( 1 )
			, m_apexType( type )
			, m_lods( lods )
			, m_triangles( triangles )
			, m_vertices( vertices )
		{
			/* intentionally empty */
		}
	};

	struct SInternalSpeedTreeInfo
	{
		THandle< CSRTBaseTree >		m_speedTreeResource;
		Uint32						m_refCount;
		Uint32						m_dataSize;
		Uint32						m_texturesDataSize;
		Uint32						m_textureCount;
		TDynArray< String >			m_usedTextures;

		SInternalSpeedTreeInfo( const CSRTBaseTree* speedTreeResource, Uint32 refCount, Uint32 dataSize, Uint32 textureCount, Uint32 texturesDataSize )
			: m_speedTreeResource( speedTreeResource )
			, m_refCount( refCount )
			, m_dataSize( dataSize )
			, m_textureCount( textureCount )
			, m_texturesDataSize( texturesDataSize )
		{
			/* intentionally empty */
		}
	};

public:
	CEdWorldSceneDebugger( wxWindow* parent );
	~CEdWorldSceneDebugger();

	// implement virtual function from wxFrame
	virtual bool Show( bool show = true );

	void UpdateForceNoAutohideButtonLabel( Bool forceNoAutohide );

private:
	void LoadAndCreateGeneralControls();
	void LoadAndCreateEntitiesTab();
	void LoadAndCreateMeshesTab();
	void LoadAndCreateTexturesTab();
	void LoadAndCreateTextureArraysTab();
	void LoadAndCreateApexTab();
	void LoadAndCreateSpeedTreeTab();

	void CreateAdditionalEntityContextMenu();
	void CreateAdditionalMeshesContextMenu();
	void CreateAdditionalTextureContextMenu();
	void CreateAdditionalTextureArrayContextMenu();

	void OnAdditionalEntitiesContextMenu( wxCommandEvent& event );
	void OnAdditionalMeshesContextMenu( wxCommandEvent& event );
	void OnAdditionalTexturesContextMenu( wxCommandEvent& event );
	void OnAdditionalTextureArraysContextMenu( wxCommandEvent& event );

	void CollectDataFromWorld();
	void CollectEntityData( TDynArray< CMeshComponent* >& meshComponents, TDynArray< SInternalEntityTemplateInfo >& allEntitiesInfos, TDynArray< SInternalMeshInfo >& allMeshesInfos );
	void CollectMeshesData( TDynArray< CMeshComponent* >& meshComponents, TDynArray< SInternalMeshInfo >& allMeshesInfos );
	void CollectTexturesData( TDynArray< SInternalMeshInfo >& allMeshesInfos, TDynArray< SInternalTextureInfo >& allTexturesInfos );
	void CollectTextureArrayData( TDynArray< SInternalMeshInfo >& allMeshesInfos, TDynArray< SInternalTextureArrayInfo >& allTextureArraysInfos, TDynArray< SInternalTextureInfo >& allTexturesInfos );
	void CollectApexData( TDynArray< SInternalApexInfo >& allApexInfos );
	void CollectSpeedTreeData( TDynArray< SInternalSpeedTreeInfo >& allSpeedTreeInfos );

	void UpdateTexturesDataBasedOnTexureArray( SInternalTextureArrayInfo& textureArray, TDynArray< SInternalTextureInfo >& allTexturesInfos, SInternalMeshInfo& meshInfo );

	void FillEntitiesTab( TDynArray< SInternalEntityTemplateInfo >& allEntitiesInfos );
	void FillMeshesTab( TDynArray< SInternalMeshInfo >& allMeshesInfos );
	void FillTexturesTab( TDynArray< SInternalTextureInfo >& allTexturesInfos );
	void FillTextureArraysTab( TDynArray< SInternalTextureArrayInfo >& allTextureArraysInfos );
	void FillApexTab( TDynArray< SInternalApexInfo >& allApexInfos );
	void FillSpeedTreeTab( TDynArray< SInternalSpeedTreeInfo >& allSpeedTreeInfos );

	void OnExportToCSV( wxCommandEvent& event );
	void OnRefreshData( wxCommandEvent& event );
	void OnScrollToSelectedEntity( wxCommandEvent& event );

	void OnRowClicked( wxGridEvent& event );
	void OnOpenEditWindow( wxGridEvent& event );
	void OnSortByColumn( wxGridEvent& event );
	void OnShowColumnContextMenu( wxGridEvent& event );
	void OnShowRowContextMenu( wxGridEvent& event );
	void OnRowPopupClick( wxCommandEvent &event );
	void OnColumnPopupClick( wxCommandEvent &event );

	void OnSelectAllTabsToExport( wxCommandEvent& event );
	void OnExportButtonClicked( wxCommandEvent& event );
	void OnExportCancelButtonClicked( wxCommandEvent& event );

	void OnAutohideToggle( wxCommandEvent& event );
	void OnModeTabChanged( wxNotebookEvent& event );
	void OnSliderChanged( wxCommandEvent& event );
	void OnSpinnerChanged( wxSpinEvent& event );

	void Goto( wxGrid* grid, Uint32 tabIndex, const String& texturePath );

	void Export();
	void ExportGrid( C2dArray* array, wxGrid* exportedGrid, const String& gridName );

	void ShowInAssetBrowser( const String& path = TXT("") );
	void ShowInExplorer();
	void ShowAllResourceInstances();
	void ShowEntityInSceneExplorer();
	void ShowEntityOnMap( const String& entityName = String::EMPTY );
	void SelectRow( wxGrid* newSelectedGrid, Uint32 rowIndex );
	void ScrollToSelectedRow();

	Uint32 GridPointerToIndex( wxGrid* grid );
	void UpdateThumbnail( wxGridEvent& e );

	void GatherInformationAboutApexResource( const THandle< CApexResource >& apexResource, TDynArray< SInternalApexInfo >& allApexInfos, EApexType apexType );

	void UpdateInternalData();
	void UpdateForceRenderOptions();
	void GetSRenderSettingsFromINI();
	void UpdateUI();
	void PrepareParamsToUpdate();

#ifndef NO_EDITOR_EVENT_SYSTEM
	// Editor global envet
	void DispatchEditorEvent( const CName& name, IEdEventData* data );
#endif

private:
	// helpers
	Float						m_treeDistPrevious;
	Float						m_treeDistCurrent;
	Float						m_grassDistPrevious;
	Float						m_grassDistCurrent;
	SFoliageRenderParams		m_foliageRenderParams;
	
	SMeshRenderParams			m_meshRenderParams;
	Float						m_meshRenderDistCurrent;
	Float						m_meshRenderDistPrevious;
	Float						m_meshLODRenderDistCurrent;
	Float						m_meshLODRenderDistPrevious;
	Bool						m_dataCollected;

	// force render tab
	wxSlider*					m_foliageDistanceSlider;
	wxSlider*					m_grassDistSlider;
	wxSlider*					m_meshRenderDistSlider;
	wxSlider*					m_meshLODDistSlider;

	wxSpinCtrl*					m_foliageSpinner;
	wxSpinCtrl*					m_grassSpinner;
	wxSpinCtrl*					m_meshRenderDistSpinner;
	wxSpinCtrl*					m_meshLODDistSpinner;

	// general controls
	wxFrame*					m_exportDialogWindow;
	wxCheckListBox*				m_tabsListToExport;
	wxRadioBox*					m_exportToFile;
	wxCheckBox*					m_selectAllTabsToExport;
	wxCheckBox*					m_exportOnlyDuplicated;
	wxNotebook*					m_notebook;
	wxNotebook*					m_modeNotebook;
	wxButton*					m_helpClose;

	// entities tab
	wxGrid*						m_entitiesGrid;
	wxStaticText*				m_duplicateEntitiesCount;
	wxStaticText*				m_differentEntitiesCount;
	wxStaticText*				m_differentEntitiesDataCount;
	wxStaticText*				m_differentEntitiesOccuringOnceCount;
	wxStaticText*				m_differentDataInEntitiesOccuringOnceCount;
	wxStaticBitmap*				m_entityThumbnailBitmap;

	// meshes tab
	wxGrid*						m_meshesGrid;
	wxStaticText*				m_differentMeshesCount;
	wxStaticText*				m_differentChunksCount;
	wxStaticText*				m_differentMeshesDataCount;
	wxStaticText*				m_differentMeshesOccuringOnceCount;
	wxStaticText*				m_differentChunksInMeshesOccuringOnceCount;
	wxStaticText*				m_differentMeshDataInMeshesOccuringOnceCount;
	wxStaticBitmap*				m_meshThumbnailBitmap;

	// textures tab
	wxGrid*						m_texturesGrid;
	wxStaticText*				m_differentTexturesCount;
	wxStaticText*				m_differentTexturesDataCount;
	wxStaticText*				m_differentTextureOccuringOnceCount;
	wxStaticText*				m_differentDataInTextureOccuringOnceCount;
	wxStaticBitmap*				m_textureThumbnailBitmap;

	// texture array			s tab
	wxGrid*						m_textureArraysGrid;
	wxStaticText*				m_differentTextureArraysCount;
	wxStaticText*				m_differentTextureArraysDataCount;
	wxStaticText*				m_differentTextureArraysOccuringOnceCount;
	wxStaticText*				m_differentDataInTextureArraysOccuringOnceCount;
	wxStaticBitmap*				m_textureArrayThumbnailBitmap;

	// apex tab
	wxGrid*						m_apexGrid;
	wxStaticText*				m_differentApexClothComponentCount;
	wxStaticText*				m_differentApexDestructibleComponentCount;
	wxStaticText*				m_differentApexComponentOccuringOnceCount;
	wxStaticBitmap*				m_apexThumbnailBitmap;

	// speed tree tab
	wxGrid*						m_speedTreeGrid;
	wxStaticBitmap*				m_speedTreeThumbnailBitmap;
	wxStaticText*				m_totalSpeedTreeData;
	wxStaticText*				m_differentSpeedTreeTypeOccuringOnceCount;
	wxStaticText*				m_differentDataInSpeedTreeTypeOccuringOnceCount;
	wxStaticText*				m_speedTreeNumTypes;
	wxButton*					m_diffrentSpeedTreeTypesLassThanTenExport;

	// additional submenus
	wxMenuItem*					m_meshesSubMenu;
	wxMenuItem*					m_meshesSubMenu2;
	wxMenuItem*					m_entitiesSubMenu;
	wxMenuItem*					m_entitiesSubMenu2;
	wxMenuItem*					m_texturesSubMenu;
	wxMenuItem*					m_texturesSubMenu2;
	wxMenuItem*					m_texturesSubMenu3;
	wxMenuItem*					m_texturesSubMenu4;
	wxMenuItem*					m_textureArraySubMenu;
	wxMenuItem*					m_textureArraySubMenu2;
	wxMenuItem*					m_textureArraySubMenu3;
	wxMenuItem*					m_textureArraySubMenu4;

	// helpers
	wxGrid*						m_selectedGrid;
	Int32						m_selectedRow;

};
