/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "materialListManager.h"
#include "meshMaterialList.h"
#include "meshLODList.h"
#include "meshEditor.h"
#include "shortcutsEditor.h"
#include "assetBrowser.h"
#include "voxelization.h"
#include <omp.h>
#include "vertexPaintTool.h"
#include "meshPhysicalRepresentation.h"
#include "meshLiveBindings.h"
#include "previewWorld.h"
#include "undoManager.h"
#include "batchLODGenerator.h"
#include "../../common/engine/apexClothResource.h"
#include "../../common/engine/apexDestructionResource.h"
#include "../../common/engine/meshDataBuilder.h"
#include "undoMeshEditor.h"
#include "batchLODGenerator.h"
#include "../../common/core/versionControl.h"
#include "../../common/core/depot.h"
#include "../../common/core/xmlFileReader.h"
#include "../../common/core/feedback.h"

#include "textControl.h"
#include "meshStats.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/materialDefinition.h"
#include "../../common/engine/materialInstance.h"
#include "../../common/engine/furMeshResource.h"
#include "../../common/core/exporter.h"
#include "meshSoundInfo.h"
#include "destructionPhysicalRepresentation.h"
#include "../../common/engine/physicsDestructionResource.h"
// RE
#include "re_archive/include/reFileArchive.h"
#include "re_archive/include/reFileNodes.h"
#include "../importer/meshExport.h"

// Event table
BEGIN_EVENT_TABLE( CEdMeshEditor, wxSmartLayoutPanel )
	EVT_MENU( XRCID("meshSave"), CEdMeshEditor::OnSave )
	EVT_MENU( XRCID("editUndo"), CEdMeshEditor::OnUndo )
	EVT_MENU( XRCID("editRedo"), CEdMeshEditor::OnRedo )
	EVT_NOTEBOOK_PAGE_CHANGED( XRCID("Notebook1"), CEdMeshEditor::OnPageChanged )
	EVT_TOOL( XRCID("toolMeshSave"), CEdMeshEditor::OnSave )
	EVT_MENU( XRCID("toolRecalculateBoundingBox"), CEdMeshEditor::OnToolRecalculateBoundingBox )
	EVT_MENU( XRCID("toolRemoveUnusedMaterials"), CEdMeshEditor::OnRemoveUnusedMaterials )
	EVT_MENU( XRCID("toolRemoveUnusedBones"), CEdMeshEditor::OnRemoveUnusedBones )
	EVT_MENU( XRCID("toolRemoveSkinning"), CEdMeshEditor::OnRemoveSkinningData )	
	EVT_TOOL( XRCID("toolReimport"), CEdMeshEditor::OnReimportMesh )

	EVT_TOOL( XRCID("toolShowWireframe"), CEdMeshEditor::OnShowWireframe )
	EVT_TOOL( XRCID("toolShowBoundingBox"), CEdMeshEditor::OnShowBoundingBox )
	EVT_TOOL( XRCID("toolShowCollision"), CEdMeshEditor::OnShowCollision )
	EVT_TOOL( XRCID("toolShowNavObstacles"), CEdMeshEditor::OnShowNavObstacles )
	EVT_TOOL( XRCID("toolShowTBN"), CEdMeshEditor::OnShowTBN )
	EVT_TOOL( XRCID("toolSwapCollisionTriangles"), CEdMeshEditor::OnSwapCollisionTriangles )
	//EVT_TOOL( XRCID("toolGenerateBillboards"), CEdMeshEditor::OnGenerateBillboards )
	EVT_TOOL( XRCID("toolExportRE"), CEdMeshEditor::OnExportRE )	
	EVT_TOOL( XRCID("toolAddMesh"), CEdMeshEditor::OnAddMesh )
	EVT_TOOL( XRCID("toolExportFbx"), CEdMeshEditor::OnExportFbx )		
	
	EVT_TOOL( XRCID("toolZoomExtentsPreview"), CEdMeshEditor::OnZoomExtents )
	EVT_MENU( XRCID("menuZoomExtentsPreview"), CEdMeshEditor::OnZoomExtents )

	EVT_CHOICE( XRCID("LODChoice"), CEdMeshEditor::OnDisplayedLODChanged )

	EVT_HTML_LINK_CLICKED( XRCID("Stats"), CEdMeshEditor::OnLinkClicked )
	EVT_BUTTON( XRCID("UpdateStats"), CEdMeshEditor::OnUpdateStats )
	EVT_BUTTON( XRCID("m_buildOctree"), CEdMeshEditor::OnBuildOctree )
	EVT_BUTTON( XRCID("m_bakeAO"), CEdMeshEditor::OnBakeAO )
	EVT_CHECKBOX( XRCID("m_useGround"), CEdMeshEditor::OnUseGroundAO )
	EVT_TEXT_ENTER( XRCID("m_groundZ"), CEdMeshEditor::OnUseGroundAO )	
	EVT_TOGGLEBUTTON( XRCID("m_VEditToggle"), CEdMeshEditor::OnVertexPaintEditToggle )

	EVT_BUTTON( XRCID("clothRestoreDefaults"), CEdMeshEditor::OnApexRestoreDefaults )
	EVT_BUTTON( XRCID("destRestoreDefaults"), CEdMeshEditor::OnApexRestoreDefaults )
	EVT_BUTTON( XRCID("destReset"), CEdMeshEditor::OnApexDestructibleReset )
	EVT_BUTTON( XRCID("DestructFillMtl"), CEdMeshEditor::OnApexDestructibleMtlFill )

#ifdef USE_SIMPLYGON
	EVT_BUTTON( XRCID("AddLOD"), CEdMeshEditor::OnAddLOD )
	EVT_BUTTON( XRCID("GenerateLODs"), CEdMeshEditor::OnGenerateLODs )
	EVT_BUTTON( XRCID("RemoveLOD"), CEdMeshEditor::OnRemoveLOD )
#endif

	EVT_BUTTON( XRCID("ExportLOD"), CEdMeshEditor::OnLODExportRE )
	EVT_BUTTON( XRCID("ExportFbx"), CEdMeshEditor::OnExportFbx )
	EVT_BUTTON( XRCID("MergeChunks"), CEdMeshEditor::OnMergeChunks )
	EVT_BUTTON( XRCID("AutoMergeChunks"), CEdMeshEditor::OnAutoMergeChunks )
	EVT_BUTTON( XRCID("RemoveChunk"), CEdMeshEditor::OnRemoveChunk )
	EVT_BUTTON( XRCID("RemoveSkinning"), CEdMeshEditor::OnRemoveSkinning )
	EVT_CHOICE( XRCID("ChunkMaterial"), CEdMeshEditor::OnChunkMaterialChanged )

	EVT_BUTTON( XRCID("clothPresetSelect"), CEdMeshEditor::OnClothPresetSelectFile )
	EVT_UPDATE_UI( wxID_ANY, CEdMeshEditor::OnUpdateUI )
END_EVENT_TABLE()


