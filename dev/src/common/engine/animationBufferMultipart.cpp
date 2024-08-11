/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "animationBuffer.h"
#include "animationBufferMultipart.h"
#include "../redMath/redmathbase.h"

IMPLEMENT_ENGINE_CLASS(CAnimationBufferMultipart);

CAnimationBufferMultipart::CAnimationBufferMultipart()
{
}

CAnimationBufferMultipart::~CAnimationBufferMultipart()
{
	m_parts.ClearPtr();
}

void CAnimationBufferMultipart::OnSerialize( IFile& file )
{
	IAnimationBuffer::OnSerialize(file);

	if ( file.IsGarbageCollector() )
	{
		file << m_parts;
	}
}

void CAnimationBufferMultipart::Preload()
{
	for ( Uint32 i=0; i<m_parts.Size(); ++i )
	{
		if ( NULL != m_parts[i] )
		{
			m_parts[i]->Preload();
		}
	}
}

void CAnimationBufferMultipart::SyncLoad()
{
	for ( Uint32 i=0; i<m_parts.Size(); ++i )
	{
		if ( NULL != m_parts[i] )
		{
			m_parts[i]->SyncLoad();
		}
	}
}

EAnimationBufferDataAvailable CAnimationBufferMultipart::GetAnimationBufferDataAvailable( Uint32 bonesRequested, Uint32 & outBonesLoaded, Uint32 & outBonesAlwaysLoaded )
{
	Bool loaded = true;
	outBonesLoaded = (Uint32) -1;
	EAnimationBufferDataAvailable abda = ABDA_All;
	for ( Uint32 i=0; i<m_parts.Size(); ++i )
	{
		if ( NULL != m_parts[i] )
		{
			Uint32 bonesLoaded;
			Uint32 bonesAlwaysLoaded;
			EAnimationBufferDataAvailable abdaAnim = m_parts[i]->GetAnimationBufferDataAvailable( bonesRequested, bonesLoaded, bonesAlwaysLoaded );
			abda = Min( abda, abdaAnim );
			outBonesLoaded = Min( outBonesLoaded, bonesLoaded );
			outBonesAlwaysLoaded = Min( outBonesAlwaysLoaded, bonesAlwaysLoaded );
		}
	}
	return abda;
}

Bool CAnimationBufferMultipart::SetStreamingOption( SAnimationBufferStreamingOption streamingOption, Uint32 nonStreamableBones )
{
	Bool retVal = false;
	for ( Uint32 i=0; i<m_parts.Size(); ++i )
	{
		if ( NULL != m_parts[i] )
		{
			retVal |= m_parts[i]->SetStreamingOption( streamingOption, nonStreamableBones );
		}
	}
	return retVal;
}

void CAnimationBufferMultipart::AddUsage( DEBUG_ANIMATION_USAGE_PARAM_LIST )
{
	for ( Uint32 i=0; i<m_parts.Size(); ++i )
	{
		if ( NULL != m_parts[i] )
		{
			m_parts[i]->AddUsage( DEBUG_ANIMATION_USAGE_PARAM_NAME );
		}
	}
}

void CAnimationBufferMultipart::ReleaseUsage()
{
	for ( Uint32 i=0; i<m_parts.Size(); ++i )
	{
		if ( NULL != m_parts[i] )
		{
			m_parts[i]->ReleaseUsage();
		}
	}
}

Uint32 CAnimationBufferMultipart::GetFramesNum() const
{
	return m_numFrames;
}

Uint32 CAnimationBufferMultipart::GetBonesNum() const
{
	return m_numBones;
}

Uint32 CAnimationBufferMultipart::GetTracksNum() const
{
	return m_numTracks;
}

Uint32 CAnimationBufferMultipart::GetPartsNum() const
{
	return m_parts.Size();
}

IAnimationBuffer const * CAnimationBufferMultipart::GetPart(Uint32 partIndex) const
{
	if ( partIndex < m_parts.Size())
	{
		return m_parts[partIndex];
	}

	return nullptr;
}

Bool CAnimationBufferMultipart::IsCompressed() const
{
	if ( !m_parts.Empty() && m_parts[0])
	{
		return m_parts[0]->IsCompressed();
	}

	return false;
}

