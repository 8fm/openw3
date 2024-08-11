/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "dialogEditorComment.h"

class CEdSceneVideoDescriptionPanel : public CEdStorySceneCommentPanel
{
	DECLARE_EVENT_TABLE();

private:
	CStorySceneVideoElement* m_videoElement;

public:
	CEdSceneVideoDescriptionPanel( wxWindow* parent, CEdSceneSectionPanel* sectionPanel, CEdUndoManager* undoManager );
	~CEdSceneVideoDescriptionPanel();

	virtual void SetStorySceneElement( CStorySceneElement* storySceneElement );
	virtual CStorySceneElement* GetDialogElement();
	virtual void RefreshData();
	virtual void OnCommentFocusLost( wxFocusEvent& event ) override;

protected:
	void OnRefreshData( wxCommandEvent& event );

private:
	virtual void ImplCommitChanges() override;
};