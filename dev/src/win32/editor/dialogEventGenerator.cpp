/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
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
#include "..\..\common\game\storySceneCutsceneSection.h"
#include "..\..\common\game\storySceneEventEnhancedCameraBlend.h"
#include "..\..\common\game\storySceneEventMimicsAnim.h"
#include "..\..\common\game\storySceneEventMimics.h"
#include "..\..\common\game\storySceneEventCameraInterpolation.h"
#include "..\..\common\game\storySceneEventCameraAnimation.h"
#include "..\..\common\engine\localizationManager.h"

//////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME(ALL);
RED_DEFINE_STATIC_NAME(medium_shake)

CStorySceneEventGenerator::CStorySceneEventGenerator( CEdSceneEditor* edit, const CDialogEventGeneratorConfig & config ) 
	: m_config(config),
	  m_editor(edit)
{} 

CStorySceneEventGenerator::~CStorySceneEventGenerator()
{
}


void CStorySceneEventGenerator::GenerateEvents( CStorySceneSection* section )
{
	if ( section->IsA< CStorySceneCutsceneSection >() || ! section->IsExactlyA< CStorySceneSection >() || section->IsGameplay() )
	{
		return;
	}

	
	SContext context;
	TDynArray<SSectionChunk>& sectionChunks = context.m_chunks;
	context.ProcessSection( section );
	SSectionChunk* currentChunk = NULL;
	CName	currentVoicetag;		
	CName	currentSpeakingTo;
	Uint32	numOfElem = section->GetNumberOfElements();
	for ( Uint32 i = 0; i < numOfElem; ++i )
	{
		CStorySceneLine* nextLine = nullptr;
		for (Uint32 j = i + 1; j < numOfElem && !nextLine; j++ )
		{
			nextLine = Cast<CStorySceneLine>( section->GetElement( j ) );
		}

		CStorySceneLine* line = Cast<CStorySceneLine>( section->GetElement( i ) );		
		if( line )
		{
			if( !currentChunk )
			{
				currentChunk		= new (sectionChunks) SSectionChunk();
				currentVoicetag		= line->GetVoiceTag();
				currentSpeakingTo	= line->GetSpeakingTo();
			}	
			currentChunk->m_elements.PushBack( line );
		}
		CStoryScenePauseElement* pause = Cast<CStoryScenePauseElement>( section->GetElement( i ) );		
		if ( pause )
		{
			if( !currentChunk )
			{
				currentChunk = new (sectionChunks) SSectionChunk();			
				if( nextLine )
				{
					currentVoicetag		= nextLine->GetVoiceTag();
					currentSpeakingTo	= nextLine->GetSpeakingTo();
				}
			}				
			currentChunk->m_elements.PushBack( pause );
		}
		if( nextLine && !( currentVoicetag == nextLine->GetVoiceTag() && currentSpeakingTo == nextLine->GetSpeakingTo() ) )
		{
			currentChunk = NULL;
		}
	}

	CStorySceneChoice* choice = section->GetChoice();
	if( choice )
	{
		currentChunk = new (sectionChunks) SSectionChunk();
		currentChunk->m_elements.PushBack( choice );
	}

	if( !m_config.m_preserveExisting )
	{
		RemoveEventsFromSection( context );
	}	

	for ( Uint32 i = 0; i < sectionChunks.Size(); ++i )
	{
		context.ProcessChunk( &sectionChunks[i] );

		if( m_config.m_generateCam && !m_editor->OnEvtGenerator_IsTimelineTrackLocked( CEdDialogTimeline::TIMELINE_DEFAULT_CAMERA_TRACK ) )
		{
			GenerateCameraForChunk( context );
		}
	}

	if ( m_config.m_generateShake )
	{
		GenerateShake( context );
	}

	if ( m_config.m_generateLookats )
	{
		GenerateLookats( context );
	}

	if( m_config.m_generateAnim )
	{
		GenerateAnimationEvents( context );
	}

	if( m_config.m_generateMimic )
	{
		GenerateMimicEvents( context );
	}
}


