/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderTextureStreaming.h"
#include "renderTextureStreamingNonCooked.h"
#include "renderTextureStreamingCooked.h"
#include "renderTextureStreamRequest.h"
#include "../core/configVar.h"
#include "../core/ioTags.h"
#include "../engine/texture.h"
#include "../engine/renderSettings.h"
#include "../core/taskRunner.h"
#include "../core/taskDispatcher.h"

#include "../engine/renderCommands.h"


/////////////////////////////////////////////////////////////////////////////////
// IDEAS FOR POSSIBLE IMPROVEMENT:

/* Instead of each texture holding a streaming task, streaming manager could hold and
manage those. Saves a pointer per texture, and gives a single list to go over to
update and flush any pending jobs. */

/* More stuff could maybe be moved into STextureStreamingRegistration, so the texture
itself needs to be touched less. */

/* Different strategies for game/cinematic mode? In game, we're typically moving
towards textures, and cinematic is more stationary (with sudden changes). Maybe this
could be used to make smarter heuristics? */

/* Split the main update task into multiple smaller tasks that can chain together. Can
probably be a lower priority too, since we don't require the update to be finished
each frame. */

/* Should have a proper "size on screen" estimate. This would require generating some
sort of UV density information for meshes, and including that when binding materials. */

/* Maybe streaming tasks could be chained together instead of submitting everything
at once. Would lessen the chance of saturating the job threads, and we only check
results once per frame. */

/* STextureStreamingContext.m_results doesn't really need to be a map. Could put it
in a regular array instead. */

/* In registered textures, maybe track last non-free index, so we don't have to go
through them all every time if there's a bunch of empty space at the end. Or do some
sort of compaction pass sometimes? */

/* Still a lot of jumping around between UpdateEntry, RefinerEntry, Registration. Maybe
that could be reduced? At least they should tend to be somewhat close together. */

/* Could move some more stuff into UpdateEntry, to reduce some jumping around? */

/////////////////////////////////////////////////////////////////////////////////


namespace Config
{
	TConfigVar< Bool >								cvForceUnstreamTextures( "Streaming/Textures", "ForceUnstream", false );
	TConfigVar< Bool >								cvStreamingEnabled( "Streaming/Textures", "StreamingEnabled", true );
	TConfigVar< Bool >								cvAlwaysStreamMax( "Streaming/Textures", "AlwaysStreamMax", false );
	TConfigVar< Bool >								cvUseMipRefiner( "Streaming/Textures", "UseMipRefiner", true );
	TConfigVar< Bool >								cvForceCinematicModeOn( "Streaming/Textures", "ForceCinematicModeOn", false );
	TConfigVar< Bool >								cvForceCinematicModeOff( "Streaming/Textures", "ForceCinematicModeOff", false );
	TConfigVar< Int32, Validation::IntRange<0,3> >	cvCinematicModeMipBias( "Streaming/Textures", "CinematicModeMipBias", 1 );
	TConfigVar< Int32, Validation::IntRange<0,3> >	cvLockedMipBias( "Streaming/Textures", "LockedMipBias", 1 );
	TConfigVar< Int32 >								cvDroppedMipSizeThreshold( "Streaming/Textures", "DroppedMipSizeThreshold", 512 );
}


// Affects precision of a texture's distance when deciding streaming order. Lower values will give lower precision. Low precision may
// result in a texture that is farther away from another having slightly higher priority; but high precision can lead to rapid ping-ponging
// if two textures are at similar distances.
#define STREAMING_PRIORITY_PRECIS_MULT		100
#define MAX_PRIORITY						120.0f

// When choosing a mip level for a texture, this distance is assumed to give a 1-to-1 pixel-texel ratio.
#define IDEAL_TEXEL_DISTANCE				20.0f

// For non-locked textures, this is the range of mips from base to ideal, where we have an "acceptable" mip level
#define NON_LOCKED_MIP_RANGE				2

// For locked textures, streaming order is adjusted based on this and the number of streamable mips. Making this
// bigger will make the streamable mips have more impact on the order adjustment.
#define LOCKED_TEXTURE_DIST_FACTOR_MULT		0.25f


#ifndef RED_FINAL_BUILD

STextureStreamingStats::STextureStreamingStats()
	:	m_initialRequestedSize( 0 ),
	m_finalRequestedSize( 0 )
{}

STextureStreamingStats::STextureStreamingStats( STextureStreamingStats && other )
	:	m_initialRequestedSize( std::move( other.m_initialRequestedSize ) ),
	m_finalRequestedSize( std::move( other.m_finalRequestedSize ) ),
	m_textures( std::move( other.m_textures ) )
{}


STextureStreamingStats::STextureStreamingStats( const STextureStreamingStats & other )
	:	m_initialRequestedSize( other.m_initialRequestedSize ),
		m_finalRequestedSize( other.m_finalRequestedSize ),
		m_textures( other.m_textures )
{}

STextureStreamingStats & STextureStreamingStats::operator=( const STextureStreamingStats & other )
{
	if( this != &other )
	{
		m_initialRequestedSize = other.m_initialRequestedSize;
		m_finalRequestedSize = other.m_finalRequestedSize;
		m_textures = other.m_textures;
	}

	return *this;
}
STextureStreamingStats & STextureStreamingStats::operator=( STextureStreamingStats &&  other )
{
	STextureStreamingStats( std::move( other ) ).Swap( *this );
	return *this;
}

void STextureStreamingStats::Swap( STextureStreamingStats & other )
{
	::Swap( m_initialRequestedSize, other.m_initialRequestedSize );
	::Swap( m_finalRequestedSize, other.m_finalRequestedSize );
	m_textures.SwapWith( other.m_textures );
}

STextureStreamingTexStat::STextureStreamingTexStat()
	:	m_streamingOrder( 0 ),
		m_currentMip( 0 ),
		m_pendingMip( 0 ),
		m_originalRequestedMip( 0 ),
		m_requestedMip( 0 ),
		m_baseMip( 0 ),
		m_hasLock( false )
{}

STextureStreamingTexStat::STextureStreamingTexStat( STextureStreamingTexStat && other )
	:	m_textureName( std::move( other.m_textureName ) ),
	m_streamingOrder( std::move( other.m_streamingOrder ) ),
	m_currentMip( std::move( other.m_currentMip ) ),
	m_pendingMip( std::move( other.m_pendingMip ) ),
	m_originalRequestedMip( std::move( other.m_originalRequestedMip ) ),
	m_requestedMip( std::move( other.m_requestedMip ) ),
	m_baseMip( std::move( other.m_baseMip ) ),
	m_hasLock( std::move( other.m_hasLock ) ) 
{}

STextureStreamingTexStat::STextureStreamingTexStat( const STextureStreamingTexStat & other )
	:	m_textureName( other.m_textureName ),
		m_streamingOrder( other.m_streamingOrder  ),
		m_currentMip( other.m_currentMip ),
		m_pendingMip( other.m_pendingMip ),
		m_originalRequestedMip( other.m_originalRequestedMip ),
		m_requestedMip( other.m_requestedMip ),
		m_baseMip( other.m_baseMip ),
		m_hasLock( other.m_hasLock ) 
{}

STextureStreamingTexStat & STextureStreamingTexStat::operator=( const STextureStreamingTexStat & other )
{
	if( this != &other )
	{
		m_textureName = other.m_textureName;
		m_streamingOrder = other.m_streamingOrder;
		m_currentMip = other.m_currentMip;
		m_pendingMip = other.m_pendingMip;
		m_originalRequestedMip = other.m_originalRequestedMip;
		m_requestedMip = other.m_requestedMip;
		m_baseMip =other.m_baseMip;
		m_hasLock = other.m_hasLock; 
	}
	
	return *this;
}

STextureStreamingTexStat & STextureStreamingTexStat::operator=( STextureStreamingTexStat && other )
{
	STextureStreamingTexStat( std::move( other ) ).Swap( *this );
	return *this;
}

void STextureStreamingTexStat::Swap( STextureStreamingTexStat & other )
{
	m_textureName.SwapWith( other.m_textureName );
	::Swap( m_streamingOrder, other.m_streamingOrder );
	::Swap( m_currentMip, other.m_currentMip );
	::Swap( m_pendingMip, other.m_pendingMip );
	::Swap( m_originalRequestedMip, other.m_originalRequestedMip );
	::Swap( m_requestedMip, other.m_requestedMip );
	::Swap( m_baseMip, other.m_baseMip );
	::Swap( m_hasLock, other.m_hasLock );
}

#endif


struct STextureStreamingRegistration
{
	union
	{
		struct
		{
			CRenderTextureBase*				m_texture;				// Holds reference to this.
			IBitmapTextureStreamingSource*	m_streamingSource;		// does not hold reference.

			// Copy over some basic information from the texture. This is stuff that doesn't really change, and
			// is accessed quite a bit during the streaming update. So it's nice to not have to follow the texture
			// pointer. (okay, so we still have to follow a Registration ref, but they're generally going to be
			// closer in memory and smaller, so there's at least a chance of being in some level of cache).

			Uint32 m_textureBaseSize;

			Uint16 m_baseWidth;
			Uint16 m_baseHeight;
			Uint8 m_smallestLoadableMip;

			Int8 m_maxStreamingMipIndex;
			Uint8 m_maxMipCount;
			Int8 m_streamingPriority;
		};
		Uint16 m_nextFree;
	};
	Bool m_isFree;

	STextureStreamingRegistration()
		: m_nextFree( 0xffff )
		, m_isFree( true )
	{}

	~STextureStreamingRegistration()
	{
		if ( !m_isFree )
		{
			SAFE_RELEASE( m_texture );
		}
	}

	void Clear( Uint16 nextFree )
	{
		if ( !m_isFree )
		{
			SAFE_RELEASE( m_texture );
		}

		m_nextFree = nextFree;
		m_isFree = true;
	}

	void InitWithTexture( CRenderTextureBase* texture )
	{
		if ( m_isFree )
		{
			m_texture = nullptr;
		}
		m_isFree = false;
		SAFE_COPY( m_texture, texture );
		m_streamingSource		= m_texture->GetStreamingSource();

		m_baseWidth				= m_streamingSource->GetBaseWidth();
		m_baseHeight			= m_streamingSource->GetBaseHeight();
		m_smallestLoadableMip	= m_streamingSource->GetSmallestLoadableMip();
		m_textureBaseSize		= m_texture->GetApproxSize();
		m_maxStreamingMipIndex	= m_texture->GetMaxStreamingMipIndex();
		m_maxMipCount			= m_texture->GetMaxMipCount();

		const TextureGroup* textureGroup = SRenderSettingsManager::GetInstance().GetTextureGroup( m_texture->GetTextureGroupName() );
		if ( textureGroup != nullptr )
		{
			m_streamingPriority = textureGroup->m_highPriority ? 1 : 0;
		}

	}

	void Update()
	{
		InitWithTexture( m_texture );
	}

	bool IsLastReference() const
	{
		// Yes yes, GetRefCount says it's temporary and don't use it... but that was over a year ago and it's still here :)
		return !m_isFree && m_texture->GetRefCount() == 1;
	}
};


struct STextureStreamingUpdateEntry
{
	CRenderTextureBase*	texture;				//TODO: Remove this, just use index?
	Float				distance;
	Uint16				textureIdx;
	Uint16				streamingOrder;

	Uint8				currentOrPending;
	Uint8				resident;
	Uint8				current;
	Bool				hasHiRes			: 1;
	Bool				hasPending			: 1;
	Bool				hasLock				: 1;
	Bool				hasWaitingRequest	: 1;
	Uint8				category			: 3;

	static_assert( eTextureCategory_MAX <= (1<<3), "Too many texture categories to fit in 3-bit index" );
};


// This should be kept as small and simple as possible, so sorting a list of them doesn't have huge structs
// to move around.
struct STextureStreamingRefinerEntry
{
	Uint16	texEntry;
	Uint8	requestedMip			: 4;
	Uint8	currentMip				: 4;
	Uint8	baseMip					: 4;
	Uint8	originalRequestedMip	: 4;		// Only used for stats, but otherwise would be unused 4 bits.
};


