
/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "dialogEditorCutscene.h"

#include "../../common/game/storySceneCutscene.h"

BEGIN_EVENT_TABLE( CEdSceneCutsceneHeaderPanel, CEdStorySceneCommentPanel )
END_EVENT_TABLE()

CEdSceneCutsceneHeaderPanel::CEdSceneCutsceneHeaderPanel( wxWindow* parent, CEdSceneSectionPanel* sectionPanel, CEdUndoManager* undoManager )
: CEdStorySceneCommentPanel( parent, sectionPanel, undoManager )
{
	m_commentField->SetForegroundColour( wxColour( 128, 0, 128 ) );
	m_commentField->ChangeValue( TXT( "Cutscene description" ) );
	//m_commentField->Enable( false );
	m_commentField->SetWindowStyleFlag( wxTE_MULTILINE|wxTE_RICH|wxNO_BORDER|wxTE_PROCESS_ENTER|wxTE_READONLY );

	m_elementType = SSET_Cutscene;
}

CEdSceneCutsceneHeaderPanel::~CEdSceneCutsceneHeaderPanel()
{

}

void CEdSceneCutsceneHeaderPanel::SetStorySceneElement( CStorySceneElement* storySceneElement )
{
	if ( storySceneElement->IsA< CStorySceneCutscenePlayer >() == true )
	{
		m_cutsceneDesc = Cast< CStorySceneCutscenePlayer >( storySceneElement );
		RefreshData();
	}
}

CStorySceneElement* CEdSceneCutsceneHeaderPanel::GetDialogElement()
{
	return m_cutsceneDesc;
}

void CEdSceneCutsceneHeaderPanel::RefreshData()
{
	m_commentField->SetValue( m_cutsceneDesc->GetDescriptionText().AsChar() );
}

void CEdSceneCutsceneHeaderPanel::ConnectHandlers( CEdStorySceneHandlerFactory* handlerFactory )
{
	CEdStorySceneCommentPanel::ConnectHandlers( handlerFactory );
}

void CEdSceneCutsceneHeaderPanel::OnCommentFocusLost( wxFocusEvent& event )
{
	if ( m_cutsceneDesc )
	{
		m_cutsceneDesc->SetDescriptionText( m_commentField->GetValue().wc_str() );
	}

	m_sectionPanel->GetStorySceneEditor()->RefreshWordCount();

	event.Skip();
}

void CEdSceneCutsceneHeaderPanel::ImplCommitChanges()
{
	m_cutsceneDesc->SetDescriptionText( m_commentField->GetValue().wc_str() );
}
