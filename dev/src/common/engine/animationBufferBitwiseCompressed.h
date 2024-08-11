/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "animationBuffer.h"
#include "../redMath/redmathbase.h"
#include "..\core\deferredDataBuffer.h"

#define USE_RED_ANIM_COMPRESSION

#ifdef NO_EDITOR
#define ANIM_COMPRESSION_SCALE_OPTS
#endif

class CAnimationBufferBitwiseCompressed;

//
//	More info about bitwise animation compression can be found at
//	https://docs.google.com/a/cdprojektred.com/document/d/1emcS3Ft4F-APEHePgytrUvoFzw7jAqlHR-LZmnEWZxw/edit?usp=sharing
//

///////////////////////////////////////////////////////////////////////////////

struct SAnimationBufferBitwiseCompressedData;
struct SAnimationBufferBitwiseCompressedBoneTrack;

typedef const Int8 * AnimationConstDataPtr;
typedef TDynArray< Int8, MC_BufferAnimation, MemoryPool_Animation > AnimationDataBuffer;
typedef TDynArray< SAnimationBufferBitwiseCompressedBoneTrack, MC_BufferAnimation, MemoryPool_Animation > AnimationBoneTrackArray;
typedef TDynArray< SAnimationBufferBitwiseCompressedData, MC_BufferAnimation, MemoryPool_Animation > AnimationTracksArray;

void FlushAllAnimationBuffer();

struct SAnimationBufferBitwiseCompressionWorkerBoneInfo
{
	Uint32 m_positionSingleCompressedDataSize;
	Uint32 m_orientationSingleCompressedDataSize;
	Uint32 m_scaleSingleCompressedDataSize;
	Uint32 m_requiresUpToDataSize;
};

struct SAnimationBufferBitwiseCompressionWorkerTrackInfo
{
	Uint32 m_singleCompressedDataSize;
};

struct SAnimationBufferBitwiseCompressedData
{
	DECLARE_RTTI_STRUCT( SAnimationBufferBitwiseCompressedData );

	// please note that when dt changes, last frame may not be at the same time as duration (if calculated from dt * numFrames, this has to be fixed when uncompressing data)
	Float							m_dt;				//! delta time between frames
	Int8							m_compression;		//! compression type (uses SAnimationBufferBitwiseCompression) but stores in 8 bytes
	Uint16							m_numFrames;		//! number of frames
	Uint32							m_dataAddr;			//! address in data array
	Uint32							m_dataAddrFallback;	//! fallback for first frame when no data exists - this either uses data array or data fallback array (for extra bits that can't be found in data array)

	//! returns true if requested tolerance was met
	Bool Compress( CAnimationBufferBitwiseCompressed* forAnimBuffer, Uint32 numChannels, Uint32 numFrames, Float dt, Float* rawData, Uint32 & outSingleCompressedDataSize, Float tolerance, Float frameSkipTolerance, SAnimationBufferDataCompressionMethod method = ABDCM_Plain );

	static void Uncompress( const SAnimationBufferBitwiseCompressedData &  data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float time, Float duration, Float* out );
	static void UncompressQuaternion( const SAnimationBufferBitwiseCompressedData &  data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float time, Float duration, Float* out );
	static void UncompressQuaternionXYZSignedW( const SAnimationBufferBitwiseCompressedData &  data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float time, Float duration, Float* out );
	static void UncompressQuaternionXYZSignedWInLastBit( const SAnimationBufferBitwiseCompressedData &  data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float time, Float duration, Float* out );
	static void UncompressQuaternion64bXYZW( const SAnimationBufferBitwiseCompressedData &  data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float time, Float duration, Float* out );
	static void UncompressQuaternion48bXYZ( const SAnimationBufferBitwiseCompressedData &  data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float time, Float duration, Float* out );
	static void UncompressQuaternion48bXYZW( const SAnimationBufferBitwiseCompressedData &  data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float time, Float duration, Float* out );
	static void UncompressQuaternion40bXYZ( const SAnimationBufferBitwiseCompressedData &  data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float time, Float duration, Float* out );
	static void UncompressQuaternion40bXYZW( const SAnimationBufferBitwiseCompressedData &  data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float time, Float duration, Float* out );
	static void UncompressQuaternion32bXYZ( const SAnimationBufferBitwiseCompressedData &  data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float time, Float duration, Float* out );

