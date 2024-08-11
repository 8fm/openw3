#include "build.h"
#include "dialogEventGenerator.h"

#include "dialogEventGeneratorInternals.h"
#include "dialogEventGeneratorSetupDialog.h"

#include "dialogTimeline.h"
#include "..\..\common\game\storySceneSection.h"
#include "..\..\common\game\storySceneEvent.h"
#include "..\..\common\game\storySceneElement.h"
#include "..\..\common\game\storyScene.h"
#include "..\..\common\game\storySceneEventCustomCameraInstance.h"
#include "..\..\common\game\storySceneEventCustomCamera.h"
#include "..\..\common\game\storySceneLine.h"
#include "..\..\common\game\storyScenePauseElement.h"
#include "..\..\common\game\storySceneChoice.h"
#include "..\..\common\game\storySceneEventAnimation.h"
#include "..\..\common\game\storySceneEventCameraBlend.h"
#include "..\..\common\game\storySceneEventLookAt.h"
#include "..\..\common/game/storySceneEventEnhancedCameraBlend.h"
#include "../../common/game/storySceneControlPartsUtil.h"
#include "../../common/game/storySceneEventCameraInterpolation.h"
#include "../../common/engine/localizationManager.h"

RED_DEFINE_STATIC_NAME(zero);

RED_DEFINE_STATIC_NAME(ext);
RED_DEFINE_STATIC_NAME(int);
RED_DEFINE_STATIC_NAME(noBlend);
RED_DEFINE_STATIC_NAME(blend);
RED_DEFINE_STATIC_NAME(blendIn);
RED_DEFINE_STATIC_NAME( resetAxis );
RED_DEFINE_STATIC_NAME(forceCloseup);
RED_DEFINE_STATIC_NAME(onListener);
RED_DEFINE_STATIC_NAME(firstInLine );
RED_DEFINE_STATIC_NAME(multipleInLine)
RED_DEFINE_STATIC_NAME(forceInt)

