/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../core/latentDataBuffer.h"

// this hack is to help with rigs that are missing Ref/IK bones
// in the end all rigs should have them, but for time being we have to deal with this monstrosity
// of checking on loading whether animation has such data and reimporting it or filling with mockup info
#define USE_REF_IK_MISSING_BONES_HACK

//#define DEBUG_ANIMATION_USAGE
//#define DEBUG_ANIMATION_USAGE__DETAILED

#ifdef DEBUG_ANIMATION_USAGE
#define DEBUG_ANIMATION_USAGE_PARAM_LIST CName const & animationName
#define DEBUG_ANIMATION_USAGE_PARAM_NAME animationName
#define DEBUG_ANIMATION_USAGE_PARAM_LIST_CONT , DEBUG_ANIMATION_USAGE_PARAM_LIST
#define DEBUG_ANIMATION_USAGE_PARAM_NAME_CONT , animationName
#else
#define DEBUG_ANIMATION_USAGE_PARAM_LIST
#define DEBUG_ANIMATION_USAGE_PARAM_LIST_CONT
#define DEBUG_ANIMATION_USAGE_PARAM_NAME
#define DEBUG_ANIMATION_USAGE_PARAM_NAME_CONT
#endif

class CSkeletalAnimation;
class IAnimationBuffer;
class CSkeletalAnimationSet;

///////////////////////////////////////////////////////////////////////////////

enum EAnimationBufferDataAvailable
{
	ABDA_None, // nothing has been loaded - you will have to rely on compressed pose
	ABDA_Partial, // data is partially available - you will have to rely on compressed pose
	ABDA_All // everything is loaded
};

BEGIN_ENUM_RTTI( EAnimationBufferDataAvailable );
	ENUM_OPTION( ABDA_None );
	ENUM_OPTION( ABDA_Partial );
	ENUM_OPTION( ABDA_All );
END_ENUM_RTTI();

///////////////////////////////////////////////////////////////////////////////

enum SAnimationBufferStreamingOption
{
	ABSO_NonStreamable,
	ABSO_PartiallyStreamable,
	ABSO_FullyStreamable
};

BEGIN_ENUM_RTTI( SAnimationBufferStreamingOption );
	ENUM_OPTION( ABSO_NonStreamable );
	ENUM_OPTION( ABSO_PartiallyStreamable );
	ENUM_OPTION( ABSO_FullyStreamable );
END_ENUM_RTTI();

///////////////////////////////////////////////////////////////////////////////

// see below where compression settings are used
#include "animationBufferBitwiseCompressedStructs.h"
#include "../redMath/redmathbase.h"

/// Animation buffer base interface
class IAnimationBuffer : public ISerializable
{
	DECLARE_RTTI_SIMPLE_CLASS_WITH_ALLOCATOR( IAnimationBuffer, MC_Animation );

public:
	/// Animation buffer source data (factory)
	struct SourceData
	{
		struct Part
		{
			// Number of frames
			Uint32 m_numFrames;

			// Values for keys (per frame, per bone)
			const RedQsTransform* m_bones;

			// Values for tracks (per frame, per track)
			const Float* m_track;

			RED_INLINE Part()
				: m_numFrames( 0 )
				, m_bones( nullptr )
				, m_track( nullptr )
			{}

			RedQsTransform const * GetFrameBoneData(Uint32 frameIdx, Uint32 numBones) const { return &m_bones[frameIdx * numBones]; }
			Float const * GetFrameTrackData(Uint32 frameIdx, Uint32 numTracks) const { return &m_track[frameIdx * numTracks]; }

			Uint32 GetMemorySize(Uint32 numBones, Uint32 numTracks) const { return m_numFrames * ( numBones * sizeof(RedQsTransform) + numTracks * sizeof(RedQsTransform) ); }
		};

		// Number of keys in the data
		Uint32 m_numBones;

		// Number of tracks in the data
		Uint32 m_numTracks;

		// Number of frames in each animation part
		TDynArray< Part > m_parts;

		// Duration for all parts
		Float m_totalDuration;

		// The number of seconds between each sample
		Float m_dT;

#ifdef USE_REF_IK_MISSING_BONES_HACK
		Bool m_hasRefIKBones;
#endif

		RED_INLINE SourceData()
			: m_numBones( 0 )
			, m_numTracks( 0 )
			, m_totalDuration( 1.f )
			, m_dT( 1.f / 30.f )
#ifdef USE_REF_IK_MISSING_BONES_HACK
			, m_hasRefIKBones( false )
#endif
		{}

#ifndef NO_EDITOR
		void BuildFrom(CSkeletalAnimation const * _animation);
#endif