	static void UncompressFallback( const SAnimationBufferBitwiseCompressedData &  data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float* out );
	static void UncompressFallbackQuaternion( const SAnimationBufferBitwiseCompressedData &  data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float* out );
	static void UncompressFallbackQuaternionXYZSignedW( const SAnimationBufferBitwiseCompressedData &  data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float* out );
	static void UncompressFallbackQuaternionXYZSignedWInLastBit( const SAnimationBufferBitwiseCompressedData &  data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float* out );
	static void UncompressFallbackQuaternion64bXYZW( const SAnimationBufferBitwiseCompressedData &  data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float* out );
	static void UncompressFallbackQuaternion48bXYZ( const SAnimationBufferBitwiseCompressedData &  data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float* out );
	static void UncompressFallbackQuaternion48bXYZW( const SAnimationBufferBitwiseCompressedData &  data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float* out );
	static void UncompressFallbackQuaternion40bXYZ( const SAnimationBufferBitwiseCompressedData &  data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float* out );
	static void UncompressFallbackQuaternion40bXYZW( const SAnimationBufferBitwiseCompressedData &  data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float* out );
	static void UncompressFallbackQuaternion32bXYZ( const SAnimationBufferBitwiseCompressedData &  data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float* out );

	static Bool FindWithinExistingData( const Int8 * newData, Uint32 newDataSize, const AnimationDataBuffer& alreadyCompressedData, Uint32 & outFoundAtAddr );

private:
	Bool CheckIfCanCompressAsConstant( Uint32 numChannels, Float* rawData, Float tolerance, SAnimationBufferDataCompressionMethod method ) const;
	Bool TryToCompressUsingCompression( Uint32 numChannels, Float dt, Float* rawData, Float tolerance, Float frameSkipTolerance, AnimationDataBuffer& outData, Uint32 & outSingleCompressedDataSize, Float& outDt, Uint32& outNumFrames, SAnimationBufferBitwiseCompression compression, SAnimationBufferDataCompressionMethod method );
	Bool TryToCompressUsingCompressionWithFrameSkip( Uint32 numChannels, Float dt, Uint32 frameSkip, Float* rawData, Float tolerance, AnimationDataBuffer& outData, Uint32 & outSingleCompressedDataSize, Float& outDt, Uint32& outNumFrames, SAnimationBufferBitwiseCompression compression, SAnimationBufferDataCompressionMethod method );

	void UncompressFrameWorker( Uint32 numChannelsCorrected, Uint32 singleDataSize, const Int8*& dataPtr, Float*& out ) const;
	void UncompressFrame( Uint32 numChannelsCorrected, Uint32 singleDataSize, const Int8* dataPtr, Float* out ) const;
	void UncompressFrameQuaternionXYZSignedW( Uint32 singleDataSize, const Int8* dataPtr, Float* out ) const;
	void UncompressFrameQuaternionXYZSignedWInLastBit( Uint32 singleDataSize, const Int8* dataPtr, Float* out ) const;
	void UncompressFrameQuaternion64bXYZW( Uint32 singleDataSize, const Int8* dataPtr, Float* out ) const;
	void UncompressFrameQuaternion48bXYZ( Uint32 singleDataSize, const Int8* dataPtr, Float* out ) const;
	void UncompressFrameQuaternion48bXYZW( Uint32 singleDataSize, const Int8* dataPtr, Float* out ) const;
	void UncompressFrameQuaternion40bXYZ( Uint32 singleDataSize, const Int8* dataPtr, Float* out ) const;
	void UncompressFrameQuaternion40bXYZW( Uint32 singleDataSize, const Int8* dataPtr, Float* out ) const;
	void UncompressFrameQuaternion32bXYZ( Uint32 singleDataSize, const Int8* dataPtr, Float* out ) const;
	void CalculateWofQuaternion( Float* out ) const;
	void NormalizeQuaternion( Float* out ) const;

	// utils
	static Uint32 SizeOfSingleData( SAnimationBufferBitwiseCompression compression );
	static Float CompressSingleData( Float value, Int8* where, SAnimationBufferBitwiseCompression compression );
	static void UncompressSingleDataStream( const Int8*& from, Float*& out, Uint32 singleDataSize, Uint32 numDecompressions, SAnimationBufferBitwiseCompression compression );
	static Bool MatchesTolerance( Float value, Float other, Float tolerance );
	static Bool CheckIfDataTheSame( const Int8 * newData, Uint32 newDataSize, const AnimationDataBuffer& alreadyCompressedData, Uint32 alreadyCompressedDataAddr );
};

