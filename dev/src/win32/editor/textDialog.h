#pragma once

// Output
class CTextDialog : public wxDialog
{
	DECLARE_EVENT_TABLE()

	String		m_contents;
	Uint32		m_linesCount;

public:
	CTextDialog( wxWindow* parent );
	virtual ~CTextDialog();


	virtual void Write( const Char* str );
	virtual void Clear();
};

