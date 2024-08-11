/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "animationBufferBitwiseCompressed.h"
#include "skeletalAnimation.h"
#include "skeletalAnimationSet.h"
#include "../core/ioTags.h"
#include "../redMath/redScalar_simd.h"
#include "../redMath/redVector4_simd.h"
#include "../redMath/vectorArithmetic_simd.h"
#include "../redMath/vectorFunctions_simd.h"

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_RTTI_ENUM( SAnimationBufferBitwiseCompression );
IMPLEMENT_RTTI_ENUM( SAnimationBufferDataCompressionMethod );
IMPLEMENT_RTTI_ENUM( SAnimationBufferOrientationCompressionMethod );
IMPLEMENT_RTTI_ENUM( SAnimationBufferBitwiseCompressionPreset );
IMPLEMENT_ENGINE_CLASS( CAnimationBufferBitwiseCompressed );
IMPLEMENT_ENGINE_CLASS( SAnimationBufferBitwiseCompressedData );
IMPLEMENT_ENGINE_CLASS( SAnimationBufferBitwiseCompressedBoneTrack );
IMPLEMENT_ENGINE_CLASS( SAnimationBufferBitwiseCompressionSettings );

///////////////////////////////////////////////////////////////////////////////

#ifdef RED_LOGGING_ENABLED
//#define BITWISE_COMPRESSION_LOG_DEBUG
#endif

///////////////////////////////////////////////////////////////////////////////

// buffer:
//		positions for all bones
//		orientations for all bones
//		scales for all bones
//		float tracks
#define BITWISE_COMPRESSION_VERSION__BLOCK_BY_BLOCK 0
// buffer:
//		bones (interleaved positions/orientations/scales)
//			per bone:
//				positions
//				orientations
//				scales
//		float tracks
#define BITWISE_COMPRESSION_VERSION__INTERLEAVED 1
// tracks have fallback
#define BITWISE_COMPRESSION_VERSION__WITH_TRACK_FALLBACK 2

#define CURRENT_BITWISE_COMPRESSION_VERSION BITWISE_COMPRESSION_VERSION__WITH_TRACK_FALLBACK

///////////////////////////////////////////////////////////////////////////////

// #define DEBUG_BITWISE_COMPRESSION

///////////////////////////////////////////////////////////////////////////////

#define INDEX_NONE -1
// how many animations can be active at once (if this limit is hit, it HAS TO BE INCREASED)
#define BITWISE_COMPRESSION_RING_BUFFER 8192
// memory limit (it's not guaranteed that whole memory will be within this limit - if all loaded animations are required/used and they exceed it, they won't be unloaded)
#define BITWISE_COMPRESSION_RING_BUFFER_MEMORY_LIMIT 1048576 * 20

///////////////////////////////////////////////////////////////////////////////

// Ring buffer holds loaded streamed animations.
// It stores if animations are used, if they've been recently used etc.
// Every load first checks if there is enough place in memory for new buffer,
// if there isn't unused buffers are removed until there is place.
// Then entry gets next index and checks if it is free (it's unlikely that all
// would be used, but there's fatal assert for that) and loads new buffer.

// For higher level: AnimationBufferBitwiseCompressed (ABBC) holds weak pointer
// to deferred data buffer. With that and information in ring buffer about
// ABBC (to work as check in both directions) it should know if deferred data
// for its contents is loaded or not (although that is job of ring buffer)
// If ABBC gets destroyed, it forces unloading.

// One inconvenience is that all stuff that uses animations requires to
// mark them as being used and release them after using. All animation nodes
// should also check if animation is loaded or not. Although this is now taken
// care of by function in "BehaviorGraphUtils.inl" file. And there should be
// always some fallback data to use in case animation is not loaded. But again,
// that's handled by already mentioned function.

///////////////////////////////////////////////////////////////////////////////

#ifdef BITWISE_COMPRESSION_LOG_DEBUG
void SBitwiseCompressionDataRingBuffer__LogUsage( const String & action );
#endif

struct SBitwiseCompressionDataRingBufferEntry
{
public:
	AnimationDeferredDataBufferHandle m_data;
	BufferAsyncDataHandle m_loadingAsyncToken;
	CAnimationBufferBitwiseCompressed* m_animationBuffer;

private:
	mutable Red::Threads::CRWSpinLock m_lock;
	Red::Threads::CAtomic< Int32 > m_usageCount;
	Red::Threads::CAtomic< Int32 > m_touched;
	Red::Threads::CAtomic< Int32 > m_beingLoadedAsync;
#ifdef DEBUG_ANIMATION_USAGE
	CName m_animationName;
#endif

	typedef Red::Threads::CScopedLock< Red::Threads::CRWSpinLock > ScopedWriteLock;
	typedef Red::Threads::CScopedSharedLock< Red::Threads::CRWSpinLock > ScopedReadLock;

public:
	SBitwiseCompressionDataRingBufferEntry()
	: m_usageCount( 0 )
	, m_touched( 0 )
	, m_beingLoadedAsync( 0 )
	{
	}

	~SBitwiseCompressionDataRingBufferEntry()
	{
		// we should be deinitialized!
		RED_ASSERT(m_animationBuffer == nullptr);
	}

	RED_INLINE void Load( CAnimationBufferBitwiseCompressed* buffer, Bool loadSync )
	{
		m_touched.Increment();
		// we should be deinitialized!
		RED_ASSERT(m_animationBuffer == nullptr);
		m_animationBuffer = buffer;
#ifndef NO_EDITOR
		if( ( GIsEditor && !GGame->IsActive() ) || loadSync )
#else
		if( loadSync )
#endif
		{
			// load sync - we want to have data immediately
			m_data = buffer->m_deferredData.AcquireBufferHandleSync();
			// cancel async loading if was issued (unlikely but still...)
			m_loadingAsyncToken.Reset();
#ifdef BITWISE_COMPRESSION_LOG_DEBUG
#ifdef DEBUG_ANIMATION_USAGE
			SBitwiseCompressionDataRingBuffer__LogUsage( String::Printf( TXT("EntryLoadedSync %s"), m_animationName.AsChar() ) );
#else
			SBitwiseCompressionDataRingBuffer__LogUsage( TXT("EntryLoadedSync") );
#endif
#endif
		}
		else if ( m_beingLoadedAsync.CompareExchange( 1, 0 ) == 0 )
		{
			// load async - we want to have steady framerate
			// we may add priorities to animations, right now load everything with the same priority
			const auto tag = eIOTag_AnimationsNormal;
			m_loadingAsyncToken = buffer->m_deferredData.AcquireBufferHandleAsync( tag, [&]( BufferHandle result ){ LoadedAsync( result ); } );
		}
	}

	RED_INLINE void LoadedAsync( BufferHandle buffer )
	{
		{
			// Once again, atomic doesnt mean thread safe ... Applying duck tape now!.
			ScopedWriteLock scopedLock( m_lock );
			m_data = buffer;
		}

		m_beingLoadedAsync.SetValue( 0 );
		m_loadingAsyncToken.Reset();
		
#ifdef BITWISE_COMPRESSION_LOG_DEBUG
#ifdef DEBUG_ANIMATION_USAGE
		SBitwiseCompressionDataRingBuffer__LogUsage( String::Printf( TXT("EntryLoadedAsync %s"), m_animationName.AsChar() ) );
#else
		SBitwiseCompressionDataRingBuffer__LogUsage( TXT("EntryLoadedAsync") );
#endif
#endif
	}

	RED_INLINE void Unload()
	{
		AnimationDeferredDataBufferHandle unloadedHandle;
		BufferAsyncDataHandle unloadedToken;

		{
			// To reduce contention, I'm moving the memory free out of the exclusive lock.
			ScopedReadLock scopedLock( m_lock );
			unloadedHandle = m_data;
			unloadedToken = m_loadingAsyncToken;
		}

		{
			ScopedWriteLock scopedLock( m_lock );

			// This won't actully free memory, stack variable unloadedHandle and unloadedToken also have strong reference.
			m_animationBuffer = nullptr;
			m_data.Reset();
			m_loadingAsyncToken.Reset();
			m_usageCount.SetValue( 0 ); // mark that it won't be used
			m_touched.SetValue( 0 );
		}

		// No more lock, now free memory.
		unloadedHandle.Reset();
		unloadedToken.Reset();

#ifdef BITWISE_COMPRESSION_LOG_DEBUG
#ifdef DEBUG_ANIMATION_USAGE
		SBitwiseCompressionDataRingBuffer__LogUsage( String::Printf( TXT("EntryUnloaded %s"), m_animationName.AsChar() ) );
#else
		SBitwiseCompressionDataRingBuffer__LogUsage( TXT("EntryUnloaded") );
#endif
#endif
	}

	RED_INLINE Uint32 UnloadUnused()
	{
		if ( Uint32 unloadedSize = GetMemorySize() )
		{
			if ( /* m_touched.GetValue() == 0 && */ m_usageCount.GetValue() == 0 ) // check touched when it would be periodically cleared
			{
				Unload();
				return unloadedSize;
			}
		}
		return 0;
	}

	RED_INLINE void AddUsage( DEBUG_ANIMATION_USAGE_PARAM_LIST )
	{
#ifdef DEBUG_ANIMATION_USAGE
		m_animationName = DEBUG_ANIMATION_USAGE_PARAM_NAME;
#endif
		m_usageCount.Increment();
	}

	RED_INLINE void ReleaseUsage()
	{
		m_usageCount.Decrement();
	}

	RED_INLINE void Touch()
	{
		m_touched.Increment();
	}	

	Uint32 GetMemorySize() const
	{
		// Another Thread can release the handle. 
		AnimationDeferredDataBufferHandle data;

		{
			ScopedReadLock scopedLock( m_lock );
			data = m_data;
		}
		
		if ( data )
		{
			return data.Get()->GetSize();
		}
		else
		{
			return 0;
		}
	}

	RED_INLINE Bool IsUsed() const { return m_usageCount.GetValue() != 0; }

	RED_INLINE Bool IsOccupied() const { return IsUsed() || m_data.Get(); }

	RED_INLINE Uint32 GetUsageCount() const { return m_usageCount.GetValue(); }

	RED_INLINE Bool IsBeingStreamedIn() const { return m_beingLoadedAsync.GetValue() != 0; }

#ifdef DEBUG_ANIMATION_USAGE
	RED_INLINE CName const & GetAnimationName() const { return m_animationName; }
#endif

	Bool GetDataBufferPtr( CAnimationBufferBitwiseCompressed const* buffer, AnimationDeferredDataBufferPtr& ptr )
	{
		AnimationDeferredDataBufferHandle handle;

		{
			ScopedReadLock scopedLock( m_lock );
			if (m_animationBuffer != buffer)
			{
				// owner has changed
				return false;
			}

			if( !m_data.Get() && !IsBeingStreamedIn() )
			{
				return false;
			}

			handle = m_data;
		}
	
		// If we reach this, and handle variable is true, it will be true until this we exit this function.
		if ( handle )
		{
			// HACK atomic doesnt mean thread safe. Two thread can call this function with same ptr variable. That is not thread safe ...
			// Applying duck tape now. I'm doing some locking magic to reduce thread issue. But It can still potentially explode! 
			// Needs heavy refactor, but this too heavy as game is already on shelve. Should be fix in patch at one point, with a lot of test and QA!
			AnimationDeferredDataBufferHandle strongPtr;
			{
				ScopedReadLock scopedLock( m_lock );
				strongPtr = ptr.Lock();
			}

			if( handle != strongPtr)
			{
				// This is insanely dangerous ... I have no control on what happen to ptr varible outside this function. I can't garanty thread safety if I'm not in charge of variable!
				ScopedWriteLock scopedLock( m_lock );
				strongPtr = ptr.Lock();
				if( handle != strongPtr )
				{
					ptr = m_data;
				}
				else
				{ /* another thread got the lock before me and modified the ptr variable. DO NOTHING else the universe will explode! */ }
			}
		}
		
		// loaded or being loaded
		return true;
	}
};

struct SBitwiseCompressionDataRingBuffer
{
	static SBitwiseCompressionDataRingBufferEntry s_ring[BITWISE_COMPRESSION_RING_BUFFER];
	static Red::Threads::CAtomic< Uint32 > s_ringIndex; // Cannot be signed, the increment function can called up 4k times for one get. After hours of gameplay without quitting game, overflow is possible resulting evil memory stomp

	// TODO add method that will remove unused ring buffer entries - run once in a minute or so

	/* returns true if already loaded or being loaded. false if no longer available. */
	RED_INLINE static Bool GetDataBufferPtr(CAnimationBufferBitwiseCompressed const* buffer, AnimationDeferredDataBufferPtr& ptr )
	{
		Int32 index = buffer->GetRingIndex();
		if ( index != INDEX_NONE )
		{
			SBitwiseCompressionDataRingBufferEntry & entry = s_ring[index];

			return entry.GetDataBufferPtr( buffer, ptr );
		}
		// not available, not even requested to load
		return false;
	}

	/* returns true if already loaded or being loaded. false if no longer available. */
	RED_INLINE static Bool IsThereDataBufferPtr(CAnimationBufferBitwiseCompressed const* buffer)
	{
		Int32 index = buffer->GetRingIndex();
		if ( index != INDEX_NONE )
		{
			const SBitwiseCompressionDataRingBufferEntry & entry = s_ring[index];
			if (entry.m_animationBuffer != buffer)
			{
				// owner has changed
				return false;
			}
			// loaded or being loaded
			return true;
		}
		// not available, not even requested to load
		return false;
	}

	RED_INLINE static Int32 Load( CAnimationBufferBitwiseCompressed* buffer, Bool loadSync )
	{
		Uint32 dataToBeLoaded = buffer->m_deferredData.GetSize();
		Uint32 dataAlreadyLoaded = 0;
		{	// if during this process something is created or destroyed, we should be fine with that (sounds like "most famous last words before death")
			for ( SBitwiseCompressionDataRingBufferEntry *entry = s_ring, *endEntry = &s_ring[BITWISE_COMPRESSION_RING_BUFFER]; entry != endEntry; ++ entry )
			{
				dataAlreadyLoaded += entry->GetMemorySize();
			}
			if ( dataToBeLoaded + dataAlreadyLoaded > BITWISE_COMPRESSION_RING_BUFFER_MEMORY_LIMIT )
			{
				// unload starting with following (which should be the oldest)
				Uint32 offset = s_ringIndex.GetValue() + 1;
				for ( Uint32 idx = 0; idx < BITWISE_COMPRESSION_RING_BUFFER && dataToBeLoaded + dataAlreadyLoaded > BITWISE_COMPRESSION_RING_BUFFER_MEMORY_LIMIT; ++ idx )
				{
					SBitwiseCompressionDataRingBufferEntry & entry = s_ring[ ( idx + offset ) % BITWISE_COMPRESSION_RING_BUFFER ];
					dataAlreadyLoaded -= entry.UnloadUnused();
				}
			}
		}
		Int32 useIndex = s_ringIndex.Increment() % BITWISE_COMPRESSION_RING_BUFFER;
		SBitwiseCompressionDataRingBufferEntry* entry = &s_ring[useIndex];
		// check if maybe this one is used
		
		Int32 tryCount = 0;
		while ( entry->IsUsed() )
		{
			useIndex = s_ringIndex.Increment() % BITWISE_COMPRESSION_RING_BUFFER;
			entry = &s_ring[useIndex];
			++ tryCount;
			RED_FATAL_ASSERT( tryCount < BITWISE_COMPRESSION_RING_BUFFER, "No place for entry! Increase BITWISE_COMPRESSION_RING_BUFFER" );
		}
		entry->Unload();
		entry->Load( buffer, loadSync );
#ifdef BITWISE_COMPRESSION_LOG_DEBUG
		LogUsage( TXT("Load"));
#endif
		return useIndex;
	}
	
	RED_INLINE static SBitwiseCompressionDataRingBufferEntry * GetEntry( const CAnimationBufferBitwiseCompressed * buffer )
	{
		Int32 index = buffer->GetRingIndex();
		if ( index != INDEX_NONE )
		{
			SBitwiseCompressionDataRingBufferEntry* entry = &s_ring[ index ];
			if ( entry->m_animationBuffer == buffer )
			{
				return entry;
			}
		}
		return nullptr;

	}
	
	RED_INLINE static void Unload( CAnimationBufferBitwiseCompressed* buffer )
	{
		if ( SBitwiseCompressionDataRingBufferEntry * entry = GetEntry( buffer ) )
		{
			entry->Unload();
		}
	}

	RED_INLINE static void AddUsage( const CAnimationBufferBitwiseCompressed * buffer DEBUG_ANIMATION_USAGE_PARAM_LIST_CONT )
	{
		if ( SBitwiseCompressionDataRingBufferEntry * entry = GetEntry( buffer ) )
		{
			entry->AddUsage( DEBUG_ANIMATION_USAGE_PARAM_NAME );
		}
#ifdef BITWISE_COMPRESSION_LOG_DEBUG
#ifdef DEBUG_ANIMATION_USAGE
		LogUsage( String::Printf( TXT("AddUsage %s"), DEBUG_ANIMATION_USAGE_PARAM_NAME.AsChar() ) );
#else
		LogUsage( TXT("AddUsage") );
#endif
#endif
	}
	
	RED_INLINE static void ReleaseUsage( const CAnimationBufferBitwiseCompressed * buffer )
	{
		if ( SBitwiseCompressionDataRingBufferEntry * entry = GetEntry( buffer ) )
		{
			entry->ReleaseUsage();
#ifdef BITWISE_COMPRESSION_LOG_DEBUG
#ifdef DEBUG_ANIMATION_USAGE
			LogUsage( String::Printf( TXT("ReleaseUsage %s"), entry->GetAnimationName().AsChar() ) );
#else
			LogUsage( TXT("ReleaseUsage") );
#endif
#endif
		}
		else
		{
#ifdef BITWISE_COMPRESSION_LOG_DEBUG
			LogUsage( TXT("ReleaseUsage") );
#endif
		}
	}

