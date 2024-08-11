/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once


#include "../../common/engine/mesh.h"

class IMaterial;
class CBitmapTexture;
class CTextureArray;

// vertexFactoryMeshStatic.fx -> check for VS_INPUT
struct IntermediateVStream
{
	Float	m_position[2];			// UV.xy * TargetTexSize.xy
	Float	m_uv0[2];				// First uv set
	Float	m_normal[4];			// Vertex normal
	Float	m_tangent[4];			// Vertex tangent
	Float	m_color[4];				// Vertex color
	Float	m_uv1[2];				// Second uv set
	Float	m_pad[2];
};

#ifdef USE_SIMPLYGON

#include "../../common/engine/simplygonHelpers.h"


// Utility for generating a proxy mesh for an entity
class CEdEntityMeshGenerator
{
public:
	struct Settings
	{
		Uint32	m_mergeDistance;			//!< Distance, in pixels, for vertices to be merged.
		Uint32	m_gutterSpace;				//!< Minimum space between charts
		Uint32	m_dilation;					//!< How much to extend edges in generated textures
		Uint32	m_multisampling;			//!< 1-8 -- Amount of multisampling
		Uint32	m_textureSize;				//!< Hint for texture size. 0 to choose automatically based on screen size

		// TODO : These two are kind of related, probably possible to just provide one
		Uint32	m_screenSize;				//!< 40-1200 -- Expected size of the generated mesh on-screen, in pixels
		Float	m_showDistance;				//!< Distance at which the generated mesh should become visible

		Float	m_generateDistance;			//!< Reference distance for deciding which meshes/lods to use

		Float	m_autohideDistance;			//!< Auto-hide distance for the generated mesh
		
		Bool	m_useCullingPlane;			//!< Use culling plane to remove inside faces
		Float	m_cullingPlaneOffset;		//!< offset the culling plane relative to the bounding box

		Uint32	m_hardEdgeAngle;			//!< Hard edge angle used for normal recalculation

		Settings()
			: m_mergeDistance( 5 )
			, m_gutterSpace( 1 )
			, m_dilation( 6 )
			, m_multisampling( 2 )
			, m_textureSize( 512 )
			, m_screenSize( 99 )
			, m_showDistance( 70.0f )
			, m_generateDistance( 0.0f )
			, m_autohideDistance( 2000.0f )
			, m_useCullingPlane( true )
			, m_cullingPlaneOffset( 0.f )
			, m_hardEdgeAngle( 40 )
		{}
	};

private:
	SimplygonSDK::ISimplygonSDK*				m_simplygon;
	SimplygonSDK::spScene						m_scene;
	SimplygonSDK::spMaterialTable				m_sourceMaterialTable;
	SimplygonSDK::spTextureTable				m_sourceTextureTable;

	TDynArray< SimplygonSDK::spGeometryData >	m_originalGeometry;			// Remeshing clobbers the scene and geometry, so we keep a backup.
	TDynArray< ::IMaterial* >					m_geometryMtlMap;

	TDynArray< CMeshComponent* >				m_sourceMeshComponents;
	Box											m_sourceMeshBounds;

	CMesh*										m_generatedMesh;
	TSortedMap< StringAnsi, CBitmapTexture* >	m_generatedTextures;
	CMesh::FactoryInfo							m_meshFactoryInfo;

	TDynArray< StringAnsi >						m_textureNames;

	String										m_errorMessage;

	Settings									m_settings;

	struct MaterialTexture
	{
		StringAnsi					m_texturePath;
		SimplygonSDK::spImageData	m_image;

		MaterialTexture() {}
		MaterialTexture( const StringAnsi& texturePath, const SimplygonSDK::spImageData& image )
			: m_texturePath( texturePath )
			, m_image( image )
		{}
	};

	TDynArray< MaterialTexture >				m_materialTextures;


	CEntityTemplate*							m_entityTemplate;

public:
	CEdEntityMeshGenerator( CEntityTemplate* templ );
	~CEdEntityMeshGenerator();

	void Reset();

	// Prepare Simplygon scene, gather list of texture names.
	Bool Prepare( CEntity* entity );

	// Calculate an estimate for the default show distance, based on the components that were gathered during Prepare.
	Float EstimateShowDistance();
	// Calculate an estimate for the default screen size, based on the components that were gathered during Prepare and a given show distance.
	Uint32 EstimateScreenSize( Float showDistance );

