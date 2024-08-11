
#include "build.h"
#include "storySceneEventPoseKey.h"
#include "storyScenePlayer.h"
#include "../../common/engine/behaviorGraphUtils.inl"
#include "storySceneEventAnimClip.h"
#include "storySceneEventsCollector_events.h"
#include "../engine/animMath.h"
#include "../engine/mimicComponent.h"
#include "storySceneEventChangePose.h"
#include "storySceneEventMimics.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneEventPoseKey );
IMPLEMENT_ENGINE_CLASS( SSSBoneTransform );
IMPLEMENT_ENGINE_CLASS( SSSTrackTransform );

Uint32 CStorySceneEventPoseKey::ID = 0;

CStorySceneEventPoseKey::CStorySceneEventPoseKey()
	: m_actor( CName::NONE )
	, m_weight( 1.f )
	, m_blendIn( 0.f )
	, m_blendOut( 0.f )
	, m_linkToDialogset( false )
	, m_useWeightCurve( false )
	, m_weightBlendType( IT_Bezier )
#ifndef NO_EDITOR
	,m_version( 0 )
	,m_presetVersion( 0 )
#endif

{
	m_id = ++ID;
}

CStorySceneEventPoseKey::CStorySceneEventPoseKey( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName )
	: CStorySceneEventDuration( eventName, sceneElement, startTime, 0.f, trackName )
	, m_actor( actor )
	, m_weight( 1.f )
	, m_blendIn( 0.f )
	, m_blendOut( 0.f )
	, m_linkToDialogset( false )
	, m_useWeightCurve( false )
	, m_weightBlendType( IT_Bezier )
#ifndef NO_EDITOR
	,m_version( 0 )
	,m_presetVersion( 0 )
#endif
{
	m_id = ++ID;
}

/*
Cctor.
*/
CStorySceneEventPoseKey::CStorySceneEventPoseKey( const CStorySceneEventPoseKey& other )
: CStorySceneEventDuration( other )
, m_id( ++ID )
, m_actor( other.m_actor )
, m_weightBlendType( other.m_weightBlendType )
, m_weight( other.m_weight )
, m_blendIn( other.m_blendIn )
, m_blendOut( other.m_blendOut )
, m_linkToDialogset( other.m_linkToDialogset )
, m_useWeightCurve( other.m_useWeightCurve )
, m_weightCurve( other.m_weightCurve )
#ifndef NO_EDITOR
, m_presetName( other.m_presetName )
, m_presetVersion( other.m_presetVersion )
, m_bones( other.m_bones )
, m_cachedBonesIK( other.m_cachedBonesIK )
, m_cachedTransformsIK( other.m_cachedTransformsIK )
, m_bonesHands( other.m_bonesHands )
, m_tracks( other.m_tracks )
#endif // !NO_EDITOR
, m_version( other.m_version )
, m_cachedBones( other.m_cachedBones )
, m_cachedTransforms( other.m_cachedTransforms )
, m_cachedTracks( other.m_cachedTracks )
, m_cachedTracksValues( other.m_cachedTracksValues )
#ifndef NO_EDITOR
, m_editorCachedHandTracks( other.m_editorCachedHandTracks )
, m_editorCachedIkEffectorsID( other.m_editorCachedIkEffectorsID )
, m_editorCachedIkEffectorsPos( other.m_editorCachedIkEffectorsPos )
, m_editorCachedIkEffectorsWeight( other.m_editorCachedIkEffectorsWeight )
, m_editorCachedMimicSliders( other.m_editorCachedMimicSliders )
#endif // !NO_EDITOR
{}

CStorySceneEventPoseKey* CStorySceneEventPoseKey::Clone() const
{
	return new CStorySceneEventPoseKey( *this );
}

Float CStorySceneEventPoseKey::CalcWeight( const CStorySceneInstanceBuffer& instanceBuffer, const SStorySceneEventTimeInfo& timeInfo ) const
{
	if ( !m_useWeightCurve )
	{
		Float blendWeight = 1.f;

		const Float duration = GetInstanceDuration( instanceBuffer );
		if ( duration > 0.f )
		{
			if ( timeInfo.m_timeLocal <= m_blendIn )
			{
				blendWeight = timeInfo.m_timeLocal / m_blendIn;
			}
			else if ( timeInfo.m_timeLocal >= duration - m_blendOut )
			{
				blendWeight = ( duration - timeInfo.m_timeLocal ) / m_blendOut;
			}
		}

		const Float finalWeight = Clamp( blendWeight * m_weight, 0.0f, 1.0f );;

		return BehaviorUtils::Interpolate( m_weightBlendType, finalWeight );
	}
	else
	{
		const Float blendWeight = m_weightCurve.GetFloatValue( timeInfo.m_progress );
		return Clamp( blendWeight * m_weight, 0.0f, 1.0f );
	}
}

