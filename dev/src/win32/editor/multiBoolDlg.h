#pragma once


class CEdMultiBoolDlg : public wxDialog
{
	TDynArray<wxCheckBox*> m_boxes;
	TDynArray<Bool>		   m_answers;

public:
	CEdMultiBoolDlg(wxWindow* parent, const String& title, const TDynArray<String>& questions, TDynArray<Bool>& answers);
	RED_INLINE void GetValues(TDynArray<Bool>& answers)
	{
		answers = m_answers;
	}

protected:
	void OnOK( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
};

