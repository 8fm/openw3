#pragma once

class CEdCheckListDialog : private wxDialog
{
	DECLARE_EVENT_TABLE();

protected:
	String							m_description;
	wxCheckListBox*					m_list;
	TDynArray< String >				m_options;
	TDynArray< Bool >&				m_optionStates;
	TSet< String >*					m_chosenSet;
	Bool							m_cancelButtonVisible;

public:
	CEdCheckListDialog( wxWindow* parent, const String &title, const TDynArray< String >& options, TDynArray< Bool >& optionStates, TSet< String >& chosen, Bool withCancel = false );
	CEdCheckListDialog( wxWindow* parent, const String &title, const TDynArray< String >& options, TDynArray< Bool >& optionStates, Bool withCancel = false, const String& desc = String::EMPTY );

	virtual void OnOK( wxCommandEvent& event );
	virtual void OnCancel( wxCommandEvent& event );

	using wxDialog::ShowModal ;

private:
	void Init();
	void CreateWidget();
};

class CEdResourcesOverwriteChecker 
{
public:
	CEdResourcesOverwriteChecker( wxWindow* parent, const TDynArray< CResource* >& resources, Bool modifiedOnly = false );

	Bool Execute();

private:
	THashMap< String, CResource* > m_resources;
	wxWindow* m_parent;

};
