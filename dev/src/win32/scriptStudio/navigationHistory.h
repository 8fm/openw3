/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __SS_NAVIGATION_HISTORY_H_
#define __SS_NAVIGATION_HISTORY_H_

#include "events/eventNavigationHistory.h"

class CSSNavigationHistory : public wxListCtrl
{
	wxDECLARE_CLASS( CSSNavigationHistory );

public:
	CSSNavigationHistory( wxWindow* parent );
	virtual ~CSSNavigationHistory();

	void OnAdd( CNavigationHistoryEvent& event );

	void GotoPrevious();
	void GotoNext();

	bool CanGotoNext() const;
	bool CanGotoPrevious() const;

private:
	void OnItemActivated( wxListEvent& event );
	void Goto( long item );

private:
	enum EColumn
	{
		Col_File = 0,
		Col_Line,
		Col_Snippet,

		Col_Max
	};

	long m_selectedItem;
};

#endif // __SS_NAVIGATION_HISTORY_H_
