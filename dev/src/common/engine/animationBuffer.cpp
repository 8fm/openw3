/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "animationBuffer.h"
#include "animationBufferMultipart.h"
#include "animationBufferUncompressed.h"
#include "animationBufferBitwiseCompressed.h"
#include "skeletalAnimation.h"
#include "behaviorGraphUtils.inl"

IMPLEMENT_ENGINE_CLASS(IAnimationBuffer);
IMPLEMENT_RTTI_ENUM( EAnimationBufferDataAvailable );
IMPLEMENT_RTTI_ENUM( SAnimationBufferStreamingOption );

RED_DEFINE_STATIC_NAME( Reference );

// #define SHOW_COMPRESSION_SIZES

IAnimationBuffer::~IAnimationBuffer()
{
}

#ifdef SHOW_COMPRESSION_SIZES
#include "skeletalAnimation.h"

IAnimationBuffer* InitializeBufferAndLogSize( const CSkeletalAnimationSet* animSet, IAnimationBuffer* buffer, const IAnimationBuffer::SourceData& data, IAnimationBuffer* previousAnimBuffer, bool preferBetterQuality, bool discard, Uint32& sizeOfFirst )
{
	if ( !buffer )
	{
		return NULL;
	}

	if ( !buffer->Initialize( animSet, data, previousAnimBuffer, preferBetterQuality ) )
	{
		delete buffer;
		return NULL;
	}

	if ( sizeOfFirst == 0 )
	{
		sizeOfFirst = buffer->GetMemorySize();
	}

	Float orginalSize = data.m_parts.Size() == 0? 0.0f : (Float)(data.m_parts[0].m_numFrames * sizeof(Float) * ( (3 + 4 + 3) * data.m_numBones + data.m_numTracks ));
	RED_LOG( Animation, TXT("Size : %8i ub %7.3f%% raw: %7.3f%% ratio: %4.1f : %s"), buffer->GetMemorySize(),
		100.0f * (Float)buffer->GetMemorySize() / (Float)sizeOfFirst,
		orginalSize == 0.0f? 0.0f : 100.0f * (Float)buffer->GetMemorySize() / orginalSize,
		sizeOfFirst == 0.0f? 0.0f : orginalSize / (Float)buffer->GetMemorySize(),
		buffer->GetClass()->GetName().AsString().AsChar() );

	if ( discard )
	{
		delete buffer;
		return NULL;
	}

	return buffer;
}
#endif
#include "skeleton.h"
#include "skeletalAnimationSet.h"
#include "skeletalAnimationEntry.h"

IAnimationBuffer* IAnimationBuffer::CreateFromSourceData( const CSkeletalAnimationSet* animSet, CSkeletalAnimation* animation, const SourceData& data, 
														 IAnimationBuffer* previousAnimBuffer, Bool preferBetterQuality )
{
	// Empty data
	if ( data.m_numBones == 0 || data.m_parts.Empty() )
	{
		return NULL;
	}

	// Single buffer
	if ( data.m_parts.Size() == 1 )
	{
		Bool forceUncompressed = false;

#ifdef SHOW_COMPRESSION_SIZES
		Uint32 sizeOfUncompressed = 0;
		if ( forceUncompressed )
		{
			InitializeBufferAndLogSize( animSet, new CAnimationBufferUncompressed, data, previousAnimBuffer, preferBetterQuality, true, sizeOfUncompressed );
			InitializeBufferAndLogSize( animSet, new CAnimationBufferBitwiseCompressed, data, previousAnimBuffer, preferBetterQuality, true, sizeOfUncompressed )
			// don't return
		}
		else
		{
			InitializeBufferAndLogSize( animSet, new CAnimationBufferUncompressed, data, previousAnimBuffer, preferBetterQuality, true, sizeOfUncompressed );
			return InitializeBufferAndLogSize( animSet, new CAnimationBufferBitwiseCompressed, data, previousAnimBuffer, preferBetterQuality, false, sizeOfUncompressed );
		}
#else
		IAnimationBuffer* buffer;
		if ( forceUncompressed )
		{
			buffer = new CAnimationBufferUncompressed;
		}
		else
		{
			buffer = new CAnimationBufferBitwiseCompressed;
		}

		if ( !buffer )
		{
			return NULL;
		}

		if ( !buffer->Initialize( animSet, animation, data, previousAnimBuffer, preferBetterQuality ) )
		{
			delete buffer;
			return NULL;
		}

		return buffer;
#endif
	}
	else
	{
		CAnimationBufferMultipart* buffer = new CAnimationBufferMultipart;
		if ( !buffer )
		{
			return NULL;
		}

		if ( !buffer->Initialize( animSet, animation, data, previousAnimBuffer, preferBetterQuality ) )
		{
			delete buffer;
			return NULL;
		}

		return buffer;
	}
}