struct STextureStreamingContext
{
	static const Uint32 MAX_TEXTURES = 8*1024;
	typedef Red::Threads::CLightMutex TMutex;
	typedef Red::Threads::CScopedLock<TMutex> TScopedLock;

	mutable TMutex												m_registeredTexturesMutex;
	TStaticArray<STextureStreamingRegistration,MAX_TEXTURES>	m_registeredTextures;
	Uint16														m_registeredTexturesFreeIndex;

	TDynArray< STextureStreamingUpdateEntry >					m_updateEntries;
	TDynArray< STextureStreamingRefinerEntry >					m_refinerEntries;
	TDynArray< Uint32 >											m_sortKeys;

	Float														m_maxStreamingDistance[eTextureCategory_MAX];

	TDynArray<TPair<CRenderTextureBase*, STextureStreamResults >>	m_results;


	STextureStreamingContext()
		: m_registeredTexturesFreeIndex( 0 )
	{
		const Uint32 INITIAL_RESERVE = 5000;

		m_updateEntries.Reserve( INITIAL_RESERVE );
		m_refinerEntries.Reserve( INITIAL_RESERVE );
		m_sortKeys.Reserve( INITIAL_RESERVE );
	}
};


/////////////////////////////////////////////////////////////////////////////////

IRenderTextureStreamingTask::IRenderTextureStreamingTask( const GpuApi::TextureDesc& textureDesc, Int8 streamingMipIndex )
	: m_textureDesc( textureDesc )
	, m_streamingMipIndex( streamingMipIndex )
{
}

IRenderTextureStreamingTask::~IRenderTextureStreamingTask()
{
}

IRenderTextureStreamingTask* IRenderTextureStreamingTask::Create( const GpuApi::TextureRef& existingTexture, Bool copyFromExisting, IBitmapTextureStreamingSource* streamingSource, Uint32 firstMipmap, const Int8 priority, Bool& inoutAllowNewStreaming )
{
	RED_FATAL_ASSERT( streamingSource != nullptr, "Trying to create texture streaming object with no streaming source" );


	// Get texture info
	GpuApi::TextureDesc residentTextureDesc = GpuApi::GetTextureDesc( existingTexture );

	// Declare texture object info
	GpuApi::TextureDesc desc;
	
	desc.type		= residentTextureDesc.type;
	desc.initLevels	= streamingSource->GetNumMipmaps() - firstMipmap;
	desc.format		= residentTextureDesc.format;
	desc.usage		= GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_Immutable;
	desc.sliceNum	= residentTextureDesc.sliceNum;

	if ( streamingSource->GetEncodedFormat() != 0 )
	{
		// Overwrite the format if we know it
		desc.format		= ITexture::DecodeTextureFormat( streamingSource->GetEncodedFormat() );
	}

	desc.width		= GpuApi::CalculateTextureMipDimension( streamingSource->GetBaseWidth(), firstMipmap, desc.format );
	desc.height		= GpuApi::CalculateTextureMipDimension( streamingSource->GetBaseHeight(), firstMipmap, desc.format );

	// Create appropriate streaming helper
	if ( !streamingSource->IsCooked() )
	{
		// Stream from non cooked data source
		return CRenderTextureStreamingTaskNotCooked::Create( desc, firstMipmap, streamingSource );
	}
	else
	{
		// Get the data source
		CTextureCacheStreamingSourcePC* source = static_cast< CTextureCacheStreamingSourcePC* >( streamingSource );
		const CTextureCacheQuery& query = source->GetTextureCacheQuery();
		if ( !query )
		{
#ifndef RED_FINAL_BUILD
			WARN_TEXSTREAM( TXT("Unable to create cooked texture streaming task for '%ls'"), streamingSource->GetDebugName().AsChar() );
#endif
			return nullptr;
		}

		// Determine priority tag
		Uint8 priotyTag = eIOTag_TexturesNormal;
		if ( priority < 0 )
		{
			priotyTag = eIOTag_TexturesLow;
		}
		else if ( priority > 0 )
		{
			priotyTag = eIOTag_TexturesImmediate;
		}

		// Stream from non cooked data source
		return CRenderTextureStreamingTaskCooked::Create( desc, copyFromExisting ? existingTexture : GpuApi::TextureRef::Null(), firstMipmap, query, priotyTag, inoutAllowNewStreaming );
	}
}

void IRenderTextureStreamingTask::SetTextureInfo( const GpuApi::TextureRef& texture )
{
	switch ( m_textureDesc.type )
	{
		case GpuApi::TEXTYPE_2D:
			GpuApi::SetTextureDebugPath( texture, "streamed texture" );
			break;

		case GpuApi::TEXTYPE_ARRAY:
			GpuApi::SetTextureDebugPath( texture, "streamed texture array" );
			break;	

		case GpuApi::TEXTYPE_CUBE:
			GpuApi::SetTextureDebugPath( texture, "streamed cube texture" );
			break;
	}
}

/////////////////////////////////////////////////////////////////////////////////


Bool CRenderTextureBase::StartStreaming( Int8 requestedMip, Int8 priority, Bool& inoutAllowNewStreaming )
{
	// Not really thread safe, but we can make some assumptions:
	//    - m_streamingTask is only set by streaming update task, or between tasks.
	//    - Outside of these cases, it is only checked for non-null, for seeing if anything is streaming. Doesn't need to be exact.

	// There's no streaming source to stream from
	if ( !m_streamingSource )
	{
		return false;
	}

	RED_ASSERT( requestedMip < m_residentMipIndex, TXT("Requesting streaming for the resident mip or lower! Check streaming logic. Request: %d, Resident: %d"), requestedMip, m_residentMipIndex );
	RED_ASSERT( requestedMip <= m_streamingSource->GetSmallestLoadableMip(), TXT("Requesting streaming for a mip smaller than our streaming source allows! Check streaming logic. Request: %d, Smallest: %d"), requestedMip, m_streamingSource->GetSmallestLoadableMip() );

	// If we already have this, don't need to stream anything. Don't use <= here, so that we can still stream in
	// a lower mip, if we want to drop the highest quality after a cutscene.
	if ( m_streamingMipIndex == requestedMip )
	{
		return true;
	}

	// We are already streaming the same
	if ( m_streamingTask && m_pendingMipIndex == requestedMip )
	{
		return true;
	}

	// Cancel any current streaming job.
	if ( m_streamingTask )
	{
		m_streamingTask->Discard();
		m_streamingTask = nullptr;
		m_pendingMipIndex = -1;
	}

	// Use a previously streamed texture as existing if we have one, otherwise the resident.
	GpuApi::TextureRef existingTexture = m_hiResTexture ? m_hiResTexture : m_texture;
	// We can copy from the existing texture if it's a previously streamed texture, or our resident is "valid".
	// Resident is "invalid" for SpeedTree textures, where m_texture is the Blank internal texture, and m_residentMipIndex == 15
	Bool copyFromExisting = m_hiResTexture || m_residentMipIndex < m_streamingSource->GetNumMipmaps();

	// Create new streaming task for streaming the right number of mipmaps as the new texture
	m_streamingTask = IRenderTextureStreamingTask::Create( existingTexture, copyFromExisting, m_streamingSource, requestedMip, priority, inoutAllowNewStreaming );

	if ( m_streamingTask == nullptr )
	{
		return false;
	}

	m_pendingMipIndex = requestedMip;
	return true;
}

Bool CRenderTextureBase::TryFinishStreaming( STextureStreamResults& results )
{
	// Not really thread safe, but we can make some assumptions:
	//    - m_streamingTask is only set by streaming update task, or between tasks.
	//    - Outside of these cases, it is only checked for non-null, for seeing if anything is streaming. Doesn't need to be exact.
	if ( m_streamingTask && m_streamingTask->TryFinish( results ) )
	{
		// Release streaming task
		m_streamingTask->Discard();
		m_streamingTask = nullptr;
		m_pendingMipIndex = -1;

		return true;
	}
	return false;
}

void CRenderTextureBase::TickStreaming( Bool& allowNewStreaming )
{
	// Not really thread safe, but we can make some assumptions:
	//    - m_streamingTask is only set by streaming update task, or between tasks.
	//    - Outside of these cases, it is only checked for non-null, for seeing if anything is streaming. Doesn't need to be exact.
	if ( m_streamingTask )
	{
		m_streamingTask->Tick( allowNewStreaming );
	}
}


void CRenderTextureBase::ApplyStreamingResults( const STextureStreamResults& results )
{
	// This is always called from render thread, between update tasks.

	GpuApi::SafeRelease( m_hiResTexture );
	m_hiResTexture = results.m_hiresTexture;

	if ( results.m_finalizer != nullptr )
	{
		results.m_finalizer->Finalize();
	}


	if ( m_hiResTexture )
	{
		// Remember the mip index of the streamed texture
		m_streamingMipIndex = results.m_streamedMip;
		GetRenderer()->GetTextureStreamingManager()->OnTextureStreamed( this );
	}
	else
	{
		m_streamingMipIndex = m_residentMipIndex;
		GetRenderer()->GetTextureStreamingManager()->OnTextureUnloaded( this );
	}

	// Notify that the streaming status has changed
	OnStreamingChanged();
}

void CRenderTextureBase::CancelStreaming()
{
	// As in TryFinishStreaming, this is not generally safe. But it should be okay with our assumption that it is set only in
	// specific situations.
	if ( m_streamingTask )
	{
		m_streamingTask->Discard();
		m_streamingTask = nullptr;
		m_pendingMipIndex = -1;
	}
	// Even if we didn't have an active streaming task, notify streaming manager that we've been canceled.
	// Need to check if renderer/texmgr exist, in case this comes during shutdown.
	if ( GetRenderer() && GetRenderer()->GetTextureStreamingManager() )
	{
		GetRenderer()->GetTextureStreamingManager()->OnTextureCanceled( this );
	}
}

Bool CRenderTextureBase::UnstreamHiRes()
{
	RED_FATAL_ASSERT( !GetRenderer()->GetTextureStreamingManager()->HasActiveUpdate(), "UnloadTexturesOverBudget called while active update" );

	// NOTE : This should not be called while a TextureStreamingUpdate task is running.
	if ( !m_hiResTexture.isNull() )
	{
		// Unload hi res
		GpuApi::SafeRelease( m_hiResTexture );

		// Restore mip index to the number of resident levels
		m_streamingMipIndex = m_residentMipIndex;
		m_pendingMipIndex = -1;
 
		// Notify that the streaming status has changed
		OnStreamingChanged();

		GetRenderer()->GetTextureStreamingManager()->OnTextureUnloaded( this );

		// Unstreamed
		return true;
	}

	// Not released
	return false;
}


Bool CRenderTextureBase::LockStreaming()
{
	if ( m_streamingSource == nullptr )
	{
		return false;
	}

	m_streamingLocks.Increment();
	return true;
}

void CRenderTextureBase::UnlockStreaming()
{
	Int32 lockCount = m_streamingLocks.Decrement();
	RED_ASSERT( lockCount >= 0, TXT("Unlocked texture stream too many times! counter: %d"), lockCount );
}


//////////////////////////////////////////////////////////////////////////


/// Given a set of textures, and a requested mip for each, modify the requested mips to try to fit it within a memory limit.
struct CTextureStreamingMipRefiner : Red::System::NonCopyable
{

	// Function prototype for selecting a lowest mip index to drop to during the refinement. Takes index into m_refinerEntries.
	typedef std::function< Uint8( Uint32 ) > LowestMipFunc;

	const STextureStreamingContext&						m_context;
	const TDynArray< STextureStreamingUpdateEntry >&	m_texEntries;
	TDynArray< STextureStreamingRefinerEntry >&			m_refinerEntries;

	const Uint32										m_memoryLimit;
	Uint32												m_totalSize;

	Uint32												m_lastImportantIndex;

public:
	CTextureStreamingMipRefiner( STextureStreamingContext& context, Uint32 sizeLimit, Uint32 intialSize )
		: m_context( context )
		, m_texEntries( context.m_updateEntries )
		, m_refinerEntries( context.m_refinerEntries )
		, m_memoryLimit( sizeLimit )
		, m_totalSize( intialSize )
	{
	}

