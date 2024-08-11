#include "build.h"
#include "tagTreeCtrl.h"
#include "tagViewEditor.h"
#include "tagEditor.h"

BEGIN_EVENT_TABLE(CEdTagViewEditor, wxDialog)
	EVT_ACTIVATE(CEdTagViewEditor::OnActivate)
	EVT_BUTTON(wxID_OK, CEdTagViewEditor::OnOK)
	EVT_BUTTON(wxID_CANCEL, CEdTagViewEditor::OnCancel)
END_EVENT_TABLE()

CEdTagViewEditor::CEdTagViewEditor(wxWindow* parent, const TDynArray<CName>& tagList, wxTextCtrl *textCtrl, bool addOkCancelButtons /*= false*/)
: wxDialog(parent, wxID_ANY, wxT("Tags"), wxDefaultPosition, wxDefaultSize, wxRESIZE_BORDER)
, m_textCtrl(textCtrl)
, m_tagTreeCtrl(NULL)
, m_hasCaption(textCtrl == NULL || addOkCancelButtons)
{
	SetIcon(wxNullIcon);
	SetMinSize(wxSize(150, 150));

	// Base sizer
	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	if (!m_textCtrl)
	{
		m_textCtrl = new wxTextCtrl(this, wxID_ANY);
		sizer->Add(m_textCtrl, 0, wxALL|wxEXPAND, 1);
	}

	if (m_hasCaption)
	{
		// Add caption to the window
		SetWindowStyleFlag(GetWindowStyle() | wxCAPTION | wxCLOSE_BOX);
	}

	m_tagTreeCtrl = new wxTagTreeCtrl(this, tagList, m_textCtrl);
	sizer->Add(m_tagTreeCtrl, 1, wxALL|wxEXPAND, 0);
	SetSizer(sizer);
	//m_tagTreeCtrl->SetFocus();

	m_tagTreeCtrl->LoadOptionsFromConfig(m_hasCaption);
	wxRect screenRect = m_textCtrl->GetScreenRect();

	if ( addOkCancelButtons == true )
	{
		wxBoxSizer *buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

		wxButton *okButton = new wxButton(this, wxID_OK, wxT("Ok"));
		wxButton *cancelButton = new wxButton(this, wxID_CANCEL, wxT("Cancel"));

		buttonsSizer->AddStretchSpacer(1);
		buttonsSizer->Add(okButton, 0, wxRIGHT|wxEXPAND, 2);
		buttonsSizer->Add(cancelButton, 0, wxLEFT|wxEXPAND, 2);
		buttonsSizer->AddStretchSpacer(1);
		sizer->Add(buttonsSizer, 0, wxALL|wxEXPAND, 5);
	}
	else
	{
		Int32 width = max(screenRect.GetWidth(), m_tagTreeCtrl->GetSize().GetWidth());
		SetSize(width, 350);
	}

	if (m_hasCaption)
	{
		Center();
	}
	else
	{
		Move(screenRect.GetLeftBottom());
	}
}

const TDynArray<CName> &CEdTagViewEditor::GetTags() const
{
	return m_tagTreeCtrl->GetTags();
}

void CEdTagViewEditor::SetTagListProviders( const TDynArray<CTagListProvider *> &providers, Bool providersWillBeDisposedExternally /*= false */ )
{
	m_tagTreeCtrl->SetTagListProviders( providers, providersWillBeDisposedExternally );
}

void CEdTagViewEditor::OnOK( wxCommandEvent& event )
{
	m_tagTreeCtrl->RememberTags();
	m_tagTreeCtrl->SaveOptionsToConfig();

	// Send to parent
	wxCommandEvent okEvent(wxEVT_TAGEDITOR_OK);
	ProcessEvent(okEvent);

	// Close window
	EndDialog(wxOK);
}

void CEdTagViewEditor::OnCancel( wxCommandEvent& event )
{
	m_tagTreeCtrl->SaveOptionsToConfig();

	// Send to parent
	wxCommandEvent cancelEvent( wxEVT_TAGEDITOR_CANCEL );
	ProcessEvent( cancelEvent );

	// Close window
	EndDialog(wxCANCEL);
}

void CEdTagViewEditor::OnActivate(wxActivateEvent& event)
{
	if (!m_hasCaption)
	{
		if (!event.GetActive())
		{
			m_tagTreeCtrl->RememberTags();
			m_tagTreeCtrl->SaveOptionsToConfig();

			// Deactivating dialog without the caption treat as if the OK button has been pressed
			wxCommandEvent okEvent(wxEVT_TAGEDITOR_OK);
			ProcessEvent(okEvent);

			EndDialog(wxOK);
		}
	}
}
