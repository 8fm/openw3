/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderCommandBuffer.h"
#include "../redThreads/redThreadsThread.h"

/// Rendering thread interface
class IRenderThread : public Red::Threads::CThread
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );
protected:
	CRenderCommandBuffer	m_commandBuffer;		//!< Command buffer

public:
	//! Get command buffer
	RED_INLINE CRenderCommandBuffer& GetCommandBuffer() { return m_commandBuffer; }

#ifdef USE_ANSEL
	virtual Bool IsPlayingVideo() const { return false; }
#endif // USE_ANSEL

public:
	IRenderThread();
};