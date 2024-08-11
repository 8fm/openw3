/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiManager.h"
#include "redGuiLabel.h"
#include "redGuiPanel.h"
#include "redGuiProgressBar.h"
#include "redGuiGroupBox.h"
#include "redGuiButton.h"
#include "redGuiGridLayout.h"
#include "../core/configFileManager.h"
#include "renderer.h"
#include "renderSettings.h"
#include "game.h"
#include "renderProxy.h"
#include "world.h"
#include "materialInstance.h"
#include "../core/redProfiler.h"
#include "debugWindowFrameStats.h"

namespace DebugWindows
{
	CDebugWindowSceneStats::CDebugWindowSceneStats()
	: RedGui::CRedGuiWindow( 300, 300, 440, 500 )	
	, m_timeSinceLastRetrival( 0.0f )
	{
		GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindowSceneStats::NotifyOnTick );

		SetCaption( TXT("Scene Stats") );
		
		// add progress bars
		{
			Uint32 fieldsCountPerformance = 15;
			RedGui::CRedGuiGroupBox* groupPerformance = new RedGui::CRedGuiGroupBox( 0, 0, 440, (fieldsCountPerformance + 1) * 20 );
			groupPerformance->SetDock( RedGui::DOCK_Top );
			groupPerformance->SetText( TXT("Performance") );
			
			this->AddChild( groupPerformance );
			{
				AddDescriptionField( groupPerformance, TXT("Category") , TXT("Count") , TXT("Time Taken"));
				AddField( groupPerformance , TXT("Apex Cloth") , &m_apexTickedProgressBar , &m_apexTickedTimeProgressBar , true );
				AddField( groupPerformance , TXT("Apex Destr") , &m_apexDestroTickedProgressBar , &m_apexDestroTickedTimeProgressBar , true );
				AddField( groupPerformance , TXT("Lights + shadows") , &m_lightsShadowProgressBar , &m_lightsShadowTimeProgressBar , true );
				AddField( groupPerformance , TXT("Lights no shadows") , &m_lightsNonShadowsProgressBar , &m_lightsNonShadowsTimeProgressBar , true );
				AddField( groupPerformance , TXT("SS Decals") , &m_decalsSSProgressBar , &m_decalsSSTimeProgressBar , true );
				AddField( groupPerformance , TXT("Dynamic Decals") , &m_decalsDynamicProgressBar , &m_decalsDynamicTimeProgressBar , true );
				AddField( groupPerformance , TXT("Particles") , &m_particlesCountProgressBar , &m_particlesCountTimeProgressBar , true );
				AddField( groupPerformance , TXT("HiRes Chunks") , &m_hiresChunksProgressBar , &m_hiresChunksTimeProgressBar , true );
				AddField( groupPerformance , TXT("SpeedTree") , &m_speedTreeProgressBar , &m_speedTreeTimeProgressBar , true );
				AddField( groupPerformance , TXT("Grass") , &m_grassProgressBar , &m_grassTimeProgressBar , true );
				AddField( groupPerformance , TXT("Static Chunks") , &m_chunksStaticProgressBar , &m_chunksStaticTimeProgressBar , true );
				AddField( groupPerformance , TXT("Static Tris") , &m_trianglesStaticProgressBar );
				AddField( groupPerformance , TXT("Skinned Chunks") , &m_chunksSkinnedProgressBar , &m_chunksSkinnedTimeProgressBar , true );
				AddField( groupPerformance , TXT("Skinned Tris") , &m_trianglesSkinnedProgressBar );
			}

			Uint32 fieldsCountMemory = 5;
			RedGui::CRedGuiGroupBox* groupMemory = new RedGui::CRedGuiGroupBox( 0, 0, 500, (fieldsCountMemory + 1) * 20 );
			groupMemory->SetDock( RedGui::DOCK_Top );
			groupMemory->SetText( TXT("Memory") );
			this->AddChild( groupMemory );
			{
				AddDescriptionField( groupMemory, TXT("Category") , TXT("Size") , TXT(""));
				AddField( groupMemory , TXT("Char Meshes") , &m_DynMeshesProgressBar );
				AddField( groupMemory , TXT("Enviro Meshes") , &m_StatMeshesProgressBar );
				AddField( groupMemory , TXT("Char Textures") , &m_CharTexturesProgressBar );
				AddField( groupMemory , TXT("Enviro Textures") , &m_StatTexturesProgressBar );
			}
			
			// create button for make dump file
			m_makeMeshesDumpFile = new RedGui::CRedGuiButton( 325, 50, 300, 25);
			m_makeMeshesDumpFile->SetMargin(Box2(5, 5, 5, 5));
			m_makeMeshesDumpFile->SetText(TXT("Dump visible meshes into file"));
			m_makeMeshesDumpFile->EventButtonClicked.Bind(this, &CDebugWindowSceneStats::NotifyButtonClicked);
			m_makeMeshesDumpFile->SetDock(RedGui::DOCK_Bottom);
			this->AddChild(m_makeMeshesDumpFile);
		}
		
