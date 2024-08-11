/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiLabel.h"
#include "redGuiPanel.h"
#include "memoryMetricsPoolAreaControl.h"
#include "memoryClassDebugColourPalette.h"

namespace DebugWindows
{
	CMemoryMetricsPoolAreaControl::CMemoryMetricsPoolAreaControl( Uint32 x, Uint32 y, Uint32 width, Uint32 height )
		: CRedGuiControl( x, y, width, height )
		, m_drawPanel( nullptr )
		, m_label( nullptr )
		, m_poolAllocator( nullptr )
		
	{
		SetBorderVisible( false );
		SetBackgroundColor( Color::CLEAR );

		m_label = new RedGui::CRedGuiLabel( 0, 0, 100, 15 );
		m_label->SetMargin( Box2( 5, 0, 5, 0 ) );
		m_label->SetDock( RedGui::DOCK_Top );
		AddChild( m_label );

		m_drawPanel = new RedGui::CRedGuiPanel( 0, 0, 100, 100 );
		m_drawPanel->SetDock( RedGui::DOCK_Fill );
		m_drawPanel->SetMargin( Box2( 5, 5, 5 ,5 ) );
		m_drawPanel->SetBackgroundColor( Color::CLEAR );
		m_drawPanel->SetBorderVisible( false );
		AddChild( m_drawPanel );
	}

	CMemoryMetricsPoolAreaControl::~CMemoryMetricsPoolAreaControl()
	{
		/* intentionally empty */
	}

	void CMemoryMetricsPoolAreaControl::Draw()
	{
		GetTheme()->DrawPanel( this );

		const Vector freeColour( 0.0f, 1.0f, 0.0f, 1.0f );
		const Vector usedColour( 1.0f, 0.0f, 0.0f, 1.0f );

		GetTheme()->SetCroppedParent( m_drawPanel );

		// reset
		m_currentOffset = 0;
		m_blockFreeCounter = 0;
		m_blockUsedCounter = 0;
		m_totalUsed = 0;
		m_blockPxByteBoundary = m_size / m_drawPanel->GetWidth();

		m_poolAllocator->WalkPoolArea( m_address, m_size, this );

		GetTheme()->ResetCroppedParent();
	}

	void CMemoryMetricsPoolAreaControl::SetMemory( Red::MemoryFramework::IAllocator* poolAllocator, Red::System::MemUint address, Red::System::MemSize size )
	{
		m_poolAllocator = poolAllocator;
		m_address = address;
		m_size = size;

		m_label->SetText( String::Printf( TXT( "0x%p - %s" ), m_address, GetFormattedSize( m_size ).AsChar() ) );
	}

	String CMemoryMetricsPoolAreaControl::GetFormattedSize( Red::System::Int64 size )
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

		return String::EMPTY;
	}

	Red::System::MemUint CMemoryMetricsPoolAreaControl::GetMemoryAddress() const
	{
		return m_address;
	}

	void CMemoryMetricsPoolAreaControl::OnUsedArea( Red::System::MemUint address, Red::System::MemSize size, Red::System::Uint16 memoryClass )
	{
		RED_UNUSED( address );
		m_blockUsedCounter += size;
		m_totalUsed += size;

		Red::System::MemSize currentBlockSize = m_blockUsedCounter + m_blockFreeCounter;
		if( currentBlockSize > m_blockPxByteBoundary )	// Its big enough to draw a block of at least 1 pixel
		{
			// draw the thing and reset the counters
			Color color = GenerateMemoryClassColour( static_cast< Red::MemoryFramework::MemoryClass >( memoryClass ) );
			DrawAreaBlockAndReset( color.ToVector() );
		}
	}

	void CMemoryMetricsPoolAreaControl::OnFreeArea( Red::System::MemUint address, Red::System::MemSize size )
	{
		RED_UNUSED( address );
		m_blockFreeCounter += size;

		Red::System::MemSize currentBlockSize = m_blockUsedCounter + m_blockFreeCounter;
		if( currentBlockSize > m_blockPxByteBoundary )	// Its big enough to draw a block of at least 1 pixel
		{
			// draw the thing and reset the counters
			DrawAreaBlockAndReset();
		}
	}

	void CMemoryMetricsPoolAreaControl::DrawAreaBlockAndReset( const Vector& usedColor )
	{
		const Vector c_freeColour( 0.0f, 1.0f, 0.0f, 1.0f );

		// Draw the block
		Red::System::MemSize blockSizeBytes = m_blockUsedCounter + m_blockFreeCounter;
		Red::System::MemSize blockSizePx = blockSizeBytes / m_blockPxByteBoundary;
		Int32 blockSizePxInt = (Int32)blockSizePx;
		Float freeFloat = (Float) m_blockFreeCounter;
		Float usedFloat = (Float) m_blockUsedCounter;
		Float freeRatio = freeFloat / (freeFloat + usedFloat);
		Vector actualColour = usedColor * (1.0f - freeRatio);
		actualColour = actualColour + c_freeColour * freeRatio;

		Color colourToDraw( (Uint8)(actualColour.X * 255.0f), (Uint8)(actualColour.Y * 255.0f), (Uint8)(actualColour.Z * 255.0f) );

		GetTheme()->DrawRawFilledRectangle( Vector2( (Float)( m_drawPanel->GetAbsoluteLeft() + m_currentOffset ), (Float)m_drawPanel->GetAbsoluteTop() ), Vector2( (Float)blockSizePxInt, (Float)m_drawPanel->GetHeight() ), colourToDraw );

		// Reset stats. Try to scale them back properly
		m_blockFreeCounter = ( Red::System::MemSize )( m_blockFreeCounter - MFloor( freeRatio * (Float)blockSizePx * m_blockPxByteBoundary ) );
		m_blockUsedCounter = ( Red::System::MemSize )( m_blockUsedCounter - MFloor( (1.0f - freeRatio ) * (Float)blockSizePx * m_blockPxByteBoundary ) );
		m_currentOffset += blockSizePxInt;
	}

}	// namespace DebugWindows

#endif	// NO_RED_GUI
