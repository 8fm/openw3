/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

struct CSyncInfo
{
public:
	Float								m_prevTime;
	Float								m_currTime;
	Float								m_totalTime;
	TDynArray< CExtAnimEvent* >			m_syncEvents;
	bool								m_wantSyncEvents; // set to true if you want m_syncEvents updated

	CSyncInfo() : m_prevTime( 0.f ), m_currTime( 0.0f ) , m_totalTime( 0.0f ), m_wantSyncEvents( true ) 
	{
		m_syncEvents.Reserve( 16 );
	}

	void SetInterpolate( const CSyncInfo &lhs, const CSyncInfo &rhs, Float alpha )
	{	
		m_prevTime	= ( 1.0f - alpha ) * lhs.m_prevTime + alpha * rhs.m_prevTime;
		m_currTime	= ( 1.0f - alpha ) * lhs.m_currTime + alpha * rhs.m_currTime;
		m_totalTime	= ( 1.0f - alpha ) * lhs.m_totalTime + alpha * rhs.m_totalTime;

		if ( m_wantSyncEvents )
		{
			if ( alpha > 0.f && alpha < 1.f )
			{
				m_syncEvents.PushBack( lhs.m_syncEvents );
				m_syncEvents.PushBack( rhs.m_syncEvents );
			}
			else if ( alpha > 0.f )
			{
				m_syncEvents.PushBack( rhs.m_syncEvents );
			}
			else
			{
				m_syncEvents.PushBack( lhs.m_syncEvents );
			}
		}
	}
};