void CStorySceneEventPoseKey::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	if ( m_actor )
	{
		const CStorySceneDialogsetInstance* dialogset = m_linkToDialogset ? scenePlayer->GetDialogsetForEvent( this ) : nullptr;

		{
			StorySceneEventsCollector::BodyPose evt( this, m_actor );
			evt.m_weight = CalcWeight( data, timeInfo );
			evt.m_poseId = m_id;
			evt.m_boneIndices = m_cachedBones;
			evt.m_boneTransforms = m_cachedTransforms;
			evt.m_enable = true;

			if ( m_linkToDialogset )
			{
				evt.m_linkToDialogset = true;
				evt.m_linkToDialogsetPtr = dialogset;
			}
			else if ( HasLinkParent() )
			{
				if ( const CStorySceneEvent* linkParent = scenePlayer->FindEventByGUID( GetLinkParentGUID() ) )
				{
					if ( linkParent->GetClass()->IsA< CStorySceneEventChangePose >() )
					{
						const CStorySceneEventChangePose* chpEvt = static_cast< const CStorySceneEventChangePose* >( linkParent );

						SCENE_ASSERT( ARRAY_COUNT( evt.m_linkToChangePoseState ) == 4 );

						evt.m_linkToChangePose = true;
						evt.m_linkToChangePoseState[ 0 ] = chpEvt->GetBodyFilterStatus();
						evt.m_linkToChangePoseState[ 1 ] = chpEvt->GetBodyFilterEmotionalState();
						evt.m_linkToChangePoseState[ 2 ] = chpEvt->GetBodyFilterPoseName();
						evt.m_linkToChangePoseState[ 3 ] = chpEvt->GetForceBodyIdleAnimation();

					}
					else if ( linkParent->GetClass()->IsA< CStorySceneEventAnimClip >() )
					{
						evt.m_correctionID = GetLinkParentGUID();
					}
				}
			}

			collector.AddEvent( evt );
		}

		{
			StorySceneEventsCollector::MimicPose evt( this, m_actor );
			evt.m_weight = CalcWeight( data, timeInfo );
			evt.m_poseId = m_id;
			evt.m_trackIndices = m_cachedTracks;
			evt.m_trackValues = m_cachedTracksValues;
			evt.m_enable = true;

			if ( m_linkToDialogset )
			{
				evt.m_linkToDialogset = true;
				evt.m_linkToDialogsetPtr = dialogset;
			}
			else if ( HasLinkParent() )
			{
				if ( const CStorySceneEvent* linkParent = scenePlayer->FindEventByGUID( GetLinkParentGUID() ) )
				{
					if ( linkParent->GetClass()->IsA< CStorySceneEventMimics >() )
					{
						const CStorySceneEventMimics* chpEvt = static_cast< const CStorySceneEventMimics* >( linkParent );

						SCENE_ASSERT( ARRAY_COUNT( evt.m_linkToChangePoseState ) == 4 );

						evt.m_linkToChangePose = true;
						evt.m_linkToChangePoseState[ 0 ] = chpEvt->GetMimicFilterEyesLayer();
						evt.m_linkToChangePoseState[ 1 ] = chpEvt->GetMimicFilterPoseLayer();
						evt.m_linkToChangePoseState[ 2 ] = chpEvt->GetMimicFilterAnimationLayer();
						evt.m_linkToChangePoseState[ 3 ] = CName::NONE;
					}
					else if ( linkParent->GetClass()->IsA< CStorySceneEventAnimClip >() )
					{
						evt.m_correctionID = GetLinkParentGUID();
					}
				}
			}

			collector.AddEvent( evt );
		}
	}
}

