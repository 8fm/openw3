/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../core/loadingJobManager.h"
#include "animationStreamingJob.h"

namespace
{
	Bool TempCs( const CSkeletalAnimation* anim )
	{
		RED_MESSAGE( "CSkeletalAnimation has no longer access to GetParent() - this logic may be flawed now" );
		//return anim->GetParent() && anim->GetParent()->GetParent() && !anim->GetParent()->GetParent()->IsExactlyA< CCutsceneTemplate >();
		return false;
	}
}

Bool CSkeletalAnimation::SerializeBufferOnLoading() const
{
#ifdef NO_ANIM_CACHE
	return true;
#else
	return !GAnimationManager || !HasValidId();
#endif
}

Bool CSkeletalAnimation::CanBeStreamed() const 
{ 
#ifdef NO_ANIM_CACHE
	return false;
#else
	ASSERT( GAnimationManager );
	return GAnimationManager && HasValidId() && TempCs( this );
#endif 
}

Bool CSkeletalAnimation::StartStreaming()
{
	if ( !CanBeStreamed() )
	{
		// Panic check
		ASSERT( !CanBeStreamed() );
		return false;
	}

	if ( !IsLoaded() )
	{
		// Create new job
		m_streamingJob = new CAnimationStreamingJob( m_id );

		// Issue the streaming task for processing
		SJobManager::GetInstance().Issue( m_streamingJob );

		// OK if streaming task was created
		return m_streamingJob != NULL;
	}

	return true;
}

void CSkeletalAnimation::OnStreamed()
{
	// Check
	ASSERT( IsLoaded() );

	// Fill times
#ifdef USE_HAVOK_ANIMATION
	FillAnimTimes(); // removed in new multi-part animation
#endif
}

void CSkeletalAnimation::CancelStreaming()
{
	ASSERT( !IsLoaded() );

	// Cancel pending streaming and clear buffer
	if ( m_streamingJob )
	{
		m_streamingJob->Cancel();
		m_streamingJob->Release();
		m_streamingJob = NULL;
	}
}

Bool CSkeletalAnimation::UpdateStreaming()
{
	// Update streaming job
	if ( m_streamingJob && m_streamingJob->HasEnded() )
	{
		// Extract data from finished task
		if ( m_streamingJob->HasFinishedWithoutErrors() )
		{
			// Check
#ifdef USE_HAVOK_ANIMATION
			ASSERT( m_cachedAnimBufferSize > 0 );
			ASSERT( m_streamingJob->GetLoadedDataSize() == m_cachedAnimBufferSize );
#endif

			// Get loaded buffer from job
			m_animBuffer = m_streamingJob->GetLoadedBuffer();
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
