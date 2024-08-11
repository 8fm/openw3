/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "changesDetectedDialog.h"

#include "documentView.h"
#include "frame.h"

#include "solution/slnDeclarations.h"
#include "solution/file.h"

CDetectOfflineChanges::CDetectOfflineChanges( vector< CSSDocument* >& openDocuments )
{
	FilterDocuments( openDocuments );
}

CDetectOfflineChanges::~CDetectOfflineChanges()
{

}

void CDetectOfflineChanges::FilterDocuments( vector< CSSDocument* >& documents )
{
	const size_t size = documents.size();
	for( size_t i = 0; i < size; ++i )
	{
		CSSDocument* doc = documents[ i ];
		const SolutionFilePtr& file = doc->GetFile();

		if( !file->IgnoreChanges() )
		{
			file->RefreshStatus();

			if( file->IsDeleted() )
			{
				m_sourceControlDeleted.push_back( doc );
			}
			else if( !file->ExistsOnDisk() )
			{
				m_offlineDeleted.push_back( doc );
			}
			else if( !file->CheckModificationTime() )
			{
				m_offlineChanges.push_back( doc );
			}
		}
	}
}

void CDetectOfflineChanges::DisplayDialog( wxWindow* parent )
{
	if( HasOfflineChanges() )
	{
		CSSChangesDetectedDialog* dialog = new CSSChangesDetectedDialog( parent );

		if( !m_offlineChanges.empty() )
		{
			for( size_t i = 0; i < m_offlineChanges.size(); ++i )
			{
				m_actionMap[ m_offlineChanges[ i ] ] = A_Reload;
			}

			wxEvtHandler* eventHandler = dialog->CreateSection( m_offlineChanges, wxT( "Files with offline changes" ), wxT( "Selected files will be reloaded (Unsaved changes will be lost)" ), CSSCheckListCtrl::Checked_On );
			eventHandler->Bind( ssEVT_ALTERED_FILES_STATE_CHANGE_EVENT, &CDetectOfflineChanges::OnOfflineChangeStateChange, this );
		}

		if( !m_offlineDeleted.empty() )
		{
			for( size_t i = 0; i < m_offlineDeleted.size(); ++i )
			{
				m_actionMap[ m_offlineDeleted[ i ] ] = A_Close;
			}

			wxEvtHandler* eventHandler = dialog->CreateSection( m_offlineDeleted, wxT( "Files deleted offline" ), wxT( "These files were deleted outside of script studio, would you like to keep them open in memory?" ), CSSCheckListCtrl::Checked_Off );
			eventHandler->Bind( ssEVT_ALTERED_FILES_STATE_CHANGE_EVENT, &CDetectOfflineChanges::OnOfflineDeleteStateChange, this );
		}

		if( !m_sourceControlDeleted.empty() )
		{
			for( size_t i = 0; i < m_sourceControlDeleted.size(); ++i )
			{
				m_actionMap[ m_sourceControlDeleted[ i ] ] = A_Close;
			}

			dialog->CreateSection( m_sourceControlDeleted, wxT( "Files deleted by source control" ), wxT( "These files were deleted by source control, would you like to keep them open in memory?" ), CSSCheckListCtrl::Checked_Off );
		}

		dialog->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CDetectOfflineChanges::OnDialogClose, this, wxID_OK );
		
		dialog->ShowModal();

		dialog->Close();
	}
}

void CDetectOfflineChanges::OnOfflineChangeStateChange( CAlteredFilesListStateChangeEvent& event )
{
	if( event.GetState() == CSSAlteredFilesList::Checked_On )
	{
		m_actionMap[ event.GetDocument() ] = A_Reload;
	}
	else
	{
		m_actionMap[ event.GetDocument() ] = A_Ignore;
	}
}

void CDetectOfflineChanges::OnOfflineDeleteStateChange( CAlteredFilesListStateChangeEvent& event )
{
	if( event.GetState() == CSSAlteredFilesList::Checked_On )
	{
		m_actionMap[ event.GetDocument() ] = A_Ignore;
	}
	else
	{
		m_actionMap[ event.GetDocument() ] = A_Close;
	}
}

void CDetectOfflineChanges::OnDialogClose( wxCommandEvent& event )
{
	PerformFileActions();

	event.Skip();
}

