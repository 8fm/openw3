
#include "build.h"
#include "storySceneAnimationList.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/core/feedback.h"
#include "../engine/skeletalAnimationContainer.h"

CGatheredResource resSceneAnimations_Body( TXT("gameplay\\globals\\scene_body_animations.csv"), RGF_Startup );
CGatheredResource resSceneAnimations_Mimics( TXT("gameplay\\globals\\scene_mimics_animations.csv"), RGF_Startup );
CGatheredResource resSceneAnimations_Mimics_EmoStates( TXT("gameplay\\globals\\scene_mimics_emotional_states.csv"), RGF_Startup );

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

RED_DEFINE_STATIC_NAME( High )
RED_DEFINE_STATIC_NAME( Determined )
RED_DEFINE_STATIC_NAME( Standing )
RED_DEFINE_STATIC_NAME( Eyes )

const CName&	CStorySceneAnimationList::ANIMATION_TYPE_IDLE = CNAME( Idle );
const CName&	CStorySceneAnimationList::ANIMATION_TYPE_TRANSITION = CNAME( Transition );
const CName&	CStorySceneAnimationList::GESTURE_KEYWORD = CNAME( Gesture );
const CName&	CStorySceneAnimationList::ADDITIVE_KEYWORD = CNAME( Add );
const CName&	CStorySceneAnimationList::IDLE_KEYWORD = CNAME( Idle );
const CName&	CStorySceneAnimationList::EXIT_KEYWORD = CNAME( Exit );
const CName&	CStorySceneAnimationList::ENTER_KEYWORD = CNAME( Enter );

const CName&	CStorySceneAnimationList::DEFAULT_STATUS = CNAME( High );
const CName&	CStorySceneAnimationList::DEFAULT_EMO_STATE = CNAME( Determined );
const CName&	CStorySceneAnimationList::DEFAULT_POSE = CNAME( Standing );
const CName&	CStorySceneAnimationList::DEFAULT_MIMICS_EMO_STATE = CNAME( Neutral );
const CName&	CStorySceneAnimationList::DEFAULT_MIMICS_LAYER_EYES = CNAME( Neutral );
const CName&	CStorySceneAnimationList::DEFAULT_MIMICS_LAYER_POSE = CNAME( Neutral );
const CName&	CStorySceneAnimationList::DEFAULT_MIMICS_LAYER_ANIMATION = CNAME( Neutral );

const CName&	CStorySceneAnimationList::LAYER_EYES = CNAME( Eyes );
const CName&	CStorySceneAnimationList::LAYER_POSE = CNAME( Pose );
const CName&	CStorySceneAnimationList::LAYER_ANIMATION = CNAME( Animation );

CSceneAnimationsResourcesManager::CSceneAnimationsResourcesManager()
	: m_resSceneAnimationsBody( resSceneAnimations_Body ),
	  m_resSceneAnimationsMimics( resSceneAnimations_Mimics ),
	  m_resSceneAnimationsMimicsEmoStates( resSceneAnimations_Mimics_EmoStates )
{
}

CStorySceneAnimationList::~CStorySceneAnimationList()
{
	ASSERT( m_dataBody.Size() == 0 );
	ASSERT( m_dataMimics.Size() == 0 );
	ASSERT( m_dataMimicsIdleMapping.Size() == 0 );
}

void CStorySceneAnimationList::Destroy()
{
	if ( m_dataBody.Size() > 0 )
	{
		// we don't support not-const iterators so
		for ( AllBodyAnimationsIterator it( *this ); it; ++it )
		{
			AnimationBodyData data = *it;
			if ( data.m_transitionTo )
			{
				delete data.m_transitionTo;
			}
		}

		m_dataBody.Clear();
	}

	m_dataMimics.Clear();
	m_dataMimicsIdleMapping.Clear();
}

Int32 CStorySceneAnimationList::FindTypeLevel( const TBodyAnimations& data, Int32 s, Int32 e, Int32 p, const TAnimationType& type ) const
{
	const TBodyTypeLevel& level = data[ s ].m_second[ e ].m_second[ p ].m_second;
	
	const Int32 size = level.SizeInt();
	for ( Int32 i=0; i<size; ++i )
	{
		if ( level[ i ].m_first == type )
		{
			return i;
		}
	}

	return -1;
}

Int32 CStorySceneAnimationList::FindOrAddTypeLevel( TBodyAnimations& data, Int32 s, Int32 e, Int32 p, const TAnimationType& type ) const
{
	Int32 ret = FindTypeLevel( data, s, e, p, type );
	if ( ret == -1 )
	{
		ret = (Int32)data[ s ].m_second[ e ].m_second[ p ].m_second.Grow( 1 );
		data[ s ].m_second[ e ].m_second[ p ].m_second[ ret ].m_first = type;
	}

	ASSERT( ret != -1 );

	return ret;
}

Bool CStorySceneAnimationList::IsValid() const
{
	return m_dataBody.Size() > 0 && m_dataMimics.Size() > 0;
}

Bool CStorySceneAnimationList::FindStateByAnimation( const CName& animationName, SStorySceneActorAnimationState& out, CName* outAnimType ) const
{
	for ( AllBodyAnimationsIterator it( *this ); it; ++it )
	{
		const AnimationBodyData& data = *it;
		if ( data.m_animationName == animationName )
		{
			out.m_status = it.GetStatus();
			out.m_emotionalState = it.GetEmoState();
			out.m_poseType = it.GetPose();
			if ( outAnimType )
			{
				*outAnimType = it.GetTypeName();
			}

			return true;
		}
	}

	return false;
}

Bool CStorySceneAnimationList::RandBodyIdleAnimation( const SStorySceneActorAnimationState& in, CStorySceneAnimationList::IdleAndLookAtAnimationData& dataOut, const CAnimatedComponent* filter ) const
{
	CSkeletalAnimationContainer* c = filter ? filter->GetAnimationContainer() : nullptr;

	for ( BodyAnimationIterator it( *this, in.m_status, in.m_emotionalState, in.m_poseType, ANIMATION_TYPE_IDLE ); it; ++it )
	{
		if ( ( c && c->HasAnimation( (*it).m_animationName ) ) || !c )
		{
			dataOut.m_idle = (*it).m_animationName;
			dataOut.m_lookAtBody = (*it).m_lookAtBodyAnimationName;
			dataOut.m_lookAtHead = (*it).m_lookAtHeadAnimationName;
			return true;
		}
	}

	return false;
}

Bool CStorySceneAnimationList::RandMimicsIdleAnimation( const SStorySceneActorAnimationState& in, CName& animationNameOut, const CAnimatedComponent* filter, const CName& layerName ) const
{
	//CSkeletalAnimationContainer* c = filter ? filter->GetAnimationContainer() : nullptr;

	CName animationFriendlyName;

	const CStorySceneAnimationList::MimicsEmoStatePreset* preset = nullptr;
	if ( layerName == LAYER_EYES )
	{
		preset = FindMimicsAnimationByEmoState( in.m_mimicsLayerEyes );
		if ( preset )
		{
			animationFriendlyName = preset->m_layerEyes;
		}
	}
	else if ( layerName == LAYER_POSE )
	{
		preset = FindMimicsAnimationByEmoState( in.m_mimicsLayerPose );
		if ( preset )
		{
			animationFriendlyName = preset->m_layerPose;
		}
	}
	else if ( layerName == LAYER_ANIMATION )
	{
		preset = FindMimicsAnimationByEmoState( in.m_mimicsLayerAnimation );
		if ( preset )
		{
			animationFriendlyName = preset->m_layerAnimation;
		}
	}

	if ( preset && animationFriendlyName )
	{
		for ( IdleAnimationMimicsIterator it( *this ); it; ++it )
		{
			const AnimationMimicsData& anim = *it;

			if ( anim.m_friendlyNameAsName == animationFriendlyName )
			{
				SCENE_ASSERT( anim.m_layerName == layerName );

				animationNameOut = anim.m_animationName;
				return true;
			}
		}
	}

	return false;
}

