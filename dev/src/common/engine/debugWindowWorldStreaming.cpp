/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiManager.h"
#include "redGuiLabel.h"
#include "redGuiPanel.h"
#include "redGuiGridLayout.h"
#include "redGuiComboBox.h"
#include "redGuiCheckBox.h"
#include "debugWindowWorldStreaming.h"
#include "fonts.h"
#include "streamingSectorData.h"
#include "../core/streamingGrid.h"

namespace DebugWindows
{
	class CDebugWindowWSGrid : public RedGui::CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );

	public:
		CDebugWindowWSGrid()
			: RedGui::CRedGuiControl( 0, 0, 512, 512 )
			, m_gridLevel(0)
			, m_drawGridHistogram(true)
			, m_drawGridElements(false)
			, m_drawGridCounts(false)
		{}

		void SetGridLevel( const Uint32 gridLevel )
		{
			m_gridLevel = gridLevel;
		}

		void ToggleHistogramDrawing( const Bool state )
		{
			m_drawGridHistogram = state;
		}

		void ToggleCountDrawing( const Bool state )
		{
			m_drawGridCounts = state;
		}

		void ToggleElementsDrawing( const Bool state )
		{
			m_drawGridElements = state;
		}

		virtual void Draw() override
		{	
			// border
			Vector2 drawBase = GetAbsoluteCoord().Min;
			Vector2 drawSize = GetAbsoluteCoord().Max;
			GetTheme()->DrawRawRectangle( drawBase, drawSize, Color::GRAY, true );
			GetTheme()->DrawRawRectangle( drawBase - Vector2(1,1), drawSize+Vector2(1,1), Color::WHITE, true );

			// Font info
			auto font = GetFont();
			Int32 fx = 0, fy = 0;
			Uint32 fw = 0, fh = 0;
			if ( font != nullptr )
			{
				font->GetTextRectangle( TXT("W"), fx, fy, fw, fh );
				fh += 2;
			}

			// Get streaming data manager
			CWorld* world = GGame ? GGame->GetActiveWorld() : nullptr;
			CStreamingSectorData* data = world ? world->GetStreamingSectorData() : nullptr;
			if ( !data )
				return;

			// get debug info
			SStreamingDebugData generalInfo;
			data->GetDebugInfo( generalInfo );

			// get data for given grid level
			SStreamingGridDebugData gridInfo;
			data->GetGridDebugInfo( m_gridLevel, gridInfo );

			// get current camera position (for quantization)
			const auto cameraPos = world->GetCameraPosition();
			const auto cameraDir = world->GetCameraForward();

			// quantize (lame) the camera position
			const Int32 cameraX = (Int32)( 65535.0f * (cameraPos.X + (generalInfo.m_worldSize/2.0f) ) / generalInfo.m_worldSize );
			const Int32 cameraY = (Int32)( 65535.0f * (cameraPos.Y + (generalInfo.m_worldSize/2.0f) ) / generalInfo.m_worldSize );

			// calculate position in grid
			const Int32 gridQuantization = (15 - m_gridLevel);
			const Int32 gridCameraX = cameraX >> gridQuantization;
			const Int32 gridCameraY = cameraY >> gridQuantization;

			// draw grid histogram
			if ( m_drawGridHistogram && gridInfo.m_gridSize )
			{
				const Uint32 size = gridInfo.m_gridSize;
				const Float pixelSizeX = drawSize.X / (Float)size;
				const Float pixelSizeY = drawSize.Y / (Float)size;

				Float py = drawBase.Y;
				for ( Uint32 y=0; y<size; ++y, py += pixelSizeY )
				{
					Float px = drawBase.X;
					for ( Uint32 x=0; x<size; ++x, px += pixelSizeX )
					{
						const Uint32 count = gridInfo.m_gridElemCount[x + y*size];
						if ( count > 0 )
						{
							const Color color = CalcColor( count, gridInfo.m_gridMaxElemCount );

							const Vector2 cmin( px,py );
							const Vector2 csize( pixelSizeX, pixelSizeY );
							GetTheme()->DrawRawFilledRectangle( cmin, csize, color, true );

							// print count
							if ( m_drawGridCounts )
							{
								GetTheme()->DrawRawText( cmin + csize*0.5f, ToString<Int32>(count), Color::WHITE );
							}
						}
					}
				}
			}

			// highlight active grid cells
			if ( gridInfo.m_gridSize )
			{
				const Uint32 size = gridInfo.m_gridSize;
				const Float pixelSizeX = drawSize.X / (Float)size;
				const Float pixelSizeY = drawSize.Y / (Float)size;

				const Int32 gridMinX = Max<Int32>( 0, gridCameraX-1 );
				const Int32 gridMinY = Max<Int32>( 0, gridCameraY-1 );
				const Int32 gridMaxX = Min<Int32>( gridCameraX+1, size-1 );
				const Int32 gridMaxY = Min<Int32>( gridCameraY+1, size-1 );

				for ( Int32 y=gridMinY; y<=gridMaxY; ++y )
				{
					for ( Int32 x=gridMinX; x<=gridMaxX; ++x )
					{
						const Float px = drawBase.X + x*pixelSizeX;
						const Float py = drawBase.Y + y*pixelSizeY;

						const Vector2 cmin( px,py );
						const Vector2 csize( pixelSizeX, pixelSizeY );
						GetTheme()->DrawRawRectangle( cmin, csize, Color::GRAY, true );

						if ( (x==gridCameraX) && (y==gridCameraY) )
						{
							GetTheme()->DrawRawRectangle( cmin+Vector2(1,1), csize-Vector2(2,2), Color::GRAY, true );
						}
					}
				}
			}

			// draw elements
			if ( m_drawGridElements )
			{
				for ( const auto& elem : gridInfo.m_elems )
				{
					const Float x = drawBase.X + drawSize.X * elem.m_x;
					const Float y = drawBase.Y + drawSize.Y * elem.m_y;
					const Float r = elem.m_r * drawSize.X;

					GetTheme()->DrawRawCircle( Vector2(x,y), r, Color::WHITE, 16, false );
				}
			}

			// draw camera (quantized)
			{
				const Float cx = cameraX / 65535.0f;
				const Float cy = cameraY / 65535.0f;
				if ( cx >= 0.0f && cy >= 0.0f && cx <= 1.0f && cy <= 1.0f )
				{
					const Float px = drawBase.X + drawSize.X * cx;
					const Float py = drawBase.Y + drawSize.Y * cy;
					GetTheme()->DrawRawCircle( Vector2(px, py), 3.0f, Color::BLUE, 10, false );
				}
			}
			
			// Draw streaming state
			GetTheme()->DrawRawText( drawBase + Vector2( 0.5f, 5.0f ), 
				String::Printf( TXT("Level%d: Size=%ix%i Buckets=%d Elems=%d Waste=%d"),
					m_gridLevel, gridInfo.m_gridSize, gridInfo.m_gridSize, 
					gridInfo.m_numBuckets, gridInfo.m_numElements, gridInfo.m_numWastedElements ), 
				Color::WHITE );

			GetTheme()->DrawRawText( drawBase + Vector2( 0.5f, 5.0f + (Float)fh ), 
				String::Printf( TXT("Proxies: All=%d,  InRange=%d,  Streaming=%d"), 
					generalInfo.m_numProxiesRegistered,
					generalInfo.m_numProxiesInRange,
					generalInfo.m_numProxiesStreaming ),
				Color::WHITE );

			GetTheme()->DrawRawText( drawBase + Vector2( 0.5f, 5.0f + (Float)fh*2 ), 
				String::Printf( TXT("Camera: [%1.2f,%1.2f,%1.2f]   Quantized: [%d,%d]   Grid: [%d,%d]"),
				cameraPos.X, cameraPos.Y, cameraPos.Z,
				cameraX, cameraY, 
				gridCameraX, gridCameraY ),
				Color::WHITE );

			if ( generalInfo.m_numGridBucketsMax )
			{
				GetTheme()->DrawRawText( drawBase + Vector2( 0.5f, 5.0f + (Float)fh*3 ), 
					String::Printf( TXT("Buckets: Used=%d (%1.2f%%)"), 
					generalInfo.m_numGridBucketsUsed,
					(float)(100.0f * generalInfo.m_numGridBucketsUsed) / (float)generalInfo.m_numGridBucketsMax ),
					Color::WHITE );
			}
		}

	private:
		// histogram helper
		RED_INLINE static Color CalcColor( const Uint32 count, const Uint32 max )
		{
			const Uint8 a = (const Uint8)( 255*count / max );
			return Color( 200, 64, 64, a );
		}


		Uint32		m_gridLevel;
		Bool		m_drawGridHistogram;
		Bool		m_drawGridElements;
		Bool		m_drawGridCounts;
	};

	CDebugWindowWorldStreaming::CDebugWindowWorldStreaming()
		: RedGui::CRedGuiWindow( 300, 250, 800, 600 )
		, m_gridLevelBox( nullptr )
	{
		GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindowWorldStreaming::NotifyOnTick );
		
		// top row
		RedGui::CRedGuiGridLayout* menuRow1 = new RedGui::CRedGuiGridLayout( 0, 0, 100, 26 );
		menuRow1->SetDock( RedGui::DOCK_Top );
		menuRow1->SetMargin( Box2( 5, 5, 5, 5 ) );
		menuRow1->SetDimensions( 5, 1 );
		AddChild( menuRow1 );

		// grid levels
		m_gridLevelBox = new RedGui::CRedGuiComboBox( 40, 10, 80, 20 );
		m_gridLevelBox->AddItem( TXT("0") );
		m_gridLevelBox->AddItem( TXT("1") );
		m_gridLevelBox->AddItem( TXT("2") );
		m_gridLevelBox->AddItem( TXT("3") );
		m_gridLevelBox->AddItem( TXT("4") );
		m_gridLevelBox->AddItem( TXT("5") );
		m_gridLevelBox->AddItem( TXT("6") );
		m_gridLevelBox->AddItem( TXT("7") );
		m_gridLevelBox->AddItem( TXT("8") );
		m_gridLevelBox->AddItem( TXT("9") );
		m_gridLevelBox->SetSelectedIndex( 0 );
		m_gridLevelBox->SetSimpleToolTip( TXT("Grid level") );
		m_gridLevelBox->SetMargin( Box2( 15, 5, 10, 0 ) );
		m_gridLevelBox->EventSelectedIndexChanged.Bind( this, &CDebugWindowWorldStreaming::NotifyEventGridLevelSelected );
		menuRow1->AddChild( m_gridLevelBox );

		// show elements option
		m_gridShowElements = new RedGui::CRedGuiCheckBox( 150, 10, 70, 20 );
		m_gridShowElements->SetText( TXT("Show proxies") );
		m_gridShowElements->SetMargin( Box2( 15, 5, 10, 0 ) );
		m_gridShowElements->SetChecked( false );
		menuRow1->AddChild( m_gridShowElements );

		// show histogram option
		m_gridShowHistogram = new RedGui::CRedGuiCheckBox( 210, 10, 70, 20 );
		m_gridShowHistogram->SetText( TXT("Show histogram") );
		m_gridShowHistogram->SetMargin( Box2( 15, 5, 10, 0 ) );
		m_gridShowHistogram->SetChecked( true );
		menuRow1->AddChild( m_gridShowHistogram );

		// show element counts
		m_gridShowCounts = new RedGui::CRedGuiCheckBox( 270, 10, 70, 20 );
		m_gridShowCounts->SetText( TXT("Show counts") );
		m_gridShowCounts->SetMargin( Box2( 15, 5, 10, 0 ) );
		m_gridShowCounts->SetChecked( false );
		menuRow1->AddChild( m_gridShowCounts );

		// bind events
		m_gridShowElements->EventCheckedChanged.Bind( this, &CDebugWindowWorldStreaming::NotifyEventOptionChanged );
		m_gridShowHistogram->EventCheckedChanged.Bind( this, &CDebugWindowWorldStreaming::NotifyEventOptionChanged );
		m_gridShowCounts->EventCheckedChanged.Bind( this, &CDebugWindowWorldStreaming::NotifyEventOptionChanged );

		// Streaming grid
		m_custom = new CDebugWindowWSGrid();
		m_custom->SetDock( RedGui::DOCK_Fill );
		AddChild( m_custom );
	}

	CDebugWindowWorldStreaming::~CDebugWindowWorldStreaming()
	{
		GRedGui::GetInstance().EventTick.Unbind( this, &CDebugWindowWorldStreaming::NotifyOnTick );
	}

	void CDebugWindowWorldStreaming::OnWindowOpened( CRedGuiControl* control )
	{
	}

	void CDebugWindowWorldStreaming::NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime )
	{
	}

	void CDebugWindowWorldStreaming::NotifyEventGridLevelSelected( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedIndex )
	{
		m_custom->SetGridLevel( selectedIndex );
	}

	void CDebugWindowWorldStreaming::NotifyEventOptionChanged( RedGui::CRedGuiEventPackage& eventPackage, Bool option )
	{
		m_custom->ToggleElementsDrawing( m_gridShowElements->GetChecked() );
		m_custom->ToggleHistogramDrawing( m_gridShowHistogram->GetChecked() );
		m_custom->ToggleCountDrawing( m_gridShowCounts->GetChecked() );
	}
}


#endif
#endif