void CStorySceneEventPoseKey::OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnProcess( data, scenePlayer, collector, timeInfo );

	if ( m_actor )
	{
		const CStorySceneDialogsetInstance* dialogset = m_linkToDialogset ? scenePlayer->GetDialogsetForEvent( this ) : nullptr;

		{
			StorySceneEventsCollector::BodyPose evt( this, m_actor );
			evt.m_weight = CalcWeight( data, timeInfo );
			evt.m_poseId = m_id;
			evt.m_boneIndices = m_cachedBones;
			evt.m_boneTransforms = m_cachedTransforms;
			evt.m_enable = true;

			if ( m_linkToDialogset )
			{
				evt.m_linkToDialogset = true;
				evt.m_linkToDialogsetPtr = dialogset;
			}
			else if ( HasLinkParent() )
			{
				if ( const CStorySceneEvent* linkParent = scenePlayer->FindEventByGUID( GetLinkParentGUID() ) )
				{
					if ( linkParent->GetClass()->IsA< CStorySceneEventChangePose >() )
					{
						const CStorySceneEventChangePose* chpEvt = static_cast< const CStorySceneEventChangePose* >( linkParent );
						
						SCENE_ASSERT( ARRAY_COUNT( evt.m_linkToChangePoseState ) == 4 );

						evt.m_linkToChangePose = true;
						evt.m_linkToChangePoseState[ 0 ] = chpEvt->GetBodyFilterStatus();
						evt.m_linkToChangePoseState[ 1 ] = chpEvt->GetBodyFilterEmotionalState();
						evt.m_linkToChangePoseState[ 2 ] = chpEvt->GetBodyFilterPoseName();
						evt.m_linkToChangePoseState[ 3 ] = chpEvt->GetForceBodyIdleAnimation();
					}
					else if ( linkParent->GetClass()->IsA< CStorySceneEventAnimClip >() )
					{
						evt.m_correctionID = GetLinkParentGUID();
					}
				}
			}

			collector.AddEvent( evt );
		}

		{
			StorySceneEventsCollector::MimicPose evt( this, m_actor );
			evt.m_weight = CalcWeight( data, timeInfo );
			evt.m_poseId = m_id;
			evt.m_trackIndices = m_cachedTracks;
			evt.m_trackValues = m_cachedTracksValues;
			evt.m_enable = true;

			if ( m_linkToDialogset )
			{
				evt.m_linkToDialogset = true;
				evt.m_linkToDialogsetPtr = dialogset;
			}
			else if ( HasLinkParent() )
			{
				if ( const CStorySceneEvent* linkParent = scenePlayer->FindEventByGUID( GetLinkParentGUID() ) )
				{
					if ( linkParent->GetClass()->IsA< CStorySceneEventMimics >() )
					{
						const CStorySceneEventMimics* chpEvt = static_cast< const CStorySceneEventMimics* >( linkParent );

						SCENE_ASSERT( ARRAY_COUNT( evt.m_linkToChangePoseState ) == 4 );

						evt.m_linkToChangePose = true;
						evt.m_linkToChangePoseState[ 0 ] = chpEvt->GetMimicFilterEyesLayer();
						evt.m_linkToChangePoseState[ 1 ] = chpEvt->GetMimicFilterPoseLayer();
						evt.m_linkToChangePoseState[ 2 ] = chpEvt->GetMimicFilterAnimationLayer();
						evt.m_linkToChangePoseState[ 3 ] = CName::NONE;
					}
					else if ( linkParent->GetClass()->IsA< CStorySceneEventAnimClip >() )
					{
						evt.m_correctionID = GetLinkParentGUID();
					}
				}
			}

			collector.AddEvent( evt );
		}
	}
}

void CStorySceneEventPoseKey::OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnEnd( data, scenePlayer, collector, timeInfo );

	if ( m_actor )
	{
		const CStorySceneDialogsetInstance* dialogset = m_linkToDialogset ? scenePlayer->GetDialogsetForEvent( this ) : nullptr;

		{
			Bool removeEvt = GetDurationProperty() > 0.f;

			StorySceneEventsCollector::BodyPose evt( this, m_actor );

			if ( m_linkToDialogset )
			{
				removeEvt = false;
			}
			else if ( HasLinkParent() )
			{
				if ( const CStorySceneEvent* linkParent = scenePlayer->FindEventByGUID( GetLinkParentGUID() ) )
				{
					if ( linkParent->GetClass()->IsA< CStorySceneEventChangePose >() )
					{
						removeEvt = false;

					}
					else if ( linkParent->GetClass()->IsA< CStorySceneEventAnimClip >() )
					{
						removeEvt = false;
					}
				}
			}

			if ( removeEvt )
			{
				collector.RemoveEvent( evt );
			}
		}

		{
			Bool removeEvt = GetDurationProperty() > 0.f;

			StorySceneEventsCollector::MimicPose evt( this, m_actor );

			if ( m_linkToDialogset )
			{
				removeEvt = false;
			}
			else if ( HasLinkParent() )
			{
				if ( const CStorySceneEvent* linkParent = scenePlayer->FindEventByGUID( GetLinkParentGUID() ) )
				{
					if ( linkParent->GetClass()->IsA< CStorySceneEventMimics >() )
					{
						removeEvt = false;

					}
					else if ( linkParent->GetClass()->IsA< CStorySceneEventAnimClip >() )
					{
						removeEvt = false;
					}
				}
			}

			if ( removeEvt )
			{
				collector.RemoveEvent( evt );
			}
		}
	}
}

