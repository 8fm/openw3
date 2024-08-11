/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "animationBuffer.h"
#include "animationBufferUncompressed.h" 
#include "../redMath/redmathbase.h"

IMPLEMENT_ENGINE_CLASS(CAnimationBufferUncompressed);

CAnimationBufferUncompressed::CAnimationBufferUncompressed()
	: m_numFrames( 0 )
	, m_numBones( 0 )
	, m_numTracks( 0 )
	, m_numDynamicTracks( 0 )
{
}

CAnimationBufferUncompressed::~CAnimationBufferUncompressed()
{
}

void CAnimationBufferUncompressed::OnSerialize( IFile& file )
{
	IAnimationBuffer::OnSerialize(file);

	if ( !file.IsGarbageCollector() )
	{
		if ( file.IsWriter() && file.IsCooker() )
		{
			file << m_keyMask;
			file << m_constKeys;
			file << m_boneKeys;
			file << m_trackKeys;
		}
		else
		{
			m_keyMask.SerializeBulk(file);
			m_constKeys.SerializeBulk(file);
			m_boneKeys.SerializeBulk(file);
			m_trackKeys.SerializeBulk(file);
		}
	}
}

Uint32 CAnimationBufferUncompressed::GetFramesNum() const
{
	return m_numFrames;
}

Uint32 CAnimationBufferUncompressed::GetBonesNum() const
{
	return m_numBones;
}

Uint32 CAnimationBufferUncompressed::GetTracksNum() const
{
	return m_numTracks;
}

Uint32 CAnimationBufferUncompressed::GetPartsNum() const
{
	return 1;
}

IAnimationBuffer const * CAnimationBufferUncompressed::GetPart(Uint32 partIndex) const
{
	return partIndex == 0? this : nullptr;
}

void CAnimationBufferUncompressed::GetMemorySize(Uint32 & outNonStreamable, Uint32 & outStreamableLoaded, Uint32 & outStreamableTotal, Uint32 & outSourceDataSize) const
{
	outNonStreamable = sizeof(CAnimationBufferUncompressed);
	outNonStreamable += (Uint32)m_keyMask.DataSize();
	outNonStreamable += (Uint32)m_boneKeys.DataSize();
	outNonStreamable += (Uint32)m_constKeys.DataSize();
	outNonStreamable += (Uint32)m_trackKeys.DataSize();
	outStreamableLoaded = 0;
	outStreamableTotal = 0;
	outSourceDataSize = 0;
}

Bool CAnimationBufferUncompressed::IsCompressed() const
{
	return false;
}

enum EKeyOrder
{
	Translation_X = FLAG(0),
	Translation_Y = FLAG(1),
	Translation_Z = FLAG(2),
	Rotation_X = FLAG(3),
	Rotation_Y = FLAG(4),
	Rotation_Z = FLAG(5),
	Rotation_W = FLAG(6),
	Scale_X = FLAG(7),
	Scale_Y = FLAG(8),
	Scale_Z = FLAG(9),

	ALL_KEYS = 0x3FF, // 10 keys
};

static inline void GetTracks( const RedQsTransform& qs, Float* outTracks )
{
	outTracks[0] = qs.Translation.X;
	outTracks[1] = qs.Translation.Y;
	outTracks[2] = qs.Translation.Z;
	outTracks[3] = qs.Rotation.Quat.X;
	outTracks[4] = qs.Rotation.Quat.Y;
	outTracks[5] = qs.Rotation.Quat.Z;
	outTracks[6] = qs.Rotation.Quat.W;
	outTracks[7] = qs.Scale.X;
	outTracks[8] = qs.Scale.Y;
	outTracks[9] = qs.Scale.Z;
}