	// Adjust the mip indices. Returns the final size of the set of mips. This might still be greater than the limit, if the refiner
	// wasn't able to make everything fit.
	Uint32 DoRefine()
	{
		// If we already fit, nothing to do.
		if ( m_totalSize <= m_memoryLimit )
		{
			return m_totalSize;
		}

		// Find the index of the last entry with non-resident request. We don't need to do anything past this, because we
		// can't really drop them further.
		m_lastImportantIndex = m_refinerEntries.Size();
		for ( Int32 i = m_refinerEntries.SizeInt() - 1; i >= 0; --i )
		{
			if ( m_refinerEntries[ i ].requestedMip < m_texEntries[ m_refinerEntries[ i ].texEntry ].resident )
			{
				m_lastImportantIndex = i;
				break;
			}
		}

		// No non-resident requests?? Just return...
		if ( m_lastImportantIndex == m_refinerEntries.Size() )
		{
			return m_totalSize;
		}


		const Uint8 DROP_BY_LOTS_TO_FORCE_ONE_STEP = 15;

		Uint32 workingSize = m_totalSize;

		// First, drop down to current (or base if current is worse). Here we just do an immediate drop, rather than going 1-by-1.
		workingSize = DoSinglePass( workingSize, 0, m_refinerEntries, DROP_BY_LOTS_TO_FORCE_ONE_STEP, [this]( Uint32 i ) {
			return Min( m_refinerEntries[ i ].currentMip, m_refinerEntries[ i ].baseMip );
		} );
		if ( workingSize <= m_memoryLimit )
		{
			return workingSize;
		}


		// Next, try dropping to baseMips. Again, just drop right down to base. It's been picked as an acceptable quality, so it
		// should be fine to go there. This avoids us dropping by one to make a little more room, then by one again later to make
		// a bit more room again, then by another one. Instead of dropping several textures by one, requiring a reload on each,
		// maybe we can just drop one texture.
		workingSize = DoSinglePass( workingSize, 0, m_refinerEntries, DROP_BY_LOTS_TO_FORCE_ONE_STEP, [this]( Uint32 i ) {
			// If the texture is locked, with a waiting request, it's probably being requested for some scene, and might not have
			// been spawned yet. If it isn't spawned, then we probably don't have a proper distance for it, so our baseMip will
			// be potentially too small. In that case, we won't drop it further at this point.
			return m_texEntries[ m_refinerEntries[ i ].texEntry ].hasLock ? 0: m_refinerEntries[ i ].baseMip;
		} );
		if ( workingSize <= m_memoryLimit )
		{
			return workingSize;
		}


		// If we're still over budget, drop even more, up to resident. Now we go mip-by-mip, to try and stay as close to base as
		// possible.
		workingSize = ProgressiveRefine( workingSize, m_refinerEntries, 1, [this]( Uint32 i ) {
			return m_texEntries[ m_refinerEntries[ i ].texEntry ].resident;
		} );


		return workingSize;
	}

private:


	void FillWorkingEntries( TDynArray< STextureStreamingRefinerEntry >& workingEntries )
	{
		Red::System::MemoryCopy( workingEntries.Data(), m_refinerEntries.Data(), m_refinerEntries.DataSize() );
	}

	// Apply a progressive refinement of a set of mip selections, hopefully to make it fit within budget.
	// Operating on a section of the mips (tail end, so the textures with lower priority), decrease mips until they either
	// have reached a limit (decided by the provided lowestMip function), or until we've gotten within budget. If we process
	// the full section and still don't fit, try with a larger section.
	Uint32 ProgressiveRefine( Uint32 totalSize, TDynArray< STextureStreamingRefinerEntry >& workingEntries, Uint8 step, const LowestMipFunc& lowestMip )
	{
		// Working buffer of refiner entries. Each time through the outermost loop, we want to start fresh with the original
		// data, so we modify a local copy.
		TDynArray< STextureStreamingRefinerEntry > working( m_refinerEntries.Size() );

		Int32 beginIdx = m_refinerEntries.Size();
		Uint32 workingTotalSize = totalSize;
		while ( workingTotalSize > m_memoryLimit )
		{
			// Start fresh with original mip requests. Since it's just int, can use memcpy instead of array copy.
			FillWorkingEntries( working );

			// Each time through, increase the size of the region we're refining.
			beginIdx = beginIdx / 3;

			// Eventually we may just have to give up.
			if ( beginIdx == 0 )
			{
				break;
			}

			workingTotalSize = RefineSection( totalSize, beginIdx, working, step, lowestMip );
		}

		// Move the local working entries (with final mip selections) back to the output.
		workingEntries.SwapWith( working );

		return workingTotalSize;
	}


	Uint32 RefineSection( Uint32 totalSize, Int32 rangeMin, TDynArray< STextureStreamingRefinerEntry >& entries, Uint8 step, const LowestMipFunc& lowestMip )
	{
		// Do at most 2 passes
		for ( Uint32 p = 0; p < 2; ++p )
		{
			totalSize = DoSinglePass( totalSize, rangeMin, entries, step, lowestMip );
			if ( totalSize <= m_memoryLimit )
			{
				return totalSize;
			}

			rangeMin = ( rangeMin + entries.Size() ) / 2;
		}

		return totalSize;
	}

	Uint32 DoSinglePass( Uint32 totalSize, Int32 rangeMin, TDynArray< STextureStreamingRefinerEntry >& entries, Uint8 step, const LowestMipFunc& lowestMip )
	{
		for ( Int32 i = m_lastImportantIndex; i >= rangeMin; --i )
		{
			const STextureStreamingUpdateEntry& tex = m_texEntries[ m_refinerEntries[ i ].texEntry ];
			const STextureStreamingRegistration& reg = m_context.m_registeredTextures[tex.textureIdx];
			RED_FATAL_ASSERT( !reg.m_isFree, "How do we have a freed texture registration?" );

			STextureStreamingRefinerEntry& entry = entries[i];

			const Uint8 oldNeededMipmap = entry.requestedMip;

			// If requestedMip is already past what lowestMip gives, then we shouldn't be increasing it.
			const Uint8 maxMip = Max< Uint8 >( lowestMip( i ), entry.requestedMip );
			const Uint8 newNeededMipmap = Min< Uint8 >( entry.requestedMip + step, maxMip );

			if ( oldNeededMipmap != newNeededMipmap )
			{
				entry.requestedMip = newNeededMipmap;


				if ( oldNeededMipmap < tex.resident )
				{
					totalSize -= CRenderTextureBase::GetApproxSize( reg.m_textureBaseSize, oldNeededMipmap );
				}
				if ( newNeededMipmap < tex.resident )
				{
					totalSize += CRenderTextureBase::GetApproxSize( reg.m_textureBaseSize, newNeededMipmap );
				}

				if ( totalSize <= m_memoryLimit )
				{
					break;
				}
			}
		}

		return totalSize;
	}

};


//////////////////////////////////////////////////////////////////////////

class CTextureStreamingUpdateTask : public CTask
{
private:
	Red::Threads::CMutex				m_runningMutex;

	STextureStreamingContext&			m_context;

	Int8								m_cinematicMipBias;
	Int8								m_cinematicPriorityBias;

	// HACK : Getting and updating the last bind distances is not threadsafe. Updating properly isn't possible with just
	// basic atomic operations, and it's not worth protecting it with a mutex. So instead, we just mark when we finish with
	// that part (which happens first), and if we happen to hit RenderFrame before it's done, we'll wait for it. In practice,
	// we probably shouldn't ever need to wait.
	Red::Threads::CAtomic< Bool >		m_pastUnsafeArea;

	const Bool							m_allowExpire;
	Bool								m_wasOverloaded;

	const Bool							m_allowMipDrop;				// Are we allowed to drop mips on waited-for textures?
	const Uint32						m_droppedMipSizeThreshold;

	const Uint32						m_memoryLimit;


#ifndef RED_FINAL_BUILD
	void UpdateTexturesStats( const TDynArray< STextureStreamingRefinerEntry > & refinerEntries );
public:
	STextureSteamingStatsContainer *	m_statsContainer;
#endif

public:
	CTextureStreamingUpdateTask( Bool allowExpire, Bool cinematicMode, const Float (&streamingDistances)[ eTextureCategory_MAX ], STextureStreamingContext& context );
	~CTextureStreamingUpdateTask();

	virtual void			Run() override;

	// Make sure the update task is past the first "unsafe" part. This will either run that section on the
	// current thread, or wait for the task to finish it.
	void					FinishUnsafePart();

	void					ApplyResults();

#ifndef NO_DEBUG_PAGES
public:
	virtual const Char*		GetDebugName() const override { return TXT("TEXSTREAM_Texture Streaming Update"); }
	virtual Uint32			GetDebugColor() const override { return COLOR_UINT32( 100, 192, 127 ); }

private:
	Float					m_processingTime;
public:
	RED_INLINE Float		GetProcessingTime() const { return m_processingTime; }
#endif

	RED_INLINE Bool			WasOverloaded() const { return m_wasOverloaded; }


private:
	void CollectAllTextures();
	void ProcessStreaming();

	// Fill m_context.m_refinerEntries, return the total size of all requested mips.
	Uint32 FillRefinerEntries();

	// Either drop mips using MipRefiner to fit budget, or increase mips if under budget. Return new size of mip requests.
	Uint32 RefineMipRequests( Uint32 accumulatedSize );

	// Unload any textures that are over budget. Return true if streaming can continue.
	Bool UnloadOverBudget( Uint32 mipRequestsSize );

	// Allow any current streaming tasks to finish. Some may be canceled here as well, if the current mip is no longer viable.
	void FinishCurrentStreaming();

	// Tick current streaming tasks, and start new ones. Return size of current (or pending, if there's an active task) mips.
	Uint32 TickAndStartNewStreaming();

	// If stream fails because it could temporarily cause an over-budget, return the amount that must be freed first. Else 0.
	Uint32 TickAndStartStreamingTexture( const STextureStreamingRefinerEntry& refinerEntry, Bool& allowNewStreaming, Uint32& accumulatedSize );


	// Fill mip requests. texEntry must be already set.
	void CalcRefinerEntry( STextureStreamingRefinerEntry& outRefinerEntry ) const;


	RED_INLINE Int8 GetMipBias( ETextureCategory category ) const
	{
		switch ( category )
		{
		case eTextureCategory_Generic:
		case eTextureCategory_World:
			return 0;

		case eTextureCategory_Scene:
		case eTextureCategory_Characters:
		case eTextureCategory_Heads:
			return m_cinematicMipBias;

		default:
			RED_HALT( "Invalid category: %d", category );
			return 0;
		}
	}

	RED_INLINE Int8 GetPriorityBias( ETextureCategory category, Bool additionalBoost ) const
	{
		if ( additionalBoost )
		{
			return 1;
		}

		switch ( category )
		{
		case eTextureCategory_Generic:
		case eTextureCategory_World:
			return 0;

		case eTextureCategory_Scene:
		case eTextureCategory_Characters:
		case eTextureCategory_Heads:
			return m_cinematicPriorityBias;

		default:
			RED_HALT( "Invalid category: %d", category );
			return 0;
		}
	}

	void AddResults( CRenderTextureBase* texture, STextureStreamResults&& results )
	{
		texture->AddRef();
		m_context.m_results.PushBack( MakePair( texture, std::move(results) ) );
	}


	// Tests in-flight memory budget
	RED_INLINE Bool CanStreamTextureThisFrame( Uint32 textureSizeBytes ) const
	{
		// This is only an estimation
		const GpuApi::TextureStats* textureStats = GpuApi::GetTextureStats();
		RED_FATAL_ASSERT( textureStats != nullptr, "Null texture stats! How??" );

		const Uint32 memoryInFlight = textureStats->m_streamableTextureMemoryInFlight;
		const Uint32 countInFlight = textureStats->m_streamableTextureCountInFlight;
		const Uint32 memoryInFlightLimit = Config::cvTextureInFlightBudget.Get() * 1024 * 1024;
		const Uint32 countInFlightLimit = Config::cvTextureInFlightCountBudget.Get();

		return ( memoryInFlight + textureSizeBytes <= memoryInFlightLimit ) && countInFlight < countInFlightLimit;
	}

};