void CStorySceneEventGenerator::GenerateCameraForChunk( SContext& context )
{
	SPositionInChunk* posMarker;
	TDynArray<SPositionInChunk> positions;

	Float length = context.m_currentChunk->Length();
	Bool singleCamera = context.FirstChunkInSection() || context.m_currentChunk->IsChoice();

	TDynArray<const CStorySceneEvent*> events;
	context.m_currentChunk->GetEvents( events );
	for ( Uint32 i = 0 ; i < events.Size(); i++ )
	{
		if( Cast<const CStorySceneEventCustomCamera>( events[i] ) || Cast<const CStorySceneEventCustomCameraInstance>( events[i] ) || Cast<const CStorySceneEventCameraBlend> ( events[i] ) || Cast<const CStorySceneEventEnhancedCameraBlend> ( events[i] ) )
		{
			return;
		}
	}

	posMarker = new (positions) SPositionInChunk( singleCamera ? 0.f : 0.02 );
	posMarker->tags.AddTag( CNAME( firstInLine ) );

	if( context.m_currentChunk->IsChoice() )
	{
		posMarker->tags.AddTag( CNAME( forceInt ) );
	}

	if( length > 8 && length < 15  && !singleCamera )
	{
		if( GEngine->GetRandomNumberGenerator().Get< Uint16 >( 2 ) && !( context.FirstSectionInScene() && context.FirstChunkInSection() ) )
		{			
			posMarker->durationRel = 0.9f;
			posMarker->tags.AddTag( CNAME( blend ) );
		}
		else
		{
			posMarker =  new (positions) SPositionInChunk( 0.66f );
			posMarker->durationRel = 0.33f;
		}
	}

	if( length > 15 && !singleCamera )
	{
		if( !GEngine->GetRandomNumberGenerator().Get< Uint16 >( 3 ) )
		{				
			posMarker->tags.AddTag( CNAME( blend ) );
			//posMarker->tagsForCamera.AddTag( CNAME( forceCloseup ) );
			posMarker->durationRel = 0.45f;
			posMarker = new (positions) SPositionInChunk( 0.50f );

		}
		else if( !GEngine->GetRandomNumberGenerator().Get< Uint16 >( 2 ) )
		{					
			posMarker = new (positions) SPositionInChunk( 0.50f );
			posMarker->tags.AddTag( CNAME( blend ) );
			//posMarker->tagsForCamera.AddTag( CNAME( forceCloseup ) );
			posMarker->durationRel = 0.45f;

		}
		else
		{
			posMarker = new (positions) SPositionInChunk( 0.40f );
			posMarker->tags.AddTag( CNAME( forceCloseup ) );
			posMarker->tags.AddTag( CNAME( onListener ) );		
			posMarker->durationAbs = 2.f;
			posMarker = new (positions) SPositionInChunk( 0.40f, 2.f );

			posMarker->tags.AddTag( CNAME( forceCloseup ) );			
			posMarker->tags.AddTag( CNAME( blend ) );
			posMarker->durationRel = 0.45f;
		}
	}


	Uint32 size = positions.Size();
	for( Uint32 i = 0; i < size; ++i )
	{
		if( size > 1 )
		{
			positions[i].tags.AddTag( CNAME( multipleInLine ) );
		}
		if( positions[i].tags.HasTag( CNAME( blend )  ) && GEngine->GetRandomNumberGenerator().Get< Uint16 >( 2 ) )
		{
			positions[i].tags.AddTag( CNAME( blendIn ) );
		}		
	}	

	for( Uint32 i = 0; i < size; ++i )
	{
		context.m_nextCameraPositionTags = ( i+1 < size )		?  positions[i+1].tags : TagList::EMPTY;
		context.m_prevCameraPositionTags = ( (Int32)i-1 >= 0 )	?  positions[i-1].tags : TagList::EMPTY; 

		String	combinedDebugString;
		CameraData chosenCam;

		if( FindCameraForPosition( context, positions[i], chosenCam, combinedDebugString ) < 3  //|| posMarker.durationAbs + posMarker.durationRel*length < 3.f 
				|| length - positions[i].PosAbs( *context.m_currentChunk ) < 3.f
				)
		{
			CreateCameraEvent( context, chosenCam, positions[i], combinedDebugString );
		}
		else
		{
			CameraData tempCam = chosenCam;
			positions[i].tags.AddTag( CNAME( onListener )  );
			if( FindCameraForPosition( context, positions[i], chosenCam, combinedDebugString ) < 3   )
			{
				CreateCameraEvent( context, chosenCam, positions[i], combinedDebugString );
				positions[i].valAbs += 2.f; // 2 seconds later
				positions[i].tags.SubtractTag( CNAME( onListener )  );
				FindCameraForPosition( context, positions[i], chosenCam, combinedDebugString );
				CreateCameraEvent( context, chosenCam, positions[i], combinedDebugString );
			}
			else
			{
				//reversed failed as well take whatever we had for the first time
				CreateCameraEvent( context, tempCam, positions[i], combinedDebugString );
			}
		}

	}		
}