static inline void TestTransform( const RedQsTransform& base, const RedQsTransform& cur, Uint16& keyMask )
{
	const Float maxDifference = 0.001f;

#if 1
	Float trackBase[10];
	GetTracks(base, trackBase);

	Float trackCur[10];
	GetTracks(cur, trackCur);

	for ( Uint32 j=0; j<10; ++j )
	{
		const Uint16 trackMask = (Uint16)(1 << j);
		if ( keyMask & trackMask )
		{
			if ( Red::Math::MAbs(trackBase[j] - trackCur[j]) > maxDifference )
			{
				keyMask &= ~trackMask;
			}
		}
	}
#else
	if (keyMask & Translation_X && (maxDifference < Red::Math::MAbs(base.Translation.X - cur.Translation.X)) ) 
		keyMask &= ~Translation_X;
	if (keyMask & Translation_Y && (maxDifference < Red::Math::MAbs(base.Translation.Y - cur.Translation.Y)) ) 
		keyMask &= ~Translation_Y;
	if (keyMask & Translation_Z && (maxDifference < Red::Math::MAbs(base.Translation.Z - cur.Translation.Z)) ) 
		keyMask &= ~Translation_Z;
	if (keyMask & Rotation_X && (maxDifference < Red::Math::MAbs(base.Rotation.Quat.X - cur.Rotation.Quat.X)) ) 
		keyMask &= ~Rotation_X;
	if (keyMask & Rotation_Y && (maxDifference < Red::Math::MAbs(base.Rotation.Quat.Y - cur.Rotation.Quat.Y)) ) 
		keyMask &= ~Rotation_Y;
	if (keyMask & Rotation_Z && (maxDifference < Red::Math::MAbs(base.Rotation.Quat.Z - cur.Rotation.Quat.Z)) ) 
		keyMask &= ~Rotation_Z;
	if (keyMask & Rotation_W && (maxDifference < Red::Math::MAbs(base.Rotation.Quat.W - cur.Rotation.Quat.W)) ) 
		keyMask &= ~Rotation_W;
	if (keyMask & Scale_X && (maxDifference < Red::Math::MAbs(base.Scale.X - cur.Scale.X)) ) 
		keyMask &= ~Scale_X;
	if (keyMask & Scale_Y && (maxDifference < Red::Math::MAbs(base.Scale.Y - cur.Scale.Y)) ) 
		keyMask &= ~Scale_Y;
	if (keyMask & Scale_Z && (maxDifference < Red::Math::MAbs(base.Scale.Z - cur.Scale.Z)) ) 
		keyMask &= ~Scale_Z;
#endif
}

Bool CAnimationBufferUncompressed::Initialize( const CSkeletalAnimationSet* animSet, CSkeletalAnimation* animation, const SourceData& sourceData, IAnimationBuffer* previousAnimBuffer, Bool preferBetterQuality )
{
	// calculate total number of frames
	Uint32 numFrames = 0;
	Int32 firstNonEmptyPart = 0;
	const Uint32 numParts = sourceData.m_parts.Size();
	for ( Uint32 i=0; i<numParts; ++i )
	{
		const Uint32 numFramesInPart = sourceData.m_parts[i].m_numFrames;
		if ( numFramesInPart > 0 )
		{
			if ( firstNonEmptyPart == -1 )
			{
				firstNonEmptyPart = i;
			}

			numFrames += numFramesInPart;
		}
	}

	// make sure we have at least one part
	if ( numFrames == 0 )
	{
		WARN_ENGINE( TXT("There are no animation parts in animation source data. Unable to initialize.") );
		return false;
	}

	// Copy general data
	m_numBones = sourceData.m_numBones;
	m_numTracks = sourceData.m_numTracks;
	m_numFrames = numFrames;
#ifdef USE_REF_IK_MISSING_BONES_HACK
	m_hasRefIKBones = sourceData.m_hasRefIKBones;
#endif

	// Test each key
	const Uint32 numKeys = 10 * sourceData.m_numBones;
	m_keyMask.Resize( numKeys );

	// Decide if key is constant or not
	Uint32 numConstantTracks = 0;
	Uint32 numNormalTracks = 0;
	Uint8* keyMaskWrite = m_keyMask.TypedData();
	for ( Uint32 i=0; i<m_numBones; ++i )
	{
		Uint16 mask = ALL_KEYS;
		for ( Uint32 k=0; k<numParts && mask; ++k )
		{
			const SourceData::Part& basePart = sourceData.m_parts[firstNonEmptyPart];
			const SourceData::Part& curPart = sourceData.m_parts[k];
			const Uint32 numFramesInPart = curPart.m_numFrames;

			// Gather key mask 
			const RedQsTransform* base = &basePart.m_bones[i];
			const RedQsTransform* cur = &curPart.m_bones[i+m_numBones];
			for ( Uint32 j=1; j<numFramesInPart && mask; ++j )
			{
				TestTransform( *base, *cur, mask );
				cur += m_numBones; // advance to next frame for THIS BONE
			}
		}

		// Write key mask
		for ( Uint32 j=0; j<10; ++j )
		{
			if ( mask & FLAG(j) )
			{
				numConstantTracks += 1;
				keyMaskWrite[j] = 1;
			}
			else
			{
				numNormalTracks += 1;
				keyMaskWrite[j] = 0;
			}
		}

		keyMaskWrite += 10;
	}

	ASSERT(keyMaskWrite - m_keyMask.TypedData() == (Int32)m_keyMask.Size());

	LOG_ENGINE( TXT("Compressing animation, %d frames, %d bones: %d constant tracks, %d normal tracks"), 
		m_numFrames, m_numBones, numConstantTracks, numNormalTracks );

	// Save the number of dynamic tracks - it's needed for "decompression"
	m_numDynamicTracks = numNormalTracks;

	// Preallocate buffers
	m_constKeys.Resize( numConstantTracks );
	m_boneKeys.Resize( numNormalTracks * m_numFrames );
	m_trackKeys.Resize( m_numFrames * m_numTracks );

	// Write the const keys
	{
		Float* writePtr = m_constKeys.TypedData();
		const Uint8* keyMask = m_keyMask.TypedData();
		for ( Uint32 i=0; i<m_numBones; ++i )
		{
			const SourceData::Part& basePart = sourceData.m_parts[firstNonEmptyPart];

			// Get base bone tracks for Frame 0
			Float tracks[10];
			GetTracks( basePart.m_bones[i], tracks );

			// Save the constant ones in the stream
			for ( Uint32 j=0; j<10; ++j, ++keyMask )
			{
				if ( *keyMask == 1 )
				{
					*writePtr++ = tracks[j];
				}
			}
		}

		ASSERT(writePtr - m_constKeys.TypedData() == (Int32)m_constKeys.Size());
	}

	// Write the changing keys
	{
		Float* writeBonePtr = m_boneKeys.TypedData();
		Float* writeTrackPtr = m_trackKeys.TypedData();

		for ( Uint32 k=0; k<numParts; ++k )
		{
			const SourceData::Part& curPart = sourceData.m_parts[k];
			const Uint32 numFramesInPart = curPart.m_numFrames;

			// Copy bone keys
			const RedQsTransform* readKey = curPart.m_bones;
			for ( Uint32 k=0; k<numFramesInPart; ++k )
			{
				const Uint8* keyMask = m_keyMask.TypedData();
				for ( Uint32 i=0; i<m_numBones; ++i, ++readKey )
				{
					// Get base bone tracks for Frame 0
					Float tracks[10];
					GetTracks( *readKey, tracks );

					// Save the constant ones in the stream
					for ( Uint32 j=0; j<10; ++j, ++keyMask )
					{
						if ( *keyMask == 0 )
						{
							*writeBonePtr++ = tracks[j];
						}
					}
				}
			}

			// Copy track keys
			Red::System::MemoryCopy( writeTrackPtr, curPart.m_track, sizeof(Float) * m_numTracks * numFramesInPart );
			writeTrackPtr += m_numTracks * numFramesInPart;
		}

		ASSERT( writeBonePtr - m_boneKeys.TypedData() == (Int32)m_boneKeys.Size());
		ASSERT( writeTrackPtr - m_trackKeys.TypedData() == (Int32)m_trackKeys.Size());
	}

	return true;
}

