
#include "build.h"
#include "virtualSkeletalAnimation.h"
#include "controlRig.h"
#include "controlRigDefinition.h"
#include "skeletalAnimationSet.h"
#include "skeleton.h"

IMPLEMENT_ENGINE_CLASS( CVirtualSkeletalAnimation );

RED_DISABLE_WARNING_MSC( 4355 )

CVirtualSkeletalAnimation::CVirtualSkeletalAnimation()
	: m_mixer( this )
	, m_cachedHasMotionExtraction( true )
	, m_cachedBonesNum( 0 )
	, m_cachedTracksNum( 0 )
	, m_cachedProps( false )
	, m_controlRig( NULL )
	, m_controlRigDef( NULL )
{
#ifdef USE_HAVOK_ANIMATION
	m_animTimes.Resize( 1 );
	m_animTimes[ 0 ] = 1.f;
#else
	m_duration = 1.0f;
	m_framesPerSecond = 30.0f;
#endif
}

CVirtualSkeletalAnimation::~CVirtualSkeletalAnimation()
{
	delete m_controlRig;
	m_controlRig = NULL;
}

void CVirtualSkeletalAnimation::Bind( CSkeletalAnimationSetEntry* animationSetEntry, CSkeletalAnimationSet* animationSet )
{
	CSkeletalAnimation::Bind( animationSetEntry, animationSet );

	if ( animationSet )
	{
		const CSkeleton* skeleton = animationSet->GetSkeleton();
		if ( skeleton && skeleton->GetControlRigDefinition() && skeleton->GetDefaultControlRigPropertySet() )
		{
			m_controlRigDef = skeleton->GetControlRigDefinition();
			m_controlRig = m_controlRigDef->CreateRig( skeleton->GetDefaultControlRigPropertySet() );
		}
	}
}

void CVirtualSkeletalAnimation::OnSerialize( IFile &file )
{
	ISerializable::OnSerialize( file );
}

#ifndef NO_EDITOR

TDynArray< VirtualAnimation >& CVirtualSkeletalAnimation::GetVirtualAnimations( EVirtualAnimationTrack track )
{
	ASSERT( track < VAT_Last );

	switch ( track )
	{
	case VAT_Base:
		return m_virtualAnimations;
	case VAT_Override:
		return m_virtualAnimationsOverride;
	case VAT_Additive:
		return m_virtualAnimationsAdditive;
	}

	ASSERT( 0 );
	return m_virtualAnimations;
}

TDynArray< VirtualAnimationPoseFK >& CVirtualSkeletalAnimation::GetVirtualFKs()
{
	return m_virtualFKs;
}

TDynArray< VirtualAnimationPoseIK >& CVirtualSkeletalAnimation::GetVirtualIKs()
{
	return m_virtualIKs;
}

Bool CVirtualSkeletalAnimation::AddAnimation( VirtualAnimation& anim, EVirtualAnimationTrack track )
{
	ASSERT( track < VAT_Last );

	GetVirtualAnimations( track ).PushBack( anim );

	CacheProperties();

	return true;
}

Bool CVirtualSkeletalAnimation::RemoveAnimation( const VirtualAnimationID& animation )
{
	ASSERT( animation.m_track < VAT_Last );
	ASSERT( animation.m_index < GetVirtualAnimations( animation.m_track ).SizeInt() );

	GetVirtualAnimations( animation.m_track ).RemoveAt( animation.m_index );

	CacheProperties();

	return true;
}

void CVirtualSkeletalAnimation::SetAnimation( const VirtualAnimationID& animation, const VirtualAnimation& dest )
{
	GetVirtualAnimations( animation.m_track )[ animation.m_index ] = dest;

	CacheProperties();
}

Bool CVirtualSkeletalAnimation::AddMotion( VirtualAnimationMotion& motion )
{
	m_virtualMotions.PushBack( motion );

	CacheProperties();

	return true;
}

Bool CVirtualSkeletalAnimation::RemoveMotion( const VirtualAnimationMotionID& motion )
{
	m_virtualMotions.RemoveAt( motion );

	CacheProperties();

	return true;
}

void CVirtualSkeletalAnimation::SetMotion( const VirtualAnimationMotionID& motion , const VirtualAnimationMotion& dest )
{
	ASSERT( motion >= 0 );

	if ( motion < m_virtualMotions.SizeInt() && motion >= 0 )
	{
		m_virtualMotions[ motion ] = dest;

		CacheProperties();
	}
}