void CDetectOfflineChanges::PerformFileActions()
{
	for( auto iter : m_actionMap )
	{
		switch( iter.second )
		{
		case A_Reload:
			iter.first->LoadFile();
			break;

		case A_Close:
			wxTheFrame->CloseFile( iter.first->GetFile(), true );
			break;

		case A_Ignore:
			iter.first->GetFile()->SetIgnoreChanges( true );
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CAlteredFilesListStateChangeEvent::CAlteredFilesListStateChangeEvent( CSSDocument* document, CSSCheckListCtrl::EChecked state )
:	wxEvent( wxID_ANY, ssEVT_ALTERED_FILES_STATE_CHANGE_EVENT )
,	m_document( document )
,	m_state( state )
{

}

CAlteredFilesListStateChangeEvent::CAlteredFilesListStateChangeEvent( wxEventType commandType, int winid )
:	wxEvent( commandType, winid )
{

}

CAlteredFilesListStateChangeEvent::~CAlteredFilesListStateChangeEvent()
{
}

wxEvent* CAlteredFilesListStateChangeEvent::Clone() const  
{
	return new CAlteredFilesListStateChangeEvent( m_document, m_state );
}

wxDEFINE_EVENT( ssEVT_ALTERED_FILES_STATE_CHANGE_EVENT, CAlteredFilesListStateChangeEvent );
wxIMPLEMENT_DYNAMIC_CLASS( CAlteredFilesListStateChangeEvent, wxEvent );

//////////////////////////////////////////////////////////////////////////

wxIMPLEMENT_CLASS( CSSAlteredFilesList, CSSCheckListCtrl );

CSSAlteredFilesList::CSSAlteredFilesList( wxWindow* parent )
:	CSSCheckListCtrl( parent, wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_NO_HEADER )
{

}

CSSAlteredFilesList::~CSSAlteredFilesList()
{

}

void CSSAlteredFilesList::Add( CSSDocument* document, EChecked state )
{
	int item = InsertItem( GetItemCount(), state );

	const SolutionFilePtr& file = document->GetFile();
	SetItem( item, Col_Path, file->m_solutionPath.c_str() );
	SetItemData( item, reinterpret_cast< wxUIntPtr >( document ) );
}

void CSSAlteredFilesList::OnStateChange( int itemIndex, EChecked state )
{
	CSSDocument* document = reinterpret_cast < CSSDocument* >( GetItemData( itemIndex ) );

	wxEvent* eventToSend = new CAlteredFilesListStateChangeEvent( document, state );
	QueueEvent( eventToSend );
}

//////////////////////////////////////////////////////////////////////////

wxIMPLEMENT_CLASS( CSSChangesDetectedDialog, wxDialog );

CSSChangesDetectedDialog::CSSChangesDetectedDialog( wxWindow* parent )
:	wxDialog( parent, wxID_ANY, wxT( "Offline changes" ), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxSYSTEM_MENU | wxRESIZE_BORDER | wxCLIP_CHILDREN | wxCLOSE_BOX )
{
	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	SetSizer( sizer );

	m_sectionSizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( m_sectionSizer, 1, wxEXPAND | wxALL, 5 );

	wxSizer* okButtons = CreateButtonSizer( wxOK );
	sizer->Add( okButtons, 0, wxEXPAND | wxALL, 5 );
}

CSSChangesDetectedDialog::~CSSChangesDetectedDialog()
{

}

wxEvtHandler* CSSChangesDetectedDialog::CreateSection( vector< CSSDocument* >& documents, wxString title, wxString description, CSSCheckListCtrl::EChecked defaultState )
{
	wxStaticBoxSizer* section = new wxStaticBoxSizer( wxVERTICAL, this, title );

	CSSAlteredFilesList* listCtrl = new CSSAlteredFilesList( this );

	listCtrl->InsertColumn( 1, wxT( "Path" ) );

	const size_t size = documents.size();
	for( size_t i = 0; i < size; ++i )
	{
		CSSDocument* doc = documents[ i ];

		listCtrl->Add( doc, defaultState );
	}

	listCtrl->SetColumnWidth( CSSAlteredFilesList::Col_Checkbox, wxLIST_AUTOSIZE );
	listCtrl->SetColumnWidth( CSSAlteredFilesList::Col_Path, wxLIST_AUTOSIZE );

	section->Add( listCtrl, 1, wxEXPAND | wxALL | wxFIXED_MINSIZE, 5 );

	wxStaticText* label = new wxStaticText( this, wxID_ANY, description );
	section->Add( label, 0, wxEXPAND | wxALL, 5 );

	m_labels.push_back( label );
	m_descriptions.push_back( description );

	m_sectionSizer->Add( section, 1, wxEXPAND | wxALL, 5 );

	Layout();
	Fit();

	return listCtrl;
}
