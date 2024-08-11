
#pragma once

#include "animCacheEntry.h"
#include "animationBuffer.h"
#include "skeletalAnimationSet.h"

#include "../core/loadingJob.h"

//////////////////////////////////////////////////////////////////////////
/// Animation streaming job
class CAnimationStreamingJob : public ILoadJob
{
private:
	Uint32			m_animationId;
#ifdef USE_HAVOK_ANIMATION
	AnimBuffer*		m_animationBuffer;
#else
	IAnimationBuffer* m_animationBuffer;
#endif
	Uint32			m_loadedData;

public:
	CAnimationStreamingJob( Uint32 animationId );
	virtual ~CAnimationStreamingJob();

	//! Get loaded data size
	Uint32 GetLoadedDataSize() const;

	//! Get loaded buffer
#ifdef USE_HAVOK_ANIMATION
	AnimBuffer* GetLoadedBuffer();
#else
	IAnimationBuffer* GetLoadedBuffer();
#endif

public:
	//! Process the job, is called from job thread
	EJobResult Process();

	RED_INLINE Int32 GetSubPriority() const { return m_animationId; }

	virtual const Char* GetDebugName() const override { return TXT("IO Animation"); }
};

//////////////////////////////////////////////////////////////////////////
/// Animset streaming job
class CAnimsetStreamingJob : public ILoadJob
{
private:
	const Uint32					m_firstAnimId;
	const TDynArray< Uint32 >		m_animationsBuffersSizes;
	const TDynArray< Bool >		m_animationsToLoad;
#ifdef USE_HAVOK_ANIMATION
	TDynArray< AnimBuffer* > m_animationsBuffers;
#else
	TDynArray< IAnimationBuffer* > m_animationsBuffers;
#endif

public:
	CAnimsetStreamingJob( Uint32 firstAnimId, TDynArray< Uint32 >& animationsBuffersSizes, TDynArray< Bool >& animationsToLoad );
	virtual ~CAnimsetStreamingJob();

public:
	//! Process the job, is called from job thread
	EJobResult Process();

	//! Get loaded buffers
#ifdef USE_HAVOK_ANIMATION
	void GetLoadedBuffers( TDynArray< AnimBuffer* >& buffers );
#else
	void GetLoadedBuffers( TDynArray< IAnimationBuffer* >& buffers );
#endif

	RED_INLINE Int32 GetSubPriority() const { return (Int32)m_firstAnimId; }

	virtual const Char* GetDebugName() const override { return TXT("IO Animset"); }

private:
	void ClearBuffers();
};

//////////////////////////////////////////////////////////////////////////
/// Animset async loader
class AnimsetLoader
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Animation );

	THandle< CSkeletalAnimationSet >	m_animset;
	Uint32								m_dataSizeToLoad;
	Uint32								m_dataSizeLoaded;
	Uint32								m_firstAnimId;
	TDynArray< Uint32 >					m_animationsBuffersSizes;
	TDynArray< Bool >					m_animationsToLoad;
	CAnimsetStreamingJob*				m_streamingJob;

public:
	explicit AnimsetLoader( const CSkeletalAnimationSet* set, Bool loadAll = false );
	~AnimsetLoader();

	Bool IsEmpty() const;

	Bool StartStreaming();

	Bool UpdateStreaming();

	void CancelStreaming();

	Bool HasAnimationToLoad( const CSkeletalAnimation* anim ) const;

	Uint32 GetDataSizeToLoad() const;
	Uint32 GetDataSizeLoaded() const;

	Bool CompareAnimset( const CSkeletalAnimationSet* set ) const;

private:
#ifdef USE_HAVOK_ANIMATION
	void WriteBuffersToAnimations( TDynArray< AnimBuffer* >& loadedBuffers );
#else
	void WriteBuffersToAnimations( TDynArray< IAnimationBuffer* >& loadedBuffers );
#endif
};
