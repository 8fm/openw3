/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifdef ENABLE_EXTENDED_MEMORY_METRICS

#include "redGuiGraphBase.h"

namespace Memory { class Adapter; }

class CDebugWindowMemoryHistoryViewer : public RedGui::CRedGuiGraphBase
{
public:
	CDebugWindowMemoryHistoryViewer( Uint32 left, Uint32 top, Uint32 width, Uint32 height );
	virtual ~CDebugWindowMemoryHistoryViewer();
	void SetParameters( Memory::Adapter* manager, Red::MemoryFramework::PoolLabel poolLabel );
	enum DisplayMode
	{
		Display_Groups,
		Display_Classes
	};
	void SetDisplayMode( DisplayMode mode )	{ m_displayMode = mode; ResetAll(); }

private:
	virtual void DrawGraph( const Vector2& origin, const Vector2& dimensions );
	void UpdateKey();
	void ResetAll();
	void RefreshData();

	static const Uint32 k_historyBufferSize = 256;
	struct MemoryClassRingBuffer
	{
		MemoryClassRingBuffer()
			: m_hasAllocations( false )
			, m_hasDrawn( false )
		{
			Red::System::MemorySet( m_allocatedBytes, 0, sizeof( m_allocatedBytes ) );
		}
		Uint64 m_allocatedBytes[ k_historyBufferSize ];
		Bool m_hasAllocations;	// Cache whether any value in history > 0 so we know when to show the key
		Bool m_hasDrawn;		// Track which are renderered for the memory class key
	};
	
	void PushAllocations( MemoryClassRingBuffer& target, Uint64 allocatedBytes );
	Uint64 GetPreviousValue( const MemoryClassRingBuffer& target, Uint32 framesBack );

	Red::MemoryFramework::PoolLabel m_poolLabel;
	Memory::Adapter* m_memoryManager;	
	MemoryClassRingBuffer m_history[ Red::MemoryFramework::k_MaximumMemoryClasses ];
	Uint32 m_headIndex;	
	Uint16 m_count;
	DisplayMode m_displayMode;
};

#endif
#endif