Bool CVirtualSkeletalAnimation::AddFK( VirtualAnimationPoseFK& data )
{
	m_virtualFKs.PushBack( data );

	CacheProperties();

	return true;
}

Bool CVirtualSkeletalAnimation::RemoveFK( const VirtualAnimationPoseFKID& dataID )
{
	m_virtualFKs.RemoveAt( dataID );

	CacheProperties();

	return true;
}

void CVirtualSkeletalAnimation::SetFK( const VirtualAnimationPoseFKID& dataID , const VirtualAnimationPoseFK& data )
{
	ASSERT( dataID >= 0 );

	if ( dataID < m_virtualFKs.SizeInt() && dataID >= 0 )
	{
		m_virtualFKs[ dataID ] = data;

		CacheProperties();
	}
}

Bool CVirtualSkeletalAnimation::AddIK( VirtualAnimationPoseIK& data )
{
	m_virtualIKs.PushBack( data );

	CacheProperties();

	return true;
}

Bool CVirtualSkeletalAnimation::RemoveIK( const VirtualAnimationPoseIKID& dataID )
{
	m_virtualIKs.RemoveAt( dataID );

	CacheProperties();

	return true;
}

void CVirtualSkeletalAnimation::SetIK( const VirtualAnimationPoseIKID& dataID , const VirtualAnimationPoseIK& data )
{
	ASSERT( dataID >= 0 );

	if ( dataID < m_virtualIKs.SizeInt() && dataID >= 0 )
	{
		m_virtualIKs[ dataID ] = data;

		CacheProperties();
	}
}

#endif

#ifndef NO_EDITOR

void CVirtualSkeletalAnimation::ConnectEditorMixerLinstener( VirtualAnimationMixerEdListener* listener )
{
	const_cast< VirtualAnimationMixer& >( m_mixer ).ConnectEditorListener( listener );
}

#endif

const TDynArray< VirtualAnimation >& CVirtualSkeletalAnimation::GetVirtualAnimations( EVirtualAnimationTrack track ) const
{
	ASSERT( track < VAT_Last );

	switch ( track )
	{
	case VAT_Base:
		return m_virtualAnimations;
	case VAT_Override:
		return m_virtualAnimationsOverride;
	case VAT_Additive:
		return m_virtualAnimationsAdditive;
	}

	ASSERT( 0 );
	return m_virtualAnimations;
}

const TDynArray< VirtualAnimationMotion >& CVirtualSkeletalAnimation::GetVirtualMotions() const
{
	return m_virtualMotions;
}

const TDynArray< VirtualAnimationPoseFK >& CVirtualSkeletalAnimation::GetVirtualFKs() const
{
	return m_virtualFKs;
}

const TDynArray< VirtualAnimationPoseIK >& CVirtualSkeletalAnimation::GetVirtualIKs() const
{
	return m_virtualIKs;
}

void CVirtualSkeletalAnimation::CacheProperties()
{
	Float min = 0.f;
	Float max = 0.03333333f;
	m_mixer.CalcMinMaxTime( min, max );

#ifdef USE_HAVOK_ANIMATION
	m_animTimes[ 0 ] = max; // not max - min
#endif

	m_cachedBonesNum = 0;
	m_cachedTracksNum = 0;
	m_cachedHasMotionExtraction = !m_virtualMotions.Empty();

	for ( InternalAnimIterator it( this ); it; ++it )
	{
		VirtualAnimation& animation = *it;

		animation.m_cachedAnimation = NULL;

		CSkeletalAnimationSet* set = animation.m_animset.Get();
		if ( set )
		{
			animation.m_cachedAnimation = set->FindAnimation( animation.m_name );
			if ( animation.m_cachedAnimation )
			{
				CSkeletalAnimation* skAnim = animation.m_cachedAnimation->GetAnimation();
				if ( skAnim )
				{
					m_cachedBonesNum = Max( m_cachedBonesNum, skAnim->GetBonesNum() );
					m_cachedTracksNum = Max( m_cachedTracksNum, skAnim->GetTracksNum() );

					m_cachedHasMotionExtraction |= skAnim->HasExtractedMotion();
				}
			}
		}
		else
		{
			BEH_LOG( TXT("Virtual animation error - animset '%ls' is null"), animation.m_animset.GetPath().AsChar() );
		}
	}


}

Bool CVirtualSkeletalAnimation::HasExtractedMotion() const
{
	return m_cachedHasMotionExtraction;
}
#ifdef USE_HAVOK_ANIMATION
hkQsTransform CVirtualSkeletalAnimation::GetMovementAtTime( Float time ) const
{
	return m_mixer.GetMovementAtTime( time );
}