CTextureStreamingUpdateTask::CTextureStreamingUpdateTask( Bool allowExpire, Bool cinematicMode, const Float (&streamingDistances)[ eTextureCategory_MAX ], STextureStreamingContext& context )
	: m_context( context )
	, m_cinematicMipBias( 0 )
	, m_cinematicPriorityBias( 0 )
	, m_allowExpire( allowExpire )
	, m_pastUnsafeArea( false )
	, m_wasOverloaded( false )
	, m_allowMipDrop( GetRenderer()->GetTextureStreamingManager()->IsPrefetchMipDropAllowed() )
	, m_droppedMipSizeThreshold( Config::cvDroppedMipSizeThreshold.Get() )
	, m_memoryLimit( Config::cvTextureMemoryBudget.Get() * 1024 * 1024 )
#ifndef NO_DEBUG_PAGES
	, m_processingTime( 0.0f )
#endif
{
	// Copy (non-squared) streaming distances into the update context.
	for ( Uint32 i = 0; i < eTextureCategory_MAX; ++i )
	{
		m_context.m_maxStreamingDistance[i] = MSqrt( streamingDistances[i] );
	}


	// Calculate cinematic mode mip bias
	// The cinematic mode works like this: For some texture categories (characters, heads, scene items) 
	// we are using reduced the texture quality during normal gameplay and full texture resolution only in scenes.
	// The cvCinematicModeMipBias controls how many mips are removed when the cinematic mode is not active.

	if ( !cinematicMode )
	{
		if ( Config::cvTextureStreamingReduceGameplayLOD.Get() )
		{
			m_cinematicMipBias = (Int8) Config::cvCinematicModeMipBias.Get();
		}
		m_cinematicPriorityBias = 1;
	}


	static_assert( eTextureCategory_MAX == 5, "Texture categories have changed. Probably need to update this" );
}

CTextureStreamingUpdateTask::~CTextureStreamingUpdateTask()
{
	if ( !m_context.m_results.Empty() )
	{
		WARN_TEXSTREAM( TXT("Destroying CTextureStreamingUpdateTask with non-empty results. Didn't apply them? This is okay at shutdown.") );
		for ( auto& it : m_context.m_results )
		{
			CRenderTextureBase* texture = it.m_first;
			STextureStreamResults& results = it.m_second;

			texture->Release();
			GpuApi::SafeRelease( results.m_hiresTexture );
		}
		m_context.m_results.Clear();
	}
}


void CTextureStreamingUpdateTask::FinishUnsafePart()
{
	// If we're already past, nothing to do.
	if ( m_pastUnsafeArea.GetValue() )
	{
		return;
	}

	if ( !m_runningMutex.TryAcquire() )
	{
		// Couldn't get the lock, so task is already running. Just wait for the flag to be set.
		while ( !m_pastUnsafeArea.GetValue() ) { RED_BUSY_WAIT(); }
		return;
	}

	Red::System::StopClock timer;

	// Got the lock, so we can run the collection now.
	CollectAllTextures();

	// And set the flag, so we don't collect again
	m_pastUnsafeArea.SetValue( true );

	// Don't forget to release!
	m_runningMutex.Release();
#ifndef NO_DEBUG_PAGES
	m_processingTime += static_cast< Float >( timer.GetDelta() );
#endif
}


void CTextureStreamingUpdateTask::Run()
{
	PC_SCOPE_PIX( CTextureStreamingUpdateTask );

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_runningMutex );

	Red::System::StopClock timer;

	if ( !m_pastUnsafeArea.GetValue() )
	{
		CollectAllTextures();

		// We're done updating/getting last bind distances. The rest of the update is reasonably safe, assuming our assumptions are made.
		m_pastUnsafeArea.SetValue( true );
	}

	// process stream in only if allowed
	if ( Config::cvStreamingEnabled.Get() )
	{
		ProcessStreaming();
	}

	{
		PC_SCOPE_PIX( TEXSTREAM_Cleanup );
		m_context.m_updateEntries.ClearFast();
		m_context.m_refinerEntries.ClearFast();
		m_context.m_sortKeys.ClearFast();
	}


#ifndef NO_DEBUG_PAGES
	m_processingTime += static_cast< Float >( timer.GetDelta() );
#endif
}


void CTextureStreamingUpdateTask::CalcRefinerEntry( STextureStreamingRefinerEntry& outRefinerEntry ) const
{
	STextureStreamingUpdateEntry& texEntry = m_context.m_updateEntries[ outRefinerEntry.texEntry ];
	const auto& reg = m_context.m_registeredTextures[texEntry.textureIdx];
	const Uint8 residentMip = texEntry.resident;

	// If it's out of range, we just want resident.
	if ( texEntry.distance >= m_context.m_maxStreamingDistance[texEntry.category] )
	{
		outRefinerEntry.baseMip					= residentMip;
		outRefinerEntry.currentMip				= texEntry.currentOrPending;
		outRefinerEntry.requestedMip			= residentMip;
		outRefinerEntry.originalRequestedMip	= residentMip;
		return;
	}

	const Bool hasLock = texEntry.hasLock;

	// Calculate mipmap needed for displaying texture

	// Maximum mip, with any global biasing for cinematic mode.
	const Int8 mipBias = ( hasLock ? 0 : GetMipBias( (ETextureCategory)texEntry.category ) );
	const Uint8 minMip = reg.m_maxStreamingMipIndex + mipBias;


	// Locked textures should request a higher resolution, so that even if they're offscreen for a bit and get
	// pushed back by expiration, they may stay reasonable.
	const Int8 baseMipBias = ( hasLock ? Config::cvLockedMipBias.Get() : 0 );


	// Get an estimate of how big the texture is on-screen.
	Float pixelsForTex;
	{
		// HACK : We don't have a proper UV density estimate in meshes, so we can't really get a decent screen size
		// estimate for a texture. This just says that at some fixed distance any texture should have a 1-to-1 ratio of
		// texel to screen pixel.

		const Float maxDimension = (Float)( Max( reg.m_baseWidth, reg.m_baseHeight ) >> minMip );
		// Clamping to maxDimension doesn't really matter, because we can't add more mips if the screen size is already the max.
		pixelsForTex = Min< Float >( IDEAL_TEXEL_DISTANCE * maxDimension / texEntry.distance, maxDimension );
	}

	// Based on the texture's on-screen size, pick an appropriate mip level.
	const Uint8 maxMipCount = reg.m_maxMipCount;
	const Uint8 numWantedMips = Min( 1 + Max( MLog2( (Int32)pixelsForTex ), 0 ) + baseMipBias, maxMipCount );
	const Uint8 baseMip = Clamp< Uint8 >( maxMipCount - numWantedMips + 1, minMip, residentMip );


	// Ideal is a bit higher than the base. For locked textures, our ideal is the full resolution.
	const Uint8 idealMip = hasLock ? minMip : (Uint8)Max< Int8 >( baseMip - NON_LOCKED_MIP_RANGE, minMip );


	// If a texture is locked and doesn't have a suitable current (worse than base), treat it as though the best is the
	// current. This prevents the texture from being dropped in the first refinement pass in cases where maybe an actor
	// has not yet been spawned for a scene and so won't give a proper distance during any frame prefetch.
	const Uint8 currentMip = ( hasLock && texEntry.current > baseMip ) ? idealMip : texEntry.currentOrPending;



	// NOTE : alwaysRequestMax and useRefiner are mainly just for dev purposes. The important case here is the middle one, where
	// we are using the refiner and not requesting max.

	Int8 requestedMip;
	if ( Config::cvAlwaysStreamMax.Get() )
	{
		requestedMip = minMip;
	}
	// If we're going to use the refiner, we use some heuristics to choose a requested mip. These should help reduce the
	// amount of extra reloading needed, and keep the texture streaming somewhat stable.
	else if ( Config::cvUseMipRefiner.Get() )
	{
		// If we have pending, and the pending mip is within our acceptable range, we'll just continue to request
		// the pending mip.
		if ( texEntry.hasPending && currentMip >= idealMip && currentMip <= baseMip )
		{
			requestedMip = currentMip;
		}
		// If the current mip is in acceptable range, try and improve it. Bump by 2 mips, instead of 1. The refiner's
		// first step will be to drop to current, so this way we should only be increasing if we actually have budget
		// for it, and prevent flip-flopping.
		else if ( currentMip >= idealMip && currentMip <= baseMip )
		{
			requestedMip = (Uint8)Clamp< Int8 >( currentMip - 2, idealMip, baseMip );
		}
		// Current mip is lower that the base mip, so we'll just go directly for ideal if we can. If we were to go
		// to base first instead, we would just end up trying to improve it up to ideal anyways. If ideal is too
		// much, we'll get dropped by the refiner.
		else
		{
			requestedMip = idealMip;
		}
	}
	// Without mip refiner, we don't really have the luxury of trying for better, so just go with the base.
	else
	{
		requestedMip = baseMip;
	}


	const Uint32 inflightMemoryLimit = Config::cvTextureInFlightBudget.Get() * 1024 * 1024;
	// In the off-change that a texture is too big for the in-flight limit, drop some mips.
	if ( CRenderTextureBase::GetApproxSize( reg.m_textureBaseSize, requestedMip ) > inflightMemoryLimit )
	{
		ERR_TEXSTREAM( TXT("Impossible to stream %ls at mip %d! %u is bigger than inflight limit %u"), texEntry.texture->GetDepotPath().AsChar(), requestedMip, texEntry.texture->GetApproxSize( requestedMip ), inflightMemoryLimit );
		while ( CRenderTextureBase::GetApproxSize( reg.m_textureBaseSize, requestedMip ) > inflightMemoryLimit && requestedMip < residentMip )
		{
			++requestedMip;
		}
	}


	outRefinerEntry.baseMip					= baseMip;
	outRefinerEntry.currentMip				= currentMip;
	outRefinerEntry.requestedMip			= requestedMip;
	outRefinerEntry.originalRequestedMip	= requestedMip;
}



static Uint32 g_MaxNumTextures = 0;

void CTextureStreamingUpdateTask::CollectAllTextures()
{
	PC_SCOPE_PIX( TEXSTREAM_GatherAllTextures );

	STextureStreamingContext::TScopedLock lock( m_context.m_registeredTexturesMutex );
	const Uint32 maxTextures = m_context.m_registeredTextures.Size();
	for ( Uint32 i = 0; i < maxTextures; ++i )
	{
		auto& reg = m_context.m_registeredTextures[i];

		if ( !reg.m_isFree )
		{
			// If this is the last reference to the texture, we can clear it out instead of processing it.
			if ( reg.IsLastReference() )
			{
				reg.Clear( m_context.m_registeredTexturesFreeIndex );
				m_context.m_registeredTexturesFreeIndex = i;
				continue;
			}

			CRenderTextureBase* texture = reg.m_texture;
			RED_FATAL_ASSERT( texture != nullptr, "Non-free texture registration, but no texture!" );

			// No reason to update expiration on non-streamed textures, so we do it after skipping non-streamed.
			// NOT SAFE IF RENDER THREAD IS BUSY RENDERING
			// So this is only safe if renderer waits until the m_pastUnsafeArea flag below is set.
			if ( m_allowExpire )
			{
				texture->UpdateStreamingExpiration();
			}

			// More assumptions:
			//    - texture category is assigned at creation
			//    - hi-res texture is always assigned as a result of applying this job. So happens between job executions
			//    - texture streaming task is created and finished by this job.
			// LAST BIND DISTANCE NOT THREAD SAFE
			//    - as above, renderer needs to wait until the m_pastUnsafeArea flag below is set.

			m_context.m_updateEntries.Grow(1);
			STextureStreamingUpdateEntry& entry = m_context.m_updateEntries.Back();

			// Don't addref, texture registrations already hold a reference, and won't be released during this task (well,
			// aside from above where we clear if it's the last reference).
			entry.texture			= texture;
			entry.textureIdx		= i;
			entry.distance			= MSqrt( texture->GetLastBindDistance() );
			entry.category			= (Uint8)texture->GetTextureCategory();
			entry.hasHiRes			= texture->HasHiResLoaded();
			entry.hasPending		= texture->HasStreamingPending();
			entry.hasLock			= texture->HasStreamingLock();
			entry.hasWaitingRequest	= texture->HasWaitingRequest();
			entry.current			= texture->GetStreamingMipIndex();
			entry.currentOrPending	= entry.hasPending ? texture->GetPendingMip() : entry.current;
			entry.resident			= texture->GetResidentMipIndex();

			// Don't fill in order yet. We'll do that later.
		}
	}

	g_MaxNumTextures = Max(g_MaxNumTextures, m_context.m_updateEntries.Size());
}