CEdMeshEditor::CEdMeshEditor( wxWindow* parent, CMeshTypeResource* mesh )
	: wxSmartLayoutPanel( parent, TXT("MeshEditor"), false )
	, m_mesh( mesh )
	, m_octree( NULL )
	, m_freezeApexUpdates( false )
	, m_windDirection( 0.0f, 0.0f, 0.0f )
	, m_windStrength( 0.0f )
	, m_windStrengthScale( 1.0f )
	, m_lodList( nullptr )
	, m_physicalRepresentation( nullptr )
	, m_soundInfo( nullptr )
{
	GetOriginalFrame()->Bind( wxEVT_ACTIVATE, &CEdMeshEditor::OnActivate, this );

	m_dataErrors.ClearFast();

	// Keep reference to mesh as long as editor is opened
	m_mesh->AddToRootSet();

	// Set title
	String title = GetTitle().wc_str();
	SetTitle( String::Printf( TEXT("%s - %s"), title.AsChar(), mesh->GetFriendlyName().AsChar() ).AsChar() );

	// Set icon
	wxIcon iconSmall;
	iconSmall.CopyFromBitmap( SEdResources::GetInstance().LoadBitmap( _T("IMG_TOOL") ) );
	SetIcon( iconSmall );

	// Create rendering panel
	{
		wxPanel* rp = XRCCTRL( *this, "RenderingPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		m_preview = new CEdMeshPreviewPanel( rp, this );
		m_preview->GetViewport()->SetRenderingMode( RM_Shaded );
		sizer1->Add( m_preview, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	// Undo manager
	{
		m_undoManager = new CEdUndoManager( this );
		m_undoManager->AddToRootSet();
		m_undoManager->SetMenuItems( GetMenuBar()->FindItem( XRCID("editUndo") ), GetMenuBar()->FindItem( XRCID("editRedo") ) );
	}

	// Create properties panel
	{
		wxPanel* rp = XRCCTRL( *this, "PropertiesPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		PropertiesPageSettings settings;
		m_properties = new CEdPropertiesBrowserWithStatusbar( rp, settings, m_undoManager );
		sizer1->Add( m_properties, 1, wxEXPAND|wxALL, 5 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}


	// Get the notebook pages
	m_pages = XRCCTRL( *this, "Notebook1", wxNotebook );
	for ( Int32 i = 0; i < MEP_MeshEditorPagesNum; ++i )
	{
		m_pageIndices.Insert( ( EEdMeshEditorPage )i, i );
	}

	// Setup material list

	if ( !mesh->IsA< CFurMeshResource >() ) // fur has a chunk but contains no real materials, which breaks MaterialList logic
	{
		wxPanel* matPanel = XRCCTRL( *this, "MaterialsPanel", wxPanel );
		matPanel->SetSizer( new wxBoxSizer( wxVERTICAL ) );
		m_materialList = new CEdMeshMaterialList( matPanel, this, m_mesh, m_undoManager );
		matPanel->GetSizer()->Add( m_materialList, 1, wxEXPAND );
	}

	// Show mesh properties
	m_properties->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdMeshEditor::OnPropertyModified ), NULL, this );
	m_properties->Get().SetObject( mesh );

	SEvents::GetInstance().RegisterListener( RED_NAME( FileReimport ), this );
	SEvents::GetInstance().RegisterListener( RED_NAME( FileReload ), this );
	SEvents::GetInstance().RegisterListener( RED_NAME( FileReloadConfirm ), this );
	SEvents::GetInstance().RegisterListener( RED_NAME( EditorPostUndoStep ), this );
	SEvents::GetInstance().RegisterListener( RED_NAME( EditorPropertyPostChange ), this );

	m_secondaryBounce = XRCCTRL( *this, "m_secondaryBounce", wxCheckBox );
	m_coloredAO = XRCCTRL( *this, "m_coloredAO", wxCheckBox );
	m_diffuseTextures = XRCCTRL( *this, "m_diffuseTextures", wxCheckBox );
	m_energyMultiplier = XRCCTRL( *this, "m_energyMultiplier", wxTextCtrl );

	m_lightType = XRCCTRL( *this, "m_lightType", wxRadioBox );
	m_lightColor = XRCCTRL( *this, "m_lightColor", wxColourPickerCtrl );

	m_lightDirectionX = XRCCTRL( *this, "m_lightDirectionX", wxTextCtrl );
	m_lightDirectionY = XRCCTRL( *this, "m_lightDirectionY", wxTextCtrl );
	m_lightDirectionZ = XRCCTRL( *this, "m_lightDirectionZ", wxTextCtrl );

	m_lightStrength = XRCCTRL( *this, "m_lightStrength", wxTextCtrl );

	m_samplesFirst = XRCCTRL( *this, "m_samplesFirst", wxTextCtrl );
	m_samplesSecond = XRCCTRL( *this, "m_samplesSecond", wxTextCtrl );

	m_useGround = XRCCTRL( *this, "m_useGround", wxCheckBox );
	m_groundColor = XRCCTRL( *this, "m_groundColor", wxColourPickerCtrl );
	m_groundZ = XRCCTRL( *this, "m_groundZ",	  wxTextCtrl );

	m_octreeDensity = XRCCTRL( *this, "m_octreeDensity", wxTextCtrl );

	m_buildOctree = XRCCTRL( *this, "m_buildOctree", wxButton );
	m_bakeAO = XRCCTRL( *this, "m_bakeAO", wxButton );

	m_progressAO = XRCCTRL( *this, "m_progressAO", wxGauge );

	m_mergeChunks = XRCCTRL( *this, "MergeChunks", wxButton );
	m_autoMergeChunks = XRCCTRL( *this, "AutoMergeChunks", wxButton );
	m_removeChunk = XRCCTRL( *this, "RemoveChunk", wxButton );
	m_removeSkinning = XRCCTRL( *this, "RemoveSkinning", wxButton );
	m_chunkMaterials = XRCCTRL( *this, "ChunkMaterial", wxChoice );

// 	m_vertexPaintLod = XRCCTRL( *this, "m_VPLod", wxSpinCtrl );
// 	m_vertexPaintLod->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxCommandEventHandler( CEdMeshEditor::OnVertexPaintLodSelected ), NULL, this );
// 	m_vertexPaintLod->SetMin( 0 );
// 	m_vertexPaintLod->SetMax( mesh->GetNumLODLevels() - 1 );
	
	m_displayedLodChoice = XRCCTRL( *this, "LODChoice", wxChoice );

	m_boundControls = new CEdBindingsBuilder();
	
	// Analyze textures
	Uint32 textureArrayDataSize = 0;
	Uint32 textureDataSize = 0;
	Uint32 meshDataSize = 0;
	
	TDynArray< MeshTextureInfo* > usedTextures;
	TDynArray< SMeshTextureArrayInfo* > usedTextureArrays;

	CEntity* e = m_preview->GetEntity();
	if(e)
	{
		TDynArray< CComponent* > cmp = e->GetComponents();
		for(Uint32 i=0; i< cmp.Size(); i++)
		{
			 CMeshComponent* cmc = Cast< CMeshComponent > ( cmp[i] );

			if( cmc )
			{
				// generate approx. resource memory alloc size
				CMesh* cmesh = cmc->GetMeshNow();

				if(cmesh)
				{
					// Aenerate approx. resource memory alloc size
					for ( Uint32 i=0; i<cmesh->GetNumLODLevels(); i++ )
					{
						meshDataSize += MeshStatsNamespace::CalcMeshLodRenderDataSize( cmesh, i );
					}			

					// Gather used texture arrays
					TDynArray< SMeshTextureArrayInfo* > usedChunkTextureArrays;
					MeshStatsNamespace::GatherTextureArraysUsedByMesh( cmesh, usedChunkTextureArrays );

					// Fill the global mesh texture arrays
					if( usedTextureArrays.Empty() == true )
					{
						usedTextureArrays = usedChunkTextureArrays;
					}
					else
					{
						for( Uint32 j=0; j<usedChunkTextureArrays.Size(); ++j )
						{
							for( Uint32 k=0; k<usedTextureArrays.Size(); ++k )
							{
								if( SMeshTextureArrayInfo::CmpFuncByDepotPath( usedTextureArrays[k], usedChunkTextureArrays[j] ) != 0 )
								{
									usedTextureArrays.PushBack( usedChunkTextureArrays[j] );
								}
							}
						}
					}
					usedChunkTextureArrays.Clear();

					// Gather used textures
					TDynArray< MeshTextureInfo* > usedChunkTextures;
					MeshStatsNamespace::GatherTexturesUsedByMesh( cmesh, usedChunkTextures );

					// Fill the global mesh textures
					if( usedTextures.Empty() ) 
					{
						usedTextures = usedChunkTextures;						
					}
					else
					{	
						for ( Uint32 j=0; j<usedChunkTextures.Size(); j++ )
						{
							for ( Uint32 i=0; i<usedTextures.Size(); i++ )
							{	
								// new texture found
								if( MeshTextureInfo::CmpFuncByDepotPath( &usedTextures[i], &usedChunkTextures[j] ) != 0 )								
								{
									usedTextures.PushBack( usedChunkTextures[j] );
									break;
								}
							}							
						}

					}
					usedChunkTextures.Clear();
				}				
			}
		}

		if( !usedTextures.Empty() )
		{
			for ( Uint32 i=0; i<usedTextures.Size(); i++ )
			{
				// Accumulate shit
				MeshTextureInfo* texInfo = usedTextures[i];
				textureDataSize += texInfo->m_dataSize;
			}
		}

		// calculate data size for all gathered texture arrays
		if( usedTextureArrays.Empty() == false )
		{
			for( Uint32 i=0; i<usedTextureArrays.Size(); ++i )
			{
				const CTextureArray* textureArray = usedTextureArrays[i]->m_textureArray.Get();

				TDynArray< CBitmapTexture* > arrayTextures;
				textureArray->GetTextures( arrayTextures );
				const Uint32 textureCount = arrayTextures.Size();

				for( Uint32 j=0; j<textureCount; ++j )
				{
					textureArrayDataSize += MeshStatsNamespace::CalcTextureDataSize( arrayTextures[j] );
				}
			}
		}
	}

	m_preview->m_textureArraysDataSize = wxString( "Texture arrays data: " ) + MeshStatsNamespace::MemSizeToText( textureArrayDataSize ) + wxString( " MB" );
	m_preview->m_textureDataSize = wxString( "Texture data: " ) + MeshStatsNamespace::MemSizeToText( textureDataSize ) + wxString( " MB" );
	m_preview->m_meshDataSize = wxString( "Mesh data: " ) + MeshStatsNamespace::MemSizeToText( meshDataSize ) + wxString( " MB" );
	
	usedTextures.ClearPtr();
	usedTextureArrays.ClearPtr();

#ifndef NO_DATA_VALIDATION
	if( GDataError != nullptr )
	{	
		TDynArray< CResource::DependencyInfo > dependencies;
		m_mesh->GetDependentResourcesPaths( dependencies, TDynArray< String >() );
		for ( const CResource::DependencyInfo& info : dependencies )
		{
			m_dependentResourcesPaths.PushBack( info.m_path );
		}

		TSortedArray< SDataError > arrayForErrors;
		GDataError->GetCurrentCatchedFilteredForResource( arrayForErrors, m_dependentResourcesPaths );

		for( auto it = arrayForErrors.Begin(); it != arrayForErrors.End(); ++it )
		{
			const SDataError& error = ( *it );
			m_dataErrors.PushBackUnique( error );
		}
		m_dataErrors.Sort();
		// Connect to Data error reporter
		if( GDataError != nullptr )
		{
			GDataError->RegisterListener( this );
		}
	}
#endif

	UpdateMaterialList();
	UpdateDisplayedLODChoice();
	m_displayedLodChoice->SetSelection( 0 );

	SetupPages();
	// Update and finalize layout
	Layout();
	LoadOptionsFromConfig();

	m_pages->SetSelection(0);
	Show();

	SEvents::GetInstance().RegisterListener( CNAME( EditorTick ), this );
}

CEdMeshEditor::~CEdMeshEditor()
{
#ifndef NO_DATA_VALIDATION
	// Disconnect to Data error reporter
	if( GDataError != nullptr )
	{
		GDataError->UnregisterListener( this );
	}
#endif

	// Save local options
	SaveOptionsToConfig();

	delete m_boundControls;

	// Undo any local changes made to the apex resource.
	if ( m_mesh->IsA< CApexResource >() )
	{
		SafeCast< CApexResource >( m_mesh )->RevertPreviewAsset();
	}

	// Unbind reference
	m_mesh->RemoveFromRootSet();

	if ( m_octree )
	{
		delete m_octree;
	}

	delete m_physicalRepresentation;

	delete m_soundInfo;

	// Unregister event listeners
	SEvents::GetInstance().UnregisterListener( this );

	{
		m_undoManager->RemoveFromRootSet();
		m_undoManager->Discard();
		m_undoManager = NULL;
	}
}


void CEdMeshEditor::SetupPages()
{
	m_boundControls->Clear();

	// Based on the mesh type, we can remove pages that are not useful. Getting the mesh type from the preview is a bit ugly, but it works.
	switch ( m_preview->GetMeshType() )
	{
	case MTPT_Mesh:
		{
			RemovePage( MEP_BakeAO );
			RemovePage( MEP_Cloth );
			RemovePage( MEP_Destructible );

			CMesh* theMesh = SafeCast< CMesh >( m_mesh );
			// Setup LOD list
			if ( m_lodList ) delete m_lodList;
			m_lodList = new CEdMeshLODList( this, theMesh, m_undoManager );
			m_lodList->Bind( wxEVT_COMMAND_TREE_SEL_CHANGED, &CEdMeshEditor::OnLODSelected, this );

			if ( m_physicalRepresentation ) delete m_physicalRepresentation;
			m_physicalRepresentation = new CEdMeshPhysicalRepresentation( this, m_preview, theMesh );

			if( m_soundInfo )
			{
				delete m_soundInfo;
			}
			m_soundInfo = new CEdMeshSoundInfo( this, m_preview, theMesh );
		}
		break;

	case MTPT_Destruction:
		{
			RemovePage( MEP_LODs );
			RemovePage( MEP_BakeAO );
			RemovePage( MEP_VertexPaint );
			RemovePage( MEP_PhysicalRepresentation );
			RemovePage( MEP_Cloth );
			RemovePage( MEP_SoundInfo );

			m_materialList->EnableListModification( false );

			CApexDestructionResource* apexRes = SafeCast< CApexDestructionResource >( m_mesh );

			m_boundControls->SetCallback( &OnApexPropertyChangedCallback, this );

			m_boundControls->Begin( XRCCTRL( *this, "DestructiblePanel", wxPanel ) );

			// Special handler for Max Depth so we can update chunk material list.
			m_boundControls->SetCallback( &OnApexDestMaxDepthChangedCallback, this );
			m_boundControls->AddNumericTextMinU32( "Max Depth", 0, &apexRes->m_maxDepth, "The maximum chunk depth. This is 0 for an unfractured mesh." );
			m_boundControls->SetCallback( &OnApexPropertyChangedCallback, this );

			m_boundControls->AddNumericTextMinU32( "Original Max Depth", 0, &apexRes->m_originalMaxDepth, "The maximum depth when authored - this way we can tell how far it's been reduced." );
			m_boundControls->AddNumericTextMinU32( "Support Depth", 0, &apexRes->m_supportDepth, "The chunk hierarchy depth at which to create a support graph. Higher depth levels give more detailed support, but will give a higher computational load. Chunks below the support depth will never be supported." );

			m_boundControls->NewColumn();

			m_boundControls->AddNumericTextMinFloat( "Neighbor Padding", 0.0f, &apexRes->m_neighborPadding, "Padding used for chunk neighbor tests. This padding is relative to the largest diagonal of the asset's local bounding box. This value must be non-negative. Default value = 0.001f." );
			m_boundControls->AddCheckbox( "Form Extended Structures", &apexRes->m_formExtendedStructures, "If initially static, the destructible will become part of an extended support structure if it is in contact with another static destructible that also has this flag set." );
			m_boundControls->AddCheckbox( "Use Asset Defined Support", &apexRes->m_useAssetSupport, "If set, then chunks which are tagged as 'support' chunks (via NxDestructibleChunkDesc::isSupportChunk) will have environmental support in static destructibles.\n\nNote: if both ASSET_DEFINED_SUPPORT and WORLD_SUPPORT are set, then chunks must be tagged as 'support' chunks AND overlap the NxScene's static geometry in order to be environmentally supported." );
			m_boundControls->AddCheckbox( "Use World Support", &apexRes->m_useWorldSupport, "If set, then chunks which overlap the NxScene's static geometry will have environmental support in static destructibles.\n\nNote: if both ASSET_DEFINED_SUPPORT and WORLD_SUPPORT are set, then chunks must be tagged as 'support' chunks AND overlap the NxScene's static geometry in order to be environmentally supported." );

			m_boundControls->AddSeparator();

			m_boundControls->AddNumericTextU32( "Initial Allowance for Instancing", &apexRes->m_initialAllowance, "Initial actor estimate for instance buffer allocation. Used for setting instance buffer sizes." );

			m_boundControls->End();
		}
		break;

	case MTPT_Cloth:
		{
			RemovePage( MEP_BakeAO );
			RemovePage( MEP_PhysicalRepresentation );
			RemovePage( MEP_VertexPaint );
			RemovePage( MEP_Destructible );
			RemovePage( MEP_SoundInfo );

			m_materialList->EnableListModification( false );

			CApexClothResource* apexRes = SafeCast< CApexClothResource >( m_mesh );
			apexRes->CreateDefaults();

			// Setup LOD list
			if ( m_lodList ) delete m_lodList;
			m_lodList = new CEdMeshLODList( this, apexRes, m_undoManager );
			m_lodList->Bind( wxEVT_COMMAND_TREE_SEL_CHANGED, &CEdMeshEditor::OnLODSelected, this );


			m_boundControls->SetCallback( &OnApexPropertyChangedCallback, this );

			m_boundControls->Begin( XRCCTRL( *this, "ClothSimulationPanel", wxPanel ) );

			m_boundControls->AddNumericTextMinFloat( "Collision Thickness", 0.0f, &apexRes->m_simThickness, "Minimal amount of separation between cloth particles and collision volumes.\n\nEach cloth particle will minimally have as much distance to any collision volume. Most stable when this value corresponds roughly half the average edge length. Can be increased to prevent penetration artifacts." );
			m_boundControls->AddCheckboxInverted( "CCD Enabled", &apexRes->m_simDisableCCD, "Turn on/off CCD when colliding cloth particles with collision volumes\n\nWhen turning off CCD, cloth particles can tunnel through fast moving collision volumes. But sometimes ill turning collision volumes can excert large velocities on particles. This can help prevent it." );
			m_boundControls->NewColumn();
			m_boundControls->AddSlider( "Virtual Particle Density", 0.0f, 3.0f, 0.01f, &apexRes->m_simVirtualParticleDensity, "Select the amount of virtual particles generated for each triangle." );

			m_boundControls->End();

			m_boundControls->Begin( XRCCTRL( *this, "ClothMaterialPanel", wxPanel ) );

			//UI setup as in DCC plugin
			m_boundControls->AddNumericTextFloat( "Gravity Scale", &apexRes->m_mtlGravityScale, "Amount of gravity that is applied to the cloth.\n\nA value of 0 will make the cloth ignore gravity, a value of 10 will apply 10 times the gravity." );
			m_boundControls->AddSlider( "Friction", 0.0f, 1.0f, 0.01f, &apexRes->m_mtlFriction, "Friction coefficient in the range [0, 1]" );
			m_boundControls->AddSlider( "Bending Stiffness", 0.0f, 1.0f, 0.01f, &apexRes->m_mtlBendingStiffness, "Bending stiffness of the cloth in the range [0, 1]." );
			m_boundControls->AddSlider( "Shearing Stiffness", 0.0f, 1.0f, 0.01f, &apexRes->m_mtlShearingStiffness, "Shearing stiffness of the cloth in the range [0, 1]." );
			m_boundControls->AddSlider( "Tether Stiffness (1-Relax)", 0.0f, 1.0f, 0.01f, &apexRes->m_mtlTetherStiffness, "Range [0, 1], but should be 0. The higher this value, the more the piece of clothing is allowed to stretch." );
			m_boundControls->AddSlider( "Tether Limit (1+Stretch Limit)", 1.0f, 4.0f, 0.01f, &apexRes->m_mtlTetherLimit, "Range [1, 4], but should be 1. This scales the restlength of the tether constraints." );
			m_boundControls->NewColumn();
			m_boundControls->AddSlider( "Damping", 0.0f, 1.0f, 0.01f, &apexRes->m_mtlDamping, "Spring damping of the cloth in the range [0, 1]" );
			m_boundControls->AddSlider( "Drag", 0.0f, 1.0f, 0.01f, &apexRes->m_mtlDrag, "Drag coefficient in the range [0, 1]\n\nThe drag coefficient is the portion of local frame velocity that is applied to each particle." );
			m_boundControls->AddSlider( "Inertia Scale", 0.0f, 1.0f, 0.01f, &apexRes->m_mtlInertiaScale, "Amount of inertia that is kept when using local space simulation.\n\nA value of 0 will make the cloth move in global space without inertia, a value of 1 will keep all inertia." );

			m_boundControls->AddSeparator();
			
			m_boundControls->AddSlider( "Mass Scale", 1.0f, 100.0f, 1.01f, &apexRes->m_mtlMassScale, "Controls the amount of mass scaling during collision [1, 100]" );
			m_boundControls->AddSlider( "Max Distance Bias", -1.0f, 1.0f, 0.01f, &apexRes->m_mtlMaxDistanceBias, "Deform the max distance sphere into a capsule or a disc.\n\nA value smaller than 0 will turn the sphere into a capsule and eventually a line (at value -1) along the normal of the vertex. A value bigger than 0 will turn the sphere into a disc." );

			//expose phys mesh stat value
			m_boundControls->AddSeparator();
			m_boundControls->AddLabel( wxString::Format( "Average Edge Length: %f", apexRes->GetAverageEdgeLength() ), "Statistics only.\nFor use with Self Collision Thickness." );

			m_boundControls->AddNumericTextMinFloat( "Self Collision Thickness", 0.0f, &apexRes->m_mtlSelfcollisionThickness, "If you are using self collision this value should be greater than average edge length.\n\nMinimal amount of distance particles will keep of each other.\n\nThis feature prevents meshes from self-intersecting. Only works properly when configured properly." );
			m_boundControls->AddNumericTextMinFloat( "Self Collision Stiffness", 0.0f, &apexRes->m_mtlSelfcollisionStiffness, "If you are using self collision this value should be set to 1.\n\nStiffness of self collision solver.\n\nThis feature prevents meshes from self-intersecting. Only works properly when configured properly." );
			m_boundControls->NewColumn();
			m_boundControls->AddNumericTextMinFloat( "Hard Stretch Limitation", 0.0f, &apexRes->m_mtlHardStretchLimitation, "Make cloth simulation less stretchy. A value smaller than 1 will turn it off.\n\nGood values are usually between 1 and 1.1. Any value >= 1 will guarantee that a certain set of edges is not longer than that value times the initial rest length." );
			m_boundControls->AddCheckbox( "CoM Damping", &apexRes->m_mtlComDamping, "Enable/disable center of mass damping of internal velocities. \n\nIf set, the global rigid body modes (translation and rotation) are extracted from damping. This way, the cloth can freely move and rotate even under high damping." );
			
			m_boundControls->End();

			m_boundControls->SetCallback( &OnPreviewWindChangedCallback, this );

			m_boundControls->Begin( XRCCTRL( *this, "ClothPreviewPanel", wxPanel ) );

			// Wind Yaw's slider range is reversed, so that moving right will rotate the window CW (looking from above), which feels a bit more natural.
			m_boundControls->AddLiveSlider( "Wind Direction", "Yaw", 360, 0, -1, &m_windDirection.Yaw, "Yaw angle -- turns the wind North, Sound, East, West, etc." );
			m_boundControls->AddLiveSlider( "", "Pitch", -90, 90, 1, &m_windDirection.Pitch, "Pitch angle -- positive turns the wind to blow upward, negative turns it downward" );
			m_boundControls->NewColumn();
			m_boundControls->AddNumericTextMinFloat( "Wind Strength", 0.0f, &m_windStrength, "Strength of the wind. 0 for no wind" );
			m_boundControls->AddLiveSlider( "", "Scale", 0, 1, 0.01f, &m_windStrengthScale, "Multiplier applied to the wind strength" );

			m_boundControls->End();

			wxChoice* presetList = XRCCTRL( *this, "clothPresetList", wxChoice );
			// Disconnect from the preset list's event. We may have been previously connected if we are refreshing after a reload/reimport.
			presetList->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdMeshEditor::OnClothPresetChoice ), NULL, this );

			CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
			String lastPresetPath = config.Read( TXT("/Frames/MeshEditor/ClothPresetsPath"), String::EMPTY );
			LoadClothPresets( lastPresetPath );

			// Select this mesh's last-known preset in the preset list.
			Int32 idx = presetList->FindString( apexRes->m_materialPresetName.AsChar() );

			// If the name was not found, select item 0 (Resource Defaults)
			if ( idx == wxNOT_FOUND ) idx = 0;

			presetList->Select( idx );

			// Reconnect to the preset list's event, so we'll get notified of any further changes.
			presetList->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdMeshEditor::OnClothPresetChoice ), NULL, this );
		}
		break;
	
	case MTPT_Fur:
		{
			RemovePage( MEP_LODs );
			RemovePage( MEP_BakeAO );
			RemovePage( MEP_VertexPaint );
			RemovePage( MEP_PhysicalRepresentation );
			RemovePage( MEP_Cloth );
			RemovePage( MEP_Destructible );
			RemovePage( MEP_Materials );
			RemovePage( MEP_SoundInfo );
		}
		break;
	case MTPT_PhysxDestruction:
		{
			RemovePage( MEP_BakeAO );
			RemovePage( MEP_Cloth );
			RemovePage( MEP_Destructible );
			RemovePage( MEP_SoundInfo );

			CPhysicsDestructionResource* theMesh = SafeCast< CPhysicsDestructionResource >( m_mesh );
			// Setup LOD list
			if ( m_lodList ) delete m_lodList;
			m_lodList = new CEdMeshLODList( this, theMesh, m_undoManager );
			m_lodList->Bind( wxEVT_COMMAND_TREE_SEL_CHANGED, &CEdMeshEditor::OnLODSelected, this );

			if ( m_physicalRepresentation ) delete m_physicalRepresentation;
			m_physicalRepresentation = new CEdMeshPhysicalRepresentation( this, m_preview, theMesh );

			if ( m_destrPhysRepresentation ) delete m_destrPhysRepresentation;
			m_destrPhysRepresentation = new CEdDestructionPhysicalRepresentation( this, m_preview, theMesh, m_undoManager );
		}
		break;

	default:
		{
			RemovePage( MEP_LODs );
			RemovePage( MEP_BakeAO );
			RemovePage( MEP_VertexPaint );
			RemovePage( MEP_PhysicalRepresentation );
			RemovePage( MEP_SoundInfo );
			RemovePage( MEP_Cloth );
			RemovePage( MEP_Destructible );
			m_materialList->EnableListModification( false );
			m_materialList->EnableMaterialRemapping( false );
		}
	}
}

