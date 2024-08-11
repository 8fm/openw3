/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "debugWindowTerrain.h"
#include "redGuiControl.h"
#include "redGuiManager.h"
#include "redGuiPanel.h"
#include "redGuiScrollPanel.h"
#include "redGuiTab.h"
#include "redGuiLabel.h"
#include "redGuiSpin.h"
#include "redGuiClipmapVisualizer.h"
#include "game.h"
#include "clipMap.h"
#include "terrainTile.h"

namespace DebugWindows
{

	CDebugWindowTerrain::CDebugWindowTerrain()
		: RedGui::CRedGuiWindow( 100, 100, 600, 600 )
	{
		GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindowTerrain::NotifyOnTick );

		SetCaption( TXT("Terrain") );
		CreateControls();
	}

	CDebugWindowTerrain::~CDebugWindowTerrain()
	{
		GRedGui::GetInstance().EventTick.Unbind( this, &CDebugWindowTerrain::NotifyOnTick );
	}

	void CDebugWindowTerrain::CreateControls()
	{
		m_tabs = new RedGui::CRedGuiTab( 0, 0, 100, 100 );
		m_tabs->SetMargin( Box2(5, 5, 5, 5) );
		m_tabs->AddTab( TXT("Tile Load Status") );
		m_tabs->SetActiveTab( 0 );
		m_tabs->SetDock( RedGui::DOCK_Fill );
		AddChild( m_tabs );

		RedGui::CRedGuiScrollPanel* loadStatusTab = m_tabs->GetTabAt( TAB_TileLoadStatus );
		{
			RedGui::CRedGuiPanel* levelPanel = new RedGui::CRedGuiPanel( 0, 0, 600, 20 );
			loadStatusTab->AddChild( levelPanel );
			levelPanel->SetMargin( Box2( 10, 10, 5, 5 ) );
			levelPanel->SetBorderVisible( false );
			levelPanel->SetBackgroundColor( Color::CLEAR );
			levelPanel->SetDock( RedGui::DOCK_Top );

			RedGui::CRedGuiLabel* levelLabel = new RedGui::CRedGuiLabel( 0,0, 50, 20 );
			levelPanel->AddChild( levelLabel );
			levelLabel->SetText( TXT("Clip stack level: ") );
			levelLabel->SetDock(RedGui::DOCK_Left);

			m_tileLoadClipStackLevel = new RedGui::CRedGuiSpin( 0, 0, 50, 20 );
			levelPanel->AddChild( m_tileLoadClipStackLevel );
			m_tileLoadClipStackLevel->SetDock( RedGui::DOCK_Left );
			m_tileLoadClipStackLevel->SetMinValue( 0 );
			m_tileLoadClipStackLevel->SetMaxValue( 0 );
			m_tileLoadClipStackLevel->SetValue( 0 );

			m_tileLoadLoading = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
			levelPanel->AddChild( m_tileLoadLoading );
			m_tileLoadLoading->SetDock( RedGui::DOCK_Right );


			m_tileLoadClipmapView = new RedGui::CRedGuiClipmapVisualizer( 0, 0, 100, 100 );
			loadStatusTab->AddChild( m_tileLoadClipmapView );
			m_tileLoadClipmapView->SetDock( RedGui::DOCK_Fill );
		}
	}

	void CDebugWindowTerrain::NotifyOnTick( RedGui::CRedGuiEventPackage& /*eventPackage*/, Float /*deltaTime*/ )
	{
		if ( !GetVisible() )
		{
			return;
		}

		// If we don't have any terrain, reset the view.
		if ( GGame == nullptr || GGame->GetActiveWorld() == nullptr || GGame->GetActiveWorld()->GetTerrain() == nullptr )
		{
			m_tileLoadClipStackLevel->SetMinValue( 0 );
			m_tileLoadClipStackLevel->SetMaxValue( 0 );
			m_tileLoadClipStackLevel->SetValue( 0 );
			m_tileLoadClipmapView->Reset();
			return;
		}

		if ( m_tabs->GetActiveTabIndex() == TAB_TileLoadStatus )
		{
			UpdateTileLoadStatus();
		}
	}


	void CDebugWindowTerrain::UpdateTileLoadStatus()
	{
		// We checked these for nulls in NotifyOnTick
		CWorld* world = GGame->GetActiveWorld();
		CClipMap* terrain = world->GetTerrain();


		//////////////////////////////////////////////////////////////////////////
		// Update number of clip stack levels.

		Int32 maxLevel = terrain->GetNumClipmapStackLevels() - 1;
		if ( m_tileLoadClipStackLevel->GetValue() > maxLevel )
		{
			m_tileLoadClipStackLevel->SetValue( maxLevel );
		}
		m_tileLoadClipStackLevel->SetMaxValue( maxLevel );

		//////////////////////////////////////////////////////////////////////////
		// Update the clipmap visualizer

		// First pass along the clipmap parameters. This has to be done first, since the visualizer may need to set up for
		// a new configuration.
		SClipmapParameters params;
		terrain->GetClipmapParameters( &params );
		m_tileLoadClipmapView->SetClipmapParameters( params );


		// Now set the state of each tile, for the selected level in the clip stack.
		Int32 whichLevel = m_tileLoadClipStackLevel->GetValue();
		Uint32 tilesPerEdge = terrain->GetNumTilesPerEdge();
		for ( Uint32 y = 0; y < tilesPerEdge; ++y )
		{
			for ( Uint32 x = 0; x < tilesPerEdge; ++x )
			{
				const CTerrainTile* tile = terrain->GetTile( x, y );

				Color border;
				Color fill;

				if ( tile->IsLoaded( whichLevel ) )
				{
					if ( tile->IsMipCounting( whichLevel ) )
					{
						Uint8 alpha = (Uint8)Clamp( tile->GetMipCountdownPercent( whichLevel ) * 255.0f, 0.0f, 255.0f );

						border = Color( 64, 192, 64, 255 );
						fill   = Color( 64, 192, 64, alpha );
					}
					else
					{
						border = Color( 192, 64, 64, 255);
						fill   = Color( 192, 64, 64, 255);
					}
				}
				else if ( tile->IsPartiallyLoaded( whichLevel ) )
				{
					if ( tile->IsMipCounting( whichLevel ) )
					{
						Uint8 alpha = (Uint8)Clamp( tile->GetMipCountdownPercent( whichLevel ) * 255.0f, 0.0f, 255.0f );

						border = Color( 64, 64, 192, 255 );
						fill   = Color( 64, 64, 192, alpha );
					}
					else
					{
						border = Color( 192, 64, 192, 255);
						fill   = Color( 192, 64, 192, 255);
					}
				}
				else if ( tile->IsLoadingAsync( whichLevel ) )
				{
					Uint8 alpha = (Uint8)Clamp( tile->GetMipCountdownPercent( whichLevel ) * 255.0f, 0.0f, 255.0f );
					border = Color( 192, 192, 64, 255 );
					fill   = Color( 192, 192, 64, alpha );
				}
				else
				{
					border = Color( 0, 0, 0, 0 );
					fill   = Color( 0, 0, 0, 0 );
				}

				if ( tile->DidRequestAsyncSinceLastTime( whichLevel ) )
				{
					border = Color( 0, 255, 255, 255 );
				}

				m_tileLoadClipmapView->SetTileColor( x, y, border, fill );
			}
		}

		// Camera position
		const Vector& camPos = world->GetCameraPosition();
		m_tileLoadClipmapView->SetNormalizedViewPosition( terrain->GetTexelSpaceNormalizedPosition( camPos ) );

		// And clip windows
		for ( Uint32 level = 0; level < terrain->GetNumClipmapStackLevels(); ++level )
		{
			Rect windowRect = terrain->GetClipWindowRect( camPos, level );
			m_tileLoadClipmapView->SetClipWindow( level, windowRect );
		}


		if ( terrain->IsLoadingTiles() )
		{
			m_tileLoadLoading->SetText( TXT("Loading some stuff") );
		}
		else
		{
			m_tileLoadLoading->SetText( TXT("") );
		}
	}

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
