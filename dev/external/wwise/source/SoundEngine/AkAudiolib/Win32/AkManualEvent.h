/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/
#pragma once
#include <AK/Tools/Common/AkAssert.h>
#include <AK/SoundEngine/Common/AkTypes.h>

class AkManualEvent
{
public:
	AkManualEvent() : m_Event(NULL)
	{
	}

	~AkManualEvent()
	{
		if(m_Event)
			::CloseHandle(m_Event);
	}

	inline AKRESULT Init()
	{
#ifdef AK_USE_METRO_API
		m_Event = ::CreateEventEx( nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, STANDARD_RIGHTS_ALL|EVENT_MODIFY_STATE );
#else
		m_Event = ::CreateEvent( NULL,		// No security attributes
			true,							// Reset type: manual
			false,							// Initial signaled state: not signaled
			NULL);							// No name
#endif
		return m_Event ? AK_Success : AK_Fail;
	}

	inline void Wait()
	{
#ifdef AK_USE_METRO_API
		AKVERIFY( ::WaitForSingleObjectEx( m_Event, INFINITE, FALSE ) == WAIT_OBJECT_0 );
#else
		AKVERIFY( ::WaitForSingleObject( m_Event, INFINITE ) == WAIT_OBJECT_0 );
#endif
	}

	inline void Signal()
	{
		AKVERIFY( ::SetEvent( m_Event ) );
	}

	inline void Reset()
	{
		AKVERIFY( ::ResetEvent( m_Event ) );
	}

private:
	HANDLE m_Event;
};