void CEdMeshEditor::RefreshDestructMaterials()
{
	CApexDestructionResource* apexRes = SafeCast< CApexDestructionResource >( m_mesh );
	ASSERT( apexRes, TXT("RefreshDestructMaterials() without a CApexDestructionResource?") );
	if ( !apexRes ) return;

	// Material name panel
	wxPanel* panel = XRCCTRL( *this, "DestructibleChunkMtlPanel", wxPanel );
	ASSERT( panel, TXT("Could not find DestructibleChunkMtlPanel") );
	if ( !panel ) return;

	wxFlexGridSizer* grid = wxStaticCast( panel->GetSizer(), wxFlexGridSizer );
	ASSERT( grid, TXT("Could not find DestructibleChunkMtlSizer") );
	if ( !grid ) return;

	panel->Freeze();

	panel->DestroyChildren();
	grid->Clear();

	TDynArray< CName > mtlNames;
	GPhysicEngine->GetPhysicalMaterialNames( mtlNames );
	wxArrayString mtlNameStrs;
	for ( Uint32 i = 0; i < mtlNames.Size(); ++i )
	{
		mtlNameStrs.Add( mtlNames[i].AsString().AsChar() );
	}

	// Add column headers
	grid->Add( new wxStaticText( panel, wxID_ANY, "Chunk Depth" ), 0, wxEXPAND );
	grid->Add( new wxStaticText( panel, wxID_ANY, "Material" ), 0, wxEXPAND );
	grid->Add( new wxStaticText( panel, wxID_ANY, "Density scaler" ), 0, wxEXPAND );
	grid->Add( new wxStaticText( panel, wxID_ANY, "Result mass" ), 0, wxEXPAND );

	Float unfractured = 1.0f;
	Float fractured = 1.0f;
	apexRes->GetDensities( unfractured, fractured );

	Float assetMass = 0.0f;
	
#ifdef USE_APEX
	if( m_preview->GetMeshPreviewComponent() && Cast< CMeshTypePreviewDestructionComponent >( m_preview->GetMeshPreviewComponent() ) )
	{
		if( CDestructionSystemComponent* component = Cast< CDestructionSystemComponent >( Cast< CMeshTypePreviewDestructionComponent >( m_preview->GetMeshPreviewComponent() )->GetDestructionSystemComponent() ) )
		{
			if( CApexDestructionWrapper* wrapper = component->GetDestructionBodyWrapper() )
			{
				void* actor = wrapper->GetActor( 0 );
				if( !actor )
				{
					RunLaterOnce( [ this ]() {
						RefreshDestructMaterials();
					} );
				}
			}
		}
	}
#endif

	Uint8 max = apexRes->m_maxDepth;
	if( max > 1 ) max = 1;
	// Add label and choice box for each chunk depth.
	for ( Uint32 i = 0; i <= max; ++i )
	{
		wxStaticText* numLbl = new wxStaticText( panel, wxID_ANY, wxString::Format( "%d:", i ) );
		grid->Add( numLbl, wxSizerFlags().Right() );

		CEdChoice* mtlChoice = new CEdChoice( panel, wxDefaultPosition, wxDefaultSize );
		grid->Add( mtlChoice, wxSizerFlags().Right() );
		mtlChoice->SetId( i );
		mtlChoice->Append( mtlNameStrs );

		Int32 selectIdx = mtlChoice->FindString( apexRes->GetMaterialForChunkDepth( i ).AsString().AsChar() );
		if ( selectIdx < 0 ) selectIdx = 0;

		mtlChoice->Select( selectIdx );

		mtlChoice->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( CEdMeshEditor::OnApexDestructibleMtlSelected ), NULL, this );

		CEdTextControl* densityChoice = new CEdTextControl( panel, wxID_ANY );
		grid->Add( densityChoice, wxEXPAND );
		densityChoice->SetId( i );
		i == 0 ? densityChoice->SetValue( ToString( unfractured ).AsChar() ) : densityChoice->SetValue( ToString( fractured ).AsChar() );
		densityChoice->Connect( wxEVT_KILL_FOCUS, wxCommandEventHandler( CEdMeshEditor::OnApexDestructibleDensity ), NULL, this );

		Float mass = ( i == 0 ? unfractured : fractured );
		const SPhysicalMaterial* physicalMaterial = GPhysicEngine->GetMaterial( apexRes->GetMaterialForChunkDepth( i ) );
		if( physicalMaterial )
		{
			mass *= physicalMaterial->m_density;
		}

#ifdef USE_APEX
		if( m_preview->GetMeshPreviewComponent() && Cast< CMeshTypePreviewDestructionComponent >( m_preview->GetMeshPreviewComponent() ) )
		{
			if( CDestructionSystemComponent* component = Cast< CDestructionSystemComponent >( Cast< CMeshTypePreviewDestructionComponent >( m_preview->GetMeshPreviewComponent() )->GetDestructionSystemComponent() ) )
			{
				if( CApexDestructionWrapper* wrapper = component->GetDestructionBodyWrapper() )
				{
					if( i == 0 )
					{
						mass = wrapper->GetMass( 0 );
					}
					else
					{
						Uint32 actorsCount = wrapper->GetActorsCount();
						for( Uint32 i = 1; i < actorsCount; i++ )
						{
							mass = wrapper->GetMass( i );
						}

					}
				}
			}
		}
#endif
		if( i > 0 ) continue;
		wxStaticText* massChoice = new wxStaticText( panel, wxID_ANY, ToString( mass ).AsChar() );

		grid->Add( massChoice, wxEXPAND );
	}

	// Propagate Layout() up through parent hierarchy. While not very nice, this makes sure everything is sized correctly with our new material list.
	wxWindow* wnd = panel;
	while ( wnd )
	{
		wnd->Layout();
		wnd = wnd->GetParent();
	}

	panel->Thaw();
}

void CEdMeshEditor::RefreshPreviewRenderingProxy()
{
	m_preview->GetMeshPreviewComponent()->GetDrawableComponent()->RefreshRenderProxies();
}

void CEdMeshEditor::RefreshPreviewBBoxInfo()
{
	m_preview->UpdateBBoxInfo();
}

void CEdMeshEditor::OnApexDestructibleMtlSelected( wxCommandEvent& event )
{
	CApexDestructionResource* apexRes = SafeCast< CApexDestructionResource >( m_mesh );
	if ( !apexRes ) return;

	CEdChoice* mtlChoice = wxStaticCast( event.GetEventObject(), CEdChoice );
	if ( !mtlChoice ) return;
	
	Uint32 depth = ( Uint32 )mtlChoice->GetId();

	apexRes->SetMaterialForChunkDepth( depth, CName( ((wxItemContainerImmutable*)mtlChoice)->GetStringSelection().wc_str() ) );

	if ( !m_freezeApexUpdates )
	{
		SafeCast< CApexResource >( m_mesh )->RebuildPreviewAsset();
		m_preview->Reload();
	}
}

void CEdMeshEditor::OnApexDestructibleMtlFill( wxCommandEvent& event )
{
	CApexDestructionResource* apexRes = SafeCast< CApexDestructionResource >( m_mesh );
	if ( !apexRes ) return;

	m_freezeApexUpdates = true;

	CName firstMaterial = apexRes->GetMaterialForChunkDepth( 0 );
	Float densFrac, densUnfrac;
	apexRes->GetDensities( densUnfrac, densFrac );
	for ( Uint32 i = 1; i <= apexRes->m_maxDepth; ++i )
	{
		apexRes->SetMaterialForChunkDepth( i, firstMaterial );
	}
	apexRes->SetDensityFractured( densUnfrac );

	m_freezeApexUpdates = false;

	RunLaterOnce( [ this ]() {
		RefreshDestructMaterials();
	} );

	SafeCast< CApexResource >( m_mesh )->RebuildPreviewAsset();
	m_preview->Reload();
}

void CEdMeshEditor::OnApexDestructibleDensity( wxCommandEvent& event )
{
	CApexDestructionResource* apexRes = SafeCast< CApexDestructionResource >( m_mesh );
	if ( !apexRes ) return;

	CEdTextControl* textControl = ( CEdTextControl* ) event.GetEventObject();
	if ( !textControl ) return;

	m_freezeApexUpdates = true;

	wxString string = textControl->GetValue();
	Double value = 1.0f;
	string.ToCDouble( &value );

	Uint32 depth = ( Uint32 )textControl->GetId();
	if( depth )
	{
		apexRes->SetDensityFractured( value );
	}
	else
	{
		apexRes->SetDensityUnfractured( value );
	}
	m_freezeApexUpdates = false;

	RunLaterOnce( [ this ]() {
		RefreshDestructMaterials();
	} );

	SafeCast< CApexResource >( m_mesh )->RebuildPreviewAsset();
	m_preview->Reload();

}

void CEdMeshEditor::OnApexPropertyChanged( CBoundControl* boundControl )
{
	if ( !m_freezeApexUpdates )
	{
		SafeCast< CApexResource >( m_mesh )->RebuildPreviewAsset();
		m_preview->Reload();
	}
}

void CEdMeshEditor::OnApexDestMaxDepthChanged( CBoundControl* boundControl )
{
	OnApexPropertyChanged( boundControl );
	if ( !m_freezeApexUpdates )
	{
		RefreshDestructMaterials();
	}
}


void CEdMeshEditor::OnPreviewWindChanged( CBoundControl* boundControl )
{
	Vector windDir;
	m_windDirection.ToAngleVectors( &windDir, NULL, NULL );

	// Since the Mesh Editor does not modify the default world, we know we can cast it to CEdPreviewWorld.
	// The method unfortunatelly can be called so early in the initialization of this window that the m_preview can be NULL
	if ( NULL != m_preview )
	{
		SafeCast< CEdPreviewWorld >( m_preview->GetPreviewWorld() )->SetWind( windDir, m_windStrength * m_windStrengthScale );
		m_preview->SetWindIndicatorScale( m_windStrengthScale );
	}
}

void CEdMeshEditor::RefreshBoundControls()
{
	// Freeze updates on the apex resource, otherwise we will create many new copies here (one for each updated control).
	m_freezeApexUpdates = true;

	m_boundControls->RefreshControls();

	// Unfreeze updates so the controls will affect the resource again.
	m_freezeApexUpdates = false;
}


void CEdMeshEditor::OnApexRestoreDefaults( wxCommandEvent& event )
{
	if ( !m_mesh->IsA< CApexResource >() )
	{
		return;
	}

	
	CApexResource* apexRes = SafeCast< CApexResource >( m_mesh );
	apexRes->RestoreDefaults();
	apexRes->RebuildPreviewAsset();

	m_preview->Reload();

	if ( apexRes->IsA< CApexClothResource >() )
	{
		wxChoice* presetList = XRCCTRL( *this, "clothPresetList", wxChoice );
		ASSERT( presetList != NULL, TXT("Restoring cloth resource, but no preset list?") );
		if ( presetList )
		{
			// Select the "Defaults" entry in the preset list
			if ( presetList->GetSelection() > 0 )
			{
				// Temporarily disconnect from the preset list, or else we'll just end up getting called again in response to
				// changing the preset selection.
				presetList->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdMeshEditor::OnClothPresetChoice ), NULL, this );
				presetList->Select( 0 );
				presetList->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdMeshEditor::OnClothPresetChoice ), NULL, this );
			}
		}
	}

	RefreshBoundControls();
}


void CEdMeshEditor::OnApexDestructibleReset( wxCommandEvent& event )
{
	m_preview->Reload();
}



void CEdMeshEditor::OnSave( wxCommandEvent& event )
{
	if ( m_mesh->IsA< CApexClothResource >() )
	{
		String newPresetName = String::EMPTY;

		wxChoice* presetList = XRCCTRL( *this, "clothPresetList", wxChoice );
		int selIdx = presetList->GetSelection();
		if ( selIdx > 0 )
		{
			wxString selStr = presetList->GetString( selIdx );
			SClothMaterialPreset* preset = m_clothPresets.FindPtr( selStr.wc_str() );
			if ( preset )
			{
				newPresetName = preset->m_name;
			}
		}

		CApexClothResource* cloth = Cast< CApexClothResource >( m_mesh );
		cloth->m_materialPresetName = newPresetName;
	}
	else if ( m_mesh->IsA< CApexDestructionResource >() )
	{
		// Verify materials, try to avoid saving destructibles that don't have proper material settings.
		if ( !Cast< CApexDestructionResource >( m_mesh )->VerifyChunkMaterials() )
		{
			if ( !YesNo( TXT("This destructible mesh has one or more chunk depths set to use the default physical material. You should fix this. Do you want to save anyways?") ) )
			{
				return;
			}
		}
	}


	// Save mesh
	if ( m_mesh->GetFile() && m_mesh->GetFile()->IsLoaded() )
	{
		m_mesh->Save();
	}
}

void CEdMeshEditor::OnUndo( wxCommandEvent& event )
{
	m_undoManager->Undo();
}

void CEdMeshEditor::OnRedo( wxCommandEvent& event )
{
	m_undoManager->Redo();
}

void CEdMeshEditor::OnUseGroundAO( wxCommandEvent& event )
{	
	m_preview->m_showPlaneFloor = m_useGround->IsChecked();
	
	double val2 = 0.0;
	m_groundZ->GetValue().ToDouble( &val2 );
	m_preview->m_debugPlaneZ = (Float)val2;
}

void CEdMeshEditor::OnZoomExtents( wxCommandEvent& event )
{
	Box box = m_mesh->GetBoundingBox();
	m_preview->ShowZoomExtents( box );
}

void CEdMeshEditor::UpdateDisplayedLODChoice()
{
	Int32 sel = m_displayedLodChoice->GetSelection();
	m_displayedLodChoice->Freeze();
	m_displayedLodChoice->Clear();

	m_displayedLodChoice->AppendString( TXT("Auto") );

	for ( Uint32 lodIdx = 0; lodIdx < m_mesh->GetNumLODLevels(); ++lodIdx )
	{
		m_displayedLodChoice->AppendString( wxString::Format( TXT("LOD %d"), lodIdx ) );
	}

	if ( sel < static_cast< Int32 >( m_displayedLodChoice->GetCount() ) )
	{
		m_displayedLodChoice->SetSelection( sel );
	}

	m_displayedLodChoice->Thaw();
}

void CEdMeshEditor::OnDisplayedLODChanged( wxCommandEvent& event )
{
	Int32 sel = m_displayedLodChoice->GetSelection();
	m_preview->OverrideViewLOD( sel-1 ); // if sel=0 then overrides with -1 (auto)
}

#ifdef USE_SIMPLYGON

