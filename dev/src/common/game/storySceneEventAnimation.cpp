/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "storySceneEvent.h"
#include "storySceneEventAnimation.h"
#include "storyScenePlayer.h"
#include "storySceneSystem.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/skeletalAnimationContainer.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneEventAnimation );

CStorySceneEventAnimation::CStorySceneEventAnimation() 
	: m_disableLookAt( false )
	, m_disableLookAtSpeed( 1.f )
	, m_useMotionExtraction( false )
	, m_useFakeMotion( true )
	, m_animationType( AAST_Normal )
	, m_addAdditiveType( AT_Local )
	, m_addConvertToAdditive( true )
	, m_gatherSyncTokens( false )
	, m_muteSoundEvents( false )
	, m_useLowerBodyPartsForLookAt( true )
#ifndef NO_EDITOR
	, m_recacheWeightCurve( false )
	, m_weightCurveChanged( false )
#endif
	, m_supportsMotionExClipFront( false )
{}

CStorySceneEventAnimation::CStorySceneEventAnimation( const String& eventName,
	   CStorySceneElement* sceneElement, Float startTime, const CName& actor,
	   const String& animName, const String& trackName)
	: CStorySceneEventAnimClip( eventName, sceneElement, startTime, actor, trackName )
	, m_animationName( animName )
	, m_disableLookAt( false )
	, m_disableLookAtSpeed( 1.f )
	, m_useMotionExtraction( false )
	, m_useFakeMotion( true )
	, m_animationType( AAST_Normal )
	, m_addAdditiveType( AT_Local )
	, m_addConvertToAdditive( true )
	, m_gatherSyncTokens( false )
	, m_muteSoundEvents( false )
	, m_useLowerBodyPartsForLookAt( true )
#ifndef NO_EDITOR
	, m_recacheWeightCurve( false )
	, m_weightCurveChanged( false )
#endif
	, m_supportsMotionExClipFront( true )
{

}

/*
Cctor.

Compiler generated cctor would also copy instance vars - we don't want that.
*/
CStorySceneEventAnimation::CStorySceneEventAnimation( const CStorySceneEventAnimation& other )
	: CStorySceneEventAnimClip( other )
	, m_animationName( other.m_animationName )
	, m_useMotionExtraction( other.m_useMotionExtraction )
	, m_useFakeMotion( other.m_useFakeMotion )
	, m_disableLookAt( other.m_disableLookAt )
	, m_disableLookAtSpeed( other.m_disableLookAtSpeed )
	, m_useLowerBodyPartsForLookAt( other.m_useLowerBodyPartsForLookAt )
	, m_gatherSyncTokens( other.m_gatherSyncTokens )
	, m_muteSoundEvents( other.m_muteSoundEvents )
	#ifndef NO_EDITOR
	, m_bonesGroupName( other.m_bonesGroupName )
	, m_bones( other.m_bones )
	#endif // !NO_EDITOR
	, m_bonesIdx( other.m_bonesIdx )
	, m_bonesWeight( other.m_bonesWeight )
	#ifndef NO_EDITOR
	, m_status( other.m_status )
	, m_emotionalState( other.m_emotionalState )
	, m_poseName( other.m_poseName )
	, m_typeName( other.m_typeName )
	, m_friendlyName( other.m_friendlyName )
	#endif // !NO_EDITOR
	, m_animationType( other.m_animationType )
	, m_addConvertToAdditive( other.m_addConvertToAdditive )
	, m_addAdditiveType( other.m_addAdditiveType )
	, m_useWeightCurve( other.m_useWeightCurve )
	, m_weightCurve( other.m_weightCurve )
	, m_weightCurveChanged( other.m_weightCurveChanged )
	#ifndef NO_EDITOR
	, m_recacheWeightCurve( other.m_recacheWeightCurve )
	#endif // !NO_EDITOR
	, m_supportsMotionExClipFront( other.m_supportsMotionExClipFront )
{}

