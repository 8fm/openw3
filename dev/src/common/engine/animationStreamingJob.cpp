
#include "build.h"
#include "animationStreamingJob.h"
#include "../core/loadingJobManager.h"
#include "../core/fileLatentLoadingToken.h"
#include "animationCache2.h"
#include "behaviorIncludes.h"

namespace
{
	void PlaceForBreakpoint()
	{
		Int32 i = 0;
		i++;
	}
}

#define PROG_ASSERT( expr ) \
	ASSERT( expr );\
	if ( !(expr) )\
{\
	LOG_ENGINE( TXT(#expr) );\
	PlaceForBreakpoint();\
}

CAnimationStreamingJob::CAnimationStreamingJob( Uint32 animationId )
	: ILoadJob( JP_Animation )
	, m_animationId( animationId )
	, m_animationBuffer( NULL )
	, m_loadedData( 0 )
{

}

CAnimationStreamingJob::~CAnimationStreamingJob()
{
	delete m_animationBuffer;
	m_animationBuffer = NULL;
}

EJobResult CAnimationStreamingJob::Process()
{
	if ( m_animationBuffer )
	{
		PROG_ASSERT( !m_animationBuffer );
		return JR_Finished;
	}

	IFileLatentLoadingToken* loadingToken = SAnimationsCache::GetInstance().CreateLoadingToken( m_animationId );
	if ( !loadingToken )
	{
		WARN_ENGINE( TXT("AnimStremaing: no source loading token. Failed.") );
		return JR_Failed;
	}

	// Resume file reading
	IFile* sourceFile = loadingToken->Resume(0);
	if ( !sourceFile )
	{
		delete loadingToken;
		WARN_ENGINE( TXT("AnimStremaing: unable to resume loading from '%ls'."), loadingToken->GetDebugInfo().AsChar() );
		return JR_Failed;
	}

	// Delete loading token, it's not needed any more
	delete loadingToken;

#ifdef USE_HAVOK_ANIMATION
	// Create buffer
	m_animationBuffer = new AnimBuffer();

	// Serialize animation buffer
	const Uint32 start = static_cast< Uint32 >( sourceFile->GetOffset() );
	*sourceFile << *m_animationBuffer;
	const Uint32 end = static_cast< Uint32 >( sourceFile->GetOffset() );

	// Set loaded data
	m_loadedData = end - start;
#else
	// Deserialize animation buffer

#endif

	// Close streaming file
	delete sourceFile;

	// Finished
	return JR_Finished;
}

Uint32 CAnimationStreamingJob::GetLoadedDataSize() const
{
	return m_loadedData;
}


IAnimationBuffer* CAnimationStreamingJob::GetLoadedBuffer()
{
	PROG_ASSERT( m_animationBuffer );

	IAnimationBuffer* ret = m_animationBuffer;
	m_animationBuffer = NULL;

	return ret;
}

//////////////////////////////////////////////////////////////////////////

namespace
{
	static Bool CheckAnimationStreamingType( const CSkeletalAnimation* anim )
	{
		ESkeletalAnimationStreamingType type = anim->GetStreamingType();
		return type == SAST_Prestreamed; // || type == SAST_Persistent;
	}
}

AnimsetLoader::AnimsetLoader( const CSkeletalAnimationSet* set, Bool loadAll )
	: m_animset( set )
	, m_streamingJob( NULL )
	, m_firstAnimId( 0xFFFFFFFF )
	, m_dataSizeToLoad( 0 )
	, m_dataSizeLoaded( 0 )
{
	const TDynArray< CSkeletalAnimationSetEntry* >& animsEntry = set->GetAnimations();

	Uint32 lastId = 0xFFFFFFFF;

	const Uint32 size = animsEntry.Size();

	m_animationsBuffersSizes.Reserve( size );
	m_animationsToLoad.Reserve( size );

	for ( Uint32 i=0; i<size; ++i )
	{
		const CSkeletalAnimationSetEntry* ae = animsEntry[ i ];
		const CSkeletalAnimation* anim = ae ? ae->GetAnimation() : NULL;
		if ( anim && anim->HasValidId() )
		{
			Bool canBeLoad = !anim->IsLoaded() && !anim->HasStreamingPending();

			if ( !loadAll )
			{
				canBeLoad &= CheckAnimationStreamingType( anim );
			}

			if ( canBeLoad && m_firstAnimId == 0xFFFFFFFF )
			{
				m_firstAnimId = anim->GetId();
			}

			if ( m_firstAnimId != 0xFFFFFFFF )
			{
				PROG_ASSERT( anim->GetSizeOfAnimBuffer() > 0 );

				m_animationsBuffersSizes.PushBack( anim->GetSizeOfAnimBuffer() );
				m_animationsToLoad.PushBack( canBeLoad );

				if ( canBeLoad )
				{
					m_dataSizeToLoad += anim->GetSizeOfAnimBuffer();
				}
			}

			if ( lastId != 0xFFFFFFFF && lastId >= anim->GetId() )
			{
				HALT( "Error in AnimsetLoader" );
			}

			lastId = anim->GetId();
		}
	}
}

AnimsetLoader::~AnimsetLoader()
{
	if ( m_streamingJob )
	{
		CancelStreaming();
	}
}

Bool AnimsetLoader::CompareAnimset( const CSkeletalAnimationSet* set ) const
{
	return m_animset.Get() == set;
}

Bool AnimsetLoader::IsEmpty() const
{
	return m_animationsBuffersSizes.Empty();
}

Bool AnimsetLoader::StartStreaming()
{
	if ( m_streamingJob || m_animationsBuffersSizes.Empty() )
	{
		PROG_ASSERT( m_streamingJob );
		return false;
	}

	// Create new job
	m_streamingJob = new CAnimsetStreamingJob( m_firstAnimId, m_animationsBuffersSizes, m_animationsToLoad );

	// Issue the streaming task for processing
	SJobManager::GetInstance().Issue( m_streamingJob );

	// OK if streaming task was created
	return m_streamingJob != NULL;
}

Bool AnimsetLoader::UpdateStreaming()
{
	PROG_ASSERT( m_streamingJob );

	// Update streaming job
	if ( m_streamingJob && m_streamingJob->HasEnded() )
	{
		// Extract data from finished task
		if ( m_streamingJob->HasFinishedWithoutErrors() )
		{
#ifdef USE_HAVOK_ANIMATION
			TDynArray< AnimBuffer* > loadedBuffers;
#else
			TDynArray< IAnimationBuffer* > loadedBuffers;
#endif

			// Get loaded buffers
			m_streamingJob->GetLoadedBuffers( loadedBuffers );
			PROG_ASSERT( loadedBuffers.Size() > 0 );

			// Write buffers
			WriteBuffersToAnimations( loadedBuffers );

			PROG_ASSERT( loadedBuffers.Size() == 0 );
		}

		// Release streaming task
		m_streamingJob->Release();
		m_streamingJob = NULL;

		// Finished !
		return true;
	}

	// Still pending or not going
	return false;
}

void AnimsetLoader::CancelStreaming()
{
	if ( m_streamingJob )
	{
		m_streamingJob->Cancel();
		m_streamingJob->Release();
		m_streamingJob = NULL;
	}
	else
	{
		PROG_ASSERT( m_streamingJob );
	}
}

#ifdef USE_HAVOK_ANIMATION
void AnimsetLoader::WriteBuffersToAnimations( TDynArray< AnimBuffer* >& loadedBuffers )
#else
void AnimsetLoader::WriteBuffersToAnimations( TDynArray< IAnimationBuffer* >& loadedBuffers )
#endif
{
	const CSkeletalAnimationSet* set = m_animset.Get();
	if ( set )
	{
		const TDynArray< CSkeletalAnimationSetEntry* >& animsEntry = set->GetAnimations();

		Uint32 animIt = 0;
		Bool startReading = false;

		const Uint32 size = animsEntry.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const CSkeletalAnimationSetEntry* ae = animsEntry[ i ];
			CSkeletalAnimation* anim = ae ? ae->GetAnimation() : NULL;

			if ( anim && anim->HasValidId() )
			{
				if ( !startReading && anim->GetId() >= m_firstAnimId )
				{
					PROG_ASSERT( anim->GetId() == m_firstAnimId );

					startReading = true;
				}

				if ( startReading )
				{
					PROG_ASSERT( m_animationsToLoad.Size() > animIt );
					PROG_ASSERT( loadedBuffers.Size() > animIt );

					// Is animation still not loaded?
					if ( m_animationsToLoad[ animIt ] && !anim->IsLoaded() && !anim->HasStreamingPending() )
					{
						ASSERT( CheckAnimationStreamingType( anim ) );

						anim->m_animBuffer = loadedBuffers[ animIt ];
						anim->OnStreamed();

						loadedBuffers[ animIt ] = NULL;

						m_dataSizeLoaded += anim->GetSizeOfAnimBuffer();
					}

					animIt++;
				}
			}
		}

		PROG_ASSERT( animIt == loadedBuffers.Size() );
	}

	loadedBuffers.ClearPtr();

	PROG_ASSERT( m_dataSizeLoaded <= m_dataSizeToLoad );
}

Bool AnimsetLoader::HasAnimationToLoad( const CSkeletalAnimation* animToCheck ) const
{
	const CSkeletalAnimationSet* set = m_animset.Get();
	if ( set )
	{
		const TDynArray< CSkeletalAnimationSetEntry* >& animsEntry = set->GetAnimations();

		const Uint32 size = animsEntry.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const CSkeletalAnimationSetEntry* ae = animsEntry[ i ];
			CSkeletalAnimation* anim = ae ? ae->GetAnimation() : NULL;

			if ( anim == animToCheck )
			{
				return true;
			}
		}
	}

	return false;
}