void CEdMeshEditor::OnAddLOD( wxCommandEvent& event )
{
	if ( CMesh* mesh = Cast< CMesh >( m_mesh ) )
	{
		Int32 selectedLodIdx = m_lodList->GetSelectedLODIndex();

		SLODPresetDefinition definition;
		if ( CEdAddLODDialog( this ).Execute( definition, mesh->GetNumLODLevels(), selectedLodIdx ) )
		{
			Uint32 numOfRemovedChunks;
			String message;
			CUndoMeshChunksChanged::CreateStep( *m_undoManager, mesh, TXT("add LOD") );
			if ( !mesh->GenerateLODWithSimplygon( selectedLodIdx, definition, numOfRemovedChunks, message ) )
			{
				GFeedback->ShowError( message.AsChar() );
			}

			if ( definition.m_removeSkinning )
			{
				mesh->RemoveSkinningFromLOD( mesh->GetNumLODLevels()-1 );
			}

			if ( numOfRemovedChunks != 0 )
			{
				GFeedback->ShowMsg( TXT("Chunks removed"), String::Printf( TXT("%i chunk(s) removed"), numOfRemovedChunks ).AsChar() );
			}

			UpdateLODList( true );
			UpdateDisplayedLODChoice();
			UpdateMeshStats(); // modifying chunks affects the stats
			RefreshPreviewRenderingProxy();
		}
	}
}

void CEdMeshEditor::OnGenerateLODs( wxCommandEvent& event )
{
	if ( CMesh* mesh = Cast< CMesh >( m_mesh ) )
	{
		CEdBatchLodGenerator generator( this, mesh, m_undoManager );

		if ( generator.Execute() )
		{
			UpdateLODList( true );
			UpdateDisplayedLODChoice();
			UpdateMeshStats(); // modifying chunks affects the stats
			RefreshPreviewRenderingProxy();
		}
	}
}

void CEdMeshEditor::OnRemoveLOD( wxCommandEvent& event )
{
	if ( CMesh* mesh = Cast< CMesh >( m_mesh ) )
	{
		if ( mesh->GetNumLODLevels() == 1 )
		{
			GFeedback->ShowError( TXT("Cannot remove the last LOD") );
			return;
		}

		Int32 idx = m_lodList->GetSelectedLODIndex();

		if ( idx >= 0 && YesNo( TXT("Are you sure to remove selected LOD?") ) )
		{
			CUndoMeshChunksChanged::CreateStep( *m_undoManager, mesh, TXT("remove LOD") );
			mesh->RemoveLOD( idx );

			UpdateLODList( false );
			UpdateDisplayedLODChoice();
			UpdateMeshStats();
			RefreshPreviewRenderingProxy();

			wxTreeEvent dummy;
			OnLODSelected( dummy );
		}
	}
}
#endif

Bool CEdMeshEditor::DoMergeChunks( const TDynArray< Uint32 >& chunkIndices, Uint32 materialIndex )
{
	if ( CMesh* mesh = Cast< CMesh >( m_mesh ) )
	{
		String errorStr;
		CUndoMeshChunksChanged::CreateStep( *m_undoManager, mesh, TXT("merge chunks") );
		if ( !mesh->MergeChunks( chunkIndices, materialIndex, &errorStr ) )
		{
			GFeedback->ShowError( errorStr.AsChar() );
			return false;
		}
	}

	UpdateLODList( true );
	RefreshPreviewRenderingProxy();

	return true;
}

void CEdMeshEditor::OnMergeChunksWith( wxCommandEvent& event )
{
	if ( CMesh* mesh = Cast< CMesh >( m_mesh ) )
	{
 		TDynArray< Uint32 > chunkIndices; 
 		m_lodList->GetSelectedChunkIndices( chunkIndices );

		if ( chunkIndices.Empty() )
		{
			return;
		}

		Uint32 matID = static_cast< Uint32 >( event.GetId() );
		DoMergeChunks( chunkIndices, matID );
	}
}

void CEdMeshEditor::OnMergeChunks( wxCommandEvent& event )
{
	if ( CMesh* mesh = Cast< CMesh >( m_mesh ) )
	{
 		TDynArray< Uint32 > chunkIndices; 
 		m_lodList->GetSelectedChunkIndices( chunkIndices );

		if ( chunkIndices.Empty() )
		{
			return;
		}

		const TDynArray< SMeshChunkPacked >& chunks = mesh->GetChunks();

		TDynArray< Uint32 > materialsInSelectedChunks;
		
		for ( auto idxIt = chunkIndices.Begin(); idxIt != chunkIndices.End(); ++idxIt )
		{
			ASSERT ( *idxIt < chunks.Size() );
			Uint32 matId = chunks[ *idxIt ].m_materialID;
			materialsInSelectedChunks.PushBackUnique( matId );
		}

		if ( materialsInSelectedChunks.Size() > 0 )
		{
			const TDynArray< String >& matNames = mesh->GetMaterialNames();

			wxMenu menu( TXT("") );

			for ( auto matIdIt = materialsInSelectedChunks.Begin(); matIdIt != materialsInSelectedChunks.End(); ++matIdIt )
			{
				ASSERT ( *matIdIt < matNames.Size() );
				menu.Append( *matIdIt, matNames[*matIdIt].AsChar() );
				menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdMeshEditor::OnMergeChunksWith, this, *matIdIt );
			}

			m_mergeChunks->PopupMenu( &menu, 0, m_mergeChunks->GetSize().y );
		}
	}
}

Bool CEdMeshEditor::CheckIfChunksShouldBeMerged( Bool showWarning )
{
	if ( CMesh* mesh = Cast< CMesh >( m_mesh ) )
	{
		Int32 chunksToMerge = 0;
		String errorStr;

		for ( Uint32 lodIdx = 0; lodIdx < mesh->GetNumLODLevels(); ++lodIdx )
		{
			chunksToMerge += mesh->MergeChunks( lodIdx, true, &errorStr );
		}

		if ( showWarning && chunksToMerge > 0 )
		{
			return 
				GFeedback->AskYesNo( String::Printf(
						TXT("Found %i chunk(s) using the same shader and texture(s). Should they be merged now?"), chunksToMerge
					).AsChar() );
		}

		return chunksToMerge > 0;	
	}

	return false;
}

void CEdMeshEditor::DoAutoMergeChunks()
{
	if ( CMesh* mesh = Cast< CMesh >( m_mesh ) )
	{
		CUndoMeshChunksChanged::CreateStep( *m_undoManager, mesh, TXT("auto merge chunks") );

		Int32 mergedChunks = 0;
		Int32 affectedLODs = 0;

		{
			CMesh::BatchOperation batch( mesh );
			String errorStr;
			for ( Uint32 lodIdx = 0; lodIdx < mesh->GetNumLODLevels(); ++lodIdx )
			{
				Int32 mergedChunksHere = mesh->MergeChunks( lodIdx, false, &errorStr );

				if ( !errorStr.Empty() )
				{
					GFeedback->ShowError( errorStr.AsChar() );
					continue;
				}

				if ( mergedChunksHere > 0 )
				{
					mergedChunks += mergedChunksHere;
					++affectedLODs;
				}
			}
		}

		if ( mergedChunks == 0 )
		{
			GFeedback->ShowMsg( TXT("Merging result"), TXT("No chunks merged") );
		}
		else
		{
			GFeedback->ShowMsg( TXT("Merging result"), String::Printf( TXT("Merged %u chunks in %u LODs"), mergedChunks, affectedLODs ).AsChar() );
		}

		UpdateLODList( true );
	}
}

void CEdMeshEditor::OnAutoMergeChunks( wxCommandEvent& event )
{
	if ( CMesh* mesh = Cast< CMesh >( m_mesh ) )
	{
		if ( YesNo( TXT("Are you sure to automatically merge chunks with same materials?") ) )
		{
			DoAutoMergeChunks();
		}
	}
}

void CEdMeshEditor::OnRemoveChunk( wxCommandEvent& event )
{
	if ( CMesh* mesh = Cast< CMesh >( m_mesh ) )
	{
 		TDynArray< Uint32 > chunkIndices; 
 		m_lodList->GetSelectedChunkIndices( chunkIndices );

		if ( chunkIndices.Empty() )
		{
			return;
		}

		if ( YesNo( String::Printf( TXT("Are you sure to remove selected chunk%s?"), chunkIndices.Size()>1 ? TXT("s") : TXT("") ).AsChar() ) )
		{
			CUndoMeshChunksChanged::CreateStep( *m_undoManager, mesh, TXT("remove chunk(s)") );

			{
				CMesh::BatchOperation batch( mesh );

				Sort( chunkIndices.Begin(), chunkIndices.End() ); // Just to make sure we will remove chunks in the right order
				for ( Int32 chunkIdxIdx = chunkIndices.SizeInt()-1; chunkIdxIdx >= 0; --chunkIdxIdx )
				{
					mesh->RemoveChunk( chunkIndices[ chunkIdxIdx ] );
				}
			}

			UpdateLODList( true );
			UpdateMeshStats();
			RefreshPreviewRenderingProxy();
		}
	}
}

void CEdMeshEditor::OnRemoveSkinning( wxCommandEvent& event )
{
	if ( CMesh* mesh = Cast< CMesh >( m_mesh ) )
	{
 		TDynArray< Uint32 > chunkIndices; 
 		m_lodList->GetSelectedChunkIndices( chunkIndices );

		if ( chunkIndices.Empty() )
		{
			return;
		}

		if ( YesNo( String::Printf( TXT("Are you sure to remove skinning from selected chunk%s?"), chunkIndices.Size()>1 ? TXT("s") : TXT("") ).AsChar() ) )
		{
			CUndoMeshChunksChanged::CreateStep( *m_undoManager, mesh, TXT("remove skinning") );

			{
				CMesh::BatchOperation batch( mesh );

				for ( Uint32 chunkIdxIdx = 0; chunkIdxIdx < chunkIndices.Size(); ++chunkIdxIdx )
				{
					mesh->RemoveSkinningFromChunk( chunkIndices[ chunkIdxIdx ] );
				}
			}

			UpdateLODList( true );
			RefreshPreviewRenderingProxy();
		}
	}
}

void CEdMeshEditor::OnChunkMaterialChanged( wxCommandEvent& event )
{
	if ( CMesh* mesh = Cast< CMesh >( m_mesh ) )
	{
		TDynArray< Uint32 > selectedChunksIdx;
		m_lodList->GetSelectedChunkIndices( selectedChunksIdx );

		Int32 materialId = m_chunkMaterials->GetSelection();

		if ( materialId >= 0 )
		{
			for ( Uint32 chunkIdx : selectedChunksIdx )
			{
				mesh->SetChunkMaterialId( chunkIdx, materialId );
			}

			UpdateLODList( true );
			UpdateMeshStats();
			RefreshPreviewRenderingProxy();
		}
	}
}

void CEdMeshEditor::OnUpdateUI( wxUpdateUIEvent& event )
{
	m_displayedLodChoice->Enable( !IsPageSelected( MEP_LODs ) ); // On LOD page its the list that controls the displayed LOD

	if ( IsPageSelected( MEP_LODs ) )
	{		
		Int32 lodIdx = m_lodList->GetSelectedLODIndex();

		XRCCTRL( *this, "RemoveLOD", wxButton )->Enable( lodIdx >= 0 );
		XRCCTRL( *this, "ExportLOD", wxButton )->Enable( lodIdx >= 0 );
		XRCCTRL( *this, "ExportFbx", wxButton )->Enable( lodIdx >= 0 );

		TDynArray< Uint32 > chunksIndices;
		m_lodList->GetSelectedChunkIndices( chunksIndices );

		m_mergeChunks->Enable( chunksIndices.Size() > 1 );
		m_removeChunk->Enable( chunksIndices.Size() >= 1 );
		m_chunkMaterials->Enable( chunksIndices.Size() >= 1 );

		if ( CMesh* mesh = Cast< CMesh >( m_mesh ) )
		{
			const TDynArray< SMeshChunkPacked >& chunks = mesh->GetChunks();

			Bool hasSkinning = false;
			for ( auto chunkIdxIt = chunksIndices.Begin(); chunkIdxIt != chunksIndices.End(); ++chunkIdxIt )
			{
				if ( chunks[ *chunkIdxIt ].m_vertexType == MVT_SkinnedMesh || chunks[ *chunkIdxIt ].m_vertexType == MVT_DestructionMesh)
				{
					hasSkinning = true;
				}
			}

			m_removeSkinning->Enable( hasSkinning );
		}

	}
	// else if ( IsPageSelected( ...
}

void CEdMeshEditor::OnVertexPaintEditToggle( wxCommandEvent& event )
{
	if ( !m_mesh->IsA< CMesh >() )
	{
		return;
	}

	if( !m_preview->m_vertexPaintTool )
	{		
		m_preview->m_vertexPaintTool = new CEdVertexPaintTool( m_parent,  SafeCast< CMesh >( m_mesh ) );
		m_preview->m_vertexPaintTool->TogglePaint();
	}
	else
	{
		delete m_preview->m_vertexPaintTool;
		m_preview->m_vertexPaintTool = NULL;
	}
}

namespace
{
	String GetLODMaterialSuffix( Int32 selectedLodIdx )
	{
		return String::Printf( TXT("__lod%u"), selectedLodIdx );
	}
}

void CEdMeshEditor::SaveOptionsToConfig()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	
	SaveLayout( TXT("/Frames/MeshEditor") );

	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/MeshEditor") );

	wxSplitterWindow* splitter = XRCCTRL( *this, "Splitter", wxSplitterWindow );
	config.Write( TXT("Layout/Splitter"), splitter->GetSashPosition() );

	if ( m_preview->GetMeshType() != MTPT_Destruction && m_preview->GetMeshType() != MTPT_Fur )
	{
		wxSplitterWindow* lodSplitter = XRCCTRL( *this, "LODSplitter", wxSplitterWindow );
		config.Write( TXT("Layout/LODSplitter"), lodSplitter->GetSashPosition() );
	}

	if ( m_preview->GetMeshType() != MTPT_Fur )
	{
		wxSplitterWindow* matSplitter = XRCCTRL( *this, "MaterialListSplitter", wxSplitterWindow );
		config.Write( TXT("Layout/MaterialSplitter"), matSplitter->GetSashPosition() );
	}

	// Well, because of the crazy loopback caused by TextureArrayCookingUtils::PrepareTextureArrayBuffer this can be called
	// from within the constructor of CEdMeshPreviewPanel - the m_preview is NULL at that time
	if ( m_preview != nullptr && m_mesh != nullptr )
	{
		config.Write( TXT("Shadows"), m_preview->GetShadowsEnabled() ? 1 : 0 );

		config.Write( TXT("LightPosition"), m_preview->GetLightPosition() );

		Vector      cameraPos = m_preview->GetCameraPosition();
		EulerAngles cameraRot = m_preview->GetCameraRotation();
		config.Write( TXT("CameraX"), cameraPos.X );
		config.Write( TXT("CameraY"), cameraPos.Y );
		config.Write( TXT("CameraZ"), cameraPos.Z );
		config.Write( TXT("CameraPitch"), cameraRot.Pitch );
		config.Write( TXT("CameraRoll"),  cameraRot.Roll );
		config.Write( TXT("CameraYaw"),   cameraRot.Yaw );	
	}

	config.Write( TXT("WindDirectionYaw"),	m_windDirection.Yaw );
	config.Write( TXT("WindDirectionPitch"),	m_windDirection.Pitch );
	config.Write( TXT("WindStrength"),		m_windStrength );
	config.Write( TXT("WindStrengthScale"),	m_windStrengthScale );
}