Uint32 CTextureStreamingUpdateTask::FillRefinerEntries()
{
	PC_SCOPE_PIX( TEXSTREAM_GetInitialMips );

	const Uint32 numTextures = m_context.m_updateEntries.Size();

	Uint32 accumulatedSize = 0;

	m_context.m_refinerEntries.ClearFast();
	m_context.m_refinerEntries.ResizeFast( numTextures );

	for ( Uint32 i = 0; i < numTextures; ++i )
	{
		STextureStreamingUpdateEntry& tex = m_context.m_updateEntries[ i ];
		STextureStreamingRefinerEntry& entry = m_context.m_refinerEntries[ i ];
		const STextureStreamingRegistration& reg = m_context.m_registeredTextures[tex.textureIdx];

		entry.texEntry = i;
		CalcRefinerEntry( entry );


		// Now that we know what mip we want, we can figure out the priority for the texture.
		{
			// Textures that are closer, relative to their max streaming distance, should have higher priority.
			// Adjust with a curve, so close-up textures get more precision in the final streaming order values
			const Float distPct = tex.distance / m_context.m_maxStreamingDistance[tex.category];
			Float distFactor = Red::Math::MPow( distPct, 0.5f );

			// If texture is locked we boost the streaming order a bit, so it's less likely to get dropped during refinement.
			if ( tex.hasLock )
			{
				if ( Config::cvUseMipRefiner.Get() )
				{
					const Uint32 numStreamableMips = tex.resident - reg.m_maxStreamingMipIndex;
					RED_ASSERT( numStreamableMips > 0, TXT("Texture %ls has 0 streamable mips, but is considered for streaming!"), tex.texture->GetDepotPath().AsChar() );

					const Float factorScale = Max( numStreamableMips * LOCKED_TEXTURE_DIST_FACTOR_MULT, 1.0f );
					distFactor /= factorScale;
				}
				else
				{
					// When mip refiner is off, we need to ensure locked textures are higher than unlocked, or we risk losing them.
					distFactor -= 1.0f;
				}
			}

			// Quantize the calculated ordering, to reduce ping-ponging between close textures. Clamp it to a "sane" range so we won't
			// have problems when casting to Uint32 (e.g. if distance is FLT_MAX).
			tex.streamingOrder = (Uint16)Clamp< Float >( ( 1 - distFactor ) * STREAMING_PRIORITY_PRECIS_MULT, 0, MAX_PRIORITY );
		}


		// Track how big our requested mip set is.
		if ( entry.requestedMip < tex.resident )
		{
			accumulatedSize += CRenderTextureBase::GetApproxSize( reg.m_textureBaseSize, entry.requestedMip );
		}
	}

	return accumulatedSize;
}


Uint32 CTextureStreamingUpdateTask::RefineMipRequests( Uint32 accumulatedSize )
{
	// Refine the mip selections so that it will fit into memory. The final selection of mips will be in requestedMips.
	if ( accumulatedSize > m_memoryLimit )
	{
		if ( Config::cvUseMipRefiner.Get() )
		{
			PC_SCOPE_PIX( TEXSTREAM_RefineMips );
			CTextureStreamingMipRefiner refiner( m_context, m_memoryLimit, accumulatedSize );
			accumulatedSize = refiner.DoRefine();
		}
	}
	// If we have room to spare, we may be able to avoid dropping some textures. This helps reduce the number of streaming tasks
	// we having going.
	else
	{
		PC_SCOPE_PIX( TEXSTREAM_RefineMipsUnderBudget );
		// Increase entries up to current until we don't have space. This way we don't unload things we have room for just because
		// it's moved further away.
		for ( auto& refinerEntry : m_context.m_refinerEntries )
		{
			const auto& texEntry = m_context.m_updateEntries[ refinerEntry.texEntry ];
			const auto& reg = m_context.m_registeredTextures[texEntry.textureIdx];

			Uint8 oldReq = refinerEntry.requestedMip;
			Uint8 newReq = Min( texEntry.current, texEntry.currentOrPending );

			if ( newReq < oldReq )
			{
				Uint32 newAccumulatedSize = accumulatedSize;
				if ( oldReq < texEntry.resident )
				{
					newAccumulatedSize -= CRenderTextureBase::GetApproxSize( reg.m_textureBaseSize, oldReq );
				}
				if ( newReq < texEntry.resident )
				{
					newAccumulatedSize += CRenderTextureBase::GetApproxSize( reg.m_textureBaseSize, newReq );
				}

				if ( newAccumulatedSize > m_memoryLimit )
				{
					break;
				}

				refinerEntry.requestedMip = newReq;
				accumulatedSize = newAccumulatedSize;
			}
		}
	}

	return accumulatedSize;
}


Bool CTextureStreamingUpdateTask::UnloadOverBudget( Uint32 mipRequestsSize )
{
	if ( mipRequestsSize <= m_memoryLimit )
	{
		return true;
	}

	const Uint32 numTextures = m_context.m_updateEntries.Size();

	Uint32 firstUnstreamableIndex = numTextures;
	Uint32 usedBudget = 0;
	{
		PC_SCOPE_PIX( TEXSTREAM_FindFirstOverLimit );

		for ( Uint32 i = 0; i < numTextures; ++i )
		{
			const STextureStreamingRefinerEntry& refinerEntry = m_context.m_refinerEntries[ i ];
			const STextureStreamingUpdateEntry& texEntry = m_context.m_updateEntries[ refinerEntry.texEntry ];
			const STextureStreamingRegistration& reg = m_context.m_registeredTextures[ texEntry.textureIdx ];

			// If this texture is within the range of textures we are actively streaming, use the proper requested mip.
			// If it's outside that range, we aren't requesting anything special, so we just assume we'll keep the existing.

			if ( refinerEntry.requestedMip < texEntry.resident )
			{
				usedBudget += CRenderTextureBase::GetApproxSize( reg.m_textureBaseSize, refinerEntry.requestedMip );
			}

			if ( usedBudget > m_memoryLimit )
			{
				firstUnstreamableIndex = i;
				break;
			}
		}
	}

	if ( firstUnstreamableIndex < numTextures )
	{
		PC_SCOPE_PIX( TEXSTREAM_UnloadOverLimit );

		WARN_TEXSTREAM( TXT("Refiner was unable to make the textures fit! Going to force unload some textures.") );

		// Cancel streaming and release any hi-res textures on the out-of-budget stuff.
		Bool anyUnloaded = false;
		for ( Uint32 i = firstUnstreamableIndex; i < numTextures; ++i )
		{
			const STextureStreamingRefinerEntry& refinerEntry = m_context.m_refinerEntries[ i ];
			STextureStreamingUpdateEntry& texEntry = m_context.m_updateEntries[ refinerEntry.texEntry ];

			if ( texEntry.hasPending )
			{
				anyUnloaded = true;
				texEntry.texture->CancelStreaming();
			}

			if ( texEntry.hasHiRes )
			{
				anyUnloaded = true;
				// Add a new empty result so that when we apply the results of this update, the hi-res texture will be released.
				STextureStreamResults results;
				results.m_streamedMip = texEntry.resident;
				AddResults( texEntry.texture, std::move(results) );
			}

			// If a texture is being waited for and is over budget, treat it as canceled. We've got way too many textures, so this
			// will prevent stream requests from waiting indefinitely.
			// OnTextureCanceled is fine to call multiple times, if we hit also CancelStreaming above.
			if ( texEntry.hasWaitingRequest )
			{
				GetRenderer()->GetTextureStreamingManager()->OnTextureCanceled( texEntry.texture );
				texEntry.hasWaitingRequest = false;
			}
		}

		// If we unloaded anything, wait until next time to try loading new things. We haven't actually freed the memory yet,
		// that happens when flushing the job from the render thread, so we don't want to run out of memory because of that.
		if ( anyUnloaded )
		{
			// Mark as overloaded, so we don't think texture streaming is finished just because we had to unload something first.
			m_wasOverloaded = true;
			return false;
		}
	}
#ifdef RED_ASSERTS_ENABLED
	else
	{
		Uint32 refinerMipSize = 0;
		for ( const STextureStreamingRefinerEntry& refinerEntry : m_context.m_refinerEntries )
		{
			const STextureStreamingUpdateEntry& texEntry = m_context.m_updateEntries[ refinerEntry.texEntry ];
			const STextureStreamingRegistration& reg = m_context.m_registeredTextures[texEntry.textureIdx];
			if ( refinerEntry.requestedMip < texEntry.resident )
			{
				refinerMipSize += CRenderTextureBase::GetApproxSize( reg.m_textureBaseSize, refinerEntry.requestedMip );
			}
		}
		RED_HALT( "Out of budget after refiner, but didn't find textures to unload. Size from refiner: %u, and from unload: %u", refinerMipSize, usedBudget );
	}
#endif

	// Trim out anything we can't stream
	m_context.m_refinerEntries.ResizeFast( firstUnstreamableIndex );

	// Nothing was unloaded, so we can proceed to load things.
	return true;
}


void CTextureStreamingUpdateTask::FinishCurrentStreaming()
{
	PC_SCOPE_PIX( TEXSTREAM_FinishStreaming );

	const Uint32 numTextures = m_context.m_refinerEntries.Size();
	for ( Uint32 i = 0; i < numTextures; ++i )
	{
		STextureStreamingRefinerEntry& refinerEntry = m_context.m_refinerEntries[ i ];
		STextureStreamingUpdateEntry& texEntry = m_context.m_updateEntries[ refinerEntry.texEntry ];
		const STextureStreamingRegistration& reg = m_context.m_registeredTextures[ texEntry.textureIdx ];

		const Uint8 residentMip = texEntry.resident;
		const Uint8 currentMip = texEntry.current;

		const Uint8 requestedMip = refinerEntry.requestedMip;
		// Limit baseMip so it's >= requestedMip. Requested may have been pushed beyond base during the refine step.
		const Uint8 baseMip = Max( refinerEntry.baseMip, requestedMip );


		// If we're requesting resident or smaller, just unload the texture and cancel any pending.
		if ( requestedMip >= residentMip || requestedMip > reg.m_smallestLoadableMip )
		{
			if ( texEntry.hasPending )
			{
				texEntry.texture->CancelStreaming();
			}

			if ( texEntry.hasHiRes )
			{
				// Add a new empty result so that when we apply the results of this update, the hi-res texture will be released.
				STextureStreamResults results;
				results.m_streamedMip = texEntry.resident;
				AddResults( texEntry.texture, std::move(results) );
			}

			// If there's a request waiting for it, treat it as canceled to prevent stream requests from waiting indefinitely.
			// Requests will still maintain any locks on it, but they won't continue to wait on it.
			if ( texEntry.hasWaitingRequest )
			{
				GetRenderer()->GetTextureStreamingManager()->OnTextureCanceled( texEntry.texture );
				texEntry.hasWaitingRequest = false;
			}

			texEntry.hasPending = false;
			texEntry.hasHiRes = false;
			texEntry.current = texEntry.resident;
			texEntry.currentOrPending = texEntry.current;
		}
		// If it's already pending, check if it's done.
		else if ( texEntry.hasPending )
		{
			STextureStreamResults results;

			const Uint8 pendingMip = texEntry.currentOrPending;

			// If the pending is worse than both our requested mip and the current, just cancel it.
			if ( pendingMip > requestedMip && pendingMip >= currentMip )
			{
				texEntry.texture->CancelStreaming();
				texEntry.hasPending = false;
				texEntry.hasHiRes = false;
				texEntry.currentOrPending = texEntry.current;
			}
			// Also if the pending is better than our requested, cancel it. This should prevent us from accidentally going over
			// budget by loading something higher than what we've decided fits. We already started with requesting the pending,
			// so if requested is now lower, then we must have dropped down.
			else if ( pendingMip < requestedMip )
			{
				texEntry.texture->CancelStreaming();
				texEntry.hasPending = false;
				texEntry.hasHiRes = false;
				texEntry.currentOrPending = texEntry.current;
			}
			// Try to finish the pending stream.
			else if ( texEntry.texture->TryFinishStreaming( results ) )
			{
				RED_ASSERT( results.m_streamedMip >= refinerEntry.requestedMip, TXT("Finished streaming a texture with mip %u bigger than request %u"), results.m_streamedMip, refinerEntry.requestedMip );

				AddResults( texEntry.texture, std::move(results) );

				// We already have results for the texture, so we shouldn't need to start new streaming right now.
				// So adjust the texture/refiner entry so that nothing new will be started.
				texEntry.hasPending = false;
				texEntry.hasHiRes = !results.m_hiresTexture.isNull();
				refinerEntry.requestedMip = results.m_streamedMip;
				texEntry.current = results.m_streamedMip;
			}
			// Otherwise the pending mip is at an acceptable quality, let it continue.
		}
	}
}


