/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SCRIPT_STUDIO_COMMON_BASE_DIALOG_H__
#define __SCRIPT_STUDIO_COMMON_BASE_DIALOG_H__

class CSSCommonBaseDialog : public wxDialog
{
	wxDECLARE_CLASS( CSSCommonBaseDialog );

public:
	CSSCommonBaseDialog( wxWindow* parent, const wxString& xrcName );
	virtual ~CSSCommonBaseDialog();

	RED_INLINE bool GetCancelled() const { return m_cancelled; }

private:
	void OnCancel( wxCommandEvent& event );

	bool m_cancelled;
};

#endif // __SCRIPT_STUDIO_COMMON_BASE_DIALOG_H__