BEGIN_CLASS_RTTI( SAnimationBufferBitwiseCompressedData );
	PROPERTY( m_dt )
	PROPERTY( m_compression )
	PROPERTY( m_numFrames )
	PROPERTY( m_dataAddr )
	PROPERTY( m_dataAddrFallback )
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

struct SAnimationBufferBitwiseCompressedBoneTrack
{
	DECLARE_RTTI_STRUCT( SAnimationBufferBitwiseCompressedBoneTrack );
	SAnimationBufferBitwiseCompressedData							m_position;
	SAnimationBufferBitwiseCompressedData							m_orientation;
#ifndef ANIM_COMPRESSION_SCALE_OPTS
	SAnimationBufferBitwiseCompressedData							m_scale;
#endif
};

BEGIN_CLASS_RTTI( SAnimationBufferBitwiseCompressedBoneTrack );
	PROPERTY( m_position )
	PROPERTY( m_orientation )
#ifndef ANIM_COMPRESSION_SCALE_OPTS
	PROPERTY( m_scale )
#endif
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

typedef Red::TAtomicSharedPtr< BufferProxy > AnimationDeferredDataBufferHandle;
typedef Red::TAtomicWeakPtr< BufferProxy > AnimationDeferredDataBufferPtr;

///////////////////////////////////////////////////////////////////////////////

class CAnimationBufferBitwiseCompressed : public IAnimationBuffer
{
	DECLARE_RTTI_SIMPLE_CLASS( CAnimationBufferBitwiseCompressed );

	/**
	 *	Actual data is held in few arrays.
	 *
	 *		m_data			- always in memory - holds information about animation that is not streamed (whole animation / non streamable bones / or nothing if whole animation is streamed)
	 *		m_fallbackData	- always in memory - holds information about first frame just in case we're waiting for animation to be streamed in
	 *		m_deferredData	- streamed on demand - holds information about animation (or streamable bones) that is loaded on demand
	 *
	 *		If it is possible, data form existing buffer is reused, although m_deferredData and m_fallbackData can't share data between each other.
	 *		m_deferredData abd m_fallbackData may share data with m_data.
	 *
	 *	m_dataAddr points at data in one of those two buffers:
	 *		[------------ m_data ------------][-------------- m_deferredData --------------]
	 *								  ^							^
	 *								  |							|
	 *	  one.m_dataAddr -------------+							+------- other.m_dataAddr 
	 *
	 *	m_dataAddrFallback points at data in one of those two buffers:
	 *		[------------ m_data ------------][-- m_fallbackData --]
	 *								     ^				^
	 *								     |				|
	 *	  one.m_dataAddrFallback --------+				+------- other.m_dataAddrFallback 
	 *
	 *	For every bone and track there is structure that describes where data is held and how it should be decompressed.
	 *	Positions, orientations and scales are compressed as sets of 3/4 values together (no separate tracks for X,Y,Z).
	 *
	 *	Each compressed set might be compressed:
	 *		as constant value if all values fall into tolerance range
	 *		with some frames skipped (when they're interpolated and fall into tolerance range)
	 *		with every frame
	 *
	 *	Positions, scales and tracks are compressed as float values depending on tolerance:
	 *		as 4 bytes (all 4 bytes of float are stored)
	 *		as 3 bytes (the least important byte is dropped and filled with 0 on decompression)
	 *		as 2 bytes (the two least important bytes are dropped and filled with 0 on decompression)
	 *
	 *	Quaternions can be compressed as float values but it is inefficient and doesn't give that good quality.
	 *	That's why each value is compressed as discrete value from range -1 to 1.
	 *	W component is only stored as sign and restored during decompression by calculating missing part with assumption that each quaternion should be normalized.
	 *	Quaternions can be compressed:
	 *		in 64bits (8 bytes) X:16b, Y:16b, Z:16b, W:16b	
	 *		in 48bits (6 bytes) X:16b, Y:16b, Z:15b, W:1b	
	 *		in 48bits (6 bytes) X:12b, Y:12b, Z:12b, W:12b	
	 *		in 40bits (5 bytes) X:13b, Y:13b, Z:13b, W:1b	(good quality for most things)
	 *		in 40bits (5 bytes) X:10b, Y:10b, Z:10b, W:10b	
	 *		in 32bits (4 bytes) X:10b, Y:10b, Z:11b, W:1b	(very low quality)	
	 */
public:
	Uint32														m_version;
	AnimationBoneTrackArray										m_bones;
	AnimationTracksArray										m_tracks;

