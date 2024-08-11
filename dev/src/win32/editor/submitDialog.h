#pragma once
#include "../../common/core/set.h"

class CEdSubmitDialog : public wxDialog
{
	DECLARE_EVENT_TABLE();

	protected:
		String &m_description;
		wxCheckListBox *m_list;
		const TDynArray< CDiskFile * > &m_files;
		TSet< CDiskFile * > &m_chosen;

	public:
		CEdSubmitDialog( wxWindow* parent, String &description, const TDynArray< CDiskFile * > &files, TSet< CDiskFile * > &chosen );
		String& GetDescription(){
			return m_description;
		};
		void OnOK( wxCommandEvent& event );
		void OnCancel( wxCommandEvent& event );
};