void CStorySceneAnimationList::CollectBodyIdleAnimations( const SStorySceneActorAnimationState& in, TDynArray< CName >& outAnimations, const CAnimatedComponent* filter ) const
{
	// DIALOG_TOMSIN_TODO - dodac prawdopodobienstwo i bez powtorzen
	// na razie kod ze starego story scene systemu
	// przerobic na wersje bez zbierania do talblicy

	CSkeletalAnimationContainer* c = filter ? filter->GetAnimationContainer() : nullptr;

	for ( BodyAnimationIterator it( *this, in.m_status, in.m_emotionalState, in.m_poseType, ANIMATION_TYPE_IDLE ); it; ++it )
	{
		if ( ( c && c->HasAnimation( (*it).m_animationName ) ) || !c )
		{
			outAnimations.PushBack( (*it).m_animationName );
		}
	}
}

Bool CStorySceneAnimationList::FindBodyTransitions( const SStorySceneActorAnimationState& from, const SStorySceneActorAnimationState& to, TDynArray< CName >& animationNamesOut ) const
{
	for ( BodyAnimationIterator it( *this, from.m_status, from.m_emotionalState, from.m_poseType, ANIMATION_TYPE_TRANSITION ); it; ++it )
	{
		const AnimationBodyData& data = *it;
		if ( data.m_transitionTo && data.m_transitionTo->IsBodyEqual( to ) )
		{
			animationNamesOut.PushBack( data.m_animationName );
		}
	}
	return !animationNamesOut.Empty();
}

Bool CStorySceneAnimationList::FindMimicsTransitions( const SStorySceneActorAnimationState& from, const SStorySceneActorAnimationState& to, TDynArray< CName >& animationNamesOut ) const
{
	return false;
}

void CStorySceneAnimationList::ShowErrorInSceneAnimationsBody( IFeedbackSystem* f, const Uint32& index, const String& msg )
{		
	if ( f )	
	{
		String csvFileName;
		Uint32 csvLocalIndex = 0;
		SSceneAnimationsResourcesManager::GetInstance().GetFileNameAndOffsetSceneAnimationsBody( index, csvFileName, csvLocalIndex );
		f->ShowError( TXT("Row [%u]/Line [%u] - %s. (%ls)"), csvLocalIndex, csvLocalIndex + 1, msg.AsChar(), csvFileName.AsChar() );
	}
}

void CStorySceneAnimationList::ShowErrorInSceneAnimationsMimics( IFeedbackSystem* f, const Uint32& index, const String& msg )
{		
	if ( f )	
	{
		String csvFileName;
		Uint32 csvLocalIndex = 0;
		SSceneAnimationsResourcesManager::GetInstance().GetFileNameAndOffsetSceneAnimationsMimics( index, csvFileName, csvLocalIndex );
		f->ShowError( TXT("Row [%u]/Line [%u] - %s. (%ls)"), csvLocalIndex, csvLocalIndex + 1, msg.AsChar(), csvFileName.AsChar() );
	}
}

void CStorySceneAnimationList::ShowErrorInSceneAnimationsMimicsEmoStates( IFeedbackSystem* f, const Uint32& index, const String& msg )
{		
	if ( f )	
	{
		String csvFileName;
		Uint32 csvLocalIndex = 0;
		SSceneAnimationsResourcesManager::GetInstance().GetFileNameAndOffsetSceneAnimationsMimicsEmoStates( index, csvFileName, csvLocalIndex );
		f->ShowError( TXT("Row [%u]/Line [%u] - %s. (%ls)"), csvLocalIndex, csvLocalIndex + 1, msg.AsChar(), csvFileName.AsChar() );
	}
}

#define SHOW_BODY_ERROR_AND_RET( msg ) ShowErrorInSceneAnimationsBody( f, i, msg ); dataBody.Clear(); return false;
#define SHOW_MIMICS_ERROR_AND_RET( msg ) ShowErrorInSceneAnimationsMimics( f, i, msg ); dataMimicsEmoStatePresets.Clear(); return false;
#define SHOW_EMO_STATE_ERROR_AND_RET( msg ) ShowErrorInSceneAnimationsMimicsEmoStates( f, i, msg );  dataMimicsEmoStatePresets.Clear(); return false;

Bool CStorySceneAnimationList::Load()
{
	SParseParams pp;
	pp.m_dataBody = &m_dataBody;
	pp.m_dataMimics = &m_dataMimics;
	pp.m_dataMimicsEmoStatePresets = &m_dataMimicsEmoStatePresets;
	pp.m_dataMimicsIdleMapping = &m_dataMimicsIdleMapping;
	pp.m_feedback = nullptr;

#ifndef NO_EDITOR
	pp.m_additionalAddittiveAnimMan = &m_additionalAddittiveAnimMan;
	pp.m_additionalAddittiveAnimWoman = &m_additionalAddittiveAnimWoman;
#endif

	return Parse( pp );
}