#ifndef NO_EDITOR

Bool CStorySceneEventPoseKey::IsLinkedToDialogset() const
{
	return m_linkToDialogset;
}

Bool CStorySceneEventPoseKey::IsMimic() const
{
	// TODO do not use ==0, we need to check if any transform is not identity
	return m_cachedTracks.Size() > 0;
}

Bool CStorySceneEventPoseKey::IsBody() const
{
	// TODO do not use ==0, we need to check if any transform is not identity
	return m_cachedBones.Size() > 0;
}

void CStorySceneEventPoseKey::OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName )
{
	TBaseClass::OnPreviewPropertyChanged( previewPlayer, propertyName );

	if ( propertyName == TXT("bones") )
	{
		CacheBones( previewPlayer );
	}
	else if ( propertyName == TXT("duration") )
	{
		FixBlends( *previewPlayer );
	}
	else if ( propertyName == TXT("useWeightCurve") && m_useWeightCurve )
	{
		InitializeWeightCurve( *previewPlayer );
	}
}

void CStorySceneEventPoseKey::CacheBones_AddTransformLS( Int32 boneIdx, const RedQsTransform& transLS )
{
	SCENE_ASSERT( m_cachedBones.Size() == m_cachedTransforms.Size() );

	Bool found = false;

	const Int32 num = m_cachedBones.SizeInt();
	for ( Int32 i=0; i<num; ++i )
	{
		const Int32 idx = m_cachedBones[ i ];
		if ( idx == boneIdx )
		{
			EngineQsTransform& currEngineTrans = m_cachedTransforms[ i ];
			RedQsTransform& currTrans = ENGINE_QS_TRANSFORM_TO_ANIM_QS_TRANSFORM_REF( currEngineTrans );

			currTrans.SetMul( currTrans, transLS );

			found = true;
			break;
		}
	}

	if ( !found )
	{
		m_cachedBones.PushBack( boneIdx );
		m_cachedTransforms.PushBack( ANIM_QS_TRANSFORM_TO_CONST_ENGINE_QS_TRANSFORM_REF( transLS ) );
	}
}

void CStorySceneEventPoseKey::CacheBones_IK( const CAnimatedComponent* ac )
{
	SCENE_ASSERT( m_cachedBonesIK.Size() == m_cachedTransformsIK.Size() );

	const Uint32 num = m_cachedBonesIK.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		const Int32 boneIdex = m_cachedBonesIK[ i ];
		const EngineQsTransform& boneTrans = m_cachedTransformsIK[ i ];

		CacheBones_AddTransformLS( boneIdex, ENGINE_QS_TRANSFORM_TO_CONST_ANIM_QS_TRANSFORM_REF( boneTrans ) );
	}
}

void CStorySceneEventPoseKey::CacheBones_Hands( const CAnimatedComponent* ac )
{
	const Uint32 num = m_bonesHands.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		const SSSBoneTransform& boneTrans = m_bonesHands[ i ];
		if ( boneTrans.m_bone && !boneTrans.m_transform.IsIdentity() )
		{
			const Int32 boneIdex = ac->FindBoneByName( boneTrans.m_bone );
			if ( boneIdex != -1 )
			{
				Matrix mat;
				boneTrans.m_transform.CalcLocalToWorld( mat );
				AnimQsTransform trans = MatrixToAnimQsTransform( mat );

				CacheBones_AddTransformLS( boneIdex, trans );
			}
		}
	}
}

