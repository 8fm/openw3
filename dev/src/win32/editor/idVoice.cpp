#include "build.h"
#if 0
#include "interactiveDialogEditor.h"
#include "idConnectorPackEditor.h"
#include "voice.h"

#include <shellapi.h>

#include "../../games/r6/idGraph.h"
#include "../../games/r6/idBasicBlocks.h"
#include "../../games/r6/idResource.h"
#include "../../games/r6/idTopic.h"
#include "../../games/r6/idGraphBlockText.h"
#include "../../games/r6/idGraphBlockBranch.h"
#include "../../games/r6/idGraphBlockChoice.h"
#include "../../games/r6/idConnector.h"
#include "../../common/core/depot.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/localizationManager.h"

void CEdInteractiveDialogEditor::OnMenuGenAudio( wxCommandEvent& event )
{
	m_dialog->Save();

	String pathWithLanguage = AskForVOPath( this );
	if ( pathWithLanguage.Empty() )
	{
		return;
	}

	// Generate new files
	GenerateAudioFiles( m_dialog, pathWithLanguage + TXT("\\audio\\"), false );
}

void CEdInteractiveDialogEditor::OnMenuGenLipsync( wxCommandEvent& event )
{
	m_dialog->Save();

	String pathWithLanguage = AskForVOPath( this );
	if ( pathWithLanguage.Empty() )
	{
		return;
	}

	// Generate new files
	GenerateLipsyncFiles( m_dialog, pathWithLanguage, false );
}

namespace LocalizationTools
{
	Bool ConvertVoiceovers( String wwisePath, String speechResourceDirectory, String language )
	{
		String localSpeechFolder;
		GDepot->GetAbsolutePath( localSpeechFolder );
		localSpeechFolder += TXT( "speech\\" );

		String sourceFilesFolder;
		sourceFilesFolder = speechResourceDirectory;
		sourceFilesFolder += TXT( "\\" );

		String extSourceListTemplateFilePath = localSpeechFolder;
		extSourceListTemplateFilePath += TXT( "\\wwise\\ExtSourceList.wsources"  );

		CSystemFile extSourceTemplateListFile;
		extSourceTemplateListFile.CreateReader( extSourceListTemplateFilePath.AsChar() );

		char* buffer = ( char* ) RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Editor, extSourceTemplateListFile.GetSize() );
		extSourceTemplateListFile.Read( buffer, extSourceTemplateListFile.GetSize() );

		char* carret = buffer + extSourceTemplateListFile.GetSize();
		TDynArray< char > tempBuffer;
		Bool filling = false;
		TDynArray< String > files;

		carret = buffer + extSourceTemplateListFile.GetSize();
		while( carret > buffer )
		{
			if( *carret == 60 )
			{
				break;
			}
			--carret;
		}

		String extSourceListFilePath = localSpeechFolder;
		extSourceListFilePath += TXT( "\\wwise\\ExtSourceListGenerated.wsources" );

		CSystemFile extSourceListFile;
		extSourceListFile.CreateWriter( extSourceListFilePath.AsChar() );
		extSourceListFile.Write( buffer, carret - buffer );

		sourceFilesFolder += TXT( "*.wav" );

		for ( CSystemFindFile findFile( sourceFilesFolder.AsChar() ); findFile; ++findFile )
		{
			if ( !findFile.IsDirectory() )
			{
				String fileName = speechResourceDirectory;
				fileName += findFile.GetFileName();

				files.PushBack( fileName );
			}
		}

		while( carret > buffer )
		{
			if( *carret == 34 )
			{
				if( filling )
				{
					filling = false;
					tempBuffer.PushBack( 0 );
					String fileNameWithPath = ANSI_TO_UNICODE( tempBuffer.Begin() );
					for( Uint32 i = 0; i != files.Size(); ++i )
					{
						size_t index = 0;
						if( fileNameWithPath.FindSubstring( files[ i ].AsChar(), index ) )
						{
							files.RemoveAtFast( i );

							break;

						}
					}
					tempBuffer.Clear();
				}
				else
				{
					filling = true;
				}
			}
			else if( filling )
			{
				tempBuffer.Insert( 0, *carret );
			}
			--carret;
		}

		for( Uint32 i = 0; i != files.Size(); ++i )
		{
			if( i )
			{
				char temp2[] = { 13, 10, 32, 32, 32, 32 };
				extSourceListFile.Write( temp2, 6 );
			}

			String string = TXT( "<Source Path=\"" );
			string += files[ i ];

			string += TXT( "\" />" );
			StringAnsi temp = UNICODE_TO_ANSI( string.AsChar() );
			extSourceListFile.Write( temp.AsChar(), string.Size() - 1 );
		}