void CAnimationBufferMultipart::GetMemorySize(Uint32 & outNonStreamable, Uint32 & outStreamableLoaded, Uint32 & outStreamableTotal, Uint32 & outSourceDataSize) const
{
	outNonStreamable = sizeof(CAnimationBufferMultipart);

	outNonStreamable += (Uint32)m_parts.DataSize();
	outNonStreamable += (Uint32)m_firstFrames.DataSize();

	outStreamableLoaded = 0;
	outStreamableTotal = 0;

	for ( Uint32 i=0; i<m_parts.Size(); ++i )
	{
		if ( NULL != m_parts[i] )
		{
			Uint32 partNonStreamable;
			Uint32 partStreamableLoaded;
			Uint32 partStreamableTotal;
			Uint32 partSourceDataSize;
			m_parts[i]->GetMemorySize( partNonStreamable, partStreamableLoaded, partStreamableTotal, partSourceDataSize );
			outNonStreamable += partNonStreamable;
			outStreamableLoaded += partStreamableLoaded;
			outStreamableTotal += partStreamableTotal;
			outSourceDataSize += partSourceDataSize;
		}
	}
}

Bool CAnimationBufferMultipart::Initialize( const CSkeletalAnimationSet* animSet, CSkeletalAnimation* animation, const SourceData& sourceData, IAnimationBuffer* previousAnimBuffer, Bool preferBetterQuality )
{
	const Uint32 numParts = sourceData.m_parts.Size();

	// allocate the part arrays
	Uint32 numNonEmptyParts = 0;
	Uint32 currentFrame = 0;
	for ( Uint32 i=0; i<numParts; ++i )
	{
		const Uint32 numFramesInPart = sourceData.m_parts[i].m_numFrames;

		// skip empty parts (happened once when importing from Havok)
		if ( numFramesInPart == 0 )
		{
			continue;
		}

		currentFrame  += numFramesInPart;
		++numNonEmptyParts;
	}

	// make sure we have at least one part
	if ( numNonEmptyParts == 0 || currentFrame == 0 )
	{
		WARN_ENGINE( TXT("There are no animation parts in animation source data. Unable to initialize.") );
		return false;
	}

	// initialize frames
	TDynArray< IAnimationBuffer* > animationBuffers;
	for ( Uint32 i=0; i<numParts; ++i )
	{
		const Uint32 numFramesInPart = sourceData.m_parts[i].m_numFrames;
		if ( numFramesInPart == 0 )
		{
			continue;
		}

		// prepare import data for importing a single animation part
		IAnimationBuffer::SourceData partSourceData;
		partSourceData.m_numBones = sourceData.m_numBones;
		partSourceData.m_numTracks = sourceData.m_numTracks;
		partSourceData.m_parts.Resize(1);
		partSourceData.m_parts[0].m_numFrames = sourceData.m_parts[i].m_numFrames;
		partSourceData.m_parts[0].m_bones = sourceData.m_parts[i].m_bones;
		partSourceData.m_parts[0].m_track = sourceData.m_parts[i].m_track;

		// create animation buffer
		IAnimationBuffer* previousBuffer = NULL;
		if ( const CAnimationBufferMultipart* previousBufferMultipart = Cast< CAnimationBufferMultipart >( previousAnimBuffer ) )
		{
			if ( previousBufferMultipart->m_parts.Size() == numParts )
			{
				previousBuffer = previousBufferMultipart->m_parts[i];
			}
		}

		IAnimationBuffer* buffer = IAnimationBuffer::CreateFromSourceData( animSet, animation, partSourceData, previousBuffer, preferBetterQuality );
		if ( NULL == buffer )
		{
			// pre-GC cleanup
			for ( Uint32 j=0; j<i; ++j)
			{
				delete animationBuffers[j];
			}

			WARN_ENGINE( TXT("Failed to initialize animation buffer for animation part %d/%d."), i+1, numNonEmptyParts );
			return false;
		}

		// keep around
		animationBuffers.PushBack(buffer);
	}

	// copy common data
	m_numBones = sourceData.m_numBones;
	m_numTracks = sourceData.m_numTracks;
	m_parts = animationBuffers;

	// calculate the end frame index for each part
	{
		m_firstFrames.Resize( m_parts.Size() );
		Uint32 currentFrame = 0;
		for ( Uint32 i=0; i<m_parts.Size(); ++i )
		{
			m_firstFrames[i] = currentFrame;

			const Uint32 numKeysInPart = animationBuffers[i]->GetFramesNum();
			const Uint32 numFramesInPart = numKeysInPart - 1;
			currentFrame += numFramesInPart;
		}
	}

	// Total number of frames is the sum of the number of non overlapping frames in all the parts
	m_numFrames = currentFrame + 1;
	return true;
}

Bool CAnimationBufferMultipart::Load( const Uint32 frameIndex, const Uint32 numBones, RedQsTransform* outTransforms, const Uint32 numTracks, Float* outValues, Bool fallbackOnly ) const
{
	// Find the part index that contains given animation frame
	Uint32 partIndex = 0;
	Uint32 baseFrame = 0;
	for ( Uint32 i=m_firstFrames.Size()-1; i>0; --i )
	{
		if (frameIndex >= m_firstFrames[i])
		{
			baseFrame = m_firstFrames[i];
			partIndex = i;
			break;
		}
	}

	// Sample from animation part
	IAnimationBuffer* part = m_parts[partIndex];
	if ( NULL != part )
	{
		const Uint32 adjustedFrame = frameIndex - baseFrame;
		return part->Load( adjustedFrame, numBones, outTransforms, numTracks, outValues, fallbackOnly );
	}
	else
	{
		return false;
	}
}

