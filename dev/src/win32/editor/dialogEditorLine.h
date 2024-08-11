#pragma once

#include "dialogEditorElement.h"

class CAbstractStorySceneLine;

class CEdStorySceneLineContentPanel : public wxPanel
{
	wxDECLARE_CLASS( CEdStorySceneLineContentPanel );

private:
	wxTextCtrl* m_contentField;

public:
	CEdStorySceneLineContentPanel( wxWindow* parent );

	RED_INLINE wxTextCtrl* GetField() const { return m_contentField; }

	virtual bool SetBackgroundColour( const wxColour& colour );

private:
	void OnChar( wxKeyEvent& event );
	void OnClipboard( wxClipboardTextEvent& event );
};

class CEdStorySceneLinePanel : public CEdStorySceneElementPanel, public IEdDialogHandlerAware
{
	wxDECLARE_CLASS( CEdStorySceneLinePanel );
	wxDECLARE_EVENT_TABLE();

private:
	wxTextCtrl* m_speakerField;
	wxTextCtrl* m_speakingToField;
	wxTextCtrl* m_commentField;
	CEdStorySceneLineContentPanel* m_contentField;

	CAbstractStorySceneLine* m_dialogLine;

public:
	CEdStorySceneLinePanel( wxWindow* parent, CEdSceneSectionPanel* sectionPanel, CEdUndoManager* undoManager, CAbstractStorySceneLine* dialogLine = NULL );
	~CEdStorySceneLinePanel();

	Bool IsEmpty();
	void SetFocus();

	virtual void SetStorySceneElement( CStorySceneElement* storySceneElement );
	virtual CStorySceneElement* GetDialogElement();
	virtual void RefreshData();
	virtual void RefreshHelperData();

	virtual void ChangeFontSize( Int32 sizeChange );

	virtual wxWindow* GetFirstNavigableWindow() { return m_speakerField; }
	virtual wxWindow* GetLastNavigableWindow() { return m_contentField->GetField(); }

	virtual void OnSelected();
	virtual void OnDeselected();

	virtual bool SetBackgroundColour( const wxColour& colour );
	void UpdateColors();

	virtual void GetWordCount( Uint32& contentWords, Uint32& commentWords ) const;

protected:
	void GetAutoCompleteSpeakerNames( wxArrayString &actorNames );

	virtual EStorySceneElementType NextElementType();
	virtual void FillContextMenu( wxMenu& contextMenu );

	void OnSpeakerEnter( wxCommandEvent& event );
	void OnSpeakerLostFocus( wxFocusEvent& event );
	void OnSpeakerChange( wxCommandEvent& event );
	void OnSpeakerChar( wxKeyEvent& event );
	void OnSpeakerPaste( wxClipboardTextEvent& event );
	void OnSpeakerCut( wxClipboardTextEvent& event );

	void OnCommentFocus( wxFocusEvent& event );
	void OnCommentLostFocus( wxFocusEvent& event );
	void OnLineCommentChar( wxKeyEvent& event );
	void OnLineCommentPaste( wxClipboardTextEvent& event );
	void OnLineCommentCut( wxClipboardTextEvent& event );

	void OnLineContentCharEnter( wxKeyEvent& event );
	void OnLineContentLostFocus( wxFocusEvent& event );
	void OnLineContentPaste( wxClipboardTextEvent& event );

	void AddContentLine();
	void OnAddEditLineComment( wxCommandEvent& event );
	void OnMakeCopyUnique( wxCommandEvent& event );
	void OnRecVoiceForLine( wxCommandEvent& event );
	
	Bool IsTextFieldEmpty( wxTextCtrl* textField );
	void OnSpeakingToChange( wxCommandEvent& event );
	void OnSpeakingToChar( wxKeyEvent& event );
	void OnSpeakingToPaste( wxCommandEvent& event );
	void OnSpeakingToCut( wxCommandEvent& event );

private:
	virtual void ImplCommitChanges() override;
};