Bool CStorySceneAnimationList::Parse( SParseParams & pp ) const
{
	// Crazy tree parsing
	IFeedbackSystem*	f = pp.m_feedback;
	TBodyAnimations&	dataBody = *pp.m_dataBody;
	TMimicsAnimations&	dataMimics = *pp.m_dataMimics;
	TMimicsIdleMapping&	dataMimicsIdleMapping = *pp.m_dataMimicsIdleMapping;
	TMimicsEmoStatePresets&	dataMimicsEmoStatePresets = *pp.m_dataMimicsEmoStatePresets;

#ifndef NO_EDITOR
	TAdditionalAdditivesMap&	additionalAddittiveAnimMan = *pp.m_additionalAddittiveAnimMan;
	TAdditionalAdditivesMap&	additionalAddittiveAnimWoman = *pp.m_additionalAddittiveAnimWoman;
#endif

	const C2dArray& arrBody				= SSceneAnimationsResourcesManager::GetInstance().GetSceneAnimationsBody2dArray();
	const C2dArray& arrMimics			= SSceneAnimationsResourcesManager::GetInstance().GetSceneAnimationsMimics2dArray();
	const C2dArray& arrMimicsEmoStates	= SSceneAnimationsResourcesManager::GetInstance().GetSceneAnimationsMimicsEmoStates2dArray();

	if ( arrBody.Empty() == false && arrMimics.Empty() == false && arrMimicsEmoStates.Empty() == false )
	{
		// Body
		{
			const Uint32 COL_STATUS_NAME = 0;
			const Uint32 COL_EMOTIONAL_NAME = 1;
			const Uint32 COL_POSE_NAME = 2;
			const Uint32 COL_ANIM_FRIENDLY_NAME = 3;
			const Uint32 COL_ANIM_NAME = 4;
			const Uint32 COL_ANIM_TYPE = 5;
			const Uint32 COL_ANIM_TRANSITION = 6;
			const Uint32 COL_ANIM_LOOK_ATS = 7;
			const Uint32 COL_ANIM_TAGS_LISTEN = 8;
			const Uint32 COL_GEN_ANIM_SPEED = 9;
			const Uint32 COL_GEN_ANIM_WEIGHT = 10;
			const Uint32 COL_GEN_USE_WITH_MAN = 11;			
			const Uint32 COL_GEN_USE_WITH_WOMAN = 12;			

			Int32 currStatusIdx = -1;
			Int32 currEmotionalIdx = -1;
			Int32 currPoseIdx = -1;
			Int32 currTypeIdx = -1;
			Int32 currAnimationIdx = -1;

			const Uint32 size = (Uint32)arrBody.GetNumberOfRows();
			for ( Uint32 i=0; i<size; ++i )
			{
				// Parse tree
				{
					String statusName = arrBody.GetValueRef( COL_STATUS_NAME, i ).TrimCopy();
					String emotionalName = arrBody.GetValueRef( COL_EMOTIONAL_NAME, i ).TrimCopy();
					String poseName = arrBody.GetValueRef( COL_POSE_NAME, i ).TrimCopy();

					if ( !statusName.Empty() )
					{
						SCENE_ASSERT( !emotionalName.Empty() );
						SCENE_ASSERT( !poseName.Empty() );

						if ( emotionalName.Empty() || poseName.Empty() )
						{
							SHOW_BODY_ERROR_AND_RET( String( TXT("Emotional state name or pose name is empty") ) );
						}

						TStatus currStatus = TStatus( statusName );

						const Int32 size = dataBody.SizeInt();
						Int32 foundIndex = -1;
						for( Int32 i = 0 ; i < size; ++i )
						{
							if( dataBody[ i ].m_first == currStatus )
							{
								foundIndex = i;
								break;
							}
						}

						if( foundIndex == -1 )
						{
							currStatusIdx = (Int32)dataBody.Grow( 1 );
							dataBody[ currStatusIdx ].m_first = currStatus;
						}
						else
						{
							currStatusIdx = foundIndex;
						}
					}

					if ( !emotionalName.Empty() )
					{
						SCENE_ASSERT( !poseName.Empty() );
						SCENE_ASSERT( currStatusIdx != -1 );

						if ( currStatusIdx == -1 || poseName.Empty() )
						{
							SHOW_BODY_ERROR_AND_RET( String( TXT("Pose name is empty or status for this row wasn't found") ) );
						}

						TEmotionalState currEmotional = TEmotionalState( emotionalName );
						TBodyEmotionalStateLevel& bodyEmotionalStateLevel = dataBody[ currStatusIdx ].m_second;
						const Int32 size = bodyEmotionalStateLevel.SizeInt();
						Int32 foundIndex = -1;
						for( Int32 i = 0 ; i < size; ++i )
						{
							if( bodyEmotionalStateLevel[i].m_first == currEmotional )
							{
								foundIndex = i;
								break;
							}
						}

						if( foundIndex == -1 )
						{
							currEmotionalIdx = (Int32)bodyEmotionalStateLevel.Grow( 1 );
							bodyEmotionalStateLevel[ currEmotionalIdx ].m_first = currEmotional;
						}
						else
						{
							currEmotionalIdx = foundIndex;
						}						
					}

					if ( !poseName.Empty() )
					{
						SCENE_ASSERT( currStatusIdx != -1 );
						SCENE_ASSERT( currEmotionalIdx != -1 );

						if ( currEmotionalIdx == -1 || currStatusIdx == -1 )
						{
							SHOW_BODY_ERROR_AND_RET( String( TXT("Status or emotional state for this row wasn't found") ) );
						}
							
						TPose currPose = TPose( poseName );

						TBodyPoseLevel& bodyPoseLevel = dataBody[ currStatusIdx ].m_second[ currEmotionalIdx ].m_second;
						const Int32 size = bodyPoseLevel.SizeInt();
						Int32 foundIndex = -1;
						for( Int32 i = 0 ; i < size; ++i )
						{
							if( bodyPoseLevel[i].m_first == currPose )
							{
								foundIndex = i;
								break;
							}
						}

						if( foundIndex == -1 )
						{
							currPoseIdx = (Int32)bodyPoseLevel.Grow( 1 );
							bodyPoseLevel[ currPoseIdx ].m_first = currPose;
						}
						else
						{
							currPoseIdx = foundIndex;
						}
					}
				}

				// Parse animation data
				{
					SCENE_ASSERT( currStatusIdx != -1 );
					SCENE_ASSERT( currEmotionalIdx != -1 );
					SCENE_ASSERT( currPoseIdx != -1 );

					if ( currEmotionalIdx == -1 || currStatusIdx == -1 || currPoseIdx == -1 )
					{
						SHOW_BODY_ERROR_AND_RET( String( TXT("Status, emotional state or pose name for this animation wasn't found") ) );
					}

					{
						String friendlyName = arrBody.GetValueRef( COL_ANIM_FRIENDLY_NAME, i ).TrimCopy();
						String animationName = arrBody.GetValueRef( COL_ANIM_NAME, i ).TrimCopy();
						String typeName = arrBody.GetValueRef( COL_ANIM_TYPE, i ).TrimCopy();
						String transitionRawData = arrBody.GetValueRef( COL_ANIM_TRANSITION, i ).TrimCopy();
						String lookAtRawData = arrBody.GetValueRef( COL_ANIM_LOOK_ATS, i ).TrimCopy();

						if ( friendlyName.Empty() || animationName.Empty() )
						{
							SHOW_BODY_ERROR_AND_RET( String( TXT("Animation name or friendly name is empty") ) );
						}

						const String separator = TXT(",");

						CName lookAtAnimationBody;
						CName lookAtAnimationHead;

						if ( !lookAtRawData.Empty() )
						{
							TDynArray< String > tokens;
							tokens = lookAtRawData.Split( separator );
							if ( tokens.Size() == 2 )
							{
								lookAtAnimationBody = CName( tokens[ 0 ] );
								lookAtAnimationHead = CName( tokens[ 1 ] );
							}
						}

						size_t chIdx = 0;
						TDynArray< String > tokensArr;
						if ( typeName.FindCharacter( ',', chIdx ) )
						{
							const String separator = TXT(",");
							tokensArr = typeName.Split( separator );
						}
						else
						{
							tokensArr.PushBack( typeName );
						}

						for ( Uint32 t=0; t<tokensArr.Size(); ++t )
						{
							String& tokenStr = tokensArr[t];
							tokenStr.Trim();

							TAnimationType type( tokenStr );

							currTypeIdx = FindOrAddTypeLevel( dataBody, currStatusIdx, currEmotionalIdx, currPoseIdx, type );
							currAnimationIdx = (Int32)dataBody[ currStatusIdx ].m_second[ currEmotionalIdx ].m_second[ currPoseIdx ].m_second[ currTypeIdx ].m_second.Grow( 1 );

							AnimationBodyData& animData = dataBody[ currStatusIdx ].m_second[ currEmotionalIdx ].m_second[ currPoseIdx ].m_second[ currTypeIdx ].m_second[ currAnimationIdx ];
							animData.m_animationName = CName( animationName );
							animData.m_friendlyName = friendlyName;
							animData.m_lookAtBodyAnimationName = lookAtAnimationBody;
							animData.m_lookAtHeadAnimationName = lookAtAnimationHead;

							if ( type == ANIMATION_TYPE_IDLE )
							{
								if ( !animData.m_lookAtBodyAnimationName || !animData.m_lookAtHeadAnimationName )
								{
									SHOW_BODY_ERROR_AND_RET( String( TXT("Look at animation names are empty") ) );
								}
							}

							if ( !transitionRawData.Empty() )
							{
								TDynArray< String > strs = transitionRawData.Split( separator );
								if ( strs.Size() == 3 )
								{
									SStorySceneActorAnimationState* s = new SStorySceneActorAnimationState();
									s->m_status = CName( strs[ 0 ] );
									s->m_emotionalState = CName( strs[ 1 ] );
									s->m_poseType = CName( strs[ 2 ] );
									animData.m_transitionTo = s;
								}
								else
								{
									SHOW_BODY_ERROR_AND_RET( String( TXT("Transition cell doesn't have three parts - [status, emotional state, pose]") ) );
								}
							}
						}
					}
				}

#ifndef NO_EDITOR
				{
					//Dialog event generator data
					if ( currEmotionalIdx == -1 || currStatusIdx == -1 || currPoseIdx == -1 || currTypeIdx == -1  ||  currAnimationIdx == -1 )
					{
						SHOW_BODY_ERROR_AND_RET( String( TXT("Parsing error at line: ") + i ) );
					}					
					const String& animTags = arrBody.GetValueRef( COL_ANIM_TAGS_LISTEN, i );
					const String& animSpeed = arrBody.GetValueRef( COL_GEN_ANIM_SPEED, i );
					const String& animWeight = arrBody.GetValueRef( COL_GEN_ANIM_WEIGHT, i );
					const String& useWithMan = arrBody.GetValueRef( COL_GEN_USE_WITH_MAN, i );
					const String& useWithWoman = arrBody.GetValueRef( COL_GEN_USE_WITH_WOMAN, i );
					AnimationBodyData& animData = dataBody[ currStatusIdx ].m_second[ currEmotionalIdx ].m_second[ currPoseIdx ].m_second[ currTypeIdx ].m_second[ currAnimationIdx ];			
					Float token0 = 0.f, token1  = 0.f;
					TDynArray< String > tokensArr = animSpeed.Split( TXT("-") );
					if( tokensArr.Size() == 2 )
					{
						FromString<Float>( tokensArr[0], token0 );
						FromString<Float>( tokensArr[1], token1 );
						animData.m_generatorData.m_animSpeedRange = TPair<Float,Float>( token0 , token1 );
					}					
					tokensArr = animWeight.Split( TXT("-") );
					if( tokensArr.Size() == 2 )
					{
						FromString<Float>( tokensArr[0], token0 );
						FromString<Float>( tokensArr[1], token1 );
						animData.m_generatorData.m_animWeightRange = TPair<Float,Float>( token0 , token1 );
					}
					const String separator = TXT(",");
					tokensArr = useWithMan.Split( separator );
					for ( Uint32 i = 0; i < tokensArr.Size(); ++i )
					{
						tokensArr[i].Trim();
						TDynArray< String > tokensArr2 = tokensArr[i].Split( TXT(" ") );
						if ( tokensArr2.Size() < 3 )
						{
							continue;
						}
						tokensArr2[0].Trim();
						tokensArr2[1].Trim();
						tokensArr2[2].Trim();
						animData.m_generatorData.m_useOnlyWithManStates.PushBack( SStorySceneActorAnimationState( CName( tokensArr2[0] ), CName( tokensArr2[1] ), CName( tokensArr2[2] ) ) );
					}
					tokensArr = useWithWoman.Split( separator );
					for ( Uint32 i = 0; i < tokensArr.Size(); ++i )
					{
						tokensArr[i].Trim();
						TDynArray< String > tokensArr2 = tokensArr[i].Split( TXT(" ") );
						if ( tokensArr2.Size() < 3 )
						{
							continue;
						}
						tokensArr2[0].Trim();
						tokensArr2[1].Trim();
						tokensArr2[2].Trim();
						animData.m_generatorData.m_useOnlyWithWomanStates.PushBack( SStorySceneActorAnimationState( CName( tokensArr2[0] ), CName( tokensArr2[1] ), CName( tokensArr2[2] ) ) );
					}
					tokensArr = animTags.Split( separator );
					for ( Uint32 i = 0; i < tokensArr.Size(); ++i )
					{
						tokensArr[i].Trim();
						animData.m_generatorData.m_tags.PushBack( CName( tokensArr[i] ) );
					}

					CName status = dataBody[ currStatusIdx ].m_first;
					CName emotionalState = dataBody[ currStatusIdx ].m_second[ currEmotionalIdx ].m_first;
					CName pose = dataBody[ currStatusIdx ].m_second[ currEmotionalIdx ].m_second[ currPoseIdx ].m_first;
					CName type = dataBody[ currStatusIdx ].m_second[ currEmotionalIdx ].m_second[ currPoseIdx ].m_second[ currTypeIdx ].m_first;
					if ( status != CStorySceneAnimationList::ADDITIVE_KEYWORD &&
						emotionalState != CStorySceneAnimationList::ADDITIVE_KEYWORD &&
						pose != CStorySceneAnimationList::ADDITIVE_KEYWORD &&
						type != CStorySceneAnimationList::ADDITIVE_KEYWORD )
					{
						for( Uint32 i = 0; i < animData.m_generatorData.m_useOnlyWithWomanStates.Size(); i++ )
						{
							SStorySceneActorAnimationState& state = animData.m_generatorData.m_useOnlyWithWomanStates[i];
							TPair<SStorySceneActorAnimationState, TDynArray< AnimationBodyData >>& val = additionalAddittiveAnimWoman.GetRef( state );
							val.m_first = SStorySceneActorAnimationState( status, emotionalState, pose, type  ) ;
							val.m_second.PushBack( animData );
						}
						for( Uint32 i = 0; i < animData.m_generatorData.m_useOnlyWithManStates.Size(); i++ )
						{
							SStorySceneActorAnimationState& state = animData.m_generatorData.m_useOnlyWithManStates[i];
							TPair<SStorySceneActorAnimationState, TDynArray< AnimationBodyData >>& val = additionalAddittiveAnimMan.GetRef( state );
							val.m_first = SStorySceneActorAnimationState( status, emotionalState, pose, type  ) ;
							val.m_second.PushBack( animData );
						}
					}
				}
#endif
			}
		}

		// Mimics
		{
			const Uint32 COL_ACTION_NAME = 0;
			const Uint32 COL_ANIM_FRIENDLY_NAME = 1;
			const Uint32 COL_ANIM_NAME = 2;
			const Uint32 COL_TYPE = 3;
			const Uint32 COL_LAYER = 4;
			const Uint32 COL_GEN_TAGS = 5;
			const Uint32 COL_GEN_ANIM_SPEED = 6;
			const Uint32 COL_GEN_ANIM_WEIGHT = 7;			

			Int32 currActionIdx = -1;
			Int32 animationIdx = -1;

			SCENE_ASSERT( arrMimics.GetNumberOfColumns() > COL_LAYER );

			const Uint32 size = (Uint32)arrMimics.GetNumberOfRows();
			for ( Uint32 i=0; i<size; ++i )
			{
				// Parse tree
				{
					const String& actionName = arrMimics.GetValueRef( COL_ACTION_NAME, i );

					if ( !actionName.Empty() )
					{
						TActionType currAction = TActionType( actionName );
						const Int32 size = dataMimics.SizeInt();
						Int32 foundIndex = -1;
						for( Int32 i = 0 ; i < size; ++i )
						{
							if( dataMimics[i].m_first == currAction )
							{
								foundIndex = i;
								break;
							}
						}

						if( foundIndex == -1 )
						{
							currActionIdx = (Int32)dataMimics.Grow( 1 );
							dataMimics[ currActionIdx ].m_first = currAction;
						}
						else
						{
							currActionIdx = foundIndex;
						}
					}
				}

				// Parse animation data
				{
					SCENE_ASSERT( currActionIdx != -1 );

					if ( currActionIdx == -1 )
					{
						SHOW_MIMICS_ERROR_AND_RET( String( TXT("Action type for this mimics animation wasn't found") ) );
					}

					{
						const String& friendlyName = arrMimics.GetValueRef( COL_ANIM_FRIENDLY_NAME, i );
						const String& animationName = arrMimics.GetValueRef( COL_ANIM_NAME, i );
						const String& typeName = arrMimics.GetValueRef( COL_TYPE, i );
						const String& layerName = arrMimics.GetValueRef( COL_LAYER, i );

						if ( friendlyName.Empty() )
						{
							SHOW_MIMICS_ERROR_AND_RET( String( TXT("Mimics friendly animation name is empty") ) );
						}
						if ( animationName.Empty() )
						{
							SHOW_MIMICS_ERROR_AND_RET( String( TXT("Mimics animation name is empty") ) );
						}
						if ( typeName.Empty() )
						{
							SHOW_MIMICS_ERROR_AND_RET( String( TXT("Animation type name is empty") ) );
						}

						animationIdx = (Int32)dataMimics[ currActionIdx ].m_second.Grow( 1 );

						AnimationMimicsData& animData = dataMimics[ currActionIdx ].m_second[ animationIdx ];
						animData.m_animationName = CName( animationName );
						animData.m_friendlyName = friendlyName;
						animData.m_friendlyNameAsName = CName( friendlyName );
						animData.m_typeName = CName( typeName );
						animData.m_layerName = CName( layerName );

						if ( animData.m_layerName && animData.m_layerName != LAYER_POSE && animData.m_layerName != LAYER_EYES && animData.m_layerName != LAYER_ANIMATION )
						{
							SHOW_MIMICS_ERROR_AND_RET( String( TXT("Mimics layer type is invalid") ) );
						}

						if ( animData.m_typeName == IDLE_KEYWORD )
						{
							dataMimicsIdleMapping.PushBack( TMimicIdleMappingRecord( currActionIdx, animationIdx ) );
						}
					}
				}

#ifndef NO_EDITOR
				{
					//Dialog event generator data
					if ( currActionIdx == -1 )
					{
						SHOW_BODY_ERROR_AND_RET( String( TXT("Parsing error at line: ") + i ) );
					}					
					const String& animTags = arrMimics.GetValueRef( COL_GEN_TAGS, i );
					const String& animSpeed = arrMimics.GetValueRef( COL_GEN_ANIM_SPEED, i );
					const String& animWeight = arrMimics.GetValueRef( COL_GEN_ANIM_WEIGHT, i );
					
					AnimationMimicsData& animData = dataMimics[ currActionIdx ].m_second[ animationIdx ];			
					Float token0 = 0.f, token1  = 0.f;
					TDynArray< String > tokensArr = animSpeed.Split( TXT("-") );
					if( tokensArr.Size() == 2 )
					{
						FromString<Float>( tokensArr[0], token0 );
						FromString<Float>( tokensArr[1], token1 );
						animData.m_generatorData.m_animSpeedRange = TPair<Float,Float>( token0 , token1 );
					}					
					tokensArr = animWeight.Split( TXT("-") );
					if( tokensArr.Size() == 2 )
					{
						FromString<Float>( tokensArr[0], token0 );
						FromString<Float>( tokensArr[1], token1 );
						animData.m_generatorData.m_animWeightRange = TPair<Float,Float>( token0 , token1 );
					}				
					const String separator = TXT(",");
					tokensArr = animTags.Split( separator );
					for ( Uint32 i = 0; i < tokensArr.Size(); ++i )
					{
						tokensArr[i].Trim();
						animData.m_generatorData.m_tags.PushBack( CName( tokensArr[i] ) );
					}
				}
#endif
			}
		}

		// Mimics emotional states
		{
			const Uint32 COL_EMO_STATE_NAME = 0;
			const Uint32 COL_LAYER_EYES = 1;
			const Uint32 COL_LAYER_POSE = 2;
			const Uint32 COL_LAYER_ANIMATION = 3;

			const Uint32 size = (Uint32)arrMimicsEmoStates.GetNumberOfRows();
			dataMimicsEmoStatePresets.Reserve( size );

			for ( Uint32 i=0; i<size; ++i )
			{
				const String& emoState = arrMimicsEmoStates.GetValueRef( COL_EMO_STATE_NAME, i );
				const String& layerEyes = arrMimicsEmoStates.GetValueRef( COL_LAYER_EYES, i );
				const String& layerPose = arrMimicsEmoStates.GetValueRef( COL_LAYER_POSE, i );
				const String& layerAnimation = arrMimicsEmoStates.GetValueRef( COL_LAYER_ANIMATION, i );

				MimicsEmoStatePreset preset;
				preset.m_emoState = CName( emoState );
				preset.m_layerEyes = CName( layerEyes );
				preset.m_layerPose = CName( layerPose );
				preset.m_layerAnimation = CName( layerAnimation );

				if ( !preset.m_emoState || !preset.m_layerEyes || !preset.m_layerPose || !preset.m_layerAnimation )
				{
					SHOW_EMO_STATE_ERROR_AND_RET( String( TXT("You must fill all columns") ) );
				}

				dataMimicsEmoStatePresets.PushBack( preset );
			}
		}

		return true;
	}

	return false;
}

