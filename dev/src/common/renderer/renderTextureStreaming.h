/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../engine/textureGroup.h"


#define LOG_TEXSTREAM( format, ... ) RED_LOG( TextureStreaming, format, ##__VA_ARGS__ )
#define ERR_TEXSTREAM( format, ... ) RED_LOG_ERROR( TextureStreaming, format, ##__VA_ARGS__ )
#define WARN_TEXSTREAM( format, ... ) RED_LOG_WARNING( TextureStreaming, format, ##__VA_ARGS__ );

//#ifndef RED_FINAL_BUILD
//RED_DECLARE_NAME( CharacterEmissive );
//RED_DECLARE_NAME( CharacterNormalmapGloss );
//RED_DECLARE_NAME( HeadDiffuse );
//RED_DECLARE_NAME( HeadDiffuseWithAlpha );
//RED_DECLARE_NAME( HeadNormal );
//#endif


class IBitmapTextureStreamingSource;
class CTextureStreamingUpdateTask;
struct STextureStreamingContext;
struct STextureStreamingUpdateEntry;
struct STextureStreamingRefinerEntry;
class CRenderTextureStreamRequest;
class CRenderTextureBase;
class CRenderFramePrefetch;
struct STextureStreamingRegistration;


#ifndef RED_FINAL_BUILD
struct STextureStreamingTexStat
{
	String	m_textureName;
	Uint32	m_streamingOrder;
	Uint8	m_currentMip;				// Currently loaded mip
	Int8	m_pendingMip;				// -1 for no pending.
	Uint8	m_originalRequestedMip;		// Requested mip before any possible refinement
	Uint8	m_requestedMip;				// Mip that was actually requested
	Uint8	m_baseMip;					// Lowest "acceptable" mip
	Bool	m_hasLock;

	STextureStreamingTexStat();
	STextureStreamingTexStat( const STextureStreamingTexStat & other );
	STextureStreamingTexStat( STextureStreamingTexStat && other );
	
	STextureStreamingTexStat & operator=( const STextureStreamingTexStat & other );
	STextureStreamingTexStat & operator=( STextureStreamingTexStat && other );
	void Swap( STextureStreamingTexStat & other );
};


struct STextureStreamingStats
{
	Uint32 m_initialRequestedSize;
	Uint32 m_finalRequestedSize;
	TDynArray< STextureStreamingTexStat > m_textures;

	STextureStreamingStats();
	STextureStreamingStats( const STextureStreamingStats & other );
	STextureStreamingStats( STextureStreamingStats && other );

	STextureStreamingStats & operator=( const STextureStreamingStats &  other );
	STextureStreamingStats & operator=( STextureStreamingStats &&  other );
	void Swap( STextureStreamingStats & other );
};

struct STextureSteamingStatsContainer
{
	STextureStreamingStats stats;
	mutable Red::Threads::CSpinLock lock;
};

#endif


class IRenderTextureStreamingFinalizer
{
public:
	virtual ~IRenderTextureStreamingFinalizer() {}

	// Perform some final work on the render thread. This should be kept short.
	virtual void Finalize() = 0;
};


// Results from the streaming update task. We don't want to actually update the textureref from there, since
// that would complicate things with multi-threaded access. So it just makes a list of results that will be
// safely applied from the render thread.
struct STextureStreamResults : Red::NonCopyable
{
	GpuApi::TextureRef									m_hiresTexture;
	Int32												m_streamedMip;
	Red::TUniquePtr<IRenderTextureStreamingFinalizer>	m_finalizer;

	STextureStreamResults()
		: m_streamedMip( -1 )
	{}

	STextureStreamResults( STextureStreamResults&& o )
		: m_hiresTexture( std::move( o.m_hiresTexture ) )
		, m_streamedMip( std::move( o.m_streamedMip ) )
		, m_finalizer( std::move( o.m_finalizer ) )
	{}

	STextureStreamResults& operator =( STextureStreamResults&& o )
	{
		if ( this != &o )
		{
			m_hiresTexture = std::move( o.m_hiresTexture );
			m_streamedMip = std::move( o.m_streamedMip );
			m_finalizer = std::move( o.m_finalizer );
		}

		return *this;
	}
};



// Abstract texture streaming task
class IRenderTextureStreamingTask
{
public:
	IRenderTextureStreamingTask( const GpuApi::TextureDesc& textureDesc, Int8 streamingMipIndex );

