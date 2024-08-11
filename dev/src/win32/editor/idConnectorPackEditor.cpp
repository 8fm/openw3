#include "build.h"
#if 0
#include "idConnectorPackEditor.h"
#include "interactiveDialogEditor.h"
#include "voice.h"

#include <shellapi.h>

#include "../../games/r6/idConnector.h"
#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/localizationManager.h"

BEGIN_EVENT_TABLE( CEdIDConnectorPackEditor, wxSmartLayoutPanel )
	EVT_MENU( XRCID( "GenAudioMenuItem" ),		CEdIDConnectorPackEditor::OnMenuGenAudio ) 
	EVT_MENU( XRCID( "GenLipsyncMenuItem" ),	CEdIDConnectorPackEditor::OnMenuGenLipsync )
	EVT_MENU( XRCID( "SaveMenuItem" ),			CEdIDConnectorPackEditor::OnMenuSave )
	EVT_MENU( XRCID( "ExitMenuItem" ),			CEdIDConnectorPackEditor::OnMenuExit )
	EVT_CLOSE(									CEdIDConnectorPackEditor::OnClose )
END_EVENT_TABLE()

IMPLEMENT_CLASS( CEdIDConnectorPackEditor, wxSmartLayoutPanel );


CEdIDConnectorPackEditor::CEdIDConnectorPackEditor( wxWindow* parent, CIDConnectorPack* pack /*= NULL */ )
	: wxSmartLayoutPanel( parent, TEXT( "IDConnectorPackEditor" ), false )
	, m_pack( pack )
{
	// Preserve edited resource
	pack->AddToRootSet();

	// Set icon
	wxIcon iconSmall;
	iconSmall.CopyFromBitmap( SEdResources::GetInstance().LoadBitmap( TXT( "IMG_BDI_DIALOG" ) ) );
	SetIcon( iconSmall );

	SetAutoLayout( true );

	wxPanel* propertiesPanel = XRCCTRL( *this, "m_propertiesPanel", wxPanel );
	{
		propertiesPanel->SetSizer( new wxBoxSizer( wxVERTICAL ) );

		PropertiesPageSettings settings;
		settings.m_autoExpandGroups = true;
		m_properties = new CEdPropertiesPage( propertiesPanel, settings );
		propertiesPanel->GetSizer()->Add( m_properties, 1, wxEXPAND );
		m_properties->SetObject( pack );
	}

	SetTitle( pack->GetFriendlyName().AsChar() );

	LoadOptionsFromConfig();
	Layout();
}

CEdIDConnectorPackEditor::~CEdIDConnectorPackEditor()
{
	m_pack->RemoveFromRootSet();
}

void CEdIDConnectorPackEditor::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
}

void CEdIDConnectorPackEditor::SaveOptionsToConfig()
{
	SaveLayout( TXT("/Frames/IDConnectorPackEditor") );
}

void CEdIDConnectorPackEditor::LoadOptionsFromConfig()
{
	LoadLayout( TXT("/Frames/IDConnectorPackEditor") );
}

void CEdIDConnectorPackEditor::OnMenuGenAudio( wxCommandEvent& event )
{
	m_pack->Save();

	String pathWithLanguage = CEdInteractiveDialogEditor::AskForVOPath( this );
	if ( pathWithLanguage.Empty() )
	{
		return;
	}

	// Generate new files
	GenerateAudioFiles( m_pack, pathWithLanguage + TXT("\\audio\\"), false );
}

void CEdIDConnectorPackEditor::OnMenuGenLipsync( wxCommandEvent& event )
{
	m_pack->Save();

	String pathWithLanguage = CEdInteractiveDialogEditor::AskForVOPath( this );
	if ( pathWithLanguage.Empty() )
	{
		return;
	}

	// Generate new files
	GenerateLipsyncFiles( m_pack, pathWithLanguage, false );
}