const CStorySceneAnimationList::MimicsEmoStatePreset* CStorySceneAnimationList::FindMimicsAnimationByEmoState( const CName& emoState ) const
{
	if ( !emoState )
	{
		return nullptr;
	}

	for ( EmotionalStateMimicsIterator it( *this ); it; ++it )
	{
		const MimicsEmoStatePreset& data = *it;
		if ( data.m_emoState == emoState )
		{
			return &data;
		}
	}

	return nullptr;
}

#ifndef NO_EDITOR

void CStorySceneAnimationList::ShowErrors( IFeedbackSystem* f ) const
{
	SParseParams pp;

	TBodyAnimations dataB;
	TMimicsAnimations dataM;
	TMimicsIdleMapping dataMIdles;
	TMimicsEmoStatePresets dataME;

	pp.m_dataBody = &dataB;
	pp.m_dataMimics = &dataM;
	pp.m_dataMimicsEmoStatePresets = &dataME;
	pp.m_dataMimicsIdleMapping = &dataMIdles;
	pp.m_feedback = f;

#ifndef NO_EDITOR
	TAdditionalAdditivesMap	additionalAddittiveAnimMan;
	TAdditionalAdditivesMap	additionalAddittiveAnimWoman;
	pp.m_additionalAddittiveAnimMan = &additionalAddittiveAnimMan;
	pp.m_additionalAddittiveAnimWoman = &additionalAddittiveAnimWoman;
#endif
	Parse( pp );
}