void CStorySceneEventPoseKey::CacheBones_FK( const CAnimatedComponent* ac )
{
	const Uint32 num = m_bones.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		const SSSBoneTransform& boneTrans = m_bones[ i ];
		if ( boneTrans.m_bone && !boneTrans.m_transform.IsIdentity() )
		{
			const Int32 boneIdex = ac->FindBoneByName( boneTrans.m_bone );
			if ( boneIdex != -1 )
			{
				Matrix mat;
				boneTrans.m_transform.CalcLocalToWorld( mat );
				AnimQsTransform trans = MatrixToAnimQsTransform( mat );

				CacheBones_AddTransformLS( boneIdex, trans );
			}
		}
	}
}

void CStorySceneEventPoseKey::CacheBones( const CEntity* e )
{
	m_cachedBones.ClearFast();
	m_cachedTransforms.ClearFast();
	m_version = 1;

	if ( const CAnimatedComponent* ac = e->GetRootAnimatedComponent() )
	{
		CacheBones_FK( ac );
		CacheBones_Hands( ac );
		CacheBones_IK( ac );
	}
}

void CStorySceneEventPoseKey::CacheBones( const CStoryScenePlayer* previewPlayer )
{
	if ( m_actor )
	{
		if ( const CEntity* e = previewPlayer->GetSceneActorEntity( m_actor ) )
		{
			CacheBones( e );
		}
	}
}

void CStorySceneEventPoseKey::SetBoneTransformLS_FK( const CName& boneName, const EngineTransform& transform )
{
	const Uint32 num = m_bones.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		SSSBoneTransform& trans = m_bones[ i ];
		if ( trans.m_bone == boneName )
		{
			trans.m_transform = transform;
			return;
		}
	}

	SSSBoneTransform newTrans;
	newTrans.m_bone = boneName;
	newTrans.m_transform = transform;
	m_bones.PushBack( newTrans );
}

void CStorySceneEventPoseKey::ResetBone_FK( const CName& boneName )
{
	const Uint32 num = m_bones.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		if ( m_bones[ i ].m_bone == boneName )
		{			
			m_bones.RemoveAt( i );
			break;
		}
	}
}

void CStorySceneEventPoseKey::SetBoneTransformLS_Hands( const CName& boneName, const EngineTransform& transform )
{
	const Uint32 num = m_bonesHands.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		SSSBoneTransform& trans = m_bonesHands[ i ];
		if ( trans.m_bone == boneName )
		{
			trans.m_transform = transform;
			return;
		}
	}

	SSSBoneTransform newTrans;
	newTrans.m_bone = boneName;
	newTrans.m_transform = transform;
	m_bonesHands.PushBack( newTrans );
}

void CStorySceneEventPoseKey::ResetBone_Hands( const CName& boneName )
{
	const Uint32 num = m_bonesHands.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		if ( m_bonesHands[ i ].m_bone == boneName )
		{			
			m_bonesHands.RemoveAt( i );
			break;
		}
	}
}

void CStorySceneEventPoseKey::SetBonesTransformLS_IK( const TDynArray< Int32 >& bones, const TEngineQsTransformArray& transLS )
{
	SCENE_ASSERT( bones.Size() == transLS.Size() );

	m_cachedBonesIK = bones;
	m_cachedTransformsIK = transLS;
}

void CStorySceneEventPoseKey::SetBlends( Float blendIn, Float blendOut )
{
	m_blendIn = blendIn;
	m_blendOut = blendOut;
}

void CStorySceneEventPoseKey::GetBlends( Float& blendIn, Float& blendOut )
{
	blendIn = m_blendIn;
	blendOut = m_blendOut;
}

void CStorySceneEventPoseKey::FixBlends( const CStoryScenePlayer& scenePlayer )
{
	const Float duration = scenePlayer.GetEventDuration( *this );
	if ( duration == 0.f )
	{
		m_blendIn = 0.f;
		m_blendOut = 0.f;
	}
	else
	{
		const Float all = m_blendIn + m_blendOut;
		if ( all > duration )
		{
			FitBlends( all, duration );
		}
	}
}

void CStorySceneEventPoseKey::FitBlends( Float prevDuration, Float newDuration )
{
	if ( prevDuration == 0.f )
	{
		return;
	}

	if ( newDuration == 0.f )
	{
		m_blendIn = 0.f;
		m_blendOut = 0.f;
	}

	const Float pIn = m_blendIn / prevDuration;
	const Float pOut = m_blendOut / prevDuration;

	m_blendIn = pIn * newDuration;
	m_blendOut = pOut * newDuration;
}