	// Give settings to generator. Should be called before ReCollect, Process, etc. May modify settings to ensure proper values.
	// Returns the settings that will actually be used.
	Settings ProvideSettings( const Settings& settings );


	// Prepare Simplygon scene based on current settings.
	Bool ReCollect( CEntity* entity );
	// Generate mesh and texture data.
	Bool Process();

	// Check if the files that would be saved by SaveFiles() exist. Provides list of files that do exist.
	void CheckFilesExist( const String& directory, const String& baseName, const TDynArray< StringAnsi >& texturesToUse, TDynArray< String >& outFiles );

	// Write out mesh and textures. Will overwrite files if they exist.
	Bool SaveFiles( const String& directory, const String& baseName, const TDynArray< StringAnsi >& texturesToUse );

	// Get the names of the texture channels that are used by the meshes. This is valid after a successful Prepare().
	const TDynArray< StringAnsi >& GetTextureNames() { return m_textureNames; }

	// Get the mesh that was created. This is valid after a successful SaveFiles().
	CMesh* GetGeneratedMesh() { return m_generatedMesh; }

	// Get an error message, if one of the processing functions failed.
	const String& GetErrorMessage() const { return m_errorMessage; }

private:
	Bool CollectScene();
	SimplygonSDK::spMappingImage Remesh();
	Bool CastTextures( SimplygonSDK::spMappingImage mappingImage );
	Bool BuildMeshInfo( CMesh::FactoryInfo& outFactoryInfo );

	static void CollectTextureNames( SimplygonSDK::spMaterialTable materialTable, TDynArray< StringAnsi >& outTextureNames );

	SimplygonSDK::spMaterial InitSimplygonMaterial( IMaterial* engineMtl );
	StringAnsi GetTextureName( const StringAnsi& channel, CBitmapTexture* texture );
	StringAnsi FindMaterialTextureName( const StringAnsi& channel, CBitmapTexture* texture );
	SimplygonSDK::spImageData FindMaterialTexture( const StringAnsi& channel, CBitmapTexture* texture );
	SimplygonSDK::spShadingNode SampleTextureArray( const StringAnsi& channel, CTextureArray* tex, const Vector* color = nullptr, const Float colorizeBottom = 0.f, const Float colorizeTop = 0.f );
	//SimplygonSDK::spImageData SampleTextureArray_Debug( const StringAnsi& channel, CTextureArray* tex );

	CBitmapTexture* CastDiffuse( SimplygonSDK::spMappingImage mappingImage );

};

#endif


class CEdEntityMeshGeneratorSettingsDlg : public wxDialog 
{
	DECLARE_EVENT_TABLE();

public:
	CEdEntityMeshGeneratorSettingsDlg( wxWindow *parent );


	String GetOutputFolder() const;
	void SetOutputFolder( const String& path );

	String GetBaseName() const;
	void SetBaseName( const String& baseName );

	void AddTexture( const StringAnsi& name, Bool saveByDefault );
	Bool GetTextureSave( const StringAnsi& name ) const;
	void GetTexturesToSave( TDynArray< StringAnsi >& outTextures ) const;

	CEdEntityMeshGenerator::Settings GetSettings() const;
	void SetSettings( const CEdEntityMeshGenerator::Settings& settings );


private:
	Uint32 GetSelectedTextureSize() const;

	void OnCancel( wxCommandEvent& event );
	void OnGenerate( wxCommandEvent& event );

	void OnTextureSizeChanged( wxCommandEvent& event );
};


class CEdEntityProxySetupDlg : public wxDialog
{
	DECLARE_EVENT_TABLE();

public:
	CEdEntityProxySetupDlg( CEdEntityEditor *parent );
		
	TDynArray< CMeshComponent* >				m_meshComponents;			// All the mesh components inside this entity	
	TDynArray< CMeshComponent* >				m_proxyMeshComponents;	// In theory it should be just one mesh, but who knows

	Bool Insert( CMeshComponent* mesh );

private:	
	wxCheckListBox*			m_meshes;
	wxCheckListBox*			m_proxy;

	wxCheckListBox*			m_meshesStatus;
	wxCheckListBox*			m_proxyStatus;
	CEdEntityEditor*		m_parent;

	void OnCancel( wxCommandEvent& event );
	void OnProcess( wxCommandEvent& event );
	void OnMouseDblClick( wxCommandEvent& event);
};