void CStorySceneEventGenerator::CreateCameraEvent(  SContext& context, const CameraData& chosenCamData, const SPositionInChunk & cameraPosition, const String& combinedDebugString )
{
	const StorySceneCameraDefinition* chosenCam = chosenCamData.carriedData;
	CStorySceneElement* element = NULL;
	Float pos = 0.f;
	GetEventPosition( context.m_currentChunk, cameraPosition , element, pos );

	CStorySceneEventCustomCameraInstance* cameraEvent = nullptr;
	if( chosenCam && element)
	{			
		const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
		const CStorySceneSectionVariantId sectionVariantId = context.m_section->GetVariantUsedByLocale( currentLocaleId );

		context.ProcessCamera( chosenCam );		
		CStorySceneEventCustomCameraInstance* cameraEvent = new CStorySceneEventCustomCameraInstance( String::EMPTY, element, pos, CEdDialogTimeline::TIMELINE_DEFAULT_CAMERA_TRACK );
		cameraEvent->SetCustomCameraName( chosenCam->m_cameraName );
		cameraEvent->SetDebugString( String( combinedDebugString ) );
		context.m_section->AddEvent( cameraEvent, sectionVariantId );

		if( cameraPosition.tags.HasTag( CNAME( blend ) ) )
		{ 
			context.ProcessCamera( chosenCam );		
			const StorySceneCameraDefinition* secCam = Get2ndCameraForBlend( context, cameraPosition, *chosenCam );		
			if( secCam )
			{
				context.ProcessCamera( secCam );
				SPositionInChunk  secCameraPosition = cameraPosition;
				secCameraPosition.valAbs += cameraPosition.durationAbs;
				secCameraPosition.valRel += cameraPosition.durationRel;
				GetEventPosition( context.m_currentChunk, secCameraPosition , element, pos );			
				CStorySceneEventCustomCamera* secCameraEvent = new CStorySceneEventCustomCamera( String::EMPTY, element, pos, *secCam, CEdDialogTimeline::TIMELINE_DEFAULT_CAMERA_TRACK );
				context.m_section->AddEvent( secCameraEvent, sectionVariantId );

				CStorySceneEventInterpolation::CreationArgs creationArgs;
				creationArgs.m_eventName = TXT( "interpolation event" );
				creationArgs.m_keyEvents.PushBack( cameraEvent );
				creationArgs.m_keyEvents.PushBack( secCameraEvent );		

				CStorySceneEventInterpolation* scInterpolationEvent =  new CStorySceneEventCameraInterpolation;
				scInterpolationEvent->Initialize( creationArgs );
				context.m_section->AddEvent( scInterpolationEvent, sectionVariantId );
			}		
		}
	}
}

Bool CStorySceneEventGenerator::ActionAxisRule( SContext& context, const StorySceneCameraDefinition & camera ) const
{
	Bool result = true;

	//PointIsLeftOfSegment2D
	const CStorySceneDialogsetInstance* dialogset = context.m_section->GetScene()->GetFirstDialogsetAtSection( context.m_section );
	const StorySceneCameraDefinition* lastCamera = context.GetCameraFromHistory(0);	
	if( context.m_currentChunk && dialogset && lastCamera )
	{
		const CStorySceneDialogsetSlot* sourceSlot = dialogset->GetSlotByActorName( context.m_currentChunk->Speaker() );
		const CStorySceneDialogsetSlot* targetSlot = dialogset->GetSlotByActorName( context.m_currentChunk->SpeakingTo() );

		if( sourceSlot && targetSlot && sourceSlot != targetSlot )
		{
			const Plane ActionAxis( targetSlot->GetSlotPlacement().GetPosition(), targetSlot->GetSlotPlacement().GetPosition() + Vector(0.f,0.f,1.f), sourceSlot->GetSlotPlacement().GetPosition() );

			result &= ActionAxis.GetSide( lastCamera->m_cameraTransform.GetPosition() ) == ActionAxis.GetSide( camera.m_cameraTransform.GetPosition() );
		}		
	}
	if( context.m_lastChunk && dialogset && lastCamera )
	{
		const CStorySceneDialogsetSlot* sourceSlot = dialogset->GetSlotByActorName( context.m_lastChunk->Speaker() );
		const CStorySceneDialogsetSlot* targetSlot = dialogset->GetSlotByActorName( context.m_lastChunk->SpeakingTo() );
		if( sourceSlot && targetSlot && sourceSlot != targetSlot )
		{
			const Plane ActionAxis( targetSlot->GetSlotPlacement().GetPosition(), targetSlot->GetSlotPlacement().GetPosition() + Vector(0.f,0.f,1.f), sourceSlot->GetSlotPlacement().GetPosition() );
			result &= ActionAxis.GetSide( lastCamera->m_cameraTransform.GetPosition() ) == ActionAxis.GetSide( camera.m_cameraTransform.GetPosition() );
		}
	}
	return result;
}

