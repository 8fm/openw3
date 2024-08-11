/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "refreshVoicesDlg.h"
#include "../../common/engine/localizationManager.h"
#include "../../common/core/feedback.h"
#include "Shellapi.h"

BEGIN_EVENT_TABLE( CEdRefreshVoicesDlg, wxDialog )
	EVT_BUTTON( XRCID("m_refreshButton"), CEdRefreshVoicesDlg::OnRefresh )
END_EVENT_TABLE()

CEdRefreshVoicesDlg::CEdRefreshVoicesDlg( wxWindow* parent )
	: m_sourcePath( TXT( "\\\\cdprs-id574\\w3_speech" ) )
{
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("RefreshVoicesDlg") );

	m_sourceTextBox = XRCCTRL( *this, "m_sourceTextBox", wxTextCtrl );
	m_langChoice = XRCCTRL( *this, "m_langChoice", wxChoice );

	TDynArray< String > textLanguages, speechLanguages;
	SLocalizationManager::GetInstance().GetAllAvailableLanguages( textLanguages, speechLanguages );

	for ( TDynArray< String >::const_iterator langIter = textLanguages.Begin(); 
		langIter != textLanguages.End(); ++langIter )
	{
		m_langChoice->Append( langIter->AsChar() );
	}
}

void CEdRefreshVoicesDlg::Execute()
{
	m_sourceTextBox->SetValue( m_sourcePath.AsChar() );
	m_langChoice->SetStringSelection( SLocalizationManager::GetInstance().GetCurrentLocale().AsChar() );
	Show();
}

void CEdRefreshVoicesDlg::OnRefresh( wxCommandEvent& event )
{
	m_sourcePath = m_sourceTextBox->GetValue().wc_str();
	if ( m_sourcePath.Empty() )
	{
		GFeedback->ShowMsg( TXT( "Empty source path" ), TXT( "You need to specify valid source directory" ) );
		return;
	}

	if( ! m_sourcePath.EndsWith( TXT( "\\" ) ) )
	{
		m_sourcePath +=  TXT( "\\" );
	}

	String language = m_langChoice->GetStringSelection().wc_str();
	language = language.ToLower();

	String pathWithLanguage	= m_sourcePath + language;
	String destPath( GFileManager->GetDataDirectory() + TXT( "speech\\" ) + language );

	String voBatName = TXT( "vo_copy_editor.bat" );
	String audioParams = String::Printf( TXT( "%s\\audio\\ %s\\audio\\" ), pathWithLanguage.AsChar(), destPath.AsChar() );
	ShellExecute( NULL, NULL, voBatName.AsChar(), audioParams.AsChar(),
		GFileManager->GetBaseDirectory().AsChar(), SW_SHOW );

	String lipsBatName = TXT( "lips_copy_editor.bat" );
	String lipsyncParams = String::Printf( TXT( "%s\\lipsync\\ %s\\lipsync\\" ), pathWithLanguage.AsChar(), destPath.AsChar() );
	ShellExecute( NULL, NULL, lipsBatName.AsChar(), lipsyncParams.AsChar(),
		GFileManager->GetBaseDirectory().AsChar(), SW_SHOW );
}