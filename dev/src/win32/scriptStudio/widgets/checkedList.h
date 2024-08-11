/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _SS_CHECK_LIST_CTRL_H_
#define _SS_CHECK_LIST_CTRL_H_

class CSSCheckListCtrl: public wxListCtrl
{
	wxDECLARE_CLASS( CSSCheckListCtrl );

public:
	enum EChecked
	{
		Checked_On = 0,
		Checked_Off,

		Checked_Max
	};

	static const unsigned int CHECKBOX_ICON_SIZE = 16;

	CSSCheckListCtrl( wxWindow* parent, int flags );
	virtual ~CSSCheckListCtrl();

protected:
	virtual void OnStateChange( int itemIndex, EChecked state ) = 0;

private:
	void OnLeftDown( wxMouseEvent& event );
};

#endif // _SS_CHECK_LIST_CTRL_H_