	enum EStatus
	{
		eStatus_Ready,
		eStatus_Failed,
		eStatus_Pending,
	};

	//! Discard the job (can happen before the job is completed)
	virtual void Discard() = 0;

	//! Update the streaming job. If allowNewStreaming is false, this will not start new loading tasks, just check existing ones. This may
	//! change allowNewStreaming to false if it is unable to start a new loading task (if IO is too busy).
	virtual void Tick( Bool& allowNewStreaming ) = 0;

	//! Check if the streaming can finish. Return true if it's finished, in which case outResults should be filled in.
	virtual Bool TryFinish( STextureStreamResults& outResults ) = 0;

	//! Create the streaming task. Created texture will have same format, type, slice count as existingTexture.
	//! If copyFromExisting is true, the created streaming task may copy shared mip data from the existingTexture. If it's false,
	//! the task will load all mips new.
	static IRenderTextureStreamingTask* Create( const GpuApi::TextureRef& existingTexture, Bool copyFromExisting, IBitmapTextureStreamingSource* streamingSource, Uint32 firstMipmap, const Int8 priority, Bool& inoutAllowNewStreaming );

protected:
	virtual ~IRenderTextureStreamingTask();

	GpuApi::TextureDesc		m_textureDesc;			//!< Texture description data
	Int8					m_streamingMipIndex;	//!< Index of the largest mip we're streaming

	void SetTextureInfo( const GpuApi::TextureRef& texture );
};

class CRenderTextureStreamingManager
{
public:
	CRenderTextureStreamingManager();
	~CRenderTextureStreamingManager();

	//! Cancel all current streaming, and prevent any further from starting. Should only be used during shutdown. Would be simply
	//! called Shutdown, except that the streaming manager remains valid afterwards, it just prevents any new update tasks.
	void StopFurtherStreaming();


	//! Register texture for texture streaming. If successful, texture will be addref'd and non-zero key returned.
	//! Reference added here will be eventually released if it becomes the last reference held.
	Uint16 RegisterTexture( CRenderTextureBase* texture );
	void UpdateTexture( Uint16 key );


	//! Try to finish any existing update task. If 'force' is true, this will block until the task is done (possibly running it
	//! synchronously if it hasn't even been started by the task scheduler yet). If the task is finished, the results will be applied.
	//! Returns true if tasks is finished, false if it's still running (if 'force' is true, then it will always return true).
	//! 'applyResults' may only be true if called between GpuApi::BeginRender/EndRender calls.
	Bool TryFinishUpdateTask( Bool force, Bool applyResults );

	//! Is an update task running?
	Bool HasActiveUpdate() const;

	//! Ensure that if there's an update task, it has gotten past the "unsafe" part of gathering last bind distances. If the task is
	//! currently running, we'll wait until it has progressed sufficiently. If the task is still scheduled (not started yet), then
	//! we'll run it to completion.
	void EnsureUpdateTaskIsSafe();

	void CancelTextureStreaming( Bool flushOnlyUnused = false );
	void UpdateTextureStreaming( Bool allowTextureExpire = true );

	//! Update texture streaming, without running periodic "unload not visible" pass, and without letting textures expire automatically.
	void TickDuringSuppressedRendering();

	void ForceUnloadUnused();

	void EnableCinematicMode( const Bool cinematicMode );

	void SetPrefetchMipDrop( Bool allowMipDrop ) { m_allowPrefetchMipDrop = allowMipDrop; }
	Bool IsPrefetchMipDropAllowed() const { return m_allowPrefetchMipDrop; }

	Bool IsCurrentlyStreaming() const;

	void OnNewTextureCacheAttached();

	//! Register a texture request with the streaming manager, so it can get notifications about textures.
	void RegisterTextureRequest( CRenderTextureStreamRequest* request );
	//! Unregister a texture request. This should only be used when canceling a request before it is complete. Requests will
	//! automatically be unregistered when it is no longer waiting on textures (when OnTextureStreamed or OnTextureCanceled return true).
	void UnregisterTextureRequest( CRenderTextureStreamRequest* request );

	void OnTextureStreamed( CRenderTextureBase* texture );
	void OnTextureCanceled( CRenderTextureBase* texture );
	void OnTextureUnloaded( CRenderTextureBase* texture );

