/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/* Non-reentrant wxTimer replacement, when subclassing override
 * the NotifyOnce class instead of Notify */
class CEdTimer : public wxTimer
{
public:
	CEdTimer()
	:	wxTimer(),
		m_entrances( 0 )
	{
	}

	CEdTimer( wxEvtHandler* owner, int timerid = wxID_ANY )
	:	wxTimer( owner, timerid ),
		m_entrances( 0 )
	{
	}
	
	// Override this instead of Notify
	virtual void NotifyOnce()
	{
		/* Send the timer event */
		wxTimer::Notify();
	}

	virtual void Notify()
	{
		m_entrances++;

		if ( m_entrances != 1 )
		{
			RED_LOG( Editor, wxString::Format( wxT(" timer reentrancy error: %d entrances "), m_entrances ).wc_str() );
		}

		if ( m_entrances == 1 )
		{
			NotifyOnce();
		}

		m_entrances--;
	}

private:
	int m_entrances;
};
