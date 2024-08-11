#include "build.h"
#include "sceneValidator.h"
#include "..\..\common\game\storySceneEventCustomCameraInstance.h"
#include "..\..\common\game\storySceneEvent.h"
#include "..\..\common\game\storySceneElement.h"
#include "..\..\common\game\storySceneSection.h"
#include "..\..\common\game\storySceneEventCustomCamera.h"
#include "..\..\common\game\storySceneCutsceneSection.h"
#include "..\..\common\game\storySceneAbstractLine.h"
#include "..\..\common\game\StorySceneEventCameraLight.h"
#include "..\..\common\game\storySceneEventModifyEnv.h"


#define VALIDATOR_ERROR( s ) context.result.m_messages.PushBack( SValidationOutputMessage( Error, s ) ); 
#define VALIDATOR_WARNING( s ) context.result.m_messages.PushBack( SValidationOutputMessage( Warning, s ) );

void CEdSceneValidator::ProcessElement( SValidationContext& context, const CStorySceneElement* element ) const
{
	//CStorySceneScriptLine
	//CStorySceneVideoElement
	//CAbstractStorySceneLine
	//CStorySceneLine
	//CStorySceneBlockingElement
	//CStorySceneComment
	//CStorySceneQuestChoiceLine ?
	//CStorySceneCutscenePlayer
	//CStoryScenePauseElement

	//in section code
	//CStorySceneChoice

	if ( const CAbstractStorySceneLine* line = Cast<const CAbstractStorySceneLine>( element ))
	{
		CName actorName = line->GetVoiceTag();
		const TDynArray< const CStorySceneDialogsetInstance*>& dialogsets = GetDialogsetsForSection( context, element->GetSection() );
		for( const CStorySceneDialogsetInstance* dialogset : dialogsets )
		{
			String sectionName = element->GetSection()->GetName();
			String elementName = element->GetElementID();
			if ( !dialogset->GetSlotByActorName( actorName ) )
			{
				VALIDATOR_ERROR( String::Printf( TXT("Element [%s] in section [%s] uses actor [%s] not present in one of section dialogsets [%s] "), elementName.AsChar(), sectionName.AsChar(), actorName.AsChar(), dialogset->GetName().AsChar() ) )
			}
		}
	}
}

void CEdSceneValidator::ProcessEvent( SValidationContext& context, const CStorySceneEvent* event ) const
{
	if ( const CStorySceneEventCustomCameraInstance* cam = Cast< const CStorySceneEventCustomCameraInstance >( event ) )
	{
		CName cameraName = cam->GetCustomCameraName();
		if ( ! context.scene->GetCameraDefinition( cameraName ) )
		{
			String sectionName = event->GetSceneElement()->GetSection()->GetName();
			VALIDATOR_ERROR( String::Printf( TXT("Invalid camera name in camera event in section %s"), sectionName.AsChar() ) )
		}
	}
	if ( Cast< const CStorySceneEventCustomCamera >( event ) ||  Cast< const CStorySceneEventCustomCameraInstance >( event ) )
	{
		CStorySceneSection* section = event->GetSceneElement()->GetSection();
		if ( Cast< CStorySceneCutsceneSection >( section ) )
		{
			VALIDATOR_ERROR( String::Printf( TXT("Scene camera event in cutscene section %s"), section->GetName().AsChar() ) )
		}		
	}
	if ( Cast< const CStorySceneEventCameraLight >( event ) ||  Cast< const CStorySceneEventModifyEnv >( event ) )
	{
		CStorySceneSection* section = event->GetSceneElement()->GetSection();
		if ( section->IsGameplay() )
		{
			VALIDATOR_ERROR( String::Printf( TXT("Envioment change / camera light event in gameplay section [%s]"), section->GetName().AsChar() ) )
		}		
	}

	CName actorName = event->GetSubject();
	if( actorName && context.scene->GetActorDescriptionForVoicetag( actorName ) )
	{
		const CStorySceneSection* section = event->GetSceneElement()->GetSection();
		const TDynArray< const CStorySceneDialogsetInstance*>& dialogsets = GetDialogsetsForSection( context, section );
		for( const CStorySceneDialogsetInstance* dialogset : dialogsets )
		{
			String sectionName = section->GetName();
			String eventName  = event->GetName();
			if ( !dialogset->GetSlotByActorName( actorName ) )
			{
				VALIDATOR_ERROR( String::Printf( TXT("Event [%s] in section [%s] uses actor [%s] not present in one of section dialogsets [%s] "), eventName.AsChar(), sectionName.AsChar(), actorName.AsChar(), dialogset->GetName().AsChar() ) )
			}
		}
	}
}


#undef VALIDATOR_ERROR
#undef VALIDATOR_WARNING