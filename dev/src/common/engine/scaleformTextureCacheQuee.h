/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

//////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////

#include "scaleformTextureCacheImage.h"

class CScaleformTextureCacheQueue;

enum EScaleformCacheJobStatus
{
	ESJS_Uninitialized,
	ESJS_Processing,
	ESJS_Ready,
	ESJS_Finished,
	ESJS_Discarded,
	ESJS_Error
};

class CScaleformTextureCacheJob : public IRenderObject
{
	CScaleformTextureCacheQueue*	m_parent;

	CScaleformTextureCacheImage*	m_image;

	Red::MemoryFramework::MemoryRegionHandle	m_data;

	CTextureCacheQuery				m_cacheQuery;
	IFileDecompressionTask*			m_loadTask;

	GpuApi::TextureDesc				m_desc;

	EScaleformCacheJobStatus		m_status;

public:

	CScaleformTextureCacheJob( CScaleformTextureCacheQueue* parent, CScaleformTextureCacheImage* image );

	~CScaleformTextureCacheJob();

public:

	// Gets status of the current pending job
	RED_INLINE EScaleformCacheJobStatus		GetStatus() const { return m_status; }
	
	// Get corresponding image pointer
	RED_INLINE CScaleformTextureCacheImage*	GetImage() const { return m_image; }

	EScaleformCacheJobStatus	Start();

	void						Cancel();

	Bool						Check();

	GpuApi::TextureRef			CreateTexture();

	Uint32						GetDataSize() const;

	Uint32						GetBaseAlignment() const;

	RED_INLINE const CTextureCacheQuery&	GetCacheQuery() const { return m_cacheQuery; }

	RED_INLINE CScaleformTextureCacheQueue*	GetQueue() const { return m_parent; }

private:

	void						ReleaseInPlaceMemory();

};

//////////////////////////////////////////////////////////////////////////

class CScaleformTextureCacheQueue : public IRenderObject
{

	GpuApi::TextureRef		m_undefPlaceholder;
	GpuApi::TextureRef		m_pendingPlaceholder;

	TDynArray< CScaleformTextureCacheJob* > m_quee;

#ifndef RED_FINAL_BUILD
	Uint32					m_currentMemory;
	Uint32					m_peakMemory;
#endif

	Int32					m_frameToResumeRendering;

public:

	CScaleformTextureCacheQueue();

	~CScaleformTextureCacheQueue();

	void	FinishJob( CScaleformTextureCacheJob* job );

	// Cheking if placeholder texture was prepared
	RED_INLINE Bool IsPlaceholderCreated() const { return !m_undefPlaceholder.isNull() ; }

	//
	RED_INLINE const GpuApi::TextureRef& GetUndefinedTexture() const { return m_undefPlaceholder; }

	//  
	RED_INLINE const GpuApi::TextureRef& GetPendingTexture() const { return m_pendingPlaceholder; }

	// Checks if texture is in undefined mode
	RED_INLINE Bool	IsTextureUndefined( const GpuApi::TextureRef& texture ) const { return ( texture == m_undefPlaceholder ); }

	// Checks if texture is pending
	RED_INLINE Bool	IsTexturePending( const GpuApi::TextureRef& texture ) const { return ( texture == m_pendingPlaceholder ); }

	// checks if texture is somehow created by this manager
	RED_INLINE Bool IsTextureOwned( const GpuApi::TextureRef& texture ) const { return IsTextureUndefined( texture ) || IsTexturePending( texture ); }

	// Get the number of current streaming jobs
	RED_INLINE Uint32 GetNumPendingJobs() const { return m_quee.Size(); }

public:

	// Finds or creates job for given image
	CScaleformTextureCacheJob* GetJobForTexture( CScaleformTextureCacheImage* image );

	// Find job for given image
	CScaleformTextureCacheJob* FindJob( CScaleformTextureCacheImage* image );

	// Create empty placeholder texture for pending textures refs
	Bool	CreatePlaceholderTexture();

	void	SuspendUntilFrame( Int32 frame ) { m_frameToResumeRendering = frame; }
	
	Int32	GetResumeFrame() const { return m_frameToResumeRendering; }

	Bool	IsSuspended() const { return GpuApi::FrameIndex() <= m_frameToResumeRendering; };

#ifndef RED_FINAL_BUILD

	void LogCurrentMemory();

	RED_INLINE void	IncreaseMemoryCount( Uint32 memSize )
	{
		m_currentMemory += memSize;
		m_peakMemory = ::Max( m_peakMemory , m_currentMemory );
	}

	RED_INLINE void	DecreaseMemoryCount( Uint32 memSize )
	{
		m_currentMemory -= memSize;
	}

	RED_INLINE Uint32 GetCurrentMemoryUsage() const { return m_currentMemory; }

	RED_INLINE Uint32 GetPeakMemoryUsage() const { return m_peakMemory; }

#endif

};


//////////////////////////////////////////////////////////////////////////
#endif // USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////
