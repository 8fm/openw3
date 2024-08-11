/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/engine/renderSettings.h"
#include "../../common/engine/environmentAreaParams.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/world.h"
#include "gameTime.h"

// forward declarations
class CRenderFrameInfo;

enum EMinimapMask
{
	MM_Full,
	MM_Albedo,
	MM_Normals,
	MM_NormalsNoTrees,
	MM_Water,
	MM_Foliage,
	MM_Volumes,
	MM_Objects,
	MM_Heightmap,
	MM_Terrain,
	MM_Roads,
	MM_Bridges,

	// debug masks
	MM_DEBUG_GrassInstanceHeatMap,
	MM_DEBUG_TreeInstanceHeatMap,
	MM_DEBUG_GrassLayerHeatMap,

	MM_Count
};
String ToString( enum EMinimapMask value );

template<> RED_INLINE Bool FromString( const String& text, EMinimapMask& value )
{
	value = MM_Count;

	if( text == TXT("Full") )
	{
		value = MM_Full;
	}
	else if( text == TXT("Albedo") )
	{
		value = MM_Albedo;
	}
	else if( text == TXT("Normals") )
	{
		value = MM_Normals;
	}
	else if( text == TXT("NormalsNoTrees") )
	{
		value = MM_NormalsNoTrees;
	}
	else if( text == TXT("Water") )
	{
		value = MM_Water;
	}
	else if( text == TXT("Foliage") )
	{
		value = MM_Foliage;
	}
	else if( text == TXT("Volumes") )
	{
		value = MM_Volumes;
	}
	else if( text == TXT("Objects") )
	{
		value = MM_Objects;
	}
	else if( text == TXT("Heightmap") )
	{
		value = MM_Heightmap;
	}
	else if( text == TXT("Terrain") )
	{
		value = MM_Terrain;
	}
	else if( text == TXT("Roads") )
	{
		value = MM_Roads;
	}
	else if( text == TXT("Bridges") )
	{
		value = MM_Bridges;
	}
	else if( text == TXT("DEBUG_GrassInstanceHeatMap") )
	{
		value = MM_DEBUG_GrassInstanceHeatMap;
	}
	else if( text == TXT("DEBUG_TreeInstanceHeatMap") )
	{
		value = MM_DEBUG_TreeInstanceHeatMap;
	}
	else if( text == TXT("DEBUG_GrassLayerHeatMap") )
	{
		value = MM_DEBUG_GrassLayerHeatMap;
	}
	else
	{
		return false;
	}

	return true;
};


enum EDirLayout
{
	DL_Default,
	DL_Photoshop,

	DL_Count
};
String ToString( enum EDirLayout value );

enum EGenerationMode
{
	GM_CurrentTile,
	GM_CurrentTilePlus,
	GM_TileRange,
	GM_AllTiles,

	GM_Count
};
String ToString( enum EGenerationMode value );

//////////////////////////////////////////////////////////////////////////
// Struct contains all settings needed to correct render minimaps
struct SMinimapExteriorSettings
{
	Bool			m_enabledMasks[MM_Count];
	EDirLayout		m_dirLayout;
	Uint32			m_imageSize;
	Uint32			m_imageOffset;
	Uint32			m_imageZoom;
	String			m_fileNamePrefix;
	String			m_envSettingsPath;
	Vector			m_cameraPosition;
	Bool			m_continueMode;

	SMinimapExteriorSettings();
};

struct SMinimapSettings
{
	String			m_outputDir;
	EGenerationMode	m_generationMode;
	Uint32			m_tileCountPlus;
	Box2			m_tileRange;

	SMinimapExteriorSettings	m_exteriors;

	SMinimapSettings();
};

//////////////////////////////////////////////////////////////////////////
// 
class CMinimapGenerator
{
	struct SEngineSettingsBackup
	{
		THandle< CEnvironmentDefinition >	m_envDefinition;
		THandle< CEnvironmentDefinition >	m_scenesEnvDefinition;
		SWorldSkyboxParameters				m_skyboxParams;
		CGameEnvironmentParams				m_gameEnvParams;
		Bool								m_asyncMaterialCompilation;
		Bool								m_umbraOcclusion;
		Bool								m_skyActivated;
		Bool								m_skyActivatedActiveFactor;
		Float								m_skyActivateFactor;

