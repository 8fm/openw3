#pragma once

#include "dialogEditorComment.h"

class CEdSceneCutsceneHeaderPanel : public CEdStorySceneCommentPanel
{
	DECLARE_EVENT_TABLE();

private:
	CStorySceneCutscenePlayer* m_cutsceneDesc;

public:
	CEdSceneCutsceneHeaderPanel( wxWindow* parent, CEdSceneSectionPanel* sectionPanel, CEdUndoManager* undoManager );
	~CEdSceneCutsceneHeaderPanel();

	virtual void SetStorySceneElement( CStorySceneElement* storySceneElement );
	virtual CStorySceneElement* GetDialogElement();
	virtual void RefreshData();
	virtual void OnCommentFocusLost( wxFocusEvent& event ) override;

protected:
	virtual void ConnectHandlers( CEdStorySceneHandlerFactory* handlerFactory );

	void OnRefreshData( wxCommandEvent& event );

private:
	virtual void ImplCommitChanges() override;
};