#pragma once

#include "dialogEditorChoice.h"

class CEdStorySceneInjectedChoiceLinePanel : public wxPanel, public IEdDialogHandlerAware
{
	wxDECLARE_CLASS( CEdStorySceneInjectedChoiceLinePanel );

private:
	CEdStorySceneChoicePanel*	m_choicePanel;
	CEdSceneEditorScreenplayPanel*	m_storySceneEditor;

	wxStaticText*					m_indexLabel;
	wxTextCtrl*						m_choiceFileField;
	wxTextCtrl*						m_choiceContentField;

	Uint32								m_lineIndex;
	String							m_choiceLine;
	CStoryScene&					m_hostScene;

public:
	CEdStorySceneInjectedChoiceLinePanel( CEdStorySceneChoicePanel* choicePanel, Uint32 lineIndex, const String& choiceLine, CStoryScene& hostScene );
	~CEdStorySceneInjectedChoiceLinePanel();

	void RefreshData();
	void ChangeFontSize( Int32 sizeChange );
	virtual bool SetBackgroundColour( const wxColour& colour );

protected:
	void OnDoubleClick( wxMouseEvent& event );
};