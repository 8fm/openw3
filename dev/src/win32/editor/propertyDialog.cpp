
#include "build.h"
#include "../../common/core/feedback.h"
#include "propertyDialog.h"

CEdPropertyDialog::CEdPropertyDialog( wxWindow* parent, const wxString& title, PropertiesPageSettings settings )
{
	Create( parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxCAPTION|wxSYSTEM_MENU|wxCLOSE_BOX|wxRESIZE_BORDER );
	SetSizer( new wxBoxSizer( wxVERTICAL ) );
	SetMinSize( wxSize( 250, 200 ) );

	m_properties = new CEdPropertiesBrowserWithStatusbar( this, settings, nullptr );
	GetSizer()->Add( m_properties, 1, wxEXPAND, 5 );

	wxStdDialogButtonSizer* stdButtons = new wxStdDialogButtonSizer();
	stdButtons->AddButton( new wxButton( this, wxID_OK ) );
	stdButtons->AddButton( new wxButton( this, wxID_CANCEL ) );
	stdButtons->Realize();
	GetSizer()->Add( stdButtons, 0, wxEXPAND|wxALL, 5 );

	LoadOptionsFromConfig();
}

CEdPropertyDialog::~CEdPropertyDialog()
{
}

void CEdPropertyDialog::SetOkCancelLabels( const wxString& okLabel, const wxString& cancelLabel )
{
	FindWindow( wxID_OK )->SetLabel( okLabel );
	FindWindow( wxID_CANCEL )->SetLabel( cancelLabel );
}

Bool CEdPropertyDialog::Execute( CObject* objToEdit )
{
	m_properties->Get().SetObject( objToEdit );
	int res = ShowModal();
	return res == wxID_OK;
}

void CEdPropertyDialog::Show( CObject* objToEdit, OnCloseHandler onCloseHandler )
{
	m_onClose = onCloseHandler;
	m_properties->Get().SetObject( objToEdit );
	Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CEdPropertyDialog::OnButton), nullptr, this );
	wxSmartLayoutDialog::Show();
}

void CEdPropertyDialog::Hide()
{
	wxSmartLayoutDialog::Hide();
}

void CEdPropertyDialog::SaveOptionsToConfig()
{
	SaveLayout( TXT("/Frames/PropertyDialog") );
}

void CEdPropertyDialog::LoadOptionsFromConfig()
{
	LoadLayout( TXT("/Frames/PropertyDialog") );
}

void CEdPropertyDialog::OnButton( wxCommandEvent& event )
{
	m_onClose( event.GetId() == wxID_OK );
}

// -------------------------------

class CEdPropertyDialogTestObj : public CObject
{
	DECLARE_ENGINE_CLASS( CEdPropertyDialogTestObj, CObject, 0 );

public:
	CEdPropertyDialogTestObj()
		: m_propertyA( 1.0 )
		, m_propertyB( 2.0 )
	{
	}

	Float GetA() const { return m_propertyA; }
	Float GetB() const { return m_propertyB; }

private:
	Float m_propertyA;
	Float m_propertyB;
};

BEGIN_CLASS_RTTI( CEdPropertyDialogTestObj );
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_propertyA, TXT("propertyA") );
	PROPERTY_EDIT( m_propertyB, TXT("propertyB") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CEdPropertyDialogTestObj );

void CEdPropertyDialog::ShowExampleModal()
{
	CEdPropertyDialogTestObj* obj = new CEdPropertyDialogTestObj();
	obj->AddToRootSet();

	CEdPropertyDialog dlg( wxTheFrame, TXT("Sample properties") );

	if ( dlg.Execute( obj ) )
	{
		// Do sth with the obj's edited properties
		String msg = String::Printf( TXT("A = %g, B = %g"), obj->GetA(), obj->GetB() );
		GFeedback->ShowMsg( TXT("Results" ), msg.AsChar() );
	}

	obj->RemoveFromRootSet();
	obj->Discard();
}

void CEdPropertyDialog::ShowExampleNonModal()
{
	CEdPropertyDialogTestObj* obj = new CEdPropertyDialogTestObj();
	obj->AddToRootSet();

	// dlg is created on the heap, it must outlive the execution of this method. In practice this probably should be a member of some class.
	CEdPropertyDialog* dlg = new CEdPropertyDialog( wxTheFrame, TXT("Sample properties") );
	dlg->SetOkCancelLabels( TXT("GO!"), TXT("Close") );

	// Non-blocking call showing the window. After closing, the code inside the given lambda will be executed.
	dlg->Show( obj, [=]( Bool result ){
		if ( result )
		{ // ok
			String msg = String::Printf( TXT("A = %g, B = %g"), obj->GetA(), obj->GetB() );
			GFeedback->ShowMsg( TXT("Results" ), msg.AsChar() );
		}
		else
		{ // cancel
			dlg->Hide();
			obj->RemoveFromRootSet();
			obj->Discard();
		}
	});

}
