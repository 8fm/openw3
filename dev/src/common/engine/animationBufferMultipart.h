/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CAnimationBufferMultipart : public IAnimationBuffer
{
	DECLARE_RTTI_SIMPLE_CLASS( CAnimationBufferMultipart );

private:
	//! Total number of frames (all parts)
	Uint32 m_numFrames;

	//! Number of bone tracks
	Uint32 m_numBones;

	//! Number of float tracks
	Uint32 m_numTracks;

	//! First frame of each animation segment
	TDynArray< Uint32 > m_firstFrames;

	//! Partial animations
	TDynArray< IAnimationBuffer* > m_parts;

public:
	CAnimationBufferMultipart();
	virtual ~CAnimationBufferMultipart();

	//! Serialize data
	virtual void OnSerialize( IFile& file );

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
	virtual Bool Sample( const Float frame, const Uint32 numBones, RedQsTransform* outTransforms, const Uint32 numTracks, Float* outTracks, Bool fallbackOnly ) const;
	virtual void GetMemorySize(Uint32 & outNonStreamable, Uint32 & outStreamableLoaded, Uint32 & outStreamableTotal, Uint32 & outSourceDataSize) const;
	virtual void SetCompressionParams( SAnimationBufferBitwiseCompressionPreset const & compressionPreset, SAnimationBufferBitwiseCompressionSettings const & compressionSettings );

	const TDynArray< Uint32 >& GetPartsFirstFrames() const;
public:
#ifdef USE_REF_IK_MISSING_BONES_HACK
	virtual Bool HasRefIKBones() const { return m_parts.Empty()? false : m_parts[0]->HasRefIKBones(); }
#endif

	Uint64 GetDataCRC( Uint64 seed = 0 ) const override;
};

BEGIN_CLASS_RTTI(CAnimationBufferMultipart);
	PARENT_CLASS(IAnimationBuffer);
	PROPERTY(m_numFrames);
	PROPERTY(m_numBones);
	PROPERTY(m_numTracks);
	PROPERTY(m_firstFrames);
	PROPERTY_INLINED(m_parts, TXT(""));
END_CLASS_RTTI();