void CStorySceneEventGenerator::GenerateLookats( SContext& context )
{
	const CStorySceneSection* section = context.m_section;
	const CStoryScene* scene = section->GetScene();
	const CStorySceneDialogsetInstance* dialogset = scene->GetFirstDialogsetAtSection( section );
	if( !dialogset )
	{
		return;
	}
	const TDynArray<CStorySceneDialogsetSlot*>& slots = dialogset->GetSlots();

	for( Uint32 i = 0; i < slots.Size(); ++i )
	{
		CName actorName = slots[i]->GetActorName();
		GenerateLookatsForActor( context, actorName );
	}
}

void CStorySceneEventGenerator::CreateLookatEvent( SContext& context, CName actorName, const LookatData& data, const SPositionInChunk& position, const String& combinedDebugString ) 
{
	if ( context.m_lastLookatTarget == data.target )
	{
		return;
	}

	context.m_lastLookatTarget = data.target;

	CStorySceneElement* element = NULL;
	Float pos = 0.f;
	GetEventPosition( context.m_currentChunk, position, element, pos );

	if( element )
	{
		CStorySceneEventLookAt* lookat = new CStorySceneEventLookAt( String::EMPTY, element, pos, actorName, CName::NONE, true, data.timelineTrack );

		Float speed = data.speed;
		speed > 0.f ? lookat->m_speed = speed : lookat->m_instant = true;

		if( data.target == actorName )
		{
			lookat->m_type = DLT_StaticPoint;
			lookat->m_staticPoint = Vector( 0.f, 1.f, 0.8f );
		}				
		else
		{						
			lookat->m_target = data.target;
			lookat->m_type = DLT_Dynamic;
		}	

		const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
		const CStorySceneSectionVariantId sectionVariantId = context.m_section->GetVariantUsedByLocale( currentLocaleId );

		context.m_section->AddEvent( lookat, sectionVariantId );
	}
};

void CStorySceneEventGenerator::GenerateLookatsForActor( SContext& context, CName actorName )
{
	if( ! actorName )
	{
		return;
	}

	String timelineTrack = actorName.AsString() + TXT( ".lookats" );
	if( m_editor->OnEvtGenerator_IsTimelineTrackLocked( timelineTrack ) )
	{
		return;
	}

	context.m_lastLookatTarget = CName::NONE;

	String debug;
	for ( Uint32 i = 0; i < context.m_chunks.Size(); ++i )
	{
		CName lookatTarget;
		context.ProcessChunk( &context.m_chunks[i] );
		TDynArray<const CStorySceneEvent*> events;
		context.m_currentChunk->GetEvents( events );

		Bool cont = context.m_currentChunk->IsChoice() ;
		for ( Uint32 i = 0 ; i < events.Size() && !cont ; i++ )
		{
			const CStorySceneEventLookAt* lookat = Cast<const CStorySceneEventLookAt>( events[i] );
			if( lookat && lookat->GetSubject() == actorName )
			{
				cont = true;
			}
		}
		if ( cont )
		{
			continue;
		}

		CName speaker = context.m_currentChunk->Speaker();
		CName speakingTo = context.m_currentChunk->SpeakingTo();

		lookatTarget = ( actorName == speaker ? speakingTo : speaker );

		if( context.FirstChunkInSection() )
		{
			if ( context.FirstSectionInScene() )
			{
				CreateLookatEvent( context, actorName, LookatData( lookatTarget, timelineTrack, 0.f ), SPositionInChunk( 0.f), debug );  
			}
			else
			{
				CreateLookatEvent( context, actorName, LookatData( lookatTarget, timelineTrack,  GEngine->GetRandomNumberGenerator().Get< Float >( 0.5f, 1.f ) ), SPositionInChunk( 0.f), debug );  
			}
		}
		else
		{
			if( actorName == speaker )
			{
				CreateLookatEvent( context, actorName, LookatData( lookatTarget, timelineTrack, GEngine->GetRandomNumberGenerator().Get< Float >( 0.5f, 1.f ) ), SPositionInChunk( 0.f), debug );
			}
			else
			{
				CreateLookatEvent( context, actorName, LookatData( lookatTarget, timelineTrack, GEngine->GetRandomNumberGenerator().Get< Float >( 0.5f, 1.f ) ),
									SPositionInChunk( 0.f, GEngine->GetRandomNumberGenerator().Get< Float >(2.f)), debug );
			}	
		}
		if( context.m_currentChunk->Length() > 6 && GEngine->GetRandomNumberGenerator().Get< Uint16 >( 100 ) < 15 && actorName != speaker && actorName != speakingTo )
		{
			CreateLookatEvent( context, actorName, LookatData( speakingTo, timelineTrack, GEngine->GetRandomNumberGenerator().Get< Float >( 0.5f, 1.f ) ),
									 SPositionInChunk( 0.f, GEngine->GetRandomNumberGenerator().Get< Float >( context.m_currentChunk->Length() - 2.f, 3.f ) ), debug );
		}
	}	
}