	RED_INLINE Float GetMaxStreamingDistance( ETextureCategory textureCategory ) const { return m_maxStreamingDistancePerCategory[ textureCategory ]; }

#ifndef NO_DEBUG_PAGES
	RED_INLINE Float GetStreamingTaskTime() const { return m_lastStreamingTaskTime; }
#endif


#ifndef RED_FINAL_BUILD
	// Return copy of stats. Not doing reference, since it may be updated on another thread.
	void GetLastUpdateStats( STextureStreamingStats & result ) const;
#endif


	RED_INLINE Uint64 GetTotalDataLoaded() const { return m_totalDataLoaded.GetValue(); }
	RED_INLINE void ResetTotalDataLoaded() { m_totalDataLoaded.SetValue( 0 ); }
	RED_INLINE void AddTotalDataLoaded( Uint64 amount ) { m_totalDataLoaded.ExchangeAdd( amount ); }

	void AddFinishedFramePrefetch( CRenderFramePrefetch* prefetch );
	void RemoveFinishedFramePrefetch( CRenderFramePrefetch* prefetch );
	Bool HasNewFinishedFramePrefetch() const;

	void SetFramesToResumeTextureStreaming( Uint32 frames ) { m_framesToResumeTextureStreaming = frames; }
	Bool IsTemporarilyPaused() const { return m_framesToResumeTextureStreaming > 0; }


	void OnTextureStreamingTaskStarted() { m_numTextureStreamingTasksRunning.Increment(); }
	void OnTextureStreamingTaskDiscarded() { Int32 v = m_numTextureStreamingTasksRunning.Decrement(); RED_FATAL_ASSERT( v >= 0, "Bad texture streaming tasks counting!" ); }
	Int32 GetNumTextureStreamingTasksRunning() const { return m_numTextureStreamingTasksRunning.GetValue(); }


	RED_INLINE Bool IsStopped() const { return m_forceStopped; }

protected:
	typedef Red::Threads::CLightMutex TMutex;
	typedef Red::Threads::CScopedLock<TMutex> TScopedLock;

	/// Texture unload method
	enum ERenderTextureUnloadMethod
	{
		RTUM_All,			//!< Unload all streamable textures
		RTUM_NotVisible,	//!< Unload textures that are not visible
	};


	volatile Bool								m_forceStopped;							// Has further streaming been stopped?

	Bool										m_cinematicMode;
	Bool										m_requestedCinematicMode;

	Float										m_maxStreamingDistancePerCategory[ eTextureCategory_MAX ];

	mutable TMutex								m_streamRequestsMutex;
	TDynArray< CRenderTextureStreamRequest* >	m_streamRequests;

	CTextureStreamingUpdateTask*				m_updateTask;
	Red::TUniquePtr<STextureStreamingContext>	m_streamingContext;

	Red::Threads::CAtomic< Bool >				m_forceStillStreaming;					// If true, IsCurrentlyStreaming give true even if nothing is currently happening. Reset when new update starts

#ifndef NO_DEBUG_PAGES
	Float										m_lastStreamingTaskTime;
#endif

	Red::Threads::CAtomic< Uint64 >				m_totalDataLoaded;


#ifndef RED_FINAL_BUILD
	STextureSteamingStatsContainer				m_statsContainer;
#endif


	THashSet< CRenderFramePrefetch* >			m_newFramePrefetches;					// Have just been added. Need to addref these to make sure they apply at least once.
	THashSet< CRenderFramePrefetch* >			m_finishedFramePrefetches;				// Prefetches that we want to continue to apply until they're destroyed.
	Uint32										m_framesToResumeTextureStreaming;
	Red::Threads::CAtomic< Int32 >				m_numTextureStreamingTasksRunning;


	Bool										m_allowPrefetchMipDrop;					// Whether mips on newly-streamed textures can be dropped if the texture has any actively waiting prefetches. When allowed, can reduce wait time.


	//! Run an existing update task locally on the current thread, if it hasn't been started yet. Return true if the task is finished
	//! at the end (either we finished it, or it was already finished), false if it's still running (was probably already on a task
	//! thread).
	Bool TryRunUpdateTaskLocally();

	void UnloadTextures( ERenderTextureUnloadMethod unloadMethod );

	void UpdateMaxStreamingDistances();
};