	RED_INLINE static void LogUsage( const String & action )
	{
#ifdef BITWISE_COMPRESSION_LOG_DEBUG
		Uint32 loadedCount = 0;
		Uint32 usedCount = 0;
		Uint32 loadedMemory = 0;
		for ( SBitwiseCompressionDataRingBufferEntry *eIt = s_ring, *endEntry = &s_ring[BITWISE_COMPRESSION_RING_BUFFER]; eIt != endEntry; ++ eIt )
		{
			if ( eIt->IsOccupied() )
			{
				++ loadedCount;
			}
			if ( eIt->IsUsed() )
			{
				++ usedCount;
			}
			loadedMemory += eIt->GetMemorySize();
		}
		RED_LOG(AnimationStreaming, TXT("Animations used %i of %i loaded (memory used %i B) [%s]"), usedCount, loadedCount, loadedMemory, action.AsChar());
#ifdef DEBUG_ANIMATION_USAGE
#ifdef DEBUG_ANIMATION_USAGE__DETAILED
		Uint32 index = 0;
		for ( SBitwiseCompressionDataRingBufferEntry *eIt = s_ring, *endEntry = &s_ring[BITWISE_COMPRESSION_RING_BUFFER]; eIt != endEntry; ++ eIt )
		{
			if ( eIt->IsUsed() )
			{
				RED_LOG(AnimationStreaming, TXT("  %3i. %s (%i)"), index, eIt->GetAnimationName().AsChar(), eIt->GetUsageCount());
				++ index;
			}
		}
#endif
#endif
#endif
	}

	RED_INLINE static void Touch( const CAnimationBufferBitwiseCompressed * buffer )
	{
		if ( SBitwiseCompressionDataRingBufferEntry * entry = GetEntry( buffer ) )
		{
			entry->Touch();
		}
	}

	static Uint32 GetMemorySize( const CAnimationBufferBitwiseCompressed * buffer )
	{
		if ( SBitwiseCompressionDataRingBufferEntry * entry = GetEntry( buffer ) )
		{
			return entry->GetMemorySize();
		}
		return 0;
	}

	static void GetStreamingNumbers(Uint32 & outStreamedInAnimations, Uint32 & outUsedStreamedInAnimations, Uint32 & outAnimationsBeingStreamedIn)
	{
		outStreamedInAnimations = 0;
		outUsedStreamedInAnimations = 0;
		outAnimationsBeingStreamedIn = 0;
		for ( SBitwiseCompressionDataRingBufferEntry *entry = s_ring, *endEntry = &s_ring[BITWISE_COMPRESSION_RING_BUFFER]; entry != endEntry; ++ entry )
		{
			if ( entry->IsOccupied() )
			{
				if ( entry->IsBeingStreamedIn() )
				{
					++ outAnimationsBeingStreamedIn;
				}
				else
				{
					++ outStreamedInAnimations;
				}
				if ( entry->IsUsed() )
				{
					++ outUsedStreamedInAnimations;
				}
			}
		}
	}

	static void Flush()
	{
		for( Uint32 index = 0; index != BITWISE_COMPRESSION_RING_BUFFER; ++index )
		{
			SBitwiseCompressionDataRingBufferEntry & entry = s_ring[ index ];
			entry.Unload();
		}
	}
};

#ifdef BITWISE_COMPRESSION_LOG_DEBUG
void SBitwiseCompressionDataRingBuffer__LogUsage( const String & action )
{
	SBitwiseCompressionDataRingBuffer::LogUsage( action );
}
#endif

SBitwiseCompressionDataRingBufferEntry SBitwiseCompressionDataRingBuffer::s_ring[BITWISE_COMPRESSION_RING_BUFFER];
Red::Threads::CAtomic< Uint32 > SBitwiseCompressionDataRingBuffer::s_ringIndex( 0 );

void FlushAllAnimationBuffer()
{
	SBitwiseCompressionDataRingBuffer::Flush();
}


///////////////////////////////////////////////////////////////////////////////

SAnimationBufferBitwiseCompressionSettings::SAnimationBufferBitwiseCompressionSettings()
:	m_orientationCompressionMethod( ABOCM_AsFloat_XYZSignedW )
{
	UsePreset( ABBCP_NormalQuality );
}