CName CStorySceneAnimationList::FindBodyAnimationByFriendlyName( const CName& status, const CName& emoState, const CName& pose, const CName& type, const String& friendlyName ) const
{
	for ( BodyAnimationIterator it( *this, status, emoState, pose, type ); it; ++it )
	{
		const AnimationBodyData& data = *it;
		if ( data.m_friendlyName == friendlyName )
		{
			return data.m_animationName;
		}
	}

	return CName::NONE;
}

CName CStorySceneAnimationList::FindMimicsAnimationByFriendlyName( const CName& friendlyName ) const
{
	if ( !friendlyName )
	{
		return CName::NONE;
	}

	for ( AllMimicsAnimationsIterator it( *this ); it; ++it )
	{
		const AnimationMimicsData& data = *it;
		if ( data.m_friendlyNameAsName == friendlyName )
		{
			return data.m_animationName;
		}
	}

	return CName::NONE;
}

#endif

//////////////////////////////////////////////////////////////////////////

CStorySceneAnimationList::StatusBodyIterator::StatusBodyIterator( const CStorySceneAnimationList& list )
	: m_list( list )
	, m_index( 0 )
{
}

CStorySceneAnimationList::StatusBodyIterator::operator Bool () const
{
	return m_index < GetList().m_dataBody.Size();
}

