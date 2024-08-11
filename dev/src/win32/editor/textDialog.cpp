#include "build.h"


BEGIN_EVENT_TABLE( CTextDialog, wxDialog )
END_EVENT_TABLE()

CTextDialog::CTextDialog( wxWindow* parent ) : m_linesCount(0)
{
	wxXmlResource::Get()->LoadDialog( this, parent, TXT("TextDialog"));
	XRCCTRL( *this, "textControl", wxTextCtrl )->SetFont( wxFont( 8, 70, 93, 90, false, wxT("Courier New") ) );
	Layout();
}

CTextDialog::~CTextDialog()
{

}

void CTextDialog::Write( const Char* str )
{
	m_contents += String(TXT("\n")) + str;
	XRCCTRL( *this, "textControl", wxTextCtrl )->SetValue( m_contents.AsChar() );
	XRCCTRL( *this, "textControl", wxTextCtrl )->ShowPosition(++m_linesCount);
}

void CTextDialog::Clear()
{
	m_contents.Clear();
	XRCCTRL( *this, "textControl", wxTextCtrl )->Clear();
}