void CEdIDConnectorPackEditor::OnMenuSave( wxCommandEvent& event )
{
	m_pack->Save();
}

void CEdIDConnectorPackEditor::OnMenuExit( wxCommandEvent& event )
{
	Close();
}

void CEdIDConnectorPackEditor::OnClose( wxCloseEvent &event )
{
	SaveOptionsToConfig();
}

namespace LocalizationTools
{
	extern Bool ConvertVoiceovers( String wwisePath, String speechResourceDirectory, String language );
};

void CEdIDConnectorPackEditor::GenerateAudioFiles( const CIDConnectorPack* pack, const String& path, Bool cookerMode )
{
	// Start the progress bar
	const Uint32 numFiles = pack->GetNumLines() * pack->GetMatchingVoiceTags().Size();
	if ( !cookerMode )
	{
		GFeedback->BeginTask( TXT("Generating audio files..."), true );
		GFeedback->UpdateTaskProgress( 0, numFiles * 2 );
	}

	// Check the time
	const CDateTime thisFileTime( GFileManager->GetFileTime( pack->GetFile()->GetAbsolutePath() ) );

	// Generate an audio file for each line of text for each voice tag
	Uint32 progress( 0 );
	for ( Uint32 i = 0; i < pack->GetNumLines(); ++i )
	{
		for ( Uint32 k = 0; k < pack->GetMatchingVoiceTags().Size(); ++k )
		{
			// Get the text to process
			String fileName = pack->GetVoiceoverFileNameForLine( i, pack->GetMatchingVoiceTags()[ k ].m_voiceTag );
			String text = pack->GetStringInCurrentLanguageForLine( i );
			if ( fileName.Empty() || text.Empty() )
			{
				RED_LOG( Dialog, TXT("Audio file for voiceTag %s line %ld not generated. Empty data."), pack->GetMatchingVoiceTags()[ k ].m_voiceTag.AsString().AsChar(), i );
				continue;
			}

			// Generate *.wav
			String wavPath = path + fileName + TXT(".wav");
			if ( !cookerMode || thisFileTime > GFileManager->GetFileTime( wavPath ) )
			{
				if ( SEdLipsyncCreator::GetInstance().CreateWavAbs( wavPath, text ) )
				{
					RED_LOG( Dialog, TXT("Audio file generated @ %s"), wavPath.AsChar() );
				}
				else
				{
					RED_LOG( Dialog, TXT("Audio file for voiceTag %s line %ld not generated. Internal error."), pack->GetMatchingVoiceTags()[ k ].m_voiceTag.AsString().AsChar(), i );
				}
			}
			else
			{
				RED_LOG( Dialog, TXT("Audio file for voicetag %s line %ld is already there and is up to date. NOT generating."), pack->GetMatchingVoiceTags()[ k ].m_voiceTag.AsString().AsChar(), i );
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
				GFeedback->UpdateTaskProgress( ++progress, numFiles * 2 );
			}
		}
	}

	if ( !cookerMode )
	{
		LocalizationTools::ConvertVoiceovers( GFileManager->GetBaseDirectory() + TXT("tools\\WWISE\\Win32\\Release\\bin\\"), path, SLocalizationManager::GetInstance().GetCurrentLocale().ToLower() ); 

		// Update the progress
		if ( GFeedback->IsTaskCanceled() )
		{
			GFeedback->EndTask();
			return;
		}
		else
		{
			GFeedback->UpdateTaskProgress( max( 0, numFiles * 2  - 1 ), numFiles * 2 );
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

			//String lipsyncParams = String::Printf( TXT( "%s\\lipsync\\ %s\\lipsync\\" ), pathWithLanguage.AsChar(), destPath.AsChar() );
			//::ShellExecute( NULL, NULL, TXT( "lips_copy_editor.bat" ), lipsyncParams.AsChar(),
			//	GFileManager->GetBaseDirectory().AsChar(), SW_SHOW );
		} 

		GFeedback->UpdateTaskProgress( numFiles * 2, numFiles * 2 );

		GDepot->Repopulate();

		// End the progress bar
		GFeedback->EndTask(); 
	}
}

