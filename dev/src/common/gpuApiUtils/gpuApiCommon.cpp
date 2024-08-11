/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApiUtils.h"
#include "../redThreads/redThreadsAtomic.h"
#include "../redMath/numericalutils.h"


namespace GpuApi
{

#ifdef RED_PLATFORM_CONSOLE

	static const Int32 MAX_DELETES = 32768;

	// Number of "buckets" to put queued deletions in. Resources will stay in the pending queue for NUM_BUCKETS-1 frames
	static const Uint32 NUM_BUCKETS = 4;


	struct PendingDeletes
	{
		TextureRef mTextures[MAX_DELETES];
		BufferRef mBuffers[MAX_DELETES];
		QueryRef mQueries[MAX_DELETES];
		ShaderRef mShaders[MAX_DELETES];
		Red::Threads::CAtomic< Int32 > mNumTextures;
		Red::Threads::CAtomic< Int32 > mNumBuffers;
		Red::Threads::CAtomic< Int32 > mNumQueries;
		Red::Threads::CAtomic< Int32 > mNumShaders;

		PendingDeletes()
			: mNumTextures(0)
			, mNumBuffers(0)
			, mNumQueries(0)
			, mNumShaders(0)
		{}
	};



	static PendingDeletes m_pendingDeletes[NUM_BUCKETS];

	static Red::Threads::CAtomic<Uint32> m_currentFrameCounter(0);

	static Uint32 GetBufferIndex()
	{
		return m_currentFrameCounter.GetValue() % NUM_BUCKETS;
	}
	// Advance frame counter, return index of the resources to be destroyed.
	static Uint32 AdvanceFrameCounter()
	{
#ifndef NO_GPU_ASSERTS
		{
			// The buckets that we're about to make current should be empty, cleared up in the previous frame.
			Uint32 i = ( m_currentFrameCounter.GetValue() + 1 ) % NUM_BUCKETS;
			GPUAPI_FATAL_ASSERT( m_pendingDeletes[i].mNumTextures.GetValue() == 0, "pendingDeletes.mTextures is not empty!" );
			GPUAPI_FATAL_ASSERT( m_pendingDeletes[i].mNumBuffers.GetValue() == 0,  "pendingDeletes.mBuffers is not empty!" );
			GPUAPI_FATAL_ASSERT( m_pendingDeletes[i].mNumQueries.GetValue() == 0,  "pendingDeletes.mQueries is not empty!" );
			GPUAPI_FATAL_ASSERT( m_pendingDeletes[i].mNumShaders.GetValue() == 0,  "pendingDeletes.mShaders is not empty!" );
		}
#endif
		Uint32 counter = m_currentFrameCounter.Increment();
		// counter has new value, but we want to process the next bucket so we don't add new things to the one we're
		// currently clearing out.
		return ( counter + 1 ) % NUM_BUCKETS;
	}

#endif


	void UpdateQueuedResourceDeletions()
	{
#ifdef RED_PLATFORM_CONSOLE
		const Uint32 toProcessIndex = AdvanceFrameCounter();

		PendingDeletes& pending = m_pendingDeletes[toProcessIndex];

		const Int32 numTextures = Red::Math::NumericalUtils::Min( pending.mNumTextures.GetValue(), MAX_DELETES );
		for (int i = 0; i < numTextures; ++i)
		{
			Destroy (pending.mTextures[i]);
		}
		GPUAPI_FATAL_ASSERT( numTextures == pending.mNumTextures.GetValue(), "mNumTextures has changed since we started to process queued destroys. Probably going to leak %d (or more) textures!", pending.mNumTextures.GetValue() - numTextures );
		pending.mNumTextures.SetValue( 0 );

		const Int32 numBuffers = Red::Math::NumericalUtils::Min( pending.mNumBuffers.GetValue(), MAX_DELETES );
		for (int i = 0; i < numBuffers; ++i)
		{
			Destroy (pending.mBuffers[i]);
		}
		GPUAPI_FATAL_ASSERT( numBuffers == pending.mNumBuffers.GetValue(), "mNumBuffers has changed since we started to process queued destroys. Probably going to leak %d (or more) buffers!", pending.mNumBuffers.GetValue() - numBuffers );
		pending.mNumBuffers.SetValue( 0 );

		const Int32 numQueries = Red::Math::NumericalUtils::Min( pending.mNumQueries.GetValue(), MAX_DELETES );
		for (int i = 0; i < numQueries; i++)
		{
			Destroy (pending.mQueries[i]);
		}
		GPUAPI_FATAL_ASSERT( numQueries == pending.mNumQueries.GetValue(), "mNumQueries has changed since we started to process queued destroys. Probably going to leak %d (or more) queries!", pending.mNumQueries.GetValue() - numQueries );
		pending.mNumQueries.SetValue( 0 );

		const Int32 numShaders = Red::Math::NumericalUtils::Min( pending.mNumShaders.GetValue(), MAX_DELETES );
		for (int i = 0; i < numShaders; i++)
		{
			Destroy (pending.mShaders[i]);
		}
		GPUAPI_FATAL_ASSERT( numShaders == pending.mNumShaders.GetValue(), "mNumShaders has changed since we started to process queued destroys. Probably going to leak %d (or more) shaders!", pending.mNumShaders.GetValue() - numShaders );
		pending.mNumShaders.SetValue( 0 );
#endif
	}



	void QueueForDestroy(const TextureRef& ref)
	{
#ifdef RED_PLATFORM_CONSOLE
		PendingDeletes& pending = m_pendingDeletes[GetBufferIndex()];

		Int32 numPending = pending.mNumTextures.Increment();
		GPUAPI_ASSERT(numPending < MAX_DELETES, TXT("Too many texture deletes in a frame!! this will leak"));
		if (numPending < MAX_DELETES)
		{
			pending.mTextures[numPending - 1] = ref;
		}
#else
		Destroy( ref );
#endif
	}

	void QueueForDestroy(const BufferRef& ref)
	{
#ifdef RED_PLATFORM_CONSOLE
		PendingDeletes& pending = m_pendingDeletes[GetBufferIndex()];

		Int32 numPending = pending.mNumBuffers.Increment();
		GPUAPI_ASSERT(numPending < MAX_DELETES, TXT("Too many buffer deletes in a frame!! this will leak"));
		if (numPending < MAX_DELETES)
		{
			pending.mBuffers[numPending - 1] = ref;
		}
#else
		Destroy( ref );
#endif
	}

	void QueueForDestroy(const QueryRef& ref)
	{
#ifdef RED_PLATFORM_CONSOLE
		PendingDeletes& pending = m_pendingDeletes[GetBufferIndex()];

		Int32 numPending = pending.mNumQueries.Increment();
		GPUAPI_ASSERT(numPending < MAX_DELETES, TXT("Too many query deletes in a frame!! this will leak"));
		if (numPending < MAX_DELETES)
		{
			pending.mQueries[numPending - 1] = ref;
		}
#else
		Destroy( ref );
#endif
	}

	void QueueForDestroy( const ShaderRef& ref )
	{
#ifdef RED_PLATFORM_CONSOLE
		PendingDeletes& pending = m_pendingDeletes[GetBufferIndex()];

		Int32 numPending = pending.mNumShaders.Increment();
		GPUAPI_ASSERT(numPending < MAX_DELETES, TXT("Too many shader deletes in a frame!! this will leak"));
		if (numPending < MAX_DELETES)
		{
			pending.mShaders[numPending - 1] = ref;
		}
#else
		Destroy( ref );
#endif
	}
}
