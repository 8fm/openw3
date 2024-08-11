#pragma once

class CEdFileHistoryDialog : public wxDialog
{
private:
	wxTreeListCtrl *m_tree;
public:
	CEdFileHistoryDialog( wxWindow *parent, const String &file, 
						  const TDynArray< THashMap< String, String > > &history );
	~CEdFileHistoryDialog();
};