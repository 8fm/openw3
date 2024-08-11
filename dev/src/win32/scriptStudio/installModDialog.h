/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SCRIPT_STUDIO_EXPORT_SOLUTION_DIALOG_H__
#define __SCRIPT_STUDIO_EXPORT_SOLUTION_DIALOG_H__

#include "commonBaseDialog.h"

class CSSInstallModDialog : public CSSCommonBaseDialog
{
	wxDECLARE_CLASS( CSSInstallModDialog );

public:
	CSSInstallModDialog( wxWindow* parent );
	virtual ~CSSInstallModDialog();

	void SetPath( const wxString& path );

	wxString GetPath() const;
};

#endif //__SCRIPT_STUDIO_EXPORT_SOLUTION_DIALOG_H__
