/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "memoryMetricsMemoryUsageControl.h"

#include "redGuiLabel.h"
#include "redGuiPanel.h"
#include "redGuiScrollBar.h"

#include "memoryClassDebugColourPalette.h"
#include "redGuiManager.h"

namespace DebugWindows
{
	static const Color FREE_COLOUR(   0,   0,   0, 255 );
	static const Color LOCK_COLOUR(   0,   0, 102, 255 );

	CMemoryMetricsUsageRenderControl::CMemoryMetricsUsageRenderControl( Uint32 x, Uint32 y, Uint32 width, Uint32 height )
	:	CRedGuiControl( x, y, width, height )
	,	m_selectedBlockIndex( static_cast< Uint32 >( -1 ) )
	,	m_selectedMemoryClass( static_cast< Uint16 >( -1 ) )
	{
		SetBorderVisible( false );
		SetBackgroundColor( Color::CLEAR );

		// create tooltip
		m_tooltipPanel = new RedGui::CRedGuiPanel( 0, 0, 100, 30 );
		m_tooltipPanel->SetBackgroundColor( Color( 75, 75, 75, 255 ) );
		m_tooltipPanel->AttachToLayer( TXT( "Pointers" ) );
		m_tooltipPanel->SetVisible( false );
		m_tooltipPanel->SetAutoSize( false );

		m_tooltipMemoryClass = new RedGui::CRedGuiLabel( 0, 0, 10, 10 );
		m_tooltipMemoryClass->SetMargin( Box2( 3, 3, 3, 3 ) );
		m_tooltipMemoryClass->SetAlign( RedGui::IA_MiddleCenter );
		m_tooltipMemoryClass->SetDock( RedGui::DOCK_Top );
		m_tooltipMemoryClass->SetAutoSize( true );

		m_tooltipAddress = new RedGui::CRedGuiLabel( 0, 0, 10, 10 );
		m_tooltipAddress->SetMargin( Box2( 3, 3, 3, 3 ) );
		m_tooltipAddress->SetAlign( RedGui::IA_MiddleCenter );
		m_tooltipAddress->SetDock( RedGui::DOCK_Top );
		m_tooltipAddress->SetText( TXT( "TEMP" ) );
		m_tooltipAddress->SetAutoSize( true );

		m_tooltipPanel->SetAutoSize( true );
		m_tooltipPanel->AddChild( m_tooltipMemoryClass );
		m_tooltipPanel->AddChild( m_tooltipAddress );

		m_walker.SetResolution( c_blocksWide, c_blocksHigh );
	}

	CMemoryMetricsUsageRenderControl::~CMemoryMetricsUsageRenderControl()
	{
		/* intentionally empty */
	}

	void CMemoryMetricsUsageRenderControl::DrawRange( Uint32 x1, Uint32 x2, Uint32 y )
	{
		//const Color freeColour		(   0, 255,   0, 255 );
		const Color highlightColour	( 224, 245,  32, 255 );

		const MemoryMetricsUsageControlWalker::Block& startBlock = m_walker.GetBlock( x1, y );
		const MemoryMetricsUsageControlWalker::Block& endBlock = m_walker.GetBlock( x2, y );

		if( !startBlock.address )
			return;

		const MemoryMetricsUsageControlWalker::BlockMetrics* blockMetrics = m_walker.GetMetricsForBlock( startBlock );

		if( !blockMetrics )
			return;

		Color colour;

		if( m_highlightedMemoryAddress >= startBlock.address && m_highlightedMemoryAddress <= endBlock.address )
		{
			colour = highlightColour;
		}
		else if( blockMetrics->used )
		{
			colour = GenerateMemoryClassColour( blockMetrics->memoryClass );
		}
		else if( blockMetrics->locked )
		{
			colour = LOCK_COLOUR;
		}
		else if( blockMetrics->free )
		{
			colour = FREE_COLOUR;
		}

		DrawRange( x1, x2, y, colour );
	}

