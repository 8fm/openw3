/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "renderTextureBase.h"
#include "renderTextureStreaming.h"
#include "../gpuApiUtils/gpuApiMemory.h"
#include "../core/loadingJob.h"

//////////////////////////////////////////////////////////////////////////////////////////////
/// Texture streaming from non cooked data
///
/// Currently also used for consoles, but if/when platform-specific changes are needed, can
/// just create separate implementation and conditionally compile them.
//////////////////////////////////////////////////////////////////////////////////////////////

/// Texture streaming task
class CRenderTextureStreamingTaskNotCooked : public IRenderTextureStreamingTask, public ILoadJob
{
public:
	// Create the streaming task
	static CRenderTextureStreamingTaskNotCooked* Create( const GpuApi::TextureDesc& textureDesc, Int8 streamingMipIndex, IBitmapTextureStreamingSource* streamingSource );

private:
	IBitmapTextureStreamingSource*			m_streamingSource;		//!< Streaming source
	Uint32									m_textureCount;			//!< Cached number of textures
	Uint32									m_mipCount;				//!< Cached number of mip levels
	TDynArray< void* >						m_streamedData;
	Uint32									m_memoryUsedInFlight;	//!< Keep track of memory used in-flight
	Bool									m_discarded;			//!< Are we already discarded
	GpuApi::TextureRef						m_texture;				//!< Generated texture

	CRenderTextureStreamingTaskNotCooked( const GpuApi::TextureDesc& textureDesc, Int8 streamingMipIndex, IBitmapTextureStreamingSource* streamingSource );
	virtual ~CRenderTextureStreamingTaskNotCooked();

	//! Discard the job (can happen before the job is completed)
	virtual void Discard() override;

	virtual void Tick( Bool& allowNewStreaming ) override;

	virtual Bool TryFinish( STextureStreamResults& outResults ) override;

	//! Internal job processing
	virtual EJobResult Process() override;
	virtual const Char* GetDebugName() const override { return TXT("TextureNotCooked"); }

	Uint32 GetActualMipsToLoad() const;
	Uint32 CalculateTotalMemoryForLoad() const;
	Bool StreamFromStreamingSource();

	friend class LoadingJob;
};