		char newline[] = { 13, 10 };
		extSourceListFile.Write( newline, 2 );

		extSourceListFile.Write( "</ExternalSourcesList>", 22 );

		extSourceListFile.Close();
		extSourceTemplateListFile.Close();

		String depotPath;
		GDepot->GetAbsolutePath( depotPath );

		String querry = TXT( "\"" );
		querry += wwisePath;
		querry += TXT( "\\WwiseCLI.exe\" ") + depotPath;
		querry += TXT( "speech\\wwise\\wwise.wproj -ConvertExternalSources");

		RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, buffer );

		int result = system( UNICODE_TO_ANSI( querry.AsChar() ) );
		if( result != 0 && result != 2 )
		{
			LOG_EDITOR( TXT( "Error: invlid code was returned from the \"%s\" command" ), querry );
			return false;
		}
		String tempFilesFolder = depotPath; 
		tempFilesFolder += TXT( "speech\\" );
		tempFilesFolder += TXT( "*.wem" );

		for ( CSystemFindFile findFile( tempFilesFolder.AsChar() ); findFile; ++findFile )
		{
			if ( !findFile.IsDirectory() )
			{
				String newFile = speechResourceDirectory;
				newFile += findFile.GetFileName();
				newFile.Resize( newFile.Size() - 3 );
				newFile += TXT( "ogg");
				String newFileName = newFile;

				String currentFile = localSpeechFolder;
				currentFile += TXT( "\\" );
				currentFile += findFile.GetFileName();
				GSystemIO.MoveFile( currentFile.AsChar(), newFile.AsChar() );
			}
		}

		return true;
	}
}


void CEdInteractiveDialogEditor::GenerateAudioFiles( const CInteractiveDialog* dialog, const String& path, Bool cookerMode )
{
	// Start the progress bar
	if ( !cookerMode )
	{
		GFeedback->BeginTask( TXT("Generating audio files..."), true );
	}

	// Check the time
	const CDateTime thisFileTime( GFileManager->GetFileTime( dialog->GetFile()->GetAbsolutePath() ) );

	// Gather all the text blocks for generation
	Uint32 totalLines( 0 );
	TDynArray< const CIDGraphBlockText* > blockArray;
	GatherAllTextBlocks( dialog, blockArray, totalLines );

	// Set the progress to 0
	if ( !cookerMode )
	{
		GFeedback->UpdateTaskProgress( 0, totalLines * 2 );
	}

	// Generate an audio file for each line of text in each block
	Uint32 progress( 0 );
	for ( Uint32 i = 0; i < blockArray.Size(); ++i )
	{
		const CIDGraphBlockText* block = blockArray[ i ];
		for ( Uint32 k = 0; k < block->GetNumLines(); ++k )
		{
			// Get the text to process
			String fileName = block->GetVoiceoverFileNameForLine( k );
			String text = block->GetStringInCurrentLanguageForLine( k );
			if ( fileName.Empty() || text.Empty() )
			{
				RED_LOG( Dialog, TXT("Audio file for block %s line %ld not generated. Empty data."), block->GetFriendlyName().AsChar(), k );
				continue;
			}

			// Generate *.wav
			String wavPath = path + fileName + TXT(".wav");
			if ( !cookerMode || thisFileTime > GFileManager->GetFileTime( wavPath ) )
			{
				if ( SEdLipsyncCreator::GetInstance().CreateWavAbs( wavPath, text ) )
				{
					RED_LOG( Dialog, TXT("Audio file for block %s generated @ %s"), block->GetFriendlyName().AsChar(), wavPath.AsChar() );
				}
				else
				{
					RED_LOG( Dialog, TXT("Audio file for block %s line %ld not generated. Internal error."), block->GetFriendlyName().AsChar(), k );
				}
			}
			else
			{
				RED_LOG( Dialog, TXT("Audio file for block %s line %ld is already there and is up to date. NOT generating."), block->GetFriendlyName().AsChar(), k );
			}

			// Update the progress
			if ( GFeedback->IsTaskCanceled() )
			{
				if ( !cookerMode )
				{
					GFeedback->EndTask();
				}
				return;
			}
			else if ( !cookerMode )
			{
				GFeedback->UpdateTaskProgress( ++progress, totalLines * 2 );
			}
		}
	}

	if ( !cookerMode )
	{
		// Prepare path to the tool
		LocalizationTools::ConvertVoiceovers( GFileManager->GetBaseDirectory() + TXT("tools\\WWISE\\Win32\\Release\\bin\\"), path, SLocalizationManager::GetInstance().GetCurrentLocale().ToLower() ); 

		// Update the progress
		if ( GFeedback->IsTaskCanceled() )
		{
			GFeedback->EndTask();
			return;
		}
		else
		{
			GFeedback->UpdateTaskProgress( max( 0, totalLines * 2  - 1 ), totalLines * 2 );
		}

		// Copy files to local drive ("refresh voiceovers...")
		{
			// Get the current langugae
			String language = SLocalizationManager::GetInstance().GetCurrentLocale().ToLower();
			if ( language.Empty() )
			{
				GFeedback->EndTask(); 
				return;
			}

			String destPath( GFileManager->GetDataDirectory() + TXT( "speech\\" ) + language );
			String audioParams = String::Printf( TXT( "%s %s\\audio\\" ), path.AsChar(), destPath.AsChar() );
			::ShellExecute( NULL, NULL, TXT("vo_copy_editor.bat" ), audioParams.AsChar(), GFileManager->GetBaseDirectory().AsChar(), SW_SHOW );
		} 

		GFeedback->UpdateTaskProgress( totalLines * 2, totalLines * 2 );

		GDepot->Repopulate();

		// End the progress bar
		GFeedback->EndTask(); 
	}
}