	void CMemoryMetricsUsageRenderControl::DrawRange( Uint32 x1, Uint32 x2, Uint32 y, const Color& colour )
	{
		const Uint32 pxPerBlockW = GetBlockWidth();
		const Uint32 pxPerBlockH = GetBlockHeight();

		GetTheme()->DrawRawFilledRectangle
		(
			Vector2
			(
				(Float)( GetAbsoluteLeft() + ( x1 * pxPerBlockW ) ),
				(Float)( GetAbsoluteTop() + ( y * pxPerBlockH ) )
			),
			Vector2
			(
				(Float)( ( ( x2 + 1 ) - x1 ) * pxPerBlockW ),
				(Float)pxPerBlockH
			),
			colour
		);
	}

	void CMemoryMetricsUsageRenderControl::Draw()
	{
		GetTheme()->SetCroppedParent( this );

		if( m_walker.GetNumBlocks() > 0 )
		{
			if( m_selectedMemoryClass < MC_Max )
			{
				DrawSelectedClass();
			}
			else
			{
				DrawMostSignificantClass();
			}
		}

		GetTheme()->ResetCroppedParent();
	}

	void CMemoryMetricsUsageRenderControl::DrawMostSignificantClass()
	{
		const MemoryMetricsUsageControlWalker::BlockMetrics* prevBlockMetrics = nullptr;

		for( Uint32 y = 0; y < c_blocksHigh; ++y )
		{
			Uint32 currentMergeX = 0;

			for( Uint32 x = 0; x < c_blocksWide; ++x )
			{
				const MemoryMetricsUsageControlWalker::Block& block = m_walker.GetBlock( x, y );
				const MemoryMetricsUsageControlWalker::BlockMetrics* metrics = m_walker.GetMetricsForBlock( block );

				if( prevBlockMetrics && ( !metrics || *prevBlockMetrics != *metrics ) )
				{
					DrawRange( currentMergeX, x - 1, y );

					currentMergeX = x;
				}

				prevBlockMetrics = metrics;
			}

			DrawRange( currentMergeX, c_blocksWide - 1, y );
		}
	}

	void CMemoryMetricsUsageRenderControl::DrawSelectedClass()
	{
		for( Uint32 y = 0; y < c_blocksHigh; ++y )
		{
			Bool activeRange = false;
			Uint32 currentMergeX = 0;

			for( Uint32 x = 0; x < c_blocksWide; ++x )
			{
				const MemoryMetricsUsageControlWalker::Block& block = m_walker.GetBlock( x, y );

				Bool foundClass = false;
				for( Uint32 m = 0; m < block.metricsCount; ++m )
				{
					const MemoryMetricsUsageControlWalker::BlockMetrics* metrics = m_walker.GetMetricsForBlock( block, m );

					if( metrics->used && metrics->memoryClass == m_selectedMemoryClass )
					{
						if( x > 0 && !activeRange )
						{
							DrawRange( currentMergeX, x - 1, y, Color::BLACK );
							currentMergeX = x;
						}

						activeRange = true;
						foundClass = true;

						break;
					}
				}

				if( !foundClass && activeRange )
				{
					DrawRange( currentMergeX, x - 1, y );
					activeRange = false;
					currentMergeX = x;
				}
			}

			if( activeRange )
			{
				DrawRange( currentMergeX, c_blocksWide - 1, y );
			}
			else
			{
				DrawRange( currentMergeX, c_blocksWide - 1, y, Color::BLACK );
			}
		}
	}

	void CMemoryMetricsUsageRenderControl::ResetMemoryPool()
	{
		m_walker.Clear();
		m_walker.SetResolution( c_blocksWide, c_blocksHigh );
	}

	void CMemoryMetricsUsageRenderControl::SetMemory( Red::MemoryFramework::IAllocator* poolAllocator, Red::System::MemUint address, Red::System::MemSize size )
	{
		m_walker.AddArea( poolAllocator, address, size );
	}

	void CMemoryMetricsUsageRenderControl::Update()
	{
		m_walker.Walk();
	}

