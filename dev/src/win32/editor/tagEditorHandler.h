#pragma once

class CEdTagViewEditor;
class CTagListProvider;
class CEdSceneEditorScreenplayPanel;

#include "dialogEditorHandler.h"
#include "tagMiniEditor.h"

class CTagListProvider;

class CEdTagEditorHandler : public IEdDialogEditorHandler
{
protected:
	static CEdTagMiniEditor* m_tagEditor;

	wxWindow* m_handledWindow;
	CTagListProvider* m_tagListProvider;
	Bool m_blockInputWhenEditorHidden;
	CEdSceneEditorScreenplayPanel* m_storySceneEditor;

public:
	CEdTagEditorHandler( CEdSceneEditorScreenplayPanel* dialogEditor, CTagListProvider* tagListUpdater = NULL );
	void ConnectTo( wxWindow* window );

protected:
	void OnFocus( wxFocusEvent& event );

	void CreateAndShowTagEditor( wxTextCtrl* eventSource );
	void OnFocusLost( wxFocusEvent& event );
	void OnEnter( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
	void OnOk( wxCommandEvent& event );
	void OnLeftDoubleClick( wxMouseEvent& event );
	void OnKeyDown( wxKeyEvent& event );

	virtual void CreateTagEditor( wxTextCtrl* eventSource );
	virtual void SaveEditedValues();
	TagList MakeTagsFromString( String sTags );
};