#define FULL_TRACK 0
#define CONSTANT_TRACK 1
// versions
//		invalid
#define LATENT_SOURCE_ANIM_DATA_BUFFER_VERSION__INVALID					0
//		initial version
#define LATENT_SOURCE_ANIM_DATA_BUFFER_VERSION__INITIAL					1
//		added ref ik missing bones hack
#define LATENT_SOURCE_ANIM_DATA_BUFFER_VERSION__REFIK_MISSING_BONES		2
//		fix incorrect tracks
#define LATENT_SOURCE_ANIM_DATA_BUFFER_VERSION__FIX_TRACKS				3

//		current one, latest
#define LATENT_SOURCE_ANIM_DATA_BUFFER_CURRENT_VERSION 3

struct LatentSourceAnimDataBufferGatherer
{
	TDynArray< Int8 > m_data;

	template < class T >
	void Gather(T const & _t)
	{
		Uint32 storeAt = m_data.Size();
		Uint32 sizeOfT = sizeof(T);
		m_data.Grow(sizeOfT);
		Red::System::MemoryCopy( &m_data[storeAt], &_t, sizeOfT );
	}

	void GatherTrack(Float const * track, Uint32 byteStride, Uint32 count)
	{
		Bool allSame = true;
		{	// check if all values are the same
			Float const * ptr = track;
			Uint32 left = count;
			while ( left )
			{
				if (*ptr != *track)
				{
					allSame = false;
					break;
				}
				-- left;
				ptr = (Float*)(((Int8*)ptr)+byteStride);
			}
		}
		Int8 type = allSame? CONSTANT_TRACK : FULL_TRACK;
		Gather( type );
		if ( allSame )
		{
			// store just one value
			Gather( *track );
		}
		else
		{
			// store each value (we need to jump with pointer to find them by stride)
			Float const * ptr = track;
			Uint32 left = count;
			while ( left )
			{
				Gather( *ptr );
				-- left;
				ptr = (Float*)(((Int8*)ptr)+byteStride);
			}
		}
	}
};

