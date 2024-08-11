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

/***************************************************************************************************
**
** AkProfile.h
**
***************************************************************************************************/
#ifndef _PROFILE_H_
#define _PROFILE_H_

#include "AkCommon.h"
#include "AkMonitorData.h"
#include <AK/Tools/Common/AkObject.h>
#include <AK/Tools/Common/AkLock.h>
#include "AudiolibDefs.h"     // Pool IDs definition.

//-----------------------------------------------------------------------------
// Defines.
//-----------------------------------------------------------------------------

// AK_PROFILE_PERF_INTERVALS in milliseconds
#define AK_PROFILE_PERF_INTERVALS 200
#define AK_PROFILE_CURSOR_UPDATE_INTERVALS 50

class AkPerf
{
public:
	static void Init();
	static void Term();

	static void TickAudio();

	static void IncrementPrepareCount(){ ++m_ulPreparedEvents; }
	static void DecrementPrepareCount(){ --m_ulPreparedEvents; }

	static void IncrementBankMemory( AkUInt32 in_Size ) { m_ulBankMemory += in_Size; }
	static void DecrementBankMemory( AkUInt32 in_Size ) { m_ulBankMemory -= in_Size; }
	static void IncrementPreparedMemory( AkUInt32 in_Size ) { m_ulPreparedEventMemory += in_Size; }
	static void DecrementPreparedMemory( AkUInt32 in_Size ) { m_ulPreparedEventMemory -= in_Size; }


private:
	static void PostPluginTimers( AkInt64 in_iNow );
	static void PostMemoryStats( AkInt64 in_iNow );
    static void PostStreamingStats( AkInt64 in_iNow );
    static void PostPipelineStats( AkInt64 in_iNow );
	static void PostEnvironmentStats();
	static void PostSendsStats();
	static void PostObsOccStats();
	static void PostListenerStats();
	static void PostControllerStats();
	static void PostOutputStats();
	static void PostGameObjPositions();
	static void PostWatchGameSyncValues();
	static void PostInteractiveMusicInfo();
#ifdef AK_MOTION
	static void PostFeedbackStats(AkReal32 in_fInterval);
	static void PostFeedbackDevicesStats();
#endif // AK_MOTION
	static AkInt64 m_iLastUpdateAudio;			// Last update (QueryPerformanceCounter value)
	static AkInt64 m_iLastUpdateCursorPosition;	// Last update (QueryPerformanceCounter value)

	static AkInt64 m_iLastUpdatePlugins;
	static AkInt64 m_iLastUpdateMemory;
    static AkInt64 m_iLastUpdateStreaming;

	static AkUInt32 m_ulPreparedEvents;
	static AkUInt32 m_ulBankMemory;
	static AkUInt32 m_ulPreparedEventMemory;

	static AkInt32 m_iTicksPerPerfBlock;
	static AkInt32 m_iNextPerf;
	static AkInt32 m_iTicksPerCursor;
	static AkInt32 m_iNextCursor;

};

#if !defined(AK_OPTIMIZED)

// globals controlling offline rendering. modified directly by game simulator.
extern bool	g_bOfflineRendering;
extern AkChannelMask g_eOfflineSpeakerConfig;

#define AK_PERF_TICK_AUDIO()					AkPerf::TickAudio()
#define AK_PERF_INTERVAL()						AK_PROFILE_PERF_INTERVALS
#define AK_PERF_INIT()							AkPerf::Init()
#define AK_PERF_TERM()							AkPerf::Term()
#define AK_PERF_INCREMENT_PREPARE_EVENT_COUNT()		AkPerf::IncrementPrepareCount()
#define AK_PERF_DECREMENT_PREPARE_EVENT_COUNT()		AkPerf::DecrementPrepareCount()
#define AK_PERF_INCREMENT_BANK_MEMORY( _size_ )		AkPerf::IncrementBankMemory( _size_ ) 
#define AK_PERF_DECREMENT_BANK_MEMORY( _size_ )		AkPerf::DecrementBankMemory( _size_ )
#define AK_PERF_INCREMENT_PREPARED_MEMORY( _size_ )	AkPerf::IncrementPreparedMemory( _size_ )
#define AK_PERF_DECREMENT_PREPARED_MEMORY( _size_ )	AkPerf::DecrementPreparedMemory( _size_ )
#define AK_PERF_OFFLINE_RENDERING				(g_bOfflineRendering)
#define AK_PERF_OFFLINE_SPEAKERCONFIG			(g_eOfflineSpeakerConfig)

#else

#define AK_PERF_TICK_AUDIO()
#define AK_PERF_INTERVAL()						INFINITE
#define AK_PERF_INIT()
#define AK_PERF_TERM()
#define AK_PERF_INCREMENT_PREPARE_EVENT_COUNT()
#define AK_PERF_DECREMENT_PREPARE_EVENT_COUNT()
#define AK_PERF_INCREMENT_BANK_MEMORY(_size_)	
#define AK_PERF_DECREMENT_BANK_MEMORY(_size_)	
#define AK_PERF_INCREMENT_PREPARED_MEMORY(_size_)
#define AK_PERF_DECREMENT_PREPARED_MEMORY(_size_)
#define AK_PERF_OFFLINE_RENDERING				(false)
#define AK_PERF_OFFLINE_SPEAKERCONFIG			(AK_SPEAKER_SETUP_STEREO)

#endif

#endif
