/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "installModDialog.h"

#include "wx/filepicker.h"

wxIMPLEMENT_CLASS( CSSInstallModDialog, CSSCommonBaseDialog );

CSSInstallModDialog::CSSInstallModDialog( wxWindow* parent )
:	CSSCommonBaseDialog( parent, wxT( "installModDialog" ) )
{
}

CSSInstallModDialog::~CSSInstallModDialog()
{

}

void CSSInstallModDialog::SetPath( const wxString& path )
{
	wxDirPickerCtrl* picker = XRCCTRL( *this, "outputDirectory", wxDirPickerCtrl );

	picker->SetPath( path );
}

wxString CSSInstallModDialog::GetPath() const
{
	wxDirPickerCtrl* picker = XRCCTRL( *this, "outputDirectory", wxDirPickerCtrl );

	return picker->GetPath();
}
