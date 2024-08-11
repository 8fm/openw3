/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "dialogEditorComment.h"

class CStorySceneScriptLine;

class CEdSceneScriptLinePanel : public CEdStorySceneCommentPanel
{
	DECLARE_EVENT_TABLE();

private:
	CStorySceneScriptLine* m_scriptLine;

public:
	CEdSceneScriptLinePanel( wxWindow* parent, CEdSceneSectionPanel* sectionPanel, CEdUndoManager* undoManager );
	~CEdSceneScriptLinePanel(void);

	virtual void SetStorySceneElement( CStorySceneElement* storySceneElement ) override;
	virtual CStorySceneElement* GetDialogElement() override;
	virtual void RefreshData() override;

	virtual void SetFocus() override;

protected:
	virtual EStorySceneElementType NextElementType() override;
	virtual void ConnectHandlers( CEdStorySceneHandlerFactory* handlerFactory ) override;
	virtual Bool RefreshDialogObject( CObject* objectToRefresh ) override;
};