// Frame layout in multi part animation:
// 0--1--2--3--4--5--6--7--8--9
// 0--1--2--3
//          0--1--2--3
//                   0--1--2--3

Bool CAnimationBufferMultipart::Sample( const Float frame, const Uint32 numBones, RedQsTransform* outTransforms, const Uint32 numTracks, Float* outTracks, Bool fallbackOnly ) const
{
	const Uint32 maxBones = Min<Uint32>( GetBonesNum(), numBones );
	const Uint32 maxTracks = Min<Uint32>( GetTracksNum(), numTracks );

	// Get the integer part and fraction
	const Int32 intFrame = static_cast<Int32>( frame );
	const Float frac = frame - static_cast<Float>( intFrame );

	// Find part index
	Uint32 partIndex = 0;
	Uint32 partBaseFrame = 0;
	for ( Uint32 i=m_firstFrames.Size()-1; i>0; --i )
	{
		if (intFrame >= (Int32)m_firstFrames[i])
		{
			partBaseFrame = m_firstFrames[i];
			partIndex = i;
			break;
		}
	}

	// Get the animation part
	IAnimationBuffer* part = m_parts[partIndex];
	if ( NULL == part )
	{
		return false;
	}

	// Get the base and next frame interpolating data
	const Uint32 baseFrame = Min<Uint32>( intFrame - partBaseFrame, part->GetFramesNum()-1 );
	const Uint32 nextFrame = Min<Uint32>( baseFrame + 1, part->GetFramesNum()-1 );

	// Load base frame
	RedQsTransform* baseKeys = (RedQsTransform*)RED_ALLOCA( sizeof(RedQsTransform)*maxBones );
	Float* baseTracks = (Float*)RED_ALLOCA( sizeof(Float)*maxTracks);
	if (!part->Load( baseFrame, maxBones, baseKeys, maxTracks, baseTracks, fallbackOnly ))
	{
		WARN_ENGINE(TXT("Failed to load data from part %d of multipart animation at frame %d (of %d)"),
			partIndex, baseFrame, part->GetFramesNum() );
		return false;
	}

	// Load next frame
	RedQsTransform* nextKeys = (RedQsTransform*)RED_ALLOCA( sizeof(RedQsTransform)*maxBones );
	Float* nextTracks = (Float*)RED_ALLOCA( sizeof(Float)*maxTracks);
	if (!part->Load( nextFrame, maxBones, nextKeys, maxTracks, nextTracks, fallbackOnly ))
	{
		WARN_ENGINE(TXT("Failed to load data from part %d of multipart animation at frame %d (of %d)"),
			partIndex, nextFrame, part->GetFramesNum() );
		return false;
	}

	// Interpolate bone values
	for ( Uint32 i=0; i<maxBones; ++i, ++baseKeys, ++nextKeys )
	{
		outTransforms[i].Lerp( *baseKeys, *nextKeys, frac );
	}

	// Interpolate track values
	for ( Uint32 i=0; i<maxTracks; ++i, ++baseTracks, ++nextTracks )
	{
		outTracks[i] = Lerp<Float>( frac, *baseTracks, *nextTracks );
	}

	return true;
}

void CAnimationBufferMultipart::SetCompressionParams( SAnimationBufferBitwiseCompressionPreset const & compressionPreset, SAnimationBufferBitwiseCompressionSettings const & compressionSettings )
{
	for ( Uint32 i=0; i<m_parts.Size(); ++i )
	{
		if ( m_parts[ i ] )
		{
			m_parts[ i ]->SetCompressionParams( compressionPreset, compressionSettings );
		}
	}
}

const TDynArray< Uint32 >& CAnimationBufferMultipart::GetPartsFirstFrames() const 
{
	return m_firstFrames;
}

Uint64 CAnimationBufferMultipart::GetDataCRC( Uint64 seed ) const
{
	Uint64 crc = seed;
	// basically, everything that is serialized
	crc = Red::CalculateHash64( &m_numFrames, sizeof( m_numFrames ), crc );
	crc = Red::CalculateHash64( &m_numBones, sizeof( m_numBones ), crc );
	crc = Red::CalculateHash64( &m_numTracks, sizeof( m_numTracks ), crc );
	crc = Red::CalculateHash64( m_firstFrames.Data( ), m_firstFrames.DataSize( ), crc );
	crc = Red::CalculateHash64( m_parts.Data( ), m_parts.DataSize( ), crc );
	return crc;
}