void CEdMeshEditor::LoadOptionsFromConfig()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/MeshEditor") );

	CEdShortcutsEditor::Load( *this->GetMenuBar(), GetOriginalLabel() );
	// Load layout after the shortcuts (duplicate menu after the shortcuts loading)
	LoadLayout( TXT("/Frames/MeshEditor") );

	wxSplitterWindow *splitter = XRCCTRL( *this, "Splitter", wxSplitterWindow );
	splitter->SetSashPosition( config.Read( TXT("Layout/Splitter"), splitter->GetSashPosition() ) );

	if( m_preview->GetMeshType() != MTPT_Destruction && m_preview->GetMeshType() != MTPT_Fur )
	{
		wxSplitterWindow *lodSplitter = XRCCTRL( *this, "LODSplitter", wxSplitterWindow );
		lodSplitter->SetSashPosition( config.Read( TXT("Layout/LODSplitter"), lodSplitter->GetSashPosition() ) );
	}
	
	if ( m_preview->GetMeshType() != MTPT_Fur )
	{
		wxSplitterWindow* matSplitter = XRCCTRL( *this, "MaterialListSplitter", wxSplitterWindow );
		matSplitter->SetSashPosition( config.Read( TXT("Layout/MaterialSplitter"), matSplitter->GetSashPosition() ) );
	}

	// Well, because of the crazy loopback caused by TextureArrayCookingUtils::PrepareTextureArrayBuffer this can be called
	// from within the constructor of CEdMeshPreviewPanel - the m_preview is NULL at that time
	if ( m_preview != nullptr && m_mesh != nullptr )
	{
		m_preview->SetShadowsEnabled( config.Read( TXT("Shadows"), 1 ) == 1 ? true : false );

		m_preview->SetLightPosition( config.Read( TXT("LightPosition"), 135 ) );

		Float cameraX     = config.Read( TXT("CameraX"), 0.f );
		Float cameraY     = config.Read( TXT("CameraY"), 0.f );
		Float cameraZ     = config.Read( TXT("CameraZ"), 0.f );
		Float cameraPitch = config.Read( TXT("CameraPitch"), -1.f );
		Float cameraRoll  = config.Read( TXT("CameraRoll"), 0.f );
		Float cameraYaw   = config.Read( TXT("CameraYaw"), 0.f );

		if ( cameraPitch == -1.f )
		{ // zoom extends if no camera saved
			wxCommandEvent dummy;
			OnZoomExtents( dummy );
		}
 		else
 		{
 			m_preview->SetCameraPosition( Vector( cameraX, cameraY, cameraZ ) );
 			m_preview->SetCameraRotation( EulerAngles( cameraRoll, cameraPitch, cameraYaw ) );
 		}
	}

	m_windDirection.Yaw		= config.Read( TXT("WindDirectionYaw"), 0.0f );
	m_windDirection.Pitch	= config.Read( TXT("WindDirectionPitch"), 0.0f );
	m_windStrength			= config.Read( TXT("WindStrength"), 0.0f );
	m_windStrengthScale		= config.Read( TXT("WindStrengthScale"), 1.0f );

	OnPreviewWindChanged( NULL );	// NOTE: Relies on the CBoundControl argument not being used by the function.

	RefreshBoundControls();
}

void CEdMeshEditor::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == RED_NAME( EditorPostUndoStep ) )
	{
		if ( GetEventData< CEdUndoManager* >( data ) == m_undoManager )
		{
			UpdateLODList( false );
			UpdateDisplayedLODChoice();
			UpdateMaterialList();
			UpdateChunkMaterialsChoice();
			RefreshPreviewRenderingProxy();
		}
	}
	else if ( name == RED_NAME( FileReloadConfirm ) )
	{
		CResource* res = GetEventData< CResource* >( data );
		if ( res == m_mesh )
		{
			SEvents::GetInstance().QueueEvent( RED_NAME( FileReloadToConfirm ), CreateEventData( CReloadFileInfo(res, NULL, GetTitle().wc_str() ) ) );
		}
	}
	else if ( name == RED_NAME( FileReload ) || name == RED_NAME( FileReimport ) )
	{
		const CReloadFileInfo& reloadInfo = GetEventData< CReloadFileInfo >( data );
		if ( reloadInfo.m_newResource->IsA<CMeshTypeResource>() )
		{
			CMeshTypeResource* oldMesh = (CMeshTypeResource*)(reloadInfo.m_oldResource);
			CMeshTypeResource* newMesh = (CMeshTypeResource*)(reloadInfo.m_newResource);
			if ( oldMesh == m_mesh )
			{
				// Reimport keeps the same object, so we don't need to re-add our rootset reference.
				if ( name == RED_NAME( FileReload ) )
				{
					m_mesh = newMesh;
					m_mesh->AddToRootSet();
				}

				m_undoManager->ClearHistory();

				UpdateLODList( false );
				UpdateDisplayedLODChoice();

				if ( m_materialList )
				{
					m_materialList->Destroy( );
					wxPanel* matPanel = XRCCTRL( *this, "MaterialsPanel", wxPanel );
					m_materialList = new CEdMeshMaterialList( matPanel, this, m_mesh, m_undoManager );
					matPanel->GetSizer()->Add( m_materialList, 1, wxEXPAND );
					UpdateMaterialList();
				}

				UpdateChunkMaterialsChoice();

				m_preview->Reload();

				// Show mesh properties
				m_properties->Get().SetObject( m_mesh );
				Layout();

				wxTheFrame->GetAssetBrowser()->OnEditorReload(m_mesh, this);

				// Re-setup our pages. This will rebuild all bound controls. We can't just use RefreshBoundControls()
				// to update the shown values, since m_mesh may point to a different object now and the bound controls
				// wouldn't be bound to the correct memory location anymore.
				Freeze();
				SetupPages();
				Thaw();
			}
		}
	}
	else if ( RED_NAME( EditorTick ) )
	{
		ValidateResources();

		// unregister for this event - we are interested only in first preview tick
		// as bounding boxes for apexClothWrappers are created at that time
		SEvents::GetInstance().UnregisterListener( RED_NAME( EditorTick ), this );
	}
}

void CEdMeshEditor::OnPropertyModified( wxCommandEvent& event )
{
	const CEdPropertiesPage::SPropertyEventData* propertyEventData = ( (CEdPropertiesPage::SPropertyEventData*)event.GetClientData() );

	CMeshTypePreviewComponent* previewCmp = m_preview->GetMeshPreviewComponent();
	if ( previewCmp == nullptr )
	{
		return;
	}
	EMeshTypePreviewPropertyChangeAction value = previewCmp->OnResourcePropertyModified( propertyEventData->m_propertyName );

	switch( value )
	{
	case MTPPCA_Refresh:
		RunLaterOnce( [this]() { m_preview->Refresh(); } ); break;
	case MTPPCA_Reload:
		RunLaterOnce( [this]() { m_preview->Reload(); } ); break;
	default:
		RED_HALT( "Invalid operation" );
	}
}

void CEdMeshEditor::OnLODSelected( wxTreeEvent& event )
{
	Int32 selectedLOD = m_lodList->GetSelectedLODIndex();
	m_displayedLodChoice->SetSelection( selectedLOD+1 );
	m_preview->OverrideViewLOD( selectedLOD );

	// update selected chunk material choice
	TDynArray< Uint32 > selectedChunksIdx;
	m_lodList->GetSelectedChunkIndices( selectedChunksIdx );

	if ( !selectedChunksIdx.Empty() )
	{
		if ( CMesh* mesh = Cast< CMesh >( m_mesh ) )
		{
			const TDynArray< SMeshChunkPacked >& chunks = mesh->GetChunks();

			Int32 matIdToSelect = chunks[ selectedChunksIdx[0] ].m_materialID;

			for ( Uint32 i = 1; i < selectedChunksIdx.Size(); ++i )
			{
				const auto& chunk = chunks[ selectedChunksIdx[i] ];
				if ( chunk.m_materialID != matIdToSelect )
				{
					matIdToSelect = -1; // different materials in selected chunks
					break;
				}
			}

			m_chunkMaterials->SetSelection( matIdToSelect );
		}
	}
}

void CEdMeshEditor::OnPageChanged( wxNotebookEvent& event )
{
	auto pageIt    = m_pageIndices.FindByValue( event.GetSelection() );
	auto oldPageIt = m_pageIndices.FindByValue( event.GetOldSelection() );

	if ( oldPageIt != m_pageIndices.End() )
	{
		if ( oldPageIt->m_first == MEP_Materials && m_materialList )
		{
			m_materialList->EnableMaterialHighlighting( false );
		}
	}

	if ( pageIt != m_pageIndices.End() )
	{
		if ( pageIt->m_first == MEP_Materials && m_materialList )
		{
			m_materialList->EnableMaterialHighlighting( true );
		}
	
		if ( pageIt->m_first == MEP_LODs )
		{
			if ( m_lodList )
			{
				UpdateChunkMaterialsChoice(); // may get modified on the Materials page
				wxTreeEvent dummy;
				OnLODSelected( dummy ); // update chunk material choice selection
			}

			if ( CheckIfChunksShouldBeMerged( true ) )
			{
				DoAutoMergeChunks();
			}
		}

		// Generate stats on first page change to "stats"
		if ( pageIt->m_first == MEP_Report && !m_statsGenerated )
		{
			m_statsGenerated = true;
			UpdateMeshStatsNow();
		}

		if ( pageIt->m_first == MEP_Destructible )
		{
			RefreshDestructMaterials();
		}
	}
}

void CEdMeshEditor::OnToolRecalculateBoundingBox( wxCommandEvent& event )
{
	if ( m_mesh->MarkModified() )
	{
		m_mesh->CalculateBoundingBox();
	}
}

void CEdMeshEditor::OnShowWireframe( wxCommandEvent& event )
{
	wxToolBar* toolbar = XRCCTRL( *this, "Tools", wxToolBar );
	const Bool status = toolbar->GetToolState( XRCID("toolShowWireframe") );
	m_preview->ShowWireframe( status );
}

void CEdMeshEditor::OnShowBoundingBox( wxCommandEvent& event )
{
	wxToolBar* toolbar = XRCCTRL( *this, "Tools", wxToolBar );
	const Bool status = toolbar->GetToolState( XRCID("toolShowBoundingBox") );
	m_preview->ShowBoundingBox( status );
}

void CEdMeshEditor::OnShowCollision( wxCommandEvent& event )
{
	wxToolBar* toolbar = XRCCTRL( *this, "Tools", wxToolBar );
	const Bool status = toolbar->GetToolState( XRCID("toolShowCollision") );

	// Do not show collision if it's not there
	if ( status )
	{
		if ( m_mesh )
		{
			Bool hasCollision = false;

			hasCollision |= m_mesh->IsA< CMesh >() && SafeCast< CMesh >( m_mesh )->GetCollisionMesh();
			hasCollision |= m_mesh->IsA< CApexDestructionResource >();

			if ( !hasCollision )
			{
				// Disable preview
				toolbar->ToggleTool( XRCID("toolShowCollision"), false );
				m_preview->ShowCollision( false );

				// Inform
				wxMessageBox( wxT("There's no collision to show"), wxT("Error"), wxOK | wxICON_WARNING );
				return;
			}
		}
	}

	// Update preview
	m_preview->ShowCollision( status );
}

void CEdMeshEditor::OnShowNavObstacles( wxCommandEvent& event )
{
	wxToolBar* toolbar = XRCCTRL( *this, "Tools", wxToolBar );
	const Bool status = toolbar->GetToolState( XRCID("toolShowNavObstacles") );

	// Do not show collision if it's not there
	if ( status )
	{
		if ( m_mesh )
		{
			Bool hasObstacle = m_mesh->IsA< CMesh >() && !SafeCast< CMesh >( m_mesh )->GetNavigationObstacle().IsEmpty();

			if ( !hasObstacle )
			{
				// Disable preview
				toolbar->ToggleTool( XRCID("toolShowNavObstacles"), false );
				m_preview->ShowNavObstacles( false );

				// Inform
				wxMessageBox( wxT("There's no navigation obstacle to show"), wxT("Error"), wxOK | wxICON_WARNING );
				return;
			}
		}
	}

	// Update preview
	m_preview->ShowNavObstacles( status );
}

void CEdMeshEditor::OnSwapCollisionTriangles( wxCommandEvent& event )
{
}

void CEdMeshEditor::OnGenerateBillboards( wxCommandEvent& event )
{
}

void CEdMeshEditor::OnRemoveUnusedMaterials( wxCommandEvent& event )
{
	if ( CMesh* mesh = Cast< CMesh >( m_mesh ) )
	{
		if ( !m_mesh->MarkModified() )
		{
			return;
		}

		CUndoMeshMaterialsChanged::CreateStep( *m_undoManager, mesh, TXT("remove unused materials") );

		// Remove unused materials from mesh
		const Uint32 prevMaterialCount = m_mesh->GetMaterials().Size();
		SafeCast< CMesh >( m_mesh )->RemoveUnusedMaterials();

		// Something was removed, display status
		const Uint32 curMaterialCount = m_mesh->GetMaterials().Size();
		if ( prevMaterialCount != curMaterialCount )
		{
			String msg = String::Printf( TXT("Material count was reduced from %i to %i."), prevMaterialCount, curMaterialCount );
			wxMessageBox( msg.AsChar(), wxT("Remove unused materials"), wxOK | wxICON_INFORMATION );
			UpdateMaterialList();
		}
		else
		{
			wxMessageBox( wxT("No materials were removed"), wxT("Remove unused materials"), wxOK | wxICON_INFORMATION );
		}
	}
}

void CEdMeshEditor::OnRemoveUnusedBones( wxCommandEvent& event )
{
	if ( m_mesh->IsA< CMesh >() && m_mesh->MarkModified() )
	{
		CMesh* mesh = SafeCast< CMesh >( m_mesh );

		// Remove unused bones from mesh
		const Uint32 prevBoneCount = mesh->GetBoneCount();
		mesh->RemoveUnusedBones();

		// Something was removed, display status
		const Uint32 curBoneCount = mesh->GetBoneCount();
		if ( prevBoneCount != curBoneCount )
		{
			// Show the message
			String msg = String::Printf( TXT("Bone count was reduced from %i to %i."), prevBoneCount, curBoneCount );
			wxMessageBox( msg.AsChar(), wxT("Remove unused bones"), wxOK | wxICON_INFORMATION );

			// Mark mesh as modified
			m_mesh->MarkModified();

			// Update rendering
			RefreshPreviewRenderingProxy();
		}
		else
		{
			wxMessageBox( wxT("No bones were removed"), wxT("Remove unused bones"), wxOK | wxICON_INFORMATION );
		}
	}
}

void CEdMeshEditor::OnRemoveSkinningData( wxCommandEvent& event )
{
	if ( m_mesh->IsA< CMesh >() && m_mesh->MarkModified() )
	{
		// Remove skinning data
		SafeCast< CMesh >( m_mesh )->RemoveSkinning();

		// Mark mesh as modified
		m_mesh->MarkModified();

		UpdateLODList( true );
		UpdateMeshStats();
		// Update rendering
		RefreshPreviewRenderingProxy();
	}
}

void CEdMeshEditor::OnReimportMesh( wxCommandEvent& event )
{
	wxMenu importMenu;
	importMenu.Append( 0, TXT("Mesh and collision") );
	importMenu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdMeshEditor::OnReimportMeshMenu, this, 0 );
	importMenu.Append( 1, TXT("Mesh only") );
	importMenu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdMeshEditor::OnReimportMeshMenu, this, 1 );
	importMenu.Append( 2, TXT("Collision only") );
	importMenu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdMeshEditor::OnReimportMeshMenu, this, 2 );
	importMenu.Append( 3, TXT("Import mesh and regenerate collision") );
	importMenu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdMeshEditor::OnReimportMeshMenu, this, 2 );

	wxToolBar* toolbar = XRCCTRL( *this, "Tools", wxToolBar );
	int pos = toolbar->GetToolSize().x * toolbar->GetToolPos( XRCID("toolReimport") );
	toolbar->PopupMenu( &importMenu, pos, toolbar->GetSize().y );
}

void CEdMeshEditor::OnReimportMeshMenu( wxCommandEvent& event )
{
	switch( event.GetId() )
	{
	case 0:
		ReimportMesh( false, false, false );
		break;
	case 1:
		ReimportMesh( false, true, false );
		break;
	case 2:
		ReimportMesh( true, false, false );
		break;
	case 3:
		ReimportMesh( false, false, true );
		break;
	}
}

