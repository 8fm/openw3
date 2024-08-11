/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiWindow.h"

namespace DebugWindows
{
	class CDebugWindowSceneStats : public RedGui::CRedGuiWindow
	{

	public:
		CDebugWindowSceneStats();
		~CDebugWindowSceneStats();

	private:
		void	OnWindowOpened( CRedGuiControl* control );
		void	OnWindowClosed( CRedGuiControl* control );
		void	NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime );
		void	NotifyButtonClicked( RedGui::CRedGuiEventPackage& eventPackage );
		void	AddField( RedGui::CRedGuiGroupBox* parent , const String& labelname , RedGui::CRedGuiProgressBar** progressbar , RedGui::CRedGuiProgressBar** progressbartime = nullptr , Bool useTimeBar = false );
		void	AddDescriptionField( RedGui::CRedGuiGroupBox* parent , const String& labelname1 , const String& labelname2 , const String& labelname3);
		void	DisplayStat( RedGui::CRedGuiProgressBar* progressBar , Uint64 inValue , Uint32 maxAllowed , Bool isMemorySize , RedGui::CRedGuiProgressBar* progressBarTime = nullptr , Float inTime = 0, Float maxAllowedTime = 0 );
		void	PrepareStatBoxes( const TDynArray<String>& counters , TDynArray<CProfilerStatBox>& statboxes , Int32 root);
		Float	CountTime( TDynArray<CProfilerStatBox>& statboxes );
		Float	GetGpuTime( const String& name, const String& parentName = String::EMPTY );

		Float							m_timeSinceLastRetrival;
		SceneRenderingStats				m_renderingStats;
		SSpeedTreeResourceMetrics		m_speedTreeStats;
		GeneralStats					m_meshStats;
		GeneralStats					m_textureStats;

	private:
		//===DUMP BUTTONS
		RedGui::CRedGuiButton*			m_makeMeshesDumpFile;

		//===PERFORMANCE BARS
		RedGui::CRedGuiProgressBar*		m_apexTickedProgressBar;		// Apex cloth		
		RedGui::CRedGuiProgressBar*		m_apexDestroTickedProgressBar;	// Apex Destructibles
		RedGui::CRedGuiProgressBar*		m_lightsShadowProgressBar;		// Shadow casting lights
		RedGui::CRedGuiProgressBar*		m_lightsNonShadowsProgressBar;	// Non Shadow Casting lights
		RedGui::CRedGuiProgressBar*		m_decalsSSProgressBar;			// Screen space decals
		RedGui::CRedGuiProgressBar*		m_decalsDynamicProgressBar;		// Dynamic Decals
		RedGui::CRedGuiProgressBar*		m_particlesCountProgressBar;	// Number of particles
		RedGui::CRedGuiProgressBar*		m_hiresChunksProgressBar;		// Hires Chunks
		RedGui::CRedGuiProgressBar*		m_speedTreeProgressBar;			// Speed tree meshes
		RedGui::CRedGuiProgressBar*		m_grassProgressBar;
		RedGui::CRedGuiProgressBar*		m_chunksStaticProgressBar;		// Static geometry chunks
		RedGui::CRedGuiProgressBar*		m_trianglesStaticProgressBar;	// Static geometry triangles
		RedGui::CRedGuiProgressBar*		m_chunksSkinnedProgressBar;		// Dynamic geometry chunks
		RedGui::CRedGuiProgressBar*		m_trianglesSkinnedProgressBar;	// Dynamic geometry triangles

		//===PERFORMANCE TIMES BARS
		RedGui::CRedGuiProgressBar*		m_apexTickedTimeProgressBar;		// Apex cloth		
		RedGui::CRedGuiProgressBar*		m_apexDestroTickedTimeProgressBar;	// Apex Destructibles
		RedGui::CRedGuiProgressBar*		m_lightsShadowTimeProgressBar;		// Shadow casting lights
		RedGui::CRedGuiProgressBar*		m_lightsNonShadowsTimeProgressBar;	// Non Shadow Casting lights
		RedGui::CRedGuiProgressBar*		m_decalsSSTimeProgressBar;			// Screen space decals
		RedGui::CRedGuiProgressBar*		m_decalsDynamicTimeProgressBar;		// Dynamic Decals
		RedGui::CRedGuiProgressBar*		m_particlesCountTimeProgressBar;	// Number of particles
		RedGui::CRedGuiProgressBar*		m_hiresChunksTimeProgressBar;		// Hires Chunks
		RedGui::CRedGuiProgressBar*		m_speedTreeTimeProgressBar;			// Speed tree
		RedGui::CRedGuiProgressBar*		m_grassTimeProgressBar;
		RedGui::CRedGuiProgressBar*		m_chunksStaticTimeProgressBar;		// Static geometry chunks
		RedGui::CRedGuiProgressBar*		m_chunksSkinnedTimeProgressBar;		// Dynamic geometry chunks


