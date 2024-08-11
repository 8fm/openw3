/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _SS_EVENT_NAVIGATION_GOTO_H_
#define _SS_EVENT_NAVIGATION_GOTO_H_

class CNavigationGotoEvent : public wxEvent
{
public:
	CNavigationGotoEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 );
	CNavigationGotoEvent( Red::System::Bool forwards );

	virtual ~CNavigationGotoEvent();

	inline Red::System::Bool IsForwards() const { return m_forwards; }

private:
	virtual wxEvent* Clone() const override final { return new CNavigationGotoEvent( m_forwards ); }

private:
	Red::System::Bool m_forwards;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( CNavigationGotoEvent );
};

wxDECLARE_EVENT( ssEVT_NAVIGATION_GOTO_EVENT, CNavigationGotoEvent );

#endif // _SS_EVENT_NAVIGATION_GOTO_H_
