/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SS_CHECKIN_DIALOG_H__
#define __SS_CHECKIN_DIALOG_H__

#include "app.h"
#include "solution/slnDeclarations.h"

class CCheckInEvent : public wxEvent
{
public:
	CCheckInEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 );
	CCheckInEvent( wxString description, const vector< SolutionFilePtr >& filesToSubmit );
	inline ~CCheckInEvent() {}

	inline const wxString& GetDescription() const { return m_description; }
	inline const vector< SolutionFilePtr >& GetFiles() const { return m_filesToSubmit; }

private:
	virtual wxEvent* Clone() const override final { return new CCheckInEvent( m_description, m_filesToSubmit ); }

private:
	wxString m_description;
	vector< SolutionFilePtr > m_filesToSubmit;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( CCheckInEvent );
};

wxDECLARE_EVENT( ssEVT_CHECKIN_EVENT, CCheckInEvent );

//////////////////////////////////////////////////////////////////////////

class CSSCheckinDialog : public wxDialog
{
	wxDECLARE_CLASS( CSSCheckinDialog );

private:
	enum EChecked
	{
		Checked_On = 0,
		Checked_Off,

		Checked_Max
	};

	enum EColumn
	{
		Col_Enabled = 0,
		Col_Icon,
		Col_File,

		Col_Max
	};

public:
	CSSCheckinDialog( wxWindow* parent, wxString description = wxEmptyString );
	~CSSCheckinDialog();

	void AddFile( const SolutionFilePtr& file, ESolutionImage icon );

private:
	void OnMouseEvent( wxMouseEvent& event );
	void OnOkClicked( wxCommandEvent& );

private:
	wxListCtrl* m_fileList;

	struct CheckInData
	{
		SolutionFilePtr file;
		bool enabled;
	};

	vector< CheckInData > m_fileData;
};

#endif // __SS_CHECKIN_DIALOG_H__