void CEdMeshEditor::ReimportMesh( Bool reuseMesh, Bool reuseVolumes, Bool regenerateVolumes )
{
	if ( !m_mesh->MarkModified() )
	{
		return;
	}

	// Open for edit
	if ( CMesh* mesh = Cast< CMesh >( m_mesh ))
	{
		// Get supported file formats for animations
		TDynArray< CFileFormat > meshFileFormats;
		IImporter::EnumImportFormats( ClassID< CMesh >(), meshFileFormats );

		// Setup crap
		CEdFileDialog fileDialog;
		fileDialog.AddFormats( meshFileFormats );
		fileDialog.SetMultiselection( false );
		fileDialog.SetIniTag( TXT("MeshImport") );

		// Get initial file name and directory
		Bool useDirFromIni = false;
		if ( m_mesh->GetImportFile().GetLength() > 0 )
		{
			String importDir = m_mesh->GetImportFile().StringBefore( TXT("\\"), true );
			if ( importDir.GetLength() > 0 )
			{
				fileDialog.SetDirectory( importDir );
				useDirFromIni = false;
			}
		}

		// Ask for new file name
		if ( !fileDialog.DoOpen( (HWND) GetHWND(), true ) )
		{
			wxMessageBox( wxT("No data was imported"), wxT("Mesh reimport"), wxOK | wxICON_INFORMATION );
			return;
		}

		// Find mesh importer
		const String meshFile = fileDialog.GetFile();
		CFilePath filePath( meshFile );
		IImporter* meshImporter = IImporter::FindImporter( ClassID< CMesh >(), filePath.GetExtension() );
		if ( !meshImporter )
		{
			wxMessageBox( wxT("No matching mesh imported was found. No data was imported."), wxT("Mesh reimport"), wxOK | wxICON_INFORMATION );
			return;
		}

		// Setup parameters
		IImporter::ImportOptions options;
		SMeshImporterParams params( reuseMesh, reuseVolumes );
		params.m_regenerateVolumes = regenerateVolumes;
		options.m_existingResource = m_mesh;
		options.m_parentObject = m_mesh->GetParent();
		options.m_sourceFilePath = meshFile;
		options.m_params = &params;

		// Import new mesh
		GFeedback->BeginTask( TXT("Reimporting"), false );
		CMesh* newMesh = Cast< CMesh >( meshImporter->DoImport( options ) );
		GFeedback->EndTask();

		if ( !newMesh )
		{
			wxMessageBox( wxT("Internal importer error. No data was imported."), wxT("Mesh reimport"), wxOK | wxICON_INFORMATION );
			return;
		}

		// Mesh should match!
		ASSERT( newMesh == m_mesh );

		// Change import path
		m_mesh->SetImportFile( meshFile );

		// Update lists
		m_properties->Get().SetObject( m_mesh );
		UpdateMaterialList();
		UpdateChunkMaterialsChoice();
		UpdateLODList( false );
		UpdateDisplayedLODChoice();

		m_physicalRepresentation->Refresh();
		if( m_soundInfo )
		{
			m_soundInfo->Refresh();
		}

		// Save the mesh
		m_mesh->Save();

		// Update crap
		m_preview->UpdateBounds();
		RefreshPreviewRenderingProxy();

		// Update stats
		UpdateMeshStats();
	}
}

void CEdMeshEditor::ValidateResources()
{
#ifndef NO_DATA_VALIDATION
	// flush all gathered errors related with loaded resource	
	// check data errors for all dependencies
	for ( Uint32 i = 0; i < m_dependentResourcesPaths.Size(); ++i )
	{
		CDiskFile* file = GDepot->FindFileUseLinks( m_dependentResourcesPaths[i], 0 );
		if ( file )
		{
			CResource* res = file->GetResource();
			if ( res )
			{
				res->OnCheckDataErrors();
			}
		}
	}
	
	SEntityStreamingState state;
	m_preview->GetEntity()->PrepareStreamingComponentsEnumeration( state, true, SWN_DoNotNotifyWorld );
	m_preview->GetEntity()->ForceFinishAsyncResourceLoads();
	m_preview->GetEntity()->ForceUpdateBoundsNode();
	m_preview->GetEntity()->OnCheckDataErrors( false );
	m_preview->GetEntity()->FinishStreamingComponentsEnumeration( state, SWN_DoNotNotifyWorld );

#endif
}

void CEdMeshEditor::OnBuildOctree( wxCommandEvent& event )
{
	if ( !m_mesh->IsA< CMesh >() )
	{
		return;
	}

	if ( m_octree )
	{
		delete m_octree;
		m_octree = NULL;

	}

	CMesh* mesh = SafeCast< CMesh >( m_mesh );

	long val = 256;
	m_octreeDensity->GetValue().ToLong( &val );
	Uint32 octreeDensity = (Uint32)val;

	const Bool useTextures = m_diffuseTextures->IsChecked();

	{
		const CMeshData meshData( mesh );
		const TDynArray< SMeshChunk >& chunks = meshData.GetChunks();

		TDynArray<Uint16> chunksToCheck = mesh->GetMeshLODLevels()[0].m_chunks;

		Vector min = Vector(1000,1000,1000);
		Vector max = Vector(-1000,-1000,-1000);

		Uint32 triangles = 0;

		for ( Uint32 i = 0; i < chunksToCheck.Size(); ++i )
		{
			Uint32 chunkNum = chunksToCheck[i];

			auto& chunk = chunks[chunkNum];
			for ( Uint32 vert = 0; vert < chunk.m_numVertices; vert++ )
			{
				min.X = Min<Float>( min.X, chunk.m_vertices[vert].m_position[0] );
				min.Y = Min<Float>( min.Y, chunk.m_vertices[vert].m_position[1] );
				min.Z = Min<Float>( min.Z, chunk.m_vertices[vert].m_position[2] );

				max.X = Max<Float>( max.X, chunk.m_vertices[vert].m_position[0] );
				max.Y = Max<Float>( max.Y, chunk.m_vertices[vert].m_position[1] );
				max.Z = Max<Float>( max.Z, chunk.m_vertices[vert].m_position[2] );
			}

			Uint32 numTriangles = chunk.m_numIndices / 3;
			triangles += numTriangles;
		}

		Red::Threads::CAtomic< Int32 > counter;

		const Uint32 totalTriangles = triangles;

		Vector epsilon( 0.05f, 0.05f, 0.05f, 0.0f );
		CVoxelOctree* tree = new CVoxelOctree( octreeDensity, min-epsilon, max+epsilon );

		omp_set_num_threads(8);

		for ( Uint32 i = 0; i < chunksToCheck.Size(); ++i )
		{
			Uint32 chunkNum = chunksToCheck[i];

			auto& chunk = chunks[chunkNum];
			Int32 numChunkIndices = chunk.m_numIndices;

			CBitmapTexture::MipMap textureMip;

			CMaterialInstance* matInstance = Cast< CMaterialInstance >( m_mesh->GetMaterials()[chunk.m_materialID].Get() );
			if ( matInstance )
			{
				// Get definition for this material 
				IMaterialDefinition* def = matInstance->GetMaterialDefinition();
				if ( def )
				{
					// Scan for texture parameters
					const IMaterialDefinition::TParameterArray& params = def->GetPixelParameters();
					for ( Uint32 k=0; k<params.Size(); k++ )
					{
						const IMaterialDefinition::Parameter& param = params[k];

						if ( param.m_type == IMaterialDefinition::PT_Texture && ( param.m_name.AsString().ToLower().ContainsSubstring( TXT("diffuse") ) ) )
						{
							// Get value ( the bounded texture )
							THandle< CBitmapTexture > texture;
							matInstance->ReadParameter( param.m_name, texture );

							// Load the source data
							CBitmapTexture::CreateMip( textureMip, texture->GetWidth() / 8, texture->GetHeight() / 8, TRF_TrueColor , TCM_None );

							// Load data to source mip
							if ( texture && texture->GetMips().Size() )
							{
								const CBitmapTexture::MipMap& textureSourceMip = texture->GetMips()[3];
								CBitmapTexture::CopyRect( textureSourceMip, texture->GetFormat(), texture->GetCompression(), textureMip, TCM_None );
							}
						}
					}
				}
			}

			#pragma omp parallel default(none) shared( numChunkIndices, chunk, tree, textureMip, counter )
			{
				#pragma omp for schedule( static, 32 )
				for ( Int32 tri = 0; tri < numChunkIndices; tri+=3 )
				{
					//tree->InsertTriangle( chunk.m_vertices[chunk.m_indices[tri]], chunk.m_vertices[chunk.m_indices[tri+1]], chunk.m_vertices[chunk.m_indices[tri+2]] );
					if ( useTextures )
					{
						tree->InsertTriangleColor( chunk.m_vertices[chunk.m_indices[tri]], chunk.m_vertices[chunk.m_indices[tri+1]], chunk.m_vertices[chunk.m_indices[tri+2]], textureMip );
					}
					else
					{
						tree->InsertTriangleColor( chunk.m_vertices[chunk.m_indices[tri]], chunk.m_vertices[chunk.m_indices[tri+1]], chunk.m_vertices[chunk.m_indices[tri+2]], 0xFFFFFFFF );
					}
					//				LOG_ALWAYS( TXT("%d"), CVoxelNode::counter );
				
					counter.Increment();
					if ( omp_get_thread_num() == 0 )
					{
						Uint32 progress = 100 * counter.GetValue() / totalTriangles;
						m_progressAO->SetValue( progress );
					}

				}
			}
		}
		
		m_octree = tree;
		m_progressAO->SetValue( 0 );

		m_preview->SetTree( m_octree );

		/*
		Uint32 pow = 1;

		for ( Uint32 i = 0; i < 20; ++i )
		{
			LOG_EDITOR(TXT("%d level %d voxels out of %d max"), i , m_tree->m_levelsStats[i], pow*pow*pow );
			pow *= 2;
		}
		*/
	}
}

__forceinline Float CalcLightDirectional( const Vector& dir, const Vector& lightDir )
{
	return Max<Float>(dir.Dot3(lightDir),0.0f);
}
__forceinline Float CalcLightUniform( const Vector& dir, const Vector& lightDir )
{
	return 1.0f;
}
__forceinline Float CalcLightEllipsoid( const Vector& dir, const Vector& lightDir )
{
	return 0.5f * (dir.Dot3(lightDir)+1.0f);
}

__forceinline Float CalcLight( Uint32 typ, const Vector& dir, const Vector& lightDir )
{
	if ( typ == 0 )
	{
		return CalcLightEllipsoid( dir, lightDir );
	}
	else if ( typ == 1 )
	{
		return CalcLightUniform( dir, lightDir );
	}
	else
	{
		return CalcLightDirectional( dir, lightDir );
	}	
}

