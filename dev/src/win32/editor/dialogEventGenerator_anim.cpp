#include "build.h"
#include "dialogEventGenerator.h"

#include "dialogEventGeneratorInternals.h"
#include "..\..\common\game\storySceneEventAnimation.h"
#include "..\..\common\game\storySceneAnimationList.h"
#include "..\..\common\engine\engineTypeRegistry.h"
#include "..\..\common\game\storySceneEventMimicsAnim.h"
#include "..\..\common\game\storySceneSystem.h"
#include "../../common/game/sceneAnimEvents.h"
#include "../../common/engine/localizationManager.h"
#include "../../common/engine/animGlobalParam.h"
#include "../../common/engine/skeletalAnimationEntry.h"
#include "../../common/engine/skeletalAnimationContainer.h"


void CStorySceneEventGenerator::CreateAnimationEvent( SContext& context, CName actorName, const AnimData& animData, const SPositionInSection & eventPos, const String& debugString )
{
	if( !animData.animName || !actorName )
	{
		return;
	}

	if ( m_editor->OnEvtGenerator_IsTimelineTrackLocked( actorName.AsString() + animData.trackName ) )
	{
		return;
	}

	CStorySceneElement* element = NULL;
	Float pos = 0.f;

	Float clipFront = 0.f;
	Float posAbs = eventPos.PosAbs();
	if ( posAbs < 0.f )
	{
		clipFront = -posAbs;
	}

	GetEventPosition( eventPos, element, pos );
	if( element )
	{			
		context.MarkAnimationUsed( animData.animName, &eventPos.GetChunk(), actorName );	
		CStorySceneEventAnimClip* newEvent = nullptr;
		if ( animData.type == AnimData::Additive || animData.type == AnimData::Animation )
		{
			CStorySceneEventAnimation* newAnimEvent = nullptr;
			if( animData.type == AnimData::Additive )
			{
				newAnimEvent = new CStorySceneEventAdditiveAnimation( String::EMPTY , element ,pos ,actorName ,animData.animName.AsString() , actorName.AsString() +  animData.trackName );
			}
			else
			{
				newAnimEvent = new CStorySceneEventAnimation( String::EMPTY , element ,pos ,actorName ,animData.animName.AsString() , actorName.AsString() +  animData.trackName );
			}
			SCENE_ASSERT( animData.additionalData.Size() == 4 );
			newAnimEvent->SetBodyFilterStatus( animData.additionalData[0] );
			newAnimEvent->SetBodyFilterEmotionalState( animData.additionalData[1] );
			newAnimEvent->SetBodyFilterPoseName( animData.additionalData[2] );
			newAnimEvent->SetBodyFilterTypeName( animData.additionalData[3] );
			newAnimEvent->SetAnimationFriendlyName( animData.friendlyName );
			newEvent = newAnimEvent;
		}
		else if( animData.type == AnimData::Mimic )
		{
			CStorySceneEventMimicsAnim* newMimicEvent = new CStorySceneEventMimicsAnim( String::EMPTY , element ,pos ,actorName , actorName.AsString() +  animData.trackName );
			newMimicEvent->SetAnimationName( animData.animName );
			SCENE_ASSERT( animData.additionalData.Size() == 1 );
			newMimicEvent->SetMimicksActionFilter( animData.additionalData[0] );
			newMimicEvent->SetAnimationFriendlyName( animData.friendlyName );
			newEvent = newMimicEvent;
		}
		else
		{
			return;
		}

		if( animData.blendTimeIn > 0.f )
		{
			newEvent->SetAnimationBlendIn( animData.blendTimeIn );			
		}
		if( animData.blendTimeOut > 0.f )
		{
			newEvent->SetAnimationBlendOut( animData.blendTimeOut );
		}
		newEvent->SetAnimationClipStart( clipFront );
		newEvent->SetAnimationWeight( animData.animWeight );
		newEvent->SetAnimationStretch( 1.f / animData.animSpeed );

		newEvent->RefreshDuration( animData.animDuration );
		newEvent->SetDebugString( debugString );

		const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
		const CStorySceneSectionVariantId sectionVariantId = context.m_section->GetVariantUsedByLocale( currentLocaleId );
		context.m_section->AddEvent( newEvent, sectionVariantId );
	}
}



void CStorySceneEventGenerator::ProcessAnimationEntry( SContext& context, const AnimSearchContext& animContext, AnimSearchData& searchData )
{
	const CSkeletalAnimationSetEntry* anim = animContext.cont->FindAnimation( searchData.animationName );
	if( ( animContext.animTag && !searchData.generatorData.m_tags.Exist( animContext.animTag ) ) ||
		context.IsAnimationAlreadyUsed( searchData.animationName, &animContext.atChunk, animContext.actorName ) ||
		!anim || anim->GetName() != searchData.animationName )
	{
		return;
	}

	Float animDuration	= anim->GetDuration();
	Float markerPos		= 0.f;
	TDynArray<CExtAnimDialogKeyPoseMarker*> markers;
	anim->GetEventsOfType<CExtAnimDialogKeyPoseMarker>( markers );	
	if ( markers.Empty() &&  animContext.mustHaveMarker )
	{
		return;
	}
	Float speed = GEngine->GetRandomNumberGenerator().Get( searchData.generatorData.m_animSpeedRange.m_first, searchData.generatorData.m_animSpeedRange.m_second );
	Float weight = GEngine->GetRandomNumberGenerator().Get( searchData.generatorData.m_animWeightRange.m_first, searchData.generatorData.m_animWeightRange.m_second );
	for ( Uint32 i = 0; i < markers.Size(); i++)
	{
		Float markerFinalPos = markers[i]->GetStartTime() / speed;
		if( animContext.timeBefore > markerFinalPos && animContext.timeAfter > animDuration / speed - markerFinalPos )
		{
			markerPos = markers[i]->GetStartTime();
			break;
		}			
	}
	if ( markerPos == 0.f && ( animContext.mustHaveMarker || animDuration > animContext.timeAfter ) )
	{
		return;
	}	
	AnimData* animData =  new (animContext.availableAnim) AnimData( searchData.animationName, searchData.eventType, weight, speed, searchData.friendlyName );
	animData->additionalData = searchData.additionalData;
	animData->markerPosition = markerPos;
	animData->animDuration	 = animDuration;
}


Float CStorySceneEventGenerator::FindAnimation( SContext& context, const SPositionInSection & atPos, const SPositionInSection & fromPos, const SPositionInSection & toPos, CName actorName, AnimData& out, Bool mustHaveMarker, Float overlap )
{
	CActor* actor = m_editor->OnEvtGenerator_GetSceneActor( actorName );
	Float animDuration = 1.f;
	if( !actor )
	{
		return animDuration;
	}
	CAnimatedComponent* animatedComponent  = actor->GetRootAnimatedComponent();
	if( !animatedComponent )
	{
		return animDuration; 
	}
	CSkeletalAnimationContainer* cont = animatedComponent->GetAnimationContainer();
	if ( !cont )
	{
		return animDuration;
	}
	const CStorySceneAnimationList& list = GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList();
	
	SStorySceneActorAnimationState actorState = m_config.GetActorState( actorName, context.m_section );

	ESkeletonType sType = ST_Man;
	if( actor->GetEntityTemplate() )
	{
		CAnimGlobalParam* animParam = actor->GetEntityTemplate()->FindParameter< CAnimGlobalParam >( true );
		if( animParam )
		{
			sType = animParam->GetSkeletonType();
		}
	}
	CName tag = GetSentenceInfoAt( context, atPos, actorName );
	TDynArray< AnimData > availableAnim;

	Float timeAvailable = atPos.DistanceTo( toPos ) * ( 1.f + overlap );

	Float timeBefore = fromPos.DistanceTo( atPos );
	Float timeAfter = atPos.DistanceTo( toPos );
	AnimSearchContext searchContext( availableAnim, actorName, atPos.GetChunk(), cont, tag, mustHaveMarker, timeBefore, timeAfter ) ;

	//standard body animations
	for ( CStorySceneAnimationList::BodyAnimationIterator it( list, actorState.m_status, actorState.m_emotionalState, actorState.m_poseType, CStorySceneAnimationList::GESTURE_KEYWORD ); it; ++it )
	{	
		AnimSearchData searchData( *it, AnimData::Animation, actorState, CStorySceneAnimationList::GESTURE_KEYWORD );
		ProcessAnimationEntry( context, searchContext, searchData );
	}

	//universal additives
	CName aAnimKey = CStorySceneAnimationList::ADDITIVE_KEYWORD;
	for ( CStorySceneAnimationList::BodyAnimationIterator it( list, aAnimKey, aAnimKey, aAnimKey, aAnimKey ); it; ++it )
	{
		const CStorySceneAnimationList::AnimationBodyData& data = *it;
		AnimSearchData searchData( data, AnimData::Additive, SStorySceneActorAnimationState( aAnimKey, aAnimKey, aAnimKey ), aAnimKey );
		const TDynArray<SStorySceneActorAnimationState>* statesArray = (sType == ST_Woman) ? &data.m_generatorData.m_useOnlyWithWomanStates : &data.m_generatorData.m_useOnlyWithManStates;
		if(	statesArray->Empty() || statesArray->Exist( actorState ) )
		{
			ProcessAnimationEntry( context, searchContext, searchData );				
		}
	}

	//additional additives
	const CStorySceneAnimationList::TAdditionalAdditivesMap* additivesMap = (sType == ST_Woman) ? &list.GetWomanAdditionalAdditives() : &list.GetManAdditionalAdditives();
	const TPair< SStorySceneActorAnimationState, TDynArray< CStorySceneAnimationList::AnimationBodyData >>* addtitivesArr = additivesMap->FindPtr( actorState );
	if ( addtitivesArr )
	{
		const TDynArray< CStorySceneAnimationList::AnimationBodyData >& arr = addtitivesArr->m_second;
		for ( auto it = arr.Begin(); it != arr.End(); ++it )
		{
			AnimSearchData searchData( *it, AnimData::Additive, addtitivesArr->m_first, CStorySceneAnimationList::GESTURE_KEYWORD );
			ProcessAnimationEntry( context, searchContext, searchData );
		}
	}

	//chose one events from selected 
	const CSkeletalAnimationSetEntry* anim = nullptr;
	if( availableAnim.Size() > 0 ) 
	{
		out = availableAnim[ GEngine->GetRandomNumberGenerator().Get< Uint32 >( availableAnim.Size() ) ];
		return out.animDuration;
	}
	
	out = AnimData();
	return out.animDuration;
}



void CStorySceneEventGenerator::FirstPassAnimationEvents( SContext& context, const CStorySceneLine* line, const TDynArray< TPair< Float, Float > >& maxAmpPos )
{
	const String& currentLocale = SLocalizationManager::GetInstance().GetCurrentLocale();

	TDynArray<SSectionChunk>& chunks = context.m_chunks;

	for ( Uint32 i = 0; i < maxAmpPos.Size(); i++)
	{
		Float lineDuration = line->CalculateDuration( currentLocale );

		String debug = TXT("FP");	
		AnimData animData;	
		CName speaker  = line->GetVoiceTag(), listener = line->GetSpeakingTo();

		SPositionInSection from = SPositionInSection( chunks, 0.f, 0.f );
		SPositionInSection to = SPositionInSection( chunks, 1.f, 0.f );
		SPositionInSection at( chunks, line, maxAmpPos[i].m_first * lineDuration );

		if( IsSpaceAvailable< CStorySceneEventAnimation >( context, at, speaker, from, to ) )
		{
			Float animDuration = FindAnimation( context, at, from, to, speaker, animData, true );
			at -= animData.markerPosition / animData.animSpeed; 
			context.MarkAnimationUsed( animData.animName, &at.GetChunk(), speaker );	
			CreateAnimationEvent(context,speaker,animData,at,debug);
		}

		from = SPositionInSection( chunks, 0.f, 0.f );
		to = SPositionInSection( chunks, 1.f, 0.f );

		if ( IsSpaceAvailable< CStorySceneEventAnimation >( context, at, listener, from, to ) )
		{
			Float animDuration = FindAnimation( context, at, from, to, listener, animData, true );
			at -= animData.markerPosition / animData.animSpeed; 
			context.MarkAnimationUsed( animData.animName, &at.GetChunk(), listener );	
			CreateAnimationEvent(context,listener,animData,at,debug);
		}
	}
}


void CStorySceneEventGenerator::GenerateAnimationEvents( SContext& context )
{
	TDynArray<SSectionChunk>& chunks = context.m_chunks;
	CStorySceneSection* section = context.m_section;
	Uint32 nr = section->GetNumberOfElements();
	if ( nr == 0 || chunks.Empty() ) //empty section ignore
	{
		return;
	}

	MarkExistingAsUsed<CStorySceneEventAnimClip>( context );

	//first pass animations snapped to markers
	for ( Uint32 i = 0; i < nr; ++i )
	{
		CStorySceneElement* element = context.m_section->GetElement( i );
		CStorySceneLine*	line	= Cast<CStorySceneLine>( element );

		TDynArray< TPair< Float, Float > > data;
		if( line && m_editor->OnEvtGenerator_GetVoiceDataPositions( line, &data ) )
		{
			FirstPassAnimationEvents( context, line, data );
		}
	}

	//fill empty spaces with other animations

	TDynArray<CName> actorsToGenerate = m_config.GetActorsToProcess();
	for ( Uint32 j = 0; j < actorsToGenerate.Size(); ++j )
	{
		CName actorName = actorsToGenerate[j];
		if( !actorName )
		{
			continue;
		}

		Float overlap = 0.0675f;
		TDynArray< TPair<SPositionInSection,SPositionInSection> > emptySpaces;
		CollectEmptySpaces< CStorySceneEventAnimation >( context, actorName, emptySpaces );

		for ( Uint32 i = 0; i < emptySpaces.Size(); ++i )
		{
			String debug = TXT("SP");	;
			SPositionInSection from = emptySpaces[i].m_first;
			SPositionInSection to	= emptySpaces[i].m_second;

			while( to > from )
			{
				if ( actorName != from.GetChunk().Speaker() )
				{
					from += Min<Float>( GEngine->GetRandomNumberGenerator().Get< Float >( 3.5f , 8.5f ), from.ToEndOfChunk() + 0.001f );
				}
				if ( to > from )
				{
					AnimData animData;	
					Float animDuration = FindAnimation( context, from, from, to, actorName, animData, false, overlap );
					context.MarkAnimationUsed( animData.animName, &from.GetChunk(), actorName );	
					from -= animDuration * overlap;
					CreateAnimationEvent(context,actorName,animData,from,debug);
					from += animDuration * (1.f - 2.f*overlap);
					context.MarkAnimationUsed( animData.animName, &from.GetChunk(), actorName );	
				}
			}
		}
	}
}

