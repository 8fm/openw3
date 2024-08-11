#pragma once

class CEdClientDialog : public wxDialog
{
	DECLARE_EVENT_TABLE();
public:
	CEdClientDialog( wxWindow *parent );
	~CEdClientDialog();
public:
	void OnOK( wxCommandEvent &event );
	void OnCancel( wxCommandEvent &event );

	static void GetWorkspaces( TDynArray< String >& workspaces );
};
