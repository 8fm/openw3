/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Input box
class CEdInputBox: public wxDialog 
{
protected:
	wxTextCtrl*		m_editBox;
	String			m_editText;

public:
	CEdInputBox( wxWindow* parent, const String& title, const String &text, const String &defaultEdit, Bool multiline = false );
	RED_INLINE const String &GetEditText() { return m_editText; }

protected:
	virtual void OnOK( wxCommandEvent& event );
	virtual void OnCancel( wxCommandEvent& event );
};

/// Input box for file name
class CEdInputBoxFileName: public CEdInputBox 
{
protected:
	String			m_fileExtension;

private:
	Bool ValidateFileName() const;

protected:
	virtual void OnOK( wxCommandEvent& event );

public:
	CEdInputBoxFileName( wxWindow* parent, const String& title, const String &text, const String &defaultEdit, const String &fileExtension );
};

/// Input double box
class CEdInputDoubleBox: public wxDialog 
{
protected:
	wxTextCtrl*		m_editBoxA;
	wxTextCtrl*		m_editBoxB;

	String&			m_editTextA;
	String&			m_editTextB;

	Bool			m_useConfig;

public:
	CEdInputDoubleBox( wxWindow* parent, const String& title, const String& msg, String &textA, String &textB, Bool multiline = false, Bool useConfig = false );

private:
	void ReadDefaultValues();
	void SaveDefaultValues();

protected:
	virtual void OnOK( wxCommandEvent& event );
	virtual void OnCancel( wxCommandEvent& event );
};

/// Input multi box
class CEdInputMultiBox: public wxDialog 
{
protected:
	struct SLine
	{
		wxSizer*			m_sizer;
		wxStaticText*		m_label;
		wxTextCtrl*			m_inputBox;
		wxButton*			m_addBtn;
	};

	TDynArray< SLine >		m_lines;
	TDynArray< String >&	m_values;
	wxSizer*				m_linesSizer;

	static const Int32		INITIAL_Y_SIZE;
	static const Int32		LINE_Y_SIZE;
	static const Int32		MARGIN;

public:
	CEdInputMultiBox( wxWindow* parent, const String& title, const String& msg, TDynArray< String >& values );

protected:
	void OnOK( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
	void OnAdd( wxCommandEvent& event );

	void InsertLine( Uint32 index );
	void OnLayoutChange();
};
