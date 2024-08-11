/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _SS_BREAKPOINTS_CTRL_H_
#define _SS_BREAKPOINTS_CTRL_H_

#include "widgets/markers.h"

#include "solution/slnDeclarations.h"

/// Breakpoints panel
class CSSBreakpoints: public CSSMarkers
{
	wxDECLARE_CLASS( CSSBreakpoints );

public:
	CSSBreakpoints( wxWindow* parent, Solution* solution );
	virtual ~CSSBreakpoints();

	void ConfirmAll();

private:
	virtual CMarkerToggledEvent* CreateToggleEvent( const SolutionFilePtr& file, Red::System::Int32 line, EMarkerState state ) const override final;
};

#endif // _SS_BREAKPOINTS_CTRL_H_
