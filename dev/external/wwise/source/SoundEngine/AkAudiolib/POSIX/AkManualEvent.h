/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version:  Build: 
  Copyright (c) 2006-2019 Audiokinetic Inc.
 ***********************************************************************/

#pragma once
#include <AK/Tools/Common/AkAssert.h>

#include <pthread.h>

class AkManualEvent
{
public:
	AkManualEvent()
	{
		pthread_mutex_init(&m_mutex, NULL);
		pthread_cond_init(&m_condition, NULL);
	}

	~AkManualEvent()
	{
		pthread_cond_destroy(&m_condition);
		pthread_mutex_destroy(&m_mutex);
	}

	inline AKRESULT Init()
	{
		m_bSignaled = false;	// Initially not signaled.
		return AK_Success;
	}

	inline void Wait()
	{
		pthread_mutex_lock(&m_mutex);
		if(!m_bSignaled)
		{
			pthread_cond_wait(&m_condition,&m_mutex);
		}
		pthread_mutex_unlock(&m_mutex);
	}

	inline void Signal()
	{
		pthread_mutex_lock(&m_mutex);
		m_bSignaled = true;
		pthread_cond_broadcast(&m_condition);
		pthread_mutex_unlock(&m_mutex);
	}

	inline void Reset()
	{
		pthread_mutex_lock(&m_mutex);
		m_bSignaled = false;
		pthread_mutex_unlock(&m_mutex);
	}

private:
	pthread_mutex_t m_mutex;
	pthread_cond_t m_condition;
	bool m_bSignaled;
};