void LatentSourceAnimDataBuffer::LoadAnimDataFrom(IAnimationBuffer::SourceData const & sourceData)
{
	LatentSourceAnimDataBufferGatherer g;

	// gather data
	Uint32 version = LATENT_SOURCE_ANIM_DATA_BUFFER_CURRENT_VERSION;
	g.Gather(version);
	g.Gather(sourceData.m_dT);
	g.Gather(sourceData.m_numBones);
	g.Gather(sourceData.m_numTracks);
	g.Gather(sourceData.m_totalDuration);
#ifdef USE_REF_IK_MISSING_BONES_HACK
	g.Gather(sourceData.m_hasRefIKBones);
#else
	Bool tempBool = true;
	g.Gather(tempBool);
#endif
	Uint32 partsCount = sourceData.m_parts.Size();
	g.Gather(partsCount);

	// gather parts, track by track
	for ( auto iPart = sourceData.m_parts.Begin(); iPart != sourceData.m_parts.End(); ++ iPart )
	{
		g.Gather(iPart->m_numFrames);
		for ( Uint32 iBone = 0; iBone < sourceData.m_numBones; ++ iBone )
		{
			RedQsTransform const * bone = &iPart->m_bones[iBone];
			Uint32 byteStride = sizeof(RedQsTransform) * sourceData.m_numBones;
			g.GatherTrack(&bone->Translation.X, byteStride, iPart->m_numFrames);
			g.GatherTrack(&bone->Translation.Y, byteStride, iPart->m_numFrames);
			g.GatherTrack(&bone->Translation.Z, byteStride, iPart->m_numFrames);
			g.GatherTrack(&bone->Rotation.Quat.X, byteStride, iPart->m_numFrames);
			g.GatherTrack(&bone->Rotation.Quat.Y, byteStride, iPart->m_numFrames);
			g.GatherTrack(&bone->Rotation.Quat.Z, byteStride, iPart->m_numFrames);
			g.GatherTrack(&bone->Rotation.Quat.W, byteStride, iPart->m_numFrames);
			g.GatherTrack(&bone->Scale.X, byteStride, iPart->m_numFrames);
			g.GatherTrack(&bone->Scale.Y, byteStride, iPart->m_numFrames);
			g.GatherTrack(&bone->Scale.Z, byteStride, iPart->m_numFrames);
		}
		for ( Uint32 iTrack = 0; iTrack < sourceData.m_numTracks; ++ iTrack )
		{
			Float const * track = &iPart->m_track[iTrack];
			Uint32 byteStride = sizeof(Float) * sourceData.m_numTracks;
			g.GatherTrack(track, byteStride, iPart->m_numFrames);
		}
	}

	// store in buffer
	Clear();
	Uint32 gSize = g.m_data.Size();
	Allocate(gSize);
	Red::System::MemoryCopy( GetData(), g.m_data.Data(), gSize );
}

struct LatentSourceAnimDataBufferReader
{
	Int8* m_ptr;
	LatentSourceAnimDataBufferReader(void * data)
	:	m_ptr( (Int8*)data )
	{
	}

	template < class T >
	void Read(T & _t)
	{
		Uint32 sizeOfT = sizeof(T);
		Red::System::MemoryCopy( &_t, m_ptr, sizeOfT );
		m_ptr += sizeOfT;
	}

	void FillTrack(Float * track, Uint32 byteStride, Uint32 count, Float withValue)
	{
		// read whole track
		Float * ptr = track;
		Uint32 left = count;
		while ( left )
		{
			*ptr = withValue;
			-- left;
			ptr = (Float*)(((Int8*)ptr)+byteStride);
		}
	}

	void ReadTrack(Float * track, Uint32 byteStride, Uint32 count)
	{
		Int8 type = 0;
		Read( type );
		if ( type == CONSTANT_TRACK )
		{
			// fill whole track with one value
			Float constTrack;
			Read( constTrack );
			Float * ptr = track;
			Uint32 left = count;
			while ( left )
			{
				*ptr = constTrack;
				-- left;
				ptr = (Float*)(((Int8*)ptr)+byteStride);
			}
		}
		else
		{
			// read whole track
			Float * ptr = track;
			Uint32 left = count;
			while ( left )
			{
				Read( *ptr );
				-- left;
				ptr = (Float*)(((Int8*)ptr)+byteStride);
			}
		}
	}
};

#ifdef USE_REF_IK_MISSING_BONES_HACK
static const CSkeleton* FindSkeletonFor( const CSkeletalAnimation * skeletalAnimation )
{
#ifndef NO_EDITOR
	if (skeletalAnimation->GetSkeleton())
	{
		return skeletalAnimation->GetSkeleton();
	}
#endif
	CSkeletalAnimationSet* foundAnimSet = (skeletalAnimation->GetEntry() != nullptr) ? skeletalAnimation->GetEntry()->GetAnimSet() : nullptr;
	if (foundAnimSet)
	{
		if ( foundAnimSet->GetSkeleton() )
		{
			return foundAnimSet->GetSkeleton();
		}
		else
		{
			//ERR_ENGINE( TXT( "No skeleton set for animset '%ls'" ), foundAnimSet->GetDepotPath().AsChar() );
		}
	}
	// there can be no animset for animation (for example, for mimic animations)
	return nullptr;
}

static Int32 GetReferenceBoneFrom( const CSkeletalAnimation * skeletalAnimation )
{
	if (const CSkeleton* skeleton = FindSkeletonFor( skeletalAnimation ))
	{
		return skeleton->FindBoneByName( CNAME( Reference ) );
	}
	else
	{
		return -1;
	}
}
#endif

