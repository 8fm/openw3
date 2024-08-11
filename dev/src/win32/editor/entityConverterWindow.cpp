
#include "build.h"
#include "entityConverterWindow.h"
#include "entityEditor.h"
#include "npcConverter.h"

void CEdEntityEditor::OnExportEntity( wxCommandEvent& event )
{
	if ( !m_preview->GetEntity() )
	{
		ASSERT( m_preview->GetEntity() );
		return;
	}

	CEdEntityConverterWindow* win = new CEdEntityConverterWindow( NULL, m_template, m_preview->GetEntity() );
	win->Show();
	win->SetFocus();
}

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( CEdEntityConverterWindow, wxSmartLayoutPanel )
	EVT_BUTTON( XRCID("btnCancel"), CEdEntityConverterWindow::OnCancelWin )
	EVT_BUTTON( XRCID("btnExport"), CEdEntityConverterWindow::OnExportEntity )
END_EVENT_TABLE()

CEdEntityConverterWindow::CEdEntityConverterWindow( wxWindow* parent, CEntityTemplate* templ, CEntity* entity )
	: wxSmartLayoutPanel( parent, TXT("EntityExporter"), false )
	, m_templ( templ )
	, m_entity( entity )
{
	SRTTI::GetInstance().EnumClasses( CEntityConverter::GetStaticClass(), m_classes );

	FillChoice( templ );
}

CEdEntityConverterWindow::~CEdEntityConverterWindow()
{

}

void CEdEntityConverterWindow::FillChoice( const CEntityTemplate* templ )
{
	if ( !m_entity.Get() )
	{
		return;
	}

	const CClass* entityObjClass = m_entity.Get()->GetClass();

	wxChoice* choice = XRCCTRL( *this, "choice", wxChoice );
	choice->Clear();
	choice->Freeze();

	if ( m_classes.Size() > 0 )
	{
		choice->AppendString( wxT("") );

		for ( Uint32 i=0; i<m_classes.Size(); ++i )
		{
			CEntityConverter* obj = m_classes[ i ]->GetDefaultObject< CEntityConverter >();
			if ( obj->CanConvert( entityObjClass ) )
			{
				choice->AppendString( obj->GetDestClass()->GetName().AsString().AsChar() );
			}
		}
	}
	else
	{
		choice->AppendString( wxT("Empty") );
	}
	

	choice->SetSelection( 0 );
	choice->Thaw();
}

void CEdEntityConverterWindow::OnCancelWin( wxCommandEvent& event )
{
	Close();
}

void CEdEntityConverterWindow::OnExportEntity( wxCommandEvent& event )
{
	if ( !m_templ.Get() || !m_templ.Get()->GetFile() || !m_entity.Get() )
	{
		ASSERT( m_templ.Get() );
		return;
	}

	CEntityTemplate* templToExport = m_templ.Get();

	String fileName;
	if ( !InputBox( NULL, TXT("File name"), TXT("Write file name"), fileName, false ) )
	{
		return;
	}
	fileName += TXT(".w2ent");

	wxChoice* choice = XRCCTRL( *this, "choice", wxChoice );
	Int32 sel = choice->GetSelection();
	if ( sel > 0 && sel < m_classes.SizeInt() )
	{
		CClass* c = m_classes[ sel ];
		CEntityConverter* conv = c->GetDefaultObject< CEntityConverter >();

		CEntityTemplate* newTempl = conv->Convert( templToExport, m_entity.Get() );
		if ( newTempl )
		{
			if ( !newTempl->SaveAs( templToExport->GetFile()->GetDirectory(), fileName ) )
			{
				WARN_EDITOR( TXT("Unable to save created '%s'"), fileName );
				wxMessageBox( wxT("Unable to save file") );
			}

			wxTheFrame->GetAssetBrowser()->UpdateResourceList();
		}
		else
		{
			wxMessageBox( wxT("Fail") );
		}

		Close();
	}
}