CStorySceneEventAnimation* CStorySceneEventAnimation::Clone() const
{
	return new CStorySceneEventAnimation( *this );
}

void CStorySceneEventAnimation::Interface_SetDragAndDropBodyAnimation( const TDynArray< CName >& animData, Float animDuration ) 
{
#ifndef NO_EDITOR
	SetAnimationState( animData ); 
	RefreshDuration( animDuration );
#endif
}

void CStorySceneEventAnimation::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_animation;
	compiler << i_hasMotion;
}

void CStorySceneEventAnimation::OnInit( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer ) const
{
	TBaseClass::OnInit( data, scenePlayer );

	CSkeletalAnimationSetEntry*& anim = data[ i_animation ];
	anim = FindAnimation( scenePlayer );

	Bool& hasMotion = data[ i_hasMotion ];
	hasMotion = false;
	if ( anim )
	{
		hasMotion = HasAnimationMotion( anim );
	}
}

void CStorySceneEventAnimation::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	if ( m_disableLookAt == true )
	{
		ASSERT( scenePlayer != NULL );

		CActor* actor = scenePlayer->GetMappedActor( m_actor );
		if( actor == NULL )
		{
			return;
		}

		actor->DisableDialogsLookAts( m_disableLookAtSpeed );
	}
}

void CStorySceneEventAnimation::OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnProcess( data, scenePlayer, collector, timeInfo );

	ApplyAnimationMotion( data, collector, timeInfo );
}

void CStorySceneEventAnimation::OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnEnd( data, scenePlayer, collector, timeInfo );

	ApplyAnimationMotion( data, collector, timeInfo );
}

CSkeletalAnimationSetEntry* CStorySceneEventAnimation::FindAnimation( const CStoryScenePlayer* scenePlayer ) const
{
	const CAnimatedComponent* ac = GetAnimatedComponentForActor( scenePlayer );
	if ( ac && ac->GetAnimationContainer() )
	{
		CSkeletalAnimationSetEntry* anim = ac->GetAnimationContainer()->FindAnimation( GetAnimationName() );
		if ( anim && anim->GetAnimation() )
		{
			return anim;
		}
	}

	return nullptr;
}

void CStorySceneEventAnimation::ApplyAnimationMotion( CStorySceneInstanceBuffer& data, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	if ( m_useMotionExtraction && data[ i_hasMotion ] && data[ i_animation ] )
	{
		StorySceneEventsCollector::ActorMotion evt( this, m_actor );

		evt.m_eventTimeLocal = timeInfo.m_timeLocal;
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;

		evt.m_eventTimeAbsStart = GetInstanceStartTime( data );
		evt.m_eventTimeAbsEnd = GetInstanceStartTime( data ) + GetInstanceDuration( data );
		evt.m_animation = data[ i_animation ];
		evt.m_weight = m_weight;
		evt.m_blendIn = m_blendIn;
		evt.m_blendOut = m_blendOut;
		evt.m_stretch = GetAnimationStretch() * GetInstanceScalingFactor( data );
		evt.m_clipFront = m_clipFront;
		evt.m_supportsClipFront = m_supportsMotionExClipFront;

		collector.AddEvent( evt );
	}
}

