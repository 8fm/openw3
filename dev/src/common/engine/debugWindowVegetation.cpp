/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiManager.h"
#include "redGuiPanel.h"
#include "redGuiTab.h"
#include "redGuiLabel.h"
#include "redGuiList.h"
#include "redGuiGroupBox.h"
#include "redGuiGridLayout.h"
#include "redGuiScrollPanel.h"
#include "debugWindowVegetation.h"
#include "game.h"

namespace DebugWindows
{
	CDebugWindowVegetation::CDebugWindowVegetation() 
		: RedGui::CRedGuiWindow( 100, 100, 600, 600 )
	{
		GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindowVegetation::NotifyOnTick );

		SetCaption( TXT("Vegetation") );
		CreateControls();
	}

	CDebugWindowVegetation::~CDebugWindowVegetation()
	{
		GRedGui::GetInstance().EventTick.Unbind( this, &CDebugWindowVegetation::NotifyOnTick );
	}

	void CDebugWindowVegetation::CreateControls()
	{
		// add panel with general information
		RedGui::CRedGuiGroupBox* groupBox = new RedGui::CRedGuiGroupBox( 0, 0, 100, 250 );
		groupBox->SetText( TXT("Memory metrics") );
		groupBox->SetDock( RedGui::DOCK_Top );
		AddChild( groupBox );
		{
			RedGui::CRedGuiGridLayout* horizontalLayout = new RedGui::CRedGuiGridLayout( 0, 0, 100, 100 );
			horizontalLayout->SetDock( RedGui::DOCK_Fill );
			horizontalLayout->SetDimensions( 2, 1 );
			groupBox->AddChild( horizontalLayout );
			{
				RedGui::CRedGuiGridLayout* layout2 = new RedGui::CRedGuiGridLayout( 0, 0, 100, 100 );
				layout2->SetDock( RedGui::DOCK_Fill );
				layout2->SetDimensions( 1, 8 );
				horizontalLayout->AddChild( layout2 );
				{
					const Uint32 heapInfoLabelCount = 8;
					for( Uint32 i=0; i<heapInfoLabelCount; ++i )
					{
						m_memoryHeapMetrics[i] = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
						layout2->AddChild( m_memoryHeapMetrics[i] );
					}
				}

				RedGui::CRedGuiGridLayout* layout3 = new RedGui::CRedGuiGridLayout( 0, 0, 100, 100 );
				layout3->SetDock( RedGui::DOCK_Fill );
				layout3->SetDimensions( 1, sc_numVegetationInfos );
				horizontalLayout->AddChild( layout3 );
				{
					for( Uint32 i=0; i<sc_numVegetationInfos; ++i )
					{
						m_vegetationInfos[i] = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
						layout3->AddChild( m_vegetationInfos[i] );
					}
				}
			}
		}

		// add tabs for lods
		m_lods = new RedGui::CRedGuiTab( 0, 0, 100, 100 );
		m_lods->SetMargin( Box2(5, 5, 5, 5) );
		m_lods->AddTab( TXT("LOD 0") );
		m_lods->AddTab( TXT("LOD 1") );
		m_lods->AddTab( TXT("LOD 2") );
		m_lods->SetActiveTab( 2 );
		m_lods->SetDock( RedGui::DOCK_Fill );
		AddChild( m_lods );

		// create list for lods
		RedGui::CRedGuiScrollPanel* lod0Tab = m_lods->GetTabAt( 0 );
		if( lod0Tab != nullptr )
		{
			m_lodList[0] = new RedGui::CRedGuiList( 0, 0, 100, 100 );
			m_lodList[0]->SetDock( RedGui:: DOCK_Fill );
			m_lodList[0]->AppendColumn( TXT("Name"), 400 );
			m_lodList[0]->AppendColumn( TXT("Count"), 200, RedGui::SA_Integer );
			m_lodList[0]->SetSorting( true );
			m_lodList[0]->SetSelectionMode( RedGui::SM_None );
			lod0Tab->AddChild( m_lodList[0] );
		}

		RedGui::CRedGuiScrollPanel* lod1Tab = m_lods->GetTabAt( 1 );
		if( lod1Tab != nullptr )
		{
			m_lodList[1] = new RedGui::CRedGuiList( 0, 0, 100, 100 );
			m_lodList[1]->SetDock( RedGui:: DOCK_Fill );
			m_lodList[1]->AppendColumn( TXT("Name"), 400 );
			m_lodList[1]->AppendColumn( TXT("Count"), 200, RedGui::SA_Integer );
			m_lodList[1]->SetSorting( true );
			m_lodList[1]->SetSelectionMode( RedGui::SM_None );
			lod1Tab->AddChild( m_lodList[1] );
		}

		RedGui::CRedGuiScrollPanel* lod2Tab = m_lods->GetTabAt( 2 );
		if( lod2Tab != nullptr )
		{
			m_lodList[2] = new RedGui::CRedGuiList( 0, 0, 100, 100 );
			m_lodList[2]->SetDock( RedGui:: DOCK_Fill );
			m_lodList[2]->AppendColumn( TXT("Name"), 400 );
			m_lodList[2]->AppendColumn( TXT("Count"), 200, RedGui::SA_Integer );
			m_lodList[2]->SetSorting( true );
			m_lodList[2]->SetSelectionMode( RedGui::SM_None );
			lod2Tab->AddChild( m_lodList[2] );
		}
	}

	void CDebugWindowVegetation::NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime )
	{
		if( GetVisible() == false )
		{
			return;
		}

		if( GGame != nullptr )
		{
			CWorld* world = GGame->GetActiveWorld();
			if( world != nullptr )
			{
				// TODO you cannot access LOD directly anymore. Will have to be fix.
			}
		}

		// update memory metrics
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
		MemSize speedTreeHeapUsed = ( MemSize ) Memory::GetAllocatedBytesPerMemoryClass< MemoryPool_SpeedTree >( MC_SpeedTree );
		MemSize speedTreeHeapPeak = ( MemSize )  Memory::GetAllocatedBytesPerMemoryClassPeak< MemoryPool_SpeedTree >( MC_SpeedTree );
#else
		MemSize speedTreeHeapUsed = 0ull;
		MemSize speedTreeHeapPeak = 0ull;
#endif

		SSpeedTreeResourceMetrics speedTreeStats;
		GRender->PopulateSpeedTreeMetrics( speedTreeStats );

		SSpeedTreeResourceMetrics::HeapStats speedTreeHeapUsedStat;
		speedTreeHeapUsedStat.m_currentBytes = speedTreeHeapUsed;
		speedTreeHeapUsedStat.m_peakBytes = speedTreeHeapPeak;

		UpdateHeapStatLabel( 0, TXT( "MC_SpeedTree" ), speedTreeHeapUsedStat );
		UpdateHeapStatLabel( 1, TXT( "Speedtree Heap" ), speedTreeStats.m_heapStats );
		UpdateHeapStatLabel( 2, TXT( "Vertex Buffers" ), speedTreeStats.m_resourceStats[ SSpeedTreeResourceMetrics::RESOURCE_VB ] );
		UpdateHeapStatLabel( 3, TXT( "Index Buffers" ), speedTreeStats.m_resourceStats[ SSpeedTreeResourceMetrics::RESOURCE_IB ] );
		UpdateHeapStatLabel( 4, TXT( "Vertex Shaders" ), speedTreeStats.m_resourceStats[ SSpeedTreeResourceMetrics::RESOURCE_VERTEX_SHADER ] );
		UpdateHeapStatLabel( 5, TXT( "Pixel Shaders" ), speedTreeStats.m_resourceStats[ SSpeedTreeResourceMetrics::RESOURCE_PIXEL_SHADER ] );
		UpdateHeapStatLabel( 6, TXT( "Textures" ), speedTreeStats.m_resourceStats[ SSpeedTreeResourceMetrics::RESOURCE_TEXTURE ] );
		UpdateHeapStatLabel( 7, TXT( "Other" ), speedTreeStats.m_resourceStats[ SSpeedTreeResourceMetrics::RESOURCE_OTHER ] );

		UpdateVegetationInfo( 0, String::Printf( TXT( "Grass Layers: %" ) RED_PRIWu64, speedTreeStats.m_renderStats.m_grassLayerCount ) );
		UpdateVegetationInfo( 1, String::Printf( TXT( "Max. View Distance: %3.2f" ), speedTreeStats.m_renderStats.m_maximumGrassLayerCullDistance ) );
		UpdateVegetationInfo( 2, String::Printf( TXT( "Min. Grass Cell Size: %3.2f" ), speedTreeStats.m_renderStats.m_minGrassCellSize ) );
		UpdateVegetationInfo( 3, String::Printf( TXT( "Max. Grass Cell Size: %3.2f" ), speedTreeStats.m_renderStats.m_maxGrassCellSize ) );
		UpdateVegetationInfo( 4, String::Printf( TXT( "Visible Grass Cells: %" ) RED_PRIWu64, speedTreeStats.m_renderStats.m_visibleGrassCellCount ) );
		UpdateVegetationInfo( 5, String::Printf( TXT( "Visible Grass Cells memory: %2.2fkb / %2.2fkb" ), speedTreeStats.m_renderStats.m_visibleGrassCellArraySize / 1024.0f, speedTreeStats.m_renderStats.m_visibleGrassCellArrayCapacity / 1024.0f ) );
		UpdateVegetationInfo( 6, String::Printf( TXT( "Visible Grass Instances: %" ) RED_PRIWu64, speedTreeStats.m_renderStats.m_visibleGrassInstanceCount ) );
		UpdateVegetationInfo( 7, String::Printf( TXT( "Visible Grass Instances memory: %2.2fkb / %2.2fkb" ), speedTreeStats.m_renderStats.m_visibleGrassInstanceArraySize / 1024.0f, speedTreeStats.m_renderStats.m_visibleGrassInstanceArrayCapacity / 1024.0f ) );
		UpdateVegetationInfo( 8, String::Printf( TXT( "Visible Tree Cells: %" ) RED_PRIWu64, speedTreeStats.m_renderStats.m_visibleTreeCellCount ) );
		UpdateVegetationInfo( 9, String::Printf( TXT( "Visible Tree Cells memory: %2.2fkb / %2.2fkb" ), speedTreeStats.m_renderStats.m_visibleTreeCellArraySize / 1024.0f, speedTreeStats.m_renderStats.m_visibleTreeCellArrayCapacity / 1024.0f ) );
		UpdateVegetationInfo( 10, String::Printf( TXT( "Visible Tree Instances: %" ) RED_PRIWu64, speedTreeStats.m_renderStats.m_visibleTreeInstanceCount ) );
		UpdateVegetationInfo( 11, String::Printf( TXT( "Visible Tree Instances memory: %2.2fkb / %2.2fkb" ), speedTreeStats.m_renderStats.m_visibleTreeInstanceArraySize / 1024.0f, speedTreeStats.m_renderStats.m_visibleTreeInstanceArrayCapacity / 1024.0f ) );
		UpdateVegetationInfo( 12, String::Printf( TXT( "Trees rendered: %") RED_PRIWu64 TXT(" in %" ) RED_PRIWu64 TXT(" drawcalls" ), speedTreeStats.m_renderStats.m_treesRendered,  speedTreeStats.m_renderStats.m_treeDrawcalls ) );
		UpdateVegetationInfo( 13, String::Printf( TXT( "Billboards rendered: %") RED_PRIWu64 TXT(" in %" ) RED_PRIWu64 TXT(" drawcalls" ), speedTreeStats.m_renderStats.m_billboardsRendered,  speedTreeStats.m_renderStats.m_billboardDrawcalls ) );
		UpdateVegetationInfo( 14, String::Printf( TXT( "Grass rendered: %") RED_PRIWu64 TXT(" in %" ) RED_PRIWu64 TXT(" drawcalls" ), speedTreeStats.m_renderStats.m_grassRendered,  speedTreeStats.m_renderStats.m_grassDrawcalls ) );
	}

	void CDebugWindowVegetation::UpdateHeapStatLabel( Uint32 index, const String& heapName, const SSpeedTreeResourceMetrics::HeapStats& stats )
	{
		const Float c_oneMeg = 1024.0f * 1024.0f;
		String heapStatsTxt = String::Printf( TXT("%s: %2.2f Mb (%2.2f Mb Peak)" ), heapName.AsChar(), stats.m_currentBytes / c_oneMeg, stats.m_peakBytes / c_oneMeg );
		m_memoryHeapMetrics[index]->SetText( heapStatsTxt );
	}

	void CDebugWindowVegetation::UpdateVegetationInfo( Uint32 index, const String& text )
	{
		m_vegetationInfos[index]->SetText( text );
	}

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