Uint32 CTextureStreamingUpdateTask::TickAndStartNewStreaming()
{
	PC_SCOPE_PIX( TEXSTREAM_StartStreaming );

	// Sum up the current memory footprint of all streamed textures. This will be used to determine whether we can fit a given
	// request into budget -- Mip refinement will ensure that the entire set of mip selections will fit, but because we can't
	// necessarily apply the full set of selections at one time (because of in-flight budgets and I/O throttling), it's possible
	// for some intermediate state to be over budget:
	//    - for simplicity, start with stabilized textures, fitting and filling the budget.
	//    - in some update, several high-priority textures have a request larger than the current mip
	//    - in the same update, several low-priority textures have a request smaller than the current (to balance out and keep budget)
	//    - start loading from high-priority, until in-flight budget is reached. Don't get the the low-priority drops.
	//    - new streaming finishes before other textures can be dropped, and now over budget.
	// So to get around this, we keep a running total of how much memory is used by the current-or-newlyPending, and interleave
	// streaming in high priority and streaming out low priority.
	Uint32 accumulatedSize = 0;

	for ( const STextureStreamingRefinerEntry& refinerEntry : m_context.m_refinerEntries )
	{
		STextureStreamingUpdateEntry& texEntry = m_context.m_updateEntries[ refinerEntry.texEntry ];
		const STextureStreamingRegistration& reg = m_context.m_registeredTextures[ texEntry.textureIdx ];
		if ( texEntry.current < texEntry.resident )
		{
			accumulatedSize += CRenderTextureBase::GetApproxSize( reg.m_textureBaseSize, texEntry.current );
		}
	}

	Bool allowNewStreaming = true;

	Uint32 numEntries = m_context.m_refinerEntries.Size();
	for ( Uint32 i = 0; i < numEntries; ++i )
	{
		const auto& refinerEntry = m_context.m_refinerEntries[i];
		Uint32 amountToFree = TickAndStartStreamingTexture( refinerEntry, allowNewStreaming, accumulatedSize );
		if ( amountToFree > 0 )
		{
			const Uint32 targetSize = accumulatedSize - amountToFree;

			// Process streaming-drops until targetSize is reached, or we run out of entries.
			while ( accumulatedSize >= targetSize && i+1 < numEntries )
			{
				const auto& backRefinerEntry = m_context.m_refinerEntries[numEntries-1];
				const auto& backTexEntry = m_context.m_updateEntries[backRefinerEntry.texEntry];
				const Bool isDrop = backRefinerEntry.requestedMip >= backTexEntry.current;

				// We've run out of drops to process. In a perfect world, this wouldn't happen, since the dropped mips will
				// allow the rest of the textures to fit into budget. But if some of the drops weren't able to start, due to
				// in-flight budgets, then we could get through them all and still not have enough. In that case we just
				// break out and continue going through the refiner entries. Don't want to exit this function entirely, since
				// we do still want to tick existing streaming tasks.
				if ( !isDrop )
				{
					break;
				}

				TickAndStartStreamingTexture( backRefinerEntry, allowNewStreaming, accumulatedSize );
				--numEntries;
			}

			// If we managed to get within the target size, back up and try this entry again.
			if ( accumulatedSize <= targetSize )
			{
				--i;
			}
		}
	}

	m_wasOverloaded = !allowNewStreaming;

	return accumulatedSize;
}


Uint32 CTextureStreamingUpdateTask::TickAndStartStreamingTexture( const STextureStreamingRefinerEntry& refinerEntry, Bool& allowNewStreaming, Uint32& accumulatedSize )
{
	const STextureStreamingUpdateEntry& texEntry = m_context.m_updateEntries[ refinerEntry.texEntry ];
	const STextureStreamingRegistration& reg = m_context.m_registeredTextures[texEntry.textureIdx];

	const Uint8 residentMip = texEntry.resident;
	const Uint8 currentMip = texEntry.current;

	const Uint32 sizeOfCurrent = CRenderTextureBase::GetApproxSize( reg.m_textureBaseSize, currentMip );

	Uint8 requestedMip = refinerEntry.requestedMip;
	// HACK : If we're waiting for this texture and we don't have anything yet, push the request a little, so we can stop
	// waiting sooner. Only do this for large mips, so the resulting pop will be less visible.
	if ( m_allowMipDrop && texEntry.hasWaitingRequest && !texEntry.hasHiRes )
	{
		const Uint32 mipWidth = reg.m_baseWidth >> requestedMip;
		const Uint32 mipHeight = reg.m_baseHeight >> requestedMip;
		const Uint32 smallerDimension = Min( mipWidth, mipHeight );
		if ( smallerDimension > m_droppedMipSizeThreshold )
		{
			requestedMip = (Uint8)Min( requestedMip + 1, currentMip );
		}
	}

	// Limit baseMip so it's >= requestedMip. Requested may have been pushed beyond base during the refine step.
	const Uint8 baseMip = Max( refinerEntry.baseMip, requestedMip );


	// If the pending mip is at an acceptable quality, let it continue.
	if ( texEntry.hasPending && texEntry.currentOrPending <= baseMip )
	{
		texEntry.texture->TickStreaming( allowNewStreaming );

		// No memory difference, we're leaving it as-is.
		return 0;
	}


	const Uint8 smallestLoadableMip = reg.m_smallestLoadableMip;

	// Start streaming if it's bigger than resident. If it's smaller, then we would have already canceled it in FinishCurrentStreaming
	// Make sure we don't have too many textures in flight at once.
	if ( currentMip != requestedMip && requestedMip < residentMip && requestedMip <= smallestLoadableMip )
	{
		const Uint32 approxMipSize = CRenderTextureBase::GetApproxSize( reg.m_textureBaseSize, requestedMip );

		const Uint32 estimatedSizeAfterLoad = accumulatedSize - sizeOfCurrent + approxMipSize;

		// If we're increasing the texture's size, make sure it's not going to put us over budget.
		if ( approxMipSize > sizeOfCurrent && estimatedSizeAfterLoad > m_memoryLimit )
		{
			return estimatedSizeAfterLoad - m_memoryLimit;
		}

		if ( CanStreamTextureThisFrame( approxMipSize ) )
		{
			// Streaming priority - locked textures get bumped up. non-locked use the current default for the texture category.
			const Int8 priorityBias	= GetPriorityBias( (ETextureCategory)texEntry.category, texEntry.hasWaitingRequest );
			const Int8 priority		= reg.m_streamingPriority + priorityBias;

			// Request streaming for texture. If this mip is already pending, StartStreaming does nothing.
			if ( texEntry.texture->StartStreaming( requestedMip, priority, allowNewStreaming ) )
			{
				accumulatedSize = estimatedSizeAfterLoad;
			}
		}
	}

	return 0;
}


void CTextureStreamingUpdateTask::ProcessStreaming()
{
	PC_SCOPE_PIX( TEXSTREAM_ProcessStreaming );


	// Nothing for us to stream
	if ( m_context.m_updateEntries.Empty() )
	{
		return;
	}

	// Create refiner entries for each of the textures. These specify which mips we'd like to load and what the minimum/maximum
	// "acceptable" level of detail are. These can be used later by the refiner if we need to make our textures fit into memory.
	Uint32 accumulatedSize = FillRefinerEntries();

#ifndef RED_FINAL_BUILD
	m_statsContainer->stats.m_initialRequestedSize = accumulatedSize;
#endif


	// Sort the refiner entries according to their textures' sort order. So higher priority textures will be less likely to be dropped
	// by the refiner, and will have first chance to start streaming.
	{
		PC_SCOPE_PIX( TEXSTREAM_SortByImportance );
		const Uint32 numTextures = m_context.m_updateEntries.Size();
		m_context.m_sortKeys.ResizeFast( numTextures );
		{
			PC_SCOPE_PIX( TEXSTREAM_GenSortKeys );
			for ( Uint32 i = 0; i < numTextures; ++i )
			{
				const STextureStreamingUpdateEntry& entry = m_context.m_updateEntries[i];
				RED_ASSERT( entry.streamingOrder < 128 );
				RED_ASSERT( entry.currentOrPending < 64 );

				// WARNING: Here be dragons
				m_context.m_sortKeys[i] = 0
					| ( Uint32( 127 - ( entry.streamingOrder & 0x7f ) ) << 22 )
					| ( Uint32( entry.currentOrPending & 0x3f ) << 16 )
					| ( Uint32( entry.textureIdx ) );
			}
		}
		const Uint32* sortKeysPtr = m_context.m_sortKeys.TypedData();
		Sort( m_context.m_refinerEntries.Begin(), m_context.m_refinerEntries.End(), [sortKeysPtr]( const STextureStreamingRefinerEntry& a, const STextureStreamingRefinerEntry& b )
		{
			return sortKeysPtr[a.texEntry] < sortKeysPtr[b.texEntry];
		} );
	}

	accumulatedSize = RefineMipRequests( accumulatedSize );

#ifndef RED_FINAL_BUILD
	UpdateTexturesStats(m_context.m_refinerEntries);
#endif


	// Sort again, this time according to the order we want to load things. This will put all textures with requests waiting
	// on them first, with highest preference to those that have not been streamed at all.
	{
		PC_SCOPE_PIX( TEXSTREAM_SortByLoadOrder );
		// Patch in some more info to sort into load order.
		// NOTE: We could almost get away with filling these in the first pass, and just masking it out during the first sort, except that isDrop depends on the refinement results
		const Uint32 numTextures = m_context.m_refinerEntries.Size();
		{
			PC_SCOPE_PIX( TEXSTREAM_GenSortKeys );
			for ( Uint32 i = 0; i < numTextures; ++i )
			{
				STextureStreamingRefinerEntry& refinerEntry = m_context.m_refinerEntries[i];
				const STextureStreamingUpdateEntry& entry = m_context.m_updateEntries[refinerEntry.texEntry];

				const Bool isDrop = refinerEntry.requestedMip > entry.current;

				// WARNING: Here be dragons
				m_context.m_sortKeys[refinerEntry.texEntry] |= 0
					| ( Uint32( isDrop ) << 31 )
					| ( Uint32( !entry.hasWaitingRequest ) << 30 )
					| ( Uint32( entry.hasHiRes & entry.hasWaitingRequest ) << 29 );
			}
		}
		const Uint32* sortKeysPtr = m_context.m_sortKeys.TypedData();
		Sort( m_context.m_refinerEntries.Begin(), m_context.m_refinerEntries.End(), [sortKeysPtr]( const STextureStreamingRefinerEntry& a, const STextureStreamingRefinerEntry& b )
		{
			return sortKeysPtr[a.texEntry] < sortKeysPtr[b.texEntry];
		} );
	}

	// After refining the mip requests, check if we need to clear out any old textures to make it fit.
	if ( !UnloadOverBudget( accumulatedSize ) )
	{
		return;
	}

	// Nothing needed to be unloaded, so we can start loading some new stuff!

	// First let any existing streaming tasks finish. That way maybe we can make room for new ones.
	FinishCurrentStreaming();

	// Track whether we should try to start new streaming tasks. If IO gets saturated, we should stop pushing more.
	accumulatedSize = TickAndStartNewStreaming();

#ifndef RED_FINAL_BUILD
	m_statsContainer->stats.m_finalRequestedSize = accumulatedSize;
#endif
}