Bool LatentSourceAnimDataBuffer::ReadAnimDataTo(IAnimationBuffer::SourceData & outData, CSkeletalAnimation * skeletalAnimation)
{
	LatentSourceAnimDataBufferReader r(GetData());

	Uint32 version = 0;
	r.Read(version);
	if (version == LATENT_SOURCE_ANIM_DATA_BUFFER_VERSION__INVALID ||
		version > LATENT_SOURCE_ANIM_DATA_BUFFER_CURRENT_VERSION)
	{
		return false;
	}
	r.Read(outData.m_dT);
	r.Read(outData.m_numBones);
	r.Read(outData.m_numTracks);
	if (version < LATENT_SOURCE_ANIM_DATA_BUFFER_VERSION__FIX_TRACKS && outData.m_numTracks)
	{
		// tracks are incorrectly stored
		return false;
	}
	r.Read(outData.m_totalDuration);
	if (version >= LATENT_SOURCE_ANIM_DATA_BUFFER_VERSION__REFIK_MISSING_BONES)
	{
#ifdef USE_REF_IK_MISSING_BONES_HACK
		r.Read(outData.m_hasRefIKBones);
#else
		Bool tempBool;
		r.Read(tempBool);
#endif
	}
	else
	{
#ifdef USE_REF_IK_MISSING_BONES_HACK
		outData.m_hasRefIKBones = false;
#endif
	}
	Uint32 partsCount = 0;
	r.Read(partsCount);

#ifdef USE_REF_IK_MISSING_BONES_HACK
	const CSkeleton* skeletonForMissingBones = FindSkeletonFor(skeletalAnimation);
	Int32 skeletonReferenceBoneIdx = GetReferenceBoneFrom(skeletalAnimation);
	Bool skeletonHasIKRefBones = skeletonReferenceBoneIdx != -1;

	// get struct for filling missing bones
	struct SFillMissingBoneInfo
	{
		CName m_boneName;
		Int32 m_boneIdx;
		Int32 m_parentBoneIdx;
		CName m_useBoneName;
		Int32 m_useBoneIdx;
		SFillMissingBoneInfo() {}
		SFillMissingBoneInfo(const CSkeleton* skeleton, CName const & boneName, CName const & useBoneName)
		{
			m_boneName = boneName;
			m_boneIdx = skeleton? skeleton->FindBoneByName(m_boneName) : -1;
			m_parentBoneIdx = skeleton? skeleton->GetParentBoneIndex(m_boneIdx) : -1;
			m_useBoneName = useBoneName;
			m_useBoneIdx = skeleton? skeleton->FindBoneByName(m_useBoneName) : -1;
			/*
			if ( m_boneIdx != -1 )
			{
				ERR_ENGINE( TXT("Could not find bone '%ls'"), m_boneName.AsChar());
			}
			if ( m_parentBoneIdx != -1 )
			{
				ERR_ENGINE( TXT("Could not find parent of bone '%ls'"), m_boneName.AsChar());
			}
			if ( m_useBoneIdx != -1 )
			{
				ERR_ENGINE( TXT("Could not find (use) bone '%ls'"), m_boneName.AsChar());
			}
			*/
		}
	};
	TDynArray<SFillMissingBoneInfo> fillMissingBonesInfo;
	if ( skeletonHasIKRefBones )
	{
		fillMissingBonesInfo.PushBack(SFillMissingBoneInfo(skeletonForMissingBones, CName( TXT("Reference") ), CName( TXT("Trajectory") )));
		fillMissingBonesInfo.PushBack(SFillMissingBoneInfo(skeletonForMissingBones, CName( TXT("IK_r_foot") ), CName( TXT("r_foot") )));
		fillMissingBonesInfo.PushBack(SFillMissingBoneInfo(skeletonForMissingBones, CName( TXT("IK_l_foot") ), CName( TXT("l_foot") )));
		fillMissingBonesInfo.PushBack(SFillMissingBoneInfo(skeletonForMissingBones, CName( TXT("IK_pelvis") ), CName( TXT("pelvis") )));
		fillMissingBonesInfo.PushBack(SFillMissingBoneInfo(skeletonForMissingBones, CName( TXT("IK_r_hand") ), CName( TXT("r_hand") )));
		fillMissingBonesInfo.PushBack(SFillMissingBoneInfo(skeletonForMissingBones, CName( TXT("IK_l_hand") ), CName( TXT("l_hand") )));
		fillMissingBonesInfo.PushBack(SFillMissingBoneInfo(skeletonForMissingBones, CName( TXT("IK_torso3") ), CName( TXT("torso3") )));
		// let's check if skeletonHasIKRefBones actually shows us that we have ik and reference bones (skeletonHasIKRefBones only checks if there is reference bone)
		// if there isn't don't run the hack, don't fill missing bones, just leave everything as there is
		Bool emptyFillMissingBonesInfo = false;
		for ( Uint32 i = 0; i < fillMissingBonesInfo.Size(); ++ i )
		{
			if ( fillMissingBonesInfo[i].m_boneIdx == -1 ||
				 fillMissingBonesInfo[i].m_useBoneIdx == -1 )
			{
				emptyFillMissingBonesInfo = true;
			}
		}
		if ( emptyFillMissingBonesInfo )
		{
			fillMissingBonesInfo.Clear();
			skeletonHasIKRefBones = false;
		}
	}
	Uint32 addMissingRefIKBones = fillMissingBonesInfo.Size();

	Bool fillMissingBones = skeletonHasIKRefBones && ! outData.m_hasRefIKBones && outData.m_numBones > 2;
	if ( fillMissingBones )
	{
		outData.m_numBones += 7;
		// it will now have those bones
		outData.m_hasRefIKBones = skeletonHasIKRefBones;
	}
#endif

	ASSERT(partsCount < 100000, TXT("It seems that we're in year 3094 using same technology to get very long animations or we've read garbage"));
	// read parts
	outData.m_parts.Resize(partsCount);
	Uint32 totalNumFrames = 0;
	for ( auto iPart = outData.m_parts.Begin(); iPart != outData.m_parts.End(); ++ iPart )
	{
		r.Read(iPart->m_numFrames);
		totalNumFrames += iPart->m_numFrames;
		Uint32 byteStride = sizeof(RedQsTransform) * outData.m_numBones;
		if ( outData.m_numBones )
		{
			RedQsTransform* readBones = new RedQsTransform[iPart->m_numFrames * outData.m_numBones];
			for ( Uint32 iBone = 0; iBone < outData.m_numBones; ++ iBone )
			{
				RedQsTransform * bone = &readBones[iBone];
#ifdef USE_REF_IK_MISSING_BONES_HACK
				// filling missing bones
				if ( fillMissingBones && iBone == 2 ) // start with bone just after trajectory - to put new bones between trajectory and following bone
				{
					Uint32 fillBones = addMissingRefIKBones;
					while ( fillBones )
					{
						RedQsTransform * bone = &readBones[iBone];
						r.FillTrack(&bone->Translation.X, byteStride, iPart->m_numFrames, 0.0f);
						r.FillTrack(&bone->Translation.Y, byteStride, iPart->m_numFrames, 0.0f);
						r.FillTrack(&bone->Translation.Z, byteStride, iPart->m_numFrames, 0.0f);
						r.FillTrack(&bone->Rotation.Quat.X, byteStride, iPart->m_numFrames, 0.0f);
						r.FillTrack(&bone->Rotation.Quat.Y, byteStride, iPart->m_numFrames, 0.0f);
						r.FillTrack(&bone->Rotation.Quat.Z, byteStride, iPart->m_numFrames, 0.0f);
						r.FillTrack(&bone->Rotation.Quat.W, byteStride, iPart->m_numFrames, 1.0f);
						r.FillTrack(&bone->Scale.X, byteStride, iPart->m_numFrames, 1.0f);
						r.FillTrack(&bone->Scale.Y, byteStride, iPart->m_numFrames, 1.0f);
						r.FillTrack(&bone->Scale.Z, byteStride, iPart->m_numFrames, 1.0f);
						-- fillBones;
						++ iBone;
					}
					// continue from here
					bone = &readBones[iBone];
				}
#endif
				r.ReadTrack(&bone->Translation.X, byteStride, iPart->m_numFrames);
				r.ReadTrack(&bone->Translation.Y, byteStride, iPart->m_numFrames);
				r.ReadTrack(&bone->Translation.Z, byteStride, iPart->m_numFrames);
				r.ReadTrack(&bone->Rotation.Quat.X, byteStride, iPart->m_numFrames);
				r.ReadTrack(&bone->Rotation.Quat.Y, byteStride, iPart->m_numFrames);
				r.ReadTrack(&bone->Rotation.Quat.Z, byteStride, iPart->m_numFrames);
				r.ReadTrack(&bone->Rotation.Quat.W, byteStride, iPart->m_numFrames);
				r.ReadTrack(&bone->Scale.X, byteStride, iPart->m_numFrames);
				r.ReadTrack(&bone->Scale.Y, byteStride, iPart->m_numFrames);
				r.ReadTrack(&bone->Scale.Z, byteStride, iPart->m_numFrames);
			}
#ifdef USE_REF_IK_MISSING_BONES_HACK
			if (fillMissingBones && skeletonForMissingBones)
			//if (skeletonHasIKRefBones && outData.m_hasRefIKBones && skeletonForMissingBones) // <- this is to force IK to be where bones are
			{
				// add reference and IK bones
				for ( auto iMissingBoneInfo = fillMissingBonesInfo.Begin(); iMissingBoneInfo != fillMissingBonesInfo.End(); ++ iMissingBoneInfo )
				{
					for ( Uint32 frameIdx = 0; frameIdx < iPart->m_numFrames; ++ frameIdx )
					{
						RedQsTransform* frameBones = &readBones[frameIdx * outData.m_numBones];
#ifdef USE_HAVOK_ANIMATION
						// it is not needed
#else
						ASSERT( iMissingBoneInfo->m_useBoneIdx >=0 && iMissingBoneInfo->m_useBoneIdx < skeletonForMissingBones->GetBonesNum() );
						ASSERT( iMissingBoneInfo->m_parentBoneIdx >=0 && iMissingBoneInfo->m_parentBoneIdx < skeletonForMissingBones->GetBonesNum() );
						ASSERT( iMissingBoneInfo->m_boneIdx >=0 && iMissingBoneInfo->m_boneIdx < skeletonForMissingBones->GetBonesNum() );
						AnimQsTransform useBoneTMS = skeletonForMissingBones->GetBoneMS( iMissingBoneInfo->m_useBoneIdx, frameBones, outData.m_numBones );
						AnimQsTransform parentBoneTMS = skeletonForMissingBones->GetBoneMS( iMissingBoneInfo->m_parentBoneIdx, frameBones, outData.m_numBones );
						AnimQsTransform & boneTLS = frameBones[ iMissingBoneInfo->m_boneIdx ];
						boneTLS.SetMulInverseMul( parentBoneTMS, useBoneTMS );
#endif
					}
				}
			}
#endif
			delete [] iPart->m_bones;
			iPart->m_bones = readBones;
		}
		if ( outData.m_numTracks )
		{
			Float* readTracks = new Float[iPart->m_numFrames * outData.m_numTracks];
			for ( Uint32 iTrack = 0; iTrack < outData.m_numTracks; ++ iTrack )
			{
				Float * track = &readTracks[iTrack];
				Uint32 byteStride = sizeof(Float) * outData.m_numTracks;
				r.ReadTrack(track, byteStride, iPart->m_numFrames);
			}
			delete [] iPart->m_track;
			iPart->m_track = readTracks;
		}
	}

#ifdef USE_REF_IK_MISSING_BONES_HACK
	if (fillMissingBones)
	{
		// fill back
		LoadAnimDataFrom( outData );
	}
#endif

	if ( outData.m_totalDuration == 1.0f )
	{
		// it might be incorrect, fix it with duration from parts
		outData.m_totalDuration = (Float)(totalNumFrames - 1) * outData.m_dT;
	}

	return true;
}

