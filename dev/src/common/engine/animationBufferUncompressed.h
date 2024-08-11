/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CAnimationBufferUncompressed : public IAnimationBuffer
{
	DECLARE_RTTI_SIMPLE_CLASS( CAnimationBufferUncompressed );

protected:
	//! Number of frames
	Uint32 m_numFrames;

	//! Number of bone tracks
	Uint32 m_numBones;

	//! Number of float tracks
	Uint32 m_numTracks;

	//! Number of dynamic tracks
	Uint32 m_numDynamicTracks;
	
	//! Key masks (const vs non-const)
	TDynArray< Uint8, MC_Animation, MemoryPool_Animation > m_keyMask;

	//! Bone keys tracks (per frame, per bone)
	TDynArray< Float, MC_Animation, MemoryPool_Animation > m_boneKeys;

	//! Const key values
	TDynArray< Float, MC_Animation, MemoryPool_Animation > m_constKeys;

	//! Float track keys (per frame, per track)
	TDynArray< Float, MC_Animation, MemoryPool_Animation > m_trackKeys;

#ifdef USE_REF_IK_MISSING_BONES_HACK
	Bool m_hasRefIKBones;
#endif

public:
	CAnimationBufferUncompressed();
	virtual ~CAnimationBufferUncompressed();

	//! Serialize data
	virtual void OnSerialize( IFile& file );

	//! IAnimationBuffer interface implementation
	virtual void Preload() {}
	virtual void SyncLoad() {}
	virtual EAnimationBufferDataAvailable GetAnimationBufferDataAvailable( Uint32 bonesRequested, Uint32 & outBonesLoaded, Uint32 & outBonesAlwaysLoaded ) { outBonesLoaded = (Uint32) -1; outBonesAlwaysLoaded = (Uint32) -1; return ABDA_All; }
	virtual Bool SetStreamingOption( SAnimationBufferStreamingOption streamingOption, Uint32 nonStreamableBones ) { return false; }
	virtual void AddUsage( DEBUG_ANIMATION_USAGE_PARAM_LIST ) {}
	virtual void ReleaseUsage() {}
	virtual Uint32 GetFramesNum() const;
	virtual Uint32 GetBonesNum() const;
	virtual Uint32 GetTracksNum() const;
	virtual Uint32 GetPartsNum() const;
	virtual IAnimationBuffer const * GetPart(Uint32 partIndex) const;
	virtual Bool IsCompressed() const;
	virtual Bool Initialize( const CSkeletalAnimationSet* animSet, CSkeletalAnimation* animation, const SourceData& sourceData, IAnimationBuffer* previousAnimBuffer, Bool preferBetterQuality );
	virtual Bool Load( const Uint32 frameIndex, const Uint32 numBones, RedQsTransform* outTransforms, const Uint32 numTracks, Float* outValues, Bool fallbackOnly ) const;
	virtual Bool Sample( const Float frame, const Uint32 numBones, RedQsTransform* outTransforms, const Uint32 numTracks, Float* outTracks, Bool fallbackOnly ) const;
	virtual void GetMemorySize(Uint32 & outNonStreamable, Uint32 & outStreamableLoaded, Uint32 & outStreamableTotal, Uint32 & outSourceDataSize) const;
	virtual void SetCompressionParams( SAnimationBufferBitwiseCompressionPreset const & compressionPreset, SAnimationBufferBitwiseCompressionSettings const & compressionSettings ) {}

public:
#ifdef USE_REF_IK_MISSING_BONES_HACK
	virtual Bool HasRefIKBones() const { return m_hasRefIKBones; }
#endif

	//! Initialize from a range of frames
	Bool Initialize( const SourceData& sourceData, Uint32 firstFrame, Uint32 numFrames );

	Uint64 GetDataCRC( Uint64 seed = 0 ) const override;
};

BEGIN_CLASS_RTTI(CAnimationBufferUncompressed);
	PARENT_CLASS(IAnimationBuffer);
	PROPERTY(m_numFrames);
	PROPERTY(m_numBones);
	PROPERTY(m_numTracks);
	PROPERTY(m_numDynamicTracks);
	//PROPERTY(m_keyMask); // serialized directly in OnSerialize
	//PROPERTY(m_constKeys); // serialized directly in OnSerialize
	//PROPERTY(m_boneKeys); // serialized directly in OnSerialize
	//PROPERTY(m_trackKeys); // serialized directly in OnSerialize
#ifdef USE_REF_IK_MISSING_BONES_HACK
	PROPERTY(m_hasRefIKBones);
#endif
END_CLASS_RTTI();
