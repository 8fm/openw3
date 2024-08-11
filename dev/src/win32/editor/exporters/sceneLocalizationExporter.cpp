/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "sceneLocalizationExporter.h"

#include "../../../common/game/storySceneInput.h"
#include "../../../common/game/storySceneCutsceneSection.h"
#include "../../../common/game/storySceneChoice.h"
#include "../../../common/game/storySceneChoiceLine.h"
#include "../../../common/game/storySceneLine.h"
#include "../../../common/game/storySceneComment.h"
#include "../../../common/game/storySceneSection.h"
#include "../../../common/game/storySceneControlPartsUtil.h"

#include "../../../common/core/depot.h"

// =================================================================================================
namespace {
// =================================================================================================

/*
Counts number of dialog lines with empty "Speaking to" property.

\param scene Scene to check. Must not be nullptr.
\return Number of dialog lines with empty "Speaking to" property.

Only non-gameplay sections are checked - gameplay sections are skipped.
*/
Uint32 CountLinesWithEmptySpeakingToProperty( const CStoryScene* scene )
{
	ASSERT( scene );

	Uint32 numLinesFound = 0;

	for( Uint32 iSection = 0, numSections = scene->GetNumberOfSections(); iSection < numSections; ++iSection )
	{
		const CStorySceneSection* section = scene->GetSection( iSection );

		if( !section->IsGameplay() )
		{
			TDynArray< CAbstractStorySceneLine* > lines;
			section->GetLines( lines );

			for( auto itLine = lines.Begin(), endLines = lines.End(); itLine != endLines; ++itLine )
			{
				const CAbstractStorySceneLine* line = *itLine;

				if( line->GetSpeakingTo() == CName::NONE )
				{
					++numLinesFound;
				}
			}
		}
	}

	return numLinesFound;
}

/*
Checks whether director's comments for all dialog lines of a scene are valid.

\param scene Scene to check. Must not be nullptr.
\return True - all director's comments are valid, false - otherwise.

We consider director's comment to be not valid if LocalizedString representing the comment
has index 0 (see LocalizedString::GetIndex()). Why? Because this means that our string db
doesn't have an entry for this comment which in turn makes it impossible for the user to
add director's comments via xls/csv file created by scene export.
*/
Bool CheckDirectorsComments( const CStoryScene* scene )
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
			CAbstractStorySceneLine* line = *it;

