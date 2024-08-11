/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "dialogEditorVideo.h"

#include "../../common/game/storySceneVideo.h"

BEGIN_EVENT_TABLE( CEdSceneVideoDescriptionPanel, CEdStorySceneCommentPanel )
END_EVENT_TABLE()

CEdSceneVideoDescriptionPanel::CEdSceneVideoDescriptionPanel( wxWindow* parent, CEdSceneSectionPanel* sectionPanel, CEdUndoManager* undoManager )
	: CEdStorySceneCommentPanel( parent, sectionPanel, undoManager )
{
	m_commentField->SetForegroundColour( wxColour( 128, 0, 128 ) );
	m_commentField->ChangeValue( TXT( "Video description" ) );
	//m_commentField->Enable( false );
	m_commentField->SetWindowStyleFlag( wxTE_MULTILINE|wxTE_RICH|wxNO_BORDER|wxTE_PROCESS_ENTER|wxTE_READONLY );

	m_elementType = SSET_Video;
}

CEdSceneVideoDescriptionPanel::~CEdSceneVideoDescriptionPanel()
{

}

void CEdSceneVideoDescriptionPanel::SetStorySceneElement( CStorySceneElement* storySceneElement )
{
	if ( storySceneElement->IsA< CStorySceneVideoElement >() == true )
	{
		m_videoElement = Cast< CStorySceneVideoElement >( storySceneElement );
		RefreshData();
	}
}

CStorySceneElement* CEdSceneVideoDescriptionPanel::GetDialogElement()
{
	return m_videoElement;
}

void CEdSceneVideoDescriptionPanel::RefreshData()
{
	m_commentField->SetValue( m_videoElement->GetDescriptionText().AsChar() );
}

void CEdSceneVideoDescriptionPanel::OnCommentFocusLost( wxFocusEvent& event )
{
	if ( m_videoElement )
	{
		m_videoElement->SetDescriptionText( m_commentField->GetValue().wc_str() );
	}

	m_sectionPanel->GetStorySceneEditor()->RefreshWordCount();

	event.Skip();
}

void CEdSceneVideoDescriptionPanel::ImplCommitChanges()
{
	m_videoElement->SetDescriptionText( m_commentField->GetValue().wc_str() );
}