#undef FULL_TRACK
#undef CONSTANT_TRACK
#undef LATENT_SOURCE_ANIM_DATA_BUFFER_CURRENT_VERSION

#ifndef NO_EDITOR
void IAnimationBuffer::SourceData::BuildFrom(CSkeletalAnimation const * _animation)
{
	ASSERT(_animation->GetAnimBuffer() && _animation->GetAnimBuffer()->GetFramesNum() != 0, TXT("Trying to build source data from empty animation %s"), _animation->GetName().AsChar());
	// will be filled later
	m_numBones = 0;
	m_numTracks = 0;
	m_dT = 1.0f / _animation->GetFramesPerSecond();

	// fill from animation
	m_totalDuration = _animation->GetDuration();
#ifdef USE_REF_IK_MISSING_BONES_HACK
	m_hasRefIKBones = false; 
#endif

	// clean up
	for (Uint32 i=0; i < m_parts.Size(); ++i)
	{
		delete [] m_parts[i].m_bones;
		delete [] m_parts[i].m_track;
	}

	// clear parts
	m_parts.Clear();

	if ( const IAnimationBuffer * animBuffer = _animation->GetAnimBuffer() )
	{
#ifdef USE_REF_IK_MISSING_BONES_HACK
		m_hasRefIKBones = animBuffer->HasRefIKBones(); 
		if ( const CSkeleton* skeleton = _animation->GetSkeleton() )
		{
			if ( animBuffer->GetBonesNum() == skeleton->GetBonesNum() &&
				 GetReferenceBoneFrom(_animation) != -1 )
			{
				// big assumption - we have same number of bones in animation as in skeleton,
				// that might be a hint that we have all data for all skeleton bones
				// -- and if skeleton has refik bones, animation has them too!
				m_hasRefIKBones = true;
			}
		}
#endif
		m_parts.Resize( animBuffer->GetPartsNum() );
		Uint32 partIndex = 0;
		_animation->SyncLoad();
		_animation->AddUsage();
		while ( partIndex < animBuffer->GetPartsNum() )
		{
			if ( const IAnimationBuffer * part = animBuffer->GetPart(partIndex) )
			{
				Part& sourceDataPart = m_parts[partIndex];
				ASSERT(m_numBones == 0 || m_numBones == part->GetBonesNum(), TXT("Bones number should match through all parts"));
				m_numBones = part->GetBonesNum();
				ASSERT(m_numTracks == 0 || m_numTracks == part->GetTracksNum(), TXT("Tracks number should match through all parts"));
				m_numTracks = part->GetTracksNum();
				if ( const CAnimationBufferBitwiseCompressed * bitwiseCompressed = Cast< CAnimationBufferBitwiseCompressed >( part ) )
				{
					// sort of hack - required only by bitwise compressed
					m_dT = bitwiseCompressed->GetDeltaFrameTime();
				}

				// prepare place for data
				sourceDataPart.m_numFrames = part->GetFramesNum();
				RedQsTransform* sourceDataBones = new RedQsTransform[sourceDataPart.m_numFrames * m_numBones];
				Float* sourceDataTracks = new Float[sourceDataPart.m_numFrames * m_numTracks];

				// read data frame by frame
				RedQsTransform* tempTransforms = new RedQsTransform[m_numBones];
				Float* tempTracks = new Float[m_numTracks];
				for( Uint32 frameIndex = 0; frameIndex < sourceDataPart.m_numFrames; ++ frameIndex )
				{
					part->Load( frameIndex, m_numBones, tempTransforms, m_numTracks, tempTracks );
					Red::System::MemoryCopy( &sourceDataBones[ frameIndex * m_numBones ], tempTransforms, m_numBones * sizeof(RedQsTransform) );
					Red::System::MemoryCopy( &sourceDataTracks[ frameIndex * m_numTracks ], tempTracks, m_numTracks * sizeof(Float) );
				}
				delete [] tempTransforms;
				delete [] tempTracks;

				// assign pointers (value* to const value*)
				sourceDataPart.m_bones = sourceDataBones;
				sourceDataPart.m_track = sourceDataTracks;
			}
			else
			{
				ASSERT(false, TXT("If anim buffer tells that it has parts, it should have all parts!"));
				Part& sourceDataPart = m_parts[partIndex];
				sourceDataPart.m_bones = nullptr;
				sourceDataPart.m_track = nullptr;
			}
			++ partIndex;
		}
		_animation->ReleaseUsage();
	}
}
#endif