Uint32 AnimsetLoader::GetDataSizeToLoad() const
{
	return m_dataSizeToLoad;
}

Uint32 AnimsetLoader::GetDataSizeLoaded() const
{
	return m_dataSizeLoaded;
}

//////////////////////////////////////////////////////////////////////////

CAnimsetStreamingJob::CAnimsetStreamingJob( Uint32 firstAnimId, TDynArray< Uint32 >& animationsBuffersSizes, TDynArray< Bool >& animationsToLoad )
	: ILoadJob( JP_Animset )
	, m_firstAnimId( firstAnimId )
	, m_animationsBuffersSizes( animationsBuffersSizes )
	, m_animationsToLoad( animationsToLoad )
{
	
}

CAnimsetStreamingJob::~CAnimsetStreamingJob()
{
#ifdef USE_HAVOK_ANIMATION
	m_animationsBuffers.ClearPtr();
#else
	for ( Uint32 i=0; i<m_animationsBuffers.Size(); ++i )
	{
		IAnimationBuffer* buf = m_animationsBuffers[i];
		if ( NULL != buf )
		{
			delete buf;
		}
	}
#endif
}

EJobResult CAnimsetStreamingJob::Process()
{
	if ( m_animationsBuffersSizes.Empty() )
	{
		PROG_ASSERT( !m_animationsBuffersSizes.Empty() );
		return JR_Finished;
	}

	if ( m_firstAnimId == 0xFFFFFFFF )
	{
		PROG_ASSERT( m_firstAnimId != 0xFFFFFFFF );
		return JR_Finished;
	}

	// Create loading token
	IFileLatentLoadingToken* loadingToken = SAnimationsCache::GetInstance().CreateLoadingToken( m_firstAnimId );
	if ( !loadingToken )
	{
		WARN_ENGINE( TXT("AnimsetStremaing: no source loading token. Failed.") );
		return JR_Failed;
	}

	// Resume file reading
	IFile* sourceFile = loadingToken->Resume(0);
	if ( !sourceFile )
	{
		delete loadingToken;
		WARN_ENGINE( TXT("AnimsetStremaing: unable to resume loading from '%ls'."), loadingToken->GetDebugInfo().AsChar() );
		return JR_Failed;
	}

	// Delete loading token, it's not needed any more
	delete loadingToken;

	const Uint32 size = m_animationsBuffersSizes.Size();

	// Create place for anim buffers
	m_animationsBuffers.Reserve( size );

	Uint32 prevOffset = static_cast< Uint32 >( sourceFile->GetOffset() );

	// Read all animations
	for ( Uint32 i=0; i<size; ++i )
	{
		const Uint32 animSize = static_cast< Uint32 >( m_animationsBuffersSizes[ i ] );
		PROG_ASSERT( animSize > 0 );

		const Bool animToLoad = m_animationsToLoad[ i ];
		if ( animToLoad )
		{
#ifdef USE_HAVOK_ANIMATION
			// Serialize animation buffer
			AnimBuffer* buff = new AnimBuffer();
			*sourceFile << *buff;
#else
			// Serialize animation buffer
			IAnimationBuffer* buff = NULL;
			//*sourceFile << *buff;
#endif

			// Check if everything is ok
			Uint32 currOffset = static_cast< Uint32 >( sourceFile->GetOffset() );

			if ( currOffset - prevOffset != animSize )
			{
				PROG_ASSERT( currOffset - prevOffset == animSize );
				HALT( "Error in CAnimsetStreamingJob" );
			}

			prevOffset = currOffset;

			// Store buffer
			m_animationsBuffers.PushBack( buff );
		}
		else
		{
			// Seek loaded animation
			sourceFile->Seek( prevOffset + animSize );

			prevOffset = static_cast< Uint32 >( sourceFile->GetOffset() );

			m_animationsBuffers.PushBack( NULL );
		}
	}

	PROG_ASSERT( m_animationsBuffers.Size() == m_animationsToLoad.Size() );

	// Close streaming file
	delete sourceFile;

	// Finished
	return JR_Finished;
}

#ifdef USE_HAVOK_ANIMATION
void CAnimsetStreamingJob::GetLoadedBuffers( TDynArray< AnimBuffer* >& buffers )
#else
void CAnimsetStreamingJob::GetLoadedBuffers( TDynArray< IAnimationBuffer* >& buffers )
#endif
{
	PROG_ASSERT( !m_animationsBuffers.Empty() );

	buffers = m_animationsBuffers;

#ifndef USE_HAVOK_ANIMATION
	for ( Uint32 i=0; i<m_animationsBuffers.Size(); ++i )
	{
		if ( NULL != m_animationsBuffers[i] )
		{
			delete m_animationsBuffers[i];
		}
	}
#endif

	m_animationsBuffers.Clear();
}