	Uint32 CMemoryMetricsUsageRenderControl::BlockIndexFromMousePosition( const Vector2& mousePosition ) const
	{
		const Uint32 pxPerBlockW = GetBlockWidth();
		const Uint32 pxPerBlockH = GetBlockHeight();

		Vector2 relativeMousePosition = mousePosition - GetAbsolutePosition();

		Uint32 x = static_cast< Uint32 >( relativeMousePosition.X / pxPerBlockW );
		Uint32 y = static_cast< Uint32 >( relativeMousePosition.Y / pxPerBlockH );

		if( y < m_walker.GetHeight() && x < m_walker.GetWidth() )
		{
			return x + ( y * m_walker.GetWidth() );
		}

		return static_cast< Uint32 >( -1 );
	}

	Uint32 CMemoryMetricsUsageRenderControl::GetLastBlockInAllocation( Uint32 index ) const
	{
		RED_FATAL_ASSERT( index < m_walker.GetNumBlocks(), "Invalid starting index" );

		MemUint endAddress = m_walker.GetBlock( index ).address;
		Uint32 endIndex = index + 1;
		while( endIndex < m_walker.GetNumBlocks() && m_walker.GetBlock( endIndex ).address == endAddress )
		{
			index = endIndex;
			++endIndex;
		}

		return index;
	}

	void CMemoryMetricsUsageRenderControl::OnMouseMove( const Vector2& mousePosition )
	{
		Uint32 blockIndex = BlockIndexFromMousePosition( mousePosition );

		if( blockIndex < m_walker.GetNumBlocks() )
		{
			// Find the last block in the highlighted range, as that will have more than one allocation
			blockIndex = GetLastBlockInAllocation( blockIndex );

			const MemoryMetricsUsageControlWalker::Block& block = m_walker.GetBlock( blockIndex );
			m_highlightedMemoryAddress = block.address;

			if( !m_highlightedMemoryAddress || ( block.metricsStart + block.metricsCount ) > m_walker.GetNumMetricsBlocks() )
			{
				m_tooltipPanel->SetVisible( false );
				return;
			}

			m_tooltipAddress->SetText( String::Printf( TXT( "%p" ), block.address ) );
			m_tooltipMemoryClass->SetText( String::Printf( TXT( "%u areas" ), block.metricsCount ) );

			// Tooltip position and size
			Vector2 position = mousePosition - m_tooltipPanel->GetSize() - Vector2( 15.0f, 15.0f );
			
			if( position.X < 0 )
			{
				position.X = 0;
			}
			
			if( position.Y < 0 )
			{
				position.Y = 0;
			}
			
			Vector2 viewSize = GRedGui::GetInstance().GetRenderManager()->GetViewSize();
			
			if( position.X + m_tooltipPanel->GetWidth() > viewSize.X )
			{
				position.X -= ( position.X + m_tooltipPanel->GetWidth() ) - viewSize.X;
			}
			
			if( position.Y + m_tooltipPanel->GetHeight() > viewSize.Y )
			{
				position.Y -= ( position.Y + m_tooltipPanel->GetHeight() ) - viewSize.Y;
			}
			
			m_tooltipPanel->SetPosition( position );
			m_tooltipPanel->SetVisible( true );
		}
		else
		{
			m_tooltipPanel->SetVisible( false );
		}
	}

	void CMemoryMetricsUsageRenderControl::OnMouseLostFocus( CRedGuiControl* )
	{
		m_tooltipPanel->SetVisible( false );
	}

	void CMemoryMetricsUsageRenderControl::OnMouseButtonReleased( const Vector2&, RedGui::EMouseButton button )
	{
		if( button == RedGui::MB_Right )
		{
			if( m_selectedMemoryClass < MC_Max )
			{
				m_selectedMemoryClass = static_cast< Uint16 >( -1 );

				if( m_memoryClassDeselectedCallback )
				{
					m_memoryClassDeselectedCallback();
				}
			}
			else if( m_selectedBlockIndex < m_walker.GetNumBlocks() )
			{
				m_selectedBlockIndex = static_cast< Uint32 >( -1 );
			}
			else
			{
				m_walker.ZoomOut();
			}
		}
	}

