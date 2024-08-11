#pragma once

struct STagNode;
class CTagListProvider;
class CHistoryTagListProvider;

class wxTagTreeCtrl : public wxTreeListCtrl
{
	DECLARE_EVENT_TABLE()

public:

	wxTagTreeCtrl(wxWindow *parent, const TDynArray<CName> &tagList, wxTextCtrl *editCtrl);
	~wxTagTreeCtrl();

	const TDynArray<CName> &GetTags() const { return m_tags; }
	void SetTagListProviders(const TDynArray<CTagListProvider *> providers, Bool providersWillBeDisposedExternally = false );
	void RememberTags();

	// Save / Load options from config file
	void SaveOptionsToConfig();
	void LoadOptionsFromConfig(Bool resize);

private:

	wxTextCtrl *m_editCtrl;
	String m_filter;
	TDynArray<CName> m_tags;
	CHistoryTagListProvider *m_historyTagListProvider;
	TDynArray<CTagListProvider *> m_availableTagsProviders;
	Bool m_refreshPending;
	Bool m_lockOnEditTagChanged;

	Bool m_providersWillBeDisposedExternally;

	// Internal methods
	void UpdateTagTree(Bool force = false);
	void BuildTree(wxTreeItemId parentNode, const STagNode *tagNode);
	String ExtractFilter();

	// Event handlers
	void OnClose(wxCloseEvent& event);
	void OnTreeSelectionChanged(wxTreeEvent& event);
	void OnTreeNodeContext(wxMouseEvent& event);
	void OnEditTagChanged(wxCommandEvent& event);
	void OnForgetTag(wxCommandEvent& event);

};
