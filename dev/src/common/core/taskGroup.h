/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

//////////////////////////////////////////////////////////////////////////
// ETaskSchedulerGroup
//////////////////////////////////////////////////////////////////////////
enum ETaskSchedulerGroup
{
	TSG_Normal,
#ifdef RED_PLATFORM_ORBIS
	TSG_Service, // don't create an alias like TSG_Service = TSG_Normal otherwise, because priority may not match expectation.
#endif
	TSG_Count,
};

//////////////////////////////////////////////////////////////////////////
// STaskGroupInitParams
//////////////////////////////////////////////////////////////////////////
struct STaskGroupInitParams
{
	typedef Red::Threads::EThreadPriority	EThreadPriority;
	typedef Red::Threads::TAffinityMask		TAffinityMask;
	typedef Red::Threads::SThreadMemParams	SThreadMemParams;

	struct SThreadParams
	{
		Uint32				m_numTaskThreads;
		EThreadPriority		m_priority;
		SThreadMemParams	m_memParams;
		TAffinityMask*		m_affinityMasks;


		SThreadParams()
			: m_numTaskThreads( 0 )
			, m_priority( EThreadPriority::TP_Normal )
			, m_memParams()
			, m_affinityMasks( nullptr )
		{}
	};

	SThreadParams						m_threadParams[ TSG_Count ];
};