	void CMemoryMetricsUsageRenderControl::OnMouseButtonClick( const Vector2& mousePosition, RedGui::EMouseButton button )
	{
		if( button == RedGui::MB_Left )
		{
			m_selectedBlockIndex = BlockIndexFromMousePosition( mousePosition );

			if( m_selectedBlockIndex < m_walker.GetNumBlocks() )
			{
				m_selectedBlockIndex = GetLastBlockInAllocation( m_selectedBlockIndex );
			}
		}
	}

	void CMemoryMetricsUsageRenderControl::OnMouseButtonDoubleClick( const Vector2&, RedGui::EMouseButton button )
	{
		if( button == RedGui::MB_Left )
		{
			m_walker.ZoomIn( m_highlightedMemoryAddress );
		}
	}

	const MemoryMetricsUsageControlWalker::Block* CMemoryMetricsUsageRenderControl::GetSelectedBlock() const
	{
		if( m_selectedBlockIndex < m_walker.GetNumBlocks() )
		{
			return &m_walker.GetBlock( m_selectedBlockIndex );
		}

		return nullptr;
	}

	const MemoryMetricsUsageControlWalker::BlockMetrics* CMemoryMetricsUsageRenderControl::GetSelectedBlockMetrics() const
	{
		const MemoryMetricsUsageControlWalker::Block* block = GetSelectedBlock();

		if( block )
		{
			if( block->metricsStart + block->metricsCount <= m_walker.GetNumMetricsBlocks() )
			{
				return m_walker.GetMetricsForBlock( *block );
			}
		}

		return nullptr;
	}

	void CMemoryMetricsUsageRenderControl::SetMemoryClassDeselectionCallback( std::function< void() > callback )
	{
		m_memoryClassDeselectedCallback = callback;
	}

	//////////////////////////////////////////////////////////////////////////
	// CMemoryMetricsUsageControl
	CMemoryMetricsUsageControl::CMemoryMetricsUsageControl( Uint32 x, Uint32 y, Uint32 width, Uint32 height )
	:	CRedGuiControl( x, y, width, height )
	{
		SetBorderVisible( false );
		SetBackgroundColor( Color::CLEAR );

		m_legend = new CMemoryMetricsUsageLegendControl( 0, 0, 200, 100 );

		m_legend->SetDock( RedGui::DOCK_Right );
		m_legend->SetMargin( Box2( 5, 5, 5 ,5 ) );
		m_legend->SetBackgroundColor( Color::CLEAR );
		m_legend->SetBorderVisible( true );
		AddChild( m_legend );

		m_render = new CMemoryMetricsUsageRenderControl( 0, 0, 100, 100 );

		CMemoryMetricsUsageLegendControl* legend = m_legend;
		m_render->SetMemoryClassDeselectionCallback( [ legend ](){ legend->DeselectMemoryClass(); } );

		m_render->SetDock( RedGui::DOCK_Fill );
		m_render->SetMargin( Box2( 5, 5, 5 ,5 ) );
		m_render->SetBackgroundColor( Color::CLEAR );
		m_render->SetBorderVisible( true );
		AddChild( m_render );
	}

	CMemoryMetricsUsageControl::~CMemoryMetricsUsageControl()
	{
	}

	void CMemoryMetricsUsageControl::ResetMemoryPool()
	{
		m_render->ResetMemoryPool();
	}

	void CMemoryMetricsUsageControl::SetMemory( Red::MemoryFramework::IAllocator* poolAllocator, MemUint address, MemSize size )
	{
		m_render->SetMemory( poolAllocator, address, size );
	}