	AnimationDataBuffer											m_data;							//! this is compressed data for non streaming
	AnimationDataBuffer											m_fallbackData;					//! this is compressed fallback data (extra that didn't have match in m_data)

	DeferredDataBuffer											m_deferredData;					//! this is compressed data for streaming
	AnimationDeferredDataBufferPtr								m_dataPtr;						//! this is weak pointer to deferred data buffer
	Int32														m_ringIndex;
	SAnimationBufferStreamingOption								m_streamingOption;				//! should this buffer be streamable and how it should be streamed? during saving it will save as either deferred data buffer or data embedded
	Uint32														m_nonStreamableBones;			//! how many bones are non streamable

	SAnimationBufferOrientationCompressionMethod				m_orientationCompressionMethod;	//! how quaternions are compressed (compression settings/preset can be changed without reimporting and this is actual compression method, and it is cooked)
	Float														m_duration;						//! duration calculated as (numframes - 1) * dt (note: there is start and end frame)
	Uint32														m_numFrames;					//! number of frames in loaded animation
	Float														m_dt;							//! delta time for frames in loaded animation

#ifndef NO_EDITOR
	Uint32														m_sourceDataSize;				//! just for stats
#endif

#ifdef USE_REF_IK_MISSING_BONES_HACK
	Bool														m_hasRefIKBones;
#endif

#ifndef NO_EDITOR
protected: friend class CSkeletalAnimation;
	SAnimationBufferBitwiseCompressionPreset					m_compressionPreset;
	SAnimationBufferBitwiseCompressionSettings					m_compressionSettings;
#endif

public:
	CAnimationBufferBitwiseCompressed();
	virtual ~CAnimationBufferBitwiseCompressed();

	RED_INLINE Int32 GetRingIndex() const { return m_ringIndex; }

	virtual void OnPropertyPostChange( IProperty* prop );

	//! IAnimationBuffer interface implementation
	virtual void Preload();
	virtual void SyncLoad();
	virtual EAnimationBufferDataAvailable GetAnimationBufferDataAvailable( Uint32 bonesRequested, Uint32 & outBonesLoaded, Uint32 & outBonesAlwaysLoaded );
	virtual Bool SetStreamingOption( SAnimationBufferStreamingOption streamingOption, Uint32 nonStreamableBones );
	virtual void AddUsage( DEBUG_ANIMATION_USAGE_PARAM_LIST );
	virtual void ReleaseUsage();
	virtual Uint32 GetFramesNum() const;
	virtual Uint32 GetBonesNum() const;
	virtual Uint32 GetTracksNum() const;
	virtual Uint32 GetPartsNum() const;
	virtual IAnimationBuffer const * GetPart(Uint32 partIndex) const;
	virtual Bool IsCompressed() const;
	virtual Bool Initialize( const CSkeletalAnimationSet* animSet, CSkeletalAnimation* animation, const SourceData& sourceData, IAnimationBuffer* previousAnimBuffer, Bool preferBetterQuality );
	virtual Bool Load( const Uint32 frameIndex, const Uint32 numBones, RedQsTransform* outTransforms, const Uint32 numTracks, Float* outValues, Bool fallbackOnly ) const;
	virtual void GetMemorySize(Uint32 & outNonStreamable, Uint32 & outStreamableLoaded, Uint32 & outStreamableTotal, Uint32 & outSourceDataSize) const;
	virtual void SetCompressionParams( const SAnimationBufferBitwiseCompressionPreset & compressionPreset, const SAnimationBufferBitwiseCompressionSettings & compressionSettings );

#ifndef NO_EDITOR
	void GetPartiallyStreamableMemoryStats(Uint32 & outNonStreamable, Uint32 & outPartiallyStreamable) const;
#endif