void CEdIDConnectorPackEditor::GenerateLipsyncFiles( const CIDConnectorPack* pack, const String& path, Bool cookerMode )
{
	// Start the progress bar
	const Uint32 numFiles = pack->GetNumLines() * pack->GetMatchingVoiceTags().Size();
	if ( !cookerMode )
	{
		GFeedback->BeginTask( TXT("Generating lipsync files..."), true );
		GFeedback->UpdateTaskProgress( 0, numFiles * 2 );
	}

	// Check the time
	const CDateTime thisFileTime( GFileManager->GetFileTime( pack->GetFile()->GetAbsolutePath() ) );

	// Get the current langugae
	String language = SLocalizationManager::GetInstance().GetCurrentLocale().ToLower();
	if ( language.Empty() )
	{
		GFeedback->EndTask(); 
		return;
	}

	// Generate a lipsync file for each entry, for each voicetag
	Uint32 progress( 0 );
	for ( Uint32 i = 0; i < pack->GetNumLines(); ++i )
	{
		for ( Uint32 k = 0; k < pack->GetMatchingVoiceTags().Size(); ++k )
		{
			// Get the text to process
			String fileName = pack->GetVoiceoverFileNameForLine( i, pack->GetMatchingVoiceTags()[ k ].m_voiceTag );
			String text = pack->GetStringInCurrentLanguageForLine( i );
			if ( fileName.Empty() || text.Empty() )
			{
				RED_LOG( Dialog, TXT("Lipsync file for %s line %ld not generated. Empty data."), pack->GetFriendlyName().AsChar(), k );
				continue;
			}

			// Generate *.re
			if ( !cookerMode || thisFileTime > GFileManager->GetFileTime( ( path + TXT("\\lipsync\\") + fileName + TXT(".re") ) ) )
			{
				if ( SEdLipsyncCreator::GetInstance().CreateLipsyncAbs( path, fileName, text )  )
				{
					RED_LOG( Dialog, TXT("Lipsync file for %s generated @ %s\\lipsync\\%s.re"), pack->GetFriendlyName().AsChar(), path.AsChar(), fileName.AsChar() );
				}
				else
				{
					RED_LOG( Dialog, TXT("Lipsync file for %s line %ld not generated. Internal error."), pack->GetFriendlyName().AsChar(), k );
				}
			}
			else
			{
				RED_LOG( Dialog, TXT("Lipsync file for %s line %ld is already there and is up to date. NOT generating."), pack->GetFriendlyName().AsChar(), k );
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
				GFeedback->UpdateTaskProgress( ++progress, numFiles * 2 );
			}
		}
	}

	// Update the progress
	if ( !cookerMode )
	{
		if ( GFeedback->IsTaskCanceled() )
		{
			GFeedback->EndTask();
			return;
		}
		else
		{
			GFeedback->UpdateTaskProgress( max( 0, numFiles * 2  - 1 ), numFiles * 2 );
		}

		// Copy files to local drive ("refresh voiceovers...")
		{
			String destPath( GFileManager->GetDataDirectory() + TXT( "speech\\" ) + language );
			String lipsyncParams = String::Printf( TXT( "%s\\lipsync\\ %s\\lipsync\\" ), path.AsChar(), destPath.AsChar() );
			::ShellExecute( NULL, NULL, TXT("lips_copy_editor.bat" ), lipsyncParams.AsChar(), GFileManager->GetBaseDirectory().AsChar(), SW_SHOW );
		} 

		GFeedback->UpdateTaskProgress( numFiles * 2, numFiles * 2 );

		GDepot->Repopulate();

		// End the progress bar
		GFeedback->EndTask(); 
	}
}
#endif