
#pragma once

#include "behaviorEditorPanel.h"
#include "animationTreeBrowser.h"

class CEdBehaviorAnimationBrowserPanel : public CEdBehaviorEditorSimplePanel
{
	DECLARE_EVENT_TABLE()

	CEdAnimationTreeBrowser* m_browser;

public:
	CEdBehaviorAnimationBrowserPanel( CEdBehaviorEditor* editor, wxWindow* window = NULL );

	virtual wxString	GetPanelName() const	{ return wxT("Browser"); }
	virtual wxString	GetPanelCaption() const { return wxT("Animation Browser"); }
	wxAuiPaneInfo		GetPaneInfo() const;

	virtual void OnLoadEntity();

private:
	void LoadEntityForPreview();
};
