/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class IAIQueueMetalinkInterface
{
protected:
	static const EngineTime RECALCULATION_MIN_DELAY;
	struct Member
	{
		THandle< CActor >				m_guy;
		Float							m_priority;
		Bool operator<( const Member& m ) const									{ return m_priority < m.m_priority; }
	};
	TSortedArray< Member >			m_registered;
	EngineTime						m_nextPriorityCalculation;
	Bool							m_locked;

	void		LazyCalculatePriorities();
public:
	IAIQueueMetalinkInterface();
	~IAIQueueMetalinkInterface();

	void		Register( CActor* actor, Float priority );
	void		Unregister( CActor* actor );

	void		Lock( Bool b );

	Bool		CanIGo( CActor* actor );

	static CName AIPriorityEventName();
	static CName AIReleaseLockEventName();
};