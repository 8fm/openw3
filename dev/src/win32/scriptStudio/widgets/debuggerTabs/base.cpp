/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "base.h"

wxIMPLEMENT_CLASS( CSSFloatingTab, wxPanel );

CSSFloatingTab::CSSFloatingTab( wxAuiNotebook* parent )
:	wxPanel( parent )
{

}

CSSFloatingTab::~CSSFloatingTab()
{

}

wxIMPLEMENT_CLASS( CSSDebuggerTabBase, CSSFloatingTab );

CSSDebuggerTabBase::CSSDebuggerTabBase( wxAuiNotebook* parent )
:	CSSFloatingTab( parent )
{

}

CSSDebuggerTabBase::~CSSDebuggerTabBase()
{

}

bool CSSDebuggerTabBase::Paste( const wxString& text )
{
	return false;
}
