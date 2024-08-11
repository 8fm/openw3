/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "createModDialog.h"
#include "wx/filepicker.h"

wxIMPLEMENT_CLASS( CSSCreateModDialog, CSSCommonBaseDialog );

const wxColour CSSCreateModDialog::ERROR_HIGHLIGHT( 255, 128, 128 );

CSSCreateModDialog::CSSCreateModDialog( wxWindow* parent )
:	CSSCommonBaseDialog( parent, wxT( "createNewModDialog" ) )
{
	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CSSCreateModDialog::OnOkClicked, this, wxID_OK );
}

CSSCreateModDialog::~CSSCreateModDialog()
{

}

template< typename Type >
Type* CSSCreateModDialog::GetControl( const Red::System::AnsiChar* name ) const
{
	Type* ctrl = XRCCTRL( *this, name, Type );
	RED_FATAL_ASSERT( ctrl, "Missing control: %hs", name );
	return ctrl;
}

template<>
wxTextCtrl* CSSCreateModDialog::GetTextControl< wxTextCtrl >( const Red::System::AnsiChar* name )
{
	return GetControl< wxTextCtrl >( name );
}

template<>
wxTextCtrl* CSSCreateModDialog::GetTextControl< wxFilePickerCtrl >( const Red::System::AnsiChar* name )
{
	wxFilePickerCtrl* ctrl = GetControl< wxFilePickerCtrl >( name );
	return ctrl->GetTextCtrl();
}

wxString CSSCreateModDialog::GetControlValue( const wxTextCtrl* ctrl ) const
{
	return ctrl->GetValue();
}

wxString CSSCreateModDialog::GetControlValue( const wxFilePickerCtrl* ctrl ) const
{
	return ctrl->GetPath();
}

template< typename Type >
bool CSSCreateModDialog::ValidateControl( const Red::System::AnsiChar* name )
{
	wxTextCtrl* ctrl = GetTextControl< Type >( name );
	wxString value = GetControlValue( ctrl );

	bool isEmpty = value.IsEmpty();

	HighlightControl( ctrl, isEmpty );

	return !isEmpty;
}

void CSSCreateModDialog::HighlightControl( wxTextCtrl* ctrl, bool hasError ) const
{
	if( hasError )
	{
		ctrl->SetBackgroundColour( ERROR_HIGHLIGHT );
	}
	else
	{
		wxColour colour = ctrl->GetClassDefaultAttributes().colBg;
		ctrl->SetBackgroundColour( colour );
	}

	ctrl->Refresh();
}

void CSSCreateModDialog::OnOkClicked( wxCommandEvent& event )
{
	bool success = true;

	success &= ValidateControl< wxTextCtrl >( "modName" );
	
	if( ValidateControl< wxFilePickerCtrl >( "gameInstallLocation" ) )
	{
		wxFileName path = GetInstalledScriptsPath();
		if( !path.DirExists() )
		{
			wxTextCtrl* ctrl = GetTextControl< wxFilePickerCtrl >( "gameInstallLocation" );
			HighlightControl( ctrl, true );

			wxMessageBox( wxT( "Could not finds scripts in specified game install folder" ), wxT( "Scripts missing" ), wxOK | wxICON_ERROR );
			success = false;
		}
	}
	else
	{
		success = false;
	}

	if( ValidateControl< wxFilePickerCtrl >( "workspaceLocationPicker" ) )
	{
		wxFilePickerCtrl* ctrl = GetControl< wxFilePickerCtrl >( "workspaceLocationPicker" );
		wxFileName workspacePath( ctrl->GetPath() );

		if( !workspacePath.IsOk() )
		{
			success = false;
		}
	}
	else
	{
		success = false;
	}

	if( success )
	{
		event.Skip();
	}
}

wxFileName CSSCreateModDialog::GetInstalledScriptsPath() const
{
	wxString install = GetInstallPath();

	wxFileName installSourcePath( install + wxFileName::GetPathSeparator() );
	installSourcePath.AppendDir( wxT( "content" ) );
	installSourcePath.AppendDir( wxT( "content0" ) );
	installSourcePath.AppendDir( wxT( "scripts" ) );

	return installSourcePath;
}

wxString CSSCreateModDialog::GetName() const
{
	wxTextCtrl* ctrl = GetControl< wxTextCtrl >( "modName" );
	return GetControlValue( ctrl );
}

wxString CSSCreateModDialog::GetWorkspacePath() const
{
	wxFilePickerCtrl* ctrl = GetControl< wxFilePickerCtrl >( "workspaceLocationPicker" );
	return GetControlValue( ctrl );
}

wxString CSSCreateModDialog::GetInstallPath() const
{
	wxFilePickerCtrl* ctrl = GetControl< wxFilePickerCtrl >( "gameInstallLocation" );
	return GetControlValue( ctrl );
}