void CEdMeshEditor::OnBakeAO( wxCommandEvent& event )
{
	if ( !m_mesh->IsA< CMesh >() )
	{
		return;
	}

	CMesh* mesh = SafeCast< CMesh >( m_mesh );

	if( !mesh->CanUseExtraStreams() )
	{
		if( GFeedback->AskYesNo(TEXT("This mesh doesn't use vertex color (useExtraStreams)\ndo you want to turn it on?") ) )
		{
			mesh->SetUseExtractStreams( true );
			mesh->Save();
		}
	}
	else
	{
		if ( !m_octree )
		{
			//GFeedback->ShowError( TXT("Need to create octree first!") );
			OnBuildOctree( event );
			//return;
		}

		bool useRedChannel = XRCCTRL( *this, "m_BAOChRed", wxCheckBox )->IsChecked();
		bool useGreenChannel = XRCCTRL( *this, "m_BAOChGreen", wxCheckBox )->IsChecked();
		bool useBlueChannel = XRCCTRL( *this, "m_BAOChBlue", wxCheckBox )->IsChecked();

		if ( mesh->MarkModified() )
		{
			CMeshData meshData( mesh );
			TDynArray< SMeshChunk >& chunks = meshData.GetChunks();

			TDynArray<Uint16> chunksToCheck = mesh->GetMeshLODLevels()[0].m_chunks;

			Float epsX = m_octree->m_minNodeSizeX;
			Float epsY = m_octree->m_minNodeSizeY;
			Float epsZ = m_octree->m_minNodeSizeZ;

			long val = 0;
			m_samplesFirst->GetValue().ToLong( &val );
			const Uint32 samplesFirstBounce = (Uint32)val;
			m_samplesSecond->GetValue().ToLong( &val );
			const Uint32 samplesSecondBounce = (Uint32)val;
			const Bool useGround = m_useGround->IsChecked();
			double val2 = 0.0;
			m_groundZ->GetValue().ToDouble( &val2 );
			const Float groundValue = (Float)val2;

			Uint32 groundColorTemp = m_groundColor->GetColour().GetRGB();
			const Uint8 groundColorB = ((Uint8)((groundColorTemp)>>16));
			const Uint8 groundColorG = ((Uint8)((groundColorTemp & 0x0000FF00)>>8));
			const Uint8 groundColorR = ((Uint8)((groundColorTemp & 0x000000FF)));

			const Uint32 groundColor = (groundColorR << 16) + (groundColorG << 8) + groundColorB;

			Bool coloredAO = m_coloredAO->IsChecked();

			m_energyMultiplier->GetValue().ToDouble( &val2 );
			const Float energyMultiplier = (Float)val2;

			const Uint32 lightType = m_lightType->GetSelection();

			Float lightStrength;
			m_lightStrength->GetValue().ToDouble( &val2 );
			lightStrength = (Float)val2;

			Uint32 lightColor = m_lightColor->GetColour().GetRGB();
			const Float lightColorB = lightStrength * (Float)((Uint8)((lightColor)>>16))/255.0f * (Float)((Uint8)((lightColor)>>16))/255.0f;
			const Float lightColorG = lightStrength * (Float)((Uint8)((lightColor & 0x0000FF00)>>8))/255.0f * (Float)((Uint8)((lightColor & 0x0000FF00)>>8))/255.0f;
			const Float lightColorR = lightStrength * (Float)((Uint8)((lightColor & 0x000000FF)))/255.0f * (Float)((Uint8)((lightColor & 0x000000FF)))/255.0f;

			Float lX,lY,lZ;
			m_lightDirectionX->GetValue().ToDouble( &val2 );
			lX = (Float)val2;
			m_lightDirectionY->GetValue().ToDouble( &val2 );
			lY = (Float)val2;
			m_lightDirectionZ->GetValue().ToDouble( &val2 );
			lZ = (Float)val2;

			const Vector lightDir = Vector( lX, lY, lZ ).Normalized3();

			const Float epsMag = Vector(m_octree->m_minNodeSizeX,m_octree->m_minNodeSizeY,m_octree->m_minNodeSizeZ).Mag3();

			const Bool secondaryBounce = m_secondaryBounce->IsChecked();
			const Bool classicAO = (lightType == 3);
				
			const CVoxelOctree* tree = m_octree;

			omp_set_num_threads(8);

			Red::Threads::CAtomic< Int32 > counter;

			Uint32 triangles = 0;
			for ( Uint32 i = 0; i < chunksToCheck.Size(); ++i )
			{
				Uint32 chunkNum = chunksToCheck[i];
				SMeshChunk& chunk = chunks[chunkNum];

				Uint32 numTriangles = chunk.m_numIndices / 3;
				triangles += numTriangles;
			}

			const Uint32 totalTriangles = triangles;

			for ( Uint32 i = 0; i < chunksToCheck.Size(); ++i )
			{
				Uint32 chunkNum = chunksToCheck[i];

				SMeshChunk& chunk = chunks[chunkNum];

				Uint32 numTriangles = chunk.m_numIndices / 3;
				TDynArray<Float> aosR;
				TDynArray<Float> aosG;
				TDynArray<Float> aosB;

				aosR.Resize(numTriangles);
				aosG.Resize(numTriangles);
				aosB.Resize(numTriangles);

				{
	#pragma omp parallel default(none) shared( aosR, aosG, aosB, chunk, numTriangles, tree, counter, GEngine )
					{
	#pragma omp for schedule( static, 32 )
						for ( Int32 j = 0; j < (Int32)chunk.m_numIndices; j+= 3 )
						{
							Vector p1 = Vector( chunk.m_vertices[chunk.m_indices[j]].m_position[0],chunk.m_vertices[chunk.m_indices[j]].m_position[1],chunk.m_vertices[chunk.m_indices[j]].m_position[2] );
							Vector p2 = Vector( chunk.m_vertices[chunk.m_indices[j+1]].m_position[0],chunk.m_vertices[chunk.m_indices[j+1]].m_position[1],chunk.m_vertices[chunk.m_indices[j+1]].m_position[2] );
							Vector p3 = Vector( chunk.m_vertices[chunk.m_indices[j+2]].m_position[0],chunk.m_vertices[chunk.m_indices[j+2]].m_position[1],chunk.m_vertices[chunk.m_indices[j+2]].m_position[2] );

							Vector n1 = Vector( chunk.m_vertices[chunk.m_indices[j]].m_normal[0],chunk.m_vertices[chunk.m_indices[j]].m_normal[1],chunk.m_vertices[chunk.m_indices[j]].m_normal[2] );
							Vector n2 = Vector( chunk.m_vertices[chunk.m_indices[j+1]].m_normal[0],chunk.m_vertices[chunk.m_indices[j+1]].m_normal[1],chunk.m_vertices[chunk.m_indices[j+1]].m_normal[2] );
							Vector n3 = Vector( chunk.m_vertices[chunk.m_indices[j+2]].m_normal[0],chunk.m_vertices[chunk.m_indices[j+2]].m_normal[1],chunk.m_vertices[chunk.m_indices[j+2]].m_normal[2] );

							Vector normal = (n1+n2+n3).Mul3( 0.333333f ).Normalized3();
							Vector position = (p1+p2+p3).Mul3( 0.333333f );

							Float collidedWeightsR = 0.0f;
							Float collidedWeightsG = 0.0f;
							Float collidedWeightsB = 0.0f;


							Float allWeights = 0.0f;

							if ( classicAO )
							{
								for ( Uint32 k = 0; k < samplesFirstBounce; ++k )
								{
									Vector randVec( GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ), GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ), GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ) );
								
									// Vector::ZERO sanity checking
									while( randVec.Mag2() < 0.001 )
									{
										randVec = Vector( GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ), GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ), GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ) );
									}

									randVec.Normalize3();

									Float weight = randVec.Dot3( normal );
									if ( weight > 0.01f )
									{
										allWeights += 1.0f;

										// shift by 2mm
										Vector rayOrigin = position + normal * 0.002f;

										Vector finalPos;
										Uint32 color = 0;
										Vector hitNormal;

										Bool wasHit = m_octree->RayCast( rayOrigin, randVec, epsMag * 1.0f, 0.7f, finalPos, color, hitNormal ) && finalPos.DistanceTo( rayOrigin ) < 0.7f;
									
										if ( useGround && !wasHit && (finalPos.Z<groundValue) )
										{
											wasHit = true;
										}

										collidedWeightsR += wasHit ? 1.0f : 0.0f;
									}
								}
							
								aosR[j/3] = (1.0f - collidedWeightsR/allWeights)*(1.0f - collidedWeightsR/allWeights)*(1.0f - collidedWeightsR/allWeights);
								aosG[j/3] = (1.0f - collidedWeightsR/allWeights)*(1.0f - collidedWeightsR/allWeights)*(1.0f - collidedWeightsR/allWeights);
								aosB[j/3] = (1.0f - collidedWeightsR/allWeights)*(1.0f - collidedWeightsR/allWeights)*(1.0f - collidedWeightsR/allWeights);

							}
							else
							{
								for ( Uint32 k = 0; k < samplesFirstBounce; ++k )
								{
									Vector randVec = Vector( GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ), GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ), GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ) ) + normal;
									randVec.Normalize3();

									Float weight = randVec.Dot3( normal );
									if ( weight > 0.02f )
									{
										allWeights += weight;

										Vector rayOrigin = position + normal * epsMag * 1.1f;

										Vector finalPos;
										Uint32 color = 0;
										Vector hitNormal;

										Bool wasHit = m_octree->RayCast( rayOrigin, randVec, epsMag * 1.5f, 100000.0f, finalPos, color, hitNormal );

										if ( useGround && !wasHit && (finalPos.Z<groundValue) )
										{
											wasHit = true;
											finalPos.Z = groundValue;
											hitNormal = Vector( 0,0,1);
											color = groundColor;
										}

										if ( wasHit )
										{
											Float accumBounceEnergyR = 0.0f;
											Float accumBounceEnergyG = 0.0f;
											Float accumBounceEnergyB = 0.0f;

											Float accumBounceWeights = 0.001f;

											// Secondary bounce :> 
											for ( Uint32 l = 0; l < samplesSecondBounce; ++l )
											{
												Vector randVec2 = Vector( GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ), GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ), GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ) ) + hitNormal;
												randVec2.Normalize3();

												Float weight2 = randVec2.Dot3( hitNormal );
												if ( weight2 > 0.02f )
												{
													accumBounceWeights += weight2;

													Vector rayOrigin2 = finalPos + hitNormal * epsMag * 1.1f;

													Vector finalPos2;
													Uint32 color2 = 0;
													Vector normal2;

													Bool wasHit2 = m_octree->RayCast( rayOrigin2, randVec2, epsMag * 1.5f, 100000.0f, finalPos2, color2, normal2 );

													if ( useGround && !wasHit2 && (finalPos2.Z<groundValue) )
													{
														wasHit2 = true;
														finalPos2.Z = groundValue;
														normal2 = Vector( 0,0,1);
														color2 = groundColor;
													}

													if ( !wasHit2 )
													{
														Vector dirToFinalPos = finalPos2-rayOrigin2;
														dirToFinalPos.Normalize3();

														Float lightStrength = CalcLight( lightType, dirToFinalPos, lightDir );

														accumBounceEnergyR += lightColorR * lightStrength * weight2;
														accumBounceEnergyG += lightColorG * lightStrength * weight2;
														accumBounceEnergyB += lightColorB * lightStrength * weight2;
													}
													else if ( secondaryBounce )
													{
														// Are you nuts??? :> 
														Float accumBounceEnergyR2 = 0.0f;
														Float accumBounceEnergyG2 = 0.0f;
														Float accumBounceEnergyB2 = 0.0f;

														Float accumBounceWeights2 = 0.001f;

														// Third bounce :> 
														for ( Uint32 m = 0; m < samplesSecondBounce; ++m )
														{
															Vector randVec3 = Vector( GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ), GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ), GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ) ) + normal2;
															randVec3.Normalize3();

															Float weight3 = randVec3.Dot3( normal2 );
															if ( weight3 > 0.02f )
															{
																accumBounceWeights2 += weight3;

																Vector rayOrigin3 = finalPos2 + normal2 * epsMag * 1.1f;

																Vector finalPos3;
																Uint32 color3 = 0;
																Vector normal3;

																Bool wasHit3 = m_octree->RayCast( rayOrigin3, randVec3, epsMag * 1.5f, 100000.0f, finalPos3, color3, normal3 );

																if ( !wasHit3 )
																{
																	Vector dirToFinalPos = finalPos3-rayOrigin3;
																	dirToFinalPos.Normalize3();

																	Float lightStrength = CalcLight( lightType, dirToFinalPos, lightDir );

																	accumBounceEnergyR2 += lightColorR * lightStrength * weight3;
																	accumBounceEnergyG2 += lightColorG * lightStrength * weight3;
																	accumBounceEnergyB2 += lightColorB * lightStrength * weight3;
																}
															}
														}

														Uint8 b = (color2 & 0x000000FF);
														Uint8 r = (color2 & 0x00FF0000)>> 16;
														Uint8 g = (color2 & 0x0000FF00 ) >> 8;

														accumBounceEnergyR += ((Float)r / 255.0f)*((Float)r / 255.0f)*energyMultiplier*accumBounceEnergyR2/accumBounceWeights2 * weight2;
														accumBounceEnergyG += ((Float)g / 255.0f)*((Float)g / 255.0f)*energyMultiplier*accumBounceEnergyG2/accumBounceWeights2 * weight2;
														accumBounceEnergyB += ((Float)b / 255.0f)*((Float)b / 255.0f)*energyMultiplier*accumBounceEnergyB2/accumBounceWeights2 * weight2;
													}
												}
											}

											Uint8 b = (color & 0x000000FF);
											Uint8 r = (color & 0x00FF0000)>> 16;
											Uint8 g = (color & 0x0000FF00 ) >> 8;

											Float energyInPointR = ((Float)r / 255.0f)*((Float)r / 255.0f)*energyMultiplier*accumBounceEnergyR/accumBounceWeights;
											Float energyInPointG = ((Float)g / 255.0f)*((Float)g / 255.0f)*energyMultiplier*accumBounceEnergyG/accumBounceWeights;
											Float energyInPointB = ((Float)b / 255.0f)*((Float)b / 255.0f)*energyMultiplier*accumBounceEnergyB/accumBounceWeights;

											collidedWeightsR += (1.0f - energyInPointR) * weight;
											collidedWeightsG += (1.0f - energyInPointG) * weight;
											collidedWeightsB += (1.0f - energyInPointB) * weight;
										}
										else
										{
											Vector dirToFinalPos = finalPos-rayOrigin;
											dirToFinalPos.Normalize3();
											Float lightStrength = CalcLight( lightType, dirToFinalPos, lightDir );

											Float energyInPointR = lightColorR * lightStrength;
											Float energyInPointG = lightColorG * lightStrength;
											Float energyInPointB = lightColorB * lightStrength;

											collidedWeightsR += (1.0f - energyInPointR) * weight;
											collidedWeightsG += (1.0f - energyInPointG) * weight;
											collidedWeightsB += (1.0f - energyInPointB) * weight;
										}

										/*
										{
										Uint8 b = (color & 0x000000FF);
										Uint8 r = (color & 0x00FF0000)>> 16;
										Uint8 g = (color & 0x0000FF00 ) >> 8;

										collidedWeightsR += 1.1f*(1.0f - ((Float)r / 255.0f)) * weight;
										collidedWeightsG += 1.1f*(1.0f - ((Float)g / 255.0f)) * weight;
										collidedWeightsB += 1.1f*(1.0f - ((Float)b / 255.0f)) * weight;
										}
										else
										{
										Vector dirToFinalPos = finalPos-rayOrigin;
										dirToFinalPos.Normalize3();
										collidedWeightsR += (1.0f - Min<Float>(dirToFinalPos.Dot3(Vector(0,0,1,0))+1.0f,1.0f)) * weight;
										collidedWeightsG += (1.0f - Min<Float>(dirToFinalPos.Dot3(Vector(0,0,1,0))+1.0f,1.0f)) * weight;
										collidedWeightsB += (1.0f - Min<Float>(dirToFinalPos.Dot3(Vector(0,0,1,0))+1.0f,1.0f)) * weight;
										}
										*/
									}
								}

								Float r = 1.0f - Clamp<Float>( collidedWeightsR / allWeights, 0.0f,1.0f );
								Float g = 1.0f - Clamp<Float>( collidedWeightsG / allWeights, 0.0f,1.0f );
								Float b = 1.0f - Clamp<Float>( collidedWeightsB / allWeights, 0.0f,1.0f );

								aosR[j/3] = r;
								aosG[j/3] = g;
								aosB[j/3] = b;
							}

							counter.Increment();
							if ( omp_get_thread_num() == 0 )
							{
								Uint32 progress = 100 * counter.GetValue() / totalTriangles;
								m_progressAO->SetValue( progress );
							}
						}
					}
				}

				for ( Uint32 vert = 0; vert < chunk.m_numVertices; vert++ )
				{
					TDynArray<Float> triangleAosR;
					TDynArray<Float> triangleAosG;
					TDynArray<Float> triangleAosB;

					TDynArray<Float> triangleWeights;

					Float totalTriangleWeight = 0.000001f;

					for ( Uint32 j = 0; j < chunk.m_numIndices; j += 3 )
					{
						if ( chunk.m_indices[j] == vert || chunk.m_indices[j+1] == vert || chunk.m_indices[j+2] == vert )
						{
							Vector p1 = Vector( chunk.m_vertices[chunk.m_indices[j]].m_position[0],chunk.m_vertices[chunk.m_indices[j]].m_position[1],chunk.m_vertices[chunk.m_indices[j]].m_position[2] );
							Vector p2 = Vector( chunk.m_vertices[chunk.m_indices[j+1]].m_position[0],chunk.m_vertices[chunk.m_indices[j+1]].m_position[1],chunk.m_vertices[chunk.m_indices[j+1]].m_position[2] );
							Vector p3 = Vector( chunk.m_vertices[chunk.m_indices[j+2]].m_position[0],chunk.m_vertices[chunk.m_indices[j+2]].m_position[1],chunk.m_vertices[chunk.m_indices[j+2]].m_position[2] );

							Float a = (p2-p1).Mag3();
							Float b = (p3-p1).Mag3();
							Float c = (p3-p2).Mag3();

							// area
							Float weight = sqrtf((a+b+c)*(a+b-c)*(a-b+c)*(-a+b+c));

							triangleAosR.PushBack( aosR[j/3] );
							triangleAosG.PushBack( aosG[j/3] );
							triangleAosB.PushBack( aosB[j/3] );

							triangleWeights.PushBack( weight );
							totalTriangleWeight += weight;
						}
					}

					Float aoR = 0.0f, aoG = 0.0f, aoB = 0.0f;
					for ( Uint32 k = 0; k < triangleAosR.Size(); ++k )
					{
						aoR += (triangleAosR[k]*triangleWeights[k]/totalTriangleWeight);
						aoG += (triangleAosG[k]*triangleWeights[k]/totalTriangleWeight);
						aoB += (triangleAosB[k]*triangleWeights[k]/totalTriangleWeight);
					}

					if ( coloredAO )
					{
						Uint32 uColor = chunk.m_vertices[vert].m_color;

						if( useRedChannel )		uColor = (uColor & 0XFF00FFFF) | ( ((Uint32)(255*sqrtf(aoR)) <<16) );
						if( useGreenChannel )	uColor = (uColor & 0XFFFF00FF) | ( ((Uint32)(255*sqrtf(aoG)) <<8) );
						if( useBlueChannel )	uColor = (uColor & 0XFFFFFF00) | ( (Uint32) (255*sqrtf(aoB)) );

						chunk.m_vertices[vert].m_color = uColor;
						//chunk.m_vertices[vert].m_color = (Uint8)(255 * sqrtf(aoB)) + ((Uint8)(255 * sqrtf(aoG)) << 8) + ((Uint8)(255 * sqrtf(aoR)) << 16);
					}
					else
					{
						Float aoFinal = (sqrtf(aoR)+sqrtf(aoG)+sqrtf(aoB))/3.0f;

						Uint32 uColor = chunk.m_vertices[vert].m_color;

						if( useRedChannel )		uColor = (uColor & 0XFF00FFFF) | ( ((Uint32)(255*aoFinal) <<16) );
						if( useGreenChannel )	uColor = (uColor & 0XFFFF00FF) | ( ((Uint32)(255*aoFinal) <<8) );
						if( useBlueChannel )	uColor = (uColor & 0XFFFFFF00) | ( (Uint32) (255*aoFinal) );

						chunk.m_vertices[vert].m_color = uColor;
						//chunk.m_vertices[vert].m_color = (Uint8)(255 * aoFinal) + ((Uint8)(255 * aoFinal) << 8) + ((Uint8)(255 * aoFinal) << 16);
					}
				}
			}

		}
	}
	m_progressAO->SetValue( 0 );
	mesh->Save();
	mesh->CreateRenderResource();	
	RefreshPreviewRenderingProxy();
}


void CEdMeshEditor::OnClothPresetChoice( wxCommandEvent& event )
{
	ASSERT( m_mesh->IsA< CApexClothResource >(), TXT("Cloth preset chosen without a cloth resource??") );

	CApexClothResource* cloth = Cast< CApexClothResource >( m_mesh );

	// Get preset data
	wxChoice* choiceSelector = (wxChoice*)event.GetEventObject();
	int selIdx = choiceSelector->GetSelection();

	// Selecting index 0 restores the cloth to its defaults (from the original imported APB file).
	if ( selIdx == 0 )
	{
		OnApexRestoreDefaults( wxCommandEvent() );
	}
	else
	{
		wxString selStr = choiceSelector->GetString( selIdx );
		SClothMaterialPreset* preset = m_clothPresets.FindPtr( selStr.wc_str() );
		if ( preset )
		{
			// Apply to current cloth
			cloth->m_mtlFriction = preset->m_friction;
			cloth->m_mtlGravityScale = preset->m_gravityScale;
			cloth->m_mtlBendingStiffness = preset->m_bendingStiffness;
			cloth->m_mtlShearingStiffness = preset->m_shearingStiffness;
			cloth->m_mtlTetherStiffness = preset->m_tetherStiffness;
			cloth->m_mtlTetherLimit = preset->m_tetherLimit;
			cloth->m_mtlDamping = preset->m_damping;
			cloth->m_mtlDrag = preset->m_drag;
			cloth->m_mtlInertiaScale = preset->m_inertiaScale;
			cloth->m_mtlMaxDistanceBias = preset->m_maxDistanceBias;
			cloth->m_mtlSelfcollisionThickness = preset->m_selfcollisionThickness;
			cloth->m_mtlSelfcollisionStiffness = preset->m_selfcollisionStiffness;
			cloth->m_mtlHardStretchLimitation = preset->m_hardStretchLimitation;
			cloth->m_mtlComDamping = preset->m_comDamping;
			cloth->m_mtlMassScale = preset->m_massScale;
			
			// Rebuild the cloth and we're done!
			cloth->RebuildPreviewAsset();
			m_preview->Reload();
			RefreshBoundControls();
		}
	}
}