Bool IAnimationBuffer::SourceData::IsValid( const CSkeletalAnimation* animation )
{
	ASSERT( animation->GetAnimBuffer() &&  animation->GetAnimBuffer()->GetFramesNum() != 0, TXT("Trying to validate source data using empty animation %s"), animation->GetName().AsChar());

	String reasonExtraInfo;

	Bool retVal = true;
	if ( const IAnimationBuffer* animBuffer = animation->GetAnimBuffer() )
	{
		//validation
		if ( m_numBones != animBuffer->GetBonesNum() )
		{
			if ( m_numBones == 0 )
			{
				reasonExtraInfo += String(TXT("there are no bones, this may mean that source data was not loaded at all - it was corrupted; "));
			}
			retVal = false;
		}
		else if ( m_numTracks != animBuffer->GetTracksNum() )
		{
			retVal = false;
		}
		else if ( m_parts.Size() != animBuffer->GetPartsNum() )
		{
			retVal = false;
		}
		// more than duration we care about number of frames (?)
		// no need to check if has ref ik bones as number of bones should be enough - and ref ik bones is just general information
		else if ( !m_parts.Empty() )
		{
			Uint32 iPart = 0;
			// I know that c++11 range-based iterators are nice feature, but where should we increase iPart? at the end? forgetting to update iPart may lead to not so great bugs :(
			// for ( const IAnimationBuffer::SourceData::Part& part : m_parts )
			for ( TDynArray< IAnimationBuffer::SourceData::Part >::const_iterator part = m_parts.Begin(), partEnd = m_parts.End(); part != partEnd; ++ part, ++ iPart )
			{
				const IAnimationBuffer* goodPart = animBuffer->GetPart( iPart );
				if ( part->m_numFrames != goodPart->GetFramesNum() )
				{
					retVal = false;
					break;
				}

				if ( Abs( m_dT - 1.0f / animation->GetFramesPerSecond() ) > 0.001f )
				{
					const CAnimationBufferBitwiseCompressed* compressed = Cast< CAnimationBufferBitwiseCompressed >( goodPart );
					if ( compressed == nullptr || Abs( m_dT - compressed->GetDeltaFrameTime() ) > 0.001f )
					{
						retVal = false;
						break;
					}
				}

				Uint32 bonesSize = part->m_numFrames * m_numBones;

				for ( Uint32 iBone = 0; iBone < bonesSize; ++iBone )
				{
					const RedQsTransform* bone = &part->m_bones[iBone];
					if ( !bone )
					{
						retVal = false;
						break;
					}
					if ( !bone->GetTranslation().IsOk() )
					{
						retVal = false;
						break;
					}
					if ( !bone->GetScale().IsOk() )
					{
						retVal = false;
						break;
					}
					if ( MAbs( 1.0f - bone->GetRotation().Quat.Length4() ) > 0.02f ) // we don't want to be too restrictive. we're just checking if animation buffer doesn't contain any rubbish data (not normalized quaternion is ok)
					{
						retVal = false;
						break;
					}
					// don't check for valid axis of quaternion (x,y,z length) as for identity quaternion (0,0,0,1) it will fail! (!bone->GetRotation().HasValidAxis())
				}

				if ( !retVal )
				{
					break;
				}
			}
		}
	}

	if ( !retVal )
	{
		String filePath = animation->GetEntry()->GetAnimSet() ? animation->GetEntry()->GetAnimSet()->GetDepotPath() : String::EMPTY;
		RED_LOG_ERROR( RED_LOG_CHANNEL( AnimationSourceValidation ), TXT( "IAnimationBuffer::SourceData validation failed for file %ls, animation %ls, extra info: %ls" ),
			filePath.AsChar(), animation->GetName().AsString().AsChar(), reasonExtraInfo.AsChar() );
	}

	return retVal;
}