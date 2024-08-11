/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../engine/textureCache.h"
#include "renderTextureBase.h"
#include "renderTextureStreaming.h"
#include "../gpuApiUtils/gpuApiMemory.h"
#include "../core/fileDecompression.h"

//////////////////////////////////////////////////////////////////////////////////////////////
/// Texture streaming from cooked data
//////////////////////////////////////////////////////////////////////////////////////////////

/// Texture streaming task
class CRenderTextureStreamingTaskCooked : public IRenderTextureStreamingTask
{
public:
	// Create cooked streaming task
	static CRenderTextureStreamingTaskCooked* Create( const GpuApi::TextureDesc& textureDesc, GpuApi::TextureRef existingTexture, Int8 streamingMipIndex, const class CTextureCacheQuery& cacheQuerry, const Uint8 ioTag, Bool& inoutAllowNewStreaming );

private:
	GpuApi::TextureRef							m_existingTexture;		//!< Texture that we can copy from
	CTextureCacheQuery							m_cacheQuery;			//!< Source cache query
	Red::MemoryFramework::MemoryRegionHandle	m_inplaceMemory;		//!< Texture inplace memory

	Uint8										m_numMipsToLoad;		//!< Number of mips to load from disk. The rest will be copied.

	Uint32										m_inflightSize;			//!< Size of uncompressed cooked data we're loading.
	Uint32										m_loadDataSize;			//!< Size of data loading from disk.
	Uint32										m_loadMipDataSize;		//!< Size of loaded data in memory.

	IFileDecompressionTask*						m_loadTask;				//!< Data loading task

	Uint32										m_mipOffset;			//!< Mipmap data placement offset

	Bool										m_discarded;			//!< Already discarded ?
	volatile Bool								m_finished;				//!< Are we finished ?
	Uint8										m_ioTag;				//!< Loading priority tag 

	Bool										m_loadTaskFinished;


	CRenderTextureStreamingTaskCooked( const GpuApi::TextureDesc& textureDesc, GpuApi::TextureRef existingTexture, Int8 streamingMipIndex, const Uint8 ioTag );
	virtual ~CRenderTextureStreamingTaskCooked();

	//! Discard the job (can happen before the job is completed)
	virtual void Discard() override;


	virtual void Tick( Bool& allowNewStreaming ) override;

	virtual Bool TryFinish( STextureStreamResults& outResults ) override;


private:
	//! Start the streaming. If there's data to load, try to start it. Sets outAllowNewStreaming to false if the IO cannot be started,
	//! to indicate that further streaming shouldn't be started (it will also likely fail)
	void Start( Bool& outAllowNewStreaming );

	//! Try to start loading data from the texture cache. This may fail if the decompression thread is currently overloaded.
	//! Returns false if the decompression thread is busy. If it returns true, m_loadTask will be non-null on success, null on failure.
	Bool TryStartLoadFromCache();


	void Finalize( STextureStreamResults& outResults );
};