Uint64 CAnimationBufferUncompressed::GetDataCRC( Uint64 seed ) const
{
	Uint64 crc = seed;
	// basically, everything that is serialized
	crc = Red::CalculateHash64( &m_numFrames, sizeof( m_numFrames ), crc );
	crc = Red::CalculateHash64( &m_numBones, sizeof( m_numBones ), crc );
	crc = Red::CalculateHash64( &m_numTracks, sizeof( m_numTracks ), crc );
	crc = Red::CalculateHash64( &m_numDynamicTracks, sizeof( m_numDynamicTracks ), crc );
	crc = Red::CalculateHash64( m_keyMask.Data( ), m_keyMask.DataSize( ), crc );
	crc = Red::CalculateHash64( m_constKeys.Data( ), m_constKeys.DataSize( ), crc );
	crc = Red::CalculateHash64( m_boneKeys.Data( ), m_boneKeys.DataSize( ), crc );
	crc = Red::CalculateHash64( m_trackKeys.Data( ), m_trackKeys.DataSize( ), crc );
#ifdef USE_REF_IK_MISSING_BONES_HACK
	crc = Red::CalculateHash64( &m_hasRefIKBones, sizeof( m_hasRefIKBones ), crc );
#endif
	return crc;
}

Bool CAnimationBufferUncompressed::Load( const Uint32 frameIndex, const Uint32 numBones, RedQsTransform* outTransforms, const Uint32 numTracks, Float* outValues, Bool fallbackOnly ) const
{
	// This should not happen under normal operation 
	if ( frameIndex >= m_numFrames )
	{
		HALT( "Frame index outside range in LoadKeys. Debug." );
		return false;
	}

	// Copy the keys
	if ( numBones && outTransforms )
	{
		const Uint32 maxBonesToCopy = Min<Uint32>( numBones, m_numBones );
		const Float* srcConstKeys = m_constKeys.TypedData();
		const Float* srcBoneKeys = m_boneKeys.TypedData() + (m_numDynamicTracks*frameIndex);
		const Uint8* srcKeyMask = m_keyMask.TypedData();
		RedQsTransform* writePtr = outTransforms;
		for ( Uint32 i=0; i<maxBonesToCopy; ++i, ++writePtr )
		{
			// TODO(dex): I should look into the assembly... this can be optimized to be totally branchles
			// TODO(dex): Also, switch to using a bit-wise key mask, it's nicer ( we are memory limited here anyway )
			writePtr->Translation.X = ( *srcKeyMask++  ) ? ( *srcConstKeys++ ) : ( *srcBoneKeys++ );
			writePtr->Translation.Y = ( *srcKeyMask++  ) ? ( *srcConstKeys++ ) : ( *srcBoneKeys++ );
			writePtr->Translation.Z = ( *srcKeyMask++  ) ? ( *srcConstKeys++ ) : ( *srcBoneKeys++ );
			writePtr->Translation.W = 1.0f;
			writePtr->Rotation.Quat.X = ( *srcKeyMask++  ) ? ( *srcConstKeys++ ) : ( *srcBoneKeys++ );
			writePtr->Rotation.Quat.Y = ( *srcKeyMask++  ) ? ( *srcConstKeys++ ) : ( *srcBoneKeys++ );
			writePtr->Rotation.Quat.Z = ( *srcKeyMask++  ) ? ( *srcConstKeys++ ) : ( *srcBoneKeys++ );
			writePtr->Rotation.Quat.W = ( *srcKeyMask++  ) ? ( *srcConstKeys++ ) : ( *srcBoneKeys++ );
			writePtr->Scale.X = ( *srcKeyMask++  ) ? ( *srcConstKeys++ ) : ( *srcBoneKeys++ );
			writePtr->Scale.Y = ( *srcKeyMask++  ) ? ( *srcConstKeys++ ) : ( *srcBoneKeys++ );
			writePtr->Scale.Z = ( *srcKeyMask++  ) ? ( *srcConstKeys++ ) : ( *srcBoneKeys++ );
			writePtr->Scale.W = 1.0f;
		}
	}

	// Copy the tracks
	if ( numTracks && outValues )
	{
		const Uint32 maxTracksToCopy = Min<Uint32>( numTracks, m_numTracks );
		const Float* srcData = m_trackKeys.TypedData() + (frameIndex*m_numTracks);
		Red::System::MemoryCopy( outValues, srcData, maxTracksToCopy * sizeof(Float) );
	}

	return true;
}

