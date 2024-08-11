/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
#include "debugWindowGpuResourceUse.h"

#include "../engine/redGuiManager.h"
#include "../engine/redGuiLabel.h"
#include "../engine/redGuiPanel.h"
#include "../engine/redGuiProgressBar.h"

#include "../engine/shaderCacheData.h"

namespace DebugWindows
{

	CDebugWindowGpuResourceUse::CDebugWindowGpuResourceUse()
		: RedGui::CRedGuiWindow( 200, 200, 350, 200 )
	{
		GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindowGpuResourceUse::NotifyOnTick );

		SetCaption( TXT("Gpu Resource Use") );
		
		RedGui::CRedGuiPanel* panel1 = new RedGui::CRedGuiPanel( 0, 0, 110, 85 );
		panel1->SetDock( RedGui::DOCK_Fill );
		AddChild( panel1 );

		AddItem( panel1, TXT("Textures:"),			m_usedTexturesProgressBar );
		AddItem( panel1, TXT("Buffers:"),			m_usedBuffersProgressBar );
		AddItem( panel1, TXT("Sampler States:"),	m_usedSamplerStatesProgressBar );
		AddItem( panel1, TXT("Queries:"),			m_usedQueriesProgressBar );
		AddItem( panel1, TXT("Shaders:"),			m_usedShadersProgressBar );
		AddItem( panel1, TXT("Vertex Layouts:"),	m_usedVertexLayoutsProgressBar );
		AddItem( panel1, TXT("Swap Chains:"),		m_usedSwapChainsProgressBar );

		// create Compression/Decompression label
		RedGui::CRedGuiPanel* itemPanel = new RedGui::CRedGuiPanel( 0, 0, 110, 15 );
		itemPanel->SetBorderVisible( false );
		itemPanel->SetDock( RedGui::DOCK_Top );
		panel1->AddChild( itemPanel );

		RedGui::CRedGuiLabel* usedSwapChainsLabel = new RedGui::CRedGuiLabel( 0, 0, 90, 14 );
		usedSwapChainsLabel->SetAutoSize(false);
		usedSwapChainsLabel->SetText( TXT( "ShaderCache" ) );
		usedSwapChainsLabel->SetDock( RedGui::DOCK_Left );
		itemPanel->AddChild( usedSwapChainsLabel );

		m_shaderCacheCompressingDecompressingTimeLabel = new RedGui::CRedGuiLabel( 0, 0, 50, 15 );
		m_shaderCacheCompressingDecompressingTimeLabel->SetMargin( Box2( 2 ,2, 2, 2) );
		m_shaderCacheCompressingDecompressingTimeLabel->SetDock( RedGui::DOCK_Fill );
		m_shaderCacheCompressingDecompressingTimeLabel->SetText( TXT("Compression: 0.0 ms / Decompression: 0.0 ms") );
		itemPanel->AddChild( m_shaderCacheCompressingDecompressingTimeLabel );
	}

	CDebugWindowGpuResourceUse::~CDebugWindowGpuResourceUse()
	{
	}


	void CDebugWindowGpuResourceUse::AddItem( RedGui::CRedGuiPanel* parentPanel, const String& label, RedGui::CRedGuiProgressBar*& progressBar )
	{
		RedGui::CRedGuiPanel* itemPanel = new RedGui::CRedGuiPanel( 0, 0, 110, 15 );
		itemPanel->SetBorderVisible( false );
		itemPanel->SetDock( RedGui::DOCK_Top );
		parentPanel->AddChild( itemPanel );

		RedGui::CRedGuiLabel* usedSwapChainsLabel = new RedGui::CRedGuiLabel( 0, 0, 90, 14 );
		usedSwapChainsLabel->SetAutoSize(false);
		usedSwapChainsLabel->SetText( label );
		usedSwapChainsLabel->SetDock( RedGui::DOCK_Left );
		itemPanel->AddChild( usedSwapChainsLabel );

		progressBar = new RedGui::CRedGuiProgressBar( 0, 0, 50, 15 );
		progressBar->SetMargin( Box2( 2 ,2, 2, 2) );
		progressBar->SetShowProgressInformation( true );
		progressBar->SetDock( RedGui::DOCK_Fill );
		itemPanel->AddChild( progressBar );
	}


	void CDebugWindowGpuResourceUse::NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime )
	{
		if( GetVisible() )
		{
			GpuApi::SResourceUseStats stats;
			GpuApi::GetResourceUseStats( stats );

			// Set the progress bar ranges. Maximum values etc
			UpdateProgressBar( m_usedTexturesProgressBar,		stats.m_usedTextures,		stats.m_maxTextures );
			UpdateProgressBar( m_usedBuffersProgressBar,		stats.m_usedBuffers,		stats.m_maxBuffers );
			UpdateProgressBar( m_usedSamplerStatesProgressBar,	stats.m_usedSamplerStates,	stats.m_maxSamplerStates );
			UpdateProgressBar( m_usedQueriesProgressBar,		stats.m_usedQueries,		stats.m_maxQueries );
			UpdateProgressBar( m_usedShadersProgressBar,		stats.m_usedShaders,		stats.m_maxShaders );
			UpdateProgressBar( m_usedVertexLayoutsProgressBar,	stats.m_usedVertexLayouts,	stats.m_maxVertexLayouts );
			UpdateProgressBar( m_usedSwapChainsProgressBar,		stats.m_usedSwapChains,		stats.m_maxSwapChains );

			// update compression / decompression time
			String text = String::Printf( TXT("Compression: %1.3f ms / Decompression: %1.3f ms"), ShaderEntry::s_compressionTime, ShaderEntry::s_decompressionTime );
			m_shaderCacheCompressingDecompressingTimeLabel->SetText( text );
		}
	}


	void CDebugWindowGpuResourceUse::UpdateProgressBar( RedGui::CRedGuiProgressBar* progress, Int32 num, Int32 maxCount )
	{
		Float pct = ( Float )num / ( Float )maxCount;

		// Make the string
		String text = String::Printf( TXT("%d / %d"), num, maxCount );
		Color color = Lerp( pct, Color( 0, 255, 0 ).ToVector(), Color( 255, 0, 0 ).ToVector() );

		progress->SetProgressRange( ( Float )maxCount );
		progress->SetProgressPosition( ( Float )num );
		progress->SetProgressBarColor( color );
		progress->SetProgressInformation( text );
	}

}
#endif	//NO_DEBUG_WINDOWS
#endif	//NO_RED_GUI