Bool CStorySceneEventAnimation::HasAnimationMotion( const CSkeletalAnimationSetEntry* animation ) const
{
	if ( animation->GetAnimation()->HasExtractedMotion() )
	{
		Matrix motion;
		animation->GetAnimation()->GetMovementAtTime( animation->GetDuration(), motion );

		const EulerAngles rot = motion.ToEulerAngles();
		const Vector& pos = motion.GetTranslationRef();

		if ( rot.AlmostEquals( EulerAngles::ZEROS ) && Vector::Near3( pos, Vector::ZERO_3D_POINT ) )
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	return false;
}

void CStorySceneEventAnimation::OnCurveChanged()
{
	m_weightCurveChanged = true;
}

void CStorySceneEventAnimation::CopyBlendsValsToWeightCurve( Bool force )
{
#ifndef NO_EDITOR
	if ( !m_weightCurveChanged || force )
	{
		const Float duration = GetDurationProperty(); // TODO: confirm this is ok - should be ok as we do this in "edit" mody only, when
													  // section is approved so GetDurationProperty() == instance duration
													  // HOWEVER - m_blendIn and m_blendOut values doesn't take into account stretch
													  // so probably we should use baseDuration here (duration without stretch and scaling)
													  // and not the one that's used because it contains stretch
		if ( duration > 0.f )
		{
			m_weightCurve.Clear();
			m_weightCurve.AddPoint( 0.f, 0.f );
			m_weightCurve.AddPoint( m_blendIn / duration, 1.f );
			m_weightCurve.AddPoint( ( duration - m_blendOut ) / duration, 1.f );
			m_weightCurve.AddPoint( 1.f, 0.f );
		}
	}
#endif
}

void CStorySceneEventAnimation::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	//CopyBlendsValsToWeightCurve();
}

#ifndef  NO_EDITOR

void CStorySceneEventAnimation::OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName )
{
	TBaseClass::OnPreviewPropertyChanged( previewPlayer, propertyName );

	m_eventName = !m_friendlyName.Empty() ? m_friendlyName : m_animationName.AsString();

	if ( propertyName == CNAME(friendlyName) )
	{
		m_animationName = GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().FindBodyAnimationByFriendlyName( m_status, m_emotionalState, m_poseName, m_typeName, m_friendlyName );
		OnPreviewPropertyChanged( previewPlayer, CNAME( animationName ) );
	}
	else if ( propertyName == TXT("recacheWeightCurve") && m_recacheWeightCurve )
	{
		CopyBlendsValsToWeightCurve( true );
	}

	if ( ( propertyName == CNAME(friendlyName) || propertyName == CNAME( animationName ) ) )
	{
		CopyBlendsValsToWeightCurve();
	}

	if ( propertyName == CNAME(weight) )
	{
		m_weight = Clamp( m_weight, 0.f, 1.f );
	}
}

void CStorySceneEventAnimation::OnBodyPartsChanged()
{
	OnBonesListChanged();
}

void CStorySceneEventAnimation::SuckDataFromActorState( const SStorySceneActorAnimationState& state )
{
	m_status = state.m_status;
	m_emotionalState = state.m_emotionalState;
	m_poseName = state.m_poseType;

	if ( m_typeName == CName::NONE )
	{
		const CStorySceneAnimationList& list = GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList();
		for ( CStorySceneAnimationList::TypeBodyIterator it( list, m_status, m_emotionalState, m_poseName ); it; ++it )
		{
			 if( CStorySceneAnimationList::GESTURE_KEYWORD == (*it) )
			 {
				 m_typeName = (*it);
				 return;
			 }			 
		}
	}
}

void CStorySceneEventAnimation::SetAnimationState( const TDynArray< CName >& state )
{
	ASSERT( state.Size() == 6 );

	m_status = state[ 0 ];
	m_emotionalState = state[ 1 ];
	m_poseName = state[ 2 ];
	m_typeName = state[ 3 ];
	m_friendlyName = state[ 4 ].AsString();
	m_animationName = state[ 5 ];

	m_eventName = m_friendlyName;

	CopyBlendsValsToWeightCurve();
}

void CStorySceneEventAnimation::CopyFrom( const CStorySceneEventAnimation* rhs )
{
	m_status = rhs->m_status;
	m_emotionalState = rhs->m_emotionalState;
	m_poseName = rhs->m_poseName;
	m_typeName = rhs->m_typeName;
	m_friendlyName = rhs->m_friendlyName;

	m_animationName = rhs->m_animationName;
	m_disableLookAt = rhs->m_disableLookAt;
	m_disableLookAtSpeed = rhs->m_disableLookAtSpeed;

	CopyBlendsValsToWeightCurve();
}