Bool CAnimationBufferUncompressed::Sample( const Float frame, const Uint32 numBones, RedQsTransform* outTransforms, const Uint32 numTracks, Float* outTracks, Bool fallbackOnly ) const
{
	const Uint32 maxBones = Min<Uint32>( GetBonesNum(), numBones );
	const Uint32 maxTracks = Min<Uint32>( GetTracksNum(), numTracks );

	// Handle time limits
	const Float maxFrame = (Float)(GetFramesNum() - 1.0001f);
	if ( frame >= maxFrame )
	{
		// Time beyond end of animation
		return Load( GetFramesNum()-1, numBones, outTransforms, numTracks, outTracks, fallbackOnly );
	}
	else if ( frame <= 0.0f )
	{
		// Time beyond start of animation
		return Load( 0, numBones, outTransforms, numTracks, outTracks, fallbackOnly );
	}

	// Get the base and next frame for interpolating data
	const Uint32 baseFrame = static_cast<Uint32>( frame );
	const Uint32 nextFrame = baseFrame + 1;

	// Calculate time fraction
	const Float frac = frame - static_cast<Float>( baseFrame );

	// Load base frame
	RedQsTransform* baseKeys = (RedQsTransform*)RED_ALLOCA( sizeof(RedQsTransform)*maxBones );
	Float* baseTracks = (Float*)RED_ALLOCA( sizeof(Float)*maxTracks);
	Load( baseFrame, maxBones, baseKeys, maxTracks, baseTracks, fallbackOnly );

	// Load next frame
	RedQsTransform* nextKeys = (RedQsTransform*)RED_ALLOCA( sizeof(RedQsTransform)*maxBones );
	Float* nextTracks = (Float*)RED_ALLOCA( sizeof(Float)*maxTracks);
	Load( nextFrame, maxBones, nextKeys, maxTracks, nextTracks, fallbackOnly );

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

