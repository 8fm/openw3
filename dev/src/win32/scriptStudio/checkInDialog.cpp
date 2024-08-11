/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "checkInDialog.h"
#include "solution/file.h"

wxDEFINE_EVENT( ssEVT_CHECKIN_EVENT, CCheckInEvent );
wxIMPLEMENT_DYNAMIC_CLASS( CCheckInEvent, wxEvent );

CCheckInEvent::CCheckInEvent( wxEventType commandType, int winid )
:	wxEvent( commandType, winid )
{

}

CCheckInEvent::CCheckInEvent( wxString description, const vector< SolutionFilePtr >& filesToSubmit )
:	wxEvent( wxID_ANY, ssEVT_CHECKIN_EVENT )
,	m_description( description )
,	m_filesToSubmit( filesToSubmit )
{

}

wxIMPLEMENT_CLASS( CSSCheckinDialog, wxDialog );

#define ICON_SIZE 16

CSSCheckinDialog::CSSCheckinDialog( wxWindow* parent, wxString description )
{
	// Load layout from XRC
	wxXmlResource::Get()->LoadDialog( this, parent, wxT( "CheckInDialog" ) );

	m_fileList = XRCCTRL( *this, "fileList", wxListCtrl );

	m_fileList->Bind( wxEVT_LEFT_DOWN, &CSSCheckinDialog::OnMouseEvent, this );

	wxImageList* imageList = wxTheSSApp->CreateSolutionImageList();

	//	Checked_On
	imageList->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_CHECK_ON" ) ) );	
	//	Checked_Off
	imageList->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_CHECK_OFF" ) ) );	

	m_fileList->AssignImageList( imageList, wxIMAGE_LIST_SMALL );

	m_fileList->InsertColumn( Col_Enabled, wxEmptyString );
	m_fileList->InsertColumn( Col_Icon, wxEmptyString );
	m_fileList->InsertColumn( Col_File, wxT( "File" ) );

	m_fileList->SetColumnWidth( Col_Enabled, wxLIST_AUTOSIZE );
	m_fileList->SetColumnWidth( Col_Icon, wxLIST_AUTOSIZE );
	m_fileList->SetColumnWidth( Col_File, wxLIST_AUTOSIZE );

	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CSSCheckinDialog::OnOkClicked, this, wxID_OK );

	wxTextCtrl* descriptionCtrl = XRCCTRL( *this, "description", wxTextCtrl );
	RED_ASSERT( descriptionCtrl != nullptr, TXT( "Description Widget missing from Submit dialog" ) );

	descriptionCtrl->SetValue( description );
}

CSSCheckinDialog::~CSSCheckinDialog()
{

}

void CSSCheckinDialog::AddFile( const SolutionFilePtr& file, ESolutionImage icon )
{
	wxString filePath = file->m_absolutePath.c_str();

	long row = m_fileList->InsertItem( m_fileList->GetItemCount(), Checked_On + SOLIMG_Max );

	m_fileList->SetItem( row, Col_Icon, wxEmptyString, icon );
	m_fileList->SetItem( row, Col_File, filePath );

	CheckInData data;
	data.file = file;
	data.enabled = true;
	m_fileData.push_back( data );
	m_fileList->SetItemData( row, m_fileData.size() - 1 );

	m_fileList->SetColumnWidth( Col_Enabled, wxLIST_AUTOSIZE );
	m_fileList->SetColumnWidth( Col_Icon, wxLIST_AUTOSIZE );
	m_fileList->SetColumnWidth( Col_File, wxLIST_AUTOSIZE );

	// Start at a value above 0 since it seems to come out slightly under
	int calcWidth = 20;
	for( int i = 0; i < Col_Max; ++i )
	{
		calcWidth += m_fileList->GetColumnWidth( i );
	}

	SetClientSize( calcWidth, 500 );
}

void CSSCheckinDialog::OnMouseEvent( wxMouseEvent& event )
{
	if ( event.LeftDown() )
	{
		int flags;
		long item = m_fileList->HitTest( event.GetPosition(), flags );

		if ( item > -1 && ( flags & wxLIST_HITTEST_ONITEMICON ) )
		{
			CheckInData& data = m_fileData[ m_fileList->GetItemData( item ) ];

			if( data.enabled )
			{
				m_fileList->SetItemImage( item, Checked_Off + SOLIMG_Max );
			}
			else
			{
				m_fileList->SetItemImage( item, Checked_On + SOLIMG_Max );
			}

			data.enabled = !data.enabled;
		}
		else
		{
			event.Skip();
		}
	}
	else
	{
		event.Skip();
	}
}

void CSSCheckinDialog::OnOkClicked( wxCommandEvent& )
{
	// Grab check in description
	wxTextCtrl* descriptionCtrl = XRCCTRL( *this, "description", wxTextCtrl );
	RED_ASSERT( descriptionCtrl != nullptr, TXT( "Description Widget missing from Submit dialog" ) );

	wxString description = descriptionCtrl->GetValue();

	bool unsavedChangesWarningShown = false;

	if( description.Length() > 1 )
	{
		// Grab list of files to be submitted
		size_t count = m_fileList->GetItemCount();

		vector< SolutionFilePtr > files;
		files.reserve( count );

		for( size_t i = 0; i < count; ++i )
		{
			CheckInData& data = m_fileData[ m_fileList->GetItemData( i ) ];

			if( data.enabled )
			{
				if( !unsavedChangesWarningShown && data.file->IsModified() )
				{
					if( wxMessageBox( wxT( "Some of the files have unsaved changes. These files will be saved before you submit" ), wxT( "Modified files detected" ), wxOK | wxCANCEL | wxICON_WARNING ) != wxOK )
					{
						Close();
						return;
					}
					unsavedChangesWarningShown = true;
				}

				files.push_back( data.file );
			}
		}

		// Send event
		CCheckInEvent* event = new CCheckInEvent( description, files );
		QueueEvent( event );

		// Close window
		Close();
	}
	else
	{
		wxMessageBox( wxT( "You must write a description about the changes you are submitting" ), wxT( "Description Required" ), wxOK | wxICON_ERROR );
	}
}