		// PhysX
		Bool m_dontCreateTrees;
		Bool m_dontCreateRagdolls;
		Bool m_dontCreateDestruction;
		Bool m_dontCreateDestructionOnGPU;
		Bool m_dontCreateCloth;
		Bool m_dontCreateClothOnGPU;
		Bool m_dontCreateClothSecondaryWorld;
		Bool m_dontCreateParticles;
		Bool m_dontCreateParticlesOnGPU;

		GameTime m_gameTime;
	};

public:
	CMinimapGenerator();
	~CMinimapGenerator();

	void SetSettings( const SMinimapSettings& settings );
	const SMinimapSettings& GetSettings() const;

	Bool IsUnsuccessfulFileExist( const String& destinationDir, Int32& x, Int32& y );

	void GenerateExteriors();
	void GenerateInteriors();

	RED_INLINE String GetInteriorsErrors() const { return m_interiorsErrors; }

	// new way of generation interior maps
	void GatherAllEntityTemplatesWithInterior( TDynArray< String >& paths, const TDynArray<String>& specifiedEntities );
	void CreateInstances( const TDynArray< String >& paths, TDynArray< CEntity* >& entities );
	void CreateNavmesh( TDynArray< CEntity* >& entities );
	void CreateNewInteriors();
	String GetFoundEntityTemplateCount() const;
	String GetCreatedInstanceCount() const;

private:
	// Manage viewport
	void PrepareViewport( Uint32 viewportSize );
	void DestroyViewport();

	// Manage world
	void RememberEngineSettings();
	void SetNewEngineSettings();
	void RevertEngineSettings();

	// Manage render frame
	CRenderFrame* PrepareRenderFrame( enum EMinimapMask mask, const Vector& cameraPos, Float tileOffset );
	void SetDefaultSettingsForRenderFrame( CRenderFrameInfo* frameInfo );
	void SetRenderFrameSettingsForActiveMask( CRenderFrameInfo* frameInfo, enum EMinimapMask mask );
	void SetRenderFrameSettingsAfterGenerateFrame( CRenderFrameInfo* frameInfo );

	// Helper functions
	void CountTilesToRender();
	String CreatePathForTileImage( EMinimapMask  maskType, Int32 x, Int32 y );
	void PrepareObjects( EMinimapMask mask );
	void FilterAppropriateObjects( const String& tagName, Bool shouldBeVisible );

	// Main function to generate mini maps for all tiles and for selected masks
	void InternalGenerateExteriors();
	void InternalGenerateInteriors();

	// save and revert information about crashes during generation
	Bool CheckUnsuccessfulFile( Int32& x, Int32& y );
	void SaveUnsuccessfulFile( Int32 x, Int32 y );
	void RemoveUnsuccessfulFile();

	// interiors 
	void CollectAndDrawNavmeshes( CRenderFrame* frame, const Box& renderArea );
	String CreatePathForInteriorImage( const String& caveName );
	CRenderFrame* PrepareRenderFrameForInterior( const Vector& camPos, Float zoom );
	void RenderNavMeshForInterior( const String& name, const Box& renderArea, const Vector& cameraPos, const Vector& componentPos );
	void RevertObjectsVisibility();

private:
	ViewportHandle		m_viewport;
	Uint32				m_viewportSize;

	Int32				m_tileStartX;
	Int32				m_tileStartY;
	Int32				m_tileEndX;;
	Int32				m_tileEndY;
	Float				m_tileSize;
	Float				m_highestElevation;

	CWorld*				m_world;
	CClipMap*			m_terrain;

	// helpers
	SMinimapSettings		m_settings;
	SEngineSettingsBackup	m_engineSettingsBackup;
	String					m_interiorsErrors;

	Bool					generationEnabled;

	IFeedbackSystem*		m_tempHandlerFeedback;

};