void CStorySceneEventPoseKey::InitializeWeightCurve( const CStoryScenePlayer& scenePlayer )
{
	const Float duration = scenePlayer.GetEventDuration( *this );
	if ( duration > 0.f )
	{
		m_weightCurve.Clear();

		const Float pA = m_blendIn / duration;
		const Float pB = m_blendOut / duration;

		if ( pA > 0.f )
		{
			m_weightCurve.AddPoint( 0.f, 0.f, CST_BezierSmoothSymertric );
		}
		else
		{
			m_weightCurve.AddPoint( 0.f, 1.f, CST_BezierSmoothSymertric );
		}

		if ( pB > 0.f )
		{
			m_weightCurve.AddPoint( 1.f, 0.f, CST_BezierSmoothSymertric );
		}
		else
		{
			m_weightCurve.AddPoint( 1.f, 1.f, CST_BezierSmoothSymertric );
		}

		if ( pA + pB > 0.99f )
		{
			m_weightCurve.AddPoint( 1.f-pB, 1.f, CST_BezierSmoothSymertric );
		}
		else
		{
			if ( pB > 0.f )
			{
				m_weightCurve.AddPoint( 1.f-pB, 1.f, CST_BezierSmoothSymertric );
			}

			if ( pA > 0.f )
			{
				m_weightCurve.AddPoint( pA, 1.f, CST_BezierSmoothSymertric );
			}
		}
	}
}

#endif

Bool CStorySceneEventPoseKey::UsesWeightCurve() const 
{ 
	return m_useWeightCurve; 
}


void CStorySceneEventPoseKey::OnPostLoad()
{
	if ( m_version == 0 )
	{
		Bool linkedToAnim = Cast<CStorySceneEventAnimClip>( m_sceneElement->GetSection()->GetEvent( GetLinkParentGUID() ) ) != nullptr ;

		if ( !m_linkToDialogset && !linkedToAnim )
		{
			for( EngineQsTransform& transform : m_cachedTransforms )
			{		
				AnimQsTransform& animTransf = reinterpret_cast<AnimQsTransform&>( transform );
				AnimQsTransform temp = animTransf;
				animTransf.SetMul( temp, temp );
			}

#ifndef NO_EDITOR

			for( SSSBoneTransform& bone : m_bonesHands )
			{
				EulerAngles newRot = bone.m_transform.GetRotation() * 2.f;
				newRot.Normalize();
				bone.m_transform.SetRotation( newRot );				
			}

			for( SSSBoneTransform& bone : m_bones )
			{
				EulerAngles newRot = bone.m_transform.GetRotation() * 2.f;
				newRot.Normalize();
				bone.m_transform.SetRotation( newRot );	
			}

			for ( Float& track : m_editorCachedHandTracks )
			{
				track *= 2.f;
			}			
#endif
		}	
		m_version = 1;
	}
}

#ifndef NO_EDITOR

void CStorySceneEventPoseKey::GetFkBonesData( const Uint32 boneNum, CName& outBoneName, EngineTransform& outTransform ) const
{
	outBoneName = m_bones[boneNum].m_bone;
	outTransform = m_bones[boneNum].m_transform;
}

void CStorySceneEventPoseKey::CacheHandTrackVals( Int32 index, Float val )
{
	Int32 oldSize = m_editorCachedHandTracks.SizeInt();
	if( index >= oldSize )
	{			
		m_editorCachedHandTracks.Resize( index + 1 );
		for ( Int32 i = oldSize; i < m_editorCachedHandTracks.SizeInt() ; i++)
		{
			m_editorCachedHandTracks[i] = 0.f;
		}
	}
	m_editorCachedHandTracks[index] = val;
}

Bool CStorySceneEventPoseKey::LoadHandTrackVals( Int32 index, Float& val ) const
{
	val = 0.f;
	if ( index > 0 && index < m_editorCachedHandTracks.SizeInt() )
	{
		val = m_editorCachedHandTracks[ index ];
		return true;
	}

	return false;
}