void CTextureStreamingUpdateTask::ApplyResults()
{
	RED_ASSERT( IsFinished() );

	if ( !m_context.m_results.Empty() )
	{
		PC_SCOPE_PIX( TEXSTREAM_ApplyResults );

		for ( const auto& it : m_context.m_results )
		{
			CRenderTextureBase* texture = it.m_first;
			const STextureStreamResults& results = it.m_second;

			texture->ApplyStreamingResults( results );
			texture->Release();
		}
		m_context.m_results.Clear();
	}
}

#ifndef RED_FINAL_BUILD

void CTextureStreamingUpdateTask::UpdateTexturesStats( const TDynArray< STextureStreamingRefinerEntry > & refinerEntries )
{
	Red::Threads::CScopedLock< Red::Threads::CSpinLock > lock( m_statsContainer->lock );
	m_statsContainer->stats.m_textures.Resize( refinerEntries.Size() );
	// Gather stats about the texture's we're working with.
	Uint32 index = 0;
	for ( const STextureStreamingRefinerEntry& refinerEntry : refinerEntries )
	{
		const STextureStreamingUpdateEntry& texEntry = m_context.m_updateEntries[ refinerEntry.texEntry ];
		if ( texEntry.distance < m_context.m_maxStreamingDistance[texEntry.category] || texEntry.hasHiRes || texEntry.hasPending )
		{
			STextureStreamingTexStat & stat = m_statsContainer->stats.m_textures[ index++ ];
			stat.m_textureName		= texEntry.texture->GetDepotPath();
			stat.m_streamingOrder	= texEntry.streamingOrder;
			stat.m_baseMip			= refinerEntry.baseMip;
			stat.m_currentMip		= texEntry.texture->GetStreamingMipIndex();
			stat.m_pendingMip		= texEntry.hasPending ? texEntry.texture->GetPendingMip() : -1;
			stat.m_requestedMip		= refinerEntry.requestedMip;
			stat.m_originalRequestedMip = refinerEntry.originalRequestedMip;
			stat.m_hasLock			= texEntry.hasLock;
		}
	}
}

#endif


//////////////////////////////////////////////////////////////////////////


CRenderTextureStreamingManager::CRenderTextureStreamingManager()
	: m_requestedCinematicMode( false )
	, m_cinematicMode( false )
	, m_updateTask( nullptr )
#ifndef NO_DEBUG_PAGES
	, m_lastStreamingTaskTime( 0.0f )
#endif
	, m_forceStillStreaming( false )
	, m_totalDataLoaded( 0 )
	, m_framesToResumeTextureStreaming( 0 )
	, m_numTextureStreamingTasksRunning( 0 )
	, m_forceStopped( false )
	, m_allowPrefetchMipDrop( true )
{
	UpdateMaxStreamingDistances();

	m_streamingContext.Reset( new STextureStreamingContext() );
}

CRenderTextureStreamingManager::~CRenderTextureStreamingManager()
{
	// Finish the update task. Don't use TryFinishUpdateTask, because we don't really want to apply any results or anything.
	// Just need to finish the task and release it.
	if ( m_updateTask != nullptr )
	{
		if ( !m_updateTask->IsFinished() )
		{
			if ( !TryRunUpdateTaskLocally() )
			{
				PC_SCOPE_PIX( TEXSTREAM_WaitingForUpdateTask );
				while ( !m_updateTask->IsFinished() ) {RED_BUSY_WAIT();}
			}
		}

		m_updateTask->Release();
		m_updateTask = nullptr;
	}

	// Cancel all streaming. We don't want any remaining streaming tasks, otherwise if a texture is destroyed after the streaming
	// manager, then it may try to cancel a streaming task, which will notify the (deleted) streaming manager. Since the update
	// task is the only place that starts streaming tasks, we know that after this no texture can have a streaming task.
	CancelTextureStreaming();

	for ( CRenderTextureStreamRequest* request : m_streamRequests )
	{
		request->Release();
	}

	for ( CRenderFramePrefetch* prefetch : m_finishedFramePrefetches )
	{
		prefetch->Release();
	}
	for ( CRenderFramePrefetch* prefetch : m_newFramePrefetches )
	{
		prefetch->Release();
	}
}




Uint16 CRenderTextureStreamingManager::RegisterTexture( CRenderTextureBase* texture )
{
	RED_FATAL_ASSERT( texture != nullptr, "Cannot register a null texture" );

	if ( !texture->HasStreamingSource() || !texture->GetStreamingSource()->IsReady() )
	{
		return 0;
	}

	// If texture streaming has been stopped, don't allow any new registrations.
	if ( m_forceStopped )
	{
		return 0;
	}

	STextureStreamingContext::TScopedLock lock( m_streamingContext->m_registeredTexturesMutex );

	Uint16 index;
	if ( m_streamingContext->m_registeredTexturesFreeIndex < m_streamingContext->m_registeredTextures.Size() )
	{
		index = m_streamingContext->m_registeredTexturesFreeIndex;
		m_streamingContext->m_registeredTexturesFreeIndex = m_streamingContext->m_registeredTextures[index].m_nextFree;
	}
	else
	{
		index = m_streamingContext->m_registeredTextures.Size();
		m_streamingContext->m_registeredTextures.Grow(1);
		m_streamingContext->m_registeredTexturesFreeIndex = 0xffff;
	}

	RED_ASSERT( m_streamingContext->m_registeredTextures[index].m_isFree, TXT("TextureRegistration is not free") );
	m_streamingContext->m_registeredTextures[index].InitWithTexture( texture );

	return index + 1;
}

void CRenderTextureStreamingManager::UpdateTexture( Uint16 key )
{
	if ( key == 0 )
	{
		return;
	}

	STextureStreamingContext::TScopedLock lock( m_streamingContext->m_registeredTexturesMutex );

	const Uint16 index = key - 1;

	RED_ASSERT( !m_streamingContext->m_registeredTextures[index].m_isFree, TXT("TextureRegistration is already free") );

	m_streamingContext->m_registeredTextures[index].Update();
}




void CRenderTextureStreamingManager::UpdateMaxStreamingDistances()
{
	m_maxStreamingDistancePerCategory[ eTextureCategory_Generic ]		= Config::cvTextureStreamingDistanceLimitSq.Get();
	m_maxStreamingDistancePerCategory[ eTextureCategory_World ]			= Config::cvTextureStreamingDistanceLimitSq.Get();
	m_maxStreamingDistancePerCategory[ eTextureCategory_Scene ]			= Config::cvTextureStreamingDistanceLimitSq.Get();
	m_maxStreamingDistancePerCategory[ eTextureCategory_Characters ]	= Config::cvTextureStreamingCharacterDistanceLimitSq.Get();
	m_maxStreamingDistancePerCategory[ eTextureCategory_Heads ]			= Config::cvTextureStreamingHeadsDistanceLimitSq.Get();

	static_assert( eTextureCategory_MAX == 5, "Texture categories have changed. Probably need to update this" );
}


Bool CRenderTextureStreamingManager::IsCurrentlyStreaming() const
{
	// If the last update task was overloaded from IO, we still count that as currently streaming. It just means
	// we probably weren't able to start something new, even if we wanted it. 
	return m_forceStillStreaming.GetValue() || m_numTextureStreamingTasksRunning.GetValue() > 0 || HasNewFinishedFramePrefetch();
}


void CRenderTextureStreamingManager::AddFinishedFramePrefetch( CRenderFramePrefetch* prefetch )
{
	TScopedLock lock( m_streamRequestsMutex );

	// Add to the newly finished list.
	RED_ASSERT( prefetch != nullptr, TXT("Trying to add a null frame prefetch") );
	if ( prefetch != nullptr && m_newFramePrefetches.Insert( prefetch ) )
	{
		prefetch->AddRef();
	}
}

void CRenderTextureStreamingManager::RemoveFinishedFramePrefetch( CRenderFramePrefetch* prefetch )
{
	RED_ASSERT( prefetch != nullptr, TXT("Trying to remove a null frame prefetch") );
	if ( prefetch != nullptr )
	{
		TScopedLock lock( m_streamRequestsMutex );

		// Remove from finished prefetches.
		m_finishedFramePrefetches.Erase( prefetch );

		// Also check if we should remove from new prefetches. Release references to these ones.
		if ( m_newFramePrefetches.Erase( prefetch ) )
		{
			prefetch->Release();
		}
	}
}


Bool CRenderTextureStreamingManager::HasNewFinishedFramePrefetch() const
{
	TScopedLock lock( m_streamRequestsMutex );

	return !m_newFramePrefetches.Empty();
}


Bool CRenderTextureStreamingManager::TryFinishUpdateTask( Bool force, Bool applyResults )
{
	if ( m_updateTask == nullptr )
	{
		return true;
	}

	if ( !m_updateTask->IsFinished() && force )
	{
		// If we can't run now, it's already in-progress and we can wait for it.
		if ( !TryRunUpdateTaskLocally() )
		{
			PC_SCOPE_PIX( TEXSTREAM_WaitingForUpdateTask );
			while ( !m_updateTask->IsFinished() ) {RED_BUSY_WAIT();}
		}
	}

	if ( m_updateTask->IsFinished() )
	{
		// If !applyResults, then they'll just carry over to the next time.
		if ( applyResults )
		{
			m_updateTask->ApplyResults();
		}

		// If the IO was overloaded, we want to claim that streaming is still happening. Otherwise we've had a chance to get anything
		// new started, and if there is anything to be streamed, it will already be started.
		m_forceStillStreaming.SetValue( m_updateTask->WasOverloaded() );

#ifndef NO_DEBUG_PAGES
		m_lastStreamingTaskTime = m_updateTask->GetProcessingTime();
#endif

		m_updateTask->Release();
		m_updateTask = nullptr;

		return true;
	}

	return false;
}


void CRenderTextureStreamingManager::EnsureUpdateTaskIsSafe()
{
	if ( m_updateTask != nullptr )
	{
		m_updateTask->FinishUnsafePart();
	}
}


Bool CRenderTextureStreamingManager::TryRunUpdateTaskLocally()
{
	RED_ASSERT( m_updateTask != nullptr, TXT("TryRunUpdateTaskLocally called with no update task! How can this be?") );

	// I couldn't just use CTaskBatch here, because we've already started the task at this point. Could keep one as a member,
	// and fire off a batch instead of the task directly in UpdateTextureStreaming, but it seems like the task batch will run
	// some tasks locally even with the non-AndWait version. There are places using Issue and IssueAndWait, and I don't want
	// to risk breaking anything.

	CTaskDispatcher taskDispatcher( *GTaskManager );
	CTaskRunner taskRunner;
	taskRunner.RunTask( *m_updateTask, taskDispatcher );

	// Return whether the task is finished. It might not be, if it was already on a task thread. In that case, RunTask did nothing.
	return m_updateTask->IsFinished();
}


Bool CRenderTextureStreamingManager::HasActiveUpdate() const
{
	// Even if we have a task and it's finished, it may have some results to apply, so we still consider it active.
	return m_updateTask != nullptr;
}