		void CleanUp()
		{
			for (Uint32 i=0; i<m_parts.Size(); ++i)
			{
				delete [] m_parts[i].m_bones;
				delete [] m_parts[i].m_track;
			}
		}

		Uint32 GetMemorySizeOfPart(Uint32 partIdx) const { return m_parts[partIdx].GetMemorySize( m_numBones, m_numTracks ); }

		Bool IsValid( const CSkeletalAnimation* animation );
	};

public:
	virtual ~IAnimationBuffer();

	//! Load this buffer if not loaded (async)
	virtual void Preload() =0;

	//! Load this buffer if not loaded (sync!)
	virtual void SyncLoad() =0;

	//! Check if this buffer is loaded or not (if not loaded, issue loading of it)
	virtual EAnimationBufferDataAvailable GetAnimationBufferDataAvailable( Uint32 bonesRequested, Uint32 & outBonesLoaded, Uint32 & outBonesAlwaysLoaded ) =0;

	//! Mark that this buffer should be streamable (return if that changed)
	virtual Bool SetStreamingOption( SAnimationBufferStreamingOption streamingOption, Uint32 nonStreamableBones ) =0;

	//! Increase usage count - optional but advised to be used - animations with usage count won't be deleted
	virtual void AddUsage( DEBUG_ANIMATION_USAGE_PARAM_LIST ) =0;

	//! Decrease usage count
	virtual void ReleaseUsage() =0;

	//! Get number of frames in the animation
	virtual Uint32 GetFramesNum() const=0;

	//! Get number of bone tracks in the animation
	virtual Uint32 GetBonesNum() const=0;

	//! Get number of additional tracks (float tracks) in the animation
	virtual Uint32 GetTracksNum() const=0;

	//! Get number of parts in this animation buffer
	virtual Uint32 GetPartsNum() const=0;

	//! Get part of this animation buffer
	virtual IAnimationBuffer const * GetPart(Uint32 partIndex) const=0;

	//! Can this buffer be considered as compressed ? 
	virtual Bool IsCompressed() const=0;

	//! Calculate memory consumed by animation data
	virtual void GetMemorySize(Uint32 & outNonStreamable, Uint32 & outStreamableLoaded, Uint32 & outStreamableTotal, Uint32 & outSourceDataSize) const=0;

#ifdef USE_REF_IK_MISSING_BONES_HACK
	virtual Bool HasRefIKBones() const=0;
#endif

	//! Initialize with data (importing, can fail)
	virtual Bool Initialize( const CSkeletalAnimationSet* animSet, CSkeletalAnimation* animation, const SourceData& soruceData, IAnimationBuffer* previousAnimBuffer, Bool preferBetterQuality ) = 0;

	//! Get bone keys and tracks for n-th frame (without interpolation)
	virtual Bool Load( const Uint32 frameIndex, const Uint32 numBones, RedQsTransform* outTransforms, const Uint32 numTracks, Float* outValues, Bool fallbackOnly = false ) const=0;

	//! Set compression parameters (this is hack - result of quickly restoring compression settings per animation)
	virtual void SetCompressionParams( SAnimationBufferBitwiseCompressionPreset const & compressionPreset, SAnimationBufferBitwiseCompressionSettings const & compressionSettings ) = 0;

public:
	//! Sample interpolated bone and track keys at given frame time
	virtual Bool Sample( const Float frame, const Uint32 numBones, RedQsTransform* outTransforms, const Uint32 numTracks, Float* outTracks, Bool fallbackOnly = false ) const=0;

public:
	//! Create animation buffer from initialization data (supports multipart animation)
	static IAnimationBuffer* CreateFromSourceData( const CSkeletalAnimationSet* animSet, CSkeletalAnimation* animation, const SourceData& data, IAnimationBuffer* previousAnimBuffer, Bool preferBetterQuality );

public:
	//! Calculate data CRC
	virtual Uint64 GetDataCRC( Uint64 seed = 0 ) const { return 0; };
};

BEGIN_ABSTRACT_CLASS_RTTI(IAnimationBuffer);
	PARENT_CLASS(ISerializable);
END_CLASS_RTTI();

// data that is stored on disk and used only when recompressing
class LatentSourceAnimDataBuffer : public LatentDataBuffer
{
public:
	//! Load anim data from raw source data and store in buffer
	void LoadAnimDataFrom(IAnimationBuffer::SourceData const & sourceData);

	//! Read anim data from buffer and store it as raw source data
	Bool ReadAnimDataTo(IAnimationBuffer::SourceData & outData, CSkeletalAnimation * skeletalAnimation);
};