void CEdMeshEditor::OnClothPresetSelectFile( wxCommandEvent& event )
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	String lastPresetPath = config.Read( TXT("/Frames/MeshEditor/ClothPresetsPath"), String::EMPTY );

	CEdFileDialog fileDlg;
	fileDlg.AddFormat( TXT("xml"), TXT("Cloth material presets") );

	fileDlg.SetMultiselection( false );
	if ( lastPresetPath.GetLength() > 0 )
	{
		fileDlg.SetDirectory( lastPresetPath );
	}


	if (fileDlg.DoOpen( (HWND) GetHandle(), lastPresetPath.GetLength() == 0 ) )
	{
		lastPresetPath = fileDlg.GetFile();
		if ( LoadClothPresets( lastPresetPath ) )
		{
			// If loading was a success, save the path we used, so we can use the same one next time.
			config.Write( TXT("/Frames/MeshEditor/ClothPresetsPath"), lastPresetPath );
		}
	}
}


Bool CEdMeshEditor::LoadClothPresets( const String& filename )
{
	m_clothPresets.ClearFast();

	wxChoice* presetList = XRCCTRL( *this, "clothPresetList", wxChoice );
	ASSERT( presetList != NULL, TXT("Loading cloth presets, but no preset list?") );
	if ( !presetList )
	{
		return false;
	}

	presetList->Clear();

	// Add a blank item, for no preset.
	presetList->Append( "-- Resource Default --" );

	wxTextCtrl* presetFileName = XRCCTRL( *this, "clothPresetFile", wxTextCtrl );
	presetFileName->SetValue( "" );

	if ( filename.GetLength() == 0 )
	{
		return false;
	}

	IFile* defFile = GFileManager->CreateFileReader( filename, FOF_AbsolutePath );
	if ( !defFile )
	{
		WARN_EDITOR( TXT("Could not open cloth material presets file %s"), filename.AsChar() );
		return false;
	}

	CXMLFileReader xmlReader( defFile );	// CXMLFileReader will delete the file.
	if ( !xmlReader.IsXML() )
	{
		WARN_EDITOR( TXT("Could not parse XML data from %s"), filename.AsChar() );
		return false;
	}
	if ( !xmlReader.BeginNode( TXT("ApexClothMaterialPresets") ) )
	{
		WARN_EDITOR( TXT("Cloth material presets file's root element must be 'ApexClothMaterialPresets' (%s)"), filename.AsChar() );
		return false;
	}


	presetFileName->SetValue( filename.AsChar() );
	Uint32 unnamedIdx = 1;

	// Freeze updates on preset list until we're done building the list.
	presetList->Freeze();

	while (xmlReader.BeginNode( TXT("Preset") ) )
	{
		// Read in preset.
		SClothMaterialPreset preset;

		// If this preset is based on another, we'll load values from the existing one.
		String basedOn;
		if ( xmlReader.Attribute( TXT("base"), basedOn ) )
		{
			if ( !m_clothPresets.Find( basedOn, preset ) )
			{
				WARN_EDITOR( TXT("Cloth preset '%s' base ('%s') not found. Must be defined before this preset, in the XML."), preset.m_name.AsChar(), basedOn.AsChar() );
			}
		}

		if ( !xmlReader.Attribute( TXT("name"), preset.m_name ) )
		{
			WARN_EDITOR( TXT("Cloth preset missing 'name' attribute") );
			preset.m_name = String::Printf( TXT("_NoName_%i"), unnamedIdx++ );
		}

		if ( preset.m_name.EqualsNC( TXT("-- Resource Default --") ) )
		{
			WARN_EDITOR( TXT("Preset uses reserved name '-- Resource Default --'. Skipping.") );
			xmlReader.EndNode();
			continue;
		}

		ASSERT( !m_clothPresets.KeyExist( preset.m_name ), TXT("Duplicate material preset name '%s' found, skipping."), preset.m_name.AsChar() );
		if ( !m_clothPresets.KeyExist( preset.m_name ) )
		{
			// Load in attributes. If any aren't specified in the XML, the previous values (ideally from a base preset) will remain.
			// Since no constructor for SClothMaterialPreset is given, the default values when no base is used could be anything...

			//
			xmlReader.AttributeT( TXT("friction"),				preset.m_friction );
			xmlReader.AttributeT( TXT("gravScale"),				preset.m_gravityScale );
			xmlReader.AttributeT( TXT("bendStiffness"),			preset.m_bendingStiffness );
			xmlReader.AttributeT( TXT("shearStiffness"),		preset.m_shearingStiffness );
			xmlReader.AttributeT( TXT("tetherStiffness"),		preset.m_tetherStiffness );
			xmlReader.AttributeT( TXT("tetherLimit"),			preset.m_tetherLimit );
			xmlReader.AttributeT( TXT("damping"),				preset.m_damping );
			xmlReader.AttributeT( TXT("drag"),					preset.m_drag );
			xmlReader.AttributeT( TXT("inertiaScale"),			preset.m_inertiaScale );
			xmlReader.AttributeT( TXT("maxDistBias"),			preset.m_maxDistanceBias );
			xmlReader.AttributeT( TXT("selfCollThickness"),		preset.m_selfcollisionThickness );
			xmlReader.AttributeT( TXT("selfCollStiffness"),		preset.m_selfcollisionStiffness );
			xmlReader.AttributeT( TXT("hardStretchLimit"),		preset.m_hardStretchLimitation );			
			xmlReader.AttributeT( TXT("comDamping"),			preset.m_comDamping );
			xmlReader.AttributeT( TXT("massScale"),				preset.m_massScale );

			m_clothPresets.Insert( preset.m_name, preset );
			presetList->AppendString( preset.m_name.AsChar() );
		}

		xmlReader.EndNode();
	}

	presetList->Thaw();

	return true;
}


Bool CEdMeshEditor::SetSelectedPage( EEdMeshEditorPage page )
{
	Int32 pageIndex;
	// If this page is no longer around (or never was), we don't have to remove it.
	if ( !m_pageIndices.Find( page, pageIndex ) || pageIndex == -1 )
	{
		return false;
	}

	m_pages->SetSelection( pageIndex );

	return true;
}

Bool CEdMeshEditor::RemovePage( EEdMeshEditorPage page )
{
	Int32 pageIndex;
	// If this page is no longer around (or never was), we don't have to remove it.
	if ( !m_pageIndices.Find( page, pageIndex ) || pageIndex == -1 )
	{
		return false;
	}

	// Remove the page
	m_pages->DeletePage( pageIndex );

	// Adjust page indices to account for the change.
	for ( Int32 i = 0; i < MEP_MeshEditorPagesNum; ++i )
	{
		Int32 *valuePtr = m_pageIndices.FindPtr( (EEdMeshEditorPage)i );
		if ( valuePtr )
		{
			// The removed page gets index -1.
			if ( *valuePtr == pageIndex )
			{
				*valuePtr = -1;
			}
			// If this page is after the removed page, its index must be decremented.
			else if ( *valuePtr > pageIndex )
			{
				(*valuePtr)--;
			}
		}
	}
	return true;
}

Bool CEdMeshEditor::IsPageSelected( EEdMeshEditorPage page )
{
	Int32 pageIndex;
	if ( !m_pageIndices.Find( page, pageIndex ) || pageIndex == -1 )
	{
		return false;
	}
	return pageIndex == m_pages->GetSelection();
}

void CEdMeshEditor::OnExportRE( wxCommandEvent& event )
{
	if ( CMesh* mesh = Cast< CMesh >( m_mesh ) )
	{
		wxFileDialog saveFileDialog( this, TXT("Export mesh to RE"), wxEmptyString, wxEmptyString, "*.re", wxFD_SAVE );
		if ( saveFileDialog.ShowModal() == wxID_OK )
		{
			String path = String( saveFileDialog.GetPath().c_str() );
			ExportToREFile( path );
		}
	}
}

void CEdMeshEditor::OnAddMesh( wxCommandEvent& event )
{
	if ( CMesh* mesh = Cast< CMesh >( m_mesh ) )
	{
		String addedMeshPath;
		if ( GetActiveResource( addedMeshPath, ClassID< CMesh >() ) )
		{
			if ( CDiskFile* addedMeshFile = GDepot->FindFile( addedMeshPath ) )
			{
				if ( THandle< CMesh > addedMesh = Cast< CMesh >( addedMeshFile->Load() ) )
				{
					mesh->AddMesh( *addedMesh, Matrix::IDENTITY, false );
					UpdateMaterialList();
					UpdateLODList( true );
					UpdateMeshStats();
					RefreshPreviewRenderingProxy();
					RefreshPreviewBBoxInfo();
				}
			}
			return;
		}

		GFeedback->ShowError( TXT("Select valid mesh in the AssetBrowser first") );
	}
}

void CEdMeshEditor::OnExportFbx( wxCommandEvent& event )
{
	if ( m_mesh == nullptr )
	{
		wxMessageBox( wxT("Nothing to export. Load something."), wxT("Mesh export"), wxOK | wxICON_ERROR );
		return;
	}

	static Uint32 lodChosen = 0;
	if ( wxDynamicCast( event.GetEventObject(), wxButton ) != nullptr )
	{
		lodChosen =  m_lodList->GetSelectedLODIndex();
	}
	else
	{
		String dialogContents = TXT( "R(" );
		for ( Uint32 i = 0; i < m_mesh->GetNumLODLevels(); ++i )
		{
			dialogContents += String::Printf( TXT( "'%d'" ), i );
		}
		dialogContents += TXT( ")|H{~B@'&OK'|B'&Cancel'}" );

		int button = FormattedDialogBox( wxT("Export Mesh"), dialogContents.AsChar(), &lodChosen );
		if ( button != 0 )
		{
			return;
		}
	}

	IExporter* exporter = IExporter::FindExporter( ClassID< CMesh >(), TXT("fbx") );
	if ( !exporter )
	{
		wxMessageBox( wxT("No exporters found"), wxT("Scene export"), wxOK | wxICON_ERROR );
		return;
	}

	TDynArray< CFileFormat > formats;
	exporter->EnumExportFormats( ClassID< CMesh >(), formats );

	// Ask for file name
	CFilePath meshFilePath( m_mesh->GetDepotPath() );
	CEdFileDialog exportFileDialog;

	// Configure saver
	exportFileDialog.AddFormats( formats );
	exportFileDialog.SetIniTag( TXT("Mesh Exporter") );
	exportFileDialog.SetMultiselection( false );

	if ( !exportFileDialog.DoSave( (HWND) GetHWND() ) )
	{
		wxMessageBox( wxT("No mesh exporters found"), wxT("Mesh Exporter"), wxOK | wxICON_ERROR );
		return;
	}

	if ( lodChosen >= 0 && lodChosen < m_mesh->GetNumLODLevels() )
	{
		// Setup
		IExporter::ExportOptions exportOptions;
		exportOptions.m_resource = m_mesh;
		exportOptions.m_saveFileFormat = exportFileDialog.GetFileFormat();
		exportOptions.m_saveFilePath = exportFileDialog.GetFile();
		exportOptions.m_lodToUse = lodChosen;
		exporter->DoExport( exportOptions );
	}
}

void CEdMeshEditor::OnLODExportRE( wxCommandEvent& event )
{
	if ( CMesh* mesh = Cast< CMesh >( m_mesh ) )
	{
		Int32 idx = m_lodList->GetSelectedLODIndex();
		if ( idx >= 0 )
		{
			wxFileDialog saveFileDialog( this, TXT("Export LOD to RE"), wxEmptyString, wxEmptyString, "*.re", wxFD_SAVE );
			if ( saveFileDialog.ShowModal() == wxID_OK )
			{
				String path = String( saveFileDialog.GetPath().c_str() );
				ExportToREFile( path, idx );
			}
		}
	}
}

void CEdMeshEditor::OnShowTBN( wxCommandEvent& event )
{
	wxToolBar* toolbar = XRCCTRL( *this, "Tools", wxToolBar );
	const Bool status = toolbar->GetToolState( XRCID("toolShowTBN") );
	m_preview->ShowTBN( status );
}

void CEdMeshEditor::OnDataErrorReported( const SDataError& error )
{
#ifndef NO_DATA_VALIDATION

	if ( m_dependentResourcesPaths.Exist( error.m_resourcePath ) )
	{
		m_dataErrors.InsertUnique( error );
	}

#endif
}

void CEdMeshEditor::GetDataErrors( TDynArray< String >& arrayForErrors )
{
	for ( Uint32 i = 0; i < m_dataErrors.Size(); ++i )
	{
		const String& error = m_dataErrors[i].ToString();
		arrayForErrors.PushBackUnique( error );
	}
}

void CEdMeshEditor::AddDataErrors( const TDynArray< SDataError >& dataErrors )
{
	m_dataErrors.PushBackUnique( dataErrors );
	m_dataErrors.Sort();
}

IEditorPreviewCameraProvider::Info CEdMeshEditor::GetPreviewCameraInfo() const
{
	IEditorPreviewCameraProvider::Info res;
	res.m_cameraPostion  = m_preview->GetCameraPosition();
	res.m_cameraRotation = m_preview->GetCameraRotation();
	res.m_cameraFov      = m_preview->GetCameraFov();
	res.m_lightPosition  = m_preview->GetLightPosition();
	res.m_envPath		 = TXT("environment\\definitions\\") + m_preview->m_envChoice->GetString( m_preview->m_envChoice->GetSelection() );
	return res;
}


// void CEdMeshEditor::OnVertexPaintLodSelected( wxCommandEvent& event )
// {
// 	if ( IsPageSelected( MEP_VertexPaint ) )
// 	{
// 		Int32 selectedLOD = m_vertexPaintLod->GetValue();
// 		m_preview->OverrideViewLOD( selectedLOD );
// 	}
// 	else
// 	{
// 		m_preview->OverrideViewLOD( -1 );
// 	}
// }

void CEdMeshEditor::UpdateMaterialList()
{
	if ( m_materialList )
	{
		m_materialList->UpdateMaterialList();
	}

}

void CEdMeshEditor::UpdateChunkMaterialsChoice()
{
	if ( m_chunkMaterials && m_lodList )
	{
		m_chunkMaterials->Clear();
		for ( Uint32 i=0; i<m_mesh->GetMaterials().Size(); ++i )
		{
			m_chunkMaterials->AppendString( m_lodList->CreateChunkMaterialName( i ).AsChar() );
		}
	}
}

void CEdMeshEditor::UpdateLODList( Bool preserveExpansionState )
{
	if ( m_lodList )
	{
		m_lodList->UpdateList( preserveExpansionState );
	}

	UpdateMeshStats(); // modifying chunks affects the stats

	if ( m_preview->m_vertexPaintTool != nullptr )
	{
		m_preview->m_vertexPaintTool->LodsChanged();
	}
}

void CEdMeshEditor::OnActivate( wxActivateEvent& event )
{
	if ( event.GetActive() )
	{
		if ( wxTheFrame )
		{
			wxTheFrame->SetUndoHistoryFrameManager( m_undoManager, String( GetTitle().c_str() ) );
		}
	}

	event.Skip();
}

void CEdMeshEditor::ExportToREFile( const String& path, Int32 lodLevelIdx /*= -1*/ ) const
{
	if( const CMesh* mesh = Cast<CMesh>(m_mesh) )
	{
		ReFileArchive archive;

		// note: archive becomes an owner of the buffer
		ReFileBuffer* bh = archive.Append( new ReFileBuffer( 'hed2', 0 ) );

		const AnsiChar* rawPath = UNICODE_TO_ANSI( path.AsChar() );

		ReFileHeader2 hedr;
		hedr.set( "You are author", "Red Engine", rawPath, "1", "2", "3", "4" );
		hedr.write( bh );

		const CMesh::TLODLevelArray& lodLevels = mesh->GetMeshLODLevels();

		if ( lodLevelIdx >= 0 )
		{
			// Export specific level
			if ( lodLevelIdx < lodLevels.SizeInt() )
			{
				MeshReExport::ExportLODToRE( archive, lodLevelIdx, lodLevels[ lodLevelIdx ], *mesh );
			}
		}
		else
		{
			// Export all the levels
			for ( Int32 idx = 0; idx < lodLevels.SizeInt(); ++idx )
			{
				MeshReExport::ExportLODToRE( archive, lodLevelIdx, lodLevels[ idx ], *mesh );
			}
		}
		archive.Save( rawPath );
	}
}
