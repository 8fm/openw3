/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "lipsyncDataSceneExporter.h"

#include "voice.h"

#include "../../common/game/storyScene.h"
#include "../../common/game/storySceneLine.h"
#include "../../common/game/storySceneSection.h"
#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/localizationManager.h"

// =================================================================================================
namespace {
// =================================================================================================

// declarations of functions in this namespace
Bool IsSceneValid( const CStoryScene* scene );
Bool CheckVoiceovers( const CStoryScene* scene );
Bool FixVoiceovers( CStoryScene* scene );

/*
Checks whether scene is valid.

\param scene Scene to check. Must not be nullptr.
\return True - scene is valid. False - otherwise.

This function checks scene validity only from exporter point of view.
*/
Bool IsSceneValid( const CStoryScene* scene )
{
	ASSERT( scene );
	Bool sceneValid = CheckVoiceovers( scene );
	return sceneValid;
}

/*
Checks whether voiceover names in scene and voiceover names in strings database match.

\param scene Scene to check. Must not be nullptr.
\param True - all voiceover names match. False - otherwise.

One scenario in which voiceover names in scene and voiceover names in strings database become mismatched:
1. We have a scene file in which dialog line L1 is spoken by actor A1. Let's say this is version 1 of a scene file.
2. Change dialog line L1 speaker to actor A2. Save the scene and submit it to version control system as version 2.
   At this point, voiceover names are properly updated in strings database so everything is still fine.
3. Use version control system to get version 1 of a file and submit it as version 3.
   At this point, voiceover names become mismatched because voiceover names in string database are not updated.
   The way to fix this is to resave the file.
*/
Bool CheckVoiceovers( const CStoryScene* scene )
{
	ASSERT( scene );

	Bool sceneValid = true;

	Uint32 numSections = scene->GetNumberOfSections();
	for( Uint32 i = 0; i < numSections; ++i )
	{
		Bool sectionValid = true;
		const CStorySceneSection* section = scene->GetSection( i );

		TDynArray< CAbstractStorySceneLine* > lines;
		section->GetLines( lines );

		for( auto it = lines.Begin(), end = lines.End(); it != end; ++it )
		{
			CStorySceneLine* line = Cast< CStorySceneLine >( *it );
			if( line )
			{
				Uint32 stringId = line->GetLocalizedContent()->GetIndex();
				LanguagePack* languagePack = SLocalizationManager::GetInstance().GetLanguagePackSync( stringId, true );
				if( !languagePack || languagePack->GetVoiceoverFileName() != line->GetVoiceFileName() )
				{
					sectionValid = false;
					break;
				}
			}
		}

		if( !sectionValid )
		{
			sceneValid = false;
			break;
		}
	}

	return sceneValid;
}

/*
Fixes bad voiceover names in strings db.

\param scene Scene for which to fix voiceover names.
\return True - voiceovers fixed. False - otherwise.
*/
Bool FixVoiceovers( CStoryScene* scene )
{
	SLocalizationManager::GetInstance().UpdateStringDatabase( scene, true );
	return true;
}

// =================================================================================================
} // unnamed namespace
// =================================================================================================

CLipsyncDataSceneExporter::CLipsyncDataSceneExporter()
	: m_generateOnlyText( false )
{
	FillRootExportGroup( m_rootExportGroup );
}

void CLipsyncDataSceneExporter::DoExportStorySceneLine( const CStorySceneLine* sceneLine, CStoryScene* storyScene, const CStorySceneSection* section, Bool isLastLine /*= false */ )
{
#ifndef NO_EDITOR 
	Uint32 stringId = sceneLine->GetLocalizedContent()->GetIndex();
	
	LanguagePack* languagePack = SLocalizationManager::GetInstance().GetLanguagePackSync( stringId, true );

	if ( languagePack == NULL )
	{
		return;
	}

	String lipsyncPath = m_lipsyncDirectory + languagePack->GetLipsyncFileName() + TXT( ".re" );
	String voiceoverPath = m_voiceoverDirectory + sceneLine->GetVoiceFileName() + TXT( ".wav" );

	//static Bool generateWavs = false;
	//if ( generateWavs && !GFileManager->FileExist( voiceoverPath ) )
	//{
	//	SEdLipsyncCreator::GetInstance().CreateWavAbs( voiceoverPath, sceneLine->GetLocalizedContent()->GetString() );
	//}

	const Bool shouldRegenerateLipsync = ShouldRegenerateLipsync( lipsyncPath, voiceoverPath );
	if ( shouldRegenerateLipsync )
	{
		String text		= sceneLine->GetLocalizedContent()->GetString();
		String lang		= m_sourceLanguage.ToLower();
		String fileName	= sceneLine->GetVoiceFileName();

		if ( !SEdLipsyncCreator::GetInstance().CreateLipsync( fileName, lang, text, m_generateOnlyText ) )
		{
			const char* errMsg = SEdLipsyncCreator::GetInstance().GetLastError();
			if ( errMsg )
			{
				// Create Row
				TDynArray< String > csvRow;

				// Error info part		
				csvRow.PushBack( fileName );							// A - Source File 
				csvRow.PushBack( ANSI_TO_UNICODE( errMsg ) );			// B - "Error"

				m_errorData.PushBack( csvRow );
			}
		}
	}

	SLocalizationManager::GetInstance().ReleaseLanguagePack( stringId );
#endif
}