hkQsTransform CVirtualSkeletalAnimation::GetMovementBetweenTime( Float startTime, Float endTime, Int32 loops ) const
{
	return m_mixer.GetMovementBetweenTime( startTime, endTime );
}
#else
RedQsTransform CVirtualSkeletalAnimation::GetMovementAtTime( Float time ) const
{
	return m_mixer.GetMovementAtTime( time );
}

RedQsTransform CVirtualSkeletalAnimation::GetMovementBetweenTime( Float startTime, Float endTime, Int32 loops ) const
{
	return m_mixer.GetMovementBetweenTime( startTime, endTime );
}
#endif
Bool CVirtualSkeletalAnimation::IsCompressed() const
{
	return true;
}

Bool CVirtualSkeletalAnimation::GenerateBoundingBox( const CAnimatedComponent* component )
{
	ASSERT( !TXT("Dont call me!") );
	return false;
}

void CVirtualSkeletalAnimation::Preload() const
{
}	

void CVirtualSkeletalAnimation::SyncLoad() const
{
}

EAnimationBufferDataAvailable CVirtualSkeletalAnimation::GetAnimationBufferDataAvailable( Uint32 bonesRequested, Uint32 & outBonesLoaded, Uint32 & outBonesAlwaysLoaded ) const
{
	outBonesLoaded = bonesRequested;
	outBonesAlwaysLoaded = bonesRequested;
	return ABDA_All;
}

Uint32 CVirtualSkeletalAnimation::GetTracksNum() const
{
	return m_cachedTracksNum;
}

Uint32 CVirtualSkeletalAnimation::GetBonesNum() const
{
	return m_cachedBonesNum;
}
#ifdef USE_HAVOK_ANIMATION
Bool CVirtualSkeletalAnimation::Sample( Float time, Uint32 boneNumIn, Uint32 tracksNumIn, hkQsTransform* bonesOut, hkReal* tracksOut ) const
#else
Bool CVirtualSkeletalAnimation::Sample( Float time, Uint32 boneNumIn, Uint32 tracksNumIn, RedQsTransform* bonesOut, Float* tracksOut ) const
#endif
{
	Touch();

	if ( boneNumIn >= m_cachedBonesNum && tracksNumIn >= m_cachedTracksNum )
	{
		VirtualAnimationMixer::ControlRigSetup cr;
		cr.m_definition = m_controlRigDef;
		cr.m_instance = m_controlRig;
		return m_mixer.Sample( time, boneNumIn, tracksNumIn, bonesOut, tracksOut, &cr );
	}

	return false;
}
#ifdef USE_HAVOK_ANIMATION
Bool CVirtualSkeletalAnimation::Sample( Float time, TDynArray< hkQsTransform >& bonesOut, TDynArray< hkReal >& tracksOut ) const
#else
Bool CVirtualSkeletalAnimation::Sample( Float time, TDynArray< RedQsTransform >& bonesOut, TDynArray< Float >& tracksOut ) const
#endif
{
	Touch();

	if ( bonesOut.Size() >= m_cachedBonesNum && tracksOut.Size() >= m_cachedTracksNum )
	{
		VirtualAnimationMixer::ControlRigSetup cr;
		cr.m_definition = m_controlRigDef;
		cr.m_instance = m_controlRig;
		return m_mixer.Sample( time, bonesOut, tracksOut, &cr );
	}

	return false;
}

Bool CVirtualSkeletalAnimation::SampleFallback( Uint32 boneNumIn, Uint32 tracksNumIn, AnimQsTransform* bonesOut, AnimFloat* tracksOut ) const
{
	return false;
}

void CVirtualSkeletalAnimation::Touch() const
{
	for ( InternalAnimIterator it( const_cast< CVirtualSkeletalAnimation* >( this ) ); it; ++it )
	{
		const VirtualAnimation& animation = *it;

		if ( animation.m_cachedAnimation )
		{
			const CSkeletalAnimation* skAnim = animation.m_cachedAnimation->GetAnimation();
			if ( skAnim )
			{
				skAnim->Touch();
			}
		}
	}

	if ( !m_cachedProps )
	{
		CVirtualSkeletalAnimation* thisAnim = const_cast< CVirtualSkeletalAnimation* >( this );
		thisAnim->CacheProperties();
		thisAnim->m_cachedProps = true;
	}
}