void CStorySceneAnimationList::StatusBodyIterator::operator++ ()
{
	++m_index;
}

const CName& CStorySceneAnimationList::StatusBodyIterator::operator*() const
{
	return GetList().m_dataBody[ m_index ].m_first;
}

Int32 CStorySceneAnimationList::StatusBodyIterator::GetCurrentIndex() const
{
	return m_index;
}

CStorySceneAnimationList::EmotionalStatesBodyIterator::EmotionalStatesBodyIterator( const CStorySceneAnimationList& list, const CName& status )
	: CStorySceneAnimationList::StatusBodyIterator( list )
	, m_statusIdx( -1 )
{
	const Int32 size = GetList().m_dataBody.SizeInt();
	for ( Int32 i=0; i<size; ++i )
	{
		if ( GetList().m_dataBody[ i ].m_first == status )
		{
			m_statusIdx = i;
			break;
		}
	}
}

CStorySceneAnimationList::EmotionalStatesBodyIterator::EmotionalStatesBodyIterator( const CStorySceneAnimationList& list, Int32 statusIdx )
	: CStorySceneAnimationList::StatusBodyIterator( list )
	, m_statusIdx( statusIdx )
{
	if (GetList().m_dataBody.SizeInt()>0)
	{
		ASSERT( m_statusIdx <  GetList().m_dataBody.SizeInt() );
	}
}

void CStorySceneAnimationList::EmotionalStatesBodyIterator::Restart( const CStorySceneAnimationList::StatusBodyIterator& it )
{
	m_statusIdx = it.GetCurrentIndex();
	m_index = 0;
}

CStorySceneAnimationList::EmotionalStatesBodyIterator::operator Bool () const
{
	return m_statusIdx != -1 && m_index < GetList().m_dataBody[ m_statusIdx ].m_second.Size();
}

void CStorySceneAnimationList::EmotionalStatesBodyIterator::operator++ ()
{
	++m_index;
}

const CName& CStorySceneAnimationList::EmotionalStatesBodyIterator::operator*() const
{
	return GetList().m_dataBody[ m_statusIdx ].m_second[ m_index ].m_first;
}

Int32 CStorySceneAnimationList::EmotionalStatesBodyIterator::GetStatusIndex() const
{
	return m_statusIdx;
}

CStorySceneAnimationList::PoseBodyIterator::PoseBodyIterator( const CStorySceneAnimationList& list, const CName& status, const CName& emotionalState )
	: CStorySceneAnimationList::EmotionalStatesBodyIterator( list, status )
	, m_emotionalStateIdx( -1 )
{
	if ( m_statusIdx != -1 )
	{
		const Int32 size2 = GetList().m_dataBody[ m_statusIdx ].m_second.SizeInt();
		for ( Int32 j=0; j<size2; ++j )
		{
			if ( GetList().m_dataBody[ m_statusIdx ].m_second[ j ].m_first == emotionalState )
			{
				m_emotionalStateIdx = j;
				break;
			}
		}
	}
}

CStorySceneAnimationList::PoseBodyIterator::PoseBodyIterator( const CStorySceneAnimationList& list, Int32 statusIdx, Int32 emotionalStateIdx )
	: CStorySceneAnimationList::EmotionalStatesBodyIterator( list, statusIdx )
	, m_emotionalStateIdx( emotionalStateIdx )
{
	if (GetList().m_dataBody.SizeInt() > 0)
	{
		ASSERT( m_statusIdx <  GetList().m_dataBody.SizeInt() );
		ASSERT( m_emotionalStateIdx <  GetList().m_dataBody[ m_statusIdx ].m_second.SizeInt() );
	}
}

void CStorySceneAnimationList::PoseBodyIterator::Restart( const CStorySceneAnimationList::EmotionalStatesBodyIterator& it )
{
	m_statusIdx = it.GetStatusIndex();
	m_emotionalStateIdx = it.GetCurrentIndex();
	m_index = 0;
}

CStorySceneAnimationList::PoseBodyIterator::operator Bool () const
{
	return m_emotionalStateIdx != -1 && m_statusIdx != -1 && m_index < GetList().m_dataBody[ m_statusIdx ].m_second[ m_emotionalStateIdx ].m_second.Size();
}

void CStorySceneAnimationList::PoseBodyIterator::operator++ ()
{
	++m_index;
}

const CName& CStorySceneAnimationList::PoseBodyIterator::operator*() const
{
	return GetList().m_dataBody[ m_statusIdx ].m_second[ m_emotionalStateIdx ].m_second[ m_index ].m_first;
}

Int32 CStorySceneAnimationList::PoseBodyIterator::GetEmotionalStateIndex() const
{
	return m_emotionalStateIdx;
}

CStorySceneAnimationList::TypeBodyIterator::TypeBodyIterator( const CStorySceneAnimationList& list, const CName& status, const CName& emotionalState, const CName& pose )
	: CStorySceneAnimationList::PoseBodyIterator( list, status, emotionalState )
	, m_poseIdx( -1 )
{
	if ( m_emotionalStateIdx != -1 )
	{
		ASSERT( m_statusIdx != -1 );

		const Int32 size3 = GetList().m_dataBody[ m_statusIdx ].m_second[ m_emotionalStateIdx ].m_second.SizeInt();
		for ( Int32 k=0; k<size3; ++k )
		{
			if ( GetList().m_dataBody[ m_statusIdx ].m_second[ m_emotionalStateIdx ].m_second[ k ].m_first == pose )
			{
				m_poseIdx = k;
				break;
			}
		}
	}
}

CStorySceneAnimationList::TypeBodyIterator::TypeBodyIterator( const CStorySceneAnimationList& list, Int32 statusIdx, Int32 emotionalStateIdx, Int32 poseIdx )
	: CStorySceneAnimationList::PoseBodyIterator( list, statusIdx, emotionalStateIdx )
	, m_poseIdx( poseIdx )
{
	if (GetList().m_dataBody.SizeInt()>0)
	{
		ASSERT( poseIdx <  GetList().m_dataBody[ m_statusIdx ].m_second[ m_emotionalStateIdx ].m_second.SizeInt() );
	}
}

void CStorySceneAnimationList::TypeBodyIterator::Restart( const CStorySceneAnimationList::PoseBodyIterator& it )
{
	m_statusIdx = it.GetStatusIndex();
	m_emotionalStateIdx = it.GetEmotionalStateIndex();
	m_poseIdx = it.GetCurrentIndex();
	m_index = 0;
}

CStorySceneAnimationList::TypeBodyIterator::operator Bool () const
{
	return m_poseIdx != -1 && m_emotionalStateIdx != -1 && m_statusIdx != -1 && m_index < GetList().m_dataBody[ m_statusIdx ].m_second[ m_emotionalStateIdx ].m_second[ m_poseIdx ].m_second.Size();
}