const CStorySceneEventGenerator::CameraData* CStorySceneEventGenerator::GetRandomElement( const TDynArray< CameraData > & elements )
{
	Float totalWeight = 0.f;
	for ( Uint32 i = 0; i < elements.Size(); ++i )
	{
		if( elements[i].weight != NumericLimits<Float>::Infinity() )
		{
			totalWeight += elements[i].weight;
		}
		else
		{
			return &elements[i];
			;
		}
		
	}
	for ( Uint32 i = 0; i < elements.Size(); ++i )
	{
		if( elements[i].weight/totalWeight > GEngine->GetRandomNumberGenerator().Get< Float >() )
		{
			return &elements[i];
		}
		else
		{
			totalWeight -= elements[i].weight;
		}
	}
	
	ASSERT( false );
	return nullptr;
}



void CStorySceneEventGenerator::GetEventPosition( const SSectionChunk* chunk, const SPositionInChunk& pos, CStorySceneElement*& outElement, Float& outPosInElement  )
{
	Float length = chunk->Length();
	Float value = 0.f;
	value = pos.valAbs + pos.valRel*length;
	if( value > length )
	{
		return;
	}

	const String& currentLocale = SLocalizationManager::GetInstance().GetCurrentLocale();

	Uint32 i = 0;
	Float elDuration = 0.f;
	for( ; i < chunk->m_elements.Size(); ++i)
	{
		elDuration = chunk->m_elements[i]->CalculateDuration( currentLocale );
		if( value - elDuration < 0.0001f )
		{
			break;
		}
		value -= elDuration;
	}
	outPosInElement = Max<Float>( value/elDuration , 0.f );
	outElement		= chunk->m_elements[i];
}

void CStorySceneEventGenerator::GetEventPosition( const SPositionInSection& pos, CStorySceneElement*& outElement, Float& outPosInElement )
{
	return GetEventPosition( &pos.GetChunk(), pos.localPos, outElement, outPosInElement );
}