void CEdInteractiveDialogEditor::GenerateLipsyncFiles( const CInteractiveDialog* dialog, const String& path, Bool cookerMode )
{
	// Start the progress bar
	if ( !cookerMode )
	{
		GFeedback->BeginTask( TXT("Generating lipsync files..."), true );
	}

	// Check the time
	const CDateTime thisFileTime( GFileManager->GetFileTime( dialog->GetFile()->GetAbsolutePath() ) );

	// Get the current langugae
	String language = SLocalizationManager::GetInstance().GetCurrentLocale().ToLower();
	if ( !cookerMode && language.Empty() )
	{
		GFeedback->EndTask(); 
		return;
	}

	// Gather all the text blocks for generation
	Uint32 totalLines( 0 );
	TDynArray< const CIDGraphBlockText* > blockArray;
	GatherAllTextBlocks( dialog, blockArray, totalLines );

	// Set the progress to 0
	if ( !cookerMode )
	{
		if ( GFeedback->IsTaskCanceled() )
		{
			GFeedback->EndTask();
			return;
		}
		GFeedback->UpdateTaskProgress( 0, totalLines * 2 );
	}

	// Generate a lipsync file for each line of text in each block
	Uint32 progress( 0 );
	for ( Uint32 i = 0; i < blockArray.Size(); ++i )
	{
		const CIDGraphBlockText* block = blockArray[ i ];
		for ( Uint32 k = 0; k < block->GetNumLines(); ++k )
		{
			// Get the text to process
			String fileName = block->GetVoiceoverFileNameForLine( k );
			String text = block->GetStringInCurrentLanguageForLine( k );
			if ( fileName.Empty() || text.Empty() )
			{
				RED_LOG( Dialog, TXT("Lipsync file for block %s line %ld not generated. Empty data."), block->GetFriendlyName().AsChar(), k );
				continue;
			}

			// Generate *.re	
			if ( !cookerMode || thisFileTime > GFileManager->GetFileTime( ( path + TXT("\\lipsync\\") + fileName + TXT(".re") ) ) )
			{
				if ( SEdLipsyncCreator::GetInstance().CreateLipsyncAbs( path, fileName, text )  )
				{
					RED_LOG( Dialog, TXT("Lipsync file for block %s generated @ %s\\lipsync\\%s.re"), block->GetFriendlyName().AsChar(), path.AsChar(), fileName.AsChar() );
				}
				else
				{
					RED_LOG( Dialog, TXT("Lipsync file for block %s line %ld not generated. Internal error."), block->GetFriendlyName().AsChar(), k );
				}
			}
			else
			{
				RED_LOG( Dialog, TXT("Lipsync file for block %s line %ld is already there and is up to date. NOT generating."), block->GetFriendlyName().AsChar(), k );
			}

			// Update the progress
			if ( GFeedback->IsTaskCanceled() )
			{
				if ( !cookerMode )
				{
					GFeedback->EndTask();
				}
				return;
			}
			else if ( !cookerMode )
			{
				GFeedback->UpdateTaskProgress( ++progress, totalLines * 2 );
			}
		}
	}

	if ( !cookerMode )
	{
		// Update the progress
		if ( GFeedback->IsTaskCanceled() )
		{
			GFeedback->EndTask();
			return;
		}
		else
		{
			GFeedback->UpdateTaskProgress( max( 0, totalLines * 2  - 1 ), totalLines * 2 );
		}

		// Copy files to local drive ("refresh voiceovers...")
		{
			String destPath( GFileManager->GetDataDirectory() + TXT( "speech\\" ) + language );
			String lipsyncParams = String::Printf( TXT( "%s\\lipsync\\ %s\\lipsync\\" ), path.AsChar(), destPath.AsChar() );
			::ShellExecute( NULL, NULL, TXT("lips_copy_editor.bat" ), lipsyncParams.AsChar(), GFileManager->GetBaseDirectory().AsChar(), SW_SHOW );
		} 

		GFeedback->UpdateTaskProgress( totalLines * 2, totalLines * 2 );

		GDepot->Repopulate();

		// End the progress bar
		GFeedback->EndTask(); 
	}
}

