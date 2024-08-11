/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#if !defined( NO_RED_GUI ) && defined( ENABLE_EXTENDED_MEMORY_METRICS )

#include "redGuiGraphBase.h"

namespace Memory { class Adapter; }

class CDebugWindowMemoryHistogram : public RedGui::CRedGuiGraphBase
{
public:
	CDebugWindowMemoryHistogram( Uint32 left, Uint32 top, Uint32 width, Uint32 height );
	virtual ~CDebugWindowMemoryHistogram();

	void SetParameters( Memory::Adapter* manager, Red::MemoryFramework::PoolLabel pool, Red::MemoryFramework::MemoryClass memClass );

private:
	virtual void DrawGraph( const Vector2& origin, const Vector2& dimensions );
	void UpdateKey();
	Color CalculateBucketColour( Uint32 bucket );
	String GetBucketKeyString( Uint32 bucket );

	Red::MemoryFramework::PoolLabel m_poolLabel;
	Red::MemoryFramework::MemoryClass m_memoryClass;
	Memory::Adapter* m_memoryManager;
};

#endif