void CStorySceneEventGenerator::RemoveEventsFromSection( SContext& context )
{

	TDynArray< CStorySceneEvent*> marked;
	TDynArray< CStorySceneEvent*> locked;
	m_editor->OnEvtGenerator_GetMarkedLockedEvents( marked, locked );

	TDynArray<CName> actorsToGenerate = m_config.GetActorsToProcess();
	TDynArray< CStorySceneEvent*> eventsToRemove;
	if ( marked.Size() > 0 )
	{
		eventsToRemove = marked;
	}
	else
	{
		const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
		const CStorySceneSectionVariantId sectionVariantId = context.m_section->GetVariantUsedByLocale( currentLocaleId );

		const TDynArray< CGUID >& evGuids = context.m_section->GetEvents( sectionVariantId );
		for ( Int32 i = evGuids.SizeInt()-1; i >=0 ; --i )
		{
			CStorySceneEvent* event = context.m_section->GetEvent( evGuids[ i ] );

			if( locked.Exist( event ) || m_editor->OnEvtGenerator_IsTimelineTrackLocked( event->GetTrackName() ) )
			{
				continue;
			}
			else if( m_config.m_generateAnim && 
					( Cast<CStorySceneEventAnimation>( event ) || Cast<CStorySceneEventAdditiveAnimation>( event ) ) &&
					actorsToGenerate.Exist( static_cast< CStorySceneEventAnimation* >( event )->GetActor() ) )
			{
				eventsToRemove.PushBack( event );
			}			
			else if( m_config.m_generateLookats && Cast<CStorySceneEventLookAt>( event ) && 
					actorsToGenerate.Exist( static_cast< CStorySceneEventLookAt* >( event )->GetSubject() ) )
			{				
				eventsToRemove.PushBack( event );
			}
			else if( m_config.m_generateCam &&
					( Cast<CStorySceneEventCustomCamera>( event ) || Cast<CStorySceneEventCustomCameraInstance>( event ) || Cast<CStorySceneEventCameraBlend> ( event ) 
					|| Cast<CStorySceneEventEnhancedCameraBlend> ( event ) ) || Cast< CStorySceneCameraBlendEvent >( event ) )
			{
				eventsToRemove.PushBack( event );
			}
			else if( m_config.m_generateCam && Cast<CStorySceneEventCameraInterpolation> ( event ) )
			{
				eventsToRemove.Insert( 0, event );
			}
			else if( m_config.m_generateMimic && (
					( Cast<CStorySceneEventMimicsAnim>( event ) && actorsToGenerate.Exist( static_cast< CStorySceneEventMimicsAnim* >( event )->GetActor() ) ) ||
					( Cast<CStorySceneEventMimics>( event ) && actorsToGenerate.Exist( static_cast< CStorySceneEventMimics* >( event )->GetActor() ) ) 
					) )
			{
				eventsToRemove.PushBack( event );
			}
			else if( m_config.m_generateShake && Cast<CStorySceneEventCameraAnim>( event ) )
			{
				eventsToRemove.PushBack( event );
			}
			

		}
	}
	for ( Uint32 i = 0; i < eventsToRemove.Size(); ++i )
	{
		context.m_section->RemoveEvent( eventsToRemove[i]->GetGUID() );
	}
}


void CStorySceneEventGenerator::GenerateShake( SContext& context )
{
	CStorySceneElement* firstEl = nullptr;

	if ( Uint32 count = context.m_section->GetNumberOfElements() )
	{
		for ( Uint32 i = 0; i < count; i++)
		{
			CStorySceneElement* el = context.m_section->GetElement( i );
			if ( Cast< CAbstractStorySceneLine >( el ) || Cast< CStoryScenePauseElement >( el ) )
			{
				firstEl = context.m_section->GetElement( i );
				break;
			}			
		}		
	}	
	
	if ( !firstEl && context.m_section->GetChoice() )
	{
		firstEl = context.m_section->GetChoice();
	}	

	if( firstEl )
	{
		String trackName = TXT("Camera shake");
		CStorySceneEventCameraAnim* anim =  new CStorySceneEventCameraAnim( String::EMPTY, firstEl, 0.0f, CName::NONE, trackName );
		anim->SetDuration( 0.f );
		anim->SetIsIdle( true );
		anim->SetAnimationBlendIn( 0.f );
		anim->SetAnimationBlendOut( 0.f );		
		anim->SetAnimName( CNAME(medium_shake) );

		const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
		const CStorySceneSectionVariantId sectionVariantId = context.m_section->GetVariantUsedByLocale( currentLocaleId );

		context.m_section->AddEvent( anim, sectionVariantId );
		m_editor->OnEvtGenerator_PinTimelineTrack( trackName );
	}
}