Bool CStorySceneEventGenerator::CanCameraBeBlended( SContext& context, const SPositionInChunk& posMarker, const StorySceneCameraDefinition& firstCamera )
{
	return true;
	//return Get2ndCameraForBlend(context, posMarker, firstCamera) != NULL;
}

const StorySceneCameraDefinition* CStorySceneEventGenerator::Get2ndCameraForBlend( SContext& context, const SPositionInChunk& posMarker, const StorySceneCameraDefinition& firstCamera )
{
	Int32 sideSign;
	const CStorySceneDialogsetInstance* dialogset = context.m_section->GetScene()->GetFirstDialogsetAtSection( context.m_section );
	if( context.m_currentChunk && dialogset )
	{
		const CStorySceneDialogsetSlot* sourceSlot = dialogset->GetSlotByActorName( context.m_currentChunk->Speaker() );
		const CStorySceneDialogsetSlot* targetSlot = dialogset->GetSlotByActorName( context.m_currentChunk->SpeakingTo() );

		if( sourceSlot && targetSlot && sourceSlot != targetSlot )
		{
			const Plane ActionAxis( targetSlot->GetSlotPlacement().GetPosition(), targetSlot->GetSlotPlacement().GetPosition() + Vector(0.f,0.f,1.f), sourceSlot->GetSlotPlacement().GetPosition() );
			sideSign = ActionAxis.GetSide( firstCamera.m_cameraTransform.GetPosition() ) == Plane::PS_Front ? -1 : 1;
		}		
	}


	context.m_currentChunk->Speaker();
	context.m_currentChunk->SpeakingTo();

	Matrix headPlacementWS;

	CName targetName = posMarker.tags.HasTag( CNAME( onListener ) ) ? context.m_currentChunk->SpeakingTo() : context.m_currentChunk->Speaker();
	CActor * actor = m_editor->OnEvtGenerator_GetSceneActor( targetName );

	if( !actor )
	{
		return &firstCamera;
	}
	Int32 head = actor->GetHeadBone();
	CAnimatedComponent * ac = actor->GetRootAnimatedComponent();

	if( head == -1 || !ac )
	{
		return &firstCamera;
	}

	headPlacementWS = ac->GetBoneMatrixWorldSpace( head );

	StorySceneCameraDefinition* cameraDef = ::new (context.m_generatedBlendCameras) StorySceneCameraDefinition( firstCamera );	
	cameraDef->m_cameraName = CName( cameraDef->m_cameraName.AsString() + TXT("_blend") );
	Matrix camera( Matrix::IDENTITY );
	cameraDef->m_cameraTransform.CalcLocalToWorld( camera );	
	
	EngineTransform targetBoneTransform;
	const EngineTransform scenePlacement = m_editor->OnEvtGenerator_GetCurrentScenePlacement();
	if ( !scenePlacement.IsIdentity() )
	{
		Matrix scenePlacementWorldToLocal;
		scenePlacement.CalcWorldToLocal( scenePlacementWorldToLocal );

		const Matrix headPlacementLS = headPlacementWS * scenePlacementWorldToLocal;

		targetBoneTransform = EngineTransform( headPlacementLS );
	}
	else
	{
		targetBoneTransform = EngineTransform( headPlacementWS );
	}

	Vector	headPos	=	targetBoneTransform.GetPosition();	

	//Matrix	translation( Matrix::IDENTITY );
	//translation.SetTranslation( headPos );
	//translation.SetRotZ33( -firstCamera.m_cameraTransform.GetRotation().Yaw );
	//Matrix	rotation( Matrix::IDENTITY );
	//EulerAngles rot( 0.f, -0.f, sideSign*( GEngine->GetRandomNumberGenerator().Get< Float >( 4.0f , 9.0f ) ) );		
	//rotation = rot.ToMatrix();
	//camera = camera * translation.Inverted() * rotation * translation;

	Bool blendIn = posMarker.tags.HasTag( CNAME( blendIn ) );
	Vector	camPos = camera.GetTranslationRef();
	Vector	cam2head =	headPos - camPos; 
	Float	weight =	m_config.m_blendIn * ( blendIn ? 1.f : -1.f );
	Vector  displacement = cam2head * weight;
	Vector	newCamPos =	camPos	+ displacement;
	Float	distance  =  displacement.Mag3() * ( blendIn ? 1.f : -1.f );

	cameraDef->m_dofFocusDistNear -= distance;
	cameraDef->m_dofBlurDistNear -= distance;
	cameraDef->m_cameraTransform.SetPosition( newCamPos );

	//Int32 plane = cameraDef->m_genParam.m_cameraPlane + ( blendIn ? 1.f : -1.f );
	//cameraDef->m_genParam.m_cameraPlane = (ECameraPlane)plane;

	//EulerAngles finalRot = camera.ToEulerAngles();
	//cameraDef->m_cameraTransform.SetRotation( finalRot );

	return cameraDef;
}