		//===MEMORY BARS
		RedGui::CRedGuiProgressBar*		m_DynMeshesProgressBar;			// Size of Dynamic meshes
		RedGui::CRedGuiProgressBar*		m_StatMeshesProgressBar;		// Size of Static meshes
		RedGui::CRedGuiProgressBar*		m_CharTexturesProgressBar;		// Size of Characters textures
		RedGui::CRedGuiProgressBar*		m_StatTexturesProgressBar;		// Size of Static textures
	
		TDynArray<CProfilerStatBox>		m_apexTickedStatboxes;		
		TDynArray<CProfilerStatBox>		m_apexDestroTickedStatboxes;
		TDynArray<CProfilerStatBox>		m_lightsShadowStatboxes;		
		TDynArray<CProfilerStatBox>		m_lightsNonShadowsStatboxes;
		TDynArray<CProfilerStatBox>		m_decalsSSStatboxes;			
		TDynArray<CProfilerStatBox>		m_decalsDynamicStatboxes;		
		TDynArray<CProfilerStatBox>		m_particlesCountStatboxes;
		TDynArray<CProfilerStatBox>		m_hiresChunksStatboxes;		
		TDynArray<CProfilerStatBox>		m_speedTreeStatboxes;
		TDynArray<CProfilerStatBox>		m_grassStatboxes;
		TDynArray<CProfilerStatBox>		m_staticGeometryStatboxes;
		TDynArray<CProfilerStatBox>		m_dynamicGeometryStatboxes;

		//===COUNT BUDGETS
		Uint32	m_MaxAllowedApexTicked;		
		Uint32	m_MaxAllowedApexDestroTicked;	
		Uint32	m_MaxAllowedLightsShadow;		
		Uint32	m_MaxAllowedLightsNonShadows;	
		Uint32	m_MaxAllowedDecalsSS;			
		Uint32	m_MaxAllowedDecalsDynamic;	
		Uint32	m_MaxAllowedParticlesCount;
		Uint32	m_MaxAllowedActiveEnvProbes;	
		Uint32	m_MaxAllowedHiresChunks;
		Uint32	m_MaxAllowedSpeedTree;
		Uint32	m_MaxAllowedGrass;
		Uint32	m_MaxAllowedChunksStatic;		
		Uint32	m_MaxAllowedTrianglesStatic;	
		Uint32	m_MaxAllowedChunksSkinned;	
		Uint32	m_MaxAllowedTrianglesSkinned;	
		Uint32	m_MaxAllowedDynMeshes;		
		Uint32	m_MaxAllowedStatMeshes;		
		Uint32	m_MaxAllowedCharTextures;		
		Uint32	m_MaxAllowedStatTextures;		

		//===TIME BUDGETS
		Float	m_MaxAllowedApexTickedTime;		
		Float	m_MaxAllowedApexDestroTickedTime;	
		Float	m_MaxAllowedLightsShadowTime;		
		Float	m_MaxAllowedLightsNonShadowsTime;	
		Float	m_MaxAllowedDecalsSSTime;			
		Float	m_MaxAllowedDecalsDynamicTime;	
		Float	m_MaxAllowedParticlesCountTime;
		Float	m_MaxAllowedHiresChunksTime;
		Float	m_MaxAllowedSpeedTreeTime;
		Float	m_MaxAllowedGrassTime;
		Float	m_MaxAllowedChunksStaticTime;		
		Float	m_MaxAllowedTrianglesStaticTime;	
		Float	m_MaxAllowedChunksSkinnedTime;	
		Float	m_MaxAllowedTrianglesSkinnedTime;

		TDynArray<SceneRenderingStats::GpuTimesStat> m_gpuEntries;
	};
}
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