void SAnimationBufferBitwiseCompressionSettings::UsePreset( SAnimationBufferBitwiseCompressionPreset preset )
{
	switch ( preset )
	{
	case ABBCP_VeryHighQuality:
		m_translationTolerance				= 0.0000f;
		m_translationSkipFrameTolerance		= 0.0000f;
		m_orientationTolerance				= 0.00005f;
		m_orientationCompressionMethod		= ABOCM_PackIn64bitsW;
		m_orientationSkipFrameTolerance		= 0.0001f;
		m_scaleTolerance					= 0.001f;
		m_scaleSkipFrameTolerance			= 0.001f;
		m_trackTolerance					= 0.0000f;
		m_trackSkipFrameTolerance			= 0.0005f;
		break;
	case ABBCP_HighQuality:
		m_translationTolerance				= 0.001f;
		m_translationSkipFrameTolerance		= 0.001f;
		m_orientationTolerance				= 0.002f;
		m_orientationCompressionMethod		= ABOCM_PackIn64bitsW;
		m_orientationSkipFrameTolerance		= 0.01f;
		m_scaleTolerance					= 0.01f;
		m_scaleSkipFrameTolerance			= 0.01f;
		m_trackTolerance					= 0.0005f;
		m_trackSkipFrameTolerance			= 0.0005f;
		break;
	case ABBCP_NormalQuality:
		m_translationTolerance				= 0.01f;
		m_translationSkipFrameTolerance		= 0.01f;
		m_orientationTolerance				= 0.002f;
		m_orientationCompressionMethod		= ABOCM_PackIn48bitsW;
		m_orientationSkipFrameTolerance		= 0.01f;
		m_scaleTolerance					= 0.01f;
		m_scaleSkipFrameTolerance			= 0.01f;
		m_trackTolerance					= 0.001f;
		m_trackSkipFrameTolerance			= 0.001f;
		break;
	case ABBCP_LowQuality:
		m_translationTolerance				= 0.01f;
		m_translationSkipFrameTolerance		= 0.02f;
		m_orientationTolerance				= 0.002f;
		m_orientationCompressionMethod		= ABOCM_PackIn40bitsW;
		m_orientationSkipFrameTolerance		= 0.02f;
		m_scaleTolerance					= 0.02f;
		m_scaleSkipFrameTolerance			= 0.02f;
		m_trackTolerance					= 0.005f;
		m_trackSkipFrameTolerance			= 0.005f;
		break;
	case ABBCP_VeryLowQuality:
		m_translationTolerance				= 0.01f;
		m_translationSkipFrameTolerance		= 0.02f;
		m_orientationTolerance				= 0.002f;
		m_orientationCompressionMethod		= ABOCM_PackIn32bits;
		m_orientationSkipFrameTolerance		= 0.02f;
		m_scaleTolerance					= 0.02f;
		m_scaleSkipFrameTolerance			= 0.02f;
		m_trackTolerance					= 0.01f;
		m_trackSkipFrameTolerance			= 0.01f;
		break;
	case ABBCP_Raw:
		m_translationTolerance				= 0.0f;
		m_translationSkipFrameTolerance		= 0.0f;
		m_orientationTolerance				= 0.0f;
		m_orientationCompressionMethod		= ABOCM_AsFloat_XYZSignedWInLastBit;
		m_orientationSkipFrameTolerance		= 0.0f;
		m_scaleTolerance					= 0.0f;
		m_scaleSkipFrameTolerance			= 0.0f;
		m_trackTolerance					= 0.0f;
		m_trackSkipFrameTolerance			= 0.0f;
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////

/**
 *	To not scare anyone!
 *	This context is to pack common code that is shared by different uncompress methods.
 *	I decided to have such code for uncompression to gain speed as much as possible - to have no branches (oh, please change "operator ?" to "select")
 *	Same goes for uncompression of frame (shifting and masking stuff) and for size of single data (assumption)
 */
struct SAnimationBufferBitwiseUncompressionContext
{
	Uint32 m_singleDataSize;
	Uint32 m_numChannels;
	Uint32 m_numChannelsCorrected;
	Uint32 m_frameOffset;

	// first frame
	Uint32 m_firstFrame;
	const Int8* m_firstDataPtr;

	// second frame
	Uint32 m_secondFrame;
	Float m_atSecondFrame;
	const Int8* m_secondDataPtr;

	Float* m_out;
	RED_ALIGNED_VAR( Float, 16 ) m_outNext[4];

	SAnimationBufferBitwiseUncompressionContext( Uint32 singleDataSize, Uint32 numChannels, Float* out );

	void PrepareFrameAddresses( Float time, Float dt, Uint32 numFrames, Float duration, Uint32 dataAddr, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData );

	const Int8* GetDataPtr( Uint32 dataAddr, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData );

	void InterpolateFramePlain();

	void InterpolateFrameQuaternions();

};

SAnimationBufferBitwiseUncompressionContext::SAnimationBufferBitwiseUncompressionContext( Uint32 singleDataSize, Uint32 numChannels, Float* out )
	:	m_singleDataSize( singleDataSize )
	,	m_numChannels( numChannels )
	,	m_numChannelsCorrected( numChannels )
	,	m_frameOffset( 0 )
	,	m_out( out )
{
	ASSERT( numChannels <= 4, TXT("More channels than 4? Increase outNext size") );
}

// modify channels and frame offset before calling this

RED_FORCE_INLINE void SAnimationBufferBitwiseUncompressionContext::PrepareFrameAddresses( Float time, Float dt, Uint32 numFrames, Float duration, Uint32 dataAddr, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData )
{
	const Float frame = time / dt;
	const Uint32 frameFloor = static_cast< Uint32 >( frame );
	m_firstFrame = frameFloor;

	// find second frame and how far are we at second frame
	m_secondFrame = Min(m_firstFrame + 1, numFrames - 1); // do not exceed range

	if( numFrames <= 1 )
	{
		m_atSecondFrame = 0.0f;
		m_frameOffset = 0;
	}
	else
	{
		const Float endsAt = m_firstFrame == (Uint32)numFrames - 2? duration : ( frameFloor + 1.0f ) * dt;
		const Float startsAt = frameFloor * dt;
		const Float frameDuration = endsAt - startsAt;
		m_atSecondFrame = ( time - startsAt ) / frameDuration;
		m_frameOffset = ( m_numChannelsCorrected * m_singleDataSize + m_frameOffset );
	}	

	m_firstDataPtr = GetDataPtr( dataAddr + m_frameOffset * m_firstFrame, firstCompDataSize, firstCompData, secondCompData );
	m_secondDataPtr = GetDataPtr( dataAddr + m_frameOffset * m_secondFrame, firstCompDataSize, firstCompData, secondCompData );
}

const Int8* SAnimationBufferBitwiseUncompressionContext::GetDataPtr( Uint32 dataAddr, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData )
{
	return dataAddr < firstCompDataSize? firstCompData + dataAddr : secondCompData + dataAddr - firstCompDataSize;
}

void SAnimationBufferBitwiseUncompressionContext::InterpolateFramePlain()
{
	// interpolate in plain manner
	const Float nextFrame = m_atSecondFrame;
	const Float currFrame = 1.0f - m_atSecondFrame;
	Float * outPtr = m_out;
	const Float * outNextPtr = m_outNext;
	for ( Uint32 i=0; i<  m_numChannels; ++ i, ++ outPtr, ++ outNextPtr )
	{
		*outPtr *= currFrame;
		*outPtr += nextFrame * *outNextPtr;
	}
}

void SAnimationBufferBitwiseUncompressionContext::InterpolateFrameQuaternions()
{
	// simple Lerp here
	RedVector4 out( m_out );
	RedVector4 outNext( m_outNext );

	const RedFloat1 oneMinus( 1.0f - m_atSecondFrame );
	const RedFloat1 dotProduct = Dot( out, outNext );
	const RedFloat1 signedT( dotProduct.X < 0.0f ? -m_atSecondFrame : m_atSecondFrame );
	RedVector4 left, right;
	Mul( left, outNext, signedT );
	Mul( right, out, oneMinus );
	Add( out, left, right );
	out.Normalize4();
	out.Store( m_out );
}

///////////////////////////////////////////////////////////////////////////////

void SAnimationBufferBitwiseCompressedData::Uncompress( const SAnimationBufferBitwiseCompressedData & data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float time, Float duration, Float* out )
{
	SAnimationBufferBitwiseUncompressionContext context( SizeOfSingleData( (SAnimationBufferBitwiseCompression)data.m_compression ), numChannels, out );

	// prepare frame addresses
	context.PrepareFrameAddresses( time, data.m_dt, data.m_numFrames, duration, data.m_dataAddr, firstCompDataSize, firstCompData, secondCompData );

	// uncompress frames
	data.UncompressFrame( context.m_numChannelsCorrected, context.m_singleDataSize, context.m_firstDataPtr, context.m_out );
	data.UncompressFrame( context.m_numChannelsCorrected, context.m_singleDataSize, context.m_secondDataPtr, context.m_outNext );

	context.InterpolateFramePlain();
	// ASSERT( ! Red::Math::NumericalUtils::IsNan( *out ) && ! Red::Math::NumericalUtils::IsNan( *(out + 1) ) && ! Red::Math::NumericalUtils::IsNan( *(out + 2) ) );
	// ASSERT( Red::Math::NumericalUtils::IsFinite( *out ) && Red::Math::NumericalUtils::IsFinite( *(out + 1) ) && Red::Math::NumericalUtils::IsFinite( *(out + 2) ) );
}

void SAnimationBufferBitwiseCompressedData::UncompressQuaternion( const SAnimationBufferBitwiseCompressedData & data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float time, Float duration, Float* out )
{
	SAnimationBufferBitwiseUncompressionContext context( SizeOfSingleData( (SAnimationBufferBitwiseCompression)data.m_compression ), numChannels, out );

	// prepare frame addresses
	context.PrepareFrameAddresses( time, data.m_dt, data.m_numFrames, duration, data.m_dataAddr, firstCompDataSize, firstCompData, secondCompData );

	// uncompress frames
	data.UncompressFrame( context.m_numChannelsCorrected, context.m_singleDataSize, context.m_firstDataPtr, context.m_out );
	data.UncompressFrame( context.m_numChannelsCorrected, context.m_singleDataSize, context.m_secondDataPtr, context.m_outNext );

	context.InterpolateFrameQuaternions();
}

void SAnimationBufferBitwiseCompressedData::UncompressQuaternionXYZSignedW( const SAnimationBufferBitwiseCompressedData & data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float time, Float duration, Float* out )
{
	SAnimationBufferBitwiseUncompressionContext context( SizeOfSingleData( (SAnimationBufferBitwiseCompression)data.m_compression ), numChannels, out );

	// we actually have one less channel
	context.m_numChannelsCorrected = numChannels - 1;
	context.m_frameOffset = 1; // for signed W if not stored in last bit

	// prepare frame addresses
	context.PrepareFrameAddresses( time, data.m_dt, data.m_numFrames, duration, data.m_dataAddr, firstCompDataSize, firstCompData, secondCompData );

	// uncompress frames
	data.UncompressFrameQuaternionXYZSignedW( context.m_singleDataSize, context.m_firstDataPtr, context.m_out );
	data.UncompressFrameQuaternionXYZSignedW( context.m_singleDataSize, context.m_secondDataPtr, context.m_outNext );

	context.InterpolateFrameQuaternions();
}

void SAnimationBufferBitwiseCompressedData::UncompressQuaternionXYZSignedWInLastBit( const SAnimationBufferBitwiseCompressedData & data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float time, Float duration, Float* out )
{
	SAnimationBufferBitwiseUncompressionContext context( SizeOfSingleData( (SAnimationBufferBitwiseCompression)data.m_compression ), numChannels, out );

	// we actually have one less channel
	context.m_numChannelsCorrected = numChannels - 1;

	// prepare frame addresses
	context.PrepareFrameAddresses( time, data.m_dt, data.m_numFrames, duration, data.m_dataAddr, firstCompDataSize, firstCompData, secondCompData );

	// uncompress frames
	data.UncompressFrameQuaternionXYZSignedWInLastBit( context.m_singleDataSize, context.m_firstDataPtr, context.m_out );
	data.UncompressFrameQuaternionXYZSignedWInLastBit( context.m_singleDataSize, context.m_secondDataPtr, context.m_outNext );

	context.InterpolateFrameQuaternions();
}

void SAnimationBufferBitwiseCompressedData::UncompressQuaternion64bXYZW( const SAnimationBufferBitwiseCompressedData & data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float time, Float duration, Float* out )
{
	SAnimationBufferBitwiseUncompressionContext context( 8 /*64b*/, 1 /*quat*/, out );

	// prepare frame addresses
	context.PrepareFrameAddresses( time, data.m_dt, data.m_numFrames, duration, data.m_dataAddr, firstCompDataSize, firstCompData, secondCompData );

	// uncompress frames
	data.UncompressFrameQuaternion64bXYZW( context.m_singleDataSize, context.m_firstDataPtr, context.m_out );
	data.UncompressFrameQuaternion64bXYZW( context.m_singleDataSize, context.m_secondDataPtr, context.m_outNext );

	context.InterpolateFrameQuaternions();
	// ASSERT( ! Red::Math::NumericalUtils::IsNan( *out ) && ! Red::Math::NumericalUtils::IsNan( *(out + 1) ) && ! Red::Math::NumericalUtils::IsNan( *(out + 2) ) && ! Red::Math::NumericalUtils::IsNan( *(out + 3) ) );
	// ASSERT( Red::Math::NumericalUtils::IsFinite( *out ) && Red::Math::NumericalUtils::IsFinite( *(out + 1) ) && Red::Math::NumericalUtils::IsFinite( *(out + 2) ) && Red::Math::NumericalUtils::IsFinite( *(out + 3) ) );
}

void SAnimationBufferBitwiseCompressedData::UncompressQuaternion48bXYZ( const SAnimationBufferBitwiseCompressedData & data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float time, Float duration, Float* out )
{
	SAnimationBufferBitwiseUncompressionContext context( 6 /*48b*/, 1 /*quat*/, out );

	// prepare frame addresses
	context.PrepareFrameAddresses( time, data.m_dt, data.m_numFrames, duration, data.m_dataAddr, firstCompDataSize, firstCompData, secondCompData );

	// uncompress frames
	data.UncompressFrameQuaternion48bXYZ( context.m_singleDataSize, context.m_firstDataPtr, context.m_out );
	data.UncompressFrameQuaternion48bXYZ( context.m_singleDataSize, context.m_secondDataPtr, context.m_outNext );

	context.InterpolateFrameQuaternions();
}

void SAnimationBufferBitwiseCompressedData::UncompressQuaternion48bXYZW( const SAnimationBufferBitwiseCompressedData & data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float time, Float duration, Float* out )
{
	SAnimationBufferBitwiseUncompressionContext context( 6 /*48b*/, 1 /*quat*/, out );

	// prepare frame addresses
	context.PrepareFrameAddresses( time, data.m_dt, data.m_numFrames, duration, data.m_dataAddr, firstCompDataSize, firstCompData, secondCompData );

	// uncompress frames
	data.UncompressFrameQuaternion48bXYZW( context.m_singleDataSize, context.m_firstDataPtr, context.m_out );
	data.UncompressFrameQuaternion48bXYZW( context.m_singleDataSize, context.m_secondDataPtr, context.m_outNext );

	context.InterpolateFrameQuaternions();
}

void SAnimationBufferBitwiseCompressedData::UncompressQuaternion40bXYZ( const SAnimationBufferBitwiseCompressedData & data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float time, Float duration, Float* out )
{
	SAnimationBufferBitwiseUncompressionContext context( 5 /*40b*/, 1 /*quat*/, out );

	// prepare frame addresses
	context.PrepareFrameAddresses( time, data.m_dt, data.m_numFrames, duration, data.m_dataAddr, firstCompDataSize, firstCompData, secondCompData );

	// uncompress frames
	data.UncompressFrameQuaternion40bXYZ( context.m_singleDataSize, context.m_firstDataPtr, context.m_out );
	data.UncompressFrameQuaternion40bXYZ( context.m_singleDataSize, context.m_secondDataPtr, context.m_outNext );

	context.InterpolateFrameQuaternions();
}

void SAnimationBufferBitwiseCompressedData::UncompressQuaternion40bXYZW( const SAnimationBufferBitwiseCompressedData & data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float time, Float duration, Float* out )
{
	SAnimationBufferBitwiseUncompressionContext context( 5 /*40b*/, 1 /*quat*/, out );

	// prepare frame addresses
	context.PrepareFrameAddresses( time, data.m_dt, data.m_numFrames, duration, data.m_dataAddr, firstCompDataSize, firstCompData, secondCompData );

	// uncompress frames
	data.UncompressFrameQuaternion40bXYZW( context.m_singleDataSize, context.m_firstDataPtr, context.m_out );
	data.UncompressFrameQuaternion40bXYZW( context.m_singleDataSize, context.m_secondDataPtr, context.m_outNext );

	context.InterpolateFrameQuaternions();
}

void SAnimationBufferBitwiseCompressedData::UncompressQuaternion32bXYZ( const SAnimationBufferBitwiseCompressedData & data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float time, Float duration, Float* out )
{
	SAnimationBufferBitwiseUncompressionContext context( 4 /*32b*/, 1 /*quat*/, out );

	// prepare frame addresses
	context.PrepareFrameAddresses( time, data.m_dt, data.m_numFrames, duration, data.m_dataAddr, firstCompDataSize, firstCompData, secondCompData );

	// uncompress frames
	data.UncompressFrameQuaternion32bXYZ( context.m_singleDataSize, context.m_firstDataPtr, context.m_out );
	data.UncompressFrameQuaternion32bXYZ( context.m_singleDataSize, context.m_secondDataPtr, context.m_outNext );

	context.InterpolateFrameQuaternions();
}

void SAnimationBufferBitwiseCompressedData::UncompressFallback( const SAnimationBufferBitwiseCompressedData & data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float* out )
{
	SAnimationBufferBitwiseUncompressionContext context( SizeOfSingleData( (SAnimationBufferBitwiseCompression)data.m_compression ), numChannels, out );

	// prepare frame addresses
	context.m_firstDataPtr = context.GetDataPtr( data.m_dataAddrFallback, firstCompDataSize, firstCompData, secondCompData );

	// uncompress frame
	data.UncompressFrame( context.m_numChannelsCorrected, context.m_singleDataSize, context.m_firstDataPtr, context.m_out );

	// ASSERT( ! Red::Math::NumericalUtils::IsNan( *out ) && ! Red::Math::NumericalUtils::IsNan( *(out + 1) ) && ! Red::Math::NumericalUtils::IsNan( *(out + 2) ) );
	// ASSERT( Red::Math::NumericalUtils::IsFinite( *out ) && Red::Math::NumericalUtils::IsFinite( *(out + 1) ) && Red::Math::NumericalUtils::IsFinite( *(out + 2) ) );
}

void SAnimationBufferBitwiseCompressedData::UncompressFallbackQuaternion( const SAnimationBufferBitwiseCompressedData & data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float* out )
{
	SAnimationBufferBitwiseUncompressionContext context( SizeOfSingleData( (SAnimationBufferBitwiseCompression)data.m_compression ), numChannels, out );

	// prepare frame addresses
	context.m_firstDataPtr = context.GetDataPtr( data.m_dataAddrFallback, firstCompDataSize, firstCompData, secondCompData );

	// uncompress frame
	data.UncompressFrame( context.m_numChannelsCorrected, context.m_singleDataSize, context.m_firstDataPtr, context.m_out );
}

void SAnimationBufferBitwiseCompressedData::UncompressFallbackQuaternionXYZSignedW( const SAnimationBufferBitwiseCompressedData & data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float* out )
{
	SAnimationBufferBitwiseUncompressionContext context( SizeOfSingleData( (SAnimationBufferBitwiseCompression)data.m_compression ), numChannels, out );

	// we actually have one less channel
	context.m_numChannelsCorrected = numChannels - 1;
	context.m_frameOffset = 1; // for signed W if not stored in last bit

	// prepare frame addresses
	context.m_firstDataPtr = context.GetDataPtr( data.m_dataAddrFallback, firstCompDataSize, firstCompData, secondCompData );

	// uncompress frame
	data.UncompressFrameQuaternionXYZSignedW( context.m_singleDataSize, context.m_firstDataPtr, context.m_out );
}

void SAnimationBufferBitwiseCompressedData::UncompressFallbackQuaternionXYZSignedWInLastBit( const SAnimationBufferBitwiseCompressedData & data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float* out )
{
	SAnimationBufferBitwiseUncompressionContext context( SizeOfSingleData( (SAnimationBufferBitwiseCompression)data.m_compression ), numChannels, out );

	// we actually have one less channel
	context.m_numChannelsCorrected = numChannels - 1;

	// prepare frame addresses
	context.m_firstDataPtr = context.GetDataPtr( data.m_dataAddrFallback, firstCompDataSize, firstCompData, secondCompData );

	// uncompress frame
	data.UncompressFrameQuaternionXYZSignedWInLastBit( context.m_singleDataSize, context.m_firstDataPtr, context.m_out );
}

void SAnimationBufferBitwiseCompressedData::UncompressFallbackQuaternion64bXYZW( const SAnimationBufferBitwiseCompressedData & data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float* out )
{
	SAnimationBufferBitwiseUncompressionContext context( 8 /*64b*/, 1 /*quat*/, out );

	// prepare frame addresses
	context.m_firstDataPtr = context.GetDataPtr( data.m_dataAddrFallback, firstCompDataSize, firstCompData, secondCompData );

	// uncompress frame
	data.UncompressFrameQuaternion64bXYZW( context.m_singleDataSize, context.m_firstDataPtr, context.m_out );

	// ASSERT( ! Red::Math::NumericalUtils::IsNan( *out ) && ! Red::Math::NumericalUtils::IsNan( *(out + 1) ) && ! Red::Math::NumericalUtils::IsNan( *(out + 2) ) && ! Red::Math::NumericalUtils::IsNan( *(out + 3) ) );
	// ASSERT( Red::Math::NumericalUtils::IsFinite( *out ) && Red::Math::NumericalUtils::IsFinite( *(out + 1) ) && Red::Math::NumericalUtils::IsFinite( *(out + 2) ) && Red::Math::NumericalUtils::IsFinite( *(out + 3) ) );
}

void SAnimationBufferBitwiseCompressedData::UncompressFallbackQuaternion48bXYZ( const SAnimationBufferBitwiseCompressedData & data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float* out )
{
	SAnimationBufferBitwiseUncompressionContext context( 6 /*48b*/, 1 /*quat*/, out );

	// prepare frame addresses
	context.m_firstDataPtr = context.GetDataPtr( data.m_dataAddrFallback, firstCompDataSize, firstCompData, secondCompData );

	// uncompress frame
	data.UncompressFrameQuaternion48bXYZ( context.m_singleDataSize, context.m_firstDataPtr, context.m_out );
}

void SAnimationBufferBitwiseCompressedData::UncompressFallbackQuaternion48bXYZW( const SAnimationBufferBitwiseCompressedData & data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float* out )
{
	SAnimationBufferBitwiseUncompressionContext context( 6 /*48b*/, 1 /*quat*/, out );

	// prepare frame addresses
	context.m_firstDataPtr = context.GetDataPtr( data.m_dataAddrFallback, firstCompDataSize, firstCompData, secondCompData );

	// uncompress frame
	data.UncompressFrameQuaternion48bXYZW( context.m_singleDataSize, context.m_firstDataPtr, context.m_out );
}

void SAnimationBufferBitwiseCompressedData::UncompressFallbackQuaternion40bXYZ( const SAnimationBufferBitwiseCompressedData & data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float* out )
{
	SAnimationBufferBitwiseUncompressionContext context( 5 /*40b*/, 1 /*quat*/, out );

	// prepare frame addresses
	context.m_firstDataPtr = context.GetDataPtr( data.m_dataAddrFallback, firstCompDataSize, firstCompData, secondCompData );

	// uncompress frame
	data.UncompressFrameQuaternion40bXYZ( context.m_singleDataSize, context.m_firstDataPtr, context.m_out );
}

void SAnimationBufferBitwiseCompressedData::UncompressFallbackQuaternion40bXYZW( const SAnimationBufferBitwiseCompressedData & data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float* out )
{
	SAnimationBufferBitwiseUncompressionContext context( 5 /*40b*/, 1 /*quat*/, out );

	// prepare frame addresses
	context.m_firstDataPtr = context.GetDataPtr( data.m_dataAddrFallback, firstCompDataSize, firstCompData, secondCompData );

	// uncompress frame
	data.UncompressFrameQuaternion40bXYZW( context.m_singleDataSize, context.m_firstDataPtr, context.m_out );
}

void SAnimationBufferBitwiseCompressedData::UncompressFallbackQuaternion32bXYZ( const SAnimationBufferBitwiseCompressedData & data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float* out )
{
	SAnimationBufferBitwiseUncompressionContext context( 4 /*32b*/, 1 /*quat*/, out );

	// prepare frame addresses
	context.m_firstDataPtr = context.GetDataPtr( data.m_dataAddrFallback, firstCompDataSize, firstCompData, secondCompData );

	// uncompress frame
	data.UncompressFrameQuaternion32bXYZ( context.m_singleDataSize, context.m_firstDataPtr, context.m_out );
}

void SAnimationBufferBitwiseCompressedData::UncompressFrameWorker( Uint32 numChannelsCorrected, Uint32 singleDataSize, const Int8*& dataPtr, Float*& out ) const
{
	UncompressSingleDataStream( dataPtr, out, singleDataSize, numChannelsCorrected, (SAnimationBufferBitwiseCompression)m_compression );
}

void SAnimationBufferBitwiseCompressedData::UncompressFrame( Uint32 numChannelsCorrected, Uint32 singleDataSize, const Int8* dataPtr, Float* out ) const
{
	UncompressFrameWorker( numChannelsCorrected, singleDataSize, dataPtr, out );
}

void SAnimationBufferBitwiseCompressedData::UncompressFrameQuaternionXYZSignedW( Uint32 singleDataSize, const Int8* dataPtr, Float* out ) const
{
	// first, uncompress frame in normal manner
	UncompressFrameWorker( 3, singleDataSize, dataPtr, out );

	// sort out W component
	// easy peasy, just read last byte and treat it as sign of W
	*out = *dataPtr ? -1.0f : 1.0f;
	++ dataPtr;

	CalculateWofQuaternion( out );
}

void SAnimationBufferBitwiseCompressedData::UncompressFrameQuaternionXYZSignedWInLastBit( Uint32 singleDataSize, const Int8* dataPtr, Float* out ) const
{
	// first, uncompress frame in normal manner
	UncompressFrameWorker( 3, singleDataSize, dataPtr, out );

	// sort out W component
	// get to last byte
#ifdef RED_ENDIAN_LITTLE
	Int32 lessSignificantByteOffset = singleDataSize;
#else
	Int32 lessSignificantByteOffset = 1;
#endif
	dataPtr -= lessSignificantByteOffset;
	// get last bit and treat it as sign of W
	*out = *dataPtr & 0x01 ? -1.0f : 1.0f;
	dataPtr += lessSignificantByteOffset;

	CalculateWofQuaternion( out );
}

void SAnimationBufferBitwiseCompressedData::UncompressFrameQuaternion64bXYZW( Uint32 singleDataSize, const Int8* dataPtr, Float* out ) const
{
	Uint16 values[4];
	Red::System::MemoryCopy( values, dataPtr, 8 );

	RedFloat1 constant( 2.0f / 65535.0f );
	RedFloat1 half( 0.5f );
	RedFloat1 one( 1.0f );
	RedVector4 vector( (Float)values[0], (Float)values[1], (Float)values[2], (Float)values[3] );
	RedVector4 result;

	Add( result, vector, half );
	Mul( result, result, constant );
	Sub( result, result, one );

	result.Normalize4();
	result.Store( out );
}

void SAnimationBufferBitwiseCompressedData::UncompressFrameQuaternion48bXYZ( Uint32 singleDataSize, const Int8* dataPtr, Float* out ) const
{
	Uint16 values[3];
	Red::System::MemoryCopy( values, dataPtr, 6 );

	// 16bit gives 65536, which means we should use 65535.0f to divide
	*(out) = ( ((Float)(values[0]) + 0.5f) * 2.0f / 65535.0f ) - 1.0f;
	*(++out) = ( ((Float)(values[1]) + 0.5f) * 2.0f / 65535.0f ) - 1.0f;
	// 15bit (last bit is sign of W) gives 32768, which means we should use 32767.0f to divide
	*(++out) = ( ((Float)(values[2] >> 1) + 0.5f) * 2.0f / 32767.0f ) - 1.0f;
	// last bit
	*(++out) = values[2] & 0x0001 ? -1.0f : 1.0f;	

	CalculateWofQuaternion( out );
}

void SAnimationBufferBitwiseCompressedData::UncompressFrameQuaternion48bXYZW( Uint32 singleDataSize, const Int8* dataPtr, Float* out ) const
{
	Uint8 valuesData[6];
	Red::System::MemoryCopy( valuesData, dataPtr, 6 );

	// unpack from 6 bytes
	// 0          1          2          3          4		  5
	// ba98 7654  3210 ba98  7654 3210  ba98 7654  3210 ba98  7654 3210
	// XXXX XXXX  XXXX YYYY  YYYY YYYY  ZZZZ ZZZZ  ZZZZ WWWW  WWWW WWWW
	RED_ALIGNED_VAR( Float, 16 ) values[4];
	values[0] = (Float)(( ( ( (Uint16)valuesData[0] ) ) << 4  ) | ( ( ( (Uint16)valuesData[1] ) & 0xf0 ) >> 4  ) ) ;
	values[1] = (Float)(( ( ( (Uint16)valuesData[1] ) & 0x0f ) << 8  ) | ( ( ( (Uint16)valuesData[2] ) ) ) );
	values[2] = (Float)(( ( ( (Uint16)valuesData[3] ) ) << 4  ) | ( ( ( (Uint16)valuesData[4] ) & 0xf0 ) >> 4  ) ) ;
	values[3] = (Float)(( ( ( (Uint16)valuesData[4] ) & 0x0f ) << 8  ) | ( ( ( (Uint16)valuesData[5] ) ) ) );

	// 12bit gives 4096, which means we should use 4095.0f to multiply
	const RedFloat1 constant( 2.0f / 4095.0f );
	const RedFloat1 one( 1.0 );
	const RedFloat1 half( 0.5f );
	RedVector4 vector( values );
	RedVector4 result;

	Add( result, vector, half );
	Mul( result, result, constant );
	Sub( result, result, one );

	result.Normalized4();
	result.Store( out );
}

void SAnimationBufferBitwiseCompressedData::UncompressFrameQuaternion40bXYZ( Uint32 singleDataSize, const Int8* dataPtr, Float* out ) const
{
	Uint8 valuesData[5];
	Red::System::MemoryCopy( valuesData, dataPtr, 5 );

	// unpack from 5 bytes
	// 0          1          2          3          4
	// cba9 8765  4321 0cba  9876 5432  10cb a987  6543 2100
	// XXXX XXXX  XXXX XYYY  YYYY YYYY  YYZZ ZZZZ  ZZZZ ZZZW
	Uint16 values[3]; // do not uncompress last bit
	values[0] = ( ( ( (Uint16)valuesData[0] )        ) << 5  ) |
				( ( ( (Uint16)valuesData[1] ) & 0xf8 ) >> 3  ) ;
	values[1] = ( ( ( (Uint16)valuesData[1] ) & 0x07 ) << 10 ) |
				( ( ( (Uint16)valuesData[2] )        ) << 2  ) |
				( ( ( (Uint16)valuesData[3] ) & 0xc0 ) >> 6  ) ;
	values[2] = ( ( ( (Uint16)valuesData[3] ) & 0x3f ) << 7  ) |
				( ( ( (Uint16)valuesData[4] ) & 0xfe ) >> 1  ) ;

	// 13bit gives 8192, which means we should use 8191.0f to multiply
	const float constant = 2.0f / 8191.0f;
	*(out) = ( ((Float)(values[0]) + 0.5f) * constant ) - 1.0f;
	*(++out) = ( ((Float)(values[1]) + 0.5f) * constant ) - 1.0f;
	*(++out) = ( ((Float)(values[2]) + 0.5f) * constant ) - 1.0f;
	// last bit
	*(++out) = valuesData[4] & 0x01 ? -1.0f : 1.0f;

	CalculateWofQuaternion( out );
}

void SAnimationBufferBitwiseCompressedData::UncompressFrameQuaternion40bXYZW( Uint32 singleDataSize, const Int8* dataPtr, Float* out ) const
{
	Uint8 valuesData[5];
	Red::System::MemoryCopy( valuesData, dataPtr, 5 );

	// unpack from 5 bytes
	// 0          1          2          3          4
	// 9876 5432  1098 7654  3210 9876  5432 1098  7654 3210
	// XXXX XXXX  XXYY YYYY  YYYY ZZZZ  ZZZZ ZZWW  WWWW WWWW
	Uint16 values[4];
	values[0] = ( ( ( (Uint16)valuesData[0] )        ) << 2  ) |
				( ( ( (Uint16)valuesData[1] ) & 0xc0 ) >> 6  ) ;
	values[1] = ( ( ( (Uint16)valuesData[1] ) & 0x3f ) << 4  ) |
				( ( ( (Uint16)valuesData[2] ) & 0xf0 ) >> 4  ) ;
	values[2] = ( ( ( (Uint16)valuesData[2] ) & 0x0f ) << 6  ) |
				( ( ( (Uint16)valuesData[3] ) & 0xfc ) >> 2  ) ;
	values[3] = ( ( ( (Uint16)valuesData[3] ) & 0x03 ) << 8  ) |
				( ( ( (Uint16)valuesData[4] )        )       ) ;

	// 10bit gives 1024, which means we should use 1023.0f to multiply
	*(out) = ( ((Float)(values[0]) + 0.5f) * 2.0f / 1023.0f ) - 1.0f;
	*(++out) = ( ((Float)(values[1]) + 0.5f) * 2.0f / 1023.0f ) - 1.0f;
	*(++out) = ( ((Float)(values[2]) + 0.5f) * 2.0f / 1023.0f ) - 1.0f;
	*(++out)     = ( ((Float)(values[3]) + 0.5f) * 2.0f / 1023.0f ) - 1.0f;

	NormalizeQuaternion( out );
}

void SAnimationBufferBitwiseCompressedData::UncompressFrameQuaternion32bXYZ( Uint32 singleDataSize, const Int8* dataPtr, Float* out ) const
{
	Uint8 valuesData[4];
	Red::System::MemoryCopy( valuesData, dataPtr, 4 );

	// unpack from 4 bytes
	// 0          1          2          3         
	// 9876 5432  1098 7654  3210 a987  6543 2100 
	// XXXX XXXX  XXYY YYYY  YYYY ZZZZ  ZZZZ ZZZW 
	Uint16 values[3]; // do not uncompress last bit
	values[0] = ( ( ( (Uint16)valuesData[0] )        ) << 2 ) |
				( ( ( (Uint16)valuesData[1] ) & 0xc0 ) >> 6 ) ;
	values[1] = ( ( ( (Uint16)valuesData[1] ) & 0x3f ) << 4 ) |
				( ( ( (Uint16)valuesData[2] ) & 0xf0 ) >> 4 ) ;
	values[2] = ( ( ( (Uint16)valuesData[2] ) & 0x0f ) << 7 ) |
				( ( ( (Uint16)valuesData[3] ) & 0xfe ) >> 1 ) ;

	// 10bit gives 1024, which means we should use 1023.0f to multiply
	*(out) = ( ((Float)(values[0]) + 0.5f) * 2.0f / 1023.0f ) - 1.0f;
	*(++out) = ( ((Float)(values[1]) + 0.5f) * 2.0f / 1023.0f ) - 1.0f;
	// 11bit gives 2048, which means we should use 2047.0f to multiply
	*(++out) = ( ((Float)(values[2]) + 0.5f) * 2.0f / 2047.0f ) - 1.0f;
	// last bit
	*(++out) = valuesData[3] & 0x01 ? -1.0f : 1.0f;

	CalculateWofQuaternion( out );
}

void SAnimationBufferBitwiseCompressedData::CalculateWofQuaternion( Float* out ) const
{
	// calculate W as rest of normal
	const Float x = *( out - 3 );
	const Float y = *( out - 2 );
	const Float z = *( out - 1 );
	Float sqLengthXYZ = Min( x * x + y * y + z * z, 1.0f );
	*out *= MSqrt( 1.0f - sqLengthXYZ );
}

void SAnimationBufferBitwiseCompressedData::NormalizeQuaternion( Float* out ) const
{
	// calculate W as rest of normal
	Float& x = *( out - 3 );
	Float& y = *( out - 2 );
	Float& z = *( out - 1 );
	Float& w = *( out );
	const Float sqLengthXYZW = x * x + y * y + z * z + w * w;
	//ASSERT( sqLengthXYZW != 0.0f );
	const Float invLengthXYZW = 1.0f / MSqrt( sqLengthXYZW );
	x *= invLengthXYZW;
	y *= invLengthXYZW;
	z *= invLengthXYZW;
	w *= invLengthXYZW;
}

Bool SAnimationBufferBitwiseCompressedData::Compress( CAnimationBufferBitwiseCompressed* forAnimBuffer, Uint32 numChannels, Uint32 numFrames, Float dt, Float* rawData, Uint32 & outSingleCompressedDataSize, Float tolerance, Float frameSkipTolerance, SAnimationBufferDataCompressionMethod method )
{
	m_numFrames = (Uint16)numFrames;
	// try most trivial case - check if all raw data is the same and change to constant (well, within given tolerance)
	if ( m_numFrames > 1 && CheckIfCanCompressAsConstant( numChannels, rawData, tolerance, method ) )
	{
		m_numFrames = 1;
	}

	Bool retVal = false;
	AnimationDataBuffer tempData;
	Float tempDt = dt;
	Uint32 tempNumFrames = m_numFrames;
	if ( method == ABDCM_Quaternion64bW || method == ABDCM_Quaternion48bW || method ==  ABDCM_Quaternion40bW ||  
		 method == ABDCM_Quaternion48b  || method == ABDCM_Quaternion40b  || method ==  ABDCM_Quaternion32b )
	{
		// always assume that tolerance is ok
		retVal = TryToCompressUsingCompression( numChannels, dt, rawData, tolerance, frameSkipTolerance, tempData, outSingleCompressedDataSize, tempDt, tempNumFrames, ABBC_None, method );
		m_dt = tempDt;
		m_numFrames = (Uint16)tempNumFrames;
		m_compression = ABBC_None;
	}
	// check if 16b coding will keep us within tolerance
	else if ( TryToCompressUsingCompression( numChannels, dt, rawData, tolerance, frameSkipTolerance, tempData, outSingleCompressedDataSize, tempDt, tempNumFrames, ABBC_16b, method ) )
	{
		retVal = true;
		m_dt = tempDt;
		m_numFrames = (Uint16)tempNumFrames;
		m_compression = ABBC_16b;
	}
	// check if 24b coding will keep us within tolerance
	else if ( TryToCompressUsingCompression( numChannels, dt, rawData, tolerance, frameSkipTolerance, tempData, outSingleCompressedDataSize, tempDt, tempNumFrames, ABBC_24b, method ) )
	{
		retVal = true;
		m_dt = tempDt;
		m_numFrames = (Uint16)tempNumFrames;
		m_compression = ABBC_24b;
	}
	else
	{
		// store pure floats
		retVal = TryToCompressUsingCompression( numChannels, dt, rawData, tolerance, frameSkipTolerance, tempData, outSingleCompressedDataSize, tempDt, tempNumFrames, ABBC_None, method );
		m_dt = tempDt;
		m_numFrames = (Uint16)tempNumFrames;
		m_compression = ABBC_None;
	}
	Uint32 foundAtAddr;
	if ( FindWithinExistingData( tempData.TypedData(), tempData.Size(), forAnimBuffer->m_data, foundAtAddr ) )
	{
		m_dataAddr = foundAtAddr;
	}
	else
	{
		m_dataAddr = forAnimBuffer->m_data.Size();
		forAnimBuffer->m_data.PushBack( tempData );
	}
	return retVal;
}

Bool SAnimationBufferBitwiseCompressedData::FindWithinExistingData( const Int8 * newData, Uint32 newDataSize, const AnimationDataBuffer& alreadyCompressedData, Uint32 & outFoundAtAddr )
{
	if ( newDataSize <= alreadyCompressedData.Size() )
	{
		// check our data doubles with something already added - we care about raw data! it might be anything, as long as it is the same, reuse it!
		Uint32 checkUpTo = alreadyCompressedData.Size() - newDataSize;
		for ( Uint32 existingDataAddr = 0; existingDataAddr < checkUpTo; ++ existingDataAddr )
		{
			if ( CheckIfDataTheSame( newData, newDataSize, alreadyCompressedData, existingDataAddr ) )
			{
				outFoundAtAddr = existingDataAddr;
				return true;
			}
		}
	}
	return false;
}

Bool SAnimationBufferBitwiseCompressedData::CheckIfDataTheSame( const Int8 * newData, Uint32 newDataSize, const AnimationDataBuffer& alreadyCompressedData, Uint32 alreadyCompressedDataAddr )
{
	ASSERT( newDataSize + alreadyCompressedDataAddr <= alreadyCompressedData.Size() );
	const Int8* iExisting = &alreadyCompressedData[ alreadyCompressedDataAddr ];
	for ( auto iNew = newData; iNew != newData + newDataSize; ++ iNew, ++ iExisting )
	{
		if ( *iNew != *iExisting )
		{
			// at least one was different, not the same
			return false;
		}
	}
	// they are the same!
	return true;
}

Bool SAnimationBufferBitwiseCompressedData::CheckIfCanCompressAsConstant( Uint32 numChannels, Float* rawData, Float tolerance, SAnimationBufferDataCompressionMethod method ) const
{
	// check if following frames are similar enough
	for ( Uint32 frame = 1; frame < m_numFrames; ++ frame )
	{
		for ( Uint32 ch = 0; ch < numChannels; ++ ch )
		{
			if ( ! MatchesTolerance( rawData[ numChannels * frame + ch ], rawData[ ch ], tolerance ) )
			{
				return false;
			}
		}
	}
	return true;
}

Bool SAnimationBufferBitwiseCompressedData::TryToCompressUsingCompression( Uint32 numChannels, Float dt, Float* rawData, Float tolerance, Float frameSkipTolerance, AnimationDataBuffer& outData, Uint32 & outSingleCompressedDataSize, Float& outDt, Uint32& outNumFrames, SAnimationBufferBitwiseCompression compression, SAnimationBufferDataCompressionMethod method )
{
	Uint32 useFrameSkip = 0;
	Uint32 numChannelsCorrected = method == ABDCM_QuaternionXYZSignedW || method == ABDCM_QuaternionXYZSignedWLastBit || 
								  method == ABDCM_Quaternion64bW || method == ABDCM_Quaternion48bW || method == ABDCM_Quaternion40bW || 
								  method == ABDCM_Quaternion48b  || method == ABDCM_Quaternion40b  || method == ABDCM_Quaternion32b ? numChannels - 1 : numChannels;
	RED_UNUSED( numChannelsCorrected );
	for ( Uint32 frameSkip = 1; frameSkip < m_numFrames; ++ frameSkip )
	{
		Float maxDiff = 0.0f;
		for ( Uint32 frame = 0; frame < (Uint32)m_numFrames - 1; ++ frame )
		{
			if ( frame % ( frameSkip + 1 ) )
			{
				Uint32 nextFrame = Min<Uint32>( frame + frameSkip + 1, m_numFrames - 1 );
				Float *curRawStart = &rawData[ numChannels * frame ];
				Float *nextRawStart = &rawData[ numChannels * nextFrame ];
				for ( Uint32 ibFrame = 1; ibFrame < frameSkip + 1; ++ ibFrame )
				{
					Float *ibRaw = &rawData[ numChannels * ( frame + ibFrame ) ];
					Float *curRaw = curRawStart;
					Float *nextRaw = nextRawStart;
					Float ibAt = (Float)ibFrame / (Float)( frameSkip + 1 );
					for ( Uint32 ch = 0; ch < numChannels; ++ ch, ++ ibRaw, ++ curRaw, ++ nextRaw )
					{
						Float shouldBe = *curRaw * ( 1.0f - ibAt ) + ibAt * *nextRaw;
						maxDiff = Max( shouldBe - *ibRaw, maxDiff );
					}
				}
			}
		}
		if ( maxDiff < frameSkipTolerance && frameSkip < 5 ) // don't skip more than "n" frames
		{
			useFrameSkip = frameSkip;
		}
		else
		{
			break;
		}
	}
	return TryToCompressUsingCompressionWithFrameSkip( numChannels, dt, useFrameSkip, rawData, tolerance, outData, outSingleCompressedDataSize, outDt, outNumFrames, compression, method );
}

Bool SAnimationBufferBitwiseCompressedData::TryToCompressUsingCompressionWithFrameSkip( Uint32 numChannels, Float dt, Uint32 frameSkip, Float* rawData, Float tolerance, AnimationDataBuffer& outData, Uint32 & outSingleCompressedDataSize, Float& outDt, Uint32& outNumFrames, SAnimationBufferBitwiseCompression compression, SAnimationBufferDataCompressionMethod method )
{
	outDt = dt * ( (Float)frameSkip + 1.0f );

	if ( method == ABDCM_Quaternion64bW )
	{
		Uint32 frameSize = 8 /*64b*/;
		outSingleCompressedDataSize = frameSize;
		outData.Resize( m_numFrames * frameSize );

		Int8* dataPtr = outData.TypedData();
		Float* rawPtr = rawData;
		outNumFrames = 0;
		for ( Uint32 frame = 0; frame < (Uint32)m_numFrames; ++ frame )
		{
#ifdef DEBUG_BITWISE_COMPRESSION
			LOG_ENGINE(TXT(" >>  in %.6f %.6f %.6f %.6f [%i]"), *(rawPtr), *(rawPtr + 1), *(rawPtr + 2), *(rawPtr + 3), frame);
#endif
			Int8* frameDataPtr = dataPtr;
			Uint16 values[4];
			// 16bit gives 65536, which means we should use 65535.0f to multiply
			values[0] = (Uint16) Clamp( ( ( *(rawPtr ++) + 1.0f ) / 2.0f ) * 65535.0f + 0.5f, 0.0f, 65535.0f );
			values[1] = (Uint16) Clamp( ( ( *(rawPtr ++) + 1.0f ) / 2.0f ) * 65535.0f + 0.5f, 0.0f, 65535.0f );
			values[2] = (Uint16) Clamp( ( ( *(rawPtr ++) + 1.0f ) / 2.0f ) * 65535.0f + 0.5f, 0.0f, 65535.0f );
			values[3] = (Uint16) Clamp( ( ( *(rawPtr ++) + 1.0f ) / 2.0f ) * 65535.0f + 0.5f, 0.0f, 65535.0f );
			// store
			Red::System::MemoryCopy( dataPtr, values, frameSize );
#ifdef DEBUG_BITWISE_COMPRESSION
			{
				Float tbl[4];
				Float* out = tbl;
				// 16bit gives 65536, which means we should use 65535.0f to divide
				*(out++) = ( ((Float)(values[0]) + 0.5f) * 2.0f / 65535.0f ) - 1.0f;
				*(out++) = ( ((Float)(values[1]) + 0.5f) * 2.0f / 65535.0f ) - 1.0f;
				*(out++) = ( ((Float)(values[2]) + 0.5f) * 2.0f / 65535.0f ) - 1.0f;
				*out     = ( ((Float)(values[3]) + 0.5f) * 2.0f / 65535.0f ) - 1.0f;

				LOG_ENGINE(TXT(" >> mid %.6f %.6f %.6f %.6f"), tbl[0], tbl[1], tbl[2], tbl[3]);
				NormalizeQuaternion( out );
				LOG_ENGINE(TXT(" >> out %.6f %.6f %.6f %.6f"), tbl[0], tbl[1], tbl[2], tbl[3]);
			}
#endif

			dataPtr += frameSize;
			if ( frameSkip == 0 || ( frame % ( frameSkip + 1 ) ) == 0 || frame == (Uint32)m_numFrames - 1 )
			{
				++ outNumFrames;
			}
			else
			{
				// allow processing of whole data as we want to go through raw data, just get back here to overwrite it
				dataPtr = frameDataPtr;
			}
		}
		outData.Resize( outNumFrames * frameSize );
		return true;
	}
	else if ( method == ABDCM_Quaternion48b )
	{
		Uint32 frameSize = 6 /*48b*/;
		outSingleCompressedDataSize = frameSize;
		outData.Resize( m_numFrames * frameSize );

		Int8* dataPtr = outData.TypedData();
		Float* rawPtr = rawData;
		outNumFrames = 0;
		for ( Uint32 frame = 0; frame < (Uint32)m_numFrames; ++ frame )
		{
#ifdef DEBUG_BITWISE_COMPRESSION
			LOG_ENGINE(TXT(" >>  in %.6f %.6f %.6f %.6f [%i]"), *(rawPtr), *(rawPtr + 1), *(rawPtr + 2), *(rawPtr + 3), frame);
#endif
			Int8* frameDataPtr = dataPtr;
			Uint16 values[3];
			// 16bit gives 65536, which means we should use 65535.0f to multiply
			values[0] = (Uint16) Clamp( ( ( *(rawPtr ++) + 1.0f ) / 2.0f ) * 65535.0f + 0.5f, 0.0f, 65535.0f );
			values[1] = (Uint16) Clamp( ( ( *(rawPtr ++) + 1.0f ) / 2.0f ) * 65535.0f + 0.5f, 0.0f, 65535.0f );
			// 15bit (last bit is sign of W) gives 32768, which means we should use 32767.0f to multiply
			values[2] = (Uint16) Clamp( ( ( *(rawPtr ++) + 1.0f ) / 2.0f ) * 32767.0f + 0.5f, 0.0f, 32767.0f ) << 1;
			// last bit
			values[2] = ( values[2] & 0xfffe ) | ( *(rawPtr ++) >= 0.0f? 0x0000 : 0x0001 );
			// store
			Red::System::MemoryCopy( dataPtr, values, frameSize );
#ifdef DEBUG_BITWISE_COMPRESSION
			{
				Float tbl[4];
				Float* out = tbl;
				// 16bit gives 65536, which means we should use 65535.0f to divide
				*(out++) = ( ((Float)(values[0]) + 0.5f) * 2.0f / 65535.0f ) - 1.0f;
				*(out++) = ( ((Float)(values[1]) + 0.5f) * 2.0f / 65535.0f ) - 1.0f;
				// 15bit (last bit is sign of W) gives 32768, which means we should use 32767.0f to divide
				*(out++) = ( ((Float)(values[2] >> 1) + 0.5f) * 2.0f / 32767.0f ) - 1.0f;
				// last bit
				*out = values[2] & 0x0001 ? -1.0f : 1.0f;

				LOG_ENGINE(TXT(" >> mid %.6f %.6f %.6f %.6f"), tbl[0], tbl[1], tbl[2], tbl[3]);
				CalculateWofQuaternion( out );
				LOG_ENGINE(TXT(" >> out %.6f %.6f %.6f %.6f"), tbl[0], tbl[1], tbl[2], tbl[3]);
			}
#endif

			dataPtr += frameSize;
			if ( frameSkip == 0 || ( frame % ( frameSkip + 1 ) ) == 0 || frame == (Uint32)m_numFrames - 1 )
			{
				++ outNumFrames;
			}
			else
			{
				// allow processing of whole data as we want to go through raw data, just get back here to overwrite it
				dataPtr = frameDataPtr;
			}
		}
		outData.Resize( outNumFrames * frameSize );
		return true;
	}
	else if ( method == ABDCM_Quaternion48bW )
	{
		Uint32 frameSize = 6 /*48b*/;
		outSingleCompressedDataSize = frameSize;
		outData.Resize( m_numFrames * frameSize );

		Int8* dataPtr = outData.TypedData();
		Float* rawPtr = rawData;
		outNumFrames = 0;
		for ( Uint32 frame = 0; frame < (Uint32)m_numFrames; ++ frame )
		{
#ifdef DEBUG_BITWISE_COMPRESSION
			LOG_ENGINE(TXT(" >>  in %.6f %.6f %.6f %.6f [%i]"), *(rawPtr), *(rawPtr + 1), *(rawPtr + 2), *(rawPtr + 3), frame);
#endif
			Int8* frameDataPtr = dataPtr;
			Uint16 valuesRaw[4];
			// 12bit gives 4096, which means we should use 4095.0f to multiply
			valuesRaw[0] = (Uint16) Clamp( ( ( *(rawPtr ++) + 1.0f ) / 2.0f ) * 4095.0f + 0.5f, 0.0f, 4095.0f );
			valuesRaw[1] = (Uint16) Clamp( ( ( *(rawPtr ++) + 1.0f ) / 2.0f ) * 4095.0f + 0.5f, 0.0f, 4095.0f );
			valuesRaw[2] = (Uint16) Clamp( ( ( *(rawPtr ++) + 1.0f ) / 2.0f ) * 4095.0f + 0.5f, 0.0f, 4095.0f );
			valuesRaw[3] = (Uint16) Clamp( ( ( *(rawPtr ++) + 1.0f ) / 2.0f ) * 4095.0f + 0.5f, 0.0f, 4095.0f );
			// pack into 6 bytes
			// 0          1          2          3          4		  5
			// ba98 7654  3210 ba98  7654 3210  ba98 7654  3210 ba98  7654 3210
			// XXXX XXXX  XXXX YYYY  YYYY YYYY  ZZZZ ZZZZ  ZZZZ WWWW  WWWW WWWW
			Uint8 values[6];
			values[0] = ( ( (Int8)( valuesRaw [0] >> 4 ) ) & 0xff ) ;
			values[1] = ( ( (Int8)( valuesRaw [0] << 4 ) ) & 0xf0 ) |
						( ( (Int8)( valuesRaw [1] >> 8 ) ) & 0x0f ) ;
			values[2] = ( ( (Int8)( valuesRaw [1]      ) ) & 0xff ) ;
			values[3] = ( ( (Int8)( valuesRaw [2] >> 4 ) ) & 0xff ) ;
			values[4] = ( ( (Int8)( valuesRaw [2] << 4 ) ) & 0xf0 ) |
						( ( (Int8)( valuesRaw [3] >> 8 ) ) & 0x0f ) ;
			values[5] = ( ( (Int8)( valuesRaw [3]      ) ) & 0xff ) ;
			// store
			Red::System::MemoryCopy( dataPtr, values, frameSize );
#ifdef DEBUG_BITWISE_COMPRESSION
			{
				Float tbl[4];
				Float* out = tbl;
				// ba98 7654  3210 ba98  7654 3210  ba98 7654  3210 ba98  7654 3210
				// XXXX XXXX  XXXX YYYY  YYYY YYYY  ZZZZ ZZZZ  ZZZZ WWWW  WWWW WWWW
				Uint16 valuesT[4];
				valuesT[0] = ( ( ( (Uint16)values[0] )        ) << 4  ) |
							 ( ( ( (Uint16)values[1] ) & 0xf0 ) >> 4  ) ;
				valuesT[1] = ( ( ( (Uint16)values[1] ) & 0x0f ) << 8  ) |
							 ( ( ( (Uint16)values[2] )        )       ) ;
				valuesT[2] = ( ( ( (Uint16)values[3] )        ) << 4  ) |
							 ( ( ( (Uint16)values[4] ) & 0xf0 ) >> 4  ) ;
				valuesT[3] = ( ( ( (Uint16)values[4] ) & 0x0f ) << 8  ) |
							 ( ( ( (Uint16)values[5] )        )       ) ;

				// 12bit gives 4096, which means we should use 4095.0f to multiply
				*(out++) = ( ((Float)(valuesT[0]) + 0.5f) * 2.0f / 4095.0f ) - 1.0f;
				*(out++) = ( ((Float)(valuesT[1]) + 0.5f) * 2.0f / 4095.0f ) - 1.0f;
				*(out++) = ( ((Float)(valuesT[2]) + 0.5f) * 2.0f / 4095.0f ) - 1.0f;
				*out     = ( ((Float)(valuesT[3]) + 0.5f) * 2.0f / 4095.0f ) - 1.0f;

				LOG_ENGINE(TXT(" >> mid %.6f %.6f %.6f %.6f"), tbl[0], tbl[1], tbl[2], tbl[3]);
				NormalizeQuaternion( out );
				LOG_ENGINE(TXT(" >> out %.6f %.6f %.6f %.6f"), tbl[0], tbl[1], tbl[2], tbl[3]);
			}
#endif

			dataPtr += frameSize;
			if ( frameSkip == 0 || ( frame % ( frameSkip + 1 ) ) == 0 || frame == (Uint32)m_numFrames - 1 )
			{
				++ outNumFrames;
			}
			else
			{
				// allow processing of whole data as we want to go through raw data, just get back here to overwrite it
				dataPtr = frameDataPtr;
			}
		}
		outData.Resize( outNumFrames * frameSize );
		return true;
	}
	else if ( method == ABDCM_Quaternion40b )
	{
		Uint32 frameSize = 5 /*40b*/;
		outSingleCompressedDataSize = frameSize;
		outData.Resize( m_numFrames * frameSize );

		Int8* dataPtr = outData.TypedData();
		Float* rawPtr = rawData;
		outNumFrames = 0;
		for ( Uint32 frame = 0; frame < (Uint32)m_numFrames; ++ frame )
		{
#ifdef DEBUG_BITWISE_COMPRESSION
			LOG_ENGINE(TXT(" >>  in %.6f %.6f %.6f %.6f [%i]"), *(rawPtr), *(rawPtr + 1), *(rawPtr + 2), *(rawPtr + 3), frame);
#endif
			Int8* frameDataPtr = dataPtr;
			Uint16 valuesRaw[4];
			// 13bit gives 8192, which means we should use 8191.0f to multiply
			valuesRaw[0] = (Uint16) Clamp( ( ( *(rawPtr ++) + 1.0f ) / 2.0f ) * 8191.0f + 0.5f, 0.0f, 8191.0f );
			valuesRaw[1] = (Uint16) Clamp( ( ( *(rawPtr ++) + 1.0f ) / 2.0f ) * 8191.0f + 0.5f, 0.0f, 8191.0f );
			valuesRaw[2] = (Uint16) Clamp( ( ( *(rawPtr ++) + 1.0f ) / 2.0f ) * 8191.0f + 0.5f, 0.0f, 8191.0f );
			valuesRaw[3] =  *(rawPtr ++) >= 0.0f? 0x0000 : 0x0001 ;
			// pack into 5 bytes
			// 0          1          2          3          4
			// cba9 8765  4321 0cba  9876 5432  10cb a987  6543 2100
			// XXXX XXXX  XXXX XYYY  YYYY YYYY  YYZZ ZZZZ  ZZZZ ZZZW
			Uint8 values[5];
			values[0] = ( ( (Int8)( valuesRaw [0] >> 5  ) ) & 0xff ) ;
			values[1] = ( ( (Int8)( valuesRaw [0] << 3  ) ) & 0xf8 ) |
						( ( (Int8)( valuesRaw [1] >> 10 ) ) & 0x07 ) ;
			values[2] = ( ( (Int8)( valuesRaw [1] >> 2  ) ) & 0xff ) ;
			values[3] = ( ( (Int8)( valuesRaw [1] << 6  ) ) & 0xc0 ) |
						( ( (Int8)( valuesRaw [2] >> 7  ) ) & 0x3f ) ;
			values[4] = ( ( (Int8)( valuesRaw [2] << 1  ) ) & 0xfe ) |
						( ( (Int8)( valuesRaw [3]       ) ) & 0x01 ) ;
			// store
			Red::System::MemoryCopy( dataPtr, values, frameSize );
#ifdef DEBUG_BITWISE_COMPRESSION
			{
				Float tbl[4];
				Float* out = tbl;
				// unpack from 5 bytes
				// 0          1          2          3          4
				// cba9 8765  4321 0cba  9876 5432  10cb a987  6543 2100
				// XXXX XXXX  XXXX XYYY  YYYY YYYY  YYZZ ZZZZ  ZZZZ ZZZW
				Uint16 valuesT[3]; // do not uncompress last bit
				valuesT[0] = ( ( ( (Uint16)values[0] )        ) << 5  ) |
							 ( ( ( (Uint16)values[1] ) & 0xf8 ) >> 3  ) ;
				valuesT[1] = ( ( ( (Uint16)values[1] ) & 0x07 ) << 10 ) |
							 ( ( ( (Uint16)values[2] )        ) << 2  ) |
							 ( ( ( (Uint16)values[3] ) & 0xc0 ) >> 6  ) ;
				valuesT[2] = ( ( ( (Uint16)values[3] ) & 0x3f ) << 7  ) |
							 ( ( ( (Uint16)values[4] ) & 0xfe ) >> 1  ) ;

				// 13bit gives 8192, which means we should use 8191.0f to multiply
				*(out++) = ( ((Float)(valuesT[0]) + 0.5f) * 2.0f / 8191.0f ) - 1.0f;
				*(out++) = ( ((Float)(valuesT[1]) + 0.5f) * 2.0f / 8191.0f ) - 1.0f;
				*(out++) = ( ((Float)(valuesT[2]) + 0.5f) * 2.0f / 8191.0f ) - 1.0f;
				// last bit
				*out = values[4] & 0x01 ? -1.0f : 1.0f;

				LOG_ENGINE(TXT(" >> mid %.6f %.6f %.6f %.6f"), tbl[0], tbl[1], tbl[2], tbl[3]);
				CalculateWofQuaternion( out );
				LOG_ENGINE(TXT(" >> out %.6f %.6f %.6f %.6f"), tbl[0], tbl[1], tbl[2], tbl[3]);
			}
#endif

			dataPtr += frameSize;
			if ( frameSkip == 0 || ( frame % ( frameSkip + 1 ) ) == 0 || frame == (Uint32)m_numFrames - 1 )
			{
				++ outNumFrames;
			}
			else
			{
				// allow processing of whole data as we want to go through raw data, just get back here to overwrite it
				dataPtr = frameDataPtr;
			}
		}
		outData.Resize( outNumFrames * frameSize );
		return true;
	}
	else if ( method == ABDCM_Quaternion40bW )
	{
		Uint32 frameSize = 5 /*40b*/;
		outSingleCompressedDataSize = frameSize;
		outData.Resize( m_numFrames * frameSize );

		Int8* dataPtr = outData.TypedData();
		Float* rawPtr = rawData;
		outNumFrames = 0;
		for ( Uint32 frame = 0; frame < (Uint32)m_numFrames; ++ frame )
		{
#ifdef DEBUG_BITWISE_COMPRESSION
			LOG_ENGINE(TXT(" >>  in %.6f %.6f %.6f %.6f [%i]"), *(rawPtr), *(rawPtr + 1), *(rawPtr + 2), *(rawPtr + 3), frame);
#endif
			Int8* frameDataPtr = dataPtr;
			Uint16 valuesRaw[4];
			// 10bit gives 1024, which means we should use 1023.0f to multiply
			valuesRaw[0] = (Uint16) Clamp( ( ( *(rawPtr ++) + 1.0f ) / 2.0f ) * 1023.0f + 0.5f, 0.0f, 1023.0f );
			valuesRaw[1] = (Uint16) Clamp( ( ( *(rawPtr ++) + 1.0f ) / 2.0f ) * 1023.0f + 0.5f, 0.0f, 1023.0f );
			valuesRaw[2] = (Uint16) Clamp( ( ( *(rawPtr ++) + 1.0f ) / 2.0f ) * 1023.0f + 0.5f, 0.0f, 1023.0f );
			valuesRaw[3] = (Uint16) Clamp( ( ( *(rawPtr ++) + 1.0f ) / 2.0f ) * 1023.0f + 0.5f, 0.0f, 1023.0f );
			// pack into 5 bytes
			// 0          1          2          3          4
			// 9876 5432  1098 7654  3210 9876  5432 1098  7654 3210
			// XXXX XXXX  XXYY YYYY  YYYY ZZZZ  ZZZZ ZZWW  WWWW WWWW
			Uint8 values[5];
			values[0] = ( ( (Int8)( valuesRaw [0] >> 2  ) ) & 0xff ) ;
			values[1] = ( ( (Int8)( valuesRaw [0] << 6  ) ) & 0xc0 ) |
						( ( (Int8)( valuesRaw [1] >> 4  ) ) & 0x3f ) ;
			values[2] = ( ( (Int8)( valuesRaw [1] << 4  ) ) & 0xf0 ) |
						( ( (Int8)( valuesRaw [2] >> 6  ) ) & 0x0f ) ;
			values[3] = ( ( (Int8)( valuesRaw [2] << 2  ) ) & 0xfc ) |
						( ( (Int8)( valuesRaw [3] >> 8  ) ) & 0x03 ) ;
			values[4] = ( ( (Int8)( valuesRaw [3]       ) ) & 0xff ) ;
			// store
			Red::System::MemoryCopy( dataPtr, values, frameSize );
#ifdef DEBUG_BITWISE_COMPRESSION
			{
				Float tbl[4];
				Float* out = tbl;
				Uint16 valuesT[4];
				// unpack from 5 bytes
				// 0          1          2          3          4
				// cba9 8765  4321 0cba  9876 5432  10cb a987  6543 2100
				// XXXX XXXX  XXXX XYYY  YYYY YYYY  YYZZ ZZZZ  ZZZZ ZZZW
				valuesT[0] = ( ( ( (Uint16)values[0] )        ) << 2  ) |
							 ( ( ( (Uint16)values[1] ) & 0xc0 ) >> 6  ) ;
				valuesT[1] = ( ( ( (Uint16)values[1] ) & 0x3f ) << 4  ) |
							 ( ( ( (Uint16)values[2] ) & 0xf0 ) >> 4  ) ;
				valuesT[2] = ( ( ( (Uint16)values[2] ) & 0x0f ) << 6  ) |
							 ( ( ( (Uint16)values[3] ) & 0xfc ) >> 2  ) ;
				valuesT[3] = ( ( ( (Uint16)values[3] ) & 0x03 ) << 8  ) |
							 ( ( ( (Uint16)values[4] )        )       ) ;

				// 10bit gives 1024, which means we should use 1023.0f to multiply
				*(out++) = ( ((Float)(valuesT[0]) + 0.5f) * 2.0f / 1023.0f ) - 1.0f;
				*(out++) = ( ((Float)(valuesT[1]) + 0.5f) * 2.0f / 1023.0f ) - 1.0f;
				*(out++) = ( ((Float)(valuesT[2]) + 0.5f) * 2.0f / 1023.0f ) - 1.0f;
				*out     = ( ((Float)(valuesT[3]) + 0.5f) * 2.0f / 1023.0f ) - 1.0f;

				LOG_ENGINE(TXT(" >> mid %.6f %.6f %.6f %.6f"), tbl[0], tbl[1], tbl[2], tbl[3]);
				NormalizeQuaternion( out );
				LOG_ENGINE(TXT(" >> out %.6f %.6f %.6f %.6f"), tbl[0], tbl[1], tbl[2], tbl[3]);
			}
#endif

			dataPtr += frameSize;
			if ( frameSkip == 0 || ( frame % ( frameSkip + 1 ) ) == 0 || frame == (Uint32)m_numFrames - 1 )
			{
				++ outNumFrames;
			}
			else
			{
				// allow processing of whole data as we want to go through raw data, just get back here to overwrite it
				dataPtr = frameDataPtr;
			}
		}
		outData.Resize( outNumFrames * frameSize );
		return true;
	}
	else if ( method == ABDCM_Quaternion32b )
	{
		Uint32 frameSize = 4 /*32b*/;
		outSingleCompressedDataSize = frameSize;
		outData.Resize( m_numFrames * frameSize );

		Int8* dataPtr = outData.TypedData();
		Float* rawPtr = rawData;
		outNumFrames = 0;
		for ( Uint32 frame = 0; frame < (Uint32)m_numFrames; ++ frame )
		{
#ifdef DEBUG_BITWISE_COMPRESSION
			LOG_ENGINE(TXT(" >>  in %.6f %.6f %.6f %.6f [%i]"), *(rawPtr), *(rawPtr + 1), *(rawPtr + 2), *(rawPtr + 3), frame);
#endif
			Int8* frameDataPtr = dataPtr;
			Uint16 valuesRaw[4];
			// 10bit gives 1024, which means we should use 1023.0f to multiply
			valuesRaw[0] = (Uint16) Clamp( ( ( *(rawPtr ++) + 1.0f ) / 2.0f ) * 1023.0f + 0.5f, 0.0f, 1023.0f );
			valuesRaw[1] = (Uint16) Clamp( ( ( *(rawPtr ++) + 1.0f ) / 2.0f ) * 1023.0f + 0.5f, 0.0f, 1023.0f );
			// 11bit gives 2048, which means we should use 2047.0f to multiply
			valuesRaw[2] = (Uint16) Clamp( ( ( *(rawPtr ++) + 1.0f ) / 2.0f ) * 2047.0f + 0.5f, 0.0f, 2047.0f );
			valuesRaw[3] =  *(rawPtr ++) >= 0.0f? 0x0000 : 0x0001 ;
			// pack into 4 bytes
			// 0          1          2          3         
			// 9876 5432  1098 7654  3210 9876  5432 10-0 
			// XXXX XXXX  XXYY YYYY  YYYY ZZZZ  ZZZZ ZZuW 
			Uint8 values[4];
			values[0] = ( ( (Int8)( valuesRaw [0] >> 2 ) ) & 0xff ) ;
			values[1] = ( ( (Int8)( valuesRaw [0] << 6 ) ) & 0xc0 ) |
						( ( (Int8)( valuesRaw [1] >> 4 ) ) & 0x3f ) ;
			values[2] = ( ( (Int8)( valuesRaw [1] << 4 ) ) & 0xf0 ) |
						( ( (Int8)( valuesRaw [2] >> 7 ) ) & 0x0f ) ;
			values[3] = ( ( (Int8)( valuesRaw [2] << 1 ) ) & 0xfe ) |
						( ( (Int8)( valuesRaw [3]      ) ) & 0x01 ) ;
			// store
			Red::System::MemoryCopy( dataPtr, values, frameSize );
#ifdef DEBUG_BITWISE_COMPRESSION
			{
				Float tbl[4];
				Float* out = tbl;
				Uint16 valuesT[3]; // do not uncompress last bit
				valuesT[0] = ( ( ( (Uint16)values[0] )        ) << 2 ) |
							 ( ( ( (Uint16)values[1] ) & 0xc0 ) >> 6 ) ;
				valuesT[1] = ( ( ( (Uint16)values[1] ) & 0x3f ) << 4 ) |
							 ( ( ( (Uint16)values[2] ) & 0xf0 ) >> 4 ) ;
				valuesT[2] = ( ( ( (Uint16)values[2] ) & 0x0f ) << 7 ) |
							 ( ( ( (Uint16)values[3] ) & 0xfe ) >> 1 ) ;
				 
				// 10bit gives 1024, which means we should use 1023.0f to multiply
				*(out++) = ( ((Float)(valuesT[0]) + 0.5f) * 2.0f / 1023.0f ) - 1.0f;
				*(out++) = ( ((Float)(valuesT[1]) + 0.5f) * 2.0f / 1023.0f ) - 1.0f;
				// 11bit gives 2048, which means we should use 2047.0f to multiply
				*(out++) = ( ((Float)(valuesT[2]) + 0.5f) * 2.0f / 2047.0f ) - 1.0f;
				// last bit
				*out = values[3] & 0x01 ? -1.0f : 1.0f;

				LOG_ENGINE(TXT(" >> mid %.6f %.6f %.6f %.6f"), tbl[0], tbl[1], tbl[2], tbl[3]);
				CalculateWofQuaternion( out );
				LOG_ENGINE(TXT(" >> out %.6f %.6f %.6f %.6f"), tbl[0], tbl[1], tbl[2], tbl[3]);
			}
#endif

			dataPtr += frameSize;
			if ( frameSkip == 0 || ( frame % ( frameSkip + 1 ) ) == 0 || frame == (Uint32)m_numFrames - 1 )
			{
				++ outNumFrames;
			}
			else
			{
				// allow processing of whole data as we want to go through raw data, just get back here to overwrite it
				dataPtr = frameDataPtr;
			}
		}
		outData.Resize( outNumFrames * frameSize );
		return true;
	}
	else
	{
		Uint32 singleDataSize = SizeOfSingleData( compression );
		Uint32 numChannelsCorrected = method == ABDCM_QuaternionXYZSignedW || method == ABDCM_QuaternionXYZSignedWLastBit ? numChannels - 1 : numChannels;
		Uint32 frameSize = numChannelsCorrected * singleDataSize
			+ ( method == ABDCM_QuaternionXYZSignedW ? 1 : 0 )
			;
		outSingleCompressedDataSize = frameSize;
		// we will resize it after we have definite number of frames stored
		outData.Resize( m_numFrames * frameSize );

		Bool allMatchTolerance = true;
		Int8* dataPtr = outData.TypedData();
		Float* rawPtr = rawData;
		outNumFrames = 0;
		for ( Uint32 frame = 0; frame < (Uint32)m_numFrames; ++ frame )
		{
#ifdef DEBUG_BITWISE_COMPRESSION
			LOG_ENGINE(TXT(" >>  in %.6f %.6f %.6f %.6f [%i]"), *(rawPtr), *(rawPtr + 1), *(rawPtr + 2), *(rawPtr + 3), frame);
#endif
			Int8* frameDataPtr = dataPtr;
			for ( Uint32 ch = 0; ch < numChannels; ++ ch )
			{
				if ( ch == numChannels - 1 && ( method == ABDCM_QuaternionXYZSignedW || method == ABDCM_QuaternionXYZSignedWLastBit ) )
				{
					if ( method == ABDCM_QuaternionXYZSignedWLastBit )
					{
#ifdef RED_ENDIAN_LITTLE
						Int32 lessSignificantByteOffset = singleDataSize;
#else
						Int32 lessSignificantByteOffset = 1;
#endif
						dataPtr -= lessSignificantByteOffset;
						// store last one as sign in last bit of less significant byte
						*dataPtr = ( *dataPtr & 0xFE ) | ( *rawPtr >= 0.0f ? 0 : 1 );
						dataPtr += lessSignificantByteOffset;
						++ rawPtr;
					}
					else
					{
						*dataPtr = ( *rawPtr >= 0.0f ? 0 : 1 );
						++ rawPtr;
						++ dataPtr;
					}
				}
				else
				{
					Float compressed = CompressSingleData( *rawPtr, dataPtr, compression );
					allMatchTolerance &= MatchesTolerance( compressed, *rawPtr, tolerance );
					rawPtr ++;
					dataPtr += singleDataSize;
				}
			}
			if ( frameSkip == 0 || ( frame % ( frameSkip + 1 ) ) == 0 || frame == (Uint32)m_numFrames - 1 )
			{
				++ outNumFrames;
			}
			else
			{
				// allow processing of whole data as we want to go through raw data, just get back here to overwrite it
				dataPtr = frameDataPtr;
			}
#ifdef DEBUG_BITWISE_COMPRESSION
			{
				Float tbl[4];
				Float* out = tbl;
				Int8* dp = frameDataPtr;
				for ( Uint32 ch = 0; ch < 3; ++ ch )
				{
					*out = UncompressSingleData( dp, (SAnimationBufferBitwiseCompression)m_compression );
					++ out;
					dp += singleDataSize;
				}
				if ( method == ABDCM_Quaternion )
				{
					*out = UncompressSingleData( dp, (SAnimationBufferBitwiseCompression)m_compression );
					dp += singleDataSize;
				}
				if ( method == ABDCM_QuaternionXYZSignedW )
				{
					*out = *dp? -1.0f : 1.0f;
				}
				if ( method == ABDCM_QuaternionXYZSignedWLastBit )
				{
					// sort out W component
					// get to last byte
#ifdef RED_ENDIAN_LITTLE
					Int32 lessSignificantByteOffset = singleDataSize;
#else
					Int32 lessSignificantByteOffset = 1;
#endif
					dataPtr -= lessSignificantByteOffset;
					// get last bit and treat it as sign of W
					*out = *dataPtr & 0x01 ? -1.0f : 1.0f;
					dataPtr += lessSignificantByteOffset;
				}
				LOG_ENGINE(TXT(" >> mid %.6f %.6f %.6f %.6f"), tbl[0], tbl[1], tbl[2], tbl[3]);
				CalculateWofQuaternion( out );
				LOG_ENGINE(TXT(" >> out %.6f %.6f %.6f %.6f"), tbl[0], tbl[1], tbl[2], tbl[3]);
			}
#endif
		}
		outData.Resize( outNumFrames * frameSize );
		return allMatchTolerance;
	}
}


Uint32 SAnimationBufferBitwiseCompressedData::SizeOfSingleData( SAnimationBufferBitwiseCompression compression )
{
	// depend on this order to make it faster to calculate data
	//ASSERT( ABBC_None == 0 );
	//ASSERT( ABBC_24b == 1 );
	//ASSERT( ABBC_16b == 2 );
	return 4 - compression;
}

Float SAnimationBufferBitwiseCompressedData::CompressSingleData( Float value, Int8* where, SAnimationBufferBitwiseCompression compression )
{
	switch ( compression )
	{
	case ABBC_None:
		{
			Red::System::MemoryCopy( where, &value, sizeof(Float) );
			return value;
		}
	case ABBC_24b:
		{
			ASSERT( sizeof( Int32 ) == sizeof( Float ) );
			Float compressed = value;
			Int8* asInt8 = ((Int8*)(&compressed));
#ifdef RED_ENDIAN_LITTLE
			*(asInt8 ++) = 0;
			*(where ++) = *(asInt8 ++ );
			*(where ++) = *(asInt8 ++ );
			*(where ++) = *(asInt8 ++ );
#else
			*(where ++) = *(asInt8 ++ );
			*(where ++) = *(asInt8 ++ );
			*(where ++) = *(asInt8 ++ );
			*(asInt8 ++) = 0;
#endif
			return compressed;
		}
	case ABBC_16b:
		{
			ASSERT( sizeof( Int32 ) == sizeof( Float ) );
			Float compressed = value;
			Int8* asInt8 = ((Int8*)(&compressed));
#ifdef RED_ENDIAN_LITTLE
			*(asInt8 ++) = 0;
			*(asInt8 ++) = 0;
			*(where ++) = *(asInt8 ++ );
			*(where ++) = *(asInt8 ++ );
#else
			*(where ++) = *(asInt8 ++ );
			*(where ++) = *(asInt8 ++ );
			*(asInt8 ++) = 0;
			*(asInt8 ++) = 0;
#endif
			return compressed;
		}
	}
	ASSERT( false, TXT("Can't understand compression type") );
	return value;
}

const Uint32 maskOfValidCompressedData[3] = { 0xffffffff, 0x00ffffff, 0x0000ffff };
const Int32 shiftCompressedData[] = { 0, 8, 16 };

void SAnimationBufferBitwiseCompressedData::UncompressSingleDataStream( const Int8*& _from, Float*& out, Uint32 singleDataSize, Uint32 numDecompressions, SAnimationBufferBitwiseCompression compression )
{
	// few assumptions here:
	// those have to be in this order 
	static_assert( ABBC_None == 0, "enum wrong order" );
	static_assert( ABBC_24b == 1, "enum wrong order" );
	static_assert( ABBC_16b == 2, "enum wrong order" );
	static_assert( sizeof( Int32 ) == sizeof( Float ), " we depend on float being the same size as int32 - 4 bytes, we're dealing here with bitwise compression, right?" );

	for( Uint32 i = 0; i < numDecompressions; ++i )
	{
		const Int32* from = reinterpret_cast< const Int32* >(_from);
		Int32* asInt32 = reinterpret_cast< Int32* >(out++);

		*(asInt32) = *(from) & maskOfValidCompressedData[ compression ];

#ifdef RED_ENDIAN_LITTLE
		*asInt32 = *asInt32 << shiftCompressedData[ compression ];
#endif
		_from += singleDataSize;
	}
}

Bool SAnimationBufferBitwiseCompressedData::MatchesTolerance( Float value, Float other, Float tolerance )
{
	return Abs( value - other ) <= tolerance;
}

///////////////////////////////////////////////////////////////////////////////

CAnimationBufferBitwiseCompressed::CAnimationBufferBitwiseCompressed()
	: m_version( 0 )
#ifndef NO_EDITOR
	, m_compressionPreset( ABBCP_NormalQuality )
	, m_sourceDataSize( 0 )
#endif
	, m_numFrames( 0 )
	, m_duration( 1.f )
	, m_dt( 0.033f )
	, m_ringIndex( INDEX_NONE )
	, m_streamingOption( ABSO_NonStreamable )
	, m_nonStreamableBones( 0 )
{
#ifndef NO_EDITOR
	m_compressionSettings.UsePreset( m_compressionPreset );
#endif
	m_deferredData.SetMemoryClass( MC_BufferAnimation );
}

CAnimationBufferBitwiseCompressed::~CAnimationBufferBitwiseCompressed()
{
	// unload it if loaded
	SBitwiseCompressionDataRingBuffer::Unload(this);
}

void CAnimationBufferBitwiseCompressed::OnPropertyPostChange( IProperty* prop )
{
	IAnimationBuffer::OnPropertyPostChange( prop );

#ifndef NO_EDITOR
	if ( prop->GetName() == TXT("compressionPreset") )
	{
		m_compressionSettings.UsePreset( m_compressionPreset );
	}
	if ( prop->GetName() == TXT("compressionSettings") )
	{
		m_compressionPreset = ABBCP_Custom;
	}
#endif
}

void CAnimationBufferBitwiseCompressed::AppendDeferredToData()
{
	if ( m_deferredData.GetSize() > 0 )
	{
		AnimationDeferredDataBufferHandle bufferHandle = m_deferredData.AcquireBufferHandleSync();
		Uint32 oldDataSize = m_data.Size();
		// make place for deferred data
		m_data.Grow(m_deferredData.GetSize());
		// copy from deferred to the end of data
		Red::System::MemoryCopy( &m_data[oldDataSize], bufferHandle.Get()->GetData(), bufferHandle.Get()->GetSize() );
		// and clear deferred data
		m_deferredData.SetBufferContent( m_data.TypedData(), 0 );
	}
}

void CAnimationBufferBitwiseCompressed::MoveDataToDeferred( Uint32 startAt )
{
	if ( m_data.Size() > startAt )
	{
		// copy to deferred data starting at "startAt"
		m_deferredData.SetBufferContent( &m_data[startAt], m_data.Size() - startAt );
		// and leave everything before "startAt"
		m_data.Resize( startAt );
	}
}

Uint32 CAnimationBufferBitwiseCompressed::GetFramesNum() const
{
	return m_numFrames;
}

Uint32 CAnimationBufferBitwiseCompressed::GetBonesNum() const
{
	return m_bones.Size();
}

Uint32 CAnimationBufferBitwiseCompressed::GetTracksNum() const
{
	return m_tracks.Size();
}

Uint32 CAnimationBufferBitwiseCompressed::GetPartsNum() const
{
	return 1;
}

const IAnimationBuffer * CAnimationBufferBitwiseCompressed::GetPart(Uint32 partIndex) const
{
	return partIndex == 0? this : nullptr;
}

#ifndef NO_EDITOR
void CAnimationBufferBitwiseCompressed::GetPartiallyStreamableMemoryStats(Uint32 & outNonStreamable, Uint32 & outPartiallyStreamable) const
{
	outNonStreamable = (Uint32)m_data.DataSize();
	outNonStreamable += (Uint32)m_fallbackData.DataSize();

	outPartiallyStreamable = m_deferredData.GetSize();
}
#endif

void CAnimationBufferBitwiseCompressed::GetMemorySize(Uint32 & outNonStreamable, Uint32 & outStreamableLoaded, Uint32 & outStreamableTotal, Uint32 & outSourceDataSize) const
{
	outNonStreamable = sizeof(CAnimationBufferBitwiseCompressed);
	outNonStreamable += (Uint32)m_bones.DataSize();
	outNonStreamable += (Uint32)m_tracks.DataSize();
	outNonStreamable += (Uint32)m_data.DataSize();
	outNonStreamable += (Uint32)m_fallbackData.DataSize();
	outStreamableLoaded = SBitwiseCompressionDataRingBuffer::GetMemorySize( this );
	outStreamableTotal = m_deferredData.GetSize();
#ifndef NO_EDITOR
	outSourceDataSize = m_sourceDataSize;
#else
	outSourceDataSize = 0;
#endif
}

Bool CAnimationBufferBitwiseCompressed::IsCompressed() const
{
	return true;
}

namespace
{
	void FillBonePositionBuffer( const IAnimationBuffer::SourceData::Part& data, Uint32 numBones, Uint32 boneIdx, TDynArray< Float >& buf )
	{
		for ( Uint32 i=0; i<data.m_numFrames; ++i )
		{
			const RedQsTransform& boneTransform = data.m_bones[ i*numBones+boneIdx ];

			buf[ i*3+0 ] = boneTransform.Translation.X;
			buf[ i*3+1 ] = boneTransform.Translation.Y;
			buf[ i*3+2 ] = boneTransform.Translation.Z;
		}
	}

	void FillBoneRotationBuffer( const IAnimationBuffer::SourceData::Part& data, Uint32 numBones, Uint32 boneIdx, TDynArray< Float >& buf )
	{
		for ( Uint32 i=0; i<data.m_numFrames; ++i )
		{
			const RedQsTransform& boneTransform = data.m_bones[ i*numBones+boneIdx ];

			buf[ i*4+0 ] = boneTransform.Rotation.Quat.X;
			buf[ i*4+1 ] = boneTransform.Rotation.Quat.Y;
			buf[ i*4+2 ] = boneTransform.Rotation.Quat.Z;
			buf[ i*4+3 ] = boneTransform.Rotation.Quat.W;
		}
	}

	void FillBoneScaleBuffer( const IAnimationBuffer::SourceData::Part& data, Uint32 numBones, Uint32 boneIdx, TDynArray< Float >& buf )
	{
		for ( Uint32 i=0; i<data.m_numFrames; ++i )
		{
			const RedQsTransform& boneTransform = data.m_bones[ i*numBones+boneIdx ];

			buf[ i*3+0 ] = boneTransform.Scale.X;
			buf[ i*3+1 ] = boneTransform.Scale.Y;
			buf[ i*3+2 ] = boneTransform.Scale.Z;
		}
	}

	void FillTrackBuffer( const IAnimationBuffer::SourceData::Part& data, Uint32 numTracks, Uint32 trackIdx, TDynArray< Float >& buf )
	{
		for ( Uint32 i=0; i<data.m_numFrames; ++i )
		{
			buf[ i ] = data.m_track[ i*numTracks+trackIdx ];
		}
	}

}

Bool CAnimationBufferBitwiseCompressed::Initialize( const CSkeletalAnimationSet* animSet, CSkeletalAnimation* animation, const SourceData& sourceData, IAnimationBuffer* previousAnimBuffer, Bool preferBetterQuality )
{
#ifdef NO_EDITOR
	ASSERT( false, TXT("You shouldn't import animations without editor"));
	return false;
#else
	if ( sourceData.m_parts.Size() != 1 )
	{
		WARN_ENGINE( TXT("Test fail?") );
		return false;
	}

	const SourceData::Part & data = sourceData.m_parts[ 0 ];

	// make sure we have at least one part
	if ( data.m_numFrames == 0 )
	{
		WARN_ENGINE( TXT("There are no animation parts in animation source data. Unable to initialize.") );
		return false;
	}

	// Copy general data
	m_numFrames = data.m_numFrames;
	m_dt = sourceData.m_dT;
	m_duration = (Float)( m_numFrames - 1 ) * m_dt; // don't use total duration as we care about delta between frames and number of frames
#ifdef USE_REF_IK_MISSING_BONES_HACK
	m_hasRefIKBones = sourceData.m_hasRefIKBones;
#endif

	const Uint32 numBones = sourceData.m_numBones;
	const Uint32 numTracks = sourceData.m_numTracks;

	m_bones.Resize( numBones );
	m_tracks.Resize( numTracks );

	// clear data, fallback data and deferred data
	m_data.Clear();
	m_fallbackData.Clear();
	m_deferredData.Clear();

	{
		if ( preferBetterQuality )
		{
			// prefer raw when importing with higher quality, unless there's previous buffer to be used
			m_compressionPreset = ABBCP_VeryHighQuality;
			m_compressionSettings.UsePreset( ABBCP_VeryHighQuality );
		}
		Bool getSettingsFromPreviousBuffer = true;

		if ( animSet )
		{
			getSettingsFromPreviousBuffer = !animSet->ShouldOverrideBitwiseCompressionSettingsOnImport();
			if ( !getSettingsFromPreviousBuffer || ! preferBetterQuality ) // override will always use settings from animset
			{
				m_compressionPreset = animSet->GetBitwiseCompressionPreset();
				m_compressionSettings = animSet->GetBitwiseCompressionSettings();
			}
		}

		if ( ( getSettingsFromPreviousBuffer && ! preferBetterQuality ) || ( animation && animation->HasOwnBitwiseCompressionParams() ) )
		{
			if ( const CAnimationBufferBitwiseCompressed* previousBitwiseCompressed = Cast< CAnimationBufferBitwiseCompressed >( previousAnimBuffer ) )
			{
				m_compressionPreset = previousBitwiseCompressed->m_compressionPreset;
				m_compressionSettings = previousBitwiseCompressed->m_compressionSettings;
			}
			else if ( animation ) // use from skeletal animation
			{
				m_compressionPreset = animation->GetBitwiseCompressionPreset();
				m_compressionSettings = animation->GetBitwiseCompressionSettings();
			}
		}
	}

	// store in animation
	animation->m_bitwiseCompressionPreset = m_compressionPreset;
	animation->m_bitwiseCompressionSettings = m_compressionSettings;

	// streaming info
	SAnimationBufferBitwiseCompressionSettings settings = m_compressionSettings;

	// store current version of bitwise compression
	m_version = CURRENT_BITWISE_COMPRESSION_VERSION;

	TDynArray<SAnimationBufferBitwiseCompressionWorkerBoneInfo> compressionWorkerBoneInfo;
	compressionWorkerBoneInfo.Resize( m_bones.Size() );

	TDynArray<SAnimationBufferBitwiseCompressionWorkerTrackInfo> compressionWorkerTrackInfo;
	compressionWorkerTrackInfo.Resize( m_tracks.Size() );
	
	// Bones
	{
		// Position
		// --------

		// compression parameters
		Uint32 pos_numChannels = 3;
		Float pos_requestedTolerance = settings.m_translationTolerance;
		Float pos_frameSkipTolerance = settings.m_translationSkipFrameTolerance;
		//
		const Int32 pos_bufSize = pos_numChannels * m_numFrames;
		TDynArray< Float > pos_buf;
		pos_buf.Resize( pos_bufSize );

		// Rotation
		// --------

		// compression parameters
		SAnimationBufferDataCompressionMethod rot_method = ABDCM_Quaternion;
		m_orientationCompressionMethod = settings.m_orientationCompressionMethod;
		if ( settings.m_orientationCompressionMethod == ABOCM_AsFloat_XYZW )
		{
			rot_method = ABDCM_Quaternion;
		}
		if ( settings.m_orientationCompressionMethod == ABOCM_AsFloat_XYZSignedW )
		{
			rot_method = ABDCM_QuaternionXYZSignedW;
		}
		if ( settings.m_orientationCompressionMethod == ABOCM_AsFloat_XYZSignedWInLastBit )
		{
			rot_method = ABDCM_QuaternionXYZSignedWLastBit;
		}
		if ( settings.m_orientationCompressionMethod == ABOCM_PackIn64bitsW )
		{
			rot_method = ABDCM_Quaternion64bW;
		}
		if ( settings.m_orientationCompressionMethod == ABOCM_PackIn48bits )
		{
			rot_method = ABDCM_Quaternion48b;
		}
		if ( settings.m_orientationCompressionMethod == ABOCM_PackIn48bitsW )
		{
			rot_method = ABDCM_Quaternion48bW;
		}
		if ( settings.m_orientationCompressionMethod == ABOCM_PackIn40bits )
		{
			rot_method = ABDCM_Quaternion40b;
		}
		if ( settings.m_orientationCompressionMethod == ABOCM_PackIn40bitsW )
		{
			rot_method = ABDCM_Quaternion40bW;
		}
		if ( settings.m_orientationCompressionMethod == ABOCM_PackIn32bits )
		{
			rot_method = ABDCM_Quaternion32b;
		}
		Uint32 rot_numChannels = 4;
		Float rot_requestedTolerance = settings.m_orientationTolerance;
		Float rot_frameSkipTolerance = settings.m_orientationSkipFrameTolerance;
		//
		const Int32 rot_bufSize = rot_numChannels * m_numFrames;
		TDynArray< Float > rot_buf;
		rot_buf.Resize( rot_bufSize );

		// Scales
		// ------

		// compression parameters
		Uint32 sca_numChannels = 3;
		Float sca_requestedTolerance = settings.m_scaleTolerance;
		Float sca_frameSkipTolerance = settings.m_scaleSkipFrameTolerance;
		//
		const Int32 sca_bufSize = sca_numChannels * m_numFrames;
		TDynArray< Float > sca_buf;
		sca_buf.Resize( sca_bufSize );

		// Compress! interleaved
		for ( Uint32 i=0; i<numBones; ++i )
		{
			FillBonePositionBuffer( data, numBones, i, pos_buf );
			FillBoneRotationBuffer( data, numBones, i, rot_buf );
			FillBoneScaleBuffer( data, numBones, i, sca_buf );
			m_bones[ i ].m_position.Compress( this, pos_numChannels, m_numFrames, m_dt, pos_buf.TypedData(), compressionWorkerBoneInfo[i].m_positionSingleCompressedDataSize, pos_requestedTolerance, pos_frameSkipTolerance );
			m_bones[ i ].m_orientation.Compress( this, rot_numChannels, m_numFrames, m_dt, rot_buf.TypedData(), compressionWorkerBoneInfo[i].m_orientationSingleCompressedDataSize, rot_requestedTolerance, rot_frameSkipTolerance, rot_method );
#ifndef ANIM_COMPRESSION_SCALE_OPTS
			m_bones[ i ].m_scale.Compress( this, sca_numChannels, m_numFrames, m_dt, sca_buf.TypedData(), compressionWorkerBoneInfo[i].m_scaleSingleCompressedDataSize, sca_requestedTolerance, sca_frameSkipTolerance );
#endif
			compressionWorkerBoneInfo[ i ].m_requiresUpToDataSize = m_data.Size(); // requires this of data size
		}
	}

	// Tracks
	{
		// compression parameters
		Uint32 numChannels = 1;
		Float requestedTolerance = settings.m_trackTolerance;
		Float frameSkipTolerance = settings.m_trackSkipFrameTolerance;

		// Buf
		const Int32 bufSize = numChannels * m_numFrames;
		TDynArray< Float > buf;
		buf.Resize( bufSize );

		// Compress!
		for ( Uint32 i=0; i<numTracks; ++i )
		{
			FillTrackBuffer( data, numTracks, i, buf );

			m_tracks[ i ].Compress( this, numChannels, m_numFrames, m_dt, buf.TypedData(), compressionWorkerTrackInfo[i].m_singleCompressedDataSize, requestedTolerance, frameSkipTolerance );
		}
	}

	if ( animSet )
	{
		animSet->GetStreamingOption( m_streamingOption, m_nonStreamableBones );
	}

	// m_data is created, apply changes
	ApplyChangesToDataDueToStreamingAndFillFallback( compressionWorkerBoneInfo, compressionWorkerTrackInfo, animation, sourceData );

	return true;
#endif
}

void CAnimationBufferBitwiseCompressed::ApplyChangesToDataDueToStreamingAndFillFallback( const TDynArray<SAnimationBufferBitwiseCompressionWorkerBoneInfo> & compressionWorkerBoneInfo, const TDynArray<SAnimationBufferBitwiseCompressionWorkerTrackInfo> & compressionWorkerTrackInfo, CSkeletalAnimation* animation, const SourceData& sourceData )
{
	// create copy of whole data in one single buffer - might be needed for fallback
	AnimationDataBuffer wholeData;
	wholeData = m_data;

	if ( m_nonStreamableBones >= compressionWorkerBoneInfo.Size() )
	{
		// will switch to fully streamable
		m_nonStreamableBones = 0;
	}

	if ( m_streamingOption == ABSO_PartiallyStreamable && m_nonStreamableBones == 0 )
	{
		m_streamingOption = ABSO_FullyStreamable;
	}
	if ( m_streamingOption == ABSO_PartiallyStreamable && m_nonStreamableBones == m_bones.Size() )
	{
		m_streamingOption = ABSO_NonStreamable;
	}
	if ( m_streamingOption == ABSO_PartiallyStreamable && m_version < BITWISE_COMPRESSION_VERSION__INTERLEAVED )
	{
		RED_WARNING( m_streamingOption == ABSO_PartiallyStreamable && m_version < BITWISE_COMPRESSION_VERSION__INTERLEAVED, "Partially streamable not possible for non-interleaved animations. Recompress animation first!" );
		m_streamingOption = ABSO_NonStreamable;
	}	

	if ( m_streamingOption == ABSO_NonStreamable )
	{
		if ( m_deferredData.GetSize() > 0 )
		{
			AppendDeferredToData();
		}
		m_nonStreamableBones = m_bones.Size();
	}
	else if ( m_streamingOption == ABSO_FullyStreamable )
	{
		if ( ! m_data.Empty() )
		{
			AppendDeferredToData();
			MoveDataToDeferred();
		}
		m_nonStreamableBones = 0;
	}
	else if ( m_streamingOption == ABSO_PartiallyStreamable )
	{
		Uint32 breakDataAtAddr = compressionWorkerBoneInfo[m_nonStreamableBones - 1].m_requiresUpToDataSize;
		if ( breakDataAtAddr != 0 && m_data.Size() != breakDataAtAddr )
		{
			// we need to move it around
			// first move all to data
			AppendDeferredToData();
			MoveDataToDeferred( breakDataAtAddr );
		}
		// check if there is anything beyond, if no, mark it as nonstreamable
		if ( m_deferredData.GetSize() == 0 )
		{
			m_streamingOption = ABSO_NonStreamable;
			m_nonStreamableBones = m_bones.Size();
		}
	}

	// create fallback data
	for( Uint32 i = 0; i < m_bones.Size(); ++ i )
	{
		SAnimationBufferBitwiseCompressedBoneTrack & bone = m_bones[i];
		const SAnimationBufferBitwiseCompressionWorkerBoneInfo & cwbi = compressionWorkerBoneInfo[i];
		FindFallbackOrAdd( wholeData, bone.m_position, cwbi.m_positionSingleCompressedDataSize );
		FindFallbackOrAdd( wholeData, bone.m_orientation, cwbi.m_orientationSingleCompressedDataSize );
#ifndef ANIM_COMPRESSION_SCALE_OPTS
		FindFallbackOrAdd( wholeData, bone.m_scale, cwbi.m_scaleSingleCompressedDataSize );
#endif
	}

	for( Uint32 i = 0; i < m_tracks.Size(); ++ i )
	{
		SAnimationBufferBitwiseCompressedData & track = m_tracks[i];
		const SAnimationBufferBitwiseCompressionWorkerTrackInfo & cwti = compressionWorkerTrackInfo[i];
		FindFallbackOrAdd( wholeData, track, cwti.m_singleCompressedDataSize );
	}

	if ( m_fallbackData.Size() == m_deferredData.GetSize() && m_streamingOption != ABSO_NonStreamable )
	{
		// that makes no sense to hold deferred data - refert to non streamable
		AppendDeferredToData();
		m_fallbackData.Clear();
		m_nonStreamableBones = m_bones.Size();
		m_streamingOption = ABSO_NonStreamable;
	}

#ifndef NO_EDITOR
	m_sourceDataSize = sourceData.GetMemorySizeOfPart(0);
#endif

#ifdef RED_LOGGING_ENABLED
	Uint32 memorySize = (Uint32)sizeof(CAnimationBufferBitwiseCompressed) + (Uint32)m_bones.DataSize() + (Uint32)m_tracks.DataSize() + (Uint32)m_data.DataSize() + (Uint32)m_fallbackData.DataSize() + (Uint32)m_deferredData.GetSize();
	String partiallyInfo = String::Printf( TXT("prtly[%02i]"), m_nonStreamableBones );
	RED_LOG(AnimationCompressionDetailed, TXT("Compressed to %5.2f%% (nst:%5.2f%%) (raw:%10i B -> ttl:%8i B -> nst:%8i B) - %s - streaming %5.2f%% %8i bytes (ttl:%8i B = non:%8i B + flb:%8i B + str:%8i B) %s"),
		100.0f * ((Float)memorySize) / ((Float)sourceData.GetMemorySizeOfPart(0)),
		100.0f * ((Float)(memorySize - m_deferredData.GetSize())) / ((Float)sourceData.GetMemorySizeOfPart(0)),
		sourceData.GetMemorySizeOfPart(0),
		memorySize,
		memorySize - m_deferredData.GetSize(),
		m_streamingOption == ABSO_NonStreamable?	TXT("nonstream") :
		(m_streamingOption == ABSO_FullyStreamable? TXT("fullystrm") :
													partiallyInfo.AsChar() ),
		100.0f * ((Float)m_deferredData.GetSize()) / ((Float)(m_data.DataSize() + m_fallbackData.DataSize() + m_deferredData.GetSize())),
		m_deferredData.GetSize(),
		m_data.DataSize() + m_fallbackData.DataSize() + m_deferredData.GetSize(),
		m_data.DataSize(),
		m_fallbackData.DataSize(),
		m_deferredData.GetSize(),
		animation? animation->GetName().AsChar() : TXT("??"));
#endif
}

Uint64 CAnimationBufferBitwiseCompressed::GetDataCRC( Uint64 seed ) const
{
	Uint64 crc = seed;
	// basically, everything that is serialized
	crc = Red::CalculateHash64( &m_version, sizeof( m_version ), crc );
	crc = Red::CalculateHash64( m_bones.Data( ), m_bones.DataSize( ), crc );
	crc = Red::CalculateHash64( m_tracks.Data( ), m_tracks.DataSize( ), crc );
	crc = Red::CalculateHash64( m_data.Data( ), m_data.DataSize( ), crc );
	crc = Red::CalculateHash64( &m_orientationCompressionMethod, sizeof( m_orientationCompressionMethod ), crc );
	crc = Red::CalculateHash64( &m_duration, sizeof( m_duration ), crc );
	crc = Red::CalculateHash64( &m_numFrames, sizeof( m_numFrames ), crc );
	crc = Red::CalculateHash64( &m_dt, sizeof( m_dt ), crc );
	crc = Red::CalculateHash64( &m_streamingOption, sizeof( m_streamingOption ), crc );
	crc = Red::CalculateHash64( &m_nonStreamableBones, sizeof( m_nonStreamableBones ), crc );
#ifdef USE_REF_IK_MISSING_BONES_HACK
	crc = Red::CalculateHash64( &m_hasRefIKBones, sizeof( m_hasRefIKBones ), crc );
#endif
	return crc;
}

void CAnimationBufferBitwiseCompressed::FindFallbackOrAdd( const AnimationDataBuffer & wholeData, SAnimationBufferBitwiseCompressedData & compressedDataInfo, Uint32 singleCompressedDataSize )
{
	Uint32 foundAtAddr;
	if ( compressedDataInfo.m_dataAddr + singleCompressedDataSize <= m_data.Size() )
	{
		// is within m_data
		compressedDataInfo.m_dataAddrFallback = compressedDataInfo.m_dataAddr;
	}
	else if ( SAnimationBufferBitwiseCompressedData::FindWithinExistingData( &wholeData[compressedDataInfo.m_dataAddr], singleCompressedDataSize, m_fallbackData, foundAtAddr ) )
	{
		compressedDataInfo.m_dataAddrFallback = m_data.Size() + foundAtAddr;
	}
	else
	{
		size_t oldSize = m_fallbackData.Grow( (size_t)singleCompressedDataSize );
		Red::System::MemoryCopy( &m_fallbackData[ (Int32)oldSize ], &wholeData[compressedDataInfo.m_dataAddr], singleCompressedDataSize );
		compressedDataInfo.m_dataAddrFallback = m_data.Size() + (Uint32)oldSize;
	}
}

Bool CAnimationBufferBitwiseCompressed::Sample( const Float _frame, const Uint32 numBones, RedQsTransform* outTransforms, const Uint32 numTracks, Float* outValues, Bool fallbackOnly ) const
{
	const Float time = Clamp( _frame * m_dt, 0.f, m_duration );
	return LoadAtTime( time, numBones, outTransforms, numTracks, outValues, fallbackOnly );
}

Bool CAnimationBufferBitwiseCompressed::Load( const Uint32 frameIndex, const Uint32 numBones, RedQsTransform* outTransforms, const Uint32 numTracks, Float* outValues, Bool fallbackOnly ) const
{
	const Float time = Clamp( (Float)frameIndex * m_dt, 0.f, m_duration );
	return LoadAtTime( time, numBones, outTransforms, numTracks, outValues, fallbackOnly );
}

void CAnimationBufferBitwiseCompressed::DecompressUsingMethods( const Float time, const Uint32 numBones, const Uint32 numfallbackBones, RedQsTransform* outTransforms,
															    Uint32 firstCompDataSize, Uint32 firstCompDataSizeForFallback, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, AnimationConstDataPtr fallbackCompData,
																void (*UncompressOrientationMethod)( const SAnimationBufferBitwiseCompressedData &  data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float time, Float duration, Float* out ),
																void (*UncompressFallbackOrientationMethod)( const SAnimationBufferBitwiseCompressedData &  data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float* out ) ) const
{
	const Uint32 posDim = 3;
	const Uint32 rotDim = 4;
	const Uint32 scaleDim = 3;

	RedQsTransform* transform = outTransforms;
	Uint32 boneIdx = 0;
	AnimationBoneTrackArray::const_iterator iBone = m_bones.Begin();
	AnimationBoneTrackArray::const_iterator iEnd = m_bones.End();

	// read from first buffer
	for (; iBone != iEnd && boneIdx < numBones; ++ iBone, ++ transform, ++ boneIdx )
	{
		SAnimationBufferBitwiseCompressedData::Uncompress( iBone->m_position, firstCompDataSize, firstCompData, secondCompData, posDim, time, m_duration, &(transform->Translation.X) );
		UncompressOrientationMethod( iBone->m_orientation, firstCompDataSize, firstCompData, secondCompData, rotDim, time, m_duration, &(transform->Rotation.Quat.X) );

#ifndef ANIM_COMPRESSION_SCALE_OPTS
		SAnimationBufferBitwiseCompressedData::Uncompress( iBone->m_scale, firstCompDataSize, firstCompData, secondCompData, scaleDim, time, m_duration, &(transform->Scale.X) );
#else
		transform->Scale.SetOnes();
#endif

		transform->Translation.W = 1.f;
		transform->Scale.W = 1.f;

#ifdef DEBUG_CORRUPT_TRANSFORMS
		{
			RED_ASSERT( transform->Rotation.Quat.IsOk(), TXT("Animation rotation data appears to be corrupt!") );
			RED_ASSERT( transform->Translation.IsOk(), TXT("Animation translation data appears to be corrupt!") );
			RED_ASSERT( transform->Scale.IsOk(), TXT("Animation scale data appears to be corrupt!") );
		}

		if ( !transform->Scale.IsOk() )
		{
			transform->Scale.SetOnes();
		}
#endif
	}

	if ( fallbackCompData )
	{
		for (; iBone != iEnd && boneIdx < numfallbackBones; ++ iBone, ++ transform, ++ boneIdx )
		{
			SAnimationBufferBitwiseCompressedData::UncompressFallback( iBone->m_position, firstCompDataSizeForFallback, firstCompData, fallbackCompData, posDim, &(transform->Translation.X) );
			UncompressFallbackOrientationMethod( iBone->m_orientation, firstCompDataSizeForFallback, firstCompData, fallbackCompData, rotDim, &(transform->Rotation.Quat.X) );

#ifndef ANIM_COMPRESSION_SCALE_OPTS
			SAnimationBufferBitwiseCompressedData::UncompressFallback( iBone->m_scale, firstCompDataSizeForFallback, firstCompData, fallbackCompData, scaleDim, &(transform->Scale.X) );
#else
			transform->Scale.SetOnes();
#endif

			transform->Translation.W = 1.f;
			transform->Scale.W = 1.f;

#ifdef DEBUG_CORRUPT_TRANSFORMS
			{
				RED_ASSERT( transform->Rotation.Quat.IsOk(), TXT("Animation rotation data appears to be corrupt!") );
				RED_ASSERT( transform->Translation.IsOk(), TXT("Animation translation data appears to be corrupt!") );
				RED_ASSERT( transform->Scale.IsOk(), TXT("Animation scale data appears to be corrupt!") );

				if ( !transform->Scale.IsOk() )
				{
					transform->Scale.SetOnes();
				}
			}
#endif
		}
	}
}

Bool CAnimationBufferBitwiseCompressed::LoadAtTime( const Float time, const Uint32 numBones, RedQsTransform* outTransforms, const Uint32 numTracks, Float* outValues, Bool fallbackOnly ) const
{
	PC_SCOPE( AnimationBitwiseDecompression );

	Uint32 hasBones = 0;
	Uint32 hasFallbackBones = 0;
	Uint32 firstCompDataSize = 0;
	Uint32 firstCompDataSizeForFallback = 0;
	AnimationConstDataPtr firstCompData = nullptr;
	AnimationConstDataPtr secondCompData = nullptr;
	AnimationConstDataPtr fallbackCompData = nullptr;
	Bool floatTracksDataPresent = false;

	AnimationDeferredDataBufferHandle dataPtr;

	if ( m_streamingOption == ABSO_NonStreamable )
	{
		// everything is in non streamed buffer
		firstCompData = m_data.TypedData();
		firstCompDataSize = m_data.Size();
		floatTracksDataPresent = true;
		hasBones = Min( numBones, m_bones.Size() );
		hasFallbackBones = 0;
	}
	else if ( m_streamingOption == ABSO_FullyStreamable )
	{
		dataPtr = m_dataPtr.Lock();

		// everything is in streamed buffer (if it's streamed in)
		if ( dataPtr )
		{
			firstCompDataSize = m_deferredData.GetSize();
			firstCompData = static_cast< const Int8 * >( dataPtr->GetData() );
			floatTracksDataPresent = true;
			hasBones = Min( numBones, m_bones.Size() );
		}
		else
		{
			ASSERT(SBitwiseCompressionDataRingBuffer::IsThereDataBufferPtr(this), TXT("Animation didn't have usage set and is fully streamable."));
		}
		hasFallbackBones = Min( numBones, m_bones.Size() );
		fallbackCompData = m_fallbackData.TypedData();
		firstCompDataSizeForFallback = m_data.Size();
	}
	else if ( m_streamingOption == ABSO_PartiallyStreamable )
	{
		// everything is in non streamed buffer
		firstCompData = m_data.TypedData();
		firstCompDataSize = m_data.Size();
		hasBones = Min( numBones, m_nonStreamableBones );
		
		dataPtr = m_dataPtr.Lock();

		if ( dataPtr )
		{
			secondCompData = static_cast< const Int8 * >( dataPtr->GetData() );
			floatTracksDataPresent = true;
			hasBones = Min( numBones, m_bones.Size() );
		}
		else
		{
			ASSERT(SBitwiseCompressionDataRingBuffer::IsThereDataBufferPtr(this) || numBones <= hasBones, TXT("Animation didn't have usage set, is partially streamable and requires more bones than are always loaded."));
		}
		hasFallbackBones = Min( numBones, m_bones.Size() );
		fallbackCompData = m_fallbackData.TypedData();
		firstCompDataSizeForFallback = m_data.Size();
	}

	if ( firstCompData == nullptr && fallbackCompData == nullptr )
	{
		return false;
	}

	if ( fallbackOnly && hasFallbackBones )
	{
		floatTracksDataPresent = false;
		hasBones = 0;
	}

	// has to reflect order in SAnimationBufferOrientationCompressionMethod
	//	ABOCM_PackIn64bitsW,
	//	ABOCM_PackIn48bitsW,
	//	ABOCM_PackIn40bitsW,
	//	ABOCM_AsFloat_XYZW,
	//	ABOCM_AsFloat_XYZSignedW,
	//	ABOCM_AsFloat_XYZSignedWInLastBit,
	//	ABOCM_PackIn48bits,
	//	ABOCM_PackIn40bits,
	//	ABOCM_PackIn32bits,
	static UncompressOrientationFunction uncompressOrientation[] =
	{
		SAnimationBufferBitwiseCompressedData::UncompressQuaternion64bXYZW,
		SAnimationBufferBitwiseCompressedData::UncompressQuaternion48bXYZW,
		SAnimationBufferBitwiseCompressedData::UncompressQuaternion40bXYZW,
		SAnimationBufferBitwiseCompressedData::UncompressQuaternion,
		SAnimationBufferBitwiseCompressedData::UncompressQuaternionXYZSignedW,
		SAnimationBufferBitwiseCompressedData::UncompressQuaternionXYZSignedWInLastBit,
		SAnimationBufferBitwiseCompressedData::UncompressQuaternion48bXYZ,
		SAnimationBufferBitwiseCompressedData::UncompressQuaternion40bXYZ,
		SAnimationBufferBitwiseCompressedData::UncompressQuaternion32bXYZ,
	};
	static UncompressFallbackOrientationFunction uncompressFallboackOrientation[] =
	{
		SAnimationBufferBitwiseCompressedData::UncompressFallbackQuaternion64bXYZW,
		SAnimationBufferBitwiseCompressedData::UncompressFallbackQuaternion48bXYZW,
		SAnimationBufferBitwiseCompressedData::UncompressFallbackQuaternion40bXYZW,
		SAnimationBufferBitwiseCompressedData::UncompressFallbackQuaternion,
		SAnimationBufferBitwiseCompressedData::UncompressFallbackQuaternionXYZSignedW,
		SAnimationBufferBitwiseCompressedData::UncompressFallbackQuaternionXYZSignedWInLastBit,
		SAnimationBufferBitwiseCompressedData::UncompressFallbackQuaternion48bXYZ,
		SAnimationBufferBitwiseCompressedData::UncompressFallbackQuaternion40bXYZ,
		SAnimationBufferBitwiseCompressedData::UncompressFallbackQuaternion32bXYZ,
	};

	// yeah, I know, but to this like this to avoid branch for every bone and have one loop pass, maybe it would be slightly better to have to passes and separate orientation?
	DecompressUsingMethods( time, hasBones, hasFallbackBones, outTransforms, firstCompDataSize, firstCompDataSizeForFallback, firstCompData, secondCompData, fallbackCompData, uncompressOrientation[m_orientationCompressionMethod], uncompressFallboackOrientation[m_orientationCompressionMethod] );
	
	Float* trackValue = outValues;
	Uint32 trackIdx = 0;
	if ( floatTracksDataPresent || m_version >= BITWISE_COMPRESSION_VERSION__WITH_TRACK_FALLBACK )
	{
		Uint32 tracksLoaded = floatTracksDataPresent? numTracks : 0;
		AnimationTracksArray::const_iterator iTrack = m_tracks.Begin();
		AnimationTracksArray::const_iterator iEnd = m_tracks.End();
		for ( ; iTrack != iEnd && trackIdx < tracksLoaded; ++ iTrack, ++ trackValue )
		{
			SAnimationBufferBitwiseCompressedData::Uncompress( *iTrack, firstCompDataSize, firstCompData, secondCompData, 1, time, m_duration, trackValue );
		}

		if ( m_version >= BITWISE_COMPRESSION_VERSION__WITH_TRACK_FALLBACK )
		{
			for ( ; iTrack != iEnd && trackIdx < numTracks; ++ iTrack, ++ trackValue )
			{
				SAnimationBufferBitwiseCompressedData::UncompressFallback( *iTrack, firstCompDataSizeForFallback, firstCompData, fallbackCompData, 1, trackValue );
			}
		}
	}

	return true;
}

void CAnimationBufferBitwiseCompressed::SetCompressionParams( const SAnimationBufferBitwiseCompressionPreset & compressionPreset, const SAnimationBufferBitwiseCompressionSettings & compressionSettings )
{
#ifndef NO_EDITOR
	m_compressionPreset = compressionPreset;
	m_compressionSettings = compressionSettings;
#endif
}

Bool CAnimationBufferBitwiseCompressed::SetStreamingOption( SAnimationBufferStreamingOption streamingOption, Uint32 nonStreamableBones )
{
	Bool retVal = m_streamingOption != streamingOption || m_nonStreamableBones != nonStreamableBones;
	m_streamingOption = streamingOption;
	m_nonStreamableBones = nonStreamableBones;
	return retVal;
}

void CAnimationBufferBitwiseCompressed::Preload()
{
	// this will issue loading of data
	Uint32 bonesLoadedTemp;
	Uint32 bonesAlwaysLoadedTemp;
	GetAnimationBufferDataAvailableLoadIfNeeded( m_bones.Size(), bonesLoadedTemp, bonesAlwaysLoadedTemp, false );
}

void CAnimationBufferBitwiseCompressed::SyncLoad()
{
	// this will issue loading of data
	Uint32 bonesLoadedTemp;
	Uint32 bonesAlwaysLoadedTemp;
	GetAnimationBufferDataAvailableLoadIfNeeded( m_bones.Size(), bonesLoadedTemp, bonesAlwaysLoadedTemp, true );
}

EAnimationBufferDataAvailable CAnimationBufferBitwiseCompressed::GetAnimationBufferDataAvailable( Uint32 requestedBones, Uint32 & outBonesLoaded, Uint32 & outBonesAlwaysLoaded )
{
	return GetAnimationBufferDataAvailableLoadIfNeeded( requestedBones, outBonesLoaded, outBonesAlwaysLoaded );
}

EAnimationBufferDataAvailable CAnimationBufferBitwiseCompressed::GetAnimationBufferDataAvailableLoadIfNeeded( Uint32 requestedBones, Uint32 & outBonesLoaded, Uint32 & outBonesAlwaysLoaded, Bool loadSync )
{
	outBonesAlwaysLoaded = m_nonStreamableBones;

	if ( m_streamingOption == ABSO_NonStreamable)
	{
		outBonesLoaded = Min( requestedBones, m_bones.Size() );
		// no need to load
		return ABDA_All;
	}

	if ( ! m_dataPtr.Expired() )
	{
		// everything is still loaded in ring buffer
		outBonesLoaded = Min( requestedBones, m_bones.Size() );
		SBitwiseCompressionDataRingBuffer::Touch( this );
		return ABDA_All;
	}
	else
	{
		// maybe we don't need to load it?
		if ( requestedBones <= m_nonStreamableBones )
		{
			outBonesLoaded = Min( requestedBones, m_nonStreamableBones );
			return ABDA_All;
		}

		// update weak pointer, maybe not yet loaded
		if ( SBitwiseCompressionDataRingBuffer::GetDataBufferPtr( this, m_dataPtr ) )
		{
			AnimationDeferredDataBufferHandle dataPtr = m_dataPtr.Lock();
			if ( dataPtr )
			{
				// loaded right now
				outBonesLoaded = Min( requestedBones, m_bones.Size() );
				return ABDA_All;
			}
		}
		else
		{
			if ( requestedBones > 0 )
			{
				// load - if we actually request any bones
				m_ringIndex = SBitwiseCompressionDataRingBuffer::Load( this, loadSync );
				// check, because maybe we've got sync/instant load!
				if ( SBitwiseCompressionDataRingBuffer::GetDataBufferPtr( this, m_dataPtr ) )
				{
					AnimationDeferredDataBufferHandle dataPtr = m_dataPtr.Lock();
					if ( dataPtr )
					{
						// loaded right now
						outBonesLoaded = Min( requestedBones, m_bones.Size() );
						return ABDA_All;
					}
				}
			}
		}
	}

	if ( m_nonStreamableBones > 0 )
	{
		// partially loaded
		outBonesLoaded = Min( requestedBones, m_nonStreamableBones );
		return ABDA_Partial;
	}

	return ABDA_None;
}

void CAnimationBufferBitwiseCompressed::AddUsage( DEBUG_ANIMATION_USAGE_PARAM_LIST )
{
	if ( m_streamingOption == ABSO_NonStreamable )
	{
		// no need
		return;
	}
	Uint32 bonesLoaded;
	Uint32 bonesAlwaysLoaded;
	GetAnimationBufferDataAvailable( m_bones.Size(), bonesLoaded, bonesAlwaysLoaded ); // TODO what to do here? this will trigger streaming always and what we'd like to have is to stream only when requested (due to LOD etc) maybe make it possible to add usage without streaming?
	SBitwiseCompressionDataRingBuffer::AddUsage( this DEBUG_ANIMATION_USAGE_PARAM_NAME_CONT );
}

void CAnimationBufferBitwiseCompressed::ReleaseUsage()
{
	if ( m_streamingOption == ABSO_NonStreamable )
	{
		// no need
		return;
	}
	SBitwiseCompressionDataRingBuffer::ReleaseUsage( this );
}

void CAnimationBufferBitwiseCompressed::GetStreamingNumbers(Uint32 & outStreamedInAnimations, Uint32 & outUsedStreamedInAnimations, Uint32 & outAnimationsBeingStreamedIn)
{
	SBitwiseCompressionDataRingBuffer::GetStreamingNumbers(outStreamedInAnimations, outUsedStreamedInAnimations, outAnimationsBeingStreamedIn);
}

void CAnimationBufferBitwiseCompressed::GetLimitsForStreamedData(Uint32 & outStreamedAnimLimit, Uint32 & outMemoryLimit)
{
	outStreamedAnimLimit = BITWISE_COMPRESSION_RING_BUFFER;
	outMemoryLimit = BITWISE_COMPRESSION_RING_BUFFER_MEMORY_LIMIT;
}

///////////////////////////////////////////////////////////////////////////////

#undef INDEX_NONE
#undef BITWISE_COMPRESSION_RING_BUFFER

///////////////////////////////////////////////////////////////////////////////
