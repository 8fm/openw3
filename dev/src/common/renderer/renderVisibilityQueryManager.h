/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "..\..\common\engine\renderObject.h"
#include "..\..\common\engine\renderVisibilityQuery.h"
#include "..\..\common\core\staticarray.h"
#include "..\..\common\core\idAllocator.h"

/// Rendering side manager for visibility queries
/// NOTE: this has nothing to do with occlusion query or Umbra
class CRenderVisibilityQueryManager
{
	DECLARE_RENDER_OBJECT_MEMORYCLASS( MC_RenderObjects )

public:
	CRenderVisibilityQueryManager();
	~CRenderVisibilityQueryManager();

	// advance internal markers - make sure to always call in pair
	void FrameFinished();

	enum EFlags : Uint8
	{
		VisibleScene = 1,					// Visible in main scene
		VisibleMainShadows = 2,				// Visible in sun shadows (cascades)
		VisibleAdditionalShadows = 4,		// Visible in other dynamic shadows
	};

	// mark object as visited
	void MarkQuery( const TRenderVisibilityQueryID id, const EFlags flags );

	// test if query is visible for this frame, 1-visible in main view, 2-visible in shadows
	ERenderVisibilityResult TestQuery( const TRenderVisibilityQueryID id ) const;

	// test if query is visible for this frame, 1-visible in main view, 2-visible in shadows
	Bool TestQuery( Uint32 elementsCount, Int16* indexes, void* inputPos, void* outputPos, size_t stride ) const;

	// allocate query ID, returns 0 if not possible to allocate
	TRenderVisibilityQueryID AllocQuerryId();

	// release allocated ID
	void ReleaseQueryId( TRenderVisibilityQueryID id );

private:
#if defined(RED_PLATFORM_CONSOLE)
	static const Uint32 MAX_QUERIES = 512 * 1024; // on consoles we can get away with less queries, mostly because of reduced entity count
#else 
	static const Uint32 MAX_QUERIES = 1024 * 1024; // in editor or on Windows we need a little bit more
#endif

	// thread safety
	typedef Red::Threads::CSpinLock			TLock;
	TLock			m_lock;

	// allocator for query IDs
	typedef IDAllocator< MAX_QUERIES >					TAllocator;
	TAllocator		m_allocator;

	// allocated and used queries
	Uint8*			m_previousFrame; // read buffer
	Uint8*			m_currentFrame;  // write buffer
};