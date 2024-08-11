/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef EDITORXMLERRORS_H
#define EDITORXMLERRORS_H

class CEdXMLErrorsDisplayer
{
public:
	CEdXMLErrorsDisplayer( wxWindow* parent );
	~CEdXMLErrorsDisplayer();
	void Execute();

private:
	String m_warningColor;
	String m_errorColor;
	String m_webString;
	wxWindow* m_parent;
	class CEdErrorsListDlg* m_dlg;

	void AppendErrors();
	String GenerateFileEntry( const String& header, const TDynArray< String >& errors );
	void GenerateWebString();
};

#endif //EDITORXMLERRORS_H
