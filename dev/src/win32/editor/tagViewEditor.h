#pragma once

class wxTagTreeCtrl;
class CTagListProvider;

class CEdTagViewEditor : public wxDialog
{
	DECLARE_EVENT_TABLE()

public:
	CEdTagViewEditor(wxWindow* parent, const TDynArray<CName>& tagList, wxTextCtrl *textCtrl, bool addOkCancelButtons = false);

	const TDynArray<CName> &GetTags() const;
	void SetTagListProviders(const TDynArray<CTagListProvider *> &providers, Bool providersWillBeDisposedExternally = false );

private:

	wxTextCtrl *m_textCtrl;
	wxTagTreeCtrl *m_tagTreeCtrl;
	Bool m_hasCaption;
	
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnActivate(wxActivateEvent& event);
};
