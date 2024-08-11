/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SCRIPT_STUDIO_COMPILE_MOD_DIALOG_H__
#define __SCRIPT_STUDIO_COMPILE_MOD_DIALOG_H__

#include "commonBaseDialog.h"

enum class EModCompilationAction
{
	UseExisting,
	Install,
	UseWorkspace,
	Invalid,
};

class CSSCompileModDialog : public CSSCommonBaseDialog
{
	wxDECLARE_CLASS( CSSCompileModDialog );

public:
	CSSCompileModDialog( wxWindow* parent );
	virtual ~CSSCompileModDialog();

	EModCompilationAction GetAction() const;
	bool GetRememberAction() const;

private:
};

#endif // __SCRIPT_STUDIO_COMPILE_MOD_DIALOG_H__
