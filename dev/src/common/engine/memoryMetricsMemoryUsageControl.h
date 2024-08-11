/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "../redMath/numericalutils.h"

#include "redGuiControl.h"
#include "redGuiEnumerates.h"

#include "memoryMetricsUsageControlWalker.h"

namespace DebugWindows
{
	class CMemoryMetricsUsageLegendControl : public RedGui::CRedGuiControl
	{
	public:
		CMemoryMetricsUsageLegendControl( Uint32 x, Uint32 y, Uint32 width, Uint32 height );
		virtual ~CMemoryMetricsUsageLegendControl() override final;

		virtual void Draw() override final;

		void Update( MemSize bytesPerBlock, MemSize largestFreeBlock );

		void ShowMetrics( MemUint selectedAddress, const MemoryMetricsUsageControlWalker::BlockMetrics* metrics, Uint32 count );
		void HideMetrics();

		RED_INLINE Uint16 GetSelectedMemoryClass() const { return m_selectedMemoryClass; }
		RED_INLINE void DeselectMemoryClass() { m_selectedEntry = static_cast< Uint32 >( -1 ); m_selectedMemoryClass = static_cast< Uint16 >( -1 ); }

	protected:
		virtual void OnMouseMove( const Vector2& mousePosition ) override final;
		virtual void OnMouseButtonReleased( const Vector2& mousePosition, RedGui::EMouseButton button ) override final;
		virtual void OnMouseLostFocus( CRedGuiControl* newControl ) override final;

	private:
		String GetFormattedSize( Red::System::Int64 size );
		
		void DrawEntry( Float& keyTextOffsetY, const Color& colour, const String& text, const Vector2& origin, Bool highlight );
		void DrawEntry( Float& keyTextOffsetY, const String& text, const Vector2& origin );
		void DrawLegend();
		void DrawMetrics();

		Float GetInitialKeyTextOffset() const;
		Uint32 FindSelectedMemoryClassIndex( const MemoryMetricsUsageControlWalker::BlockMetrics* metrics, Uint32 count ) const;

		RedGui::CRedGuiScrollBar* m_ScrollBar;
		MemSize m_bytesPerBlock;
		MemSize m_largestFreeArea;
		MemUint m_selectedAddress;

		const MemoryMetricsUsageControlWalker::BlockMetrics* m_metrics;
		Uint32 m_metricsCount;

		Uint32 m_highlightedEntry;
		Uint32 m_selectedEntry;
		Uint16 m_selectedMemoryClass;
	};

	class CMemoryMetricsUsageRenderControl : public RedGui::CRedGuiControl
	{
	public:
		CMemoryMetricsUsageRenderControl( Uint32 x, Uint32 y, Uint32 width, Uint32 height );
		virtual ~CMemoryMetricsUsageRenderControl() override final;

		virtual void Draw() override final;
		virtual void OnMouseMove( const Vector2& mousePosition ) override final;
		virtual void OnMouseLostFocus( CRedGuiControl* newControl ) override final;
		virtual void OnMouseButtonClick( const Vector2& mousePosition, RedGui::EMouseButton button ) override final;
		virtual void OnMouseButtonDoubleClick( const Vector2& mousePosition, RedGui::EMouseButton button ) override final;
		virtual void OnMouseButtonReleased( const Vector2& mousePosition, RedGui::EMouseButton button ) override final;
// 		virtual void OnMouseWheel(Int32 delta) override final;

		void Update();

		void ResetMemoryPool();
		void SetMemory( Red::MemoryFramework::IAllocator* poolAllocator, MemUint address, MemSize size );

		RED_INLINE MemSize GetBytesPerBlock() const { return m_walker.GetBytesPerBlock(); }
		RED_INLINE MemSize GetLargestFreeArea() const { return m_walker.GetLargestFreeArea(); }

		RED_INLINE Uint32 GetBlockWidth() const { return Red::Math::NumericalUtils::Max( 1u, GetWidth() / m_walker.GetWidth() ); }
		RED_INLINE Uint32 GetBlockHeight() const { return Red::Math::NumericalUtils::Max( 1u, GetHeight() / m_walker.GetHeight() ); }

		const MemoryMetricsUsageControlWalker::Block* GetSelectedBlock() const;
		const MemoryMetricsUsageControlWalker::BlockMetrics* GetSelectedBlockMetrics() const;

		RED_INLINE void SetSelectedMemoryClass( Uint16 memoryClass ) { m_selectedMemoryClass = memoryClass; }
		void SetMemoryClassDeselectionCallback( std::function< void() > callback );

		static const Int32 c_blocksWide = 256;
		static const Int32 c_blocksHigh = 128;
		static const Int32 c_totalBlocks = c_blocksWide * c_blocksHigh;

	private:
		String GetFormattedSize( Red::System::Int64 size );

		Uint32 BlockIndexFromMousePosition( const Vector2& mousePosition ) const;
		Uint32 GetLastBlockInAllocation( Uint32 index ) const;

		void DrawRange( Uint32 x1, Uint32 x2, Uint32 y );
		void DrawRange( Uint32 x1, Uint32 x2, Uint32 y, const Color& colour );
		void DrawSelectedClass();
		void DrawMostSignificantClass();

		MemoryMetricsUsageControlWalker		m_walker;
		MemUint								m_highlightedMemoryAddress;

		RedGui::CRedGuiPanel*				m_tooltipPanel;
		RedGui::CRedGuiLabel*				m_tooltipMemoryClass;
		RedGui::CRedGuiLabel*				m_tooltipAddress;

		Uint32								m_selectedBlockIndex;

		// From legend

		Uint16								m_selectedMemoryClass;
		std::function< void() >				m_memoryClassDeselectedCallback;
	};

	class CMemoryMetricsUsageControl : public RedGui::CRedGuiControl
	{
	public:
		CMemoryMetricsUsageControl( Uint32 x, Uint32 y, Uint32 width, Uint32 height );
		virtual ~CMemoryMetricsUsageControl() override final;

		void SetMemory( Red::MemoryFramework::IAllocator* poolAllocator, MemUint address, MemSize size );
		void ResetMemoryPool();

		virtual void Draw() override final {}
		void Update();

	private:
		CMemoryMetricsUsageRenderControl* m_render;
		CMemoryMetricsUsageLegendControl* m_legend;
	};

}	// namespace DebugWindows

#endif	// NO_RED_GUI
