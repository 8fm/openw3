/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "compileModDialog.h"

wxIMPLEMENT_CLASS( CSSCompileModDialog, CSSCommonBaseDialog );

CSSCompileModDialog::CSSCompileModDialog( wxWindow* parent )
:	CSSCommonBaseDialog( parent, wxT( "compileModDialog" ) )
{

}

CSSCompileModDialog::~CSSCompileModDialog()
{

}

EModCompilationAction CSSCompileModDialog::GetAction() const
{
	wxRadioBox* actionCtrl = XRCCTRL( *this, "compilationAction", wxRadioBox );
	
	int selection = actionCtrl->GetSelection();

	// These numbers match up to the order of the items as defined in the formbuilder project
	switch( selection )
	{
	case 0:
		return EModCompilationAction::UseExisting;
		
	case 1:
		return EModCompilationAction::Install;

	case 2:
		return EModCompilationAction::UseWorkspace;
	}

	return EModCompilationAction::Invalid;
}

bool CSSCompileModDialog::GetRememberAction() const
{
	wxCheckBox* saveCtrl = XRCCTRL( *this, "saveChoice", wxCheckBox );

	return saveCtrl->IsChecked();
}