Bool CLipsyncDataSceneExporter::ShouldRegenerateLipsync( const String& lipsyncPath, const String& voiceoverPath ) const
{
	if ( !GFileManager->FileExist( voiceoverPath ) )
	{
		return false;
	}
	
	if ( !GFileManager->FileExist( lipsyncPath ) )
	{
		return true;
	}

	const Bool shouldGenerate = ( GFileManager->GetFileTime( voiceoverPath ) > GFileManager->GetFileTime( lipsyncPath ) );

	return shouldGenerate;
}

void CLipsyncDataSceneExporter::BeginBatchExport()
{
	m_generateOnlyText = !GFeedback->AskYesNo( TXT("Do you want to generate lipsync and text files (Yes) or text files only(No)") );

	Bool m_wasLocalizationManagerIgnoringPackResources 
		= SLocalizationManager::GetInstance().ShouldIgnoreLanguagePackResources();
	SLocalizationManager::GetInstance().SetIgnoreLanguagePackResources( true );

	m_editorLocale = SLocalizationManager::GetInstance().GetCurrentLocale();
	SLocalizationManager::GetInstance().SetCurrentLocale( m_sourceLanguage );

	String destinationPath;
	GDepot->GetAbsolutePath( destinationPath );

	m_lipsyncDirectory		= String::Printf( TXT( "%sspeech\\%s\\lipsync\\" ),	destinationPath.AsChar(), m_sourceLanguage.ToLower().AsChar() );
	m_voiceoverDirectory	= String::Printf( TXT( "%sspeech\\%s\\audio\\" ),		destinationPath.AsChar(), m_sourceLanguage.ToLower().AsChar() );
	m_textDirectory			= String::Printf( TXT( "%sspeech\\%s\\text\\" ),		destinationPath.AsChar(), m_sourceLanguage.ToLower().AsChar() );

	TDynArray< String > destinationData;
	destinationData.PushBack( m_lipsyncDirectory );
	destinationData.PushBack( m_sourceLanguage );

	m_addedLipsyncsPaths.Clear();

	m_exportData.Clear();
	m_exportData.PushBack( destinationData );

	PrepareErrorFileHeaders();
}

void CLipsyncDataSceneExporter::EndBatchExport()
{
	if ( DumpErrorsFile() )
	{
		GFeedback->ShowError( TXT("Couldn't generate all lipsync files. Please check log.") );
	}
}

void CLipsyncDataSceneExporter::ExportResource( CResource* resource )
{
	CStoryScene* storyScene = Cast< CStoryScene >( resource );
	if ( storyScene != NULL )
	{
		ExportScene( storyScene );
	}
}

Bool CLipsyncDataSceneExporter::CanExportResource( CResource* resource )
{
	if ( resource == NULL )
	{
		return false;
	}
	return resource->IsA< CStoryScene >();
}

void CLipsyncDataSceneExporter::ExportBatchEntry( const String& entry )
{
	CStoryScene* storyScene = LoadResource< CStoryScene >( entry );
	if ( storyScene != NULL )
	{
		ExportScene( storyScene );
	}
}

Bool CLipsyncDataSceneExporter::IsBatchEntryValid( const String& entry ) const
{
	Bool sceneValid = false;

	CStoryScene* storyScene = LoadResource< CStoryScene >( entry );
	if ( storyScene )
	{
		sceneValid = IsSceneValid( storyScene );
	}
	
	return sceneValid;
}

Bool CLipsyncDataSceneExporter::ValidateBatchEntry( const String& entry )
{
	Bool sceneValid = false;

	CStoryScene* storyScene = LoadResource< CStoryScene >( entry );
	if ( storyScene )
	{
		sceneValid = FixVoiceovers( storyScene );
	}

	return sceneValid;
}

void CLipsyncDataSceneExporter::PushBatchEntryError( const String& entry, const String& errorMsg )
{
	TDynArray< String > csvRow;
	csvRow.PushBack( entry );
	csvRow.PushBack( errorMsg );
	m_errorData.PushBack( csvRow );
}
