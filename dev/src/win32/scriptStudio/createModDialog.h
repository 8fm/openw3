/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SS_CREATE_SOLUTION_DIALOG_H__
#define __SS_CREATE_SOLUTION_DIALOG_H__

#include "solution/slnDeclarations.h"

#include "commonBaseDialog.h"

class wxFilePickerCtrl;

class CSSCreateModDialog : public CSSCommonBaseDialog
{
	wxDECLARE_CLASS( CSSCreateModDialog );

public:
	CSSCreateModDialog( wxWindow* parent );
	virtual ~CSSCreateModDialog();

	wxString GetName() const;
	wxString GetWorkspacePath() const;
	wxString GetInstallPath() const;

	wxFileName GetInstalledScriptsPath() const;

private:
	template< typename Type > Type* GetControl( const Red::System::AnsiChar* name ) const;
	template< typename Type > wxTextCtrl* GetTextControl( const Red::System::AnsiChar* name );
	wxString GetControlValue( const wxTextCtrl* ctrl ) const;
	wxString GetControlValue( const wxFilePickerCtrl* ctrl ) const;
	template< typename Type > bool ValidateControl( const Red::System::AnsiChar* name );

	void HighlightControl( wxTextCtrl* ctrl, bool hasError ) const;
	void OnOkClicked( wxCommandEvent& event );

	static const wxColour ERROR_HIGHLIGHT;
};

#endif // __SS_CREATE_SOLUTION_DIALOG_H__