		//Setup Start Values
		m_apexTickedProgressBar->SetProgressPosition( 0.0f );
		m_apexDestroTickedProgressBar->SetProgressPosition( 0.0f );
		m_lightsShadowProgressBar->SetProgressPosition( 0.0f );
		m_lightsNonShadowsProgressBar->SetProgressPosition( 0.0f );
		m_decalsSSProgressBar->SetProgressPosition( 0.0f );
		m_decalsDynamicProgressBar->SetProgressPosition( 0.0f );
		m_particlesCountProgressBar->SetProgressPosition( 0.0f );
		m_hiresChunksProgressBar->SetProgressPosition( 0.0f );
		m_speedTreeProgressBar->SetProgressPosition( 0.0f );
		m_grassProgressBar->SetProgressPosition( 0.0f );
		m_chunksStaticProgressBar->SetProgressPosition( 0.0f );
		m_trianglesStaticProgressBar->SetProgressPosition( 0.0f );
		m_chunksSkinnedProgressBar->SetProgressPosition( 0.0f );
		m_trianglesSkinnedProgressBar->SetProgressPosition( 0.0f );
		m_DynMeshesProgressBar->SetProgressPosition( 0.0f );
		m_StatMeshesProgressBar->SetProgressPosition( 0.0f );
		m_CharTexturesProgressBar->SetProgressPosition( 0.0f );
		m_StatTexturesProgressBar->SetProgressPosition( 0.0f );