void CStorySceneAnimationList::TypeBodyIterator::operator++ ()
{
	++m_index;
}

const CName& CStorySceneAnimationList::TypeBodyIterator::operator*() const
{
	return GetList().m_dataBody[ m_statusIdx ].m_second[ m_emotionalStateIdx ].m_second[ m_poseIdx ].m_second[ m_index ].m_first;
}

Int32 CStorySceneAnimationList::TypeBodyIterator::GetPoseIndex() const
{
	return m_poseIdx;
}

CStorySceneAnimationList::BodyAnimationIterator::BodyAnimationIterator( const CStorySceneAnimationList& list, const CName& status, const CName& emotionalState, const CName& pose, const CName& type )
	: CStorySceneAnimationList::TypeBodyIterator( list, status, emotionalState, pose )
	, m_typeIdx( -1 )
{
	if ( m_poseIdx != -1 )
	{
		ASSERT( m_statusIdx != -1 );
		ASSERT( m_emotionalStateIdx != -1 );
		ASSERT( m_poseIdx != -1 );

		const Int32 size4 = GetList().m_dataBody[ m_statusIdx ].m_second[ m_emotionalStateIdx ].m_second[ m_poseIdx ].m_second.SizeInt();
		for ( Int32 p=0; p<size4; ++p )
		{
			if ( GetList().m_dataBody[ m_statusIdx ].m_second[ m_emotionalStateIdx ].m_second[ m_poseIdx ].m_second[ p ].m_first == type )
			{
				m_typeIdx = p;
				break;
			}
		}
	}
}

CStorySceneAnimationList::BodyAnimationIterator::BodyAnimationIterator( const CStorySceneAnimationList& list, Int32 statusIdx, Int32 emotionalStateIdx, Int32 poseIdx, Int32 typeIdx )
	: CStorySceneAnimationList::TypeBodyIterator( list, statusIdx, emotionalStateIdx, poseIdx )
	, m_typeIdx( typeIdx )
{
	if (GetList().m_dataBody.SizeInt()>0)
	{
		ASSERT( m_statusIdx < GetList().m_dataBody.SizeInt() );
		ASSERT( m_emotionalStateIdx <  GetList().m_dataBody[ m_statusIdx ].m_second.SizeInt() );
		ASSERT( poseIdx <  GetList().m_dataBody[ m_statusIdx ].m_second[ m_emotionalStateIdx ].m_second.SizeInt() );
		ASSERT( poseIdx <  GetList().m_dataBody[ m_statusIdx ].m_second[ m_emotionalStateIdx ].m_second.SizeInt() );
		ASSERT( m_typeIdx <  GetList().m_dataBody[ m_statusIdx ].m_second[ m_emotionalStateIdx ].m_second[ m_poseIdx ].m_second.SizeInt() );
	}
}

void CStorySceneAnimationList::BodyAnimationIterator::Restart( const CStorySceneAnimationList::TypeBodyIterator& it )
{
	m_statusIdx = it.GetStatusIndex();
	m_emotionalStateIdx = it.GetEmotionalStateIndex();
	m_poseIdx = it.GetPoseIndex();
	m_typeIdx = it.GetCurrentIndex();
	m_index = 0;
}

CStorySceneAnimationList::BodyAnimationIterator::operator Bool () const
{
	return m_typeIdx != -1 && m_poseIdx != -1 && m_emotionalStateIdx != -1 && m_statusIdx != -1 && m_index < GetList().m_dataBody[ m_statusIdx ].m_second[ m_emotionalStateIdx ].m_second[ m_poseIdx ].m_second[ m_typeIdx ].m_second.Size();
}

void CStorySceneAnimationList::BodyAnimationIterator::operator++ ()
{
	++m_index;
}

const CStorySceneAnimationList::AnimationBodyData& CStorySceneAnimationList::BodyAnimationIterator::operator*() const
{
	return GetList().m_dataBody[ m_statusIdx ].m_second[ m_emotionalStateIdx ].m_second[ m_poseIdx ].m_second[ m_typeIdx ].m_second[ m_index ];
}

Int32 CStorySceneAnimationList::BodyAnimationIterator::GetTypeIndex() const
{
	return m_typeIdx;
}

CStorySceneAnimationList::AllBodyAnimationsIterator::AllBodyAnimationsIterator( const CStorySceneAnimationList& list )
	: m_statusIt( list )
	, m_emoIt( list )
	, m_poseIt( list )
	, m_typeIt( list )
	, m_animIt( list )
{

}

CStorySceneAnimationList::AllBodyAnimationsIterator::operator Bool () const
{
	return m_statusIt && m_emoIt && m_poseIt && m_typeIt && m_animIt;
}

void CStorySceneAnimationList::AllBodyAnimationsIterator::operator++ ()
{
	++m_animIt;

	if ( !m_animIt )
	{
		++m_typeIt;

		m_animIt.Restart( m_typeIt );

		if ( !m_typeIt )
		{
			++m_poseIt;

			m_typeIt.Restart( m_poseIt );
			m_animIt.Restart( m_typeIt );

			if ( !m_poseIt )
			{
				++m_emoIt;

				m_poseIt.Restart( m_emoIt );
				m_typeIt.Restart( m_poseIt );
				m_animIt.Restart( m_typeIt );

				if ( !m_emoIt )
				{
					++m_statusIt;

					m_emoIt.Restart( m_statusIt );
					m_poseIt.Restart( m_emoIt );
					m_typeIt.Restart( m_poseIt );
					m_animIt.Restart( m_typeIt );
				}
			}
		}
	}
}

const CStorySceneAnimationList::AnimationBodyData& CStorySceneAnimationList::AllBodyAnimationsIterator::operator*() const
{
	return *m_animIt;
}

const CName& CStorySceneAnimationList::AllBodyAnimationsIterator::GetStatus() const
{
	return *m_statusIt;
}

const CName& CStorySceneAnimationList::AllBodyAnimationsIterator::GetEmoState() const
{
	return *m_emoIt;
}

const CName& CStorySceneAnimationList::AllBodyAnimationsIterator::GetPose() const
{
	return *m_poseIt;
}

const CName& CStorySceneAnimationList::AllBodyAnimationsIterator::GetTypeName() const
{
	return *m_typeIt;
}

//////////////////////////////////////////////////////////////////////////

CStorySceneAnimationList::IdleAnimationMimicsIterator::IdleAnimationMimicsIterator( const CStorySceneAnimationList& list )
	: m_list( list )
	, m_index( 0 )
{
}

CStorySceneAnimationList::IdleAnimationMimicsIterator::operator Bool () const
{
	return m_index < GetList().m_dataMimicsIdleMapping.Size();
}

void CStorySceneAnimationList::IdleAnimationMimicsIterator::operator++ ()
{
	++m_index;
}

const CStorySceneAnimationList::AnimationMimicsData& CStorySceneAnimationList::IdleAnimationMimicsIterator::operator*() const
{
	const TMimicIdleMappingRecord& idlePlace = GetList().m_dataMimicsIdleMapping[ m_index ];
	const AnimationMimicsData& animation = GetList().m_dataMimics[ idlePlace.m_first ].m_second[ idlePlace.m_second ];
	return animation;
}

CStorySceneAnimationList::EmotionalStateMimicsIterator::EmotionalStateMimicsIterator( const CStorySceneAnimationList& list )
	: m_list( list )
	, m_index( 0 )
{
}