String CEdInteractiveDialogEditor::AskForVOPath( wxWindow* editor )
{
	// Ask for network path
	String path = String::Printf( TXT( "\\\\cdprs-id574\\%s_speech" ), GCommonGame->GetGamePrefix() );

	if ( !InputBox( editor, TXT( "Source location" ), TXT( "Enter VO location:" ), path ) )
	{
		return String::EMPTY;
	}

	if ( !path.EndsWith( TXT( "\\" ) ) )
	{
		path +=  TXT( "\\" );
	}

	// Get the current langugae
	String language = SLocalizationManager::GetInstance().GetCurrentLocale().ToLower();
	if ( language.Empty() )
	{
		return String::EMPTY;
	}

	return path + language;
}

void CEdInteractiveDialogEditor::GatherAllTextBlocks( const CInteractiveDialog* dialog, TDynArray< const CIDGraphBlockText* > &blockArray, Uint32 &totalLines )
{
	for ( Uint32 i = 0; i < dialog->GetTopics().Size(); ++i )
	{
		dialog->GetTopics()[ i ]->GatherBlocks< CIDGraphBlockText > ( blockArray, totalLines );
	}
}

void CEdInteractiveDialogEditor::GenerateAllAudioAndLipsyncFiles()
{
	String pathWithLanguage = AskForVOPath( wxTheFrame );
	if ( pathWithLanguage.Empty() )
	{
		return;
	}

	// Start the progress bar
	GFeedback->BeginTask( TXT("Generating audio and lipsync..."), true );
	GFeedback->UpdateTaskInfo( TXT("Scanning depot...") );

	// Get the current langugae
	String language = SLocalizationManager::GetInstance().GetCurrentLocale().ToLower();
	if ( language.Empty() )
	{
		GFeedback->EndTask(); 
		return;
	}

	// Scan the depot
	TDynArray< String > dialogs;
	TDynArray< String > packs;
	GDepot->FindResourcesByExtension( TXT("idialog"), dialogs, true, true );
	GDepot->FindResourcesByExtension( TXT("idcpack"), packs, true, true );

	// Update the progress
	const Uint32 totalWork = ( dialogs.Size() + packs.Size() ) * 2 + 2;
	RED_LOG( Dialog, TXT("Found %ld dialogs and %ld packs."), dialogs.Size(), packs.Size() );
	GFeedback->UpdateTaskProgress( 0, totalWork );

	Uint32 progress( 0 );

	// loop for dialogs
	for ( Uint32 i = 0; i < dialogs.Size(); ++i )
	{
		// update the feedback
		GFeedback->UpdateTaskInfo( TXT("Processing dialog %ld out of %ld: %s"), i + 1, dialogs.Size(), dialogs[ i ].AsChar() );
		
		// load the resource 
		CResource* res = GDepot->LoadResource( dialogs[ i ] );
		if ( nullptr == res )
		{
			progress += 2;
			continue;
		}

		// cast it
		CInteractiveDialog* dialog = Cast< CInteractiveDialog > ( res );
		if ( nullptr == dialog )
		{
			progress += 2;
			continue;
		}

		// prevent gc from deleting it
		const Bool wasInRootSet = dialog->IsInRootSet();
		if ( !wasInRootSet )
		{
			dialog->AddToRootSet();
		}

		// Do the audio part for this file
		GenerateAudioFiles( dialog, pathWithLanguage + TXT("\\audio\\"), true );

		// Update the progress
		if ( GFeedback->IsTaskCanceled() )
		{
			// undo the gc change
			if ( !wasInRootSet )
			{
				dialog->RemoveFromRootSet();
			}

			GFeedback->EndTask();
			return;
		}
		else
		{
			GFeedback->UpdateTaskProgress( ++progress, totalWork );
		}

		// Do the lipsync part for this file
		GenerateLipsyncFiles( dialog, pathWithLanguage, true );

		// undo the gc change
		if ( !wasInRootSet )
		{
			dialog->RemoveFromRootSet();
		}

		// do the gc to free up the memory
		SGarbageCollector::GetInstance().CollectNow();

		// Update the progress
		if ( GFeedback->IsTaskCanceled() )
		{
			GFeedback->EndTask();
			return;
		}
		else
		{
			GFeedback->UpdateTaskProgress( ++progress, totalWork );
		}
	}

	// loop for packs
	for ( Uint32 i = 0; i < packs.Size(); ++i )
	{
		// update the feedback
		GFeedback->UpdateTaskInfo( TXT("Processing connector pack %ld out of %ld: %s"), i + 1, packs.Size(), packs[ i ].AsChar() );
		
		// load the resource 
		CResource* res = GDepot->LoadResource( packs[ i ] );
		if ( nullptr == res )
		{
			progress += 2;
			continue;
		}

		// cast it
		CIDConnectorPack* pack = Cast< CIDConnectorPack > ( res );
		if ( nullptr == pack )
		{
			progress += 2;
			continue;
		}

		// prevent gc from deleting it
		const Bool wasInRootSet = pack->IsInRootSet();
		if ( !wasInRootSet )
		{
			pack->AddToRootSet();
		}

		// Do the audio part for this file
		CEdIDConnectorPackEditor::GenerateAudioFiles( pack, pathWithLanguage + TXT("\\audio\\"), true );

		// Update the progress
		if ( GFeedback->IsTaskCanceled() )
		{
			// undo the gc change
			if ( !wasInRootSet )
			{
				pack->RemoveFromRootSet();
			}

			GFeedback->EndTask();
			return;
		}
		else
		{
			GFeedback->UpdateTaskProgress( ++progress, totalWork );
		}

		// Do the lipsync part for this file
		CEdIDConnectorPackEditor::GenerateLipsyncFiles( pack, pathWithLanguage, true );

		// undo the gc change
		if ( !wasInRootSet )
		{
			pack->RemoveFromRootSet();
		}

		// do the gc to free up the memory
		SGarbageCollector::GetInstance().CollectNow();

		// Update the progress
		if ( GFeedback->IsTaskCanceled() )
		{
			GFeedback->EndTask();
			return;
		}
		else
		{
			GFeedback->UpdateTaskProgress( ++progress, totalWork );
		}
	}

	// update local repo
	{
		GFeedback->UpdateTaskInfo( TXT("Converting voiceovers...") );
		LocalizationTools::ConvertVoiceovers( GFileManager->GetBaseDirectory() + TXT("tools\\WWISE\\Win32\\Release\\bin\\"), pathWithLanguage + TXT("\\audio\\"), language ); 

		// Update the progress
		if ( GFeedback->IsTaskCanceled() )
		{
			GFeedback->EndTask();
			return;
		}
		else
		{
			GFeedback->UpdateTaskProgress( ++progress, totalWork );
		}

		GFeedback->UpdateTaskInfo( TXT("Copying new/updated files to local repo...") );
		String destPath( GFileManager->GetDataDirectory() + TXT( "speech\\" ) + language );
		String audioParams = String::Printf( TXT( "%s\\audio\\ %s\\audio\\" ), pathWithLanguage.AsChar(), destPath.AsChar() );
		::ShellExecute( NULL, NULL, TXT("vo_copy_editor.bat" ), audioParams.AsChar(), GFileManager->GetBaseDirectory().AsChar(), SW_SHOW );
			
		String lipsyncParams = String::Printf( TXT( "%s\\lipsync\\ %s\\lipsync\\" ), pathWithLanguage.AsChar(), destPath.AsChar() );
		::ShellExecute( NULL, NULL, TXT("lips_copy_editor.bat" ), lipsyncParams.AsChar(), GFileManager->GetBaseDirectory().AsChar(), SW_SHOW );

		// Update the progress
		GFeedback->UpdateTaskProgress( ++progress, totalWork );
	}

	GDepot->Repopulate();

	// End the task
	GFeedback->EndTask();
}
#endif