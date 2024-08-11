/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_DEBUG_PAGES

#ifndef NO_DEBUG_WINDOWS
#include "debugWindowsManager.h"
#endif

#include "debugPage.h"
#include "debugPageManagerBase.h"
#include "renderer.h"
#include "inputBufferedInputEvent.h"
#include "game.h"
#include "renderFrame.h"

/// Interactive list of NPC
class CDebugPageVegetation : public IDebugPage
{
public:
	CDebugPageVegetation()
		: IDebugPage( TXT("Vegetation") )
	{
	}

	virtual ~CDebugPageVegetation()
	{
	}

	//! Generalized input event
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
		if( action == IACT_Press && key == IK_Enter )
		{
			GDebugWin::GetInstance().SetVisible( true );
			GDebugWin::GetInstance().ShowDebugWindow( DebugWindows::DW_Vegetation );
		}
		return true;
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

		// Not handled
		return false;
	};

	void DisplayHeapStats( CRenderFrame *frame, Int32 x, Int32 y, const Char* heapName, const SSpeedTreeResourceMetrics::HeapStats& stats )
	{
		const Float c_oneMeg = 1024.0f * 1024.0f;

		String heapStatsTxt = String::Printf( TXT( "%2.2f Mb (%2.2f Mb Peak)" ), stats.m_currentBytes / c_oneMeg, stats.m_peakBytes / c_oneMeg );
		frame->AddDebugScreenText( x, y, heapName, Color::WHITE );
		frame->AddDebugScreenText( x + 20, y + 15, heapStatsTxt, Color::WHITE );
	}

	//! Generate debug viewport fragments
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
#ifndef NO_DEBUG_WINDOWS
		String message = TXT("This debug page is converted to debug window. If you want use it, click key: Enter.");

		frame->AddDebugRect(49, 80, 502, 20, Color(100,100,100,255));
		frame->AddDebugScreenFormatedText( 60, 95, Color(255, 255, 255, 255), TXT("Debug Message Box"));

		frame->AddDebugRect(50, 100, 500, 50, Color(0,0,0,255));
		frame->AddDebugFrame(50, 100, 500, 50, Color(100,100,100,255));

		frame->AddDebugScreenFormatedText( 60, 120, Color(127, 255, 0, 255), message.AsChar());

		frame->AddDebugScreenFormatedText( 275, 140, Color(255, 255, 255), TXT(">> OK <<"));
		return;
#endif	// NO_DEBUG_WINDOWS

		IDebugPage::OnViewportGenerateFragments( view, frame );

		// Collect currently ticked effects
		if ( !GGame->GetActiveWorld() )
		{
			return;
		}

		const Uint32 height = frame->GetFrameOverlayInfo().m_height;

		// Display memory metrics
		frame->AddDebugFrame( 50 + 3 * 300, 50, 250, height-120, Color::WHITE );
		
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
		MemSize speedTreeHeapUsed = ( MemSize ) Memory::GetAllocatedBytesPerMemoryClass< MemoryPool_SpeedTree >( MC_SpeedTree );
		MemSize speedTreeHeapPeak = ( MemSize )  Memory::GetAllocatedBytesPerMemoryClassPeak< MemoryPool_SpeedTree >( MC_SpeedTree );
#else
		MemSize speedTreeHeapUsed = 0ull;
		MemSize speedTreeHeapPeak = 0ull;