		m_apexTickedTimeProgressBar->SetProgressPosition( 0.0f );
		m_apexDestroTickedTimeProgressBar->SetProgressPosition( 0.0f );
		m_lightsShadowTimeProgressBar->SetProgressPosition( 0.0f );
		m_lightsNonShadowsTimeProgressBar->SetProgressPosition( 0.0f );
		m_decalsSSTimeProgressBar->SetProgressPosition( 0.0f );
		m_decalsDynamicTimeProgressBar->SetProgressPosition( 0.0f );
		m_particlesCountTimeProgressBar->SetProgressPosition( 0.0f );
		m_hiresChunksTimeProgressBar->SetProgressPosition( 0.0f );
		m_speedTreeTimeProgressBar->SetProgressPosition( 0.0f );
		m_grassTimeProgressBar->SetProgressPosition( 0.0f );
		m_chunksStaticTimeProgressBar->SetProgressPosition( 0.0f );
		m_chunksSkinnedTimeProgressBar->SetProgressPosition( 0.0f );
	}

	CDebugWindowSceneStats::~CDebugWindowSceneStats()
	{
		/* intentionally empty */
	}

	void CDebugWindowSceneStats::NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime )
	{
		if( GetVisible() && GGame->GetActiveWorld() )
		{				
			m_timeSinceLastRetrival += deltaTime;
			
			m_renderingStats = GGame->GetActiveWorld()->GetRenderSceneEx()->GetRenderStats();						
			GRender->PopulateSpeedTreeMetrics( m_speedTreeStats );

			//=== Set current stats
			Uint32	CurrentApexTicked		=	0;
			Uint32	CurrentApexDestroTicked	=	0;
			Uint32	CurrentLightsShadow		=	0;
			Uint32	CurrentLightsNonShadows	=	0;
			Uint32	CurrentDecalsSS			=	0;
			Uint32	CurrentDecalsDynamic	=	0;
			Uint32	CurrentParticlesCount	=	0;
			Uint32	CurrentHiresChunks		=	0;
			Uint64	CurrentSpeedTree		=	0;
			Uint64	CurrentGrass			=	0;
			Uint32	CurrentChunksStatic		=	0;
			Uint32	CurrentTrianglesStatic	=	0;
			Uint32	CurrentChunksSkinned	=	0;
			Uint32	CurrentTrianglesSkinned	=	0;
			Uint32	CurrentDynMeshes		=	0;
			Uint32	CurrentStatMeshes		=	0;
			Uint32	CurrentCharTextures		=	0;
			Uint32	CurrentStatTextures		=	0;

			Float	CurrentApexTickedTime		=	CountTime(m_apexTickedStatboxes);		
			Float	CurrentApexDestroTickedTime	=	CountTime(m_apexDestroTickedStatboxes);				

			Float	CurrentLightsShadowTime		=	CountTime(m_lightsShadowStatboxes);	
			Float	CurrentLightsNonShadowsTime	=	CountTime(m_lightsNonShadowsStatboxes);
			Float	CurrentDecalsSSTime			=	CountTime(m_decalsSSStatboxes) / 2.0f;		
			Float	CurrentDecalsDynamicTime	=	CountTime(m_decalsDynamicStatboxes);	
			Float	CurrentParticlesCountTime	=	CountTime(m_particlesCountStatboxes);
			Float	CurrentHiresChunksTime		=	CountTime(m_hiresChunksStatboxes);
			Float	CurrentSpeedTreeTime		=	CountTime(m_speedTreeStatboxes);
			Float	CurrentGrassTime			=	CountTime(m_grassStatboxes);
			Float	CurrentChunksStaticTime		=	CountTime(m_staticGeometryStatboxes);
			Float	CurrentChunksSkinnedTime	=	CountTime(m_dynamicGeometryStatboxes);

			m_gpuEntries = m_renderingStats.m_gpuTimeStats;							
				
			Float	CurrentGPULightsShadowTime		=	GetGpuTime(TXT("RenderDynamicShadowmaps"));
			Float	CurrentGPULightsNonShadowsTime	=	GetGpuTime(TXT("LightsDeferred"));
			Float	CurrentGPUDecalsSSTime			=	GetGpuTime(TXT("RenderDecals"));
			Float	CurrentGPUDecalsDynamicTime		=	GetGpuTime(TXT("RenderDynamicDecals"));
			Float	CurrentGPUParticlesCountTime	=	GetGpuTime(TXT("RenderParticles"));
			Float	CurrentGPUHiresChunksTime		=	GetGpuTime(TXT("HiResEntityShadows"));
			Float	CurrentGPUSpeedTreeTime			=	GetGpuTime(TXT("Render3DTrees")) + GetGpuTime(TXT("RenderTreeBillboards"));
			Float	CurrentGPUGrassTime				=	GetGpuTime(TXT("RenderGrass"));
			Float	CurrentGPUChunksStaticTime		=	GetGpuTime(TXT("RenderMeshes"),TXT("RenderStaticSolidDeferredElements"));
			Float	CurrentGPUChunksSkinnedTime		=	GetGpuTime(TXT("RenderMeshes"),TXT("RenderSolidDeferredElements")) + GetGpuTime(TXT("RenderMeshes"),TXT("RenderSolidForwardElements"));
			

			CurrentApexTicked			= m_renderingStats.m_numApexClothsRendered;
			CurrentApexDestroTicked		= m_renderingStats.m_numApexDestructiblesRendered;
			CurrentLightsShadow			= m_renderingStats.m_numLightWithShadows;
			CurrentLightsNonShadows		= m_renderingStats.m_numLights - m_renderingStats.m_numLightWithShadows;
			CurrentDecalsSS				= m_renderingStats.m_numDecalProxies;
			CurrentDecalsDynamic		= m_renderingStats.m_renderedDynamicDecalsCount;
			CurrentParticlesCount		= m_renderingStats.m_numParticles;
			CurrentHiresChunks			= m_renderingStats.m_numHiResShadowsChunks;
			CurrentSpeedTree			= m_speedTreeStats.m_renderStats.m_visibleTreeInstanceCount;
			CurrentGrass				= m_speedTreeStats.m_renderStats.m_visibleGrassInstanceCount;
			CurrentChunksStatic			= m_renderingStats.m_numSceneChunksStatic;
			CurrentTrianglesStatic		= m_renderingStats.m_numSceneTrianglesStatic;
			CurrentChunksSkinned		= m_renderingStats.m_numSceneChunksSkinned;
			CurrentTrianglesSkinned		= m_renderingStats.m_numSceneTrianglesSkinned;
						
			if( m_timeSinceLastRetrival > 1.0f )
			{
				m_timeSinceLastRetrival = 0.0f;
				m_meshStats.ResetStats();
				m_textureStats.ResetStats();

				GRender->GetGeneralMeshStats( m_meshStats );
				GRender->GetGeneralTextureStats( m_textureStats );
			}		
						
			CurrentDynMeshes	= m_meshStats.m_characterMemory;
			CurrentStatMeshes	= m_meshStats.m_environmentMemory;
			CurrentCharTextures	= m_textureStats.m_characterMemory;
			CurrentStatTextures	= m_textureStats.m_environmentMemory;

			m_apexTickedProgressBar->SetProgressPosition( Float(CurrentApexTicked) );
			m_apexDestroTickedProgressBar->SetProgressPosition( Float(CurrentApexDestroTicked) );
			m_lightsShadowProgressBar->SetProgressPosition( Float(CurrentLightsShadow) );
			m_lightsNonShadowsProgressBar->SetProgressPosition( Float(CurrentLightsNonShadows) );
			m_decalsSSProgressBar->SetProgressPosition( Float(CurrentDecalsSS) );
			m_decalsDynamicProgressBar->SetProgressPosition( Float(CurrentDecalsDynamic) );
			m_particlesCountProgressBar->SetProgressPosition( Float(CurrentParticlesCount) );
			m_hiresChunksProgressBar->SetProgressPosition( Float(CurrentHiresChunks) );
			m_speedTreeProgressBar->SetProgressPosition( Float(CurrentSpeedTree) );
			m_grassProgressBar->SetProgressPosition( Float(CurrentGrass) );
			m_chunksStaticProgressBar->SetProgressPosition( Float(CurrentChunksStatic) );
			m_trianglesStaticProgressBar->SetProgressPosition( Float(CurrentTrianglesStatic) );
			m_chunksSkinnedProgressBar->SetProgressPosition( Float(CurrentChunksSkinned) );
			m_trianglesSkinnedProgressBar->SetProgressPosition( Float(CurrentTrianglesSkinned) );
			m_DynMeshesProgressBar->SetProgressPosition( Float(CurrentDynMeshes) );
			m_StatMeshesProgressBar->SetProgressPosition( Float(CurrentStatMeshes) );
			m_CharTexturesProgressBar->SetProgressPosition( Float(CurrentCharTextures) );
			m_StatTexturesProgressBar->SetProgressPosition( Float(CurrentStatTextures) );

			m_apexTickedTimeProgressBar->SetProgressPosition( CurrentApexTickedTime );
			m_apexDestroTickedTimeProgressBar->SetProgressPosition( CurrentApexDestroTickedTime );
			m_lightsShadowTimeProgressBar->SetProgressPosition( Max( CurrentGPULightsShadowTime, CurrentLightsShadowTime ) );
			m_lightsNonShadowsTimeProgressBar->SetProgressPosition( Max(CurrentGPULightsNonShadowsTime, CurrentLightsNonShadowsTime ) );
			m_decalsSSTimeProgressBar->SetProgressPosition( Max( CurrentGPUDecalsSSTime, CurrentDecalsSSTime ) );
			m_decalsDynamicTimeProgressBar->SetProgressPosition( Max( CurrentGPUDecalsDynamicTime, CurrentDecalsDynamicTime ) );
			m_particlesCountTimeProgressBar->SetProgressPosition( Max( CurrentGPUParticlesCountTime, CurrentParticlesCountTime ) );
			m_hiresChunksTimeProgressBar->SetProgressPosition( Max( CurrentGPUHiresChunksTime, CurrentHiresChunksTime ) );
			m_speedTreeTimeProgressBar->SetProgressPosition( Max( CurrentGPUSpeedTreeTime, CurrentSpeedTreeTime ) );
			m_grassTimeProgressBar->SetProgressPosition( Max( CurrentGPUGrassTime, CurrentGrassTime ) );
			m_chunksStaticTimeProgressBar->SetProgressPosition( Max( CurrentGPUChunksStaticTime, CurrentChunksStaticTime) );
			m_chunksSkinnedTimeProgressBar->SetProgressPosition( Max( CurrentGPUChunksSkinnedTime, CurrentChunksSkinnedTime ) );	

			DisplayStat( m_apexTickedProgressBar , CurrentApexTicked , m_MaxAllowedApexTicked , false , m_apexTickedTimeProgressBar , CurrentApexTickedTime , m_MaxAllowedApexTickedTime);
			DisplayStat( m_apexDestroTickedProgressBar , CurrentApexDestroTicked , m_MaxAllowedApexDestroTicked , false , m_apexDestroTickedTimeProgressBar , CurrentApexDestroTickedTime , m_MaxAllowedApexDestroTickedTime);
			DisplayStat( m_lightsShadowProgressBar , CurrentLightsShadow , m_MaxAllowedLightsShadow , false , m_lightsShadowTimeProgressBar , Max( CurrentGPULightsShadowTime, CurrentLightsShadowTime ) , m_MaxAllowedLightsShadowTime );
			DisplayStat( m_lightsNonShadowsProgressBar , CurrentLightsNonShadows , m_MaxAllowedLightsNonShadows , false ,  m_lightsNonShadowsTimeProgressBar , Max(CurrentGPULightsNonShadowsTime, CurrentLightsNonShadowsTime ) , m_MaxAllowedLightsNonShadowsTime);
			DisplayStat( m_decalsSSProgressBar , CurrentDecalsSS , m_MaxAllowedDecalsSS , false , m_decalsSSTimeProgressBar , Max( CurrentGPUDecalsSSTime, CurrentDecalsSSTime ) , m_MaxAllowedDecalsSSTime);
			DisplayStat( m_decalsDynamicProgressBar , CurrentDecalsDynamic , m_MaxAllowedDecalsDynamic, false , m_decalsDynamicTimeProgressBar , Max( CurrentGPUDecalsDynamicTime, CurrentDecalsDynamicTime ) , m_MaxAllowedDecalsDynamicTime);
			DisplayStat( m_particlesCountProgressBar , CurrentParticlesCount , m_MaxAllowedParticlesCount, false , m_particlesCountTimeProgressBar , Max( CurrentGPUParticlesCountTime, CurrentParticlesCountTime ) , m_MaxAllowedParticlesCountTime );
			DisplayStat( m_hiresChunksProgressBar , CurrentHiresChunks , m_MaxAllowedHiresChunks, false , m_hiresChunksTimeProgressBar , Max( CurrentGPUHiresChunksTime, CurrentHiresChunksTime ) , m_MaxAllowedHiresChunksTime);
			DisplayStat( m_speedTreeProgressBar , CurrentSpeedTree , m_MaxAllowedSpeedTree , false , m_speedTreeTimeProgressBar , Max( CurrentGPUSpeedTreeTime, CurrentSpeedTreeTime ) , m_MaxAllowedSpeedTreeTime );
			DisplayStat( m_grassProgressBar , CurrentGrass , m_MaxAllowedGrass , false , m_grassTimeProgressBar , Max( CurrentGPUGrassTime, CurrentGrassTime ) , m_MaxAllowedGrassTime );
			DisplayStat( m_chunksStaticProgressBar , CurrentChunksStatic , m_MaxAllowedChunksStatic, false , m_chunksStaticTimeProgressBar , Max( CurrentGPUChunksStaticTime, CurrentChunksStaticTime) , m_MaxAllowedChunksStaticTime);
			DisplayStat( m_trianglesStaticProgressBar , CurrentTrianglesStatic , m_MaxAllowedTrianglesStatic, false );
			DisplayStat( m_chunksSkinnedProgressBar , CurrentChunksSkinned , m_MaxAllowedChunksSkinned, false , m_chunksSkinnedTimeProgressBar , Max( CurrentGPUChunksSkinnedTime, CurrentChunksSkinnedTime ) , m_MaxAllowedChunksSkinnedTime);
			DisplayStat( m_trianglesSkinnedProgressBar , CurrentTrianglesSkinned , m_MaxAllowedTrianglesSkinned , false );
			DisplayStat( m_DynMeshesProgressBar , CurrentDynMeshes , m_MaxAllowedDynMeshes, true );
			DisplayStat( m_StatMeshesProgressBar , CurrentStatMeshes , m_MaxAllowedStatMeshes, true );
			DisplayStat( m_CharTexturesProgressBar , CurrentCharTextures , m_MaxAllowedCharTextures, true );
			DisplayStat( m_StatTexturesProgressBar , CurrentStatTextures , m_MaxAllowedStatTextures, true );			
		}
	}

	void CDebugWindowSceneStats::NotifyButtonClicked( RedGui::CRedGuiEventPackage& eventPackage )
	{
	}

	void CDebugWindowSceneStats::OnWindowClosed( CRedGuiControl* control )
	{
		GRender->EnableGpuProfilerFrameStatsMode( false );
	}

	void CDebugWindowSceneStats::OnWindowOpened( CRedGuiControl* control )
	{
		GRender->EnableGpuProfilerFrameStatsMode( true );
		SetPosition( 50, 50 );

		//=== Set Budgets
		m_MaxAllowedApexTicked		=	Config::cvMaxAllowedApexTicked.Get();
		m_MaxAllowedApexDestroTicked	=	Config::cvMaxAllowedApexDestroTicked.Get();	
		m_MaxAllowedLightsShadow		=	Config::cvMaxAllowedLightsShadow.Get();		
		m_MaxAllowedLightsNonShadows	=	Config::cvMaxAllowedLightsNonShadows.Get();	
		m_MaxAllowedDecalsSS			=	Config::cvMaxAllowedDecalsSS.Get();			
		m_MaxAllowedDecalsDynamic		=	Config::cvMaxAllowedDecalsDynamic.Get();		
		m_MaxAllowedParticlesCount	=	Config::cvMaxAllowedParticlesCount.Get();
		m_MaxAllowedHiresChunks		=	Config::cvMaxAllowedHiresChunks.Get();
		m_MaxAllowedSpeedTree			=	Config::cvMaxAllowedSpeedTree.Get();
		m_MaxAllowedGrass				=	Config::cvMaxAllowedGrass.Get();
		m_MaxAllowedChunksStatic		=	Config::cvMaxAllowedChunksStatic.Get();		
		m_MaxAllowedTrianglesStatic	=	Config::cvMaxAllowedTrianglesStatic.Get();	
		m_MaxAllowedChunksSkinned		=	Config::cvMaxAllowedChunksSkinned.Get();		
		m_MaxAllowedTrianglesSkinned	=	Config::cvMaxAllowedTrianglesSkinned.Get();	
		m_MaxAllowedDynMeshes			=	Config::cvMaxAllowedDynMeshes.Get();			
		m_MaxAllowedStatMeshes		=	Config::cvMaxAllowedStatMeshes.Get();	
		m_MaxAllowedCharTextures		=	Config::cvMaxAllowedCharTextures.Get();		
		m_MaxAllowedStatTextures		=	Config::cvMaxAllowedStatTextures.Get();		

		m_MaxAllowedApexTickedTime		=	Config::cvMaxAllowedApexTickedTime.Get();
		m_MaxAllowedApexDestroTickedTime	=	Config::cvMaxAllowedApexDestroTickedTime.Get();
		m_MaxAllowedLightsShadowTime		=	Config::cvMaxAllowedLightsShadowTime.Get();
		m_MaxAllowedLightsNonShadowsTime	=	Config::cvMaxAllowedLightsNonShadowsTime.Get();
		m_MaxAllowedDecalsSSTime			=	Config::cvMaxAllowedDecalsSSTime.Get();
		m_MaxAllowedDecalsDynamicTime		=	Config::cvMaxAllowedDecalsDynamicTime.Get();
		m_MaxAllowedParticlesCountTime	=	Config::cvMaxAllowedParticlesCountTime.Get();
		m_MaxAllowedHiresChunksTime		=	Config::cvMaxAllowedHiresChunksTime.Get();
		m_MaxAllowedSpeedTreeTime			=	Config::cvMaxAllowedSpeedTreeTime.Get();
		m_MaxAllowedGrassTime				=	Config::cvMaxAllowedGrassTime.Get();
		m_MaxAllowedChunksStaticTime		=	Config::cvMaxAllowedChunksStaticTime.Get();
		m_MaxAllowedChunksSkinnedTime		=	Config::cvMaxAllowedChunksSkinnedTime.Get();

		m_apexTickedProgressBar->SetProgressRange( Float(m_MaxAllowedApexTicked) );
		m_apexDestroTickedProgressBar->SetProgressRange( Float(m_MaxAllowedApexDestroTicked) );
		m_lightsShadowProgressBar->SetProgressRange( Float(m_MaxAllowedLightsShadow) );
		m_lightsNonShadowsProgressBar->SetProgressRange( Float(m_MaxAllowedLightsNonShadows) );
		m_decalsSSProgressBar->SetProgressRange( Float(m_MaxAllowedDecalsSS) );
		m_decalsDynamicProgressBar->SetProgressRange( Float(m_MaxAllowedDecalsDynamic) );
		m_particlesCountProgressBar->SetProgressRange( Float(m_MaxAllowedParticlesCount) );
		m_hiresChunksProgressBar->SetProgressRange( Float(m_MaxAllowedHiresChunks) );
		m_speedTreeProgressBar->SetProgressRange( Float(m_MaxAllowedSpeedTree) );
		m_grassProgressBar->SetProgressRange( Float(m_MaxAllowedGrass) );
		m_chunksStaticProgressBar->SetProgressRange( Float(m_MaxAllowedChunksStatic) );
		m_trianglesStaticProgressBar->SetProgressRange( Float(m_MaxAllowedTrianglesStatic) );
		m_chunksSkinnedProgressBar->SetProgressRange( Float(m_MaxAllowedChunksSkinned) );
		m_trianglesSkinnedProgressBar->SetProgressRange( Float(m_MaxAllowedTrianglesSkinned) );
		m_DynMeshesProgressBar->SetProgressRange( Float(m_MaxAllowedDynMeshes) );
		m_StatMeshesProgressBar->SetProgressRange( Float(m_MaxAllowedStatMeshes) );
		m_CharTexturesProgressBar->SetProgressRange( Float(m_MaxAllowedCharTextures) );
		m_StatTexturesProgressBar->SetProgressRange( Float(m_MaxAllowedStatTextures) );

		m_apexTickedTimeProgressBar->SetProgressRange( Float(m_MaxAllowedApexTickedTime) );
		m_apexDestroTickedTimeProgressBar->SetProgressRange( Float(m_MaxAllowedApexDestroTickedTime) );
		m_lightsShadowTimeProgressBar->SetProgressRange( Float(m_MaxAllowedLightsShadowTime) );
		m_lightsNonShadowsTimeProgressBar->SetProgressRange( Float(m_MaxAllowedLightsNonShadowsTime) );
		m_decalsSSTimeProgressBar->SetProgressRange( Float(m_MaxAllowedDecalsSSTime) );
		m_decalsDynamicTimeProgressBar->SetProgressRange( Float(m_MaxAllowedDecalsDynamicTime) );
		m_particlesCountTimeProgressBar->SetProgressRange( Float(m_MaxAllowedParticlesCountTime) );
		m_hiresChunksTimeProgressBar->SetProgressRange( Float(m_MaxAllowedHiresChunksTime) );
		m_speedTreeTimeProgressBar->SetProgressRange( Float(m_MaxAllowedSpeedTreeTime) );
		m_grassTimeProgressBar->SetProgressRange( Float(m_MaxAllowedGrassTime) );
		m_chunksStaticTimeProgressBar->SetProgressRange( Float(m_MaxAllowedChunksStaticTime) );
		m_chunksSkinnedTimeProgressBar->SetProgressRange( Float(m_MaxAllowedChunksSkinnedTime) );

		TDynArray<String>		apexTickedCounterNames;
		apexTickedCounterNames.PushBack(TXT("CApexClothWrapper PostSimulation"));
		apexTickedCounterNames.PushBack(TXT("Physics PreSim CApexClothWrapper"));
		apexTickedCounterNames.PushBack(TXT("Physics Distance CApexClothWrapper"));
		apexTickedCounterNames.PushBack(TXT("CApexClothWrapper PostSimulation"));
		apexTickedCounterNames.PushBack(TXT("CApexClothWrapperRemove"));
		TDynArray<String>		apexDestroTickedCounterNames;
		apexDestroTickedCounterNames.PushBack(TXT("CApexDestructionWrapper PostSimulation"));
		apexDestroTickedCounterNames.PushBack(TXT("Physics PreSim CApexDestructionWrapper"));
		apexDestroTickedCounterNames.PushBack(TXT("Physics Distance CApexDestructionWrapper"));
		apexDestroTickedCounterNames.PushBack(TXT("CApexDestructionWrapper PostSimulation"));
		apexDestroTickedCounterNames.PushBack(TXT("CApexDestructionWrapperRemove"));
		TDynArray<String>		lightsShadowCounterNames;		
		lightsShadowCounterNames.PushBack(TXT("RenderDynamicShadowmaps"));
		TDynArray<String>		lightsNonShadowsCounterNames;
		lightsShadowCounterNames.PushBack(TXT("RenderStaticShadowmaps"));
		TDynArray<String>		decalsSSCounterNames;
		decalsSSCounterNames.PushBack(TXT("RenderDecals"));
		TDynArray<String>		decalsDynamicCounterNames;
		decalsDynamicCounterNames.PushBack(TXT("RenderDynamicDecals"));
		decalsDynamicCounterNames.PushBack(TXT("CollectDynamicDecalList"));
		decalsDynamicCounterNames.PushBack(TXT("RenderScene_SpawnQueuedDynamicDecals"));
		decalsDynamicCounterNames.PushBack(TXT("TickDynamicDecals"));
		TDynArray<String>		particlesCountCounterNames;
		particlesCountCounterNames.PushBack(TXT("RenderParticles"));
		TDynArray<String>		hiresChunksCounterNames;
		hiresChunksCounterNames.PushBack(TXT("HiResEntityShadows"));
		TDynArray<String>		speedTreeCounterNames;
		speedTreeCounterNames.PushBack(TXT("RenderSpeedTree"));
		speedTreeCounterNames.PushBack(TXT("CMDRemoveSpeedTreeInstancesRadius"));
		speedTreeCounterNames.PushBack(TXT("CMDCreateSpeedTreeDynamicInstances"));
		speedTreeCounterNames.PushBack(TXT("CMDUpdateSpeedTreeInstancesRadius"));
		speedTreeCounterNames.PushBack(TXT("CMDAddSpeedTreeProxyToScene"));
		speedTreeCounterNames.PushBack(TXT("UpdateSpeedTreeShadows"));
		speedTreeCounterNames.PushBack(TXT("UpdateSpeedTreeShadows"));
		speedTreeCounterNames.PushBack(TXT("SpeedTreeShadows"));
		TDynArray<String>		grassCounterNames;
		grassCounterNames.PushBack(TXT("RenderFoliage"));
		TDynArray<String>		staticGeometryCounterNames;	
		staticGeometryCounterNames.PushBack(TXT("RenderDeferredFillGBuffer_Statics"));
		staticGeometryCounterNames.PushBack(TXT("FinishSceneStaticsCulling"));
		staticGeometryCounterNames.PushBack(TXT("CollectStaticDrawables"));
		TDynArray<String>		dynamicGeometryCounterNames;		
		dynamicGeometryCounterNames.PushBack(TXT("RenderDeferredFillGBuffer_NonStaticsEtc"));	
		
		PrepareStatBoxes(apexTickedCounterNames , m_apexTickedStatboxes , 0 );
		PrepareStatBoxes(apexDestroTickedCounterNames , m_apexDestroTickedStatboxes , 0 );
		PrepareStatBoxes(lightsShadowCounterNames , m_lightsShadowStatboxes , 1 );
		PrepareStatBoxes(lightsNonShadowsCounterNames , m_lightsNonShadowsStatboxes , 1 );
		PrepareStatBoxes(decalsSSCounterNames , m_decalsSSStatboxes , 1 );
		PrepareStatBoxes(decalsDynamicCounterNames , m_decalsDynamicStatboxes , 1 );
		PrepareStatBoxes(particlesCountCounterNames , m_particlesCountStatboxes , 1 );
		PrepareStatBoxes(hiresChunksCounterNames , m_hiresChunksStatboxes , 1 );
		PrepareStatBoxes(speedTreeCounterNames , m_speedTreeStatboxes , 1 );
		PrepareStatBoxes(grassCounterNames , m_grassStatboxes , 1 );
		PrepareStatBoxes(staticGeometryCounterNames , m_staticGeometryStatboxes , 1 );
		PrepareStatBoxes(dynamicGeometryCounterNames , m_dynamicGeometryStatboxes , 1 );
	}

	void CDebugWindowSceneStats::AddField( RedGui::CRedGuiGroupBox* parent , const String& labelname , RedGui::CRedGuiProgressBar** progressbar , RedGui::CRedGuiProgressBar** progressbartime , Bool useTimeBar)
	{
		RedGui::CRedGuiPanel* panel1 = new RedGui::CRedGuiPanel( 0, 0, 440, 20 );
		panel1->SetDock( RedGui::DOCK_Top );
		parent->AddChild( panel1 );
		{
			RedGui::CRedGuiLabel*	label;
			label = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
			label->SetAutoSize(false);
			label->SetText( labelname );
			label->SetDock( RedGui::DOCK_Left);
			panel1->AddChild( label );
			(*progressbar) = new RedGui::CRedGuiProgressBar( 0, 0, 170, 20 );
			(*progressbar)->SetMargin( Box2( 2 ,2, 2, 2) );
			(*progressbar)->SetShowProgressInformation( true );
			(*progressbar)->SetDock( RedGui::DOCK_Left );
			panel1->AddChild( *progressbar );
			if (useTimeBar)
			{
				(*progressbartime) = new RedGui::CRedGuiProgressBar( 0, 0, 170, 20 );
				(*progressbartime)->SetMargin( Box2( 2 ,2, 2, 2) );
				(*progressbartime)->SetShowProgressInformation( true );
				(*progressbartime)->SetDock( RedGui::DOCK_Fill );
				panel1->AddChild( *progressbartime );
			}
		}
	}

	void CDebugWindowSceneStats::AddDescriptionField( RedGui::CRedGuiGroupBox* parent , const String& labelname1 , const String& labelname2 , const String& labelname3 )
	{
		RedGui::CRedGuiPanel* panel1 = new RedGui::CRedGuiPanel( 0, 0, 440, 20 );
		panel1->SetDock( RedGui::DOCK_Top );
		parent->AddChild( panel1 );
		{
			RedGui::CRedGuiLabel*	label1;
			label1 = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
			label1->SetAutoSize(false);
			label1->SetText( labelname1 );
			label1->SetDock( RedGui::DOCK_Left);
			panel1->AddChild( label1 );
			RedGui::CRedGuiLabel*	label2;
			label2 = new RedGui::CRedGuiLabel( 0, 0, 170, 20 );
			label2->SetAutoSize(false);
			label2->SetText( labelname2 );
			label2->SetDock( RedGui::DOCK_Left);
			panel1->AddChild( label2 );
			RedGui::CRedGuiLabel*	label3;
			label3 = new RedGui::CRedGuiLabel( 0, 0, 170, 20 );
			label3->SetAutoSize(false);
			label3->SetText( labelname3 );
			label3->SetDock( RedGui::DOCK_Fill);
			panel1->AddChild( label3 );
		}
	}

	void CDebugWindowSceneStats::DisplayStat( RedGui::CRedGuiProgressBar* progressBar , Uint64 inValue , Uint32 maxAllowed , Bool isMemorySize , RedGui::CRedGuiProgressBar* progressBarTime , Float inTime , Float maxAllowedTime )
	{
		// Count Stats
		Float CurrentCount;
		String CurrentsizeUnit = TXT("");
		if ( inValue >= Uint64(isMemorySize?1024*1024:1000*1000) )
		{
			CurrentsizeUnit = (isMemorySize?TXT("Mb"):TXT("M"));
			CurrentCount = inValue / (isMemorySize?( 1024.0f * 1024.0f ):(1000.0f * 1000.0f));
		}
		else if ( inValue >= Uint64(isMemorySize?1024:1000) )
		{
			CurrentsizeUnit = (isMemorySize?TXT("Kb"):TXT("K"));
			CurrentCount = inValue / (isMemorySize?1024.0f:1000.0f); 
		}
		else
		{
			CurrentsizeUnit = (isMemorySize?TXT("B"):TXT(""));
			CurrentCount = Float(inValue);
		}
		Float MaxAllowedCount;
		String MaxAllowedsizeUnit = TXT("");
		if ( maxAllowed >= Uint32(isMemorySize?1024*1024:1000*1000) )
		{
			MaxAllowedsizeUnit = (isMemorySize?TXT("Mb"):TXT("M"));
			MaxAllowedCount = maxAllowed / (isMemorySize?( 1024.0f * 1024.0f ):(1000.0f * 1000.0f));
		}
		else if ( maxAllowed >= Uint32(isMemorySize?1024:1000) )
		{
			MaxAllowedsizeUnit = (isMemorySize?TXT("Kb"):TXT("K"));
			MaxAllowedCount = maxAllowed / (isMemorySize?1024.0f:1000.0f); 
		}
		else
		{
			MaxAllowedsizeUnit = (isMemorySize?TXT("B"):TXT(""));
			MaxAllowedCount = Float(maxAllowed);
		}

		Float proc = MCeil( (Float(inValue)/Float(maxAllowed))*100.0f );
		String text = String::Printf( TXT("%.1f %s / %.1f %s = %.0f %%"), CurrentCount, CurrentsizeUnit.AsChar(), MaxAllowedCount, MaxAllowedsizeUnit.AsChar(), proc );
		Color color;
		if ( proc <= 100 )
		{
			color = Lerp( (Float)( proc/100 ), Color( 0, 255, 0 ).ToVector(), Color( 255, 200, 0 ).ToVector() );
		}
		else
		{
			color = Color( 255, 0, 0 );
		}
		progressBar->SetProgressBarColor( color );
		progressBar->SetProgressInformation( text );

		// Time Stats
		if (progressBarTime)
		{
			proc = MCeil( (inTime/maxAllowedTime)*100.0f );
			text = String::Printf( TXT("%.3f / %.1f %s = %.0f %%"), inTime , maxAllowedTime, TXT(" ms"), proc );
			Color color;
			if ( proc <= 100 )
			{
				color = Lerp( (Float)( proc/100 ), Color( 0, 255, 0 ).ToVector(), Color( 255, 200, 0 ).ToVector() );
			}
			else
			{
				color = Color( 255, 0, 0 );
			}
			progressBarTime->SetProgressBarColor( color );
			progressBarTime->SetProgressInformation( text );
		}


	}

	void CDebugWindowSceneStats::PrepareStatBoxes( const TDynArray<String>& counters , TDynArray<CProfilerStatBox>& statboxes , Int32 root)
	{
		statboxes.ClearFast();
		TDynArray<CPerfCounter*> tempCounters;
		for ( const String& counterName : counters )
		{
			CProfiler::GetCounters( UNICODE_TO_ANSI(counterName.AsChar()) , tempCounters , root );
		}

		for ( CPerfCounter* counter : tempCounters )
		{
			CProfilerStatBox newstatbox(counter);
			statboxes.PushBack(newstatbox);
		}
	}

	Float CDebugWindowSceneStats::CountTime( TDynArray<CProfilerStatBox>& statboxes )
	{
		Double timeSum = 0.0f;

		for ( CProfilerStatBox& statBox : statboxes )
		{
			statBox.Tick();
			timeSum += (statBox.GetAverageTime());
		}
		return float(timeSum);
	}

	Float CDebugWindowSceneStats::GetGpuTime( const String& name, const String& parentName )
	{
		Float time = 0.0f;

		for ( TDynArray<SceneRenderingStats::GpuTimesStat>::const_iterator it = m_gpuEntries.Begin() ; it != m_gpuEntries.End() ; ++it )
		{
			if ( name == it->m_name && ( parentName == it->m_parent || parentName == String::EMPTY ) )
			{
				time += it->m_time;
			}
		}
		return time;
	}
}
#endif	//NO_DEBUG_WINDOWS
#endif	//NO_RED_GUI