	Float GetDeltaFrameTime() const { return m_dt; }

#ifdef USE_REF_IK_MISSING_BONES_HACK
	virtual Bool HasRefIKBones() const { return m_hasRefIKBones; }
#endif

	virtual Bool Sample( const Float time, const Uint32 numBones, RedQsTransform* outTransforms, const Uint32 numTracks, Float* outValues, Bool fallbackOnly ) const;

	//! Initialize from a range of frames
	Bool Initialize( const SourceData& sourceData, Uint32 firstFrame, Uint32 numFrames );

public:
	static void GetStreamingNumbers(Uint32 & outStreamedInAnimations, Uint32 & outUsedStreamedInAnimations, Uint32 & outAnimationsBeingStreamedIn);
	static void GetLimitsForStreamedData(Uint32 & outStreamedAnimLimit, Uint32 & outMemoryLimit);

protected:
	Bool LoadAtTime( const Float time, const Uint32 numBones, RedQsTransform* outTransforms, const Uint32 numTracks, Float* outValues, Bool fallbackOnly ) const;

	EAnimationBufferDataAvailable GetAnimationBufferDataAvailableLoadIfNeeded( Uint32 bonesRequested, Uint32 & outBonesLoaded, Uint32 & outBonesAlwaysLoaded, Bool loadSync = false );

	typedef void (*UncompressOrientationFunction)( const SAnimationBufferBitwiseCompressedData &  data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float time, Float duration, Float* out );
	typedef void (*UncompressFallbackOrientationFunction)( const SAnimationBufferBitwiseCompressedData &  data, Uint32 firstCompDataSize, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, Uint32 numChannels, Float* out );

	RED_INLINE void DecompressUsingMethods( const Float time, const Uint32 numBones, const Uint32 numfallbackBones, RedQsTransform* outTransforms,
		Uint32 firstCompDataSize, Uint32 firstCompDataSizeForFallback, AnimationConstDataPtr firstCompData, AnimationConstDataPtr secondCompData, AnimationConstDataPtr fallbackCompData,
		UncompressOrientationFunction UncompressOrientationMethod,
		UncompressFallbackOrientationFunction UncompressFallbackOrientationMethod ) const;

	friend struct SAnimationBufferBitwiseCompressedData;
	friend struct SBitwiseCompressionDataRingBufferEntry;
	friend struct SBitwiseCompressionDataRingBuffer;

private:
	// data manipulation
	void AppendDeferredToData();
	void MoveDataToDeferred( Uint32 startAt = 0 );
	void FindFallbackOrAdd( const AnimationDataBuffer & wholeData, SAnimationBufferBitwiseCompressedData & compressedDataInfo, Uint32 singleCompressedDataSize );

	void ApplyChangesToDataDueToStreamingAndFillFallback( const TDynArray<SAnimationBufferBitwiseCompressionWorkerBoneInfo> & compressionWorkerBoneInfo, const TDynArray<SAnimationBufferBitwiseCompressionWorkerTrackInfo> & compressionWorkerTrackInfo, CSkeletalAnimation* animation, const SourceData& sourceData );

public:
	Uint64 GetDataCRC( Uint64 seed = 0 ) const override;
};

BEGIN_CLASS_RTTI( CAnimationBufferBitwiseCompressed );
	PARENT_CLASS( IAnimationBuffer );
#ifndef NO_EDITOR
	PROPERTY_EDIT_NOT_COOKED( m_compressionPreset, TXT("Bitwise compression preset") )
	PROPERTY_EDIT_NOT_COOKED( m_compressionSettings, TXT("Bitwise compression settings") );
	PROPERTY_NOT_COOKED( m_sourceDataSize );
#endif
	PROPERTY( m_version );
	PROPERTY( m_bones );
	PROPERTY( m_tracks );
	PROPERTY( m_data );
	PROPERTY( m_fallbackData );
	PROPERTY( m_deferredData );
	PROPERTY( m_orientationCompressionMethod );
	PROPERTY( m_duration );
	PROPERTY( m_numFrames );
	PROPERTY( m_dt );
	PROPERTY( m_streamingOption );
	PROPERTY( m_nonStreamableBones );
#ifdef USE_REF_IK_MISSING_BONES_HACK
	PROPERTY_RO( m_hasRefIKBones, TXT("Has Reference/IK bones info") );
#endif
END_CLASS_RTTI();