void CRenderTextureStreamingManager::UpdateTextureStreaming( Bool allowTextureExpire )
{
	PC_SCOPE_PIX( TEXSTREAM_UpdateTextureStreaming );

	if ( m_framesToResumeTextureStreaming > 0 )
	{
		--m_framesToResumeTextureStreaming;
		return;
	}

	// Check if we can finish the update task. If not, we just let it continue.
	if ( !TryFinishUpdateTask( false, true ) )
	{
		return;
	}

	// Unstream all textures
	if ( Config::cvForceUnstreamTextures.Get() )
	{
		UnloadTextures( RTUM_All );
		return;
	}


	// Apply any finished prefetches we have.
	// NOTE : This should be done between update tasks! Here's why:
	//    Imagine we have a prefetch with some new texture in it (maybe even only new textures, as in loading screen).
	//    We apply the results while the update task is still running.
	//    So this creates a new texture stream request with these new textures.
	//    Now, the update task is processing textures, sees a texture is unused, and has a waiting request (because the request has been made).
	//    So it sends notification that the texture is cancelled. Request removes it from the pending list, and no longer waits on it.
	//    Prefetch may say it's done now, even though that texture has just been unloaded (or was never even started).
	// If we do it between updates, then we know that the next update will have the appropriate prefetch distances set, and
	// our requests will wait properly.
	{
		PC_SCOPE_PIX( TEXSTREAM_ApplyFramePrefetches );

		TScopedLock lock( m_streamRequestsMutex );
		for ( CRenderFramePrefetch* prefetch : m_finishedFramePrefetches )
		{
			prefetch->ApplyTextureResults();
		}

		// If we have new prefetches, make sure we claim we're active at least until the next update task finishes, so the results of
		// them can be taken into consideration.
		if ( !m_newFramePrefetches.Empty() )
		{
			m_forceStillStreaming.SetValue( true );
		}

		// Also apply the new prefetches. Move them to the finished list and release our reference when done. If they get destroyed,
		// they'll automatically remove themselves.
		// Have to move the prefetches out of m_newFramePrefetches, otherwise we may end up double-releasing them (Release here, their
		// destructor unregisters them, which see it still in m_newFramePrefetches, and releases while it's being destroyed).
		THashSet< CRenderFramePrefetch* > oldNewFramePrefetches( Move( m_newFramePrefetches ) );
		RED_ASSERT( m_newFramePrefetches.Empty(), TXT("m_newFramePrefetches not empty after moving!") );
		for ( CRenderFramePrefetch* prefetch : oldNewFramePrefetches )
		{
			// If it's not already in the finished list, apply it now. If it's in the finished list, it's already been applied.
			if ( m_finishedFramePrefetches.Insert( prefetch ) )
			{
				prefetch->ApplyTextureResults();
			}
			// This may cause the prefetch to be destroyed. In that case, it will remove itself from m_finishedFramePrefetches.
			prefetch->Release();
		}
	}

	// Force cinematic mode
	if ( Config::cvForceCinematicModeOn.Get() )
		m_requestedCinematicMode = true;
	else if ( Config::cvForceCinematicModeOff.Get() )
		m_requestedCinematicMode = false;

	// Cinematic mode change
	if ( m_requestedCinematicMode != m_cinematicMode )
	{
		m_cinematicMode = m_requestedCinematicMode;
		LOG_TEXSTREAM( TXT("Texture streaming cinematic mode: %ls"), m_cinematicMode ? TXT("ON") : TXT("OFF") );
	}

	UpdateMaxStreamingDistances();

	// Don't start new task if we've been stopped.
	if ( m_forceStopped )
	{
		return;
	}


	if ( GTaskManager != nullptr )
	{
		// Start new update task.
		m_updateTask = new (CTask::Root) CTextureStreamingUpdateTask( allowTextureExpire, m_cinematicMode, m_maxStreamingDistancePerCategory, *m_streamingContext );
#ifndef RED_FINAL_BUILD
		m_updateTask->m_statsContainer = &m_statsContainer;
#endif
		GTaskManager->Issue( *m_updateTask );
	}
}

void CRenderTextureStreamingManager::StopFurtherStreaming()
{
	// Cancel and unload all.
	CancelTextureStreaming( false );
	// Prevent any new update tasks.
	m_forceStopped = true;

	// Clear all texture registrations. Registration destructor will release the textures.
	{
		STextureStreamingContext::TScopedLock lock( m_streamingContext->m_registeredTexturesMutex );
		m_streamingContext->m_registeredTextures.Clear();
		m_streamingContext->m_registeredTexturesFreeIndex = 0;
	}

	LOG_RENDERER( TXT("Texture Streaming has been shutdown.") );
}


void CRenderTextureStreamingManager::CancelTextureStreaming( Bool flushOnlyUnused )
{
	RED_PROFILING_TIMER( timer );

	// Log
	LOG_TEXSTREAM( TXT( "Canceling texture streaming" ) );

	// Make sure there's no update in progress.
	TryFinishUpdateTask( true, false );

	UnloadTextures( flushOnlyUnused ? RTUM_NotVisible : RTUM_All );

	// Stats
	RED_PROFILING_TIMER_LOG( TXT("Texture streaming canceled in %1.2fs"), timer.GetDelta() );
}

void CRenderTextureStreamingManager::EnableCinematicMode( const Bool cinematicMode )
{
	m_requestedCinematicMode = cinematicMode;
}


void CRenderTextureStreamingManager::UnloadTextures( ERenderTextureUnloadMethod unloadMethod )
{
	RED_FATAL_ASSERT( !HasActiveUpdate(), "UnloadTexturesOverBudget called while active update" );

	if ( unloadMethod == RTUM_All )
	{
		// Unload and cancel all streaming.
		for ( TRenderResourceIterator< CRenderTextureBase > it; it; ++it )
		{
			CRenderTextureBase* tex = *it;
			tex->CancelStreaming();
			tex->UnstreamHiRes();
		}
	}
	else if( unloadMethod == RTUM_NotVisible )
	{
		// Unload and cancel streaming on textures that are not visible (i.e. beyond max streaming distance)
		for ( TRenderResourceIterator< CRenderTextureBase > it; it; ++it )
		{
			CRenderTextureBase* tex = *it;

			const Float curDistance = tex->GetLastBindDistance();
			const Float maxDistance = m_maxStreamingDistancePerCategory[ tex->GetTextureCategory() ];

			// Don't unload locked textures. If we absolutely have to, we'll hit the unload in the update task.
			if ( tex->HasStreamingLock() )
			{
				continue;
			}


			if ( curDistance > maxDistance )
			{
				tex->CancelStreaming();
				tex->UnstreamHiRes();
			}
		}
	}
	else
	{
		RED_HALT( "Unknown unload method" );
	}
}


void CRenderTextureStreamingManager::OnNewTextureCacheAttached()
{
	// Finish current update task, so we won't be reading from any active registrations.
	TryFinishUpdateTask( true, false );

	// Lock registered textures, so if multiple textures end up registering themselves
	STextureStreamingContext::TScopedLock lock( m_streamingContext->m_registeredTexturesMutex );

	// Let textures refresh their streaming sources, in case they can be made ready with this new cache
	for ( TRenderResourceIterator< CRenderTextureBase > it; it; ++it )
	{
		it->OnNewTextureCache();
	}
}


void CRenderTextureStreamingManager::TickDuringSuppressedRendering()
{
	// Need to wrap in Begin/EndRender here to ensure we can safely finish streaming tasks.
	// Don't gradually expire textures, since we aren't actively updating their current distances.

	GpuApi::BeginRender();
	UpdateTextureStreaming( false );
	GpuApi::EndRender();
}

void CRenderTextureStreamingManager::ForceUnloadUnused()
{
	TryFinishUpdateTask( true, false );
	UnloadTextures( CRenderTextureStreamingManager::RTUM_NotVisible );
}


void CRenderTextureStreamingManager::RegisterTextureRequest( CRenderTextureStreamRequest* request )
{
	if ( request != nullptr )
	{
		TScopedLock lock( m_streamRequestsMutex );

		request->AddRef();
		RED_ASSERT( !m_streamRequests.Exist( request ), TXT("TextureStreamRequest registered twice! Not the end of the world, but wasteful.") );
		m_streamRequests.PushBack( request );
	}
}

void CRenderTextureStreamingManager::UnregisterTextureRequest( CRenderTextureStreamRequest* request )
{
	if ( request != nullptr )
	{
		TScopedLock lock( m_streamRequestsMutex );

		if ( m_streamRequests.RemoveFast( request ) )
		{
			request->Release();
		}
	}
}

void CRenderTextureStreamingManager::OnTextureStreamed( CRenderTextureBase* texture )
{
	RED_ASSERT( texture != nullptr, TXT("Shouldn't get notification from null texture") );
	if ( texture != nullptr && texture->HasWaitingRequest() )
	{
		TScopedLock lock( m_streamRequestsMutex );

		// Notify all streaming requests of this texture. Go back-to-front so we can easily remove completed requests as we go.
		for ( Int32 i = m_streamRequests.SizeInt() - 1; i >= 0; --i )
		{
			CRenderTextureStreamRequest* request = m_streamRequests[i];
			if ( request->OnTextureStreamed( texture ) )
			{
				m_streamRequests.RemoveAtFast( i );
				request->Release();
			}
		}
	}
}

void CRenderTextureStreamingManager::OnTextureCanceled( CRenderTextureBase* texture )
{
	RED_ASSERT( texture != nullptr, TXT("Shouldn't get notification from null texture") );
	if ( texture != nullptr && texture->HasWaitingRequest() )
	{
		TScopedLock lock( m_streamRequestsMutex );

		// Notify all streaming requests of this texture. Go back-to-front so we can easily remove completed requests as we go.
		for ( Int32 i = m_streamRequests.SizeInt() - 1; i >= 0; --i )
		{
			CRenderTextureStreamRequest* request = m_streamRequests[i];
			if ( request->OnTextureCanceled( texture ) )
			{
				m_streamRequests.RemoveAtFast( i );
				request->Release();
			}
		}
	}
}

void CRenderTextureStreamingManager::OnTextureUnloaded( CRenderTextureBase* texture )
{
	RED_ASSERT( texture != nullptr, TXT("Shouldn't get notification from null texture") );
	if ( texture != nullptr && texture->HasWaitingRequest() )
	{
		TScopedLock lock( m_streamRequestsMutex );

		// Notify all streaming requests of this texture. Go back-to-front so we can easily remove completed requests as we go.
		for ( Int32 i = m_streamRequests.SizeInt() - 1; i >= 0; --i )
		{
			CRenderTextureStreamRequest* request = m_streamRequests[i];
			if ( request->OnTextureCanceled( texture ) )
			{
				m_streamRequests.RemoveAtFast( i );
				request->Release();
			}
		}
	}
}

#ifndef RED_FINAL_BUILD

void CRenderTextureStreamingManager::GetLastUpdateStats( STextureStreamingStats & result ) const
{
	Red::Threads::CScopedLock< Red::Threads::CSpinLock > lock( m_statsContainer.lock );
	result = m_statsContainer.stats;
}
#endif


//////////////////////////////////////////////////////////////////////////


void CRenderInterface::ShutdownTextureStreaming()
{
	RED_ASSERT( SIsMainThread(), TXT("ShutdownTextureStreaming must be called from main thread, during shutdown!") );

	// This comes in from the main thread. Just fire off the render command that will actually do the work, and wait
	// for it to finish.

	( new CRenderCommand_ShutdownTextureStreaming() )->Commit();
	Flush();
}

#ifndef RED_FINAL_BUILD
GeneralStats CRenderInterface::GetGeneralTextureStats( GeneralStats& st)
{
	CName worldSpecular = CNAME(WorldSpecular);
	CName worldDiffuse = CNAME(WorldDiffuse);
	CName worldDiffuseAlpha = CNAME(WorldDiffuseWithAlpha);
	CName normalmapGloss = CNAME(NormalmapGloss);

	CName charDiffuse = CNAME(CharacterDiffuse);
	CName charDiffuseAlpha = CNAME(CharacterDiffuseWithAlpha);
	CName charEmissive = CNAME(CharacterEmissive);
	CName charNormalmapGloss = CNAME(CharacterNormalmapGloss);
	CName headDiffuse = CNAME(HeadDiffuse);
	CName headDiffuseAlpha = CNAME(HeadDiffuseWithAlpha);
	CName headNormal = CNAME(HeadNormal);

	for ( TRenderResourceIterator< CRenderTextureBase > it; it; ++it )
	{
		CRenderTextureBase* tex = *it;

		CName name = tex->GetTextureGroupName();
		if ( name == worldSpecular || name == worldDiffuse || name == worldDiffuseAlpha || name == normalmapGloss )
		{
			st.m_environmentMemory += tex->GetCachedUsedVideoMemory();
		}
		else if ( name == charDiffuse || name == charDiffuseAlpha || name == charEmissive || name == charNormalmapGloss || name == headDiffuse || name == headDiffuseAlpha || name == headNormal )
		{
			st.m_characterMemory += tex->GetCachedUsedVideoMemory();
		}
	}
	return st;
}
#endif
