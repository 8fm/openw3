/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../engine/renderFramePrefetch.h"

class CRenderFrame;
class CRenderSceneEx;

class CRenderMaterial;
class CRenderMaterialParameters;
class CRenderTextureBase;

class CPrefetchSceneCollectTask;

class CRenderFramePrefetch;
class CRenderTextureStreamRequest;


class IRenderPrefetchable
{
public:
	virtual ~IRenderPrefetchable() {}

	// Called from the Collection task (i.e. probably not from the render thread). Allows the prefetchable object
	// to add any required resources to the prefetch.
	//
	// Should avoid modifying any state, at least, it shouldn't negatively impact rendering for the current frame
	// (since Prefetch is generally done some time before that particular camera will be used).
	//
	// Be aware that rendering is probably in-progress while this is called, so anything that might be changed
	// during rendering, and accessed here, should be done in a thread-safe manner.
	virtual void Prefetch( CRenderFramePrefetch* prefetch ) const = 0;
};


class CRenderFramePrefetch : public IRenderFramePrefetch
{
private:
	CRenderFrame*							m_frame;						// Render frame we're prefetching.
	CRenderSceneEx*							m_scene;						// Scene we should collect from.

	CPrefetchSceneCollectTask*				m_collectTask;					// Asynchronous task spawned by BeginSceneCollect.

	THashMap< CRenderTextureBase*, Float >	m_textureDistances;				// During the collect task, we can build a list of bind distances for textures.

	CRenderTextureStreamRequest*			m_textureRequest;				// If we got any texture bind distances, this will be created
																			// to allow us to wait for those textures to stream.

	CTimeCounter							m_timeout;						// Force a finished state after some time, to prevent infinite waits.


	Red::Threads::CAtomic< Bool >			m_finishedCollection;			// Is the main collection done?
	Red::Threads::CAtomic< Bool >			m_pendingTextureRequestCreate;	// Do we still need to create m_textureRequest?
	Bool									m_didEnqueue;					// Has the prefetch been enqueued?

	Bool									m_useOcclusionQuery;


public:
	CRenderFramePrefetch( CRenderFrame* frame, CRenderSceneEx* scene, Bool useOcclusionQuery = true );
	virtual ~CRenderFramePrefetch();

	// Has the prefetch finished?
	virtual Bool IsFinished() const override;


	RED_INLINE Bool UseOcclusionQuery() const { return m_useOcclusionQuery; }

	// Has the prefetch already been enqueued? We don't want to process it multiple times, so this allows us to check for that.
	RED_INLINE Bool HasBeenEnqueued() const { return m_didEnqueue; }
	// Set that this prefetch has been enqueued.
	RED_INLINE void SetEnqueued() { m_didEnqueue = true; }


	//////////////////////////////////////////////////////////////////////////
	// These are used internally by the renderer to allow prefetches to work.
	// They shouldn't normally be needed for anything else.

	// Start async scene query. This should generally be done during scene rendering, to ensure that nothing will change
	// or be destroyed while it is processing.
	void BeginSceneCollect();

	// Finish the async query. Will wait if it's in progress, or execute it locally if it wasn't even started yet. The prefetch
	// itself may not be finished yet, but it will run in a background task and eventually end. No further action needed, this
	// may be released after calling FinishSceneCollect.
	void FinishSceneCollect();

	// Do the scene collection in the current thread. Either this or the above Begin/Finish functions should be used, but not both.
	void DoSceneCollectSync();

	// After prefetch has been run, this should be called to apply results to any textures. Ideally, it should not be while
	// a texture update task is in progress.
	void ApplyTextureResults();


	//////////////////////////////////////////////////////////////////////////
	// These can be called by an IRenderPrefetchable to get information about
	// the frame we're prefetching, such as where the camera will be (e.g. for
	// appropriate LOD calculations)

	RED_INLINE CRenderFrame* GetFrame() const { return m_frame; }
	RED_INLINE CRenderSceneEx* GetScene() const { return m_scene; }

	// Return occlusion camera position. This is the one used to figure out cached distance during rendering, so it
	// should be consistent.
	const Vector& GetCameraPosition() const;
	Float GetCameraFovMultiplierUnclamped() const;


	//////////////////////////////////////////////////////////////////////////
	// The following should be called from an IRenderPrefetchable's Prefetch
	// method, to indicate that certain resources will be needed and should
	// be made ready.
	//
	// These are safe to call with null parameters, to allow simpler usage.
	// Instead, the functions will gracefully handle nulls.

	// Add a material. This will go through the material's parameters and add any textures at the given distance.
	void AddMaterialBind( CRenderMaterial* material, CRenderMaterialParameters* parameters, Float distanceSq );

	// Add a texture. Indicates that this texture will be used at the given distance, so it should be streamed as
	// appropriate.
	void AddTextureBind( CRenderTextureBase* texture, Float distanceSq );


private:
	void OnFinishedCollectTask();
};