CStorySceneAnimationList::EmotionalStateMimicsIterator::operator Bool () const
{
	return m_index < GetList().m_dataMimicsEmoStatePresets.Size();
}

void CStorySceneAnimationList::EmotionalStateMimicsIterator::operator++ ()
{
	++m_index;
}

const CStorySceneAnimationList::MimicsEmoStatePreset& CStorySceneAnimationList::EmotionalStateMimicsIterator::operator*() const
{
	return GetList().m_dataMimicsEmoStatePresets[ m_index ];
}

CStorySceneAnimationList::LayerMimicsAnimationIterator::LayerMimicsAnimationIterator( const CStorySceneAnimationList& list, const CName& layerName )
	: m_list( list )
	, m_index( -1 )
	, m_layerName( layerName )
{
	Next();
}

CStorySceneAnimationList::LayerMimicsAnimationIterator::operator Bool () const
{
	return m_index < m_list.m_dataMimicsIdleMapping.SizeInt();
}

void CStorySceneAnimationList::LayerMimicsAnimationIterator::operator++ ()
{
	Next();
}

const CStorySceneAnimationList::AnimationMimicsData& CStorySceneAnimationList::LayerMimicsAnimationIterator::operator*() const
{
	const TMimicIdleMappingRecord& idlePlace = m_list.m_dataMimicsIdleMapping[ m_index ];
	const AnimationMimicsData& animation = m_list.m_dataMimics[ idlePlace.m_first ].m_second[ idlePlace.m_second ];
	return animation;
}

void CStorySceneAnimationList::LayerMimicsAnimationIterator::Next()
{
	while ( ++m_index < m_list.m_dataMimicsIdleMapping.SizeInt() )
	{
		const TMimicIdleMappingRecord& m = m_list.m_dataMimicsIdleMapping[ m_index ];
		const AnimationMimicsData& anim = m_list.m_dataMimics[ m.m_first ].m_second[ m.m_second ];

		if ( anim.m_layerName == m_layerName )
		{
			break;
		}
	}
}

CStorySceneAnimationList::ActionTypeMimicsIterator::ActionTypeMimicsIterator( const CStorySceneAnimationList& list )
	: m_list( list )
	, m_index( 0 )
{
}

CStorySceneAnimationList::ActionTypeMimicsIterator::operator Bool () const
{
	return m_index < GetList().m_dataMimics.Size();
}

void CStorySceneAnimationList::ActionTypeMimicsIterator::operator++ ()
{
	++m_index;
}

const CName& CStorySceneAnimationList::ActionTypeMimicsIterator::operator*() const
{
	return GetList().m_dataMimics[ m_index ].m_first;
}

Int32 CStorySceneAnimationList::ActionTypeMimicsIterator::GetCurrentIndex() const
{
	return m_index;
}

CStorySceneAnimationList::MimicsAnimationIteratorByEmoState::MimicsAnimationIteratorByEmoState( const CStorySceneAnimationList& list, const CName& mimicEmoState )
	: m_list( list )
	, m_index( -1 )
	, m_emoState( mimicEmoState )
{
	Next();
}

CStorySceneAnimationList::MimicsAnimationIteratorByEmoState::operator Bool () const
{
	return m_emoState && m_index < m_list.m_dataMimicsIdleMapping.SizeInt();
}

void CStorySceneAnimationList::MimicsAnimationIteratorByEmoState::operator++ ()
{
	Next();
}

void CStorySceneAnimationList::MimicsAnimationIteratorByEmoState::Next()
{
	while ( ++m_index < m_list.m_dataMimicsIdleMapping.SizeInt() )
	{
		const TMimicIdleMappingRecord& m = m_list.m_dataMimicsIdleMapping[ m_index ];

		SCENE_ASSERT( m.m_first < m_list.m_dataMimics.SizeInt() );
		SCENE_ASSERT( m.m_second < m_list.m_dataMimics[ m.m_first ].m_second.SizeInt() );

		const AnimationMimicsData& anim = m_list.m_dataMimics[ m.m_first ].m_second[ m.m_second ];

		//if ( anim.m_emotionalState == m_emoState )
		//{
		//	break;
		//}
	}
}

const CStorySceneAnimationList::AnimationMimicsData& CStorySceneAnimationList::MimicsAnimationIteratorByEmoState::operator*() const
{
	const TMimicIdleMappingRecord& m = m_list.m_dataMimicsIdleMapping[ m_index ];
	return m_list.m_dataMimics[ m.m_first ].m_second[ m.m_second ];
}

CStorySceneAnimationList::MimicsAnimationIteratorByAction::MimicsAnimationIteratorByAction( const CStorySceneAnimationList& list, Int32 actionIdx )
	: CStorySceneAnimationList::ActionTypeMimicsIterator( list )
	, m_actionIdx( actionIdx )
{
	if (GetList().m_dataMimics.SizeInt()>0)
	{
		ASSERT( m_actionIdx < GetList().m_dataMimics.SizeInt() );
	}
}

CStorySceneAnimationList::MimicsAnimationIteratorByAction::MimicsAnimationIteratorByAction( const CStorySceneAnimationList& list, const CName& actionType )
	: CStorySceneAnimationList::ActionTypeMimicsIterator( list )
	, m_actionIdx( -1 )
{
	const Int32 size = GetList().m_dataMimics.SizeInt();
	for ( Int32 i=0; i<size; ++i )
	{
		if ( GetList().m_dataMimics[ i ].m_first == actionType )
		{
			m_actionIdx = i;
			break;
		}
	}
	if (GetList().m_dataMimics.SizeInt()>0)
	{
		ASSERT( m_actionIdx < GetList().m_dataMimics.SizeInt() );
	}
}

void CStorySceneAnimationList::MimicsAnimationIteratorByAction::Restart( const CStorySceneAnimationList::ActionTypeMimicsIterator& it )
{
	m_actionIdx = it.GetCurrentIndex();
	m_index = 0;
}

CStorySceneAnimationList::MimicsAnimationIteratorByAction::operator Bool () const
{
	return m_index != -1 && m_actionIdx != -1 && m_index < GetList().m_dataMimics[ m_actionIdx ].m_second.Size();
}

void CStorySceneAnimationList::MimicsAnimationIteratorByAction::operator++ ()
{
	++m_index;
}

const CStorySceneAnimationList::AnimationMimicsData& CStorySceneAnimationList::MimicsAnimationIteratorByAction::operator*() const
{
	return GetList().m_dataMimics[ m_actionIdx ].m_second[ m_index ];
}

Int32 CStorySceneAnimationList::MimicsAnimationIteratorByAction::GetActionTypeIndex() const
{
	return m_actionIdx;
}

CStorySceneAnimationList::AllMimicsAnimationsIterator::AllMimicsAnimationsIterator( const CStorySceneAnimationList& list )
	: m_actionIt( list )
	, m_animIt( list )
{

}

CStorySceneAnimationList::AllMimicsAnimationsIterator::operator Bool () const
{
	return m_actionIt && m_animIt;
}

void CStorySceneAnimationList::AllMimicsAnimationsIterator::operator++ ()
{
	++m_animIt;

	if ( !m_animIt )
	{
		++m_actionIt;

		m_animIt.Restart( m_actionIt );
	}
}

const CStorySceneAnimationList::AnimationMimicsData& CStorySceneAnimationList::AllMimicsAnimationsIterator::operator*() const
{
	return *m_animIt;
}

const CName& CStorySceneAnimationList::AllMimicsAnimationsIterator::GetActionType() const
{
	return *m_actionIt;
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