Int32 CStorySceneEventGenerator::FindCameraForPosition(  SContext& context, const SPositionInChunk & cameraPosition, CameraData & out, String& combinedDebugString )
{
	const TDynArray< StorySceneCameraDefinition > &				cameraDefinitions =  context.m_section->GetScene()->GetCameraDefinitions();
	TDynArray< CameraData >	primaryCameras; 
	TDynArray< CameraData >	secondaryCameras;
	CameraData					emergencyCamera( NULL, 0, String(), NumericLimits<Int32>::Max() );

	const StorySceneCameraDefinition* lastLineCamera = context.LastLineCamera();
	CName speaker		= context.m_currentChunk->Speaker();
	CName speakingTo	= context.m_currentChunk->SpeakingTo();

	if( cameraPosition.tags.HasTag( CNAME( firstInLine ) ) && context.FirstChunkInSection() )
	{
		TDynArray<const CStorySceneSection*> sections;
		StorySceneControlPartUtils::GetPrevElementsOfType( context.m_section, sections );
		for ( Uint32 j = 0; j < sections.Size(); j++ )
		{
			if ( sections[j]->GetChoice() )
			{
				// get variant id that this sections uses when played in current locale
				const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
				const CStorySceneSectionVariantId sectionVariantId = sections[j]->GetVariantUsedByLocale( currentLocaleId );

				const TDynArray< CGUID >& evGuids = sections[ j ]->GetEvents( sectionVariantId );
				for ( Int32 i = evGuids.SizeInt() - 1; i >= 0 ; --i )
				{
					const CStorySceneEvent* event = sections[ j ]->GetEvent( evGuids[ i ] );
					if ( event->GetSceneElement() == sections[ j ]->GetChoice() )
					{
						const CStorySceneEventCustomCameraInstance* camera =  Cast< const CStorySceneEventCustomCameraInstance >( event );
						if ( camera )
						{					
							combinedDebugString = TXT("Section after choice section - keep last camera");
							out = CameraData( context.m_section->GetScene()->GetCameraDefinition( camera->GetCustomCameraName() ) , 1.f, combinedDebugString, 0 ) ;
							return out.quality;
						}
					}
				}
			}			
		}
	}

	if( context.m_currentChunk->IsChoice() )
	{
		speaker		=  m_config.m_playerInScene;
		if( context.m_lastChunk )
		{
			speakingTo	=  context.m_lastChunk->Speaker() == speaker ? context.m_lastChunk->SpeakingTo() : context.m_lastChunk->Speaker();
		}
		else
		{
			const CStorySceneLine* line = m_editor->OnEvtGenerator_GetPrevLine( context.m_currentChunk->m_elements[0] );
			if( line )
			{
				speakingTo	=  line->GetVoiceTag() == speaker ?  line->GetSpeakingTo() : line->GetVoiceTag();
			}	
		}
	}

	const CStorySceneDialogsetInstance* dialogset = context.m_section->GetScene()->GetFirstDialogsetAtSection( context.m_section );
	if( !dialogset )
	{

		out = CameraData( nullptr , 0.f, String( TXT("Could not find dialogset for section: ") + context.m_section->GetName() ) , NumericLimits<Int32>::Max() );
		return NumericLimits<Int32>::Max();
	}

	if( cameraPosition.tags.HasTag( CNAME( onListener ) ) )
	{
		Swap<CName>( speaker, speakingTo );
	}

	Bool firstActorOccurence = context.FirstActorOccurence( true );
	Bool firstSectionInScene = context.FirstSectionInScene();
	Bool firstChunkInSection = context.FirstChunkInSection();

	for( Uint32 defIndex = 0; defIndex < cameraDefinitions.Size() ; ++defIndex  )
	{
		const StorySceneCameraDefinition  & camera = cameraDefinitions[defIndex];	
		String debugString;			
		Int32 cameraQuality = 0;
		Float camaraWeight = 1.f;

		CStorySceneDialogsetSlot*  srcSlot = dialogset->GetSlotBySlotNumber( camera.m_genParam.m_targetSlot );
		CStorySceneDialogsetSlot*  dstSlot = dialogset->GetSlotBySlotNumber( camera.m_genParam.m_sourceSlot );
		if( camera.m_genParam.m_targetSlot == 0 && camera.m_genParam.m_sourceSlot == 0 )
		{
			StorySceneCameraDefinition tempCam = camera;
			tempCam.ParseCameraParams();
			srcSlot = dialogset->GetSlotBySlotNumber( tempCam.m_genParam.m_targetSlot );
			dstSlot = dialogset->GetSlotBySlotNumber( tempCam.m_genParam.m_sourceSlot );
		}
		CName cameraSourceActor =  srcSlot ? srcSlot->GetActorName() : CName::NONE;
		CName cameraTargetActor =  dstSlot ? dstSlot->GetActorName() : CName::NONE;

		debugString += TXT("CameraName= ") + camera.m_cameraName.AsString() + TXT("  ");
		debugString += TXT("CameraTags=")   + camera.m_genParam.m_tags.ToString() + TXT("  ");

		if ( cameraSourceActor == CName::NONE && cameraTargetActor == CName::NONE && camera.m_genParam.m_cameraPlane == CP_None )
		{
			debugString += TXT(" Camera from old dialogset - ignoring \n");
			continue;
		}
		if( !camera.m_genParam.m_usableForGenerator )
		{
			debugString += TXT("CheckedNotToUse__");
			cameraQuality+=2;
		}

		//speaker - target
		if( cameraSourceActor != speaker )
		{
			cameraQuality += 4;
			debugString += TXT("SpeakerDoesntMatch__");
		}
		if( speakingTo != CName::NONE && cameraTargetActor != CName::NONE && cameraTargetActor != speakingTo )
		{
			debugString += TXT("SpeechTargetDoesntMatch__");
			cameraQuality += 4;
		}

		if( !cameraPosition.tags.HasTag( CNAME( resetAxis ) ) && !camera.m_genParam.m_tags.HasTag( CNAME( zero ) ) && !ActionAxisRule( context, camera ) )
		{
			debugString += TXT("ActionAxisRuleFailed__");
			cameraQuality = cameraQuality+3;
		}

		//super closeup camera rules
		if( (!firstActorOccurence && context.m_currentChunk->Length() < 2 && !context.m_currentChunk->IsChoice() )
			// || ( context.m_currentChunk->Length() > 15 && !cameraPosition.tagsForCamera.HasTag( CNAME( firstInLine ) ) )
				)
		{
			if( camera.m_genParam.m_cameraPlane == CP_Supercloseup )
			{
				camaraWeight += NumericLimits<Float>::Infinity();
			}
		}
		else if( camera.m_genParam.m_cameraPlane == CP_Supercloseup && !context.m_prevCameraPositionTags.HasTag( CNAME( blendIn ) ))
		{
			debugString += TXT("DontUseSupercloseup__");
			cameraQuality++;
		}

		//last camera rules
		if( lastLineCamera )
		{
			Int32 distance = Abs<Int32>( camera.m_genParam.m_cameraPlane - lastLineCamera->m_genParam.m_cameraPlane );
			if( ( !cameraPosition.tags.HasTag( CNAME( firstInLine ) ) || ( lastLineCamera->m_genParam.m_sourceSlot == camera.m_genParam.m_sourceSlot && lastLineCamera->m_genParam.m_targetSlot == camera.m_genParam.m_targetSlot ) )
				&& ( distance < 2 ) && !cameraPosition.tags.HasTag( CNAME( onListener ) ) 
				 )
			{
				debugString += TXT("2ndCameraLessThan2Planes__");
				cameraQuality+=2;
			}
			if( lastLineCamera->m_cameraName == camera.m_cameraName )
			{
				debugString += TXT("TheSameAsLastLine__");
				cameraQuality = cameraQuality+3;
			}
		}

		//wide camera rules
		if( ( firstSectionInScene && firstChunkInSection ) ) // || context.FirstActorOccurence()
		{
			if( !lastLineCamera || lastLineCamera->m_genParam.m_cameraPlane != CP_Wide )
			{
				if( camera.m_genParam.m_cameraPlane == CP_Wide )
				{
					camaraWeight += NumericLimits<Float>::Infinity();
				}
			}
			else if( lastLineCamera && lastLineCamera->m_genParam.m_cameraPlane == CP_Wide && camera.m_genParam.m_cameraPlane == CP_Medium )
			{
				camaraWeight += NumericLimits<Float>::Infinity();
			}
		}	
		else if( camera.m_genParam.m_cameraPlane == CP_Wide )
		{
			debugString += TXT("DontUseWideCamera__");
			cameraQuality+=3;
		}

		//blend rules
		if( cameraPosition.tags.HasTag( CNAME( blend ) ) )
		{
			if( !CanCameraBeBlended( context,cameraPosition , camera ) )
			{
				cameraQuality += 2;
				debugString += TXT("NoBlendFromThisCamera__");
			}
		}
		else if( camera.m_genParam.m_tags.HasTag( CNAME( blend ) ) )
		{
			cameraQuality += 2;
			debugString += TXT("BlendCameraInNormalShot__");
		}


		//blend int-ext rules
		CName favoredTag;

		if(  cameraPosition.tags.HasTag( CNAME( forceInt ) ) )
		{
			favoredTag = CNAME(int);
		}

		if( context.m_nextCameraPositionTags.HasTag( CNAME( blend ) ) && cameraPosition.tags.HasTag( CNAME( firstInLine ) )) 
		{
			favoredTag = context.m_nextCameraPositionTags.HasTag( CNAME( blendIn ) ) ? CNAME(ext) : CNAME(int);

			if( ( favoredTag == CNAME(int) && camera.m_genParam.m_cameraPlane != CP_Closeup  ) ||
				( favoredTag == CNAME(ext) && camera.m_genParam.m_cameraPlane != CP_Semicloseup && camera.m_genParam.m_cameraPlane != CP_Medium ) )
			{
				debugString += TXT("PreparingShotForBlend__");
				cameraQuality += 2;
			}
		}
		else if( cameraPosition.tags.HasTag( CNAME( blend ) ) )
		{
			if( cameraPosition.tags.HasTag( CNAME( firstInLine ) ) ) 
			{
				favoredTag = cameraPosition.tags.HasTag( CNAME( blendIn ) ) ? CNAME(ext) : CNAME(int);	

				if( ( favoredTag == CNAME(int) && camera.m_genParam.m_cameraPlane != CP_Closeup  ) ||
					( favoredTag == CNAME(ext) && camera.m_genParam.m_cameraPlane != CP_Semicloseup && camera.m_genParam.m_cameraPlane != CP_Medium ) )
				{
					debugString += TXT("PreparingShotForBlend__");
					cameraQuality += 2;
				}
			}			
			else
			{
				favoredTag = cameraPosition.tags.HasTag( CNAME( blendIn ) ) ? CNAME(int) : CNAME(ext);
			}	
		}
		else if( context.m_prevCameraPositionTags.HasTag( CNAME( blend ) ) && context.m_prevCameraPositionTags.HasTag( CNAME( firstInLine ) ) ) 
		{
			favoredTag = context.m_prevCameraPositionTags.HasTag( CNAME( blendIn ) ) ? CNAME(int) : CNAME(ext);
		}

		if( favoredTag )
		{
			if( !camera.m_genParam.m_tags.HasTag( favoredTag ) )
			{
				cameraQuality +=2;
			}
		}
		else if( !camera.m_genParam.m_tags.HasTag( CNAME( ext ) ) )
		{
			camaraWeight = 0.5f;
			if( firstActorOccurence )
			{
				debugString += TXT("FirstRelationOccurence-NoExtTag__");
				cameraQuality++;
			}
		}

		// dont pick semicloseup as 1st in multiple
		if( camera.m_genParam.m_tags.HasTag( CNAME(multipleInLine) ) && camera.m_genParam.m_tags.HasTag( CNAME( firstInLine ) ) 
			&& camera.m_genParam.m_cameraPlane == CP_Semicloseup )
		{
			debugString += TXT("1stOfMultipleSemicloseup__");
			cameraQuality += 2;
		}


		//other tags rules
		if( camera.m_genParam.m_tags.HasTag( CNAME(B) ) )
		{
			debugString += TXT("HasTag_B__");
			cameraQuality++;
		}
		if( camera.m_genParam.m_tags.HasTag( CNAME( zero ) ) )
		{
			debugString += TXT("HasTag__zero__");
			cameraQuality += 3;
		}

		if( cameraPosition.tags.HasTag( CNAME( forceCloseup ) ) && camera.m_genParam.m_cameraPlane != CP_Closeup && camera.m_genParam.m_cameraPlane != CP_Semicloseup )
		{
			debugString += TXT("ForcingCloseupOnThisShot__");
			cameraQuality++;
		}

		//save camera score
		if( cameraQuality == 0 )
		{
			primaryCameras.PushBack( CameraData( &camera, camaraWeight, debugString, cameraQuality ) );
		}
		if( cameraQuality > 0 && cameraQuality < 3 )
		{
			secondaryCameras.PushBack( CameraData( &camera, camaraWeight, debugString, cameraQuality ) );
		}
		if( emergencyCamera.quality > cameraQuality)
		{
			emergencyCamera = CameraData( &camera, 1.f, debugString, cameraQuality );
		}

		combinedDebugString += ToString(cameraQuality) + TXT(" - [") + debugString + TXT("]\n");
	}

	combinedDebugString = TXT("PositionTags=") + cameraPosition.tags.ToString() + TXT("\n") + combinedDebugString + TXT("\n");
	//chose best from evaluated 
	/////////////////////////////////////////////////////////////////////////////////
	if( !primaryCameras.Empty() )
	{
		const CameraData* chosenData = GetRandomElement( primaryCameras );
		combinedDebugString = TXT("Primary__") + chosenData->debugString + TXT("\n") + combinedDebugString ;
		out = *chosenData;	
		return out.quality;
	}
	if( !secondaryCameras.Empty() )
	{
		const CameraData* chosenData = GetRandomElement( secondaryCameras );
		combinedDebugString = TXT("Secondary__") + chosenData->debugString + combinedDebugString;
		out = *chosenData;
		return out.quality;
	}

	combinedDebugString = TXT("Emergency__") + emergencyCamera.debugString + combinedDebugString;
	out = emergencyCamera;
	return out.quality;
}