#endif

		SSpeedTreeResourceMetrics speedTreeStats;
		GRender->PopulateSpeedTreeMetrics( speedTreeStats );
		DisplayHeapStats( frame, 70 + 3 * 300, 65, TXT( "Speedtree Heap" ), speedTreeStats.m_heapStats );
		DisplayHeapStats( frame, 70 + 3 * 300, 100, TXT( "Vertex Buffers" ), speedTreeStats.m_resourceStats[ SSpeedTreeResourceMetrics::RESOURCE_VB ] );
		DisplayHeapStats( frame, 70 + 3 * 300, 135, TXT( "Index Buffers" ), speedTreeStats.m_resourceStats[ SSpeedTreeResourceMetrics::RESOURCE_IB ] );
		DisplayHeapStats( frame, 70 + 3 * 300, 170, TXT( "Vertex Shaders" ), speedTreeStats.m_resourceStats[ SSpeedTreeResourceMetrics::RESOURCE_VERTEX_SHADER ] );
		DisplayHeapStats( frame, 70 + 3 * 300, 205, TXT( "Pixel Shaders" ), speedTreeStats.m_resourceStats[ SSpeedTreeResourceMetrics::RESOURCE_PIXEL_SHADER ] );
		DisplayHeapStats( frame, 70 + 3 * 300, 240, TXT( "Textures" ), speedTreeStats.m_resourceStats[ SSpeedTreeResourceMetrics::RESOURCE_TEXTURE ] );
		DisplayHeapStats( frame, 70 + 3 * 300, 275, TXT( "Other" ), speedTreeStats.m_resourceStats[ SSpeedTreeResourceMetrics::RESOURCE_OTHER ] );

		String heapStatsTxt = String::Printf( TXT( "%2.2f Mb (%2.2f Mb Peak)" ), speedTreeHeapUsed / (1024.0f * 1024.0f), speedTreeHeapPeak / (1024.0f * 1024.0f) );
		frame->AddDebugScreenText( 70 + 3 * 300, 320, TXT( "MC_SpeedTree" ), Color::WHITE );
		frame->AddDebugScreenText( 90 + 3 * 300, 335, heapStatsTxt, Color::WHITE );

		String renderStatsTxt = String::Printf( TXT( "Grass Layers: %" ) RED_PRIWu64, speedTreeStats.m_renderStats.m_grassLayerCount );
		frame->AddDebugScreenText( 70 + 3 * 300, 375, renderStatsTxt, Color::WHITE );

		renderStatsTxt = String::Printf( TXT( "Max. View Distance: %3.2f" ), speedTreeStats.m_renderStats.m_maximumGrassLayerCullDistance );
		frame->AddDebugScreenText( 70 + 3 * 300, 390, renderStatsTxt, Color::WHITE );
		
		renderStatsTxt = String::Printf( TXT( "Visible Grass Cells: %" ) RED_PRIWu64, speedTreeStats.m_renderStats.m_visibleGrassCellCount );
		frame->AddDebugScreenText( 70 + 3 * 300, 410, renderStatsTxt, Color::WHITE );

		renderStatsTxt = String::Printf( TXT( "%2.2fkb / %2.2fkb" ), speedTreeStats.m_renderStats.m_visibleGrassCellArraySize / 1024.0f, speedTreeStats.m_renderStats.m_visibleGrassCellArrayCapacity / 1024.0f );
		frame->AddDebugScreenText( 90 + 3 * 300, 425, renderStatsTxt, Color::WHITE );

		renderStatsTxt = String::Printf( TXT( "Visible Grass Instances: %" ) RED_PRIWu64, speedTreeStats.m_renderStats.m_visibleGrassInstanceCount );
		frame->AddDebugScreenText( 70 + 3 * 300, 445, renderStatsTxt, Color::WHITE );

		renderStatsTxt = String::Printf( TXT( "%2.2fkb / %2.2fkb" ), speedTreeStats.m_renderStats.m_visibleGrassInstanceArraySize / 1024.0f, speedTreeStats.m_renderStats.m_visibleGrassInstanceArrayCapacity / 1024.0f );
		frame->AddDebugScreenText( 90 + 3 * 300, 460, renderStatsTxt, Color::WHITE );

		renderStatsTxt = String::Printf( TXT( "Visible Tree Cells: %" ) RED_PRIWu64, speedTreeStats.m_renderStats.m_visibleTreeCellCount );
		frame->AddDebugScreenText( 70 + 3 * 300, 490, renderStatsTxt, Color::WHITE );

		renderStatsTxt = String::Printf( TXT( "%2.2fkb / %2.2fkb" ), speedTreeStats.m_renderStats.m_visibleTreeCellArraySize / 1024.0f, speedTreeStats.m_renderStats.m_visibleTreeCellArrayCapacity / 1024.0f );
		frame->AddDebugScreenText( 90 + 3 * 300, 505, renderStatsTxt, Color::WHITE );

		renderStatsTxt = String::Printf( TXT( "Visible Tree Instances: %" ) RED_PRIWu64, speedTreeStats.m_renderStats.m_visibleTreeInstanceCount );
		frame->AddDebugScreenText( 70 + 3 * 300, 525, renderStatsTxt, Color::WHITE );

		renderStatsTxt = String::Printf( TXT( "%2.2fkb / %2.2fkb" ), speedTreeStats.m_renderStats.m_visibleTreeInstanceArraySize / 1024.0f, speedTreeStats.m_renderStats.m_visibleTreeInstanceArrayCapacity / 1024.0f );
		frame->AddDebugScreenText( 90 + 3 * 300, 540, renderStatsTxt, Color::WHITE );
	}
};

void CreateDebugPageVegetation()
{
	IDebugPage* page = new CDebugPageVegetation();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif