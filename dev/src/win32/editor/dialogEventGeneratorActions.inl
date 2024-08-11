#include "..\..\common\game\storySceneEventCustomCameraInstance.h"
#include "..\..\common\game\storySceneDialogset.h"
#include "..\..\common\game\storySceneSection.h"
#include "..\..\common\game\storyScene.h"
#include "..\..\common\game\storySceneEventAnimation.h"
#include "..\..\common\game\storySceneAnimationParams.h"


#include "dialogTimeline.h"
#include "dialogEventGenerator.h"

namespace CStorySceneEventGeneratorInternals
{
	/*
	struct EventPositionProvider
	{
		virtual Float ProvideEventPosition( const CStorySceneElement* element ) const = 0;
	};

	struct StaticEventPositionProvider : public EventPositionProvider
	{
		Float	m_position;
		Float	m_allowedOffset;

		StaticEventPositionProvider( Float position, Float allowedOffset ) : m_position( position ), m_allowedOffset( allowedOffset ) {}

		virtual Float ProvideEventPosition( const CStorySceneElement* element ) const
		{
			return Clamp( FRand( m_position, m_position + m_allowedOffset ), 0.0f, 1.0f );
		}
	};

	struct AtTimeEventPositionProvider : public EventPositionProvider
	{
		Float	m_time;
		Float	m_timeOffset;

		AtTimeEventPositionProvider( Float time, Float timeOffset ) : m_time( time ), m_timeOffset( timeOffset ) {}

		virtual Float ProvideEventPosition( const CStorySceneElement* element ) const
		{
			Float elementLength = element->CalculateDuration();
			Float suggestedPosition = m_time / elementLength;
			Float positionOffset = m_timeOffset / elementLength;
			return Clamp( FRand( suggestedPosition, suggestedPosition + positionOffset ), 0.0f, 1.0f );
		}
	};

	//////////////////////////////////////////////////////////////////////////

	struct CreateCameraAction : public Action
	{
		EventPositionProvider*	m_positionProvider;

		CreateCameraAction( EventPositionProvider* positionProvider )
			: Action( EVENT_Camera )
			, m_positionProvider( positionProvider )
		{
		}

		virtual ~CreateCameraAction()
		{
			delete m_positionProvider;
		}

		CName SelectCameraName( const CName& targetName, const CName& lastCameraName, const Context& context ) const
		{
			const TDynArray< StorySceneCameraDefinition > &  cameraDefinitions =  context.GetSection()->GetScene()->GetCameraDefinitions();

			TDynArray<const StorySceneCameraDefinition*> suitableCameras;
			for( TDynArray<StorySceneCameraDefinition>::const_iterator camIter = cameraDefinitions.Begin();
				camIter != cameraDefinitions.End(); ++camIter )
			{
				if ( context.m_placeholderMode == true && ( targetName == CName::NONE || camIter->m_targetSlotName == targetName) )
				{
					return camIter->m_cameraName;
				}

				Bool canUseCamera = camIter->m_genParam.m_usableForGenerator == true;
				canUseCamera &= ( targetName == CName::NONE || camIter->m_targetSlotName == targetName );
				canUseCamera &= ( lastCameraName == CName::NONE || camIter->m_cameraName != lastCameraName );

				if( canUseCamera == true )
				{
					suitableCameras.PushBack( &(*camIter) );
				}
			}

			CName cameraName;
			if( suitableCameras.Empty() == false )
			{
				cameraName = suitableCameras[ IRand( 0, suitableCameras.Size() ) ]->m_cameraName;
			}

			

			return cameraName;
		}

		CName GetLineSpeakerSlot( const Context& context ) const
		{
			CName slotName = CName::NONE;

			const CStorySceneDialogsetInstance* dialogSet = context.GetSection()->GetDialogsetInstance();
			if( dialogSet != NULL )
			{
				const CStorySceneDialogsetSlot* dialogSetSlot = dialogSet->GetSlotByActorName( context.m_currentSpeakingActor );
				if( dialogSetSlot != NULL )
				{
					slotName = dialogSetSlot->GetSlotName();
				}
			}

			return slotName;
		}


		virtual void Perform( Context& context ) const
		{
			CName targetName = GetLineSpeakerSlot( context );
			CName cameraName = SelectCameraName( targetName, CName::NONE, context );
			Float eventPosition = m_positionProvider->ProvideEventPosition( context.GetElement() );

			if ( cameraName != CName::NONE )
			{
				CStorySceneEventCustomCameraInstance* cameraEvent = new CStorySceneEventCustomCameraInstance(String::EMPTY, context.GetElement(), eventPosition, CEdDialogTimeline::TIMELINE_DEFAULT_CAMERA_TRACK );
				cameraEvent->SetCustomCameraName(cameraName);

				context.GetSection()->AddEvent( cameraEvent );
			}
		}
	};

	//////////////////////////////////////////////////////////////////////////

	struct CreateGestureAction : public Action
	{
		EventPositionProvider*	m_positionProvider;

		CreateGestureAction( EventPositionProvider* positionProvider )
			: Action( EVENT_Gesture )
			, m_positionProvider( positionProvider )
		{
		}

		virtual ~CreateGestureAction()
		{
			delete m_positionProvider;
		}

		virtual void Perform( Context& context ) const
		{
			Float eventPosition = m_positionProvider->ProvideEventPosition( context.GetElement() );
			CName actorName = context.m_currentSpeakingActor;
			String trackName = String::Printf( TXT( "%s.animations" ), actorName.AsString().AsChar() );

			CActor* actor = context.GetScenePlayer()->GetSceneActor< CActor >( actorName, false );

			if ( actor == NULL )
			{
				return;
			}

			CAnimatedComponent* animatedComponent = actor->GetRootAnimatedComponent();
			if ( animatedComponent == NULL )
			{
				return;
			}

			CName actorPose = context.GetScenePlayer()->GetCurrentPoseNameForActor( actorName );

			TDynArray< CName >	candidateAnimations;
			
			for ( ComponentAnimsetIterator animsetIter( animatedComponent, AST_Dialog ); animsetIter; ++animsetIter )
			{
				const TDynArray< CSkeletalAnimationSetEntry* >& animationEntries = (*animsetIter)->GetAnimations();

				for ( TDynArray< CSkeletalAnimationSetEntry* >::const_iterator entryIter = animationEntries.Begin();
					entryIter != animationEntries.End(); ++entryIter )
				{
					const CSkeletalAnimationSetEntry* animationEntry = *entryIter;
					if ( animationEntry->GetAnimation() == NULL )
					{
						continue;
					}

					for ( CSkeletalAnimationSetEntry::ParamsIterator< CStorySceneGestureAnimationParam > paramIter( animationEntry );
						paramIter; ++paramIter )
					{
						const CStorySceneGestureAnimationParam* gestureAnimParam = *paramIter;
						
						if ( gestureAnimParam->GetEmotionalState() == actorPose )
						{
							candidateAnimations.PushBack( animationEntry->GetName() );
						}
					}
				}
			}

			if ( candidateAnimations.Empty() )
			{
				return;
			}

			CName animationName = candidateAnimations[ IRand( 0, candidateAnimations.Size() ) ];

			CStorySceneEventAnimation* animationEvent = new CStorySceneEventAnimation( String::EMPTY, context.GetElement(), eventPosition, actorName, animationName.AsString(), trackName );

			animationEvent->SetAnimationClipEnd( 10000.0f );
			animationEvent->RefreshDuration( context.GetScenePlayer() );

			context.GetSection()->AddEvent( animationEvent );

		}
	};
	*/
}