void CStorySceneEventAnimation::OnBonesListChanged()
{
	const Uint32 size = m_bones.Size();

	m_bonesIdx.Resize( size );
	m_bonesWeight.Resize( size );

	for ( Uint32 i=0; i<size; ++i )
	{
		const SBehaviorGraphBoneInfo& boneInfo = m_bones[ i ];

		m_bonesIdx[ i ] = boneInfo.m_num;
		m_bonesWeight[ i ] = boneInfo.m_weight;
	}
}

#endif

void CStorySceneEventAnimation::OnAddExtraDataToEvent( StorySceneEventsCollector::BodyAnimation& event ) const
{
	TBaseClass::OnAddExtraDataToEvent( event );

	event.m_muteSoundEvents = m_muteSoundEvents;
	event.m_useMotion = m_useMotionExtraction;
	event.m_useFakeMotion = m_useFakeMotion;
	event.m_bonesIdx = m_bonesIdx;
	event.m_bonesWeight = m_bonesWeight;
	event.m_gatherSyncTokens = m_gatherSyncTokens;
	event.m_useLowerBodyPartsForLookAt = m_useLowerBodyPartsForLookAt;

	if ( m_animationType == AAST_Override )
	{
		event.m_type = EAT_Override;
	}
	else if ( m_animationType == AAST_Additive )
	{
		event.m_type = EAT_Additive;
		event.m_additiveType = m_addAdditiveType;
		event.m_convertToAdditive = m_addConvertToAdditive;
	}
}

/*
Calculates blend weight for given event time.

See CStorySceneEventAnimClip::CalculateBlendWeight() for some important information
about behavior of blend in/out in case of stretched/scaled events.
*/
Float CStorySceneEventAnimation::CalculateBlendWeight( Float eventTime, Float eventDuration ) const
{
	if ( !m_useWeightCurve )
	{
		return TBaseClass::CalculateBlendWeight( eventTime, eventDuration );
	}
	else
	{
		return Clamp( m_weightCurve.GetFloatValue( eventTime ), 0.f, 1.f );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneEventAdditiveAnimation );

CStorySceneEventAdditiveAnimation::CStorySceneEventAdditiveAnimation() 
	: m_additiveType( AT_Local )
	, m_convertToAdditive( true )
{}

CStorySceneEventAdditiveAnimation::CStorySceneEventAdditiveAnimation( const String& eventName,
	CStorySceneElement* sceneElement, Float startTime, const CName& actor,
	const String& animName, const String& trackName )
	: CStorySceneEventAnimation( eventName, sceneElement, startTime, actor, animName, trackName )
	, m_additiveType( AT_Local )
	, m_convertToAdditive( true )
{

}

CStorySceneEventAdditiveAnimation* CStorySceneEventAdditiveAnimation::Clone() const
{
	return new CStorySceneEventAdditiveAnimation( *this );
}

void CStorySceneEventAdditiveAnimation::OnAddExtraDataToEvent( StorySceneEventsCollector::BodyAnimation& event ) const
{
	TBaseClass::OnAddExtraDataToEvent( event );

	event.m_type = EAT_Additive;
	event.m_additiveType = m_additiveType;
	event.m_convertToAdditive = m_convertToAdditive;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneEventOverrideAnimation );

CStorySceneEventOverrideAnimation::CStorySceneEventOverrideAnimation() 
{}

CStorySceneEventOverrideAnimation::CStorySceneEventOverrideAnimation( const String& eventName,
	CStorySceneElement* sceneElement, Float startTime, const CName& actor,
	const String& animName, const String& trackName )
	: CStorySceneEventAnimation( eventName, sceneElement, startTime, actor, animName, trackName )
{

}

CStorySceneEventOverrideAnimation* CStorySceneEventOverrideAnimation::Clone() const
{
	return new CStorySceneEventOverrideAnimation( *this );
}

void CStorySceneEventOverrideAnimation::OnAddExtraDataToEvent( StorySceneEventsCollector::BodyAnimation& event ) const
{
	TBaseClass::OnAddExtraDataToEvent( event );

	event.m_type = EAT_Override;
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