			LocalizedString* comment = line->GetLocalizedComment();
			if( comment->GetIndex() == 0 )
			{
				sectionValid = false;
				break;
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
Validates director's comments for all dialog lines of a scene.

\param scene Scene on which to operate. Must not be nullptr.
\return True - scene validated successfully, false - couldn't validate scene.

Invalid director's comments are made valid and their value is set to "director's comment".

See CheckDirectorsComments() docs for more information on which director's comments are valid.
*/

Bool ValidateDirectorsComments( CStoryScene* scene )
{
	ASSERT( scene );

	Uint32 numSections = scene->GetNumberOfSections();
	for( Uint32 i = 0; i < numSections; ++i )
	{
		CStorySceneSection* section = scene->GetSection( i );

		TDynArray< CAbstractStorySceneLine* > lines;
		section->GetLines( lines );

		for( auto it = lines.Begin(), end = lines.End(); it != end; ++it )
		{
			CAbstractStorySceneLine* line = *it;

			LocalizedString* comment = line->GetLocalizedComment();
			if( comment->GetIndex() == 0 )
			{
				// This is enough to register new string in strings db.
				comment->SetString( String( TXT( "director's comment" ) ) );
				ASSERT( comment->GetIndex() );
			}
		}
	}

	return true;
}

// =================================================================================================
} // unnamed namespace
// =================================================================================================

CStorySceneLocalizationExporter::CStorySceneLocalizationExporter( Int32 options )
	: CStorySceneExporter()
	, m_markVoicetags( false )
	, m_isBatch( false )
	, m_options( options )
{
	//m_fomats.PushBack( CFileFormat( TXT( "csv" ), TXT( "CSV File" ) ) );
	m_markOneliners = ( options & LEAC_Oneliners ) ? true : false;
	m_cutsceneDescriptions = ( options & LEAC_CSDescription ) ? true : false;
	
	FillRootExportGroup( m_rootExportGroup );
}

void CStorySceneLocalizationExporter::BeginBatchExport()
{
	m_isBatch = true;

	DoBeginExport();
}

void CStorySceneLocalizationExporter::EndBatchExport()
{
	DoEndExport();

	m_isBatch = false;
}

void CStorySceneLocalizationExporter::ExportResource( CResource* resource )
{
	CStoryScene* storyScene = Cast< CStoryScene >( resource );
	if ( storyScene != NULL )
	{
		ExportScene( storyScene );
	}
}

Bool CStorySceneLocalizationExporter::IsBatchedExport()
{
	return m_isBatch;	
}

void CStorySceneLocalizationExporter::ExportScene( CStoryScene* scene )
{
	ASSERT( scene );

	if ( m_markVoicetags == true )
	{
		TDynArray< CName > sceneVoicetags;
		scene->CollectVoicetags( sceneVoicetags );
		if ( sceneVoicetags.GetIndex( CNAME( GERALT ) ) != -1 )
		{
			for ( Uint32 j = 0; j < sceneVoicetags.Size(); ++j )
			{
				m_importantVoicetagsStrings.PushBackUnique( sceneVoicetags[ j ].AsString() );
			}
		}
	}

	CStorySceneExporter::ExportScene( scene );
}

void CStorySceneLocalizationExporter::ExportSceneSection( CStoryScene* storyScene, const CStorySceneSection* section )
{
	if ( ( m_options & LEAC_OnlyCutscenes ) && section->IsA< const  CStorySceneCutsceneSection >() == false )
	{
		return;
	}
	CStorySceneExporter::ExportSceneSection( storyScene, section );
}

void CStorySceneLocalizationExporter::DoBeginExport()
{
	m_importantVoicetagsStrings.Clear();
	m_exportData.Clear();

	TDynArray< String > headers;
	headers.PushBack( TXT( "Source File" ) );
	headers.PushBack( TXT( "StringID" ) );
	headers.PushBack( TXT( "Comment StringID" ) );
	headers.PushBack( TXT( "Section Name" ) );
	headers.PushBack( TXT( "Section Source" ) );
	headers.PushBack( TXT( "Section Type" ) );

	PushLanguageLineData( headers, TXT( "Comment SL" ), TXT( "Speaker SL" ), 
		TXT( "Directors Comment SL" ), TXT( "Line SL" ), TXT( "Choices SL" ), TXT( "Link SL" ) );

	for ( Uint32 i = 0; i < m_numberOfTargetLanguages; ++i )
	{
		PushLanguageLineData( headers, TXT( "Comment TL" ), TXT( "Speaker TL" ), 
			TXT( "Directors Comment TL" ), TXT( "Line TL" ), TXT( "Choices TL" ), TXT( "Link TL" ) );
	}

	headers.PushBack( TXT( "VO filename" ) );

	if ( m_markOneliners == true )
	{
		headers.PushBack( TXT( "Is oneliner" ) );
	}
	if ( m_cutsceneDescriptions == true )
	{
		headers.PushBack( TXT( "Cutscene description" ) );
	}

	m_exportData.PushBack( headers );

	PrepareErrorFileHeaders();

	DoExportEmptyLine();
}
void CStorySceneLocalizationExporter::DoEndExport()
{
	if ( m_markVoicetags == true )
	{
		String speaker;
		for ( Uint32 k = 0; k < m_exportData.Size(); ++k )
		{
			Uint32 voicetagColumn = 5;
			if ( k == 0 )
			{
				m_exportData[ k ].PushBack( TXT( "Important Speaker" ) );
			}
			else
			{
				speaker = m_exportData[ k ][ voicetagColumn ];
				if ( speaker != String::EMPTY )
				{
					m_exportData[ k ].PushBack( ToString( m_importantVoicetagsStrings.GetIndex( speaker ) != -1 ) );
				}
				else
				{
					m_exportData[ k ].PushBack( String::EMPTY );
				}
			}
		}
	}

	String exportString = ConvertToCSV();
	if ( exportString.Empty() == false )
	{
		GFileManager->SaveStringToFileWithUTF8( m_savePath, exportString );
	}

	DumpErrorsFile();
}

void CStorySceneLocalizationExporter::DoExportSceneSectionHeader( const CStorySceneSection* section, CStoryScene* storyScene )
{
	if ( ( m_options & LEAC_OnlyCutscenes ) && section->IsA< CStorySceneCutsceneSection >() == false )
	{
		return;
	}

	TDynArray< String > sectionHeaderData;

	TDynArray< CStorySceneLinkElement* > previousLinkedElements;
	for ( Uint32 k = 0; k < section->GetNumberOfInputPaths(); ++k )
	{
		const CStorySceneLinkElement* sectionInputLinkElement = section->GetInputPathLinkElement( k );
		if ( sectionInputLinkElement == NULL )
		{
			continue;
		}
		previousLinkedElements.PushBackUnique( sectionInputLinkElement->GetLinkedElements() );
	}
	


	String previousControlPartName = TXT( "" );

	for ( Uint32 i = 0; i < previousLinkedElements.Size(); ++i )
	{
		CStorySceneLinkElement* previousLinkedElement = previousLinkedElements[ i ]; 
		if ( previousLinkedElement == NULL )
		{
			continue;
		}
		else if ( previousLinkedElement->IsA< CStorySceneSection >() == true )
		{
			CStorySceneSection* section = Cast< CStorySceneSection >( previousLinkedElement );
			previousControlPartName += section->GetName() + TXT( ", " );
		}
		else if ( previousLinkedElement->IsA< CStorySceneChoiceLine >() == true )
		{
			CStorySceneChoiceLine* choiceLine = Cast< CStorySceneChoiceLine >( previousLinkedElement );
			previousControlPartName += choiceLine->GetChoice()->GetSection()->GetName() + TXT( ", " );
		}
		else if ( previousLinkedElement->IsA< CStorySceneInput >() == true )
		{
			CStorySceneInput* input = Cast< CStorySceneInput >( previousLinkedElement );
			previousControlPartName += String::Printf( TXT( "INPUT( %s )" ), input->GetName().AsChar() ) + TXT( ", " );
		}
		else if ( previousLinkedElement->IsA< CStorySceneControlPart >() == true )
		{
			previousLinkedElements.PushBackUnique( previousLinkedElement->GetLinkedElements() );
		}
		else if ( previousLinkedElement->GetParent() != NULL )
		{
			previousLinkedElements.PushBackUnique( Cast< CStorySceneControlPart >( previousLinkedElement->GetParent() ) );
		}
	}

	if ( previousControlPartName.Empty() == false )
	{
		previousControlPartName = previousControlPartName.LeftString( previousControlPartName.GetLength() - 2 );
	}
	previousControlPartName.Trim();

	sectionHeaderData.PushBack( GetResourceName( storyScene ) );
	sectionHeaderData.PushBack( String::EMPTY );
	sectionHeaderData.PushBack( String::EMPTY );
	sectionHeaderData.PushBack( section->GetName() );
	sectionHeaderData.PushBack( previousControlPartName );
	sectionHeaderData.PushBack( String::EMPTY ); // section type
	
	
	
	String sectionVoicetagsString = String::EMPTY;
	if ( m_options & LEAC_OnlyActorList )
	{
		
		const CStorySceneCutsceneSection* cutsceneSection = Cast< CStorySceneCutsceneSection >( section );
		if ( cutsceneSection != NULL )
		{
			TDynArray< CName > sectionVoicetags;
			cutsceneSection->GetCutsceneVoicetags( sectionVoicetags );

			for ( Uint32 j = 0; j < sectionVoicetags.Size(); ++j )
			{
				if ( sectionVoicetagsString.Empty() == false )
				{
					sectionVoicetagsString += TXT( ", " );
				}
				sectionVoicetagsString += sectionVoicetags[ j ].AsString();
			}
		}
		
	}
	


	PushLanguageLineData( sectionHeaderData, String::EMPTY, sectionVoicetagsString, String::EMPTY, 
		String::EMPTY, String::EMPTY, String::EMPTY );

	for ( Uint32 i = 0; i < m_numberOfTargetLanguages; ++i )
	{
		PushLanguageLineData( sectionHeaderData, String::EMPTY, sectionVoicetagsString, String::EMPTY, 
			String::EMPTY, String::EMPTY, String::EMPTY );
	}

	sectionHeaderData.PushBack( String::EMPTY );

	if ( m_markOneliners )
	{
		sectionHeaderData.PushBack( String::EMPTY );
	}


	
	if ( m_cutsceneDescriptions == true && section->IsA< CStorySceneCutsceneSection >() == true )
	{
		const CStorySceneCutsceneSection* cutsceneSection = Cast< CStorySceneCutsceneSection >( section );
		sectionHeaderData.PushBack( cutsceneSection->GetDescriptionText() );
	}

	m_exportData.PushBack( sectionHeaderData );
}
void CStorySceneLocalizationExporter::DoExportStorySceneLine( const CStorySceneLine* sceneLine, CStoryScene* storyScene, const CStorySceneSection* section, Bool isLastLine /*= false */ )
{
	//sceneLine->GenerateVoiceFileName();

	if ( m_options & LEAC_OnlyActorList )
	{
		return;
	}

	const LocalizedString* lineContent = sceneLine->GetLocalizedContent();
	const LocalizedString* lineComment = sceneLine->GetLocalizedComment();

	TDynArray< String > nextSectionNames;
	if ( isLastLine == true )
	{
		CStorySceneSection* section = sceneLine->GetSection();
		for ( Uint32 k = 0; k < section->GetNumberOfInputPaths(); ++k )
		{
			GetNextSections( section->GetInputPathLinkElement( k ), nextSectionNames );	
		}
	}
	else
	{
		nextSectionNames.PushBack( String::EMPTY );
	}


	String sectionType = TXT( "DLG" );
	if ( section->IsA< CStorySceneCutsceneSection >() == true )
	{
		sectionType = TXT( "CS" );
	}
	else if ( section->IsGameplay() == true )
	{
		sectionType = TXT( "GP" );
	}


	TDynArray< String > elementData;
	elementData.PushBack( GetResourceName( storyScene ) );
	elementData.PushBack( ToString( lineContent->GetIndex() ) );
	elementData.PushBack( ToString( lineComment->GetIndex() ) );
	elementData.PushBack( TXT( "" ) );
	elementData.PushBack( TXT( "" ) );
	elementData.PushBack( sectionType );

	PushLanguageLineData( elementData, String::EMPTY, sceneLine->GetVoiceTag().AsString(), 
		GetSourceString( *lineComment ), GetSourceString( *lineContent ), String::EMPTY,
		nextSectionNames[ 0 ] );

	for ( Uint32 i = 0; i < m_numberOfTargetLanguages; ++i )
	{
		PushLanguageLineData( elementData, String::EMPTY, sceneLine->GetVoiceTag().AsString(), 
			GetTargetString( *lineComment, i ), GetTargetString( *lineContent, i ), String::EMPTY,
			nextSectionNames[ 0 ] );
	}

	elementData.PushBack( sceneLine->GetVoiceFileName() );
	if ( m_markOneliners )
	{
		Bool isOneliner = section->IsGameplay();
		elementData.PushBack( ToString( isOneliner ) );
	}

	if ( m_cutsceneDescriptions == true )
	{
		elementData.PushBack( String::EMPTY );
	}

	m_exportData.PushBack( elementData );

	for ( Uint32 j = 1; j < nextSectionNames.Size(); ++j )
	{
		DoExportAdditionalLink( storyScene, nextSectionNames[ j ] );
	}

}

void CStorySceneLocalizationExporter::DoExportStorySceneComment( const CStorySceneComment* sceneComment, CStoryScene* storyScene, Bool isLastElement /*= false */ )
{
	const LocalizedString* localizedComment = sceneComment->GetLocalizedComment();

	if ( m_options & LEAC_OnlyActorList )
	{
		return;
	}

	TDynArray< String > nextSectionNames;
	if ( isLastElement == true )
	{
		CStorySceneSection* section = sceneComment->GetSection();
		for ( Uint32 k = 0; k < section->GetNumberOfInputPaths(); ++k )
		{
			GetNextSections( section->GetInputPathLinkElement( k ), nextSectionNames );	
		}
	}
	else
	{
		nextSectionNames.PushBack( String::EMPTY );
	}

	TDynArray< String > commentStringData;
	commentStringData.PushBack( GetResourceName( storyScene ) );
	commentStringData.PushBack( ToString( localizedComment->GetIndex() ) );
	commentStringData.PushBack( TXT( "" ) );
	commentStringData.PushBack( TXT( "" ) );
	commentStringData.PushBack( TXT( "" ) );
	commentStringData.PushBack( TXT( "" ) );


	if ( sceneComment->IsExactlyA< CStorySceneQuestChoiceLine >() == false )
	{
		PushLanguageLineData( commentStringData, GetSourceString( *localizedComment ), 
			String::EMPTY, String::EMPTY, String::EMPTY, String::EMPTY, nextSectionNames[ 0 ] );

		for ( Uint32 i = 0; i < m_numberOfTargetLanguages; ++i )
		{
			PushLanguageLineData( commentStringData, GetTargetString( *localizedComment, i ), 
				String::EMPTY, String::EMPTY, String::EMPTY, String::EMPTY, nextSectionNames[ 0 ] );
		}
	}
	else
	{
		PushLanguageLineData( commentStringData, String::EMPTY, String::EMPTY, String::EMPTY, 
			String::EMPTY, GetSourceString( *localizedComment ), nextSectionNames[ 0 ] );

		for ( Uint32 i = 0; i < m_numberOfTargetLanguages; ++i )
		{
			PushLanguageLineData( commentStringData, String::EMPTY,String::EMPTY, String::EMPTY, 
				String::EMPTY, GetTargetString( *localizedComment, i ), nextSectionNames[ 0 ] );
		}
	}
	
	

	commentStringData.PushBack( String::EMPTY );
	if ( m_markOneliners )
	{
		commentStringData.PushBack( String::EMPTY );
	}
	if ( m_cutsceneDescriptions == true )
	{
		commentStringData.PushBack( String::EMPTY );
	}

	m_exportData.PushBack( commentStringData );

	for ( Uint32 j = 1; j < nextSectionNames.Size(); ++j )
	{
		DoExportAdditionalLink( storyScene, nextSectionNames[ j ] );
	}
}
void CStorySceneLocalizationExporter::DoExportStorySceneChoice( const CStorySceneChoice* sectionChoice, CStoryScene* storyScene )
{
	if ( m_options & LEAC_OnlyActorList )
	{
		return;
	}

	for ( Uint32 j = 0; j < sectionChoice->GetNumberOfChoiceLines(); ++j )
	{
		const CStorySceneChoiceLine* choiceLine = sectionChoice->GetChoiceLine( j );

		const LocalizedString* localizedChoiceLine = choiceLine->GetLocalizedChoiceLine();
		const LocalizedString* localizedChoiceComment = choiceLine->GetLocalizedComment();

		TDynArray< String > nextSectionNames;
		GetNextSections( choiceLine, nextSectionNames );

		TDynArray< String > choiceLineData;

		if ( m_options & LEAC_OnlyActorList )
		{
			choiceLineData.PushBack( String::EMPTY );
		}
		else
		{
			choiceLineData.PushBack( GetResourceName( storyScene ) );
		}

		choiceLineData.PushBack( ToString( localizedChoiceLine->GetIndex() ) );
		choiceLineData.PushBack( ToString( localizedChoiceComment->GetIndex() ) );
		choiceLineData.PushBack( String::EMPTY );
		choiceLineData.PushBack( String::EMPTY );
		choiceLineData.PushBack( String::EMPTY );

		PushLanguageLineData( choiceLineData, String::EMPTY, String::EMPTY,
			GetSourceString( *localizedChoiceComment ), String::EMPTY,
			GetSourceString( *localizedChoiceLine ), nextSectionNames[ 0 ] );

		for ( Uint32 i = 0; i < m_numberOfTargetLanguages; ++i )
		{
			PushLanguageLineData( choiceLineData, String::EMPTY, String::EMPTY, 
				GetTargetString( *localizedChoiceComment, i ), String::EMPTY, 
				GetTargetString( *localizedChoiceLine, i ), nextSectionNames[ 0 ] );
		}

		choiceLineData.PushBack( String::EMPTY );
		if ( m_markOneliners )
		{
			choiceLineData.PushBack( String::EMPTY );
		}
		if ( m_cutsceneDescriptions == true )
		{
			choiceLineData.PushBack( String::EMPTY );
		}

		m_exportData.PushBack( choiceLineData );

		for ( Uint32 k = 1; k < nextSectionNames.Size(); ++k )
		{
			DoExportAdditionalLink( storyScene, nextSectionNames[ k ] );
		}
	}
}
void CStorySceneLocalizationExporter::DoExportEmptyLine()
{
	if ( m_options & LEAC_OnlyActorList )
	{
		return;
	}

	TDynArray< String > emptyLineData;
	emptyLineData.PushBack( String::EMPTY );
	emptyLineData.PushBack( String::EMPTY );
	emptyLineData.PushBack( String::EMPTY );
	emptyLineData.PushBack( String::EMPTY );
	emptyLineData.PushBack( String::EMPTY );
	emptyLineData.PushBack( String::EMPTY );

	PushLanguageLineData( emptyLineData, String::EMPTY, String::EMPTY, String::EMPTY, 
		String::EMPTY, String::EMPTY, String::EMPTY );

	for ( Uint32 i = 0; i < m_numberOfTargetLanguages; ++i )
	{
		PushLanguageLineData( emptyLineData, String::EMPTY, String::EMPTY, String::EMPTY, 
			String::EMPTY, String::EMPTY, String::EMPTY );
	}

	emptyLineData.PushBack( String::EMPTY );
	if ( m_markOneliners )
	{
		emptyLineData.PushBack( String::EMPTY );
	}
	if ( m_cutsceneDescriptions == true )
	{
		emptyLineData.PushBack( String::EMPTY );
	}

	m_exportData.PushBack( emptyLineData );
}

Bool CStorySceneLocalizationExporter::CanExportResource( CResource* resource )
{
	if ( resource == NULL )
	{
		return false;
	}
	CStoryScene* scene = Cast< CStoryScene >( resource );
	if ( scene == NULL )
	{
		return false;
	}

	if ( m_options & LEAC_OnlyCutscenes )
	{
		for ( Uint32 i = 0; i < scene->GetNumberOfSections(); ++i )
		{
			CStorySceneSection* section = scene->GetSection( i );
			if ( section->IsA< CStorySceneCutsceneSection >() == true )
			{
				return true;
			}
		}
		return false;
	}

	return true;
}

String CStorySceneLocalizationExporter::GetSceneName( CStoryScene* scene )
{
	String path = scene->GetFriendlyName();

	if ( m_options & LEAC_OnlyActorList )
	{
		return path;
	}

	String compressedPath = TXT( "" );
	
	Uint32 firstPathDepth = 1;
	Uint32 maxPathDepth = 4;

	for ( Uint32 i = 0; i < maxPathDepth; ++i )
	{
		if ( i >= firstPathDepth && i < maxPathDepth )
		{
			String pathPart = path.StringBefore( TXT( "\\" ) );
	
			if ( pathPart.ContainsCharacter( TXT( '_' ) ) )
			{
				pathPart = pathPart.StringBefore( TXT( "_" ) );
			}

			if ( compressedPath.Empty() == false )
			{
				compressedPath += TXT( "_" );
			}
			compressedPath += pathPart;

		}

		path = path.StringAfter( TXT( "\\" ) );
	}

	if ( compressedPath.Empty() == false )
	{
		return String::Printf( TXT( "%s@%s" ), compressedPath.AsChar(), scene->GetSceneName().AsChar() );
	}

	return scene->GetSceneName();
}

void CStorySceneLocalizationExporter::GetNextSections( const CStorySceneLinkElement* link, TDynArray< String >& nextSectionNames )
{
	if ( m_options & LEAC_OnlyActorList )
	{
		nextSectionNames.PushBack( String::EMPTY );
		return;
	}

	TDynArray< CStorySceneControlPart* > nextControlParts;

	CStorySceneControlPart* immediateNextContorlPart = StorySceneControlPartUtils::GetControlPartFromLink( link->GetNextElement() );

	if ( immediateNextContorlPart != NULL )
	{
		immediateNextContorlPart->CollectControlParts( nextControlParts );
	}

	if ( nextControlParts.Empty() == true )
	{
		nextSectionNames.PushBack( TXT( "End of Scene" ) );
	}

	for ( Uint32 i = 0; i < nextControlParts.Size(); ++i )
	{
		if ( nextControlParts[ i ]->IsA< CStorySceneOutput >() == false )
		{
			String sectionName = nextControlParts[ i ]->GetName();
			sectionName.Trim();
			nextSectionNames.PushBack( sectionName );
		}
		else
		{
			nextSectionNames.PushBack( TXT( "End of Scene" ) );
		}
	}

	ASSERT( nextSectionNames.Size() > 0 );
}

void CStorySceneLocalizationExporter::PushLanguageLineData( TDynArray< String >& lineData, 
	const String& comment, const String& voicetag, const String& directorsComment, 
	const String& line, const String& choice, const String& nextSectionName )
{
	lineData.PushBack( comment );
	lineData.PushBack( voicetag );
	lineData.PushBack( directorsComment );
	lineData.PushBack( line );
	lineData.PushBack( choice );
	lineData.PushBack( nextSectionName );
}

void CStorySceneLocalizationExporter::DoExportAdditionalLink( CStoryScene* storyScene, const String& nextSectionName )
{
	if ( m_options & LEAC_OnlyActorList )
	{
		return;
	}

	TDynArray< String > linkData;

	linkData.PushBack( GetResourceName( storyScene ) );
	linkData.PushBack( String::EMPTY );
	linkData.PushBack( String::EMPTY );
	linkData.PushBack( String::EMPTY );
	linkData.PushBack( String::EMPTY );
	linkData.PushBack( String::EMPTY );

	PushLanguageLineData( linkData, String::EMPTY, String::EMPTY, String::EMPTY, 
		String::EMPTY, String::EMPTY, nextSectionName );

	for ( Uint32 i = 0; i < m_numberOfTargetLanguages; ++i )
	{
		PushLanguageLineData( linkData, String::EMPTY, String::EMPTY, String::EMPTY, 
			String::EMPTY, String::EMPTY, nextSectionName );
	}
	linkData.PushBack( String::EMPTY );

	if ( m_markOneliners == true )
	{
		linkData.PushBack( String::EMPTY );
	}
	if ( m_cutsceneDescriptions == true )
	{
		linkData.PushBack( String::EMPTY );
	}

	m_exportData.PushBack( linkData );
}

String CStorySceneLocalizationExporter::GetResourceName( CResource* resource )
{
	if ( m_options & LEAC_OnlyActorList )
	{
		return resource->GetFile()->GetDepotPath();
	}
	return AbstractLocalizationExporter::GetResourceName( resource );
}

void CStorySceneLocalizationExporter::ExportBatchEntry( const String& entry )
{
	CStoryScene* storyScene = LoadResource< CStoryScene >( entry );

	if( storyScene )
	{
		TDynArray< String > infoRows;
		PerformExtraSceneChecks( storyScene, infoRows );

		for( auto itRow = infoRows.Begin(), endRows = infoRows.End(); itRow != endRows; ++itRow )
		{
			PushBatchEntryError( entry, *itRow );
		}

		if( CanExportScene( storyScene ) )
		{
			ExportScene( storyScene );
		}
	}
}

Bool CStorySceneLocalizationExporter::CanExportScene( CStoryScene* scene )
{
	if ( scene == NULL )
	{
		return false;
	}

	if ( m_options & LEAC_OnlyCutscenes )
	{
		for ( Uint32 i = 0; i < scene->GetNumberOfSections(); ++i )
		{
			CStorySceneSection* section = scene->GetSection( i );
			if ( section->IsA< CStorySceneCutsceneSection >() == true )
			{
				return true;
			}
		}
		return false;
	}

	return true;
}

/*
Checks whether scene is valid.

\param scene Scene to check. Must not be nullptr.
\return True - scene is valid. False - otherwise.

This function checks scene validity only from exporter point of view.
*/
Bool CStorySceneLocalizationExporter::IsSceneValid( const CStoryScene* scene ) const
{
	ASSERT( scene );
	Bool sceneValid = CheckDirectorsComments( scene );
	return sceneValid;
}

/*
Validates scene.

\param scene Scene to validate. Must not be nullptr.
\return True - scene validated successfully, false - couldn't validate scene.

This function validates scene by modifying only those parts of a scene that are relevant for exporter.
*/
Bool CStorySceneLocalizationExporter::ValidateScene( CStoryScene* scene ) const
{
	ASSERT( scene );
	Bool validationResult = ValidateDirectorsComments( scene );
	return validationResult;
}

Bool CStorySceneLocalizationExporter::IsBatchEntryValid( const String& entry ) const
{
	Bool sceneValid = false;

	CStoryScene* scene = LoadResource< CStoryScene >( entry );
	if( scene != nullptr )
	{
		sceneValid = IsSceneValid( scene );
	}

	return sceneValid;
}

Bool CStorySceneLocalizationExporter::ValidateBatchEntry( const String& entry )
{
	Bool validationResult = false;

	CStoryScene* scene = LoadResource< CStoryScene >( entry );
	if( scene )
	{
		// It's important to checkout the resource before validating it. It would not be ok to validate the resource and
		// rely on CResource::Save() to checkout it as this may not be possible (e.g. someone else may have a lock on this
		// file or we have an old version of the file). This would leave us with a resource that is validated but not saved.
		if( scene->MarkModified() )
		{
			validationResult = ValidateScene( scene );
			if( validationResult )
			{
				Bool saveResult = scene->Save();
				if( !saveResult )
				{
					PushBatchEntryError( entry, TXT( "Entry was validated but it couldn't be saved." ) );
				}
			}
			else
			{
				PushBatchEntryError( entry, TXT( "Entry couldn't be validated due to validation error." ) );
			}
		}
		else
		{
			PushBatchEntryError( entry, TXT( "Entry needs validation but was not validated as it couldn't be checked out." ) );
		}
	}

	return validationResult;
}

void CStorySceneLocalizationExporter::PushBatchEntryError( const String& entry, const String& errorMsg )
{
	TDynArray< String > csvRow;
	csvRow.PushBack( entry );
	csvRow.PushBack( errorMsg );
	m_errorData.PushBack( csvRow );
}

/*
Performs extra scene checks.

\param scene Scene to check.
\param outInfoRows (out) Container to which to store info rows. Not cleared before use.
\return Number of info rows inserted into outInfoRows container.
*/
Uint32 CStorySceneLocalizationExporter::PerformExtraSceneChecks( const CStoryScene* scene, TDynArray< String >& outInfoRows  ) const
{
	Uint32 numInfoRows = 0;

	Uint32 numLinesWithEmptySpeakingToProperty = CountLinesWithEmptySpeakingToProperty( scene );
	if( numLinesWithEmptySpeakingToProperty > 0 )
	{
		String info = String::Printf( TXT("Found %u dialog line(s) with empty \"Speaking to\" property (only non-gameplay sections were checked). This doesn't make export invalid but please inform scene author about this."), numLinesWithEmptySpeakingToProperty );
		outInfoRows.PushBack( info );
		++numInfoRows;
	}

	return numInfoRows;
}
