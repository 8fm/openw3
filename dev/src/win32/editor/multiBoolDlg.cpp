#include "build.h"
#include "multiBoolDlg.h"


CEdMultiBoolDlg::CEdMultiBoolDlg(wxWindow* parent, const String& title, const TDynArray<String>& questions, TDynArray<Bool>& answers)
	: wxDialog( parent, wxID_ANY, title.AsChar(), wxDefaultPosition, wxSize( -1, -1 ) )
{
	SetSizeHints( wxDefaultSize, wxDefaultSize );
	Centre( wxBOTH );

	//// Base sizer
	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	m_boxes.Resize(questions.Size());
	sizer->AddSpacer(10);
	for(unsigned int i =0; i < questions.Size(); i++)
	{
		m_boxes[i] = new wxCheckBox(this, wxID_ANY,questions[i].AsChar(),wxDefaultPosition,wxDefaultSize, wxALIGN_CENTRE );	
		if(answers.Size() > i )
		{
			m_boxes[i]->SetValue(answers[i]);
		}
		sizer->Add( m_boxes[i], 2, wxLEFT | wxRIGHT , 5 );
	}
	sizer->AddStretchSpacer(1);

	// Sizer for buttons
	wxBoxSizer* sizer2 = new wxBoxSizer( wxHORIZONTAL );

	// OK button
	wxButton *ok = new wxButton( this, wxID_ANY, wxT("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
	ok->SetDefault(); 
	sizer2->Add( ok, 1, wxALL, 5 );

	// Cancel button
	wxButton* cancel = new wxButton( this, wxID_ANY, wxT("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	sizer2->Add( cancel, 1, wxALL, 5 );

	// Finalize
	sizer->Add( sizer2, 3,  wxALIGN_RIGHT | wxALIGN_BOTTOM | wxRIGHT | wxBOTTOM, 5 );

	// Connect Events
	ok->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdMultiBoolDlg::OnOK ), NULL, this );
	cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdMultiBoolDlg::OnCancel ), NULL, this );

	//// Update layout
	SetSizer( sizer );
	Layout();
	Fit();
}

VOID CEdMultiBoolDlg::OnOK( wxCommandEvent& event )
{
	m_answers.ResizeFast(m_boxes.Size());
	for (unsigned int i =0; i < m_boxes.Size(); i++)
	{
		m_answers[i] = m_boxes[i]->GetValue();
	}
	EndDialog( 1 );
}

VOID CEdMultiBoolDlg::OnCancel( wxCommandEvent& event )
{
	EndDialog( 0 );
}