	void CMemoryMetricsUsageControl::Update()
	{
		m_render->Update();

		m_legend->Update( m_render->GetBytesPerBlock(), m_render->GetLargestFreeArea() );
		m_render->SetSelectedMemoryClass( m_legend->GetSelectedMemoryClass() );

		const MemoryMetricsUsageControlWalker::BlockMetrics* metrics = m_render->GetSelectedBlockMetrics();
		if( metrics )
		{
			const MemoryMetricsUsageControlWalker::Block* block = m_render->GetSelectedBlock();

			m_legend->ShowMetrics( block->address, metrics, block->metricsCount );
		}
		else
		{
			m_legend->HideMetrics();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// CMemoryMetricsUsageLegendControl
	CMemoryMetricsUsageLegendControl::CMemoryMetricsUsageLegendControl( Uint32 x, Uint32 y, Uint32 width, Uint32 height )
	:	CRedGuiControl( x, y, width, height )
	,	m_selectedEntry( static_cast< Uint32 >( -1 ) )
	,	m_selectedMemoryClass( static_cast< Uint16 >( -1 ) )
	{
		m_ScrollBar = new RedGui::CRedGuiScrollBar( 0, 0, 10, 100 );

		m_ScrollBar->SetDock( RedGui::DOCK_Right );
		m_ScrollBar->SetMargin( Box2( 5, 5, 5 ,5 ) );
		m_ScrollBar->SetBackgroundColor( Color::CLEAR );
		m_ScrollBar->SetBorderVisible( true );

		AddChild( m_ScrollBar );
	}

	CMemoryMetricsUsageLegendControl::~CMemoryMetricsUsageLegendControl()
	{

	}

	const Float k_spacing = 2.0f;
	const Float k_keyBoxSize = 14.0f;
	const Float k_keyTextOffset = 4.0f;
	const Float k_legendOffset = 10.0f;
	const Float k_entrySize = k_keyBoxSize + k_spacing;
	const Color k_highlightColour( 224, 245,  32, 255 );

	RED_INLINE Float CMemoryMetricsUsageLegendControl::GetInitialKeyTextOffset() const
	{
		return k_spacing - ( m_ScrollBar->GetScrollPosition() * k_entrySize );
	}

	void CMemoryMetricsUsageLegendControl::DrawEntry( Float& keyTextOffsetY, const String& text, const Vector2& origin )
	{
		Vector2 keyBoxOrigin( k_legendOffset, keyTextOffsetY );
		GetTheme()->DrawRawText( origin + keyBoxOrigin + Vector2( k_keyBoxSize + k_keyTextOffset, k_spacing ), text, Color::WHITE );
		keyTextOffsetY += k_entrySize;
	}

	void CMemoryMetricsUsageLegendControl::DrawEntry( Float& keyTextOffsetY, const Color& colour, const String& text, const Vector2& origin, Bool highlight )
	{
		Vector2 keyBoxOrigin( k_legendOffset, keyTextOffsetY );
		GetTheme()->DrawRawFilledRectangle( origin + keyBoxOrigin, Vector2( k_keyBoxSize, k_keyBoxSize ), colour );
		
		if( highlight )
		{
			GetTheme()->DrawRawRectangle( origin + keyBoxOrigin, Vector2( k_keyBoxSize, k_keyBoxSize ), k_highlightColour );
		}

		GetTheme()->DrawRawText( origin + keyBoxOrigin + Vector2( k_keyBoxSize + k_keyTextOffset, k_spacing ), text, Color::WHITE );
		keyTextOffsetY += k_entrySize;
	}

	void CMemoryMetricsUsageLegendControl::DrawLegend()
	{
		
		const Vector2 origin = GetAbsolutePosition();

		Float keyTextOffsetY = GetInitialKeyTextOffset();

		Int32 scrollRange = ( MC_Max - (Int32)( GetHeight() / k_entrySize ) ) + 2;
		m_ScrollBar->SetScrollRange( scrollRange );

		String blockSizeInfo = String::Printf( TXT( "Block Size: %ls" ), GetFormattedSize( m_bytesPerBlock ).AsChar() );
		DrawEntry( keyTextOffsetY, blockSizeInfo, origin );

		String largestFreeAreaInfo = String::Printf( TXT( "Largest free area: %ls" ), GetFormattedSize( m_largestFreeArea ).AsChar() );
		DrawEntry( keyTextOffsetY, largestFreeAreaInfo, origin );

		DrawEntry( keyTextOffsetY, String::EMPTY, origin );

		for( Uint16 i = 0; i < MC_Max; ++i )
		{
			const Uint32 currentEntry = i + 3;
			Color colour;

			if( m_selectedEntry == currentEntry )
			{
				m_selectedMemoryClass = i;
				colour = k_highlightColour;
			}
			else
			{
				colour = GenerateMemoryClassColour( i );
			}

			AnsiChar buffer[ 128 ];
			Memory::GetMemoryClassName( i, buffer, 128 );
			String text = String::Printf( TXT( "%hs" ), buffer );

			DrawEntry( keyTextOffsetY, colour, text, origin, ( currentEntry == m_highlightedEntry ) );
		}
	}

	void CMemoryMetricsUsageLegendControl::DrawMetrics()
	{
		const Vector2 origin = GetAbsolutePosition();
		const Vector2 sizeOrigin = origin + Vector2( 5.0f, 0.0f );

		Float keyTextOffsetY = GetInitialKeyTextOffset();

		Uint32 numItems = ( m_metricsCount * 2 ) + 3;
		Uint32 scrollRange = ( numItems - (Uint32)( GetHeight() / ( k_keyBoxSize + k_spacing ) ) );
		m_ScrollBar->SetScrollRange( scrollRange );

		String blockSizeInfo = String::Printf( TXT( "Block Size: %ls" ), GetFormattedSize( m_bytesPerBlock ).AsChar() );
		DrawEntry( keyTextOffsetY, blockSizeInfo, origin );

		String largestFreeAreaInfo = String::Printf( TXT( "Largest free area: %ls" ), GetFormattedSize( m_largestFreeArea ).AsChar() );
		DrawEntry( keyTextOffsetY, largestFreeAreaInfo, origin );

		String addressText = String::Printf( TXT( "Addr: %p" ), m_selectedAddress );
		DrawEntry( keyTextOffsetY, addressText, origin );

		DrawEntry( keyTextOffsetY, String::EMPTY, origin );

		for( Uint32 i = 0; i < m_metricsCount; ++i )
		{
			const Uint32 currentEntry = ( i * 2 ) + 4;

			const MemoryMetricsUsageControlWalker::BlockMetrics& blockMetrics = m_metrics[ i ];

			Color colour;

			const Uint32 bufferSize = 128;
			Char buffer[ bufferSize ];
			const Char* desc = nullptr;

			if( blockMetrics.used )
			{
				AnsiChar narrowBuffer[ bufferSize ];
				Memory::GetMemoryClassName( blockMetrics.memoryClass, narrowBuffer, bufferSize );
				Red::System::StringConvert( buffer, narrowBuffer, bufferSize );

				desc = buffer;

				if( currentEntry == m_selectedEntry || currentEntry + 1 == m_selectedEntry )
				{
					colour = k_highlightColour;
					m_selectedMemoryClass = blockMetrics.memoryClass;
				}
				else
				{
					colour = GenerateMemoryClassColour( blockMetrics.memoryClass );
				}
			}
			else if( blockMetrics.free )
			{
				desc = TXT( "Free" );
				colour = FREE_COLOUR;
			}
			else if( blockMetrics.locked )
			{
				desc = TXT( "Locked" );
				colour = LOCK_COLOUR;
			}
			else
			{
				desc = TXT( "Error" );
				colour = Color::WHITE;
			}

			Bool highlight = ( currentEntry == m_highlightedEntry || currentEntry + 1 == m_highlightedEntry );

			DrawEntry( keyTextOffsetY, colour, String::Printf( TXT( "%u) %ls" ), i, desc ), origin, highlight );
			DrawEntry( keyTextOffsetY, GetFormattedSize( blockMetrics.size ), origin );
		}
	}

	void CMemoryMetricsUsageLegendControl::Draw()
	{
		if( m_metrics )
		{
			DrawMetrics();
		}
		else
		{
			DrawLegend();
		}
	}

	void CMemoryMetricsUsageLegendControl::OnMouseMove( const Vector2& mousePosition )
	{
		const Vector2 origin = GetAbsolutePosition();
		const Float keyTextOffsetY = GetInitialKeyTextOffset();

		const Float relativeMouseY = mousePosition.Y - origin.Y - keyTextOffsetY;

		m_highlightedEntry = static_cast< Uint32 >( relativeMouseY / k_entrySize );
	}

	void CMemoryMetricsUsageLegendControl::OnMouseButtonReleased( const Vector2& mousePosition, RedGui::EMouseButton button )  
	{
		if( button == RedGui::MB_Left )
		{
			DeselectMemoryClass();
			m_selectedEntry = m_highlightedEntry;
		}
		else if( button == RedGui::MB_Right )
		{
			DeselectMemoryClass();
		}
	}

	void CMemoryMetricsUsageLegendControl::OnMouseLostFocus( CRedGuiControl* )
	{
		m_highlightedEntry = static_cast< Uint32 >( -1 );
	}

	void CMemoryMetricsUsageLegendControl::Update( MemSize bytesPerBlock, MemSize largestFreeArea )
	{
		m_bytesPerBlock = bytesPerBlock;
		m_largestFreeArea = largestFreeArea;
	}

	String CMemoryMetricsUsageLegendControl::GetFormattedSize( Red::System::Int64 size )
	{
		const Red::System::Int64 oneKb = 1024;
		const Red::System::Int64 oneMb = 1024 * 1024;
		const Red::System::Int64 oneGb = 1024 * 1024 * 1024;

		if( size < oneKb )
		{
			return String::Printf( TXT( "%lld Bytes" ), size );
		}
		else if( size < oneMb )
		{
			Float sizeAsFloat = (Float)size / (Float)oneKb;
			return String::Printf( TXT( "%.1f Kb" ), sizeAsFloat );
		}
		else if( size < oneGb )
		{
			Float sizeAsFloat = (Float)size / (Float)oneMb;
			return String::Printf( TXT( "%.2f Mb" ), sizeAsFloat );
		}
		else
		{
			Float sizeAsFloat = (Float)size / (Float)oneGb;
			return String::Printf( TXT( "%.2f Gb" ), sizeAsFloat );
		}
	}

	Uint32 CMemoryMetricsUsageLegendControl::FindSelectedMemoryClassIndex( const MemoryMetricsUsageControlWalker::BlockMetrics* metrics, Uint32 count ) const
	{
		for( Uint32 i = 0; i < count; ++i )
		{
			const MemoryMetricsUsageControlWalker::BlockMetrics& blockMetrics = metrics[ i ];

			if( blockMetrics.used && blockMetrics.memoryClass == m_selectedMemoryClass )
			{
				// This needs to match the behaviour of CMemoryMetricsUsageLegendControl::DrawMetrics()
				// +4 for how many lines are printed before the memory classes are listed
				// *2 as each memory class occupies 2 lines
				return ( i * 2 ) + 4;
			}
		}

		return static_cast< Uint32 >( -1 );
	}

	void CMemoryMetricsUsageLegendControl::ShowMetrics( MemUint selectedAddress, const MemoryMetricsUsageControlWalker::BlockMetrics* metrics, Uint32 count )
	{
		if( m_selectedAddress != selectedAddress && m_selectedMemoryClass != static_cast< Uint16 >( -1 ) )
		{
			m_selectedEntry = FindSelectedMemoryClassIndex( metrics, count );
		}

		m_selectedAddress = selectedAddress;
		m_metrics = metrics;
		m_metricsCount = count;
	}

	void CMemoryMetricsUsageLegendControl::HideMetrics()
	{
		m_metrics = nullptr; m_metricsCount = 0;
	}

}	// namespace DebugWindows

#endif	// NO_RED_GUI
