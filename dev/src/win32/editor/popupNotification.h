/**
  * Copyright © 2013 CD Projekt Red. All Rights Reserved.
  */
#pragma once
#include "wx/generic/stattextg.h"

class CEdPopupNotification : private wxTopLevelWindow
{
public:
	enum Location
	{
		CENTER = 0, TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT
	};

	void Show( wxWindow* parent, const String& title, const String& message );

	void SetLocation( Location pos );
	Location GetLocation() const;

	void LoadOptionsFromConfig();
    void SaveOptionsToConfig();

private:
	using wxTopLevelWindow::Show; // import base method
	wxTimer m_waitTimer;
	wxTimer m_fadeOutTimer;
	wxStopWatch m_stopWatch;

	wxStaticText* m_titleLabel;
	wxStaticText* m_messageLabel;

	Location m_location;

	wxPoint CalculatePosition( wxWindow* parent ) const;
	void OnTimer( wxTimerEvent& evt );
	WXLRESULT MSWWindowProc( WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam );

	friend struct TCreateUsingNew< CEdPopupNotification >;
	CEdPopupNotification();
};

// Since this object will get deleted by wxWidgets when it shuts down, we have to create the singleton with TNoDestructionLifetime, otherwise it tries to delete twice!
typedef TSingleton< CEdPopupNotification, TNoDestructionLifetime, TCreateUsingNew > 
	SEdPopupNotification;
