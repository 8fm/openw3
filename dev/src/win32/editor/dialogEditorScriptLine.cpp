/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "dialogEditorScriptLine.h"

#include "../../common/game/storySceneScriptLine.h"

BEGIN_EVENT_TABLE( CEdSceneScriptLinePanel, CEdStorySceneCommentPanel )
END_EVENT_TABLE()

CEdSceneScriptLinePanel::CEdSceneScriptLinePanel( wxWindow* parent, CEdSceneSectionPanel* sectionPanel, CEdUndoManager* undoManager )
	: CEdStorySceneCommentPanel( parent, sectionPanel, undoManager, NULL, true )
{
	m_commentField->SetForegroundColour( wxColour( 0, 0, 255 ) );
	m_commentField->ChangeValue( TXT( "Script" ) );

	m_elementType = SSET_ScriptLine;
}

CEdSceneScriptLinePanel::~CEdSceneScriptLinePanel( void )
{

}

void CEdSceneScriptLinePanel::SetStorySceneElement( CStorySceneElement* storySceneElement )
{
	if ( storySceneElement->IsA< CStorySceneScriptLine >() == true )
	{
		m_scriptLine = Cast< CStorySceneScriptLine >( storySceneElement );
		RefreshData();
	}
}

CStorySceneElement* CEdSceneScriptLinePanel::GetDialogElement()
{
	return m_scriptLine;
}

Bool CEdSceneScriptLinePanel::RefreshDialogObject( CObject* objectToRefresh )
{
	if ( objectToRefresh == m_scriptLine 
		|| objectToRefresh == m_scriptLine->GetStorySceneScript() // for script parameters being changed
		)
	{
		RefreshData();
		return true;
	}
	return false;
}

void CEdSceneScriptLinePanel::RefreshData()
{
	if ( m_scriptLine != NULL )
	{
		m_commentField->ChangeValue( m_scriptLine->GetScriptString().AsChar() );
	}
	
	if ( m_sectionPanel->GetStorySceneEditor()->ShouldShowOnlyScriptTexts() == true )
	{
		Hide();
	}
	else
	{
		Show();
	}
}

void CEdSceneScriptLinePanel::SetFocus()
{

}

EStorySceneElementType CEdSceneScriptLinePanel::NextElementType()
{
	return SSET_Line;
}

void CEdSceneScriptLinePanel::ConnectHandlers( CEdStorySceneHandlerFactory* handlerFactory )
{
	CEdStorySceneCommentPanel::ConnectHandlers( handlerFactory );
}
