/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace DebugWindows
{
	class CMemoryMetricsPoolAreaControl : public RedGui::CRedGuiControl, public Red::MemoryFramework::PoolAreaWalker
	{
	public:
		CMemoryMetricsPoolAreaControl( Uint32 x, Uint32 y, Uint32 width, Uint32 height );
		virtual ~CMemoryMetricsPoolAreaControl();

		void Draw();

		void SetMemory( Red::MemoryFramework::IAllocator* poolAllocator, Red::System::MemUint address, Red::System::MemSize size );

		// Pool area walker interface
		void OnUsedArea( Red::System::MemUint address, Red::System::MemSize size, Red::System::Uint16 memoryClass );
		void OnFreeArea( Red::System::MemUint address, Red::System::MemSize size );
		void OnLockedArea( Red::System::MemUint address, Red::System::MemSize size )	{ }

		Red::System::MemUint GetMemoryAddress() const;

	private:
		String GetFormattedSize( Red::System::Int64 size );
		void DrawAreaBlockAndReset( const Vector& usedColor = Vector( 1.0f, 0.0f, 0.0f, 1.0f ) );

	private:
		RedGui::CRedGuiPanel*				m_drawPanel;
		RedGui::CRedGuiLabel*				m_label;

		Int32								m_currentOffset;
		Red::System::MemSize				m_blockPxByteBoundary;	// How many bytes before a pixel can be displayed
		Red::System::MemSize				m_blockFreeCounter;	// How many bytes freed for this pixel block
		Red::System::MemSize				m_blockUsedCounter;	// How many bytes used for this pixel block
		Red::System::MemSize				m_totalUsed;

		Red::System::MemUint				m_address;
		Red::System::MemSize				m_size;

		Red::MemoryFramework::IAllocator*	m_poolAllocator;
	};

}	// namespace DebugWindows

#endif	// NO_RED_GUI