void CStorySceneEventPoseKey::CacheIkEffector( Int32 id, Vector val, Float w )
{
	const Bool isZero = Vector::Near3( val, Vector::ZERO_3D_POINT );
	Bool set = false;

	const Int32 size = m_editorCachedIkEffectorsID.SizeInt();
	for ( Int32 i=0; i<size; ++i )
	{
		if ( m_editorCachedIkEffectorsID[ i ] == id )
		{
			RED_FATAL_ASSERT( m_editorCachedIkEffectorsPos.SizeInt() > i, "Dialog editor error" );
			RED_FATAL_ASSERT( m_editorCachedIkEffectorsWeight.SizeInt() > i, "Dialog editor error" );

			if ( isZero )
			{
				m_editorCachedIkEffectorsID.RemoveAt( i );
				m_editorCachedIkEffectorsPos.RemoveAt( i );
				m_editorCachedIkEffectorsWeight.RemoveAt( i );
			}
			else
			{
				m_editorCachedIkEffectorsPos[ i ] = val;
				m_editorCachedIkEffectorsWeight[ i ] = w;
			}

			set = true;

			break;
		}
	}

	if ( !set && !isZero )
	{
		m_editorCachedIkEffectorsID.PushBack( id );
		m_editorCachedIkEffectorsPos.PushBack( val );
		m_editorCachedIkEffectorsWeight.PushBack( w );
	}
}

Bool CStorySceneEventPoseKey::LoadIkEffector( Int32 id, Vector& val, Float& w ) const
{
	const Int32 size = m_editorCachedIkEffectorsID.SizeInt();
	for ( Int32 i=0; i<size; ++i )
	{
		if ( m_editorCachedIkEffectorsID[ i ] == id )
		{
			RED_FATAL_ASSERT( m_editorCachedIkEffectorsPos.SizeInt() > i, "Dialog editor error" );
			RED_FATAL_ASSERT( m_editorCachedIkEffectorsWeight.SizeInt() > i, "Dialog editor error" );

			val = m_editorCachedIkEffectorsPos[ i ];
			w = m_editorCachedIkEffectorsWeight[ i ];

			return true;
		}
	}

	return false;
}

void CStorySceneEventPoseKey::SetTrackValuesRaw( const CStoryScenePlayer* previewPlayer, const TDynArray< Float >& buffer )
{
	m_tracks.ClearFast();

	if ( const CActor* a = Cast< CActor >( previewPlayer->GetSceneActorEntity( m_actor ) ) )
	{
		if ( const CMimicComponent* m = a->GetMimicComponent() )
		{
			if ( const CSkeleton* skeleton = m->GetMimicSkeleton() )
			{
				const Uint32 bufSize = buffer.Size();
				for ( Uint32 i=0; i<bufSize; ++i )
				{
					const Float val = buffer[ i ];
					if ( val != 0.f )
					{
						SSSTrackTransform newTrans;
						newTrans.m_track = skeleton->GetTrackNameAsCName( i );
						newTrans.m_value = val;
						m_tracks.PushBack( newTrans );
					}
				}
			}
		}
	}

	CacheTracks( previewPlayer );
}

void CStorySceneEventPoseKey::CacheTracks( const CEntity* e )
{
	m_cachedTracks.ClearFast();
	m_cachedTracksValues.ClearFast();

	if ( const CActor* a = Cast< const CActor >( e ) )
	{
		if ( const CMimicComponent* m = a->GetMimicComponent() )
		{
			if ( const CSkeleton* skeleton = m->GetMimicSkeleton() )
			{
				const Uint32 num = m_tracks.Size();

				m_cachedTracks.Reserve( num );
				m_cachedTracksValues.Reserve( num );

				for ( Uint32 i=0; i<num; ++i )
				{
					const SSSTrackTransform& trackTrans = m_tracks[ i ];
					if ( trackTrans.m_track && trackTrans.m_value != 0.f )
					{
						const Int32 trackIdex = skeleton->FindTrackByName( trackTrans.m_track );
						if ( trackIdex != -1 )
						{
							m_cachedTracks.PushBack( trackIdex );
							m_cachedTracksValues.PushBack( trackTrans.m_value );
						}
					}
				}
			}
		}
	}
}

void CStorySceneEventPoseKey::CacheTracks( const CStoryScenePlayer* previewPlayer )
{
	m_cachedTracks.ClearFast();
	m_cachedTracksValues.ClearFast();

	if ( m_actor )
	{
		if ( const CEntity* e = previewPlayer->GetSceneActorEntity( m_actor ) )
		{
			CacheTracks( e );
		}
	}
}

#endif

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
