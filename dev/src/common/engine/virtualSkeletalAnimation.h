
#include "virtualAnimation.h"
#include "virtualAnimationMixer.h"

#pragma once

class CVirtualSkeletalAnimation : public CSkeletalAnimation, public IVirtualAnimationContainer
{
	DECLARE_RTTI_SIMPLE_CLASS( CVirtualSkeletalAnimation );

private:
	TDynArray< VirtualAnimation >			m_virtualAnimations;
	TDynArray< VirtualAnimation >			m_virtualAnimationsOverride;
	TDynArray< VirtualAnimation >			m_virtualAnimationsAdditive;

	TDynArray< VirtualAnimationMotion >		m_virtualMotions;
	TDynArray< VirtualAnimationPoseFK >		m_virtualFKs;
	TDynArray< VirtualAnimationPoseIK >		m_virtualIKs;

	const TCrDefinition*					m_controlRigDef;
	TCrInstance*							m_controlRig;

	VirtualAnimationMixer					m_mixer;

	Bool									m_cachedProps;
	Bool									m_cachedHasMotionExtraction;
	Uint32									m_cachedBonesNum;
	Uint32									m_cachedTracksNum;

public:
	CVirtualSkeletalAnimation();
	virtual ~CVirtualSkeletalAnimation();

	virtual void OnSerialize( IFile &file );
	virtual void Bind( CSkeletalAnimationSetEntry* animationSetEntry, CSkeletalAnimationSet* animationSet );

public:
	virtual const TDynArray< VirtualAnimation >&		GetVirtualAnimations( EVirtualAnimationTrack track ) const;
	virtual const TDynArray< VirtualAnimationMotion >&	GetVirtualMotions() const;
	virtual const TDynArray< VirtualAnimationPoseFK >&	GetVirtualFKs() const;
	virtual const TDynArray< VirtualAnimationPoseIK >&	GetVirtualIKs() const;

#ifndef NO_EDITOR
	Bool AddAnimation( VirtualAnimation& anim, EVirtualAnimationTrack track );
	Bool RemoveAnimation( const VirtualAnimationID& animation );
	void SetAnimation( const VirtualAnimationID& animation , const VirtualAnimation& dest );

	Bool AddMotion( VirtualAnimationMotion& motion );
	Bool RemoveMotion( const VirtualAnimationMotionID& motion );
	void SetMotion( const VirtualAnimationMotionID& motion , const VirtualAnimationMotion& dest );

	Bool AddFK( VirtualAnimationPoseFK& data );
	Bool RemoveFK( const VirtualAnimationPoseFKID& dataID );
	void SetFK( const VirtualAnimationPoseFKID& dataID , const VirtualAnimationPoseFK& data );

	Bool AddIK( VirtualAnimationPoseIK& data );
	Bool RemoveIK( const VirtualAnimationPoseIKID& dataID );
	void SetIK( const VirtualAnimationPoseIKID& dataID , const VirtualAnimationPoseIK& data );

public:
	TDynArray< VirtualAnimation >& GetVirtualAnimations( EVirtualAnimationTrack track );
	TDynArray< VirtualAnimationPoseFK >& GetVirtualFKs();
	TDynArray< VirtualAnimationPoseIK >& GetVirtualIKs();
#endif

#ifndef NO_EDITOR
public:
	void ConnectEditorMixerLinstener( VirtualAnimationMixerEdListener* listener );
#endif

public:
	virtual Bool GenerateBoundingBox( const CAnimatedComponent* component );

	virtual Bool HasExtractedMotion() const; 

	virtual RedQsTransform GetMovementAtTime( Float time ) const;
	virtual RedQsTransform GetMovementBetweenTime( Float startTime, Float endTime, Int32 loops ) const;

	virtual Bool IsCompressed() const;

public:
	void Touch() const;

	virtual void Preload() const;
	virtual void SyncLoad() const;

	virtual EAnimationBufferDataAvailable GetAnimationBufferDataAvailable( Uint32 bonesRequested, Uint32 & outBonesLoaded, Uint32 & outBonesAlwaysLoaded ) const;

	virtual Uint32 GetTracksNum() const;
	virtual Uint32 GetBonesNum() const;

	virtual Bool Sample( Float time, Uint32 boneNumIn, Uint32 tracksNumIn, RedQsTransform* bonesOut, Float* tracksOut ) const override final;
	virtual Bool Sample( Float time, TDynArray< RedQsTransform >& bonesOut, TDynArray< Float >& tracksOut ) const override final;

	virtual Bool SampleFallback( Uint32 boneNumIn, Uint32 tracksNumIn, AnimQsTransform* bonesOut, AnimFloat* tracksOut ) const;
	
private:
	void CacheProperties();

private:
	class InternalAnimIterator
	{
		TDynArray< TDynArray< VirtualAnimation >* > m_data;
		Uint32										m_arrayIndex;
		Int32											m_elemIndex;

	public:
		RED_INLINE InternalAnimIterator( CVirtualSkeletalAnimation* animation )
			: m_arrayIndex( 0 ), m_elemIndex( 0 )
		{
			m_data.PushBack( &(animation->m_virtualAnimations) );
			m_data.PushBack( &(animation->m_virtualAnimationsAdditive) );
			m_data.PushBack( &(animation->m_virtualAnimationsOverride) );

			Init();
		}

		RED_INLINE operator Bool () const
		{
			return m_arrayIndex < m_data.Size();
		}

		RED_INLINE void operator++ ()
		{
			Next();
		}

		RED_INLINE VirtualAnimation& operator*()
		{
			return (*m_data[m_arrayIndex])[m_elemIndex];
		}

	private:
		RED_INLINE void Init()
		{
			if ( m_arrayIndex < m_data.Size() && m_elemIndex >= m_data[ m_arrayIndex ]->SizeInt() )
			{
				Next();
			}
		}

		void Next()
		{
			TDynArray< VirtualAnimation >* arr = m_data[ m_arrayIndex ];
			if ( m_elemIndex + 1 < arr->SizeInt() )
			{
				m_elemIndex++;
			}
			else
			{
				m_arrayIndex++;
				m_elemIndex = -1;

				if ( m_arrayIndex < m_data.Size() )
				{
					Next();
				}
			}
		}
	};
};

BEGIN_CLASS_RTTI( CVirtualSkeletalAnimation );
	PARENT_CLASS( CSkeletalAnimation );
	PROPERTY( m_virtualAnimations );
	PROPERTY( m_virtualAnimationsOverride );
	PROPERTY( m_virtualAnimationsAdditive );
	PROPERTY( m_virtualMotions );
	PROPERTY( m_virtualFKs );
	PROPERTY( m_virtualIKs );
END_CLASS_RTTI();

