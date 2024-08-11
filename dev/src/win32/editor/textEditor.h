/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdTextEditor;

class IEdTextEditorHook
{
public:
	virtual void OnTextEditorModified( CEdTextEditor* editor ){}
	virtual void OnTextEditorClosed( CEdTextEditor* editor ){}
};

class CEdTextEditor : public wxFrame
{
protected:
	IEdTextEditorHook*		m_hook;
	wxTextCtrl*				m_textCtrl;
	wxButton*				m_openButton;
	wxButton*				m_saveButton;
	wxButton*				m_closeButton;

	void OnOpen( wxCommandEvent& event );
	void OnSave( wxCommandEvent& event );
	void OnClose( wxCommandEvent& event );
	void OnFrameClose( wxCloseEvent& event );
	void OnTextUpdated( wxCommandEvent& event );

public:
	CEdTextEditor( wxWindow *parent, const String& caption );

	virtual void SetText( const String& text );
	virtual String GetText() const;

	void SetHook( IEdTextEditorHook* hook );
	RED_INLINE IEdTextEditorHook* GetHook() const { return m_